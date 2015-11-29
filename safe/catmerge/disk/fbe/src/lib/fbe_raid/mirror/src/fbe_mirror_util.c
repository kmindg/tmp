/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_mirror_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains utility functions required for the common (i.e. operation
 *  independant) mirror group processing.
 *
 * @version
 *   9/03/2009  Ron Proulx  - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_library.h"
#include "fbe_spare.h"
#include "fbe_mirror_io_private.h"
#include "fbe_raid_library_proto.h"
#include "fbe_raid_fruts.h"

/*******************************
 *   FORWARD FUNCTION DEFINITIONS
 *******************************/
 
/*!***************************************************************************
 *          fbe_mirror_get_write_opcode()
 *****************************************************************************
 *
 * @brief   Verifys now use the `write & verify' operation.  This is done so 
 *          that when a verify is complete the state of the region is known
 *          to be coherent.
 *
 * @param   siots_p - ptr to the fbe_raid_siots_t
 *
 * @return  write_opcode - The opcode to use for the write request
 *
 * @note    Only verify requests need use this method since verify
 *          is only used in the verify state machine.
 *
 * @author
 *  01/13/2010  Ron Proulx  - Created
 *
 *******************************************************************/
fbe_payload_block_operation_opcode_t fbe_mirror_get_write_opcode(fbe_raid_siots_t * siots_p)
{
     fbe_payload_block_operation_opcode_t opcode;

    /* For a verify request we need to verify the data written.  Therefore
     * use the `write and verify' opcode.
     */
    if (siots_p->algorithm == RG_ZERO)
    {
        return(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO);
    }
    else if (siots_p->algorithm == MIRROR_VR)
    {
        return(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY);
    }
    else
    {     
        fbe_raid_siots_get_opcode(siots_p, &opcode);

        /* The raw mirror does not have an object interface so rebuilds are 
         * handled differently.  Raw mirror write-verify's are issued by clients 
         * to correct mirror inconsistencies so it is a valid opcode for raw mirrors.
         */
        if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY)
        {
            return FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY;
        }

        /* For all other opcodes/algorithms return `write'
         */
        return(FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE);
    }
}
/* end fbe_mirror_get_write_opcode() */
 
/*!***************************************************************************
 *              fbe_mirror_get_degraded_range_for_position()
 *****************************************************************************
 *
 * @brief   This function retrieves the degraded range for the
 *          requested position of a mirror.  We simply use the parity
 *          range of the siots to determine the degraded range for this
 *          position.
 *
 * @param   siots_p - Pointer to the siots with the request range
 * @param   position - The position to check if degraded 
 * @param   degraded_offset_p - Pointer to degraded start block to update
 * @param   degraded_blocks_p - Pointer to number of degraded blocks
 * @param   rebuild_log_needed_p - Pointer to `rebuild log needed' flag
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @note    Example of 2-way mirror where position 1 was removed and 
 *          re-inserted. (NR - Needs Rebuild, s - siots range).  Note that all
 *          'Needs Rebuild' ranges are in `chunk' size multiples where the 
 *          chunk size is 2048 (0x800) blocks.
 *
 *  pba             Position 0            Position 1
 *  0x11200         +--------+            +--------+
 *                  |        |            |        |
 *  0x11a00  siots  +--------+            +--------+
 *         0x11e00->|ssssssss|            |        |
 *  0x12200         +--------+  First RL->+========+
 *                  |ssssssss|            |NR NR NR|
 *  0x12a00         +--------+            +========+
 *                  |ssssssss|            |        |
 *  0x13200         +--------+            +--------+
 *                  |ssssssss|            |        |
 *  0x13a00         +--------+ Second RL->+========+
 *                  |ssssssss|            |NR NR NR|
 *  0x14200         +--------+            +========+
 *         0x14600->|ssssssss|            |        |
 *  0x14a00         +--------+            +--------+
 *                  |        |            |        |
 *
 *          For this example the original siots request for 0x11e00 thru
 *          0x145ff MAY (depending on algorithm) be broken into (5) siots:
 *          1. 0x11e00 -> 0x121ff:  No positions degraded
 *          2. 0x12200 -> 0x129ff:  Position 1 degraded
 *          3. 0x12a00 -> 0x139ff:  No positions degraded
 *          4. 0x13a00 -> 0x141ff:  Position 1 degraded
 *          5. 0x14200 -> 0x145ff:  No positions degraded
 *          Breaking up the I/O is required by the current raid algorithms
 *          which do not handle a degraded change within a siots.

 * @author
 *  12/01/2009  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_get_degraded_range_for_position(fbe_raid_siots_t *siots_p,
                                                        fbe_u32_t position,               
                                                        fbe_lba_t *degraded_offset_p,
                                                        fbe_block_count_t *degraded_blocks_p,
                                                        fbe_bool_t *rebuild_log_needed_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_iots_t        *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_iots_get_raid_geometry(iots_p);
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_position_bitmask_t disabled_bitmask = 0;
    fbe_bool_t              b_rebuild_logging = FBE_FALSE;
    fbe_bool_t              b_logs_available = FBE_FALSE;
    fbe_lba_t               degraded_offset = FBE_RAID_INVALID_DEGRADED_OFFSET;
    fbe_block_count_t       degraded_blocks = 0;
    fbe_bool_t              b_is_nr;
    fbe_raid_nr_extent_t    nr_extent[FBE_RAID_MAX_NR_EXTENT_COUNT];
    fbe_u32_t               max_extents = FBE_RAID_MAX_NR_EXTENT_COUNT;
   
    /* Get the opcode so that we can determine if we are `rebuild logging'
     * for a write request or totally degraded for this position.
     */
    fbe_raid_siots_get_opcode(siots_p, &opcode);
    
    /* Quickly check if we are rebuild logging or there are rebuild logs
     * available for this iots.
     */
    fbe_raid_iots_is_rebuild_logging(iots_p, position, &b_rebuild_logging);
    fbe_raid_iots_rebuild_logs_available(iots_p, position, &b_logs_available);

    /* The disabled bitmask include all positions that aren't accessible.
     * Although this includes the `rebuild logging' positions, there are
     * other reasons that the position may be marked disabled.
     */
    fbe_raid_siots_get_disabled_bitmask(siots_p, &disabled_bitmask);

    /* The entire I/O is within the degraded region, but first we need
     * to check if rebuild logging is in progress.  If it is, all or
     * part of the I/O may be non-degraded.  The degraded_offset will be
     * moved to account for this.
     *
     * If logging is still active, just log it and do a degraded write,
     * but if logging is done (the probational FRU came back) and there
     * are logs available, we have to check for clean regions.
     */
    if (disabled_bitmask & (1 << position))
    {
        /* We are `Rebuild Logging' (i.e. drive is missing) for this position.
         * For reads, we don't log but we continue degraded. 
         */
        if (fbe_payload_block_operation_opcode_is_media_modify(opcode))
        {
            /* Only generate a rebuild logging trace if we are indeed
             * rebuild logging and the trace is enabled.
             */
            if (b_rebuild_logging)
            {
                if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, 
                                                        FBE_RAID_LIBRARY_DEBUG_FLAG_REBUILD_LOG_TRACING))
                {
                    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                                "mirror: RL: Logging write siots:%08x pos:%d start:0x%llx count:0x%llx\n",
                                                fbe_raid_memory_pointer_to_u32(siots_p), position,
                                                (unsigned long long)siots_p->parity_start,
                                                (unsigned long long)siots_p->parity_count);
                }
            }
        }
        else
        {
            /* Else for all non-write type of operations we clear rebuild
             * logging.
             */
            b_rebuild_logging = FBE_FALSE;
        }

        /* For all operation types we mark the request completly degraded.  We
         * need to do this for writes for (2) reasons:
         *  1. The drive is missing so we won't allow the write anyway
         *  2. For unaligned writes we don't want to read from this position
         */
        degraded_offset = siots_p->parity_start;
        degraded_blocks = siots_p->parity_count;
    }
    else if (b_logs_available)
    {
        /* Else if there are rebuild logs (i.e. chunks that are been marked 
         * `Needs Rebuild' use the siots range and the NR chunk information
         * to set the degraded range for this position.
         */        
        if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, 
                                                FBE_RAID_LIBRARY_DEBUG_FLAG_REBUILD_LOG_TRACING))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                        "mirror: RL: Logs available siots:%08x pos:%d op:%d start:0x%llx count:0x%llx\n",
                                        fbe_raid_memory_pointer_to_u32(siots_p), position, opcode,
                                        (unsigned long long)siots_p->parity_start,
                                        (unsigned long long)siots_p->parity_count);
        }

        /* Get the `Needs Rebuild' information relative to this siots.  For
         * mirror raid groups every position has the same range independant of
         * request type.
         */
        fbe_raid_iots_get_nr_extent(iots_p,
                                    position,
                                    siots_p->parity_start, 
                                    siots_p->parity_count,
                                    &nr_extent[0], 
                                    max_extents);
        
        /* We have populated nr_extents[] with the NR information for a
         * particular position.  Now find the `remaining' (i.e. degraded)
         * blocks for this request
         */
        fbe_raid_extent_get_first_nr(siots_p->parity_start, 
                                     siots_p->parity_count,
                                     &nr_extent[0],
                                     max_extents,
                                     &degraded_offset,
                                     &degraded_blocks,
                                     &b_is_nr);

        if (b_is_nr == FBE_FALSE)
        {
            /* Although the iots had dirty regions, this particular siots doesn't.
             */
            if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, 
                                                FBE_RAID_LIBRARY_DEBUG_FLAG_REBUILD_LOG_TRACING))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                        "mirror: RL: Range clean siots:%08x offset:0x%llx blks:0x%llx\n",
                                        fbe_raid_memory_pointer_to_u32(siots_p),
                                        (unsigned long long)degraded_offset,
                                        (unsigned long long)degraded_blocks);
            }
        }
        else
        {
            /* Else we have updated the degraded range so simply log the
             * degraded range.
             */
            if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, 
                                                FBE_RAID_LIBRARY_DEBUG_FLAG_REBUILD_LOG_TRACING))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                        "mirror: RL: Range dirty siots:%08x offset:0x%llx blks:0x%llx\n",
                                        fbe_raid_memory_pointer_to_u32(siots_p),
                                        (unsigned long long)degraded_offset,
                                        (unsigned long long)degraded_blocks);
            }
        }

    } /* end else if rebuild logs available */

    /* If the status is good update the passed values.
     */
    if (status == FBE_STATUS_OK)
    {
        *degraded_offset_p = degraded_offset;
        *degraded_blocks_p = degraded_blocks;
        if (rebuild_log_needed_p != NULL)
        {
            *rebuild_log_needed_p = b_rebuild_logging;
        }
    }

    /* Always return the status.
     */
    return(status);
}
/***************************************************
 * end of fbe_mirror_get_degraded_range_for_position()
 ***************************************************/

/*!***************************************************************************
 *          fbe_mirror_is_primary_position_degraded()
 *****************************************************************************
 * 
 * @brief   Returns FBE_TRUE if the primary position is either degraded.
 *
 * @param   siots_p - Pointer to siots to determine state of primary for
 *
 * @return  bool - FBE_TRUE - Primary position is degraded
 *
 * @author
 *  12/09/2009  Ron Proulx  - Created.
 *
 ****************************************************************************/
fbe_bool_t fbe_mirror_is_primary_position_degraded(fbe_raid_siots_t *siots_p)
{
    fbe_bool_t                  is_degraded = FBE_FALSE;
    fbe_raid_position_bitmask_t degraded_bitmask;
    fbe_u32_t                   primary_position;

    /* Get the primary position and check if it degraded.
     */
    primary_position = fbe_mirror_get_primary_position(siots_p);
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    if ((1 << primary_position) & degraded_bitmask)
    {
        is_degraded = FBE_TRUE;
    }

    /* Always return degraded status.
     */
    return(is_degraded);
}
/* end fbe_mirror_is_primary_position_degraded() */

/*!***************************************************************************
 *          fbe_mirror_swap_dead_positions()
 *****************************************************************************
 * 
 * @brief   Simply swap the first and second dead positions.  No checking is
 *          performed.
 *
 * @param   siots_p - Pointer to siots to swap dead positions for
 *
 * @return  None
 *
 * @author
 *  12/07/2009  Ron Proulx  - Created.
 *
 ****************************************************************************/
static fbe_status_t fbe_mirror_swap_dead_positions(fbe_raid_siots_t *siots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u16_t   first_dead_position = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS);

    /* Update first from second and then second from first.
     */
    status = fbe_raid_siots_dead_pos_set(siots_p,
                                FBE_RAID_FIRST_DEAD_POS,
                                fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS));
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror: failed to set dead position: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return  status;
    }
    status = fbe_raid_siots_dead_pos_set(siots_p,
                                         FBE_RAID_SECOND_DEAD_POS,
                                         first_dead_position);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror: failed to set dead position: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return  status;
    }

    return status;
}
/* end fbe_mirror_swap_dead_positions() */

/*!***************************************************************************
 *          fbe_mirror_get_parity_position()
 *****************************************************************************
 * 
 * @brief   Returns the `parity' position for a mirror.  The `parity' position
 *          is defined as a secondary non-degraded position (the primary is
 *          always the first non-degraded position).  If there isn't second
 *          non-degraded position this method returns `invalid position'.
 *
 * @param   siots_p - Pointer to siots to get `parity' position for
 *
 * @return  parity position - `Invalid Pos' if no additional non-degraded
 *                            Other - Second non-degraded position
 *
 * @author
 *  12/09/2009  Ron Proulx  - Created.
 *
 ****************************************************************************/
fbe_u32_t fbe_mirror_get_parity_position(fbe_raid_siots_t *siots_p)
{
    fbe_raid_position_bitmask_t full_access_bitmask;
    fbe_u32_t                   primary_position;
    fbe_raid_position_bitmask_t primary_bitmask;
    fbe_u32_t                   parity_position = FBE_RAID_INVALID_POS;
    fbe_u32_t                   position;
    fbe_u32_t                   width = fbe_raid_siots_get_width(siots_p);
    fbe_status_t                status = FBE_STATUS_OK;
    /* Get the primary position and check if it is degraded.
     */
    primary_position = fbe_mirror_get_primary_position(siots_p);
    primary_bitmask = (1 << primary_position);
    status = fbe_raid_siots_get_full_access_bitmask(siots_p, &full_access_bitmask);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror: failed to get access bitmask: status = 0x%x, siots_p %p\n",
                             status, siots_p);
        return parity_position;
    }

    /* Now remove the primary position an find the first non-zero bit.
     */
    full_access_bitmask &= ~primary_position;
    for (position = 0; position < width; position++)
    {
        if ((1 << position) & full_access_bitmask)
        {
            parity_position = position;
            break;
        }
    }

    /* Always return the parity position.
     */
    return(parity_position);
}
/* end fbe_mirror_get_parity_position() */

/*!***************************************************************************
 *          fbe_mirror_determine_degraded_positions()
 *****************************************************************************
 * 
 * @brief   This method determines the degraded range for each mirror position.
 *          Then it updates the degraded information:
 *          o dead_pos and dead_pos_2 - Positions for which this I/O is degraded
 *          o degraded_offset - Minimum degraded offset (associated with `dead_pos')
 *          o degraded_blocks - How many blocks (within the siots range) starting
 *                              from the degraded offset are degraded (again for
 *                              `dead_pos'.
 *          This is a generic routine that handles both read and write reqeusts.
 *          Therefore even of all positions are degraded, this is fine for a write
 *          request.  
 *
 * @param   siots_p - Pointer to siots for request
 * @param   b_update_position_map - FBE_TRUE - We are determining the degraded
 *                                      positions for the first time (i.e. haven't
 *                                      populated the fruts yet)
 *                                  FBE_FALSE - We have already populated the fruts
 *                                      let find read position update the map 
 *
 * @return  status - Typically FBE_STATUS_OK  
 *
 * @author
 *  12/01/2009  Ron Proulx  - Created from mirror_check_degraded
 *
 *****************************************************************************/
static fbe_status_t fbe_mirror_determine_degraded_positions(fbe_raid_siots_t *siots_p,
                                                            fbe_bool_t b_update_position_map)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   width = fbe_raid_siots_get_width(siots_p);
    fbe_lba_t                   parity_start = siots_p->parity_start;
    fbe_lba_t                   parity_end = (parity_start + siots_p->parity_count) - 1;
    fbe_u32_t                   index;
    fbe_u32_t                   position;
    fbe_u32_t                   primary_position = fbe_mirror_get_primary_position(siots_p);
    fbe_raid_position_bitmask_t full_access_bitmask;

    /* Set the siots degraded range to invalid and initialize the
     * starting dead positions.
     */
    status = fbe_raid_siots_get_full_access_bitmask(siots_p, &full_access_bitmask);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror: failed to get access bitmask: status = 0x%x, siots_p %p\n",
                             status, siots_p);
        return status;
    }

    siots_p->degraded_start = FBE_RAID_INVALID_DEGRADED_OFFSET;
    siots_p->degraded_count = 0;

    /* Now walk thru the mirror group by index.
     */
    for (index = 0; index < width; index++)
    {
        fbe_lba_t           degraded_start;
        fbe_block_count_t   degraded_blocks;
        fbe_bool_t          rebuild_log_needed = FALSE;

        /* Get the position from the index (already configured)
         */
        position = fbe_mirror_get_position_from_index(siots_p, index);

        /* Get the degraded range for this position.  The marking of needs
         * rebuild for write request is now done in the raid group object.
         */
        if ( (status = fbe_mirror_get_degraded_range_for_position(siots_p,
                                                                  position,
                                                                  &degraded_start,
                                                                  &degraded_blocks,
                                                                  &rebuild_log_needed)) != FBE_STATUS_OK )
        {
            /* Report an error and break of the loop
             */
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: determine degraded for position: %d failed. status: 0%x\n",
                                 position, status);
            break;
        }
        else if (degraded_start > parity_end) 
        {
            /* Else if this position isn't degraded with respect to this
             * siots, remove this position from either of the dead positions.
             */
            if (fbe_raid_siots_is_position_degraded(siots_p, position) == FBE_TRUE)
            {
                if ( (status = fbe_raid_siots_remove_degraded_pos(siots_p,
                                                                  position)) != FBE_STATUS_OK )
                {
                    /* Something is wrong.  Trace this error and return failure.
                     */
                    fbe_raid_siots_trace(siots_p, 
                                         FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "mirror: clear degraded position: %d failed. status: 0%x\n",
                                         position, status);
                    break;
                }
            }
        }
        else
        {
            /* Else if this position hasn't already been marked degraded add it
             * now.
             */
            if (fbe_raid_siots_is_position_degraded(siots_p, position) == FBE_FALSE)
            {
                /* Mark this postion degraded
                 */
                if ( (status = fbe_raid_siots_add_degraded_pos(siots_p,
                                                               position)) != FBE_STATUS_OK )
                {
                    /* We have attempted to set more degraded positions than 
                     * are available. Fail the request.
                     */
                    fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: set degraded position: %d attempt to set more than (2) degraded. status: 0%x\n",
                                     position, status);
                    break;
                }

            } /* end if this position hasn't already been marked degraded */

            /* Check if this position is degraded with respect to this siots 
             * update the lowest degraded offset.
             */
            if (degraded_start < siots_p->degraded_start)
            {
                siots_p->degraded_start = degraded_start;
                siots_p->degraded_count = degraded_blocks;

                /* The first dead position must have the lowest degraded offset.  
                 * Swap degraded positions as neccessary.
                 */
                if (fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS) == position)
                {
                    /* Swap the dead positions.
                     */
                    status = fbe_mirror_swap_dead_positions(siots_p);
                    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                             "mirror: %s failed to swap dead position: status = 0x%x, siots_p = 0x%p\n",
                                             __FUNCTION__,
                                             status, siots_p);
                        return  status;
                    }
                }

            } /* end if this is the lowest degraded offset for this siots */

        } /* end else this position is degraded */
        
    } /* for all indexes in this mirror */
 
    /* If allowed, get the degraded bitmask.  If the primary position is degraded
     * locate the first non-degraded position and make that the primary.
     */
    if ((b_update_position_map == FBE_TRUE)                                          &&
        (fbe_raid_siots_is_position_degraded(siots_p, primary_position) == FBE_TRUE)    )
    {
        fbe_u32_t   new_primary_position = FBE_RAID_INVALID_DEAD_POS;

        /* Refresh the full access bitmask
         */
        fbe_raid_siots_get_full_access_bitmask(siots_p, &full_access_bitmask);

        /* Locate first non-degraded position
         */
        for (position = 0; position < width; position++)
        {
            if (full_access_bitmask & (1 << position))
            {
                /* Located a non-degraded position
                 */
                new_primary_position = position;
                break;
            }
        }

        /* If a new primary position is found update the position map
         */
        if (new_primary_position != FBE_RAID_INVALID_DEAD_POS)
        {
            status = fbe_mirror_update_position_map(siots_p, new_primary_position);
        }

    } /* end if the original primary position is degraded */

    /* Always return the execution status.
     */
    return(status);
}
/**********************************************
 * end fbe_mirror_determine_degraded_positions()
 **********************************************/

/*!***************************************************************************
 *          fbe_mirror_update_degraded()
 *****************************************************************************
 * 
 * @brief   This method updates the degraded positions by breaking up a siots
 *          so that at least one position isn't degraded.  We locate the degraded
 *          range for all position for the associate siots and then break the
 *          siots into a portion that contains at least one non-degraded area.  
 *
 * @param   siots_p - Pointer to siots for request
 * @param   b_set_siots_range - FBE_TRUE - Update the range for this siots
 *                              FBE_FALSE - Don't modify the siots range
 *
 * @return  status - FBE_STATUS_OK - Found a non-degraded range for this siots
 *                   Other - Couldn't find a non-degraded range 
 *
 * @note    Since this return will return a failure status unless at least (1)
 *          non-degraded position is found, it is meant for read algorithms (i.e.
 *          write algorithms that don't require a pre-read shouldn't be using this
 *          method.
 *
 * @note    Example of 3-way mirror where position 1 was removed and replaced then
 *          position 2 was removed and re-inserted. (NR - Needs Rebuild, s - siots
 *          range).  Note that all 'Needs Rebuild' ranges are in `chunk' size 
 *          multiples where the chunk size is 2048 (0x800) blocks.
 *
 *  pba             Position 0           Position 1           Position 2
 *  0x11200         +--------+           +--------+           +--------+
 *                  |        |           |        |           |        |
 *  0x11a00  siots  +--------+<-First RL +--------+           +--------+
 *         0x11e00->|NR NR NR|           |        |           |        |
 *  0x12200         +--------+           +--------+ First RL->+========+
 *                  |ssssssss|           |        |           |NR NR NR|
 *  0x12a00         +--------+ Degraded->+========+           +========+
 *                  |ssssssss|           |NR NR NR|           |        |
 *  0x13200         +--------+<-Second RL+--------+           +--------+
 *                  |NR NR NR|           |NR NR NR|           |        |
 *  0x13a00         +--------+           +--------+Second RL->+========+
 *                  |ssssssss|           |NR NR NR|           |NR NR NR|
 *  0x14200         +--------+           +--------+           +========+
 *         0x14600->|ssssssss|           |NR NR NR|           |        |
 *  0x14a00         +--------+           +--------+           +--------+
 *                  |        |           |NR NR NR|           |        |
 *
 *          For this example the original siots request for 0x11e00 thru
 *          0x145ff will be broken into the following siots:    Read Position Selected
 *                                                              ----------------------
 *  1. 0x11e00 -> 0x129ff:  Position 0 or position 2 degraded   Position 1
 *  2. 0x12a00 -> 0x139ff:  Position 0 or position 1 degraded   Position 2
 *  3. 0x13a00 -> 0x145ff:  Position 1 or position 2 degraded   Posiiton 0
 *          Breaking up the I/O is required by the current raid algorithms
 *          which do not handle a degraded change within a siots.
 *
 * @author
 *  12/09/2009  Ron Proulx  - Created from mirror_check_degraded
 *
 ****************************************************************/
static fbe_status_t fbe_mirror_update_degraded(fbe_raid_siots_t *siots_p,
                                               fbe_bool_t b_set_siots_range)
{
    /* We default the status to `dead' until we locate a non-degraded
     * range.
     */
    fbe_status_t        status = FBE_STATUS_DEAD;
    fbe_status_t        find_read_position_status = FBE_STATUS_OK;
    fbe_u32_t           width = fbe_raid_siots_get_width(siots_p);
    fbe_lba_t           parity_start = siots_p->parity_start;
    fbe_lba_t           parity_end = (parity_start + siots_p->parity_count) - 1;
    fbe_u32_t           index, selected_index = FBE_MIRROR_INVALID_INDEX;
    fbe_bool_t          needs_rebuild_log = FBE_FALSE;
    fbe_raid_fru_info_t drive_info_array[FBE_MIRROR_MAX_WIDTH];
    fbe_lba_t           max_degraded_offset;
    fbe_u32_t           original_primary_position;
    fbe_u32_t           updated_primary_position = FBE_RAID_INVALID_POS;          

    /* This routine updates the degraded offset.  Thus it assumes
     * the the minimum degraded offset and count have been set.
     */
    max_degraded_offset = parity_start;
    original_primary_position = fbe_mirror_get_primary_position(siots_p);

    /* Now walk thru the mirror group by index.
     */
    for (index = 0; index < width; index++)
    {
        /* Get the position from the index (already configured)
         */
        drive_info_array[index].position = fbe_mirror_get_position_from_index(siots_p, index);
        
        /* Get the degraded range for this position.
         */
       status = fbe_mirror_get_degraded_range_for_position(siots_p,
                                                           drive_info_array[index].position,
                                                           &drive_info_array[index].lba,
                                                           &drive_info_array[index].blocks,
                                                           &needs_rebuild_log);

        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            /* Report an error and return.
             */
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: failed to update degraded range for position: "
                                 "siots_p = 0x%p, position = 0%x, status = 0%x\n",
                                 siots_p, drive_info_array[index].position, status);
            return (status);
        }
    } /* end for all mirror indexes */

    /* Reset the status to `dead' and locate the maximum degraded offset.
     */
    status = FBE_STATUS_DEAD;
    for (index = 0; index < width; index++)
    {
        /* If the degraded offset is greater than the current,
         * select this index and update the max_degraded_offset.
         */
        if (drive_info_array[index].lba > max_degraded_offset)
        {
            selected_index = index;
            max_degraded_offset = drive_info_array[index].lba;
        }
    }

    /* If we have found a position (even if it is for a single block!) that
     * has a non-degraded range for this siots, select it, update the siots
     * range and update the dead positions.
     */
    if (max_degraded_offset > parity_start)
    {
        /* Change status to ok since we located a non-degraded range.
         */
        status = FBE_STATUS_OK;

        /* Update the siots range if requested.
         */
        if (b_set_siots_range == FBE_TRUE)
        {
            parity_end = FBE_MIN(parity_end, (max_degraded_offset - 1));
            siots_p->parity_count = (fbe_block_count_t)(parity_end - parity_start) + 1;
            siots_p->xfer_count = siots_p->parity_count;
            siots_p->degraded_start = max_degraded_offset;
            siots_p->degraded_count = drive_info_array[selected_index].blocks;
        }

        /* Now walk thru all positions and update dead positions as neccessary.
         */
        for (index = 0; index < width; index++)
        {
            /* If the degraded offset for this index is greater than the new
             * siots range and it has been marked dead, remove it from the
             * dead array.
             */
            if ((drive_info_array[index].lba > parity_end)                                         &&
                (fbe_raid_siots_is_position_degraded(siots_p,
                                                     drive_info_array[index].position) == FBE_TRUE)   )
            {
                /* Remove this position from either of the dead positions.
                 */
                status = fbe_raid_siots_remove_degraded_pos(siots_p,
                                                            drive_info_array[index].position);
                if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) 
                {
                    /* Something is wrong.  Trace this error and return failure.
                     */
                    fbe_raid_siots_trace(siots_p, 
                                         FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "mirror: failed to remove degraded position: "
                                         "siots_p = 0x%p, drive_info_array[%d].position = 0x%x, status = 0x%x\n",
                                         siots_p, index, drive_info_array[index].position, status);
                    return (status);
                }
            }
        } /* end for all indexes */

        /* Now check and update read position as neccessary.
         */
        if (fbe_raid_siots_is_position_degraded(siots_p, original_primary_position) == FBE_TRUE)
        {
            /* Attempt to update the preferred read position.
             */
            find_read_position_status = fbe_mirror_read_find_read_position(siots_p,
                                                                           original_primary_position,
                                                                           &updated_primary_position,
                                                                           FBE_FALSE);

            /*! @note There are cases where we ignore the `find read position 
             *        status' since verify etc may come thru here (i.e. uncorrectable
             *        errors.
             */
            if ((b_set_siots_range == FBE_TRUE)              &&
                (find_read_position_status != FBE_STATUS_OK)    )
            {
                /* Trace the error.
                 */
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: update degraded failed to find read replacement position for primary position: %d \n",
                                     original_primary_position);
            }
        }

    } /* end if non-degraded range found */
    
    /* Always return the execution status.
     */
    return(status);
}
/**************************************
 * end fbe_mirror_update_degraded()
 **************************************/

/*!***************************************************************************
 *      fbe_mirror_check_degraded()
 *****************************************************************************
 * 
 * @brief   This function determines how to split a request based on the 
 *          degraded drives.  It assumes that the minimum degraded offset has 
 *          been set and that the dead positions have been set.  We need at 
 *          least (1) non-degraded position so that we can complete the request.
 *
 * @param   siots_p - Pointer to siots that may need to be split
 * @param   b_set_siots_range - FBE_TRUE - Update the range for this siots
 *                              FBE_FALSE - Don't modify the siots range
 * @param   b_ok_to_split - FBE_TRUE indicates that it is ok to split this siots
 *                        in order to complete the request        
 *
 * @return  status - Typically FBE_STATUS_OK
 *                   Other - There are too many degraded positions
 *                           to complete this request.
 *
 * @note    Example of 3-way mirror where position 1 was removed and replaced then
 *          position 2 was removed and re-inserted. (NR - Needs Rebuild, s - siots
 *          range).  Note that all 'Needs Rebuild' ranges are in `chunk' size 
 *          multiples where the chunk size is 2048 (0x800) blocks.
 *
 *  pba             Position 0           Position 1           Position 2
 *  0x11200         +--------+           +--------+           +--------+
 *                  |        |           |        |           |        |
 *  0x11a00  siots  +--------+           +--------+           +--------+
 *         0x11e00->|ssssssss|           |        |           |        |
 *  0x12200         +--------+           +--------+ First RL->+========+
 *                  |ssssssss|           |        |           |NR NR NR|
 *  0x12a00         +--------+ Degraded->+========+           +========+
 *                  |ssssssss|           |NR NR NR|           |        |
 *  0x13200         +--------+           +--------+           +--------+
 *                  |ssssssss|           |NR NR NR|           |        |
 *  0x13a00         +--------+           +--------+Second RL->+========+
 *                  |ssssssss|           |NR NR NR|           |NR NR NR|
 *  0x14200         +--------+           +--------+           +========+
 *         0x14600->|ssssssss|           |NR NR NR|           |        |
 *  0x14a00         +--------+           +--------+           +--------+
 *                  |        |           |NR NR NR|           |        |
 *
 *          For this example the original siots request for 0x11e00 thru
 *          0x145ff MAY (depending on algorithm) be broken into (5) siots:
 *          1. 0x11e00 -> 0x121ff:  No positions degraded
 *          2. 0x12200 -> 0x129ff:  Position 2 degraded
 *          3. 0x12a00 -> 0x139ff:  Position 1 degraded
 *          4. 0x13a00 -> 0x141ff:  Position 1 and position 2 degraded
 *          5. 0x14200 -> 0x145ff:  Position 2 degraded
 *          Breaking up the I/O is required by the current raid algorithms
 *          which do not handle a degraded change within a siots.
 *
 * @author
 *  09/10/2009  Ron Proulx - Created from mirror_rd_disk_degraded
 *
 *****************************************************************************/
static fbe_status_t fbe_mirror_check_degraded(fbe_raid_siots_t *siots_p,
                                              fbe_bool_t b_set_siots_range,
                                              fbe_bool_t b_ok_to_split)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           width = fbe_raid_siots_get_width(siots_p);
    fbe_u32_t           original_primary_position = fbe_mirror_get_primary_position(siots_p);
    fbe_u32_t           alternate_position = FBE_RAID_INVALID_POS;
    fbe_u16_t           degraded_bitmask = 0;
    fbe_u32_t           degraded_count;
    
    /* Get the degraded bitmask so that we can determine if we need to continue
     * or not.  The degraded bitmask includes any disabled positions also.
     */
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    degraded_count = fbe_raid_get_bitcount(degraded_bitmask);

    /* If there aren't any degraded positions simply return.
     */
    if (degraded_count == 0)
    {
        return(status);
    }

    /* For all requests we must have at least (1) non-degraded position.
     * (Unless it is ok to split the request)
     */
    if (degraded_count >= width)
    {
        /* Either there is something wrong or it isn't ok to split
         * this siots (i.e. we have already started).
         */
        if ((degraded_count > width)     ||
            (b_ok_to_split == FBE_FALSE)    )
        {
            /* We need at least (1) non-degraded position.  Fail request.
             */
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: siots_p: 0x%p check degraded to many degraded positions. degraded_count: 0x%x width: 0x%x\n",
                                 siots_p, degraded_count, width);
            return(FBE_STATUS_GENERIC_FAILURE);
        }
    }
    
    /* If the primary position is degraded locate a non-degraded position.
     * If that fails and split is allowed, split the request in order to find
     * and non-degraded range.
     */
    if (fbe_mirror_is_primary_position_degraded(siots_p) == FBE_TRUE)
    {
        /* Attempt to find and new read position.  If the request fails and
         * we are allowed 
         */
        if ( (status = fbe_mirror_read_find_read_position(siots_p, 
                                                          original_primary_position,
                                                          &alternate_position,
                                                          FBE_FALSE))               != FBE_STATUS_OK )
        {
            status = fbe_mirror_update_degraded(siots_p, b_set_siots_range);
        } /* end Else if no alternate position. */

    } /* end if primary position is degraded */

    /* Always return execution status.
     */
    return(status);

}
/* end fbe_mirror_check_degraded() */

/*!***************************************************************************
 *          fbe_mirror_set_degraded_positions()
 *****************************************************************************
 * 
 * @brief   This method determines the degraded range for each mirror position.
 *          Then it updates the degraded information:
 *          o dead_pos and dead_pos_2 - Positions for which this I/O is degraded
 *          o degraded_offset - Minimum degraded offset (associated with `dead_pos')
 *          o degraded_blocks - How many blocks (within the siots range) starting
 *                              from the degraded offset are degraded (again for
 *                              `dead_pos'.
 *          This is a generic routine that handles both read and write reqeusts.
 *          Therefore even of all positions are degraded, this is fine for a write
 *          request.  
 *
 * @param   siots_p - Pointer to siots for request
 * @param   b_ok_to_split - FBE_TRUE indicates that it is ok to split this siots
 *                        in order to complete the request  
 *
 * @return  status - Typically FBE_STATUS_OK  
 *
 * @note    Example of 3-way mirror where position 1 was removed and replaced then
 *          position 2 was removed and re-inserted. (NR - Needs Rebuild, s - siots
 *          range).  Note that all 'Needs Rebuild' ranges are in `chunk' size 
 *          multiples where the chunk size is 2048 (0x800) blocks.
 *
 *  pba             Position 0           Position 1           Position 2
 *  0x11200         +--------+           +--------+           +--------+
 *                  |        |           |        |           |        |
 *  0x11a00  siots  +--------+           +--------+           +--------+
 *         0x11e00->|ssssssss|           |        |           |        |
 *  0x12200         +--------+           +--------+ First RL->+========+
 *                  |ssssssss|           |        |           |NR NR NR|
 *  0x12a00         +--------+ Degraded->+========+           +========+
 *                  |ssssssss|           |NR NR NR|           |        |
 *  0x13200         +--------+           +--------+           +--------+
 *                  |ssssssss|           |NR NR NR|           |        |
 *  0x13a00         +--------+           +--------+Second RL->+========+
 *                  |ssssssss|           |NR NR NR|           |NR NR NR|
 *  0x14200         +--------+           +--------+           +========+
 *         0x14600->|ssssssss|           |NR NR NR|           |        |
 *  0x14a00         +--------+           +--------+           +--------+
 *                  |        |           |NR NR NR|           |        |
 *
 *          For this example the original siots request for 0x11e00 thru
 *          0x145ff MAY (depending on algorithm) be broken into (5) siots:
 *          1. 0x11e00 -> 0x121ff:  No positions degraded
 *          2. 0x12200 -> 0x129ff:  Position 2 degraded
 *          3. 0x12a00 -> 0x139ff:  Position 1 degraded
 *          4. 0x13a00 -> 0x141ff:  Position 1 and position 2 degraded
 *          5. 0x14200 -> 0x145ff:  Position 2 degraded
 *          Breaking up the I/O is required by the current raid algorithms
 *          which do not handle a degraded change within a siots.
 *
 * @author
 *  12/01/2009  Ron Proulx  - Created from mirror_check_degraded
 *
 ****************************************************************/
fbe_status_t fbe_mirror_set_degraded_positions(fbe_raid_siots_t *siots_p,
                                               fbe_bool_t  b_ok_to_split)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_position_bitmask_t degraded_bitmask = 0;

    /* For nested siots we clear and reset the degraded positions from
     * the parent bitmask.  This is required since the parent may have
     * encountered new dead positions.  For normal siots there shouldn't
     * be any degraded positions set.
     */
    if (fbe_raid_siots_is_nested(siots_p))
    {
        /* First clear any degraded positions since they will be calculated
         * below.
         */
        fbe_raid_siots_dead_pos_set(siots_p, FBE_RAID_FIRST_DEAD_POS, FBE_RAID_INVALID_DEAD_POS);
        fbe_raid_siots_dead_pos_set(siots_p, FBE_RAID_SECOND_DEAD_POS, FBE_RAID_INVALID_DEAD_POS);
    }

    /* There shouldn't be any positions marked degraded yet.
    */
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    if (degraded_bitmask != 0)
    {
        /* Trace the failure.
         */
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: set degraded for siots: 0x%llx 0x%llx degraded_bitmask: 0x%x already set \n",
                             (unsigned long long)siots_p->start_lba,
                 (unsigned long long)siots_p->xfer_count,
                 degraded_bitmask);
        return(FBE_STATUS_GENERIC_FAILURE);
    }
    
    /* Invoke method to determine the degraded positions
     */
    status = fbe_mirror_determine_degraded_positions(siots_p, FBE_TRUE);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to determine degraded position: "
                             "siots_p = 0x%p, siots_p->start_lba = 0x%llx, siots_p->xfer_count = 0x%llx "
                             "degraded_bitmask = 0x%x, status = 0x%x\n",
                             siots_p,
                 (unsigned long long)siots_p->start_lba,
                 (unsigned long long)siots_p->xfer_count,
                 degraded_bitmask, status);
        return(status);
    }
    
    /*! Now validate that we have sufficient positions to continue
     *
     *  @note We have already populated the parity range.
     */
    status = fbe_mirror_check_degraded(siots_p, FBE_TRUE, b_ok_to_split);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: siots_p: 0x%p Failed to locate sufficient non-degraded. status: 0x%x\n",
                             siots_p, status);
    }

    /*! @note Degraded fruts will be removed in 
     *        fbe_mirror_handle_degraded_positions()
     */
    
    /* Always return the execution status.
     */
    return(status);
}
/****************************************
 * end fbe_mirror_set_degraded_positions()
 ****************************************/

/*!***************************************************************************
 *          fbe_mirror_handle_degraded_positions()
 *****************************************************************************
 * 
 * @brief   This method processes the degraded range for each mirror position.
 *          Then it updates the degraded information:
 *          o dead_pos and dead_pos_2 - Positions for which this I/O is degraded
 *          o degraded_offset - Minimum degraded offset (associated with `dead_pos')
 *          o degraded_blocks - How many blocks (within the siots range) starting
 *                              from the degraded offset are degraded (again for
 *                              `dead_pos'.
 *          This is a generic routine that handles both read and write reqeusts.
 *          Therefore even of all positions are degraded, this is fine for a write
 *          request.  
 *
 * @param   siots_p - Pointer to siots for request
 * @param   b_write_request - FBE_TRUE - This is a write request (write fruts chain)
 *                            FBE_FALSE - This is a read request (read fruts chain)
 * @param   b_write_degraded - FBE_TRUE - Move any degraded read positions to the
 *                                        write chain and set the opcode to write
 *                             FBE_FALSE - Move any degraded read to write chain
 *                                        and set opcode to NOP.
 *
 * @return  status - Typically FBE_STATUS_OK  
 *
 * @note    All `disabled' positions will be set to NOP
 *
 * @author
 *  01/25/2011  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_status_t fbe_mirror_handle_degraded_positions(fbe_raid_siots_t *siots_p,
                                                  fbe_bool_t b_write_request,
                                                  fbe_bool_t b_write_degraded)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_position_bitmask_t degraded_bitmask = 0; 

    /* Invoke method to determines the degraded positions
     */
    status = fbe_mirror_determine_degraded_positions(siots_p, FBE_FALSE);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: siots_p: 0x%p failed to determine degraded. degraded_bitmask: 0x%x status: 0x%x \n",
                             siots_p, degraded_bitmask, status);
        return(status);
    }
    
    /* If nothing is degraded return (returning positions that weren't there
     * when the request was generated will still be marked degraded)
     */
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    if (degraded_bitmask == 0)
    {
        return(FBE_STATUS_OK);
    }

    /*! Now validate that we have sufficient positions to continue
     *
     *  @note We have already populated the parity range.
     */
    status = fbe_mirror_check_degraded(siots_p, FBE_FALSE, FBE_FALSE);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: siots_p: 0x%p Failed to locate sufficient non-degraded. status: 0x%x\n",
                             siots_p, status);
        return(status);
    }

    /* Now update either the read or write fruts chains
     */
    if (b_write_request == FBE_TRUE)
    {
        /* Update the write fruts chain with the degraded information
         */
        status = fbe_mirror_write_remove_degraded_write_fruts(siots_p, b_write_degraded);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))    
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: siots_p: 0x%p Failed to remove degraded write fruts. status: 0x%x\n",
                                 siots_p, status);
            return(status);
        }
        
    }
    else
    {
        /* Else update the read fruts chain with the degraded informaiton
         */
        status = fbe_mirror_read_remove_degraded_read_fruts(siots_p, b_write_degraded);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: siots_p: 0x%p Failed to remove degraded read fruts. status: 0x%x\n",
                                 siots_p, status);
            return(status);
        }
        
    }

    /* Always return the execution status.
     */
    return(status);
}
/*********************************************
 * end fbe_mirror_handle_degraded_positions()
 *********************************************/

/*!***************************************************************************
 *          fbe_mirror_refresh_degraded_positions()
 *****************************************************************************
 * 
 * @brief   This method refreshes the degraded range for each mirror position.
 *          Then it updates the degraded information:
 *          o dead_pos and dead_pos_2 - Positions for which this I/O is degraded
 *          o degraded_offset - Minimum degraded offset (associated with `dead_pos')
 *          o degraded_blocks - How many blocks (within the siots range) starting
 *                              from the degraded offset are degraded (again for
 *                              `dead_pos'.
 *          This is a generic routine that handles both read and write reqeusts.
 *          Therefore even of all positions are degraded, this is fine for a write
 *          request.  
 *
 * @param   siots_p - Pointer to siots for request
 * @param   fruts_p - Used to determine if this is a read or write request
 *
 * @return  status - Typically FBE_STATUS_OK  
 *
 * @author
 *  01/25/2011  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_status_t fbe_mirror_refresh_degraded_positions(fbe_raid_siots_t *siots_p,
                                                   fbe_raid_fruts_t *fruts_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_bool_t          b_write_request = FBE_TRUE;
    fbe_bool_t          b_write_degraded = FBE_FALSE;
    fbe_raid_fruts_t   *read_fruts_p = NULL;
 
    /* Determine if this is a read or write request
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    while(read_fruts_p != NULL)
    {
        /* If the passed fruts is on the read chain we were sending read 
         * requests.
         */
        if (read_fruts_p == fruts_p)
        {
            b_write_request = FBE_FALSE;
            break;
        }
        read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);
    }

    /* Now switch on the mirror algorithm to determine if we should write the
     * degraded positions or not.
     */
    switch(siots_p->algorithm)
    {
        case MIRROR_RD:
            b_write_degraded = FBE_TRUE;
            break;

        case MIRROR_VR:
        case MIRROR_RD_VR:
        case MIRROR_COPY_VR:
        case MIRROR_VR_WR:
        case MIRROR_REMAP_RVR:
        case MIRROR_VR_BUF:
        case MIRROR_RB:
        case MIRROR_COPY:
        case MIRROR_PACO:
            b_write_degraded = (b_write_request == FBE_FALSE) ? FBE_TRUE : FBE_FALSE;
            break;

        case MIRROR_WR:
        case MIRROR_CORRUPT_DATA:
        case RG_ZERO:
            b_write_degraded = FBE_FALSE;
            break;

        default:
            /* Unsupported algorithm
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: siots_p: 0x%p unsupported algorithm: 0x%x \n",
                                 siots_p, siots_p->algorithm);
            return(status);
    }

    /* Now invoke the method to handle the degraded positions with the correct
     * parameters.
     */
    status = fbe_mirror_handle_degraded_positions(siots_p,
                                                  b_write_request,
                                                  b_write_degraded);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror: siots_p: 0x%p b_write_request: %d b_write_degraded: %d failed with status: 0x%x \n",
                             siots_p, b_write_request, b_write_degraded, status);
    }

    /* Return the execution status
     */
    return(status);
}
/*********************************************
 * end fbe_mirror_refresh_degraded_positions()
 *********************************************/

/*!***************************************************************************
 *          fbe_mirror_count_sgs_with_offset()
 *****************************************************************************
 *
 * @brief   Although there is a defined element size for mirror raid groups, it
 *          doesn't have the same meaning as striper or parity raid groups.
 *          In addition all data positions for a mirror raid group have the
 *          same sg.  Therefore we can't directly use fbe_raid_scatter_cache_to_bed
 *          to determine the size of the sgl for a request that has a non-zero
 *          host offset.
 *
 * @param   siots_p - this I/O
 * @param   sg_count_p - Pointer to sg count to populate
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  03/05/2010  Ron Proulx  - Created from fbe_raid_scatter_cache_to_bed
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_count_sgs_with_offset(fbe_raid_siots_t *siots_p,
                                              fbe_u32_t *sg_count_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_sg_element_with_offset_t    sg_desc;    /* track location in cache S/G list */
    fbe_sg_element_t               *src_sg_p;   /* current source S/G element */
    fbe_block_count_t                blks_remaining; /* unprocessed blocks in current src S/G element */
    fbe_block_size_t                bytes_per_blk;  /* size of block (bytes) */
    
    /* Generate the sg descriptor from the parent request.
     */
    status = fbe_raid_siots_setup_sgd(siots_p, &sg_desc);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror: failed to setup sgd: status = 0x%x, siots_p = 0x%p\n", status, siots_p);
        return  status;
    }

    /* Adjust the sg element based on the current offset.
     */
    fbe_raid_adjust_sg_desc(&sg_desc);

    /* Determine bytes per block and blocks remaining in current sg.
     */
    src_sg_p = sg_desc.sg_element;
    bytes_per_blk = FBE_RAID_SECTOR_SIZE(src_sg_p->address);
    blks_remaining = (src_sg_p->count - sg_desc.offset) / bytes_per_blk;

    /* Now invoke method that determines the number of sg elements required
     * including any extra for partial (start and end of list).
     */
    status = fbe_raid_sg_count_nonuniform_blocks(siots_p->parity_count,
                                                 &sg_desc,
                                                 bytes_per_blk,
                                                 &blks_remaining,
                                                 sg_count_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror: failed to count non-uniform blocks: status = 0x%x, "
                             "blks_remaining = 0x%llx, siots_p = 0x%p\n", 
                             status, (unsigned long long)blks_remaining,
                 siots_p);
        return  status;
    }
    
    /* Always return the execution status.
     */
    return(status);
}
/********************************************
 * end of fbe_mirror_count_sgs_with_offset()
 ********************************************/


/*!***************************************************************************
 *          fbe_mirror_plant_sg_with_offset()
 *****************************************************************************
 *
 * @brief   Although there is defined element size for mirror raid groups, it
 *          doesn't have the same meaning has striper or parity raid groups.
 *          In addition all data positions for a mirror raid group have the
 *          same trnasfer size.  Thus we cannot directly use 
 *          fbe_raid_scatter_sg_to_bed.  This method plants the host buffers
 *          into the allocated sgl.
 *
 * @param   siots_p - this I/O
 * @param   dst_info_p - Pointer to destination fru infomation
 * @param   dst_sgl_p - Pointer to sgl to plant host buffers into
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  03/05/2010  Ron Proulx  - Created from fbe_raid_scatter_sg_to_bed
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_plant_sg_with_offset(fbe_raid_siots_t *siots_p,
                                             fbe_raid_fru_info_t *dst_info_p,
                                             fbe_sg_element_t *dst_sgl_p)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_sg_element_with_offset_t    sg_desc;        /* track location in cache S/G list */
    fbe_block_size_t                bytes_per_blk;  /* num bytes per block in source S/G list */
    fbe_u32_t                       bytes_to_scatter;
    fbe_u16_t                       sg_elements_used;
    fbe_raid_sg_index_t             sg_index_used;
                              
    /* Generate the sg descriptor from the parent request and the
     * the number of bytes for each logical block.
     */
    status = fbe_raid_siots_setup_sgd(siots_p, &sg_desc);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror: failed to setup sgd: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return  status;
    }
    bytes_per_blk = FBE_RAID_SECTOR_SIZE(sg_desc.sg_element->address);

    if ((dst_info_p->blocks * bytes_per_blk) > FBE_U32_MAX)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror: bytes to scatter(0x%llx) exceeding 32bit limit, siots_p = 0x%p\n",
                             (unsigned long long)(dst_info_p->blocks * bytes_per_blk), siots_p);
        return  FBE_STATUS_GENERIC_FAILURE;
    }
    bytes_to_scatter = (fbe_u32_t)dst_info_p->blocks * bytes_per_blk;
        
    /*! Simply plant and clip the destination sgl with the host buffers.
     */
     status = fbe_raid_sg_clip_sg_list(&sg_desc,
                                       dst_sgl_p,
                                       bytes_to_scatter,
                                       &sg_elements_used);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror: failed to clip sg list: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return  status;
    }

    /* If we have exceeded the sgl alloc something is now seriously messed up.
     */
    sg_index_used = fbe_raid_memory_sg_count_index(sg_elements_used);
    if (dst_info_p->sg_index < sg_index_used)
    {
        /*! @note We have just corrupted raid memory!!  Need to modify
         *        clip sg list to take a size parameter!!
         */
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,
                             "mirror: plant sg with offset - siots: lba: %llx blks: 0x%llx Exceeded allocated sgl memory corrupted!! \n",
                             (unsigned long long)siots_p->parity_start,
                 (unsigned long long)siots_p->parity_count);

        status = FBE_STATUS_GENERIC_FAILURE;

        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "mirror: %s got unexpected error: status = 0x%x, siots_p = 0x%p\n",
                                 __FUNCTION__,
                                 status,
                                 siots_p);
            return  status;
        }
    }

    /* Always return the executions status.
     */
    return (status);
}
/********************************************
 * end of fbe_mirror_plant_sg_with_offset()
 ********************************************/

/*!***************************************************************************
 *          fbe_mirror_get_primary_fru_info()
 *****************************************************************************
 *
 * @brief   Simple helper method that extracts the primary fru infomation
 *          from an array of fru information.
 *
 * @param   siots_p - Pointer to request 
 * @param   fru_info_p - Array of fru infomation for all positions
 * @param   primary_info_pp - Address of pointer to update
 *
 * @return  status - Typically FBE_STATUS_OK
 *                   Other - Primary position was in array
 *
 * @author
 *  03/05/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_get_primary_fru_info(fbe_raid_siots_t *siots_p,
                                             fbe_raid_fru_info_t *fru_info_p,
                                             fbe_raid_fru_info_t **primary_info_pp)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t               width = fbe_raid_siots_get_width(siots_p);
    fbe_u32_t               primary_position = fbe_mirror_get_primary_position(siots_p);
    fbe_u32_t               index;                          

    /* Walk thru array and locate primary fru info
     */
    for (index = 0; index < width; index++)
    {
        if (fru_info_p[index].position == primary_position)
        {
            *primary_info_pp = fru_info_p;
            status = FBE_STATUS_OK;
            break;
        }
        fru_info_p++;
    }

    /* Always return execution status.
     */
    return(status);
}
/********************************************
 * end of fbe_mirror_get_primary_fru_info()
 ********************************************/

/*!***************************************************************************
 *          fbe_mirror_should_remap_be_requested()
 *****************************************************************************
 *
 * @brief   Based on the type of request, determine if we should request that
 *          the mirror group object send `verify' (a.k.a remap) request to:
 *          o Re-generate valid data from redundancy
 *          o Write those positions that require a remap which will force a
 *            re-assignment.
 *
 * @param   siots_p - Pointer to request that requires a remap
 *
 * @return  bool - FBE_TRUE - Request remap 
 *                 FBE_FALSE - Remap not required
 *
 * @author
 *  09/03/2009  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_bool_t fbe_mirror_should_remap_be_requested(fbe_raid_siots_t *siots_p)
{
    fbe_bool_t  b_remap_required = FBE_TRUE;
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* If we are doing Read only Background verify we will not send it to be 
     * remapped since we do not wish to fix errors. If we are doing any type
     * of verify we also do not send it to be remapped since those algorithms
     * will correct (or invalidate) the data.
     */
    if (fbe_raid_geometry_is_sparing_type(raid_geometry_p) ||
        fbe_raid_siots_is_read_only_verify(siots_p)        ||
        (siots_p->algorithm == MIRROR_VR)                  ||
        (siots_p->algorithm == MIRROR_COPY_VR)             ||
        (siots_p->algorithm == MIRROR_VR_BUF)                 )
    {
        /* Do not request a remap for the above request types.
         */
        b_remap_required = FBE_FALSE;
    }

    /* Return if a remap should be requested or not.
     */
    return(b_remap_required);
}
/***********************************************
 * end of fbe_mirror_should_remap_be_requested()
 ***********************************************/

/*!***************************************************************************
 *  fbe_mirror_get_fruts_error()
 *****************************************************************************
 *
 * @brief   Determine the overall status of a fruts chain.  Fruts chain get
 *          eboard sums any errors for all the fruts in the chain.  This method
 *          is responsible for determining the priority of the errors and
 *          reporting a status so that the caller can determine a recovery action.
 *
 * @param fruts_p - This is the chain to get status on.
 * @param eboard_p - This is the structure to save the results in.
 *
 * @return  fbe_raid_fru_error_status_t - Following values can be returned:
 *              o FBE_RAID_FRU_ERROR_STATUS_SUCCESS
 *              o FBE_RAID_FRU_ERROR_STATUS_DEAD
 *              o FBE_RAID_FRU_ERROR_STATUS_WAITING
 *              o FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN
 *              o FBE_RAID_FRU_ERROR_STATUS_ERROR
 *              o FBE_RAID_FRU_ERROR_STATUS_ABORTED
 *              o FBE_RAID_FRU_ERROR_STATUS_EXPIRED
 *              o FBE_RAID_FRU_ERROR_STATUS_UNEXPECTED
 *              o FBE_RAID_FRU_ERROR_STATUS_RETRY
 *
 * @author
 *  09/04/2009 - Ron Proulx  - Created.
 *
 ******************************************************************************/

fbe_raid_fru_error_status_t fbe_mirror_get_fruts_error(fbe_raid_fruts_t *fruts_p,
                                                       fbe_raid_fru_eboard_t *eboard_p)
{
    fbe_raid_fru_error_status_t fru_error_status = FBE_RAID_FRU_ERROR_STATUS_SUCCESS;
    fbe_raid_siots_t           *siots_p = fbe_raid_fruts_get_siots(fruts_p);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* These extractions are required here.. Reason: When we set the siots waiting bit, 
     * it will be possible for the I/O to get completed from another context, 
     * so we cannot touch the I/O after setting waiting. 
     * ref: end of this function which tries to print siots when in WAITING state... 
     * This causes an access violation.. 
     */
    fbe_u32_t siots_in_waiting_state = fbe_raid_memory_pointer_to_u32(siots_p);
    fbe_object_id_t raid_object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

    /* Note that we count the media error detected even if the request
     * was aborted. Sum up the errors encountered. 
     */
    if (fbe_raid_fruts_chain_get_eboard(fruts_p, eboard_p) != FBE_TRUE)
    {
        /* If there is problem with the error board report unexpected error
         */
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: siots lba: 0x%llx blks: 0x%llx failed to get eboard\n",
                             (unsigned long long)siots_p->start_lba,
                 (unsigned long long)siots_p->xfer_count);
        return(FBE_RAID_FRU_ERROR_STATUS_UNEXPECTED);
    }
    
    /*! If either the iots or siots has been aborted, flag the fruts as aborted.
     */
    if (fbe_raid_siots_is_aborted(siots_p))
    {
        fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
        fbe_payload_block_operation_opcode_t opcode;
        fbe_raid_iots_get_opcode(iots_p, &opcode);

        /* Media modify type operations will need to decide for themselves if they want 
         * to abort the request. 
         * Media modify operations are different since always allowing them to 
         * abort at this point could cause parity/mirror inconsistencies. 
         */
        if (!fbe_payload_block_operation_opcode_is_media_modify(opcode))
        {
            return(FBE_RAID_FRU_ERROR_STATUS_ABORTED);
        }
    }
        
    /* Now increment the total (i.e. for all fruts associated with this siots)
     * media error count for this siots.  Soft errors indicate that the request
     * was successful with a remap.
     */ 
    if ((eboard_p->soft_media_err_count > 0)  ||
        ((eboard_p->hard_media_err_count > 0) &&
         (eboard_p->menr_err_bitmap != eboard_p->hard_media_err_bitmap) ) )
    {
        siots_p->media_error_count++;

        /* If we took a media error during COPY, we should invalidate the sectors.
         */
        if ((eboard_p->hard_media_err_bitmap > 0) && 
            (siots_p->algorithm == MIRROR_PACO || siots_p->algorithm == MIRROR_COPY))
        {
            return FBE_RAID_FRU_ERROR_STATUS_INVALIDATE;
        }

        /* If we took a media error, Set a flag and when this request completes back to
         * the object, the object will mark the chunk. 
         * For the case of a hard media error, only submit a request to remap if there 
         * were some errors that weren't MENR. 
         */
        /* If we are doing Read only Background verify we will not send it to be remapped
         * since we do not wish to fix errors. 
         * If we are doing a verify also do not send it to be remapped, since it is 
         * already marked. 
         */
        if (fbe_mirror_should_remap_be_requested(siots_p))
        {
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
        return FBE_RAID_FRU_ERROR_STATUS_ABORTED;
    }

    /* Handle case where the monitor has run and we haven't exhausted retries.
     * If the monitor has set the `received
     * continue' flag, we need to refresh our degraded status.
     */
     if ((eboard_p->retry_err_count > 0) && 
         fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_RECEIVED_CONTINUE))
     {
         fbe_status_t                status = FBE_STATUS_OK;
         fbe_raid_position_bitmask_t degraded_bitmask = 0;

        /* Refresh this siots view of the iots.
         */
        status = fbe_mirror_refresh_degraded_positions(siots_p, fruts_p);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            /* Report an error and set the error to unexpected
             */
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: siots_p: 0x%p refresh degraded failed with status: 0x%x\n",
                                 siots_p, status);
            return(FBE_RAID_FRU_ERROR_STATUS_UNEXPECTED);
        }

        /* Get the updated degraded bitmask
         */
        fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    
        /* Handle case where we were retrying and the drive we retried to is gone.
         * After we get the continue we will turn this "retryable" error into a 
         * dead error if the drive is now degraded.
         */
        if ((eboard_p->retry_err_bitmap & degraded_bitmask) != 0)
        {
            fbe_raid_position_bitmask_t new_dead_bitmap;
            fbe_u32_t new_dead_count;
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                        "mirror: siots: 0x%x retry positions dead ob: 0x%x retry_b: 0x%x dead_b: 0x%x\n",
                                        fbe_raid_memory_pointer_to_u32(siots_p),
                                        fbe_raid_geometry_get_object_id(raid_geometry_p),
                                        eboard_p->retry_err_bitmap, eboard_p->dead_err_bitmap);
            /* Determine overlap between retryable errors and known dead positions. 
             * Then take this bitmap out of the retryable errors, and add it into the dead errors. 
             */
            new_dead_bitmap = eboard_p->retry_err_bitmap & degraded_bitmask;
            new_dead_count = fbe_raid_get_bitcount(new_dead_bitmap);
            eboard_p->retry_err_bitmap &= ~new_dead_bitmap;
            eboard_p->retry_err_count -= new_dead_count;
            eboard_p->dead_err_bitmap |= new_dead_bitmap;
            eboard_p->dead_err_count += new_dead_count;
        }

        /* Now fall-thru and process dead/and-or retry status
         */
    }

    /* Note that dead errors have priority over retryable errors.
     */
    if (eboard_p->dead_err_count > 0)
    {

        fbe_status_t    status;

        /* Simply invoke the method that handles dead errors.
         */
        status = fbe_mirror_process_dead_fruts_error(siots_p, 
                                                     eboard_p,
                                                     &fru_error_status);
        if (status != FBE_STATUS_OK)
        {
            /* Report an error and set the error to unexpected
             */
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: siots lba: 0x%llx blks: 0x%llx process dead failed with status: 0x%x\n",
                                 (unsigned long long)siots_p->start_lba,
                 (unsigned long long)siots_p->xfer_count,
                 status);
            fru_error_status = FBE_RAID_FRU_ERROR_STATUS_UNEXPECTED;
        }
    }
    else if (eboard_p->retry_err_count > 0)
    {
        fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
        if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_QUIESCE))
        {
            if (fbe_raid_siots_is_monitor_initiated_op(siots_p) ||
                !fbe_raid_siots_is_able_to_quiesce(siots_p))
            {
                fbe_payload_block_operation_opcode_t opcode;
                fbe_raid_iots_get_opcode(iots_p, &opcode);
                if(fbe_payload_block_operation_opcode_is_media_modify(opcode))
                {
                    fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_INCOMPLETE_WRITE);

                    /* we treat the retryable error as dead error here */
                    iots_p->dead_err_bitmap = eboard_p->retry_err_bitmap;
                }

                /* A monitor operation cannot quiesce so we will return an error.
                 */
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                            "mirror: siots: 0x%x metadata retry_b: 0x%x return dead on quiesce.\n",
                                            fbe_raid_memory_pointer_to_u32(siots_p),
                                            eboard_p->retry_err_bitmap);
                fru_error_status = FBE_RAID_FRU_ERROR_STATUS_DEAD;
            }
            else
            {
                /* If we have a retryable error and we are quiescing, then we should not 
                 * execute the retry now.  Simply wait for the continue. 
                 */
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                            "mirror: siots: 0x%x retry errs wait qsce retry_b: 0x%x\n",
                                            fbe_raid_memory_pointer_to_u32(siots_p),
                                             eboard_p->retry_err_bitmap);
                fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_QUIESCED);
                fru_error_status = FBE_RAID_FRU_ERROR_STATUS_WAITING;
            }
        }
        else
        {
            /* Otherwise perform the retry.
             */
            fru_error_status = FBE_RAID_FRU_ERROR_STATUS_RETRY;
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                        "mirror: siots: 0x%x retry errs retry err count: %d mask: 0x%x\n",
                                        fbe_raid_memory_pointer_to_u32(siots_p),
                                        eboard_p->retry_err_count, eboard_p->retry_err_bitmap);
        }
    }

    if ((fbe_raid_siots_is_monitor_initiated_op(siots_p) || !fbe_raid_siots_is_able_to_quiesce(siots_p))
        && (fru_error_status == FBE_RAID_FRU_ERROR_STATUS_WAITING))
    {
        fbe_raid_library_object_trace(raid_object_id, 
                                      FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                      "mirror: siots: 0x%x cannot return waiting on monitor op\n",
                                      siots_in_waiting_state);
    }
    /* Always return the fruts chain error status.
     */
    return(fru_error_status);
}
/* end fbe_mirror_get_fruts_error() */

/*!**************************************************************
 * fbe_mirror_handle_fruts_error()
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

fbe_raid_state_status_t fbe_mirror_handle_fruts_error(fbe_raid_siots_t *siots_p,
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
        /* Metadata operation saw a drive go away.
         */
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_dead_error);
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
    else if (fruts_error_status == FBE_RAID_FRU_ERROR_STATUS_WAITING)
    {
        /* Some number of drives are not available.  Wait for monitor to make a decision.
         */
        return FBE_RAID_STATE_STATUS_WAITING;
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
 * end fbe_mirror_handle_fruts_error()
 **************************************/

/*!***************************************************************************
 *          fbe_mirror_process_dead_fruts_error_continue_received()
 *****************************************************************************
 *
 * @brief   We have encountered a dead error for 1 or more positions for a read
 *          or write type of request.  The monitor has acknowledged at least 1
 *          dead position but it might still be out of sync (a new position 
 *          encounters a dead error).
 *
 * @param   siots_p - Pointer to siots that encounter write error
 * @param   eboard_p - Pointer to eboard that will help determine what actions
 *                     to take
 * @param   fru_error_status_p - Pointer to fru error status to populate
 *
 * @return  status - Typically FBE_STATUS_OK    
 *
 * @author
 *  8/11/2010   Ron Proulx  - Copied from fbe_parity_fruts_handle_dead_error
 *
 *****************************************************************************/
static fbe_status_t fbe_mirror_process_dead_fruts_error_continue_received(fbe_raid_siots_t *siots_p, 
                                                                   fbe_raid_fru_eboard_t *eboard_p,
                                                                   fbe_raid_fru_error_status_t *fru_error_status_p)

{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   width = fbe_raid_siots_get_width(siots_p);
    fbe_raid_iots_t            *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_position_bitmask_t degraded_bitmask;
    fbe_raid_position_bitmask_t disabled_bitmask;
    fbe_raid_position_bitmask_t new_degraded_bitmask = 0;
    fbe_raid_position_bitmask_t returning_bitmask = 0;

    /* For debug get the degraded bitmask before and after updating our 
     * degraded information.
     */
    fbe_raid_siots_get_disabled_bitmask(siots_p, &disabled_bitmask);
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);

    /* Determine the updated degraded postions (don't update the position map)
     */
    status = fbe_mirror_determine_degraded_positions(siots_p, FBE_FALSE);
    fbe_raid_siots_get_degraded_bitmask(siots_p, &new_degraded_bitmask);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to determine degraded position: siots_p = 0x%p"
                             "status = 0x%x\n",
                             siots_p, status);
        *fru_error_status_p = FBE_RAID_FRU_ERROR_STATUS_UNEXPECTED; 
        return  status;
    }

    /* Generate a bitmask for those positions that might have returned
     */
    returning_bitmask = eboard_p->dead_err_bitmap & ~new_degraded_bitmask;

    /* If any postions have returned log some debug information
     */
    if (returning_bitmask != 0)
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "mirror: cont rcvd siots 0x%x lba/bl: 0x%llx/0x%llx fl: 0x%x dsbld b: 0x%x\n",
                                    fbe_raid_memory_pointer_to_u32(siots_p),
                    (unsigned long long)siots_p->start_lba,
                    (unsigned long long)siots_p->xfer_count, 
                                    fbe_raid_siots_get_flags(siots_p),
                    disabled_bitmask);
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "mirror: siots: 0x%x cont rcvd dead_b: 0x%x cont_b: 0x%x n_c_b:0x%x ret_b: 0x%x\n",
                                    fbe_raid_memory_pointer_to_u32(siots_p), 
                                    eboard_p->dead_err_bitmap,
                    siots_p->continue_bitmask, 
                                    siots_p->needs_continue_bitmask, 
                                    returning_bitmask /* Bitmask of drives that have returned */);
    }

    /* Now handle the case where the monitor is trying to quiesce.
     * In this case we don't want to continue.
     */
    if (fbe_raid_iots_is_flag_set(iots_p, FBE_RAID_IOTS_FLAG_QUIESCE))
    {
        /* If this is a metadata or background operations we cannot wait.  
         * The monitor needs to quiesce and this operation needs to finish 
         * before we can quiesce.  Therefore fail the request with a dead
         * error.
         */
        if (fbe_raid_siots_is_metadata_operation(siots_p) ||
            fbe_raid_siots_is_background_request(siots_p)    )
        {
            *fru_error_status_p = FBE_RAID_FRU_ERROR_STATUS_DEAD; 
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                        "mirror quiesce siots: 0x%x md or bg op l/bl: 0x%llx/0x%llx\n",
                                        fbe_raid_memory_pointer_to_u32(siots_p), 
                                        (unsigned long long)siots_p->start_lba,
                    (unsigned long long)siots_p->xfer_count);

        }
        else
        {
            /* We have an error, but the monitor is trying to quiesce. 
             * We already received a continue, but need to pick up other changes. 
             * Just return waiting to give the monitor a chance to quiesce.
             */
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                        "mir prc dead fru: siots: 0x%x quiesce and cond rec l/bl: 0x%llx/0x%llx\n",
                                        fbe_raid_memory_pointer_to_u32(siots_p), 
                                        (unsigned long long)siots_p->start_lba,
                    (unsigned long long)siots_p->xfer_count);
            *fru_error_status_p = FBE_RAID_FRU_ERROR_STATUS_WAITING; 
            fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_WAITING_SHUTDOWN_CONTINUE);
        }
    } /* end if quiesce flag set*/
    else
    {
        fbe_raid_position_bitmask_t degraded_bitmask;
        fbe_u32_t                   degraded_count;

        /* Else the monitor has updated it's view of the dead positions
         * Use the `disabled' bitmask to updated the degraded positions.
         * Then determine which of (3) status we should return:
         *  o FBE_RAID_FRU_ERROR_STATUS_RETRY - The siots can be retried as is
         *  o FBE_RAID_FRU_ERROR_STATUS_ERROR - May need to switch read positions etc
         *  o FBE_RAID_FRU_ERROR_STATUS_DEAD  - Too many positions are dead
         */
        fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
        degraded_count = fbe_raid_get_bitcount(degraded_bitmask); 

        /* If all the dead positions hare now back, simply retry the request
         */
        if (eboard_p->dead_err_bitmap == returning_bitmask)
        {
            /* The drive or drives is not known to be degraded.  
             * This drive could have come back. 
             * Retry any dead errors we received. 
             */ 
            eboard_p->retry_err_bitmap |= eboard_p->dead_err_bitmap;
            eboard_p->dead_err_bitmap = 0;
            eboard_p->dead_err_count = 0;
            eboard_p->retry_err_count = fbe_raid_get_bitcount(eboard_p->retry_err_bitmap);
            *fru_error_status_p = FBE_RAID_FRU_ERROR_STATUS_RETRY;
        }
        else if (degraded_count >= width)
        {
            *fru_error_status_p = FBE_RAID_FRU_ERROR_STATUS_SHUTDOWN;
        }
        else
        {
            if ((eboard_p->dead_err_bitmap & degraded_bitmask) == eboard_p->dead_err_bitmap)
            {
                /* We maybe able to recover using a different read position
                 * Return status that will allow the mirror code to handle the
                 * new dead positions.
                 */
                *fru_error_status_p = FBE_RAID_FRU_ERROR_STATUS_ERROR;
            }
            else
            {
                /* We have dead errors that are now back.  Let's retry them.
                 */ 
                eboard_p->retry_err_bitmap |= eboard_p->dead_err_bitmap & ~degraded_bitmask;
                eboard_p->retry_err_count = fbe_raid_get_bitcount(eboard_p->retry_err_bitmap);
                eboard_p->dead_err_bitmap &= degraded_bitmask;
                eboard_p->dead_err_count = fbe_raid_get_bitcount(eboard_p->dead_err_bitmap);
                *fru_error_status_p = FBE_RAID_FRU_ERROR_STATUS_RETRY;
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                            "mirror: retry dead errors opcode: 0x%x pos_b: 0x%x dead_b: 0x%x siots: %p\n",
                                            iots_p->current_opcode, eboard_p->retry_err_bitmap, eboard_p->dead_err_bitmap, siots_p);
            }
        }
    } /* end else if not in quiesce */

    /* Always return the execution status
     */
    return(status);
}
/* end of fbe_mirror_process_dead_fruts_error_continue_received() */

/*!***************************************************************************
 *          fbe_mirror_process_dead_fruts_error()
 *****************************************************************************
 *
 * @brief   We have encountered a dead error for 1 or more positions for a read
 *          or write type of request.  We will either return `waiting', 
 *          `unexpected' or `ok to continue'.
 *
 * @param   siots_p - Pointer to siots that encounter write error
 * @param   eboard_p - Pointer to eboard that will help determine what actions
 *                     to take
 * @param   fru_error_status_p - Pointer to fru error status to populate
 *
 * @return  status - Typically FBE_STATUS_OK    
 *
 * @author
 *  8/10/2010   Ron Proulx  - Copied from fbe_parity_fruts_handle_dead_error
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_process_dead_fruts_error(fbe_raid_siots_t *siots_p, 
                                                 fbe_raid_fru_eboard_t *eboard_p,
                                                 fbe_raid_fru_error_status_t *fru_error_status_p)

{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   width = fbe_raid_siots_get_width(siots_p);
    fbe_raid_geometry_t        *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_raid_iots_t            *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_payload_block_operation_opcode_t opcode;
    fbe_bool_t                  b_continue_received = fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_RECEIVED_CONTINUE);
    fbe_raid_position_bitmask_t new_dead_bitmask = 0;         /* Bitmask of positions that monitor hasn't seen go dead */
    fbe_raid_position_bitmask_t disabled_bitmask = 0;
    
    /* We don't expect to be here if no dead positions were encountered
     */
    if (RAID_COND(eboard_p->dead_err_bitmap == 0))
    {
        /* Log an error and return
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "mirror: siots 0x%x lba: 0x%llx blks: 0x%llx flags: 0x%x \n",
                                    fbe_raid_memory_pointer_to_u32(siots_p),
                                    (unsigned long long)siots_p->start_lba,
                        (unsigned long long)siots_p->xfer_count,
                        fbe_raid_siots_get_flags(siots_p));

        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "siots: 0x%x dead bitmap is 0 c_bitmask: 0x%x n_c_bitmask: 0x%x \n",
                                    fbe_raid_memory_pointer_to_u32(siots_p), 
                                    siots_p->continue_bitmask,
                    siots_p->needs_continue_bitmask);

        *fru_error_status_p = FBE_RAID_FRU_ERROR_STATUS_UNEXPECTED; 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! @note By this time any dead positions encountered have been added to
     *        the degraded bitmask.
     */
    new_dead_bitmask = eboard_p->dead_err_bitmap & ~(siots_p->continue_bitmask);
    fbe_raid_siots_get_disabled_bitmask(siots_p, &disabled_bitmask);
    fbe_raid_siots_get_opcode(siots_p, &opcode);
    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                "mir proc dead siots: 0x%x  iots op: 0x%x cnt_rcvd: %d wdth: %d\n",
                                fbe_raid_memory_pointer_to_u32(siots_p),
                    opcode, b_continue_received, width);

    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                "siots: 0x%x l/bl: 0x%llx/0x%llx fl: 0x%x disabled bitmask: 0x%x\n",
                                fbe_raid_memory_pointer_to_u32(siots_p),
                (unsigned long long)siots_p->start_lba,
                (unsigned long long)siots_p->xfer_count, 
                                fbe_raid_siots_get_flags(siots_p),
                disabled_bitmask);

    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                "siots: 0x%x: dead bitmask: 0x%x siots c_bitmask: 0x%x n_c_bitmask:0x%x new dead bitmask:0x%x \n",
                                fbe_raid_memory_pointer_to_u32(siots_p),
                    eboard_p->dead_err_bitmap,
                    siots_p->continue_bitmask, 
                                siots_p->needs_continue_bitmask, 
                                new_dead_bitmask /* New bits monitor doesn't know about. */);

    /* If we received a continue then we never wait.  We will retry any
     * positions that took unretryable errors and are not now degraded. 
     */ 
    if (b_continue_received == FBE_TRUE)
    {
        /* Invoke method to handle the monitor acknowledging (1) or more dead
         * positions.
         */
        status = fbe_mirror_process_dead_fruts_error_continue_received(siots_p, 
                                                                       eboard_p,
                                                                       fru_error_status_p);
        /* Return now if an unexpected error occurred
         */
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "mirror: failed to process dead fruts error: status: 0x%x, siots_p = 0x%p\n",
                                        status, siots_p);
            return  status;
        }
    }
    else
    {
        /* Else if we have not received a `continue'.  Therefore we will either
         * wait for the monitor to acknowledge the dead positions or
         * immediately return `dead' for background or metadata operations.
         */
        if (fbe_raid_siots_is_monitor_initiated_op(siots_p) ||
            !fbe_raid_siots_is_able_to_quiesce(siots_p) ||
            fbe_raid_geometry_no_object(raid_geometry_p))
        {
            /* We cannot wait on background requests because the monitor
             * is waiting for the request to complete and therefore cannot
             * process the new dead position (i.e. restart this request).
             * Thus we must fail background request with a dead error.
             *
             * We also do not wait for a continue for raw mirror I/O. Raw mirrors
             * do not provide an object interface and hence do not have a monitor.
             */

            *fru_error_status_p = FBE_RAID_FRU_ERROR_STATUS_DEAD; 

            /* For media modify monitor operations set INCOMPLETE_WRITE flag 
             * and dead_err_bitmap in IOTS.
             */
            if(fbe_payload_block_operation_opcode_is_media_modify(opcode))
            {
                fbe_raid_iots_set_flag(iots_p, FBE_RAID_IOTS_FLAG_INCOMPLETE_WRITE);

                iots_p->dead_err_bitmap = eboard_p->dead_err_bitmap;
            }

            if (fbe_raid_geometry_no_object(raid_geometry_p))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                            "mir prc dead: raw mirror opcode: 0x%x siots lba: 0x%llx blks: 0x%llx siots: 0x%x\n",
                                            opcode,
                        (unsigned long long)siots_p->start_lba,
                        (unsigned long long)siots_p->xfer_count,
                        fbe_raid_memory_pointer_to_u32(siots_p));                
            }
            else
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                            "mir prc dead: background opcode: 0x%x siots lba: 0x%llx blks: 0x%llx siots: 0x%x\n",
                                            opcode,
                        (unsigned long long)siots_p->start_lba,
                        (unsigned long long)siots_p->xfer_count,
                        fbe_raid_memory_pointer_to_u32(siots_p));                
            }

        }
        else
        {
            /* Else mark that a continue is needed for this position.
             * We wait in the calling state until the raid group
             * monitor detects and processes the new dead position.
             */
            siots_p->needs_continue_bitmask |= new_dead_bitmask;

            /* There are new drives marked as dead in the group. Return 
             * FBE_RAID_FRU_ERROR_STATUS_WAITING so that this is also
             * placed in the shutdown queue.
             */
            *fru_error_status_p = FBE_RAID_FRU_ERROR_STATUS_WAITING; 
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                        "mirror: wait continue opcode: 0x%x siots lba: 0x%llx blks: 0x%llx siots: 0x%x\n",
                                        opcode, siots_p->start_lba, siots_p->xfer_count, fbe_raid_memory_pointer_to_u32(siots_p));
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                        "mirror:  dead Bits: 0x%x c_b: 0x%x n_c_b: 0x%x nw_dead_b: 0x%x siots: 0x%x\n",
                                         eboard_p->dead_err_bitmap, siots_p->continue_bitmask, 
                                         siots_p->needs_continue_bitmask, 
                                         /* New bits we don't know about. */
                                         (eboard_p->dead_err_bitmap & ~(siots_p->continue_bitmask)),
                                        fbe_raid_memory_pointer_to_u32(siots_p));
            fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_WAITING_SHUTDOWN_CONTINUE);

        } /* end else if not a monitor request */

    } /* end else if monitor hasn't sent a continue */

    /* Trace the status returned.  Can't trace if we are waiting since the monitor might have completed the I/O.
     */
    if (*fru_error_status_p != FBE_RAID_FRU_ERROR_STATUS_WAITING)
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_INFO,
                                    "mir prc dead complete: siots: 0x%x lba/bl:0x%llx/0x%llx fru error status: 0x%x\n",
                                    fbe_raid_memory_pointer_to_u32(siots_p), 
                                    siots_p->start_lba, siots_p->xfer_count, *fru_error_status_p);
    }
    /* Always return the execution status
     */
    return(status);
}
/* end of fbe_mirror_process_dead_fruts_error() */

/*!***************************************************************************
 *          fbe_mirror_is_sufficient_fruts()
 *****************************************************************************
 *
 * @brief   Based on the algorithm determine if there are sufficient
 *          read, read2 or write fruts to continue.  This doesn't
 *          imply that the raid group is shutdown simply that
 *          there is no way to complete the request.
 *
 * @param   siots_p - Pointer to siots
 *
 * @return  bool - FBE_TRUE - Ok to continue
 *                 FBE_FALSE - Fail the request
 *
 * @author
 *  12/20/2009  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_bool_t fbe_mirror_is_sufficient_fruts(fbe_raid_siots_t *siots_p)
{
    fbe_bool_t  ok_to_continue = FBE_TRUE;
    fbe_raid_fruts_t           *read_fruts_p = NULL;
    fbe_raid_fruts_t           *write_fruts_p = NULL;
    fbe_u32_t   read_count;
    fbe_u32_t   write_count;

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    read_count = fbe_raid_fruts_count_active(siots_p, read_fruts_p);
    write_count = fbe_raid_fruts_count_active(siots_p, write_fruts_p);


    /* Switch on the algorithm.
     */
    switch(siots_p->algorithm)
    {
        case MIRROR_RD:
            /* Need be at least (1) read fruts.
             */
            if (read_count < 1)
            {
                ok_to_continue = FBE_FALSE;
            }
            break;

        case MIRROR_VR:
        case MIRROR_RD_VR:
        case MIRROR_COPY_VR:
        case MIRROR_VR_WR:
        case MIRROR_REMAP_RVR:
        case MIRROR_VR_BUF:
            /* Need be at least (1) read/write  fruts
             */
            if (read_count + write_count < 1)
            {
                ok_to_continue = FBE_FALSE;
            }
            break;

        case MIRROR_WR:
        case MIRROR_CORRUPT_DATA:
        case RG_ZERO:
            /* Need at least (1) write fruts.
             */
            if (write_count < 1)
            {
                ok_to_continue = FBE_FALSE;
            }
            break;

        case MIRROR_RB:
        case MIRROR_COPY:
        case MIRROR_PACO:
            /* Need (1) read and (1) write fruts.
             */
            if ((read_count < 1)  ||
                (write_count < 1)    )
            {
                ok_to_continue = FBE_FALSE;
            }
            break;

        default:
            /* Fail the request (can only used set unexpected directly from
             * the state machine, so use siots trace here).
             */
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "mirror: sufficient fruts - unsupported algorithm: 0x%x \n", 
                                 siots_p->algorithm);
            ok_to_continue = FBE_FALSE;
            break;
    }
    
    /* Always return the `ok to continue' flag.
     */
    return(ok_to_continue);
}
/* end fbe_mirror_is_sufficient_fruts() */

/*!*****************************************************************
 *          fbe_mirror_reinit_fruts_for_mining()
 *******************************************************************
 *
 * @brief   Re-initialize both the read and write fruts chains for a
 *          mirror rebuild, verify request.  This method is invoked when we
 *          either enter or continue mining mode.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return - fbe_status_t - Typicall FBE_STATUS_OK
 *
 * @author
 *  01/07/2010  Ron Proulx  - Created
 *
 *******************************************************************/
fbe_status_t fbe_mirror_reinit_fruts_for_mining(fbe_raid_siots_t * siots_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_raid_fruts_t   *read_fruts_p = NULL;
    fbe_raid_fruts_t   *write_fruts_p = NULL;

    /* If we are not in region mode something is wrong
     */
    if (!fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE))
    {
        /* We let the calling routine execute the state transition.  So simply
         * trace the failure information.
         */
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: siots: 0x%llx 0x%llx flags: 0x%x expected to be in mining mode\n",
                             (unsigned long long)siots_p->start_lba,
                 (unsigned long long)siots_p->xfer_count,
                 fbe_raid_siots_get_flags(siots_p));
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* For each read fruts use the previously set parity count to populate
     * the fruts.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    status = fbe_raid_fruts_for_each(0,
                                  read_fruts_p,
                                  fbe_raid_fruts_reset,
                                  (fbe_u64_t) siots_p->parity_count,
                                  (fbe_u64_t) FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to set fruts to single mode for siots_p = 0x%p,  "
                             "status = 0x%x\n",
                             siots_p,
                             status);
        return  status;
    }

    /* For each write fruts use the previously set parity count to populate
     * the fruts.
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    status = fbe_raid_fruts_for_each(0,
                                  write_fruts_p,
                                  fbe_raid_fruts_reset,
                                  (fbe_u64_t) siots_p->parity_count,
                                  (fbe_u64_t) FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE);
    
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to re-set fruts chain: write_fruts_p 0x%p to single mode"
                             "for siots_p 0x%p, status = 0x%x\n",
                             write_fruts_p, siots_p, status);
        return  status;
    }

    /* Always return execution status.
     */
    return(status);
}
/* end fbe_mirror_reinit_fruts_for_mining() */
 
/*!***************************************************************************
 *          fbe_mirror_setup_reconstruct_request()
 *****************************************************************************
 *
 * @brief   Populate the reconstruct request from the read (valid read data)
 *          and write (positions that need to be rebuilt) fruts chains.
 *
 * @param   siots_p - ptr to the fbe_raid_siots_t
 * @param   reconstruct_p - Pointer to rebuild or verify request to populate
 *
 * @return  status -  
 *
 * @author
 *  04/05/2010  Ron Proulx  - Updated to generate `valid' and `needs rebuild'
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_setup_reconstruct_request(fbe_raid_siots_t * siots_p,
                                                  fbe_xor_mirror_reconstruct_t *reconstruct_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           width = fbe_raid_siots_get_width(siots_p);
    fbe_raid_fruts_t   *read_fruts_p = NULL;
    fbe_u32_t           reconstruct_src_count = 0;
    fbe_raid_fruts_t   *write_fruts_p = NULL;
    fbe_u32_t           reconstruct_dst_count = 0;
    fbe_raid_position_bitmask_t total_bitmask = 0;
    fbe_raid_position_bitmask_t expected_bitmask;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_raid_siots_get_opcode(siots_p, &opcode);

    /* Initialize the reconstruct structure.
     */
    status = fbe_raid_util_get_valid_bitmask(width, &expected_bitmask);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror: failed to get valid bitmask: status = 0x%x, width = 0x%x, "
                             "expected_bitmask = 0x%x, siots_p = 0x%p\n",
                             status, width, expected_bitmask, siots_p);
        return  status;
    }
    fbe_zero_memory((void *)reconstruct_p, sizeof(*reconstruct_p));

    /* Get the read and write chains.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    /* First initialize the position(s) we are rebuild from
     */
    while(read_fruts_p != NULL)
    {
        /* Validate position and index
         */
        if ((read_fruts_p->position >= width) ||
            (reconstruct_src_count >= width)     )
        {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: reconstruct extent setup - read position: %d invalid for width: 0x%x src count: 0x%x\n",
                                 read_fruts_p->position, width, reconstruct_src_count);
            return(FBE_STATUS_GENERIC_FAILURE);
        }
        
        /* Setup the reconstruct information for this position.
         *  o Set the bit for this position in the `valid' bitmask
         *  o Set the sg pointer for this position
         *  o Increment the number of reconstruct source count 
         */
        reconstruct_p->sg_p[read_fruts_p->position] = fbe_raid_fruts_return_sg_ptr(read_fruts_p);

        if (read_fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID)
        {
            reconstruct_p->needs_rebuild_bitmask |= (1 << read_fruts_p->position);
        }
        else
        {
            reconstruct_p->valid_bitmask |= (1 << read_fruts_p->position);
            reconstruct_src_count++;
        }
                
        /* Goto the next read position.
         */
        read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);
    }

    /* Validate source count.
     */
    if ((reconstruct_src_count == 0) || (reconstruct_src_count > width))
    {
        /* This is unexpected.
         */
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: reconstruct extent setup - Found: 0x%x source positions expected: 0x%x\n",
                             reconstruct_src_count, siots_p->data_disks);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Next, setup the drive(s) to be rebuilt.
     */
    while(write_fruts_p != NULL)
    {
        /* Validate position and destination count.
         */
        if ((write_fruts_p->position >= width)                 ||
            (reconstruct_dst_count >= FBE_XOR_NUM_REBUILD_POS)    )
        {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: reconstruct extent setup - write position: %d invalid for width: 0x%x dst count: 0x%x\n",
                                 write_fruts_p->position, width, reconstruct_dst_count);
            return(FBE_STATUS_GENERIC_FAILURE);
        }

        /* Setup the reconstruct information for this position.
         *  o Set the bit for this position in the `needs rebuild' bitmask
         *  o Set the sg pointer for this position
         *  o Increment the number of reconstruct destination count 
         */
        reconstruct_p->needs_rebuild_bitmask |= (1 << write_fruts_p->position);
        reconstruct_p->sg_p[write_fruts_p->position] = fbe_raid_fruts_return_sg_ptr(write_fruts_p);
        reconstruct_dst_count++;
                                                            
        /* Goto the next write position.
         */
        write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);
    }
        
    /* If the read bitmask overlaps with the write something is wrong.
     */
    if ((reconstruct_p->valid_bitmask & reconstruct_p->needs_rebuild_bitmask) != 0)
    {
        /* This is unexpected.
         */
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: recontruct extent setup - Overlapping read: 0x%x and write bitmask: 0x%x \n",
                             reconstruct_p->valid_bitmask, reconstruct_p->needs_rebuild_bitmask);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* The following checks are not valid with degraded invalidation */
    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE && siots_p->data_disks < width)
    {
        return status;
    }

    /* Based on the algorithm validate the request.
     */
    switch(siots_p->algorithm)
    {
        case MIRROR_COPY:
        case MIRROR_PACO:
            /* The width should be (2) and there should be exactly (1) source
             * and (1) destination.
             */
            if ((width != 2)                 ||
                (reconstruct_src_count != 1) ||
                (reconstruct_dst_count != 1)    )
            {
                /* This is unexpected.
                 */
                fbe_raid_siots_trace(siots_p,
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: reconstruct extent setup - alg: 0x%x width: %d or source: %d or dest: %d unexpected\n",
                                     siots_p->algorithm, width, reconstruct_src_count, reconstruct_dst_count);
                return(FBE_STATUS_GENERIC_FAILURE);
            }
            break;

        case MIRROR_RB:
            /* The destination count (i.e. number of positions to rebuild) must be
             * the (width - data_disks).  But before we issue the writes we will use the
             * iots `rebuild' bitmask to determine which positions are actually written.
             */
            if (reconstruct_dst_count != (width - reconstruct_src_count))
            {
                /* This is unexpected.
                 */
                fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: reconstruct extent setup - Expected: 0x%x rebuild positions.  Actual: 0x%x \n",
                             (width - reconstruct_src_count), reconstruct_dst_count);
                return(FBE_STATUS_GENERIC_FAILURE);
            }
            break;

        default:
            /* For all other algorithm there are no additional checks
             */
            break;

    } /* based on algorithm there maybe additional checks*/
        
    /* If the total bitmask doesn't match the expected something is wrong.
     */
    total_bitmask = reconstruct_p->valid_bitmask | reconstruct_p->needs_rebuild_bitmask;
    if (total_bitmask != expected_bitmask)
    {
        /* This is unexpected.
         */
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: reconstruct extent setup - Expected bitmask: 0x%x actual bitmask: 0x%x \n",
                             expected_bitmask, total_bitmask);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    reconstruct_p->raw_mirror_io_b = fbe_raid_geometry_is_raw_mirror(raid_geometry_p);

    /* Always return the execution status.
     */
    return(status);
}
/********************************************
 * end fbe_mirror_setup_reconstruct_request()
 ********************************************/

/*!***************************************************************************
 *          fbe_mirror_region_complete_update_iots()
 *****************************************************************************
 *
 * @brief   Update any fields in the iots for a region mine completion.  The
 *          reason this is necessary is the fact that we use the siots
 *          xfer_count to keep track of the blocks remaining to mine and to
 *          increment the iots transferred block count when we finish the siots.
 *
 * @param   siots_p - Pointer to siots that just completed a region request
 *
 * @return  None
 *
 * @author
 *  05/05/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
void fbe_mirror_region_complete_update_iots(fbe_raid_siots_t *siots_p)
{
    fbe_raid_iots_t    *iots_p = fbe_raid_siots_get_iots(siots_p);

    /* Whether we increment the iots blocks transferred depends if this
     * region request is completing the original request or not.  For instance
     * if this is a normal verify we do want to increment the blocks 
     * transferred.  But if this was a nested request we don't wan to
     * increment the blocks transferred since when the parent siots
     * completes it will increment the blocks transferred.
     */
    if (fbe_raid_siots_is_nested(siots_p) == FBE_FALSE)
    {
        /* The default case is to update iots transferred count with
         * the region count.
         */
        fbe_raid_iots_lock(iots_p);
        iots_p->blocks_transferred += siots_p->parity_count;
        fbe_raid_iots_unlock(iots_p);
    }

    /* Currently the only field that needs to be updated is the iots
     * `blocks_transferred' count.
     */
    return;
}
/*********************************************
 * end fbe_mirror_region_complete_update_iots()
 *********************************************/

/*!***************************************************************************
 *          fbe_mirror_reduce_request_size()
 *****************************************************************************
 *
 * @brief   Due to size limitation we need to reduce the size of a request.
 *          For mirrors we simply get the maximum per drive request size and if
 *          necessary, make sure the resulting size is aligned to the optimal
 *          block size.
 *
 * @param   siots_p - The request that needs to be reduced
 *
 * @return  status  - Typically FBE_STATUS_OK
 *                    Otherwise there was a logic error (for instance the
 *                    maximum per-drive request blocks is greater than the
 *                    current request size)
 * 
 * @note    We have already validated the maximum blocks per drive against the
 *          element size and optimal block size in fbe_raid_geometry_set_block_sizes.
 * 
 * @author  
 *  07/16/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_mirror_reduce_request_size(fbe_raid_siots_t *siots_p)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_block_count_t   max_per_drive_blocks;
    fbe_block_size_t    optimal_block_size;
    fbe_block_count_t   new_request_count = 0;
    fbe_block_count_t   new_parity_count = 0;
    fbe_bool_t          b_nested_siots = fbe_raid_siots_is_nested(siots_p);

    /* Get the maximum per-drive request size and the optimal block size
     */
    fbe_raid_geometry_get_max_blocks_per_drive(raid_geometry_p, &max_per_drive_blocks);
    fbe_raid_geometry_get_optimal_size(raid_geometry_p, &optimal_block_size);

    /* For a nested request we drop into region mode.
     */
    if (b_nested_siots == FBE_TRUE)
    {
        /* Only the following algorithms support region mode
         */
        switch (siots_p->algorithm)
        {
            case MIRROR_VR:
            case MIRROR_RD_VR:
            case MIRROR_COPY_VR:
            case MIRROR_VR_WR:
            case MIRROR_REMAP_RVR:
            case MIRROR_VR_BUF:
            case MIRROR_RB:
            case MIRROR_COPY:
            case MIRROR_PACO:
                /* All of the above support region mode
                 */
                break;

            default:
                /* The algorithm doesn't support region mode.  Log an error and
                 * fail the request.
                 */
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: siots lba: 0x%llx blks: 0x%llx max per-drive blks: 0x%llx alg: 0x%x no region mode\n",
                             (unsigned long long)siots_p->parity_start,
                 (unsigned long long)siots_p->parity_count,
                 (unsigned long long)max_per_drive_blocks,
                 siots_p->algorithm);
                return(FBE_STATUS_GENERIC_FAILURE); 
        }

        /* Split to region mode
         */
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE);

        /* Get parity count for region mining generates the proper size for any
         * runt begining and/or ending portions.  Notice that we don't modify the 
         * xfer_count. 
         */
        new_parity_count = fbe_raid_siots_get_parity_count_for_region_mining(siots_p);
        if(new_parity_count == 0)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                             "%s: mirror: Unexpected new parity count '0' returned \n",
                                             __FUNCTION__);
            return(FBE_STATUS_GENERIC_FAILURE);
        }

        new_request_count = siots_p->xfer_count;
    }
    else
    {
        /* Else if this is an optimal raid group, simply set the request to the maximum
         * per-drive size.
         */
        if (optimal_block_size == 1)
        {
            /* If the request size is already less than or equal the per-drive size
             * something is wrong.
             */
            if (siots_p->parity_count <= max_per_drive_blocks)
            {
                /* Log an error
                 */
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: parity_count: 0x%llx is already less than max per-drive count: 0x%llx \n",
                                 (unsigned long long)siots_p->parity_count,
                 (unsigned long long)max_per_drive_blocks);
                return(FBE_STATUS_GENERIC_FAILURE); 
            }

            /* Simply set the parity_count to the maximum per-disk blocks
             */
            new_request_count = max_per_drive_blocks;

            /* Align to the max per drive boundary, so that the next request starts on this boundary.
             */
            if ((siots_p->start_lba + new_request_count) % max_per_drive_blocks)
            {
                new_request_count = max_per_drive_blocks - ((siots_p->start_lba + new_request_count) % max_per_drive_blocks);
            }
        }
        else
        {
            /*! Else if it is a non-optimal raid group, there maybe a pre-read that 
             *  is larger than the transfer size. In this case if the current
             *  transfer is already less than or equal the maximum blocks per-drive,
             *  reduce it again by the optimal block size.
             *  
             *  @note We don't check the starting lba for alignment since if it
             *        wasn't aligned then there is no way to align it.
             */
            if (siots_p->parity_count <= max_per_drive_blocks)
            {
                /* If the current transfer count is less than or equal the optimal
                 * block size something is wrong.
                 */
                if (siots_p->parity_count <= optimal_block_size)
                {
                    /* Log an error
                     */
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: parity_count: 0x%llx is already less than optimal: 0x%x \n",
                                     (unsigned long long)siots_p->parity_count,
                     optimal_block_size);
                    return(FBE_STATUS_GENERIC_FAILURE); 
                }

                /* Just decrement the request by the optimal request size
                 */
                new_request_count = siots_p->xfer_count - optimal_block_size;
            }
            else
            {
                /* Else set transfer size to the maximum blocks per drive rounded
                 * down the the optimal block size.
                 */
                new_request_count = max_per_drive_blocks - (max_per_drive_blocks % optimal_block_size);
            }
        }

        /* Set the parity count to the new request count
         */
        new_parity_count = new_request_count;

    } /* end else if not nested siots */

    /* Validate the new request size
     */
    if ((fbe_s32_t)new_request_count <= 0)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: attempt to set bad request size: 0x%llx max per-drive: 0x%llx current: 0x%llx\n",
                             (unsigned long long)new_request_count,
                 (unsigned long long)max_per_drive_blocks,
                 (unsigned long long)siots_p->parity_count);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Add debug trace and return success
     */
    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_DEBUG_HIGH,
                                "mirror: Change request lba: 0x%llx blks: 0x%llx to blks: 0x%llx max per-drive: 0x%llx\n",
                                (unsigned long long)siots_p->parity_start,
                    (unsigned long long)siots_p->parity_count,
                    (unsigned long long)new_parity_count,
                    (unsigned long long)max_per_drive_blocks);
    
    siots_p->xfer_count = new_request_count;
    siots_p->parity_count = new_parity_count;
    return(FBE_STATUS_OK);
}
/* end of fbe_mirror_reduce_request_size() */

/*!***************************************************************************
 *          fbe_mirror_write_limit_request()
 *****************************************************************************
 *
 * @brief   Limits the size of a parent mirror request.  There are
 *          (2) values that will limit the request size:
 *              1. Maximum per drive request
 *              2. Maximum amount of buffers that can be allocated
 *                 per request.
 *          For unaligned write requests we will need to allocate the pre-read
 *          fruts.  Therefore we need to make sure the pre-read doesn't exceed
 *          the memory infrastructure.
 *
 * @param   siots_p - this I/O
 * @param   num_fruts_to_allocate - Number of fruts required for this request
 * @param   num_of_blocks_to_allocate - Number of blocks that need to be allocated
 * @param   get_fru_info_fn - Function to invoke to generate the fru information
 *
 * @return  status - FBE_STATUS_OK - The request was modified as required
 *                   Other - This request is too large and cannbe generated
 *
 * @note    Since we use the parent sgs if the request is too big we fail
 *          the request.
 *
 * @author  
 *  07/15/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_limit_request(fbe_raid_siots_t *siots_p,
                                      fbe_u16_t num_fruts_to_allocate,
                                      fbe_block_count_t num_of_blocks_to_allocate,
                                      fbe_status_t get_fru_info_fn(fbe_raid_siots_t *siots_p,                
                                                                   fbe_raid_fru_info_t *read_info_p,         
                                                                   fbe_raid_fru_info_t *write_info_p,        
                                                                   fbe_u16_t *num_sgs_p,
                                                                   fbe_bool_t b_log))
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_raid_fru_info_t read_info[FBE_MIRROR_MAX_WIDTH];
    fbe_raid_fru_info_t write_info[FBE_MIRROR_MAX_WIDTH];
    fbe_u16_t           num_sgs[FBE_RAID_MAX_SG_TYPE];
    
    /* Use the standard method to determine the page size.  Use the information
     * passed to this method. 
     */
    status = fbe_raid_siots_set_optimal_page_size(siots_p,
                                                  num_fruts_to_allocate,
                                                  num_of_blocks_to_allocate);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) 
    { 
        return status; 
    }    

    /* We should optimize so that we only do the below code 
     * for cases where the I/O exceeds a certain max size. 
     * This way for relatively small I/O that we know we can do we will 
     * not incur the overhead for the below. 
     */
    if (fbe_raid_siots_is_small_request(siots_p) == FBE_TRUE)
    {
        return FBE_STATUS_OK;
    }

    /* Zero the number of sgs by type
     */
    fbe_zero_memory(&num_sgs[0], (sizeof(fbe_u16_t) * FBE_RAID_MAX_SG_TYPE));

    /* Get fruts info will allocate an sg as required.  Currently the only 
     * limitation is a large host request that won't fit in the largest
     * sgl available.  If this is the case we simply split the request in half.  
     * The log flag is set to false since it isn't a failure. 
     */
    do
    {   
        /* Generate the fruts information using the get fru information
         * method that was supplied.
         */
        status = get_fru_info_fn(siots_p, 
                                 &read_info[0],
                                 &write_info[0],
                                 &num_sgs[0],
                                 FBE_FALSE);

        /* If the request exceeds the sgl limit or the per-drive transfer limit
         * split the request in half.
         */
        status = fbe_raid_siots_check_fru_request_limit(siots_p,
                                                        &read_info[0],
                                                        &write_info[0],
                                                        NULL,
                                                        status);

        if (status == FBE_STATUS_INSUFFICIENT_RESOURCES)
        {
            /* If we have successfully reduced the request, change the status
             * to FBE_STATUS_INSUFFICIENT_RESOURCES, so that we validate the 
             * updated request. 
             */
            if ((status = fbe_mirror_reduce_request_size(siots_p)) == FBE_STATUS_OK)
            {
                status = FBE_STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    
    } while ((status == FBE_STATUS_INSUFFICIENT_RESOURCES) &&
             (siots_p->parity_count  > 0)                     );

    /* Always return the execution status.
     */
    return(status);
}
/******************************************
 * end of fbe_mirror_limit_request()
 ******************************************/

/*****************************************************************************
 *          fbe_mirror_validate_parent_sgs()     
 ***************************************************************************** 
 * 
 * @brief   Validate the consistency (i.e. the parent sg is valid and is
 *          setup to contain sufficient buffers for the blocks being accessed)
 *          of the sgls for this request
 *
 * @param siots_p - pointer to SUB_IOTS data
 *
 * @return fbe_status_t
 *
 * @author
 *  09/14/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_mirror_validate_parent_sgs(fbe_raid_siots_t * siots_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_block_count_t   curr_sg_blocks;
    fbe_u32_t           sgs = 0;
    fbe_raid_fruts_t   *read_fruts_p = NULL;
    fbe_raid_fruts_t   *read2_fruts_p = NULL;
    fbe_raid_fruts_t   *write_fruts_p = NULL;

    /* Get any chains associated with this request
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_siots_get_read2_fruts(siots_p, &read2_fruts_p);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    /* Mirrors don't use the read2 chain
     */
    if (read2_fruts_p != NULL)
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "siots lba: 0x%llx blks: 0x%llx unexpected non-NULL read2 chain\n",
                                    (unsigned long long)siots_p->start_lba,
                        (unsigned long long)siots_p->xfer_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    /* We first validate the read chain
     */
    while (read_fruts_p)
    {
        /* We can't validate the container (i.e. no sg overrun) but we can validate
         * sufficient buffers for the blocks requested
         */
        sgs = 0;

        if (read_fruts_p->blocks == 0)
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "siots lba: 0x%llx blks: 0x%llx read pos: %d read blocks is 0\n",
                                        (unsigned long long)siots_p->start_lba,
                        (unsigned long long)siots_p->xfer_count,
                        read_fruts_p->position);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (read_fruts_p->sg_p == NULL)
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "siots lba: 0x%llx blks: 0x%llx read pos: %d read sg_p is NULL\n",
                                        (unsigned long long)siots_p->start_lba,
                        (unsigned long long)siots_p->xfer_count,
                        read_fruts_p->position);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Validate the sgs.
         */
        status = fbe_raid_sg_count_sg_blocks(read_fruts_p->sg_p, 
                                             &sgs,
                                             &curr_sg_blocks );
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) 
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "mirror: failed to count sg blocks: status = 0x%x, siots_p = 0x%p\n",
                                        status, siots_p);
            return status;
        }

        /* Validate that the sgl contains sufficient space for the read request.
         */
        if (curr_sg_blocks < read_fruts_p->blocks)
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "siots lba: 0x%llx blks: 0x%llx sg blks: 0x%llx is less than read pos: %d blks: 0x%llx\n",
                                        (unsigned long long)siots_p->start_lba,
                        (unsigned long long)siots_p->xfer_count,
                        (unsigned long long)curr_sg_blocks,
                                        read_fruts_p->position,
                        (unsigned long long)read_fruts_p->blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* All reads should match the parity count.
         */
        if (read_fruts_p->blocks != siots_p->parity_count)
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "siots parity lba: 0x%llx count: 0x%llx doesn't match pos: %d lba: %llx blks: 0x%llx",
                                        (unsigned long long)siots_p->parity_start,
                        (unsigned long long)siots_p->parity_count,
                        read_fruts_p->position, 
                                        (unsigned long long)read_fruts_p->lba,
                        (unsigned long long)read_fruts_p->blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Goto next fruts
         */
        read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);

    } /* end for all read fruts */

    /* Now validate the consistency of the write chain
     */
    while (write_fruts_p)
    {
        /* We can't validate the container (i.e. no sg overrun) but we can validate
         * sufficient buffers for the blocks requested
         */
        sgs = 0;

        if (write_fruts_p->blocks == 0)
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "siots lba: 0x%llx blks: 0x%llx read pos: %d write blocks is 0\n", 
                                        (unsigned long long)siots_p->start_lba,
                        (unsigned long long)siots_p->xfer_count,
                        write_fruts_p->position);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (write_fruts_p->sg_p == NULL)
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "siots lba: 0x%llx blks: 0x%llx write pos: %d read sg_p is NULL\n", 
                                        (unsigned long long)siots_p->start_lba,
                        (unsigned long long)siots_p->xfer_count,
                        write_fruts_p->position);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Validate the sgs.
         */
        status = fbe_raid_sg_count_sg_blocks(write_fruts_p->sg_p, 
                                             &sgs,
                                             &curr_sg_blocks );
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) 
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "mirror: failed to count sg blocks: status = 0x%x, siots_p = 0x%p\n", 
                                        status, siots_p);
            return status;
        }

        /* Validate that the sgl contains sufficient space for the write request.
         */
        if (curr_sg_blocks < write_fruts_p->blocks)
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "siots lba: 0x%llx blks: 0x%llx sg blks: 0x%llx is less than write pos: %d blks: 0x%llx\n", 
                                        (unsigned long long)siots_p->start_lba,
                        (unsigned long long)siots_p->xfer_count,
                        (unsigned long long)curr_sg_blocks,
                                        write_fruts_p->position,
                        (unsigned long long)write_fruts_p->blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* All writes should match the parity count.
         */
        if (write_fruts_p->blocks != siots_p->parity_count)
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "siots parity lba: 0x%llx count: 0x%llx doesn't match pos: %d lba: %llx blks: 0x%llx\n", 
                                        (unsigned long long)siots_p->parity_start,
                        (unsigned long long)siots_p->parity_count,
                        write_fruts_p->position, 
                                        (unsigned long long)write_fruts_p->lba,
                        (unsigned long long)write_fruts_p->blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Goto next
         */
        write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);

   } /* end for all write fruts */

    /* Always return the execution status
     */
    return status;
}
/******************************************
 * end of fbe_mirror_validate_parent_sgs()
 ******************************************/

/*****************************************************************************
 *          fbe_mirror_validate_allocated_fruts()     
 ***************************************************************************** 
 * 
 * @brief   Validate the consistency (i.e. the allocated sg is valid and is
 *          setup to contain sufficient buffers for the blocks being accessed)
 *          of the sgls for this request
 *
 * @param siots_p - pointer to SUB_IOTS data
 *
 * @return fbe_status_t
 *
 * @author
 *  09/13/2010  Ron Proulx  - Created from fbe_parity_468_validate_sgs
 *
 *****************************************************************************/
static fbe_status_t fbe_mirror_validate_allocated_fruts(fbe_raid_siots_t * siots_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_block_count_t   curr_sg_blocks;
    fbe_u32_t           sgs = 0;
    fbe_raid_fruts_t   *read_fruts_p = NULL;
    fbe_raid_fruts_t   *read2_fruts_p = NULL;
    fbe_raid_fruts_t   *write_fruts_p = NULL;

    /* Get any chains associated with this request
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_siots_get_read2_fruts(siots_p, &read2_fruts_p);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    /* Mirrors don't use the read2 chain
     */
    if (read2_fruts_p != NULL) {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "siots lba: 0x%llx blks: 0x%llx unexpected non-NULL read2 chain\n",
                                    (unsigned long long)siots_p->start_lba,
                        (unsigned long long)siots_p->xfer_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We first validate the read chain so that we can validate the proper pre-read
     * for unaligned write requests below.
     */
    while (read_fruts_p) {
        /* Memory validate header validates that the actual number of sg entries
         * doesn't exceed the sg type.
         */
        sgs = 0;
        if (!fbe_raid_memory_header_is_valid((fbe_raid_memory_header_t*)read_fruts_p->sg_id)) {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "siots lba: 0x%llx blks: 0x%llx read pos: %d invalid sg id\n",
                                        (unsigned long long)siots_p->start_lba,
                        (unsigned long long)siots_p->xfer_count,
                        read_fruts_p->position);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (read_fruts_p->blocks == 0) {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "siots lba: 0x%llx blks: 0x%llx read pos: %d read blocks is 0\n", 
                                        (unsigned long long)siots_p->start_lba,
                        (unsigned long long)siots_p->xfer_count,
                        read_fruts_p->position);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        status = fbe_raid_sg_count_sg_blocks(fbe_raid_fruts_return_sg_ptr(read_fruts_p), 
                                             &sgs,
                                             &curr_sg_blocks );
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "mirror: failed to count sg blocks: status = 0x%x, siots_p = 0x%p\n", 
                                        status, siots_p);
            return status;
        }

        /* Make sure the read has an SG for the correct block count */
        if (read_fruts_p->blocks != curr_sg_blocks) {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "siots lba: 0x%llx blks: 0x%llx read pos: %d blks: 0x%llx doesn't match sg blks: 0x%llx \n",
                                        (unsigned long long)siots_p->start_lba,
                                        (unsigned long long)siots_p->xfer_count,
                                        read_fruts_p->position,
                                        (unsigned long long)read_fruts_p->blocks,
                                        (unsigned long long)curr_sg_blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Except for pre-reads for unaligned writes, all reads should match the
         * parity count.
         */
        if (read_fruts_p->blocks != siots_p->parity_count) {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "siots parity lba: 0x%llx count: 0x%llx doesn't match pos: %d lba: %llx blks: 0x%llx \n",
                                        (unsigned long long)siots_p->parity_start,
                                        (unsigned long long)siots_p->parity_count,
                                        read_fruts_p->position, 
                                        (unsigned long long)read_fruts_p->lba,
                                        (unsigned long long)read_fruts_p->blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Goto next fruts
         */
        read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);

    } /* end for all read fruts */

    /* Now validate the consistency of the write chain
     */
    while (write_fruts_p) {
        /* Memory validate header validates that the actual number of sg entries
         * doesn't exceed the sg type.
         */
        sgs = 0;
        if (!fbe_raid_memory_header_is_valid((fbe_raid_memory_header_t*)write_fruts_p->sg_id)) {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "raid header invalid for fruts 0x%p, sg_id: 0x%p \n",
                                        write_fruts_p, write_fruts_p->sg_id);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (write_fruts_p->blocks == 0) {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "siots lba: 0x%llx blks: 0x%llx read pos: %d write blocks is 0 \n", 
                                        (unsigned long long)siots_p->start_lba,
                                        (unsigned long long)siots_p->xfer_count,
                                        write_fruts_p->position);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        status = fbe_raid_sg_count_sg_blocks(fbe_raid_fruts_return_sg_ptr(write_fruts_p), 
                                             &sgs,
                                             &curr_sg_blocks );
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "mirror: failed to count blocks: status = 0x%x, siots_p = 0x%p\n",
                                        status, siots_p);
            return status;
        }

        /* Make sure the write has an SG for the correct block count */
        if (write_fruts_p->blocks != curr_sg_blocks) {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "siots lba: 0x%llx blks: 0x%llx write pos: %d blks: 0x%llx doesn't match sg blks: 0x%llx \n",
                                        (unsigned long long)siots_p->start_lba,
                                        (unsigned long long)siots_p->xfer_count,
                                        write_fruts_p->position,
                                        (unsigned long long)write_fruts_p->blocks,
                                        (unsigned long long)curr_sg_blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* All writes should match the parity count.
         */
        if (write_fruts_p->blocks != siots_p->parity_count) {
        
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "siots parity lba: 0x%llx count: 0x%llx doesn't match pos: %d lba: %llx blks: 0x%llx \n",
                                        (unsigned long long)siots_p->parity_start,
                                        (unsigned long long)siots_p->parity_count,
                                        write_fruts_p->position,
                                        (unsigned long long)write_fruts_p->lba,
                                        (unsigned long long)write_fruts_p->blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Goto next
         */
        write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);

    } /* end for all write fruts */

    /* Always return the execution status
     */
    return status;
}
/**********************************************
 * end of fbe_mirror_validate_allocated_fruts()
 **********************************************/

/*****************************************************************************
 *          fbe_mirror_validate_fruts()     
 ***************************************************************************** 
 * 
 * @brief   Validate the consistency (i.e. the allocated sg is valid and is
 *          setup to contain sufficient buffers for the blocks being accessed)
 *          of the sgls for this request
 *
 * @param siots_p - pointer to SUB_IOTS data
 *
 * @return fbe_status_t
 *
 * @author
 *  09/13/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_validate_fruts(fbe_raid_siots_t *siots_p)
{
    fbe_status_t        status;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* Only enabled for checked builds.
     */
    if (RAID_DBG_ENABLED == FBE_FALSE) {
        return FBE_STATUS_OK;
    }

    /* Different validation for unaligned write request.
     */
    if (((siots_p->algorithm == MIRROR_WR)    ||
         (siots_p->algorithm == MIRROR_VR_WR) ||   
         (siots_p->algorithm == MIRROR_CORRUPT_DATA)) &&
        fbe_raid_geometry_io_needs_alignment(raid_geometry_p, 
                                             siots_p->parity_start, 
                                             siots_p->parity_count)      ) {
        status = fbe_mirror_validate_unaligned_write(siots_p);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
            /* Trace additional information if validation failed
             */
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: validate unaligned write failed: "
                                 "siots_p = 0x%p. siots lba: 0x%llx, blks: 0x%llx flags: 0x%x\n",
                                 siots_p,
                                 (unsigned long long)siots_p->start_lba,
                                 (unsigned long long)siots_p->xfer_count,
                                 fbe_raid_siots_get_flags(siots_p));
        }
    } else {
        /* Simply use the method required based on using or not using
         * the parent sgs.
         */
        status = fbe_mirror_validate_allocated_fruts(siots_p);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
            /* Trace additional information if validation failed
             */
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to validate fruts: "
                             "siots_p = 0x%p. siots lba: 0x%llx, blks: 0x%llx flags: 0x%x\n",
                             siots_p,
                                 (unsigned long long)siots_p->start_lba,
                                 (unsigned long long)siots_p->xfer_count,
                                 fbe_raid_siots_get_flags(siots_p));
        }
    }

    /* Always return the execution status
     */
    return status;
}
/******************************************
 * end of fbe_mirror_validate_fruts()
 ******************************************/

/*****************************************
 * end fbe_mirror_util.c
 *****************************************/


