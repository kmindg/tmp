/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_mirror_read_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains utility functions required for the mirror read
 *  processing.
 *
 * @version
 *   5/20/2009  Ron Proulx  - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_raid_library.h"
#include "fbe_spare.h"
#include "fbe_mirror_io_private.h"
#include "fbe_raid_fruts.h"

/********************************
 *  FORWARD FUNCTION DEFINITIONS
 ********************************/

/*!***************************************************************************
 *          fbe_mirror_read_setup_fruts()
 *****************************************************************************
 *
 * @brief   This function initializes the fruts structures for mirror read.
 *          The current expectation is that there is exactly (1) position to
 *          read from.
 *
 * @param   siots_p - Pointer to siots to setup fruts for
 * @param   read_info_p - Pointer to array of fruts info that describes I/O
 * @param   requested_opcode - The opcode to populate read fruts with
 * @param   memory_info_p - Pointer to memory request information
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @note
 *
 * HISTORY:
 *      08/22/00  RG  -Created
 *      03/23/01  BJP - Added support for N-way mirrors
 *
 * @author
 *  05/20/2009  Ron Proulx  - Updated from mirror_rd_setup_fruts
 *
 *****************************************************************************/
static fbe_status_t fbe_mirror_read_setup_fruts(fbe_raid_siots_t * siots_p, 
                                                fbe_raid_fru_info_t *read_info_p,
                                                fbe_payload_block_operation_opcode_t requested_opcode,
                                                fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_fru_info_t        *primary_info_p = NULL;
    fbe_u32_t                   primary_position = fbe_mirror_get_primary_position(siots_p);
    fbe_raid_position_bitmask_t primary_bitmask;
    fbe_raid_position_bitmask_t degraded_bitmask;
    fbe_raid_position_bitmask_t read_fruts_bitmask;
    fbe_raid_fruts_t           *read_fruts_p = NULL;

    /* Get the primary fru info pointer.
     */
    status = fbe_mirror_get_primary_fru_info(siots_p, read_info_p, &primary_info_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        /* The read fru info array wasn't setup properly.
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror:fail to get prim fru info:stat=0x%x,siots=0x%p,"
                             "read_info=0x%p\n",
                             status, siots_p, read_info_p); 
        return (status);
    }

    /* The primary position shouldn't be degraded.
     */
    primary_bitmask = (1 << primary_position);
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    if (primary_bitmask & degraded_bitmask)
    {
        /* The primary position cannot be degraded.
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: read setup fruts. primary position: %d is marked degraded - bitmask: 0x%x\n",
                             primary_position, degraded_bitmask); 
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Initialize the one and only read fruts.
     */
    read_fruts_p = fbe_raid_siots_get_next_fruts(siots_p,
                                                 &siots_p->read_fruts_head,
                                                 memory_info_p);
    
    /* If the setup failed something is wrong.
     */
    if (read_fruts_p == NULL)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: read setup fruts siots: 0x%llx 0x%llx for position: %d failed \n",
                             (unsigned long long)siots_p->start_lba,
			     (unsigned long long)siots_p->xfer_count,
			     primary_info_p->position);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Initialize the read fruts from the fru information
     */
    status = fbe_raid_fruts_init_fruts(read_fruts_p,
                                       requested_opcode,
                                       primary_info_p->lba,
                                       primary_info_p->blocks,
                                       primary_info_p->position);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to initialize fruts 0x%p for siots_p 0x%p; status = 0x%x\n",
                             read_fruts_p, siots_p, status);

        return  status;
    }

    /* Now validate that only the primary is on the read chain and validate
     * the fru information (since it was used to generate the request) is
     * as expected.
     */
    fbe_raid_siots_get_read_fruts_bitmask(siots_p, &read_fruts_bitmask);
    if ((primary_bitmask != read_fruts_bitmask)           ||
        (primary_info_p->lba != siots_p->parity_start)    ||
        (primary_info_p->blocks != siots_p->parity_count) ||
        (primary_info_p->position != primary_position)       )
    {
        /* Only the primary position should be on the read chain.
         */
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: read setup fruts primary bitmask: 0x%x read bitmask: 0x%x\n",
                             primary_bitmask, read_fruts_bitmask); 
        return (FBE_STATUS_GENERIC_FAILURE);

    }

    /* Always return the execution status.
     */
    return(status);
}
/* end fbe_mirror_read_setup_fruts() */

/*!***************************************************************************
 *          fbe_mirror_read_remove_degraded_read_fruts()
 *****************************************************************************
 *
 * @brief   This method removes all degraded fruts from the read chain.  It then
 *          places the fruts on the write chain and sets the opcode to `NOP'.
 *          The reason it does this is to keep the total fruts count consistent.
 *          In addition if the read fruts isn't dead, we most likely will want
 *          to update that position with data from a different write fruts.  We
 *          set the opcode to NOP so that anyone wishing to issue a write to the
 *          postion must explicitly set the opcode (this is to handle cases where
 *          the postion is actually dead not degraded).   
 *
 * @param   siots_p - ptr to siots.
 * @param   b_write_degraded - FBE_TRUE - Move any degraded read positions to the
 *                                        write chain and set the opcode to write
 *                             FBE_FALSE - Move any degraded read to write chain
 *                                        and set opcode to NOP.
 *
 * @return  status - FBE_STATUS_OK
 *
 * @note    All `disabled' positions will be set to NOP
 *
 * @author
 *  01/07/2010  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_mirror_read_remove_degraded_read_fruts(fbe_raid_siots_t *siots_p,
                                                        fbe_bool_t b_write_degraded)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_position_bitmask_t degraded_bitmask = 0;
    fbe_raid_position_bitmask_t disabled_bitmask = 0;
    fbe_raid_fruts_t   *read_fruts_p = NULL;

    /* Get the degraded and disabled bitmasks
     */
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    fbe_raid_siots_get_disabled_bitmask(siots_p, &disabled_bitmask);

    /* Walk read fruts chain and remove specified fruts.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    while(read_fruts_p != NULL)
    {
        fbe_raid_fruts_t   *next_read_fruts_p;

        /* In-case we remove it we need the next pointer.
         */
        next_read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);

        /* If this is the position to remove, it.
         */
        if ((1 << read_fruts_p->position) & degraded_bitmask)
        {
            /* Log an informational message and remove fruts.
             */
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_DEBUG_HIGH,
                                        "mirror: siots lba: 0x%llx blks: 0x%llx removing degraded position: %d from read chain \n",
                                        siots_p->start_lba, siots_p->xfer_count, read_fruts_p->position);
            /* Change the opcode to `NOP'.
             */
            fbe_raid_fruts_set_fruts_degraded_nop(read_fruts_p);
        }

        /* Goto next fruts.
         */
        read_fruts_p = next_read_fruts_p;
    
    } /* while more read fruts to check */

    /* Data disks becomes whatever is left active in the chain.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    siots_p->data_disks = fbe_raid_fruts_count_active(siots_p, read_fruts_p);
    /* Always return the execution status.
     */
    return(status);
}
/**********************************************
 * fbe_mirror_read_remove_degraded_read_fruts()
 *********************************************/

/*!***************************************************************************
 *          fbe_mirror_read_update_read_fruts_position()
 *****************************************************************************
 *
 * @brief   If the `old' (i.e. known degraded position) is found on the read
 *          fruts chain AND the new primary position isn't found, change the
 *          position of the read fruts to the new primary position.
 *
 * @param   siots_p - Pointer to siots to change read fruts for
 * @param   known_errored_position - Current primary position that encountered
 *                                    an error.
 * @param   new_primary_position - 
 * @param   b_log - log the trace if b_log is FBE_TRUE 
 *
 * @return  status - Typically FBE_STATUS_OK
 *                             Other - There is something wrong with the siots
 * 
 * @note    It is allowable to invoke this routine from the generate state.
 *          Therefore if the read fruts chain hasn't been setup yet it isn't
 *          an error!
 *
 * @author
 *  12/11/2009  Ron Proulx - Created.
 *
 *****************************************************************************/
 fbe_status_t fbe_mirror_read_update_read_fruts_position(fbe_raid_siots_t *siots_p,
                                                        fbe_u32_t known_errored_position,
                                                        fbe_u32_t new_primary_position,
                                                        fbe_bool_t b_log)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_raid_fruts_t   *read_fruts_p = NULL;
    fbe_raid_fruts_t   *known_degraded_fruts_p = NULL;
    fbe_bool_t          new_primary_on_read_chain = FBE_FALSE;

    /* We don't allow the known degraded to be the same as the new primary.
     */
    if (known_errored_position == new_primary_position)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: read update read fruts - old same as new position: %d \n",
                             known_errored_position);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Get the first read fruts if any.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    /* If the `old' primary is on the read chain and the new primary isn't,
     * change the position in the read fruts to the new primary position.
     * If we don't find the `old' read fruts it's not an error since we might
     * not have setup the read fruts yet.
     */
    while (read_fruts_p != NULL)
    {
        /* If this is the known degraded position set the fruts pointer.
         */
        if (read_fruts_p->position == known_errored_position)
        {
            known_degraded_fruts_p = read_fruts_p;
        }
        else if (read_fruts_p->position == new_primary_position)
        {
            /* If the new primary is already on the read fruts chain flag
             * it as break out since there is nothing else to do.
             */
            new_primary_on_read_chain = FBE_TRUE;
            break;
        }

        /* Goto the new read fruts.
         */
        read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);
    }

    /* Now update the position of the known degraded fruts to the new
     * primary position.
     */
    if (known_degraded_fruts_p != NULL)
    {
        /* Need to handle (2) cases:
         *  1. The new primary position wasn't in the read fruts chain:
         *      o Simply change the position field of the primary fruts to the new position
         *  2. Both the original and new primary are on the read fruts chain:
         *      o Remove the primary from the read chain
         */
        if ( b_log && (new_primary_on_read_chain == FBE_FALSE) )
        {
            /* We log an informational message that we are switching read positions, when b_log is True
             */
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                        "mirror: siots: 0x%x fruts: 0x%x pos old: %d new: %d\n",
                                        fbe_raid_memory_pointer_to_u32(siots_p),
                                        fbe_raid_memory_pointer_to_u32(known_degraded_fruts_p),
                                        known_errored_position, new_primary_position);
            known_degraded_fruts_p->position = new_primary_position;
        }
    }

    /* Update the read fruts chain, but don't change opcode to write
             */
    fbe_mirror_read_remove_degraded_read_fruts(siots_p, FBE_FALSE);

    /* Always return the execution status.
     */
    return(status);
}
/* end fbe_mirror_read_update_read_fruts_position() */

/*!***************************************************************************
 *      fbe_mirror_read_find_read_position()
 *****************************************************************************
 *
 * @brief   This function finds the first non-degraded position in the mirror 
 *          unit.  If there aren't any retries remaining (`retry_count') then
 *          we fail immediately.
 *
 * @param   siots_p - Pointer to siots to locate non-degraded position for
 * @param   known_errored_position - Known errored position
 * @param   new_primary_position_p - Address of new primary position to update
 *                                   (Only valid if status is FBE_STATUS_OK)
 * @param   b_log - FBE_TRUE - Trace the position change
 *
 * @return  status - Typically FBE_STATUS_OK
 *                             Other - There are no alternate positions to read
 *                                     from
 *
 * @author
 *  09/10/2009  Ron Proulx - Created from rg_mirror_rd_find_read_position
 *
 *****************************************************************************/                 
fbe_status_t fbe_mirror_read_find_read_position(fbe_raid_siots_t *siots_p,
                                                       fbe_u32_t known_errored_position,
                                                       fbe_u32_t *new_primary_position_p,
                                                       fbe_bool_t b_log)
{
    fbe_status_t                status;
    fbe_u32_t                   width = fbe_raid_siots_get_width(siots_p);
    fbe_u32_t                   new_primary_position = FBE_RAID_INVALID_POS;
    fbe_raid_position_bitmask_t full_access_bitmask = 0;
    fbe_u32_t                   index;

    /*! @note At one time we would limit the number of position changes by the
     *        retry count.  This is no longer the case.  So as long as we have
     *        positions available we will continue to retry.
     */

    /* Get the full access bitmask and walk thru the mirror position array
     * using the mirror index.
     */
    status = fbe_raid_siots_get_full_access_bitmask(siots_p, &full_access_bitmask);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror: failed to get access bitmask: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return  status;
    }

    /* Default status is dead (i.e. we couldn't find a replacement position)
     */
    status = FBE_STATUS_DEAD;
    for (index = 0; index < width; index++)
    {
        fbe_u32_t   position;

        /* If not the known degraded position and has full access,
         * and hasn't reported a hard media error, use that position.
         */
        position = fbe_mirror_get_position_from_index(siots_p, index);
        if ( (position        != known_errored_position)                      &&
             ((1 << position)  & full_access_bitmask)                          &&
            !((1 << position)  & siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap)    )
        {
            /* We have found a replacement position.
             */
            new_primary_position = position;
            status = FBE_STATUS_OK;
            break;
        }
    }

    /* If we have found a new position, update the siots position map
     * and change the read fruts position.
     */
    if (status == FBE_STATUS_OK)
    {
        /* If the update position map is successful, update the read fruts
         * position in the siots.
         */
        status = fbe_mirror_update_position_map(siots_p, new_primary_position);
        if (status == FBE_STATUS_OK)
        {
            status = fbe_mirror_read_update_read_fruts_position(siots_p,
                                                                known_errored_position,
                                                                new_primary_position,
                                                                FBE_TRUE); /*log if position changed*/
        }
    }

    /* If the update was successful update the contents of the new position
     * address.
     */
    if (status == FBE_STATUS_OK)
    {
        /* If trace logging is enabled and this isn't a verify request
         * log the position change.
         */
        if ((b_log == FBE_TRUE)                    &&
            (siots_p->algorithm != MIRROR_VR)      &&
            (siots_p->algorithm != MIRROR_RD_VR)   &&
            (siots_p->algorithm != MIRROR_COPY_VR) &&
            (siots_p->algorithm != MIRROR_VR_WR)   &&
            (siots_p->algorithm != MIRROR_VR_BUF)     )
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                        "mirror: find read position siots lba: 0x%llx blks: 0x%llx changed read position from: %d to %d \n",
                                        (unsigned long long)siots_p->start_lba,
				        (unsigned long long)siots_p->xfer_count,
                                        known_errored_position,
                                        new_primary_position);
        }

        /* Update the contents at the new primary pointer.
         */
        *new_primary_position_p = new_primary_position; 
    }

    /* Always return the execution status.
     */
    return(status);
}
/* fbe_mirror_read_find_read_position() */



/*!***************************************************************************
 *          fbe_mirror_read_setup_sgs_normal()
 *****************************************************************************
 *
 * @brief   Setup the sg lists for a `normal' read operation.  For each 
 *          read position setup the sgl entry.
 *
 * @param   siots_p - this I/O.
 * @param   memory_info_p - Pointer to memory request information
 *
 * @return  fbe_status_t
 *
 * @note    This method doesn't validate the sgs.  It is expected that the
 *          caller will validate the sgs.
 *
 * @author
 *  12/15/2009  Ron Proulx - Created from fbe_parity_verify_setup_sgs_normal
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_read_setup_sgs_normal(fbe_raid_siots_t * siots_p,
                                              fbe_raid_memory_info_t *memory_info_p)
{
    fbe_raid_fruts_t   *read_fruts_p = NULL;

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    while (read_fruts_p != NULL)
    {
        fbe_sg_element_t *sgl_p;
        fbe_u32_t dest_byte_count =0;
        fbe_u16_t sg_total =0;

        sgl_p = fbe_raid_fruts_return_sg_ptr(read_fruts_p);

        if(!fbe_raid_get_checked_byte_count(read_fruts_p->blocks, &dest_byte_count))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "%s: byte count exceeding 32bit limit \n",__FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE; 
        }
        /* Assign newly allocated memory for the whole region.
         */
        sg_total = fbe_raid_sg_populate_with_memory(memory_info_p,
                                                    sgl_p, 
                                                    dest_byte_count);
        if (sg_total == 0)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "%s: failed to populate sg for siots_p = 0x%p\n",
                                 __FUNCTION__,siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);
    }

    /* Always return the execution status.
     */
    return (FBE_STATUS_OK);
}
/******************************************
 * end fbe_mirror_read_setup_sgs_normal()
 ******************************************/

/*!***************************************************************************
 *          fbe_mirror_read_process_fruts_error()
 *****************************************************************************
 *
 * @brief   We have encountered a read error from one or more read fruts.
 *          Based on the eboard and our `dead' and degraded situation,
 *          determine if at least (1) fruts in the read chain succeeded.
 *
 * @param   siots_p - Pointer to siots that encounter read error
 * @param   eboard_p - Pointer to eboard that will help determine what actions
 *                     to take.
 *
 * @return  raid status - Typically FBE_RAID_STATUS_OK 
 *                             Other - Simply means that we can no longer
 *                                     continue processing this request.
 *
 * @note    We purposefully don't change the state here so that all state
 *          changes are visable in the state machine (based on the status
 *          returned).
 *
 * @author
 *  12/17/2009  Ron Proulx - Created.
 *
 *****************************************************************************/
fbe_raid_status_t fbe_mirror_read_process_fruts_error(fbe_raid_siots_t *siots_p, 
                                                      fbe_raid_fru_eboard_t *eboard_p)

{
    fbe_status_t        fbe_status = FBE_STATUS_INVALID;
    fbe_raid_status_t   status = FBE_RAID_STATUS_INVALID;
    fbe_raid_fruts_t   *read_fruts_p = NULL;
    fbe_u32_t           read_position = fbe_mirror_get_primary_position(siots_p);
    fbe_u32_t           new_read_position = FBE_RAID_INVALID_POS;
    fbe_u32_t           width = fbe_raid_siots_get_width(siots_p);

    /* Save the hard error bitmap in the siots eboard.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_fruts_update_media_err_bitmap(read_fruts_p);

    /* We currently (will no longer?) support the `dropped' error.
     * If it is set fail the request.
     */
    if (eboard_p->drop_err_count != 0)
    {
        /* This is unexpected.
         */
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: drop_err_count non-zero: 0x%x \n", 
                             eboard_p->drop_err_count);
        return(FBE_RAID_STATUS_UNSUPPORTED_CONDITION);
    }

    /* Next check the dead error count.  In this case the only recovery is
     * to locate another read position (i.e. one that isn't
     * degraded).  If we locate another read position that isn't dead and does
     * contain any errors.  We simply use that position for the read data.
     */
    if (eboard_p->dead_err_count > 0)
    {
        fbe_raid_position_bitmask_t degraded_bitmask;

        /* We should have already invoked `process dead fru error'.
         * Therefore the `waiting for monitor' flag should be set and
         * all dead positions should have been acknowledged.
         */
        fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
        if ( fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_WAITING_SHUTDOWN_CONTINUE) ||
            !fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_RECEIVED_CONTINUE)         ||
             ((eboard_p->dead_err_bitmap & ~degraded_bitmask) != 0)                                )
        {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: flags: 0x%x or dead bitmap: 0x%x or degraded bitmask: 0x%x unexpected \n",
                                 fbe_raid_siots_get_flags(siots_p), eboard_p->dead_err_bitmap, degraded_bitmask);
            return(FBE_RAID_STATUS_UNSUPPORTED_CONDITION);
        }
        
        /* If the current primary position is dead.  See if there is 
         * another read position that we can get the data from.
         */
        if ((1 << read_position) & eboard_p->dead_err_bitmap)
        {
            /* If there isn't another read position that we can get the
             * data from, fail the request.  Otherwise use the read data
             * from the new primary position.
             */
            if ( (fbe_status = fbe_mirror_read_find_read_position(siots_p,
                                                                  read_position,
                                                                  &new_read_position,
                                                                  FBE_TRUE)) != FBE_STATUS_OK )
            {
                /* Fail the request with a `dead' error.
                 */
                return(FBE_RAID_STATUS_MINING_REQUIRED);
            }
            
            /* This position should be marked degraded.
             */
            if (fbe_raid_siots_is_position_degraded(siots_p, read_position) != FBE_TRUE)
            {
                /* We have attempted to set more degraded positions than 
                 * are available. Fail the request.
                 */
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: process fruts error position: %d was reported dead but not marked degraded\n",
                                     read_position);
                return(FBE_RAID_STATUS_UNEXPECTED_CONDITION);
            }
                
            /* Else based on the algorithms that is being executed we will
             * either retry the read or simply continue since we have changed
             * the primary position to a non-dead position..
             */
            read_position = new_read_position;
        }

        /* Notice that we don't return here since we must process the other 
         * eboard bits.  We may have already processed the dead position during
         * retries thus always update the status to `retry possible'.
         */
            status = FBE_RAID_STATUS_RETRY_POSSIBLE;

        /* Now check if there are sufficient fruts to continue.
         */
        if (fbe_mirror_is_sufficient_fruts(siots_p) == FBE_FALSE)
        {
            /* Too many dead positions to continue.
             */
            return(FBE_RAID_STATUS_TOO_MANY_DEAD);
        }
    }

    /* Now handle unrecoverable (i.e. hard) errors.  The position isn't dead
     * but the data cannot be read.  Note that `hard' errors also include
     * `menr' errors so we don't need to check `menr' errors separately. 
     */
    if (eboard_p->hard_media_err_count > 0)
    {
        fbe_raid_position_bitmask_t read_fruts_bitmask;
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "mirror: media err siots: 0x%x siots lba: 0x%llx blks: 0x%llx dead: 0x%x hme: 0x%x\n",
                                    fbe_raid_memory_pointer_to_u32(siots_p),
			            (unsigned long long)siots_p->start_lba,
			            (unsigned long long)siots_p->xfer_count,
                                    eboard_p->dead_err_bitmap,
				    eboard_p->hard_media_err_bitmap);
        /* Sanity check the hard error count.  If it is greater than the width
         * something is wrong.
         */
        if (eboard_p->hard_media_err_count > width)
        {
            /* This is unexpected.
             */
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: process fruts error - hard_media_err_count: 0x%x greater than width: 0x%x \n", 
                                 eboard_p->hard_media_err_count, width);
            return(FBE_RAID_STATUS_UNEXPECTED_CONDITION);
        }
        
        /* If the current primary position has returned a hard error
         * check if there is another position that we could read from.
         * For multi-fruts read operations like verify it is ok to attempt
         * this since `find read position' won't select a fruts that is 
         * already on the read chain.
         */
        if ((1 << read_position) & eboard_p->hard_media_err_bitmap)
        {
            /* If the request succeeds it means that we can attempt the single
             * read to a different position.  Simply return the fact that the
             * request is retryable.
             */
            if ( (fbe_status = fbe_mirror_read_find_read_position(siots_p,
                                                                  read_position,
                                                                  &new_read_position,
                                                                  FBE_TRUE)) == FBE_STATUS_OK )
            {
                /* Return `retryable'.
                 */
                return(FBE_RAID_STATUS_RETRY_POSSIBLE);
            }
        }
        
        /* If for some reason (for instance the same position reported both
         * a dead error and a hard error and we removed the dead position
         * from the fruts chain) none of the current read fruts have reported
         * a hard error simply return success.
         */
        fbe_raid_siots_get_read_fruts_bitmask(siots_p, &read_fruts_bitmask);
        if ((eboard_p->hard_media_err_bitmap & read_fruts_bitmask) == 0)
        {
            /* None of the current read fruts reported hard errors. Return
             * success and continue processing.
             */
            return(FBE_RAID_STATUS_OK);
        }

#if 0  /* On a 3 way mirror we may have one dead position and one drive reporting media error */

        /* If this is a multi-position read and at least one read succeeded
         * then continue since XOR will generate the correct data.
         */
        if ((~eboard_p->hard_media_err_bitmap & read_fruts_bitmask) != 0)
        {
            /* XOR will use a different position to get the read data.
             */
            return(FBE_RAID_STATUS_OK);
        }
#endif

        /* Else simply indicate that a `verify' (or region verify) is required.
         */
        status = FBE_RAID_STATUS_MINING_REQUIRED;
    }

    /* Always return the execution status.
     */
    return(status);
}
/******************************************
 * end fbe_mirror_read_process_fruts_error()
 ******************************************/

/*!***************************************************************************
 *          fbe_mirror_read_reinit_fruts_from_bitmask()
 *****************************************************************************
 *
 * @brief   Use the supplied position bitmask to populate the read fruts chain.  
 *          This includes moving any non-write fruts from the write chain to
 *          the read chain and changing the opcode to read.  Since the I/O range
 *          might have changed we re-initialize both the read and write fruts.
 *          
 * @param   siots_p - ptr to the fbe_raid_siots_t
 * @param   read_bitmask - bitmap of positions to read.
 *
 * @return  fbe_status_t
 *
 * @notes   
 *
 * @author
 *  01/11/2010  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_status_t fbe_mirror_read_reinit_fruts_from_bitmask(fbe_raid_siots_t *siots_p, 
                                                       fbe_raid_position_bitmask_t read_bitmask)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_fruts_t           *write_fruts_p = NULL;
    fbe_raid_position_bitmask_t write_bitmask = 0;
    fbe_raid_fruts_t           *read_fruts_p = NULL;
    fbe_raid_position_bitmask_t exiting_read_bitmask = 0;
    fbe_raid_position_bitmask_t move_bitmask = 0;
    fbe_payload_block_operation_opcode_t write_opcode = fbe_mirror_get_write_opcode(siots_p);

    /* We need to move any read positions specified to the read chain.
     * It is assumed that there is at least (1) position to read.
     */
    if(RAID_COND(read_bitmask == 0))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror: reinit read from bitmask - no positions to read: 0x%x \n",
                             read_bitmask);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Get the existing read and write fruts bitmasks.  The `move'
     * bitmask is the read bitmask OR with the bitmask of positions on
     * the write chain.
     */
    fbe_raid_siots_get_read_fruts_bitmask(siots_p, &exiting_read_bitmask);
    fbe_raid_siots_get_write_fruts_bitmask(siots_p, &write_bitmask);
    move_bitmask = (write_bitmask & read_bitmask);

    /* It is legal (and in-fact typical) not to have any write fruts
     * that need to be moved. 
     */
    if (move_bitmask == 0)
    {
        /* Validate that there is at least (1) exiting read fruts
         * specified in the bitmask.  If there isn't we assume that there
         * is a logic error and thus report the failure.
         */
        if ((exiting_read_bitmask & read_bitmask) == 0)
        {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: reinit read from bitmask - no positions to read: 0x%x exiting: 0x%x \n",
                                 read_bitmask, exiting_read_bitmask);
            return(FBE_STATUS_GENERIC_FAILURE);
        }
    }    

    /* Move the associated read fruts from the write chain to the read chain.
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    while (write_fruts_p != NULL)
    {
        fbe_raid_fruts_t   *next_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);

        /* If this position is in the read bitmask move it to the
         * read fruts chain.
         */
        if ((1 << write_fruts_p->position) & move_bitmask)
        {
            /* Move to read chain.
             */
            status = fbe_raid_siots_remove_write_fruts(siots_p, write_fruts_p);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_set_unexpected_error(siots_p, 
                                                    FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                    "mirror: failed to remove write fruts: status = 0x%x, "
                                                    "siots_p = 0x%p, write_fruts_p = 0x%p\n",
                                                    status, siots_p, write_fruts_p);
                return  status;
            }
            fbe_raid_siots_enqueue_tail_read_fruts(siots_p, write_fruts_p);
        }

        /* Goto next.
         */
        write_fruts_p = next_fruts_p;
    }

    /* Reset the wait count.
     */
    siots_p->wait_count = 0;

    /*************************************************************
     * Loop through all the read fruts.
     * Re-init the fruts for each position.
     *************************************************************/
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    while (read_fruts_p != NULL)
    {
        /* Re-initialize each read fruts with the latest parity range.
         * do not update pdo object id
         */
        status = fbe_raid_fruts_init_read_request(read_fruts_p,
                                    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                    siots_p->parity_start,
                                    siots_p->parity_count,
                                    read_fruts_p->position);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "mirror: failed to init fruts: fruts = 0x%p, status = 0x%x, siots_p = 0x%p\n",
                                 read_fruts_p, status, siots_p);

            return  status;
        }
        
        /* Goto next read fruts.
         */
        read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);
    }

    /* Reinit the data count.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    siots_p->data_disks = fbe_raid_fruts_count_active(siots_p, read_fruts_p);

    /*************************************************************
     * Loop through all the write fruts.
     * Re-init the fruts for each position.
     *************************************************************/
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    while (write_fruts_p != NULL)
    {
        /* Re-initialize each write fruts with the latest parity range.
         */
        status = fbe_raid_fruts_init_request(write_fruts_p,
                                    write_opcode,
                                    siots_p->parity_start,
                                    siots_p->parity_count,
                                    write_fruts_p->position);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "mirror: failed to init fruts: fruts = 0x%p, status = 0x%x, siots_p = 0x%p\n",
                                 read_fruts_p, status, siots_p);
            return  status;
        }
        /* Goto next write fruts.
         */
        write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);
    } 

    /* Always return the execution status.
     */
    return(status);
}
/*****************************************************
 * end fbe_mirror_read_reinit_fruts_from_bitmask()
 ****************************************************/

/*!**************************************************************
 *          fbe_mirror_read_validate()
 ****************************************************************
 * @brief
 *  Validate the algorithm and some initial parameters to
 *  the read state machine.
 *
 * @param  siots_p - current I/O.          
 *
 * @note    We continue checking even after we have detected an error.
 *          This may help determine the source of the error.    
 *
 * @return FBE_STATUS_OK - Mirror verify request valid
 *         Other         - Malformed mirror verify request
 *
 * @author
 *  02/05/2009  Ron Proulx - Created
 *
 ****************************************************************/
fbe_status_t fbe_mirror_read_validate(fbe_raid_siots_t * siots_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           width = fbe_raid_siots_get_width(siots_p);

    /* First make sure we support the algorithm.
     */
    switch (siots_p->algorithm)
    {
        case MIRROR_RD:
            /* All the above algorithms are supported.
             */
            break;

        default:
            /* The algorithm isn't supported.  Trace some information and
             * fail the request.
             */
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: Unsupported algorithm: 0x%x\n", 
                                 siots_p->algorithm);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    /* By the time the mirror code is invoked (either by the monitor for
     * background requests or for user request) the request has been translated
     * into a physical mirror request.  Therefore the logical and physical
     * fields should match.  Also the data disks should match the width.
     * These checks are only valid when the original verify request is processed.
     * (i.e. For region mode we will execute multiple `sub' verifies.)
     */
    if (!fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE))
    {
        if (siots_p->start_lba != siots_p->parity_start)
        {
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: start_lba: 0x%llx and parity_start: 0x%llx don't agree.\n", 
                                 (unsigned long long)siots_p->start_lba,
			         (unsigned long long)siots_p->parity_start);
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        if (siots_p->xfer_count != siots_p->parity_count)
        {
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: xfer_count: 0x%llx and parity_count: 0x%llx don't agree.\n", 
                                 (unsigned long long)siots_p->xfer_count,
			         (unsigned long long)siots_p->parity_count);
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        if (siots_p->data_disks != 1)
        {
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: expected (1) for data_disks: 0x%x width: 0x%x\n", 
                                 siots_p->data_disks, width);
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    } /* end if not in region mode */

    /* Always return the status.
     */
    return(status);
}
/******************************************
 * end fbe_mirror_read_validate()
 ******************************************/

/*!***************************************************************************
 *          fbe_mirror_read_get_fru_info()
 *****************************************************************************
 * 
 * @brief   This function initializes the fru info array for a mirror read
 *          request.  Mirror reads only read from a single position.  If there
 *          is an recoverable read error simply need to change the position.
 *          For most reads we use the supplied sg.  For very large reads we
 *          need to generate an sg use the host offset.
 *
 * @param   siots_p - Current sub request.
 * @param   read_info_p - Array of read fruts info to populate
 * @param   write_info_p - Arrays of write fruts info to populate
 * @param   num_sgs_p - Array sgs indexes to populate
 * @param   b_log - If FBE_TRUE any error is unexpected
 *                  If FBE_FALSE we are determining if the siots should be split
 *
 * @return  status - FBE_STATUS_OK - read fruts information successfully generated
 *                   FBE_STATUS_INSUFFICIENT_RESOURCES - Request is too large
 *                   other - Unexpected failure
 *
 * @author
 *  11/25/2009  Ron Proulx  - Created from fbe_parity_verify_get_fruts_info
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_read_get_fru_info(fbe_raid_siots_t * siots_p,
                                          fbe_raid_fru_info_t *read_info_p,
                                          fbe_raid_fru_info_t *write_info_p,
                                          fbe_u16_t *num_sgs_p,
                                          fbe_bool_t b_log)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       width = fbe_raid_siots_get_width(siots_p);
    fbe_u32_t       index;
    fbe_u32_t       primary_position = fbe_mirror_get_primary_position(siots_p);
    fbe_u32_t       sg_count = 0;
    
    /* Page size must be set.
     */
    if (!fbe_raid_siots_validate_page_size(siots_p))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: read get fruts info - page size not valid \n");
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* We cannot use the parent's sg simply because the PVD does not expect to get an sg list that 
     * has more data in it.  In some cases of ZOD, the PVD needs to use the pre/post SGs in the payload, thus the 
     * host SG must have exactly the amount of data in it specified by the client. 
     */
    status = fbe_mirror_count_sgs_with_offset(siots_p,
                                              &sg_count);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to count sgs: siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return status;
    }
    
    /* Populate all positions for this siots.
     */
    for (index = 0; index < width; index++)
    {
        /* Initialize all positions but only configure the
         * primary position.
         */
        read_info_p->position = fbe_mirror_get_position_from_index(siots_p, index);

        /* Only configure the primary position as required.
         */
        if (read_info_p->position == primary_position)
        {
            read_info_p->lba = siots_p->parity_start;
            read_info_p->blocks = siots_p->parity_count;
            if (sg_count != 0)
            {
                /* Set sg index returns a status.  The reason is that the sg_count
                 * may exceed the maximum sgl length.
                 */
                status = fbe_raid_fru_info_set_sg_index(read_info_p, sg_count, b_log);
                if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                {
                    fbe_raid_siots_trace(siots_p,
                                         FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                         "mirror: failed to set sg index: status = 0x%x, siots_p = 0x%p, "
                                         "read_info_p = 0x%p\n", 
                                         status, siots_p, read_info_p);

                    return  status;
                }
                else
                {
                    /* Else increment the sg type count.
                     */
                    num_sgs_p[read_info_p->sg_index]++;
                }
            }
            else
            {
                /* Else use the host sg .
                 */
                read_info_p->sg_index = FBE_RAID_SG_INDEX_INVALID;
            }
        }
        else
        {
            /* Else initialize this position to invalid.
             */
            read_info_p->lba = FBE_LBA_INVALID;
            read_info_p->blocks = 0;
            read_info_p->sg_index = FBE_RAID_SG_INDEX_INVALID;
        }

        /* Populate write information as invalid
         */
        write_info_p->lba = FBE_LBA_INVALID;
        write_info_p->blocks = 0;
        write_info_p->sg_index = FBE_RAID_SG_INDEX_INVALID;

        /* Goto next fruts info.
         */
        read_info_p++;
        write_info_p++;
    }

    /* Always return the execution status.
     */
    return(status);
}
/**************************************
 * end fbe_mirror_read_get_fru_info()
 **************************************/

/*!**************************************************************************
 *          fbe_mirror_read_calculate_memory_size()
 ****************************************************************************
 *
 * @brief   This function calculates the total memory usage of the siots for
 *          this state machine.
 *
 * @param   siots_p - Pointer to siots that contains the allocated fruts
 * @param   fru_info_p - Pointer to array to populate for fruts information
 * 
 * @return  status - Typically FBE_STATUS_OK
 *                             Other - This shouldn't occur
 *
 * @author
 *  02/05/2010  Ron Proulx  - Created.
 *
 **************************************************************************/
fbe_status_t fbe_mirror_read_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                   fbe_raid_fru_info_t *fru_info_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_raid_fru_info_t write_fru_info[FBE_MIRROR_MAX_WIDTH];
    fbe_u16_t           num_sgs[FBE_RAID_MAX_SG_TYPE];
    fbe_u32_t           blocks_to_allocate = 0;

    fbe_zero_memory(&num_sgs[0], (sizeof(fbe_u16_t) * FBE_RAID_MAX_SG_TYPE));
    
    /* For a mirror read request the buffers have been allocated by the
     * parent. The first parameter to calculate page size is the number
     * of fruts which is the width of the raid group.
     */
    siots_p->total_blocks_to_allocate = blocks_to_allocate;
    fbe_raid_siots_set_optimal_page_size(siots_p,
                                         siots_p->data_disks,
                                         siots_p->total_blocks_to_allocate);    
    
    /* Now calculate the fruts information for each position including the
     * sg index if required.
     */
    status = fbe_mirror_read_get_fru_info(siots_p, 
                                          fru_info_p,
                                          &write_fru_info[0],
                                          &num_sgs[0],
                                          FBE_TRUE /* Log a failure */);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror : failed to get fru info: siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return status; 
    }

    /* Take the information we calculated above and determine how this translates 
     * into a number of pages. 
     */
    status = fbe_raid_siots_calculate_num_pages(siots_p, siots_p->data_disks, &num_sgs[0],
                                                FBE_TRUE /* In-case recovery verify is required*/);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror : failed to caclulate num of pages: siots_p = 0x%p, status = 0x%x\n",
                             siots_p,
                             status);
        return status; 
    }
    /* Always return the execution status.
     */
    return(status);
}
/**********************************************
 * end fbe_mirror_read_calculate_memory_size()
 **********************************************/

/*!**************************************************************************
 *          fbe_mirror_read_plant_parent_sgs()
 ****************************************************************************
 *
 * @brief   This method plants the sgs associated with a mirror read where the
 *          buffers have been allocated by the parent (upstream object).
 *          We simply plant the parent sg into the (1) and only read fruts.
 *          Although we would like to use fbe_raid_scatter_sg_to_bed, the
 *          element size has no meaning for mirror raid groups.
 *
 * @param   siots_p - Pointer to siots that contains the allocated fruts
 * @param   read_info_p - Pointer to array of per drive information               
 * 
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  02/10/2010  Ron Proulx  - Created
 *
 **************************************************************************/
static fbe_status_t fbe_mirror_read_plant_parent_sgs(fbe_raid_siots_t *siots_p,
                                                     fbe_raid_fru_info_t *read_info_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_fruts_t       *read_fruts_p = NULL;
    fbe_u32_t               primary_position = fbe_mirror_get_primary_position(siots_p);
    fbe_sg_element_t       *read_sg_p = NULL;
    fbe_raid_fru_info_t    *primary_info_p = NULL;
                              
    /* Get the read fruts.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    /* Validate the read fruts.
     */
    if ((read_fruts_p == NULL)                       ||
        (read_fruts_p->position != primary_position)    )
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: plant parent sg siots lba: 0x%llx blks: 0x%llx invalid read fruts\n",
                             (unsigned long long)siots_p->parity_start,
                             (unsigned long long)siots_p->parity_count);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Locate the primary fru info.
     */
    status = fbe_mirror_get_primary_fru_info(siots_p, read_info_p, &primary_info_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror:fail to get prim fru info:siots=0x%p,stat=0x%x\n",
                             siots_p, status);
        return (status);
    }
    
    /* If we didn't allocate an sg something is wrong.
     */
    if ( (read_sg_p = fbe_raid_fruts_return_sg_ptr(read_fruts_p)) == NULL )
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: plant parent sg siots lba: 0x%llx blks: 0x%llx no sg allocated\n",
                             (unsigned long long)siots_p->parity_start,
                             (unsigned long long)siots_p->parity_count);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Else plant the host data into the allocated sg.
     */
    if ((status = fbe_mirror_plant_sg_with_offset(siots_p,
                                                   primary_info_p,
                                                   read_sg_p))     != FBE_STATUS_OK )
    {
        /* Something is seriously wrong.
         */
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: plant parent sg siots lba: %llx blks: 0x%llx plant sg with offset failed\n",
                             (unsigned long long)siots_p->parity_start,
                             (unsigned long long)siots_p->parity_count);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Validate the sg was properly set up.
     */
    if (read_sg_p == NULL)
    {
        /* Report the error and fail the request.
         */
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: read - Error planting fruts: %p with parent sg: %p\n",
                             read_fruts_p, read_sg_p);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Always return the execution status.
     */
    return(status);
}
/*******************************************
 * end of fbe_mirror_read_plant_parent_sgs()
 *******************************************/

/*!**************************************************************
 *          fbe_mirror_read_setup_resources()
 ****************************************************************
 *
 * @brief   Setup the newly allocated resources for a mirror read.
 *
 * @param   siots_p - current I/O.
 * @param   read_info_p - Pointer to array of per drive information               
 *
 * @return fbe_status_t
 *
 * @author
 *  02/08/2010  Ron Proulx - Created
 *
 ****************************************************************/
fbe_status_t fbe_mirror_read_setup_resources(fbe_raid_siots_t *siots_p, 
                                             fbe_raid_fru_info_t *read_info_p)
{
    /* We initialize status to failure since it is expected to be populated. */
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_memory_info_t  memory_info;

    /* Initialize our memory request information
     */
    status = fbe_raid_siots_init_control_mem_info(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to init memory info : status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    }

    /* Setup all accessible positions with a fruts.  Only those
     * positions that aren't degraded will be populated with the
     * opcode passed. The degraded positions are populated with
     * invalid opcode (fbe_raid_fruts_send_chain() handles this).  
     * We allow degraded positions to supported because rebuilds
     * are generated from the verify state machine.
     */
    status = fbe_mirror_read_setup_fruts(siots_p, 
                                         read_info_p,
                                         FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                         &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror:fail to setup fruts:siots=0x%p,read_info=0x%p "
                             "stat=0x%x\n",
                             siots_p, read_info_p, status);
       return (status);
    }
    
    /* Plant the nested siots in case it is needed for recovery verify.
     * We don't initialize it until it is required.
     */
    status = fbe_raid_siots_consume_nested_siots(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to consume nested siots: siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return (status);
    }

    /* Initialize the RAID_VC_TS now that it is allocated.
     */
    status = fbe_raid_siots_init_vcts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {  
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to initialize vcts for siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return (status);
    }

    /* Initialize the VR_TS.
     * We are allocating VR_TS structures WITH the fruts structures.
     */
    status = fbe_raid_siots_init_vrts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {  
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to initialize vrts for siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return (status);
    }

    /* Using the sg_index information from the fru info array allocate
     * non-zero host read sg.
     */
    status = fbe_raid_fru_info_array_allocate_sgs_sparse(siots_p, 
                                                         &memory_info, 
                                                         read_info_p, 
                                                         NULL, 
                                                         NULL);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {  
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to allocate sgs sparse: siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return (status);
    }

    /* Mirror read sgs come from the upstream object.  Invoke method
     * that validates and configures the single read fruts.
     */
    status = fbe_mirror_read_plant_parent_sgs(siots_p,
                                              read_info_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {  
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to plant sgs for read fruts: siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return (status);
    }
    
    /* Make sure the buffer memory we just used does not overlap into 
     * the area used for other resources. 
     */
    status = fbe_mirror_validate_fruts(siots_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {  
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to validate sgs with status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    }

    return(status);
}
/******************************************
 * end fbe_mirror_read_setup_resources()
 ******************************************/

/*!***************************************************************************
 *          fbe_mirror_read_limit_request()
 *****************************************************************************
 *
 * @brief   Limits the size of a parent mirror read request.  There are
 *          (2) values that will limit the request size:
 *              1. Maximum per drive request
 *              2. Maximum amount of buffers that cna be allocated
 *                 per request.
 *
 * @param   siots_p - this I/O.
 *
 * @return  status - FBE_STATUS_OK - The request was modified as required
 *                   Other - Something is wrong with the request
 *
 * @note    Since we use the parent sgs if the request is too big we fail
 *          the request.
 *
 * @author  
 *  02/09/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_read_limit_request(fbe_raid_siots_t *siots_p)
{
    fbe_status_t        status = FBE_STATUS_OK;

    /* Use the standard method to determine the page size.  For mirror reads
     * we use (1) fruts and typically there are no blocks to allocate.
     */
    status = fbe_mirror_limit_request(siots_p, 
                                      1, 
                                      0,
                                      fbe_mirror_read_get_fru_info);

    /* Always return the execution status.
     */
    return (status);
}
/******************************************
 * end of fbe_mirror_read_limit_request()
 ******************************************/

/*!***************************************************************************
 *          fbe_mirror_read_set_read_optimization()
 *****************************************************************************
 *
 * @brief   This function is used to perform setting related to 
 *          mirror read optimization 
 *          If we are above the background verify checkpoint then
 *          we select the primary drive for the read.  If the read
 *          fails (due to an incomplete write) we will go into verify
 *          and correct the error.
 *
 *          If both disks are alive,allow optimizer to work on this fruts.
 *
 * @param   siots_p - this I/O.
 *
 * @return  status - FBE_STATUS_OK - The request was modified as required
 *                   Other - Something is wrong with the request
 *
 *
 * @author  
 *  04/12/2010  Swati Fursule  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_read_set_read_optimization(fbe_raid_siots_t *siots_p)
{
    fbe_status_t                   status = FBE_STATUS_OK;
    fbe_raid_iots_t                *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_position_bitmask_t     degraded_bitmask = 0x0;

    /* Don't set optimize and set the position to the primary.
     */
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    
    if  (degraded_bitmask != 0) 
    {
        return status;
    }

    if (fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS) == FBE_RAID_INVALID_DEAD_POS)
    {
        fbe_raid_fruts_t                *read_fruts_p = NULL;
        fbe_raid_group_paged_metadata_t *current_chunk_p = NULL;             
        fbe_lba_t                       lba;
        fbe_block_count_t               blocks;
        fbe_chunk_size_t                chunk_size = 0;
        fbe_chunk_count_t               start_bitmap_chunk_index;
        fbe_chunk_count_t               num_chunks;
        fbe_u32_t                       iots_chunk_index = 0;
        fbe_bool_t                      b_verify_needed = FBE_FALSE;

        //  Get the LBA and block count from the IOTS
        fbe_raid_iots_get_current_op_lba(iots_p, &lba);
        fbe_raid_iots_get_current_op_blocks(iots_p, &blocks);

        status = fbe_raid_iots_get_chunk_info(iots_p, 0, &current_chunk_p);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }

        fbe_raid_iots_get_chunk_size(iots_p, &chunk_size);
        /* Get the range of chunks affected by this iots.
         */
        fbe_raid_iots_get_chunk_range(iots_p, lba, blocks, chunk_size, &start_bitmap_chunk_index, &num_chunks);

        /*! Loop over all the affected chunks.
         */
        for (iots_chunk_index = 0; iots_chunk_index < num_chunks; iots_chunk_index++)
        {
            if (iots_p->chunk_info[iots_chunk_index].verify_bits != 0)
            {
                b_verify_needed = FBE_TRUE;
                break;
            }
        }
        fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
        /* If we are above the background verify checkpoint then
         * we select the primary drive for the read.  If the read
         * fails (due to an incomplete write) we will go into verify
         * and correct the error.
         */
        if (b_verify_needed )
        {
            read_fruts_p->position = 0;
        }
        else
        {
            /* Both disks are alive.
             * Allow optimizer to work on this fruts.
             */
            fbe_raid_fruts_set_flag(read_fruts_p, FBE_RAID_FRUTS_FLAG_OPTIMIZE);
        }
    }
    /* Always return the execution status.
     */
    return (status);
}
/******************************************
 * end of fbe_mirror_read_set_read_optimization()
 ******************************************/

/**********************************
 * end file fbe_mirror_read_util.c
 **********************************/
