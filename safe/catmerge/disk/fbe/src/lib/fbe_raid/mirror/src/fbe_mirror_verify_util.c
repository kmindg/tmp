/***************************************************************************
 * Copyright (C) EMC Corporation 2000-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file    fbe_mirror_verify_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains utility functions for the mirror_vr state machine.
 *
 * @notes
 *
 * @author
 *  9/2000.  Created. RG
 *
 ***************************************************************************/

/*************************
 **  INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_mirror_io_private.h"
#include "fbe_raid_library.h"
#include "fbe_raid_error.h"
#include "fbe/fbe_event_log_utils.h"
#include "fbe/fbe_api_lun_interface.h"

/**********************
 * STATIC FUNCTIONS
 *********************/
static fbe_status_t fbe_mirror_log_dead_peer_invalidation(fbe_raid_siots_t * siots_p);
static fbe_status_t fbe_mirror_report_retried_errors(fbe_raid_siots_t * siots_p);


/**********************
 * FUNCTION DEFINITION
 *********************/

/*!**************************************************************
 *          fbe_mirror_verify_validate()
 ****************************************************************
 * @brief
 *  Validate the algorithm and some initial parameters to
 *  the verify state machine.
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
 *  12/09/2009  Ron Proulx - Created
 *
 ****************************************************************/

fbe_status_t fbe_mirror_verify_validate(fbe_raid_siots_t * siots_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           width = fbe_raid_siots_get_width(siots_p);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_block_size_t    optimal_block_size;

    /* First make sure we support the algorithm.
     */
    switch (siots_p->algorithm)
    {
        case MIRROR_VR:
        case MIRROR_RD_VR:
        case MIRROR_COPY_VR:
        case MIRROR_VR_WR:
        case MIRROR_VR_BUF:
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
        if (siots_p->data_disks != fbe_raid_siots_get_full_access_count(siots_p))
        {
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: data_disks: 0x%x and full access count: 0x%x don't agree\n", 
                                 siots_p->data_disks, width);
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    } /* end if not in region mode */

    /* Make sure the verify request is a multiple of the optimal block size.
     * Anyone calling this code should align the request to the optimal 
     * block size.
     */
    fbe_raid_geometry_get_optimal_size(raid_geometry_p, &optimal_block_size);
    if ( (siots_p->parity_count % optimal_block_size) != 0 )
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: parity_count: 0x%llx isn't a multiple of optimal block size: 0x%x\n", 
                             (unsigned long long)siots_p->parity_count,
			     optimal_block_size);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if ( (siots_p->parity_start % optimal_block_size) != 0)
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: parity_start: 0x%llx isn't a multiple of optimal block size: 0x%x\n", 
                             (unsigned long long)siots_p->parity_start,
			     optimal_block_size);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    /* Always return the status.
     */
    return(status);
}
/******************************************
 * end fbe_mirror_verify_validate()
 ******************************************/

/*!***************************************************************************
 *          fbe_mirror_verify_get_parent_allocated_blocks()
 ***************************************************************************** 
 *
 * @brief   This function will count the number of buffers that were
 *          allocated by the fruts (normally a parent fruts) passed.  Since 
 *          the logical to physical mapping has already been performed by the
 *          fbe infrastructure, we simply get the the block count.
 * 
 * @param   parent_fruts_p - Pointer to parent fruts (original read request)
 * @param   siots_p - Pointer to siots for nested verify request
 * @param   verify_start - The starting pba for the mirror verify request
 * @param   verify_count - The number of blocks for this verify request
 * @param   parent_blocks_allocated_p - Address of parent blocks to update
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  11/30/2009  Ron Proulx  - Created from fbe_parity_verify_count_allocated_buffers
 *
 ******************************************************************************/
fbe_status_t fbe_mirror_verify_get_parent_allocated_blocks(fbe_raid_fruts_t *parent_fruts_p,
                                                           fbe_raid_siots_t *siots_p,
                                                           fbe_lba_t verify_start,
                                                           fbe_block_count_t verify_count,
                                                           fbe_u64_t *parent_blocks_allocated_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_block_count_t   overlapping_parent_blocks = 0;
    fbe_lba_t           verify_end = verify_start + verify_count - 1;

    /* Although there may be more than one parent read fruts,
     * for mirror all fruts have the same physical range.  Therefore
     * simply use the read fruts passed and determine the number of
     * overlapping blocks
     */
    if (parent_fruts_p != NULL)
    {
        fbe_lba_t   fruts_start = parent_fruts_p->lba;
        fbe_lba_t   fruts_end = (fruts_start + parent_fruts_p->blocks) - 1;

        /* Since the parent siots may have multiple fruts, there is no
         * guarantee that the current fruts is in the same range as the 
         * verify request.  Therefore only count the parent fruts allocated
         * blocks when they are in the range of this verify request.
         */
        if ((fruts_start >= verify_start) &&
            (fruts_end   <= verify_end)      )
        {
            /* At least one block overlaps.
             */
            fruts_end = FBE_MIN(verify_end, fruts_end);
            overlapping_parent_blocks += (fbe_block_count_t)(fruts_end - fruts_start) + 1;
        }
    }

    /* If the overlapping parent blocks is greater than the verify count
     * we did something wrong!
     */
    if (overlapping_parent_blocks > verify_count)
    {
        /* Something is wrong, report the failure.
         */
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: verify count allocated attempt to allocate more blocks: 0x%llx than verify: 0x%llx\n",
                             (unsigned long long)overlapping_parent_blocks,
			     (unsigned long long)verify_count);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        /* Else update the parent blocks allocated.
         */
        *parent_blocks_allocated_p = overlapping_parent_blocks; 
    }

    /* Always return the execution status.
     */
    return(status);
}
/* end of fbe_mirror_verify_get_parent_allocated_blocks() */

/*!***************************************************************************
 *          fbe_mirror_verify_get_blocks_allocated_by_parent()
 *****************************************************************************
 *
 * @brief   As the name state this method determines how many overlapping
 *          blocks are allocated by the parent for a recovery verify request.
 *          This is done so that the recovery verify will populate the read
 *          buffers of the original read request (instead of first recovering
 *          the data into new verify buffers and then copying the verify data
 *          to the parent read buffers).
 *
 * @param   siots_p - Siots for verify request
 * @param   verify_start - Starting pba for verify
 * @param   verify_count - Number of blocks to verify for this request
 * @param   parent_allocated_blocks_p - Pointer to parent allocated blocks
 *
 * @return  status - Typically FBE_STATUS_OK  
 *
 * @author
 *  11/30/2009  Ron Proulx - Created from fbe_parity_recovery_verify_count_buffers
 *
 ****************************************************************************/
fbe_status_t fbe_mirror_verify_get_blocks_allocated_by_parent(fbe_raid_siots_t * siots_p, 
                                                              fbe_lba_t verify_start, 
                                                              fbe_block_count_t verify_count,
                                                              fbe_block_count_t *parent_allocated_blocks_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_block_count_t   blocks_allocated_by_parent = 0;
    fbe_raid_siots_t   *parent_siots_p = NULL;
    fbe_raid_fruts_t   *parent_read_fruts_p = NULL;
    fbe_raid_fruts_t   *parent_read2_fruts_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;

    /* Now get the blocks (i.e. buffers that have already been allocated for a
     * parent read request) that have already been allocated.
     */
    fbe_raid_siots_get_opcode(siots_p, &opcode);
    parent_siots_p = fbe_raid_siots_get_parent(siots_p);

    if (parent_siots_p != NULL)
    {
        /* For mirrors will allow all algorithms to use read recovery verify.
         */
        fbe_raid_siots_get_read_fruts(parent_siots_p, &parent_read_fruts_p);
        if (parent_read_fruts_p != NULL)
        {
            fbe_block_count_t   blocks_allocated = 0;
            
            /* If the status is success, increment the blocks allocated by the parent.
             */
            if ( (status = fbe_mirror_verify_get_parent_allocated_blocks(parent_read_fruts_p, 
                                                                         siots_p, 
                                                                         verify_start, 
                                                                         verify_count,
                                                                         &blocks_allocated)) == FBE_STATUS_OK )
            {
                blocks_allocated_by_parent += blocks_allocated;
            }
    
            /* Mirror groups don't use the second read chain.  Therefore it is
             * an error of it isn't NULL.
             */
            fbe_raid_siots_get_read2_fruts(parent_siots_p, &parent_read2_fruts_p);
            if ((status == FBE_STATUS_OK)      &&
                (parent_read2_fruts_p != NULL)    )
            {
                /* Mirrors don't support a second read chain.
                 */
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: verify get parent blocks - second parent read fruts no expected. algorithm: 0x%x \n",
                                     siots_p->algorithm);
                status = FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }

    /* Always return the execution status.
     */
    return(status);
}
/* end of fbe_mirror_verify_get_blocks_allocated_by_parent() */

/*!***************************************************************************
 *          fbe_mirror_verify_set_parent_position_sg()
 *****************************************************************************
 * 
 * @brief   This method determines and sets the sg index for a recovery verify
 *          parent position.
 *
 * @param   parent_read_fruts_p - Pointer to parent siots read fruts 
 * @param   read_info_p - Pointer to array of read info to populate
 * @param   page_size - Page size in blocks for verify request
 * @param   blks_remaining_p - Pointer to blocks remaining 
 * @param   b_log - FBE_TRUE - Generate a trace log if sg generation fails
 *                  FBE_FALSE - Method invoked to limit request size
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  05/04/2010  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_mirror_verify_set_parent_position_sg(fbe_raid_fruts_t *parent_read_fruts_p,
                                                             fbe_raid_fru_info_t *read_info_p,
                                                             fbe_block_count_t page_size,
                                                             fbe_block_count_t *blks_remaining_p,
                                                             fbe_bool_t b_log)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_lba_t           parent_start, verify_start;
    fbe_lba_t           parent_end, verify_end;
    fbe_block_count_t   total_blocks_to_assign = read_info_p->blocks;
    fbe_block_count_t   blocks_assigned = 0;
    fbe_sg_element_t   *parent_sg_p = NULL;
    fbe_u32_t           sg_count = 0;
    fbe_raid_siots_t    *parent_siots_p;
    
    if (RAID_COND(parent_read_fruts_p == NULL))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "mirror: parent_read_fruts_p is NULL!!!\n");
        return (FBE_STATUS_GENERIC_FAILURE);
    }

    /* Initialize parent and verify range.
     */
    parent_start = parent_read_fruts_p->lba;
    parent_end = parent_start + parent_read_fruts_p->blocks - 1;
    verify_start = read_info_p->lba;
    verify_end = verify_start + read_info_p->blocks - 1;
    fbe_raid_fruts_get_sg_ptr(parent_read_fruts_p, &parent_sg_p);
    parent_siots_p = fbe_raid_fruts_get_siots(parent_read_fruts_p);

    if (RAID_COND(parent_siots_p == NULL))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "mirror: parent_siots_p is NULL!\n");
        return (FBE_STATUS_GENERIC_FAILURE);
    }
    /*! Insert parent buffers into the verify sg list.  We have allocated (2)
     *  additional sg entries so that we can split a verify sg for the beginning
     *  of the parent sgl and the end of the parent sgl.
     */
    if (parent_start > verify_start)
    {
        fbe_lba_t           end_lba;
        fbe_block_count_t   blocks_to_assign;
        fbe_u32_t           sgs_used = 0;

        /* Populate beginning of verify request with buffers from verify 
         * request.  Since we are not using the parent buffers we use the
         * blocks remaining from the verify buffers when determining the
         * sg list length.
         */
        end_lba = FBE_MIN(verify_end, (parent_start - 1));
        blocks_to_assign = (fbe_block_count_t)(end_lba - verify_start + 1);
        blocks_assigned += blocks_to_assign; 
        verify_start += blocks_to_assign;
        
        /* Update the memory left with the first portion
         */
        *blks_remaining_p = fbe_raid_sg_count_uniform_blocks(blocks_to_assign,
                                                       page_size,
                                                       *blks_remaining_p,
                                                       &sgs_used);

        /* Increment the count of sgs required for verify portion.
         */
        sg_count += sgs_used;

    }

    /* Setup the middle portion with the parent buffers if the verify request
     * overlaps with the parent range.  If the entire verify request is not less
     * than the parent or greater than the parent then it overlaps.
     */
    if ( (blocks_assigned < total_blocks_to_assign) &&
        !((verify_end    < parent_start) ||
          (verify_start  > parent_end)      )           )
    {
        fbe_lba_t           start_lba, end_lba;
        fbe_block_count_t   blocks_to_assign;
        fbe_sg_element_with_offset_t tmp_sgd;
        fbe_u32_t           offset;
        fbe_block_count_t   parent_blks_remaining;
        fbe_u32_t           sgs_used = 0;

        /* The parent overlaps the verify request.  Use the parent buffers
         * to populate this range.  We should have done the beginning with
         * if needed so the parent start should less than or equal.
         */
        if (parent_start > verify_start)
        {
            /* Logical error.
             */
            fbe_raid_siots_object_trace(parent_siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "mirror: set parent pos sg - parent_start: 0x%llx greater than verify_start: 0x%llx\n",
                                        (unsigned long long)parent_start,
			                (unsigned long long)verify_start);
            return(FBE_STATUS_GENERIC_FAILURE);
        }

        /* Populate the middle blocks with the parent buffers.  We simply
         * use the minimum of the parent and verify range.
         */
        start_lba = FBE_MAX(parent_start, verify_start);
        end_lba = FBE_MIN(parent_end, verify_end);
        blocks_to_assign = (fbe_block_count_t)(end_lba - start_lba + 1);
        blocks_assigned += blocks_to_assign;
        verify_start += blocks_to_assign;
        
        /* Now get the parent buffers using the correct offset.  The offset is
         * the difference from the original parent start lba and this start lba.
         */
        offset = (fbe_u32_t)((start_lba - parent_start) * FBE_RAID_SECTOR_SIZE(parent_sg_p->address));
        fbe_sg_element_with_offset_init(&tmp_sgd, 
                                        offset,
                                        parent_sg_p,
                                        NULL /* use default get next sg fn */);

        /* Validate that there are sufficient sgs entries in the verify sgl.
         */
        parent_blks_remaining = tmp_sgd.sg_element->count / FBE_RAID_SECTOR_SIZE(parent_sg_p->address);
        status = fbe_raid_sg_count_nonuniform_blocks(parent_read_fruts_p->blocks,
                                                                    &tmp_sgd,
                                                                    FBE_RAID_SECTOR_SIZE(parent_sg_p->address),
                                                                    &parent_blks_remaining,
                                                                    &sgs_used);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_object_trace(parent_siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "mirror: failed to count non-uniform blocks: parent_blks_remaining = 0x%llx, status = 0x%x\n",
                                        (unsigned long long)parent_blks_remaining,
				        status);    
            return  status;
        }
       
        /* Increment the count of sgs required for parent portion.
         */
        sg_count += sgs_used;
    }

    /* Now populate any remaining buffers with the verify buffers.
     */
    if (blocks_assigned < total_blocks_to_assign)
    {
        fbe_block_count_t   blocks_to_assign;
        fbe_u32_t           sgs_used = 0;
        
        /* If the remaining verify is within the parent range something is
         * wrong.
         */
        if ((parent_start >= verify_start) ||
            (parent_end   >= verify_end)      )
        {
            /* Logical error.
             */
            
            fbe_raid_siots_object_trace(parent_siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "mirror: set parent pos sg - parent_start: 0x%llx parent_end: 0x%llx within verify\n",
                                        (unsigned long long)parent_start,
				        (unsigned long long)parent_end);
            fbe_raid_siots_object_trace(parent_siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "                          - verify_start: 0x%llx verify_end: 0x%llx \n",
                                        (unsigned long long)verify_start,
				        (unsigned long long)verify_end);
            return(FBE_STATUS_GENERIC_FAILURE);
        }

        /* The blocks to assign is the remaining verify range.
         */
        blocks_to_assign = (fbe_block_count_t)(verify_end - verify_start + 1);
        blocks_assigned += blocks_to_assign; 
        verify_start += blocks_to_assign;
        
        /* Populate ending of verify request with buffers from verify 
         * request.  Since we are not using the parent buffers we use the
         * blocks remaining from the verify buffers when determining the
         * sg list length.  Also validate that there are sufficient sgs 
         * entries in the verify sgl.
         */
        *blks_remaining_p = fbe_raid_sg_count_uniform_blocks(blocks_to_assign,
                                                       page_size,
                                                       *blks_remaining_p,
                                                       &sgs_used);

        /* Increment the count of sgs required for verify portion.
         */
        sg_count += sgs_used;
    }

    /* Now sanity check the results.
     */
    if ((blocks_assigned != total_blocks_to_assign) ||
        ((verify_start - 1) != verify_end)             )
    {
        /* Logical error.
         */
        fbe_raid_siots_object_trace(parent_siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "mirror: set parent pos sg - total to assign: 0x%llx assigned: 0x%llx actual end: 0x%llx\n",
                                    (unsigned long long)total_blocks_to_assign,
			            (unsigned long long)blocks_assigned,
			            (unsigned long long)(verify_start - 1));
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Now determine and set the sg index for this position.
     */
    status = fbe_raid_fru_info_set_sg_index(read_info_p, sg_count, b_log);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
          fbe_raid_siots_object_trace(parent_siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                      "mirror: failed to set sg index: status = 0x%x, sg_count = 0x%x, "
                                      "read_info_p = 0x%p\n", 
                                      status, sg_count, read_info_p);
          return  status;
    }

    /* Always return the execution status.
     */
    return(status);
}
/* end of fbe_mirror_verify_set_parent_position_sg() */

/*!***************************************************************************
 *          fbe_mirror_verify_get_fru_info()
 *****************************************************************************
 * 
 * @brief   This function initializes the fru info array for a mirror verify 
 *          request.  For recovery verify request types we must allocate (2)
 *          additional sg entries for the parent position.  Other this special
 *          case each position evenly distributes the sg entries (i.e. uniform).
 *
 * @param   siots_p - Current sub request.
 * @param   read_info_p - Pointer to array of read info to populate
 * @param   write_info_p - Pointer to array of write (i.e. degraded positions) info
 * @param   num_sgs_p - Pointer to array of sg count by sg index array
 * @param   b_log - FBE_TRUE - Generate a trace log if sg generation fails
 *                  FBE_FALSE - Method invoked to limit request size
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  11/25/2009  Ron Proulx  - Created from fbe_parity_verify_get_fruts_info
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_verify_get_fru_info(fbe_raid_siots_t *siots_p,
                                            fbe_raid_fru_info_t *read_info_p,
                                            fbe_raid_fru_info_t *write_info_p,
                                            fbe_u16_t *num_sgs_p,
                                            fbe_bool_t b_log)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   width = fbe_raid_siots_get_width(siots_p);
    fbe_u32_t                   index;
    fbe_raid_fruts_t           *parent_read_fruts_p = NULL;
    fbe_raid_position_t         parent_position = FBE_RAID_INVALID_POS;
    fbe_raid_position_bitmask_t degraded_bitmask;
    fbe_block_count_t                blks_remaining = siots_p->data_page_size;
    fbe_u32_t                   sg_count = 0;

    /* Page size must be set.
     */
    if (!fbe_raid_siots_validate_page_size(siots_p))
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* For recovery types of requests we allocate an additional (2) sg entries
     * for each parent position so that we can insert the parent buffers.
     */
    switch(siots_p->algorithm)
    {
        /* For recovery verify set the parent position.
         */
        case MIRROR_RD_VR:
        case MIRROR_VR_BUF:
            {
                fbe_raid_siots_t   *parent_siots_p = fbe_raid_siots_get_parent(siots_p);
            
                fbe_raid_siots_get_read_fruts(parent_siots_p, &parent_read_fruts_p);
                parent_position = parent_read_fruts_p->position;
            }
            break;
        default:
            break;
    }

    /* For a mirror verify request we populate all positions.  Degraded
     * positions go on the write chain and non-degraded postions go on
     * the read chain.  Due to the way the XOR algorithms work we must
     * allocate memory for all positions.
     */
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    for (index = 0; index < width; index++)
    {
        /* Always set the position for both read and write infos
         */
        read_info_p->position = fbe_mirror_get_position_from_index(siots_p, index);
        write_info_p->position = FBE_U16_MAX;
        /* Else populate the read sg information.  If this position matches
         * the parent position we allocate an additional (2) sg entries.
         */
        read_info_p->lba = siots_p->parity_start;
        read_info_p->blocks = siots_p->parity_count;

        /* If parent is valid and matches this position, determine the sg
         * count.
         */
        if (read_info_p->position == parent_position)
        {
            status = fbe_mirror_verify_set_parent_position_sg(parent_read_fruts_p,
                                                              read_info_p,
                                                              siots_p->data_page_size,
                                                              &blks_remaining,
                                                              b_log);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "mirror_verify_get_fru_info" , 
                                     "mirror:fail to verify pos:status=0x%x,siots_p=0x%p\n",
                                     status, siots_p);
                return(status);
            }
        }
        else
        {
            /* Else use the uniform sg method.  Populate sg index returns 
             * a status.  The reason is that the sg_count may exceed the 
             * maximum sgl length.
             */
            blks_remaining = fbe_raid_sg_count_uniform_blocks(read_info_p->blocks,
                                                              siots_p->data_page_size,
                                                              blks_remaining,
                                                              &sg_count);

            /* Always add on an sg since if we drop down to a region mode, 
             * then it is possible we will need an extra sg to account for region alignment. 
             */            
             sg_count++;
            
            /* Set sg index returns a status.  The reason is that the sg_count
             * may exceed the maximum sgl length.
             */
            status = fbe_raid_fru_info_set_sg_index(read_info_p,
                                                      sg_count,
                                                      b_log);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "mirror_verify_get_fru_info",
                                            "mirror_verify_get_fru_info mirror_verify_get_fru_info fail to set sg index:stat=0x%x,fru_info=0x%p,"
                                            "sg_cnt=0x%x\n", status, read_info_p, sg_count);
                return status;
            }
        }

        /* Success, increment the sg type count.
         */
        num_sgs_p[read_info_p->sg_index]++;

        /* Set the write info to invalid.
         */
        write_info_p->lba = FBE_LBA_INVALID;
        write_info_p->blocks = 0;
        write_info_p->sg_index = FBE_RAID_SG_INDEX_INVALID;

        /* Goto next fru info.
         */
        read_info_p++;
        write_info_p++;
    }

    /* Always return the execution status.
     */
    return(status);
}
/**************************************
 * end fbe_mirror_verify_get_fru_info()
 **************************************/

/*!**************************************************************************
 *          fbe_mirror_verify_calculate_memory_size()
 ****************************************************************************
 *
 * @brief   This function calculates the total memory usage of the siots for
 *          this state machine.
 *
 * @param   siots_p - Pointer to siots that contains the allocated fruts
 * @param   read_info_p - Pointer to array to populate for fruts information
 * @param   write_info_p - Pointer to array to populate for degraded information
 * 
 * @return  status - Typically FBE_STATUS_OK
 *                             Other - This shouldn't occur
 *
 * @author
 *  11/25/2009  Ron Proulx  - Created from fbe_parity_verify_calculate_memory_size
 *
 **************************************************************************/
fbe_status_t fbe_mirror_verify_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                     fbe_raid_fru_info_t *read_info_p,
                                                     fbe_raid_fru_info_t *write_info_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           width = fbe_raid_siots_get_width(siots_p);
    fbe_u16_t           num_sgs[FBE_RAID_MAX_SG_TYPE];
    fbe_u32_t           num_fruts;
    fbe_block_count_t           blocks_to_allocate;
    fbe_block_count_t  blocks_allocated_by_parent = 0;
    
    /* Initialize the sg index count per position array.
     */
    fbe_zero_memory(&num_sgs[0], (sizeof(fbe_u16_t) * FBE_RAID_MAX_SG_TYPE));
    
    /* We allocate buffers for both the read and write positions
     */
    num_fruts = width;    
    
    /* The number of positions to allocate is normally the entire width
     * of the raid group but the exception to this rule is if we are a 
     * stacked raid group.  In this case the verify request MAY have come
     * from the upstream object (as opposed to the mirror monitor).
     * Simply invoke the routine that will determine if there any buffers
     * already supplied.
     */
    status = fbe_mirror_verify_get_blocks_allocated_by_parent(siots_p,
                                                              siots_p->parity_start,
                                                              siots_p->parity_count,
                                                              &blocks_allocated_by_parent);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
          fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                               "mirror : failed to get verify blocks allocated by parent: siots_p = 0x%p, "
                               "status = 0x%x\n",
                               siots_p, status);

          return status;
    }

    /* The purpose of switching on the algorithms is that for non-recovery
     * verifies we don't expect (and thus handle) any parent siots.
     */
    switch(siots_p->algorithm)
    {
        case MIRROR_RD_VR:
        case MIRROR_VR_BUF:
            /* For recovery types of verifies we subtract the blocks allocated
             * by the parent (will will populate the parent read data from the
             * verify request).  Subtract the blocks allocated by the parent.
             */
            blocks_to_allocate = num_fruts * siots_p->parity_count;
            blocks_to_allocate -= blocks_allocated_by_parent;
            break;

        case MIRROR_VR_WR:
        case MIRROR_VR:
        case MIRROR_COPY_VR:
            /* For normal verifies (verify, write verify, read remap)
             * we will read from all non-degraded positions simultaneously.
             * But the current XOR methods assume that each position (i.e.
             * if the those positions that are known to be bad, degraded)
             * has it own buffer.
             */
            blocks_to_allocate = num_fruts * siots_p->parity_count;
            if (RAID_COND(blocks_allocated_by_parent > 0))
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "mirror: verify calculate memory unexpected parent blocks. algorithm: 0x%x\n",
                                     siots_p->algorithm);
                return(FBE_STATUS_GENERIC_FAILURE);
            }
            break;

        default:
            /* This is unexpected so return an error.
             */
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: verify calculate memory unsupported algorithm: 0x%x\n",
                                 siots_p->algorithm);
            return(FBE_STATUS_GENERIC_FAILURE);
            break;
    }

    /* Setup the number of blocks we need to allocate for this request.
     */
    siots_p->total_blocks_to_allocate = blocks_to_allocate;
    
    /*! For a nested request we need to use the same page size as the parent.
     *  Otherwise the raid memory code will determine the page size based on
     *  the request.
     *
     *  @note This is required since the current sg methods (for example 
     *        fbe_raid_sg_clip_sg_list()) have no way to determine the size of
     *        each sg buffer.  Therefore they use the source buffer size which
     *        in this case is the parent. 
     */
    fbe_raid_siots_set_optimal_page_size(siots_p,
                                         width,
                                         siots_p->total_blocks_to_allocate);
        
    /* Generate the sg index for each read position.  A failure is 
     * unexpected since we should have already limited the request.
     */
    status = fbe_mirror_verify_get_fru_info(siots_p, 
                                            read_info_p,
                                            write_info_p,
                                            &num_sgs[0],
                                            FBE_TRUE /* Log on error */);
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
    status = fbe_raid_siots_calculate_num_pages(siots_p, num_fruts, &num_sgs[0],
                                                FBE_FALSE /* No recovery verify is required*/);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
          fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                               "mirror : failed to calculate num of pages: siots_p = 0x%p, status = 0x%x\n",
                               siots_p, status);
          return status;
    }
    
    /* Always return the execution status.
     */
    return(status);
}
/**********************************************
 * end fbe_mirror_verify_calculate_memory_size()
 **********************************************/

/*!**********************************************************************
 *          fbe_mirror_verify_setup_fruts()
 ************************************************************************
 *
 * @brief   This method initializes the fruts for a mirror verify request.
 *          All positions in the raid group are configured as either a
 *          read or write.
 *
 * @param   siots_p    - ptr to the fbe_raid_siots_t
 * @param   read_info_p - Array of information about fruts 
 * @param   write_info_p - Array of information about degraded fruts 
 * @param   requested_opcode - Opcode to initialize fruts with
 * @param   memmory_info_p - Pointer to memory request informaiton
 *
 * @return fbe_status_t
 *
 * @author
 *  12/15/2009  Ron Proulx  - Created
 *
 ****************************************************************/
static fbe_status_t fbe_mirror_verify_setup_fruts(fbe_raid_siots_t * siots_p, 
                                                  fbe_raid_fru_info_t *read_info_p,
                                                  fbe_raid_fru_info_t *write_info_p,
                                                  fbe_payload_block_operation_opcode_t requested_opcode,
                                                  fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_raid_fruts_t           *read_fruts_p = NULL;
    fbe_u32_t                   index;
    fbe_u32_t                   width = fbe_raid_siots_get_width(siots_p);
    fbe_raid_position_bitmask_t degraded_bitmask;
    fbe_u32_t                   degraded_count;
    fbe_raid_position_bitmask_t full_access_bitmask;
    fbe_u32_t                   full_access_count;
    fbe_u32_t                   read_count;

    /* Get the degraded and full access bitmasks.
     */
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    degraded_count = fbe_raid_siots_get_degraded_count(siots_p);
    status = fbe_raid_siots_get_full_access_bitmask(siots_p, &full_access_bitmask);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "mirror: failed to get access bitmask: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return  status;
    }
    full_access_count = fbe_raid_siots_get_full_access_count(siots_p);
    
    /* Validate that the number of degraded positions agrees with
     * the width minus the number of `data disks'.
     */
    if (RAID_COND(degraded_count != (width - siots_p->data_disks)))
    {
        /* There is a descrepency between the expected number of
         * data disks and the degraded count.
         */
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: verify setup fruts. degraded count: 0x%x doesn't agree with width: 0x%x or data_disks: 0x%x\n",
                             degraded_count, width, siots_p->data_disks); 
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Initialize the fruts for data drives.
     * First setup the read fruts (limited to 1), then setup
     * the write fruts.
     */
    for (index = 0; index < width; index++)
    {
        /* Else if this position isn't degraded it goes on the read
         * fruts chain.
         */

        /* Validate that the read block count isn't 0 and that the
         * write block count is.
         */
        if ((read_info_p->blocks == 0)  ||
            (write_info_p->blocks != 0)    )
        {
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: verify setup fruts read blocks is 0x%llx lba: %llx pos: %d\n",
                                 (unsigned long long)read_info_p->blocks,
			         (unsigned long long)read_info_p->lba,
				 read_info_p->position);
            return(FBE_STATUS_GENERIC_FAILURE);
        }

        read_fruts_p = fbe_raid_siots_get_next_fruts(siots_p,
                                                     &siots_p->read_fruts_head,
                                                     memory_info_p);

        /* If the setup failed something is wrong.
         */
        if (read_fruts_p == NULL)
        {
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: verify setup fruts siots: 0x%llx 0x%llx for position: 0x%x failed \n",
                                 (unsigned long long)siots_p->start_lba,
				 (unsigned long long)siots_p->xfer_count,
				 read_info_p->position);
            return(FBE_STATUS_GENERIC_FAILURE);
        }

        /* Now initialize the read fruts and set the opcode to the requested.
         */
        status = fbe_raid_fruts_init_fruts(read_fruts_p,
                                           requested_opcode,
                                           read_info_p->lba,
                                           read_info_p->blocks,
                                           read_info_p->position);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: failed in read fruts initialization for siots_p 0x%p, status = 0x%x\n",
                                 siots_p, status);

            return  status;
        }

        /* Goto the next fruts info.
         */
        read_info_p++;
        write_info_p++;
    }

    /* We expect the full access count to be exactly the number of data disks.
     */
    read_count = fbe_queue_length(&siots_p->read_fruts_head);
    if (RAID_COND(read_count != width))
    {
        /* There is a descrepency between the expected number of
         * fully accessible drives.
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: verify setup fruts. read count: 0x%x unexpected. width: 0x%x\n",
                             read_count, width); 
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* If a read fruts wasn't found something is wrong.
     */
    if (read_fruts_p == NULL)
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: verify setup fruts no valid read fruts found. degraded_bitmask: 0x%x\n",
                             degraded_bitmask);
        status = FBE_STATUS_DEAD;
    }
    
    /* Always return the execution status.
     */
    return(status);
}
/* end of fbe_mirror_verify_setup_fruts() */

/*!***************************************************************************
 *          fbe_mirror_recovery_verify_setup_overlay_buffers()
 *****************************************************************************
 *
 * @brief   This method will split and plant parent buffers into a verify sg
 *          for a read recovery verify request.  The possible overlays are:
 *          1.  There is no overlap between the verify request and parent
 *          2.  The beginning and/or middle and/or end overlpa with the parent
 *
 * @param   parent_read_fruts_p - Pointer to parent read fruts to overlay
 * @param   orig_verify_sg_p - Pointer to verify sg list
 * @param   verify_sg_count - Maximum number of usable sgs entries in verify sgl
 * @param   memory_info_p - Pointer to memory request information
 * @param   orig_verify_start - verify start lba
 * @param   orig_verify_count - number of blocks to be verified
 * 
 * @return  status - Typically FBE_STATUS_OK
 *
 * @note    The diagram below shows the worst case where an unalinged read
 *          request caused the verify request to extend below the read at the
 *          beginning and above the read at the end.  In the example below
 *          the original read request was for 32 blocks starting at lba 0x10.
 *          The resulting verify request is for 64 (0x40) blocks starting at
 *          lba 0x0. 
 *              v - Buffers come from those allocated for verify
 *              p - Buffers come from parent read request
 *                          lba
 *                    +----> 0x0 +========+
 *                    |  ^       | vvvvvv |
 *                    |  |  0x10 +--------+
 *                    |  |       | pppppp |
 *          verify sg-+0x40      | pppppp |
 *                    |  |       +--------+
 *                    |  |       | vvvvvv |
 *                    +---->0x40 +========+
 *
 * @author
 *  04/28/2010  Ron Proulx - Created from fbe_parity_recovery_verify_setup_overlay_sgs 
 *
 *****************************************************************************/
static fbe_status_t fbe_mirror_recovery_verify_setup_overlay_buffers(fbe_raid_fruts_t *parent_read_fruts_p,
                                                           fbe_sg_element_t *orig_verify_sg_p,
                                                           fbe_u32_t verify_sg_count,
                                                           fbe_raid_memory_info_t *memory_info_p,
                                                           fbe_lba_t orig_verify_start,
                                                           fbe_block_count_t orig_verify_count)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_lba_t           parent_start, verify_start;
    fbe_lba_t           parent_end, verify_end;
    fbe_block_count_t   total_blocks_to_assign = orig_verify_count;
    fbe_block_count_t   blocks_assigned = 0;
    fbe_sg_element_t   *verify_sg_p;
    fbe_sg_element_t   *parent_sg_p;
    fbe_s32_t           sgs_remaining = (fbe_s32_t)verify_sg_count;
    fbe_block_count_t  mem_left = fbe_raid_memory_info_get_bytes_remaining_in_page(memory_info_p);
    fbe_block_count_t   blocks_per_page;
    fbe_raid_siots_t    *parent_siots_p;

    /* Initialize parent and verify range.
     */
    blocks_per_page = (fbe_raid_memory_info_get_page_size_in_bytes(memory_info_p) / FBE_BE_BYTES_PER_BLOCK); 
    parent_start = parent_read_fruts_p->lba;
    parent_end = parent_start + parent_read_fruts_p->blocks - 1;
    verify_start = orig_verify_start;
    verify_end = verify_start + orig_verify_count - 1;
    verify_sg_p = orig_verify_sg_p;
    fbe_raid_fruts_get_sg_ptr(parent_read_fruts_p, &parent_sg_p);
    parent_siots_p = fbe_raid_fruts_get_siots(parent_read_fruts_p);
    if (parent_sg_p == NULL)
    {
        /* Unexpected.
         */
        fbe_raid_siots_object_trace(parent_siots_p,
                                    FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "mirror: recovery verify setup buffers - "
                                    "siots: 0x%llx blks: 0x%llx no parent sg found\n",
                                     (unsigned long long)orig_verify_start,
			             (unsigned long long)orig_verify_count);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /*! Insert parent buffers into the verify sg list.  We have allocated (2)
     *  additional sg entries so that we can split a verify sg for the beginning
     *  of the parent sgl and the end of the parent sgl.
     *
     *  @note This assumes that the parent sgl is uniformly distributed (i.e. no
     *        small sg entries in the middle of a full sg entries).
     *        
     */
    if (parent_start > verify_start)
    {
        fbe_lba_t           end_lba;
        fbe_block_count_t  blocks_to_assign;
        fbe_u32_t           sgs_used = 0;
        fbe_u32_t dest_byte_count =0;

        /* Populate beginning of verify request with buffers from verify 
         * request.  Since we are not using the parent buffers we use the
         * blocks remaining from the verify buffers when determining the
         * sg list length.
         */
        end_lba = FBE_MIN(verify_end, (parent_start - 1));
        blocks_to_assign = end_lba - verify_start + 1;
        blocks_assigned += blocks_to_assign; 
        verify_start += blocks_to_assign;
        
        /* Validate that there are sufficient sgs entries in the verify sgl.
         */
        mem_left = fbe_raid_sg_count_uniform_blocks(blocks_to_assign,
                                                                                        blocks_per_page,
                                                                                       mem_left,
                                                                                        &sgs_used);
        if ((fbe_s32_t)sgs_used > sgs_remaining)
        {
            /* Unexpected.
             */
            fbe_raid_siots_object_trace(parent_siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "mirror: recovery verify setup buffers - sgs required: 0x%x exceeded available: 0x%x total: 0x%x\n",
                                   sgs_used, sgs_remaining, verify_sg_count);

            return(FBE_STATUS_GENERIC_FAILURE);
        }

        if(!fbe_raid_get_checked_byte_count(blocks_to_assign, &dest_byte_count))
        {
            fbe_raid_siots_object_trace(parent_siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "%s: byte count exceeding 32bit limit \n",__FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE; 
        }
        /* Assign newly allocated memory to this verify range.
         */
        sgs_used = (fbe_u32_t)fbe_raid_sg_populate_with_memory(memory_info_p,
                                                                                        verify_sg_p, 
                                                                                       dest_byte_count);
        if (sgs_used == 0)
        {
            fbe_raid_siots_object_trace(parent_siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "%s: failed to populate sg for: memory_info_p = 0x%p\n",
                                     __FUNCTION__,memory_info_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        /* We need to update the verify sg pointer to the next available space.
         */
        sgs_remaining -= sgs_used;
        verify_sg_p = (verify_sg_p + sgs_used);
    }

    /* Setup the middle portion with the parent buffers if the verify request
     * ovelaps with the parent range.  If the entire verify request is not less
     * than the parent or greater than the parent then it overlaps.
     */
    if ( (blocks_assigned < total_blocks_to_assign) &&
        !((verify_end    < parent_start) ||
          (verify_start  > parent_end)      )           )
    {
        fbe_lba_t           start_lba, end_lba;
        fbe_block_count_t  blocks_to_assign;
        fbe_sg_element_with_offset_t tmp_sgd;
        fbe_u32_t           offset;
        fbe_block_count_t  parent_blks_remaining = 0;
        fbe_u32_t           sgs_used = 0;
        fbe_u16_t           sgs_used_16 = 0;

        /* The parent overlaps the verify request.  Use the parent buffers
         * to populate this range.  We should have done the beginning with
         * if needed so the parent start should less than or equal.
         */
        if (parent_start > verify_start)
        {
            /* Logical error.
             */
            fbe_raid_siots_object_trace(parent_siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "mirror: recovery verify setup buffers - parent_start: 0x%llx greater than verify_start: 0x%llx\n",
                                        (unsigned long long)parent_start,
			                (unsigned long long)verify_start);
            return(FBE_STATUS_GENERIC_FAILURE);
        }

        /* Populate the middle blocks with the parent buffers.  We simply
         * use the minimum of the parent and verify range.
         */
        start_lba = FBE_MAX(parent_start, verify_start);
        end_lba = FBE_MIN(parent_end, verify_end);
        blocks_to_assign = end_lba - start_lba + 1;
        blocks_assigned += blocks_to_assign;
        verify_start += blocks_to_assign;

        /* Now get the parent buffers using the correct offset.  The offset is
         * the difference from the original parent start lba and this start lba.
         * The offset can be non-zero when we are in mining mode.
         */
        offset = (fbe_u32_t)((start_lba - parent_start) * FBE_RAID_SECTOR_SIZE(parent_sg_p->address));
        fbe_sg_element_with_offset_init(&tmp_sgd, 
                                        offset,
                                        parent_sg_p,
                                        NULL /* use default get next sg fn */);

        /* Validate that there are sufficient sgs entries in the verify sgl.
         */
        parent_blks_remaining = tmp_sgd.sg_element->count / FBE_RAID_SECTOR_SIZE(parent_sg_p->address);
        status = fbe_raid_sg_count_nonuniform_blocks(parent_read_fruts_p->blocks,
                                                                    &tmp_sgd,
                                                                    FBE_RAID_SECTOR_SIZE(parent_sg_p->address),
                                                                    &parent_blks_remaining,
                                                                    &sgs_used);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_object_trace(parent_siots_p,
				        FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "mirror: failed to count non-uniform blocks: status = 0x%x, "
                                        "parent_blks_remaining = 0x%llx, siots_p = 0x%p\n",
                                        status,
					(unsigned long long)parent_blks_remaining,
					 parent_siots_p);
            return  status;
        }
        if ((fbe_s32_t)sgs_used > sgs_remaining)
        {
            /* Unexpected.
             */
            fbe_raid_siots_object_trace(parent_siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                   "mirror: recovery verify setup buffers - sgs required: 0x%x exceeded available: 0x%x total: 0x%x",
                                   sgs_used, sgs_remaining, verify_sg_count);
            return(FBE_STATUS_GENERIC_FAILURE);
        }

        /* Now re-initialize our temp sg descriptor.
         */
        fbe_sg_element_with_offset_init(&tmp_sgd, 
                                        offset,
                                        parent_sg_p,
                                        NULL /* use default get next sg fn */);

    if((blocks_to_assign *FBE_BE_BYTES_PER_BLOCK) > FBE_U32_MAX)
    {
            fbe_raid_siots_object_trace(parent_siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "mirror: %s byte count (0x%llx) crossing 32bit limit\n",
                                        __FUNCTION__, 
                                        (unsigned long long)(blocks_to_assign *FBE_BE_BYTES_PER_BLOCK));
            return  FBE_STATUS_GENERIC_FAILURE;
    }
        /* Now assign the parent buffers into the verify sg.
         */
        status = fbe_raid_sg_clip_sg_list(&tmp_sgd, 
                                          verify_sg_p, 
                                          (fbe_u32_t)(blocks_to_assign * FBE_BE_BYTES_PER_BLOCK),
                                          &sgs_used_16);
        sgs_used = (fbe_u32_t) sgs_used_16;
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_object_trace(parent_siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                   "mirror: %s failed to clip sg list: status = 0x%x, " 
                                   "verify_sg_p = 0x%p\n",
                                   __FUNCTION__, status, verify_sg_p);
            return  status;
        }

        /* We need to update the verify sg pointer to the next available space.
         */
        sgs_remaining -= sgs_used;
        verify_sg_p = (verify_sg_p + sgs_used);
        
    }

    /* Now populate any remaining buffers with the verify buffers.
     */
    if (blocks_assigned < total_blocks_to_assign)
    {
        fbe_block_count_t    blocks_to_assign;
        fbe_u32_t           sgs_used = 0;
        fbe_u32_t dest_byte_count =0;
        
        /* If the remaining verify is within the parent range something is
         * wrong.
         */
        if ((parent_start >= verify_start) ||
            (parent_end   >= verify_end)      )
        {
            /* Logical error.
             */
            fbe_raid_siots_object_trace(parent_siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "mirror: recovery verify setup buffers - parent_start: 0x%llx parent_end: 0x%llx within verify\n"
                                        "verify_start: 0x%llx verify_end: 0x%llx \n",
                                        (unsigned long long)parent_start,
				        (unsigned long long)parent_end,
                                        (unsigned long long)verify_start,
				        (unsigned long long)verify_end);
            return(FBE_STATUS_GENERIC_FAILURE);
        }

        /* The blocks to assign is the remaining verify range.
         */
        blocks_to_assign = verify_end - verify_start + 1;
        blocks_assigned += blocks_to_assign; 
        verify_start += blocks_to_assign;
        
        /* Populate ending of verify request with buffers from verify 
         * request.  Since we are not using the parent buffers we use the
         * blocks remaining from the verify buffers when determining the
         * sg list length.  Also validate that there are sufficient sgs 
         * entries in the verify sgl.
         */
        mem_left = fbe_raid_sg_count_uniform_blocks(blocks_to_assign,
                                                                                blocks_per_page,
                                                                                mem_left,
                                                                                &sgs_used);
        if ((fbe_s32_t)sgs_used > sgs_remaining)
        {
            /* Unexpected.
             */
            fbe_raid_siots_object_trace(parent_siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "mirror: recovery verify setup buffers - sgs required: 0x%x exceeded available: 0x%x total: 0x%x\n",
                                   sgs_used, sgs_remaining, verify_sg_count);
            return(FBE_STATUS_GENERIC_FAILURE);
        }

        if(!fbe_raid_get_checked_byte_count(blocks_to_assign, &dest_byte_count))
        {
            fbe_raid_siots_object_trace(parent_siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                                   "%s: byte count exceeding 32bit limit \n",
                                                   __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE; 
        }
        /* Assign newly allocated memory to this verify range.
         */
        sgs_used = fbe_raid_sg_populate_with_memory(memory_info_p,
                                                    verify_sg_p, 
                                                    dest_byte_count);
        if (sgs_used == 0)
        {
            fbe_raid_siots_object_trace(parent_siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "%s: failed to populate sg for: memory_info_p = 0x%p\n",
                                     __FUNCTION__,memory_info_p);
                return FBE_STATUS_GENERIC_FAILURE;
        }
        sgs_remaining -= sgs_used;
    }

    /* Now sanity check the results.
     */
    if ((blocks_assigned != total_blocks_to_assign) ||
        ((verify_start - 1) != verify_end)          ||
        (sgs_remaining < 0)                            )
    {
        /* Logical error.
         */
        fbe_raid_siots_object_trace(parent_siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "mirror: recovery verify setup buffers - total to assign: 0x%llx assigned: 0x%llx actual end: 0x%llx\n",
                                    (unsigned long long)total_blocks_to_assign,
			            (unsigned long long)blocks_assigned,
			            (unsigned long long)(verify_start - 1));
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    /* Always return the execution status.
     */
    return(status);
}
/*********************************************************
 * end of fbe_mirror_recovery_verify_setup_overlay_buffers()
 *********************************************************/

/*!***************************************************************************
 *          fbe_mirror_recovery_verify_setup_sgs()
 *****************************************************************************
 *
 * @brief   The method sets up the sgs for a read recovery verify.  We need
 *          to stitch together the parent buffers with any additional buffers
 *          that were required for alignment.
 *
 * @param   siots_p - Pointer the nested siots to execute verify
 * @param   read_info_p - Needed to validate that we don't exceed sg list
 * @param   memory_info_p - Pointer to memory request information
 * @param   verify_start - Start lba for verify request
 * @param   verify_count - Number of block for this verify request
 *
 * @return  fbe_status_t
 *
 * @note    For mirror raid groups (unlike parity raid groups) there is a
 *          one-to-one mapping between the parent lba range and this nested
 *          verify range. The allocated verify sg is for the entire verify 
 *          range. Therefore there is no need to stitch sgs since there is
 *          only (1).
 *
 * @author
 *  04/28/2010  Ron Proulx  - Created from fbe_parity_recovery_verify_setup_sgs
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_recovery_verify_setup_sgs(fbe_raid_siots_t *siots_p,
                                                  fbe_raid_fru_info_t *read_info_p,
                                                  fbe_raid_memory_info_t *memory_info_p,
                                                  fbe_lba_t verify_start, 
                                                  fbe_block_count_t verify_count)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_raid_siots_t   *parent_siots_p = fbe_raid_siots_get_parent(siots_p);
    fbe_raid_fruts_t   *read_fruts_p = NULL;
    fbe_raid_fruts_t   *parent_read_fruts_p = NULL;
    fbe_raid_fruts_t   *parent_read2_fruts_p = NULL;

    /* Initialize locals.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_siots_get_read_fruts(parent_siots_p, &parent_read_fruts_p);
    
    /* There should be a parent read fruts otherwise it is an error.
     */
    if (parent_read_fruts_p == NULL)
    {   
        /* Should be at least (1) parent read fruts
         */
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: recovery verify setup sgs - Expected at least (1) parent read fruts. algorithm: 0x%x \n",
                             siots_p->algorithm);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Mirror groups don't use the second read chain.  Therefore it is
     * an error of it isn't NULL.
     */
    fbe_raid_siots_get_read2_fruts(parent_siots_p, &parent_read2_fruts_p);
    if (parent_read2_fruts_p != NULL)
    {   
        /* Mirrors don't support a second read chain.
         */
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: recovery verify setup sgs - second parent read fruts no expected. algorithm: 0x%x \n",
                             siots_p->algorithm);
        return(FBE_STATUS_GENERIC_FAILURE);
    }
    
    /* For mirror raid group we only read from (1) position.  Therefore
     * we simply need to locate that position for the verify request and
     * use the parent buffers only for that position.  Walk thru the verify
     * read fruts and use the parent buffers with the parent position is
     * located.
     */
    while (read_fruts_p != NULL)
    {
        fbe_sg_element_t *sg_p;
        
        /* Get the sg pointer for this verify fruts.
         */
        fbe_raid_fruts_get_sg_ptr(read_fruts_p, &sg_p);
        
        /* If this verify position doesn't match the parent position simply
         * use the memory allocated for the verify request.
         */
        if (parent_read_fruts_p->position != read_fruts_p->position)
        {
             fbe_u16_t sg_total = 0;
             fbe_u32_t dest_byte_count =0;

            if(!fbe_raid_get_checked_byte_count(verify_count, &dest_byte_count))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                                   "%s: byte count exceeding 32bit limit \n",
                                                   __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE; 
            }
            /* Assign newly allocated memory for the whole verify region for
             * this position.
             */
            sg_total = fbe_raid_sg_populate_with_memory(memory_info_p,
                                             sg_p, 
                                                                                         dest_byte_count);
            if (sg_total == 0)
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                                    "%s: failed to populate sg for: siots = 0x%p\n",
                                                     __FUNCTION__,siots_p);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        else
        {
            /*! Else assign the buffers in the following order:
             *  1. Assign buffers from parent_read_fruts_p,
             *  2. Assign new ones
             *
             *  @note The allocated verify sg is for the entire verify range.
             *        Therefore there is no need to stitch sgs since there is
             *        only (1).
             */
            fbe_raid_fru_info_t *verify_fru_info_p = NULL;
            fbe_u32_t           num_sgs = 0;

            /* Get the size associated with this sg list so that we don't
             * overrun it.
             */
            fbe_raid_fru_info_get_by_position(read_info_p, 
                                              read_fruts_p->position, 
                                              fbe_raid_siots_get_width(siots_p),
                                              &verify_fru_info_p);
            if (verify_fru_info_p == NULL)
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "mir_rvr_stp_sgs failed get of vr fru for pos %d\n",
                                     read_fruts_p->position);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            num_sgs = fbe_raid_memory_sg_index_to_max_count(verify_fru_info_p->sg_index);
            
            /* Now (depending on how the parent buffers overlay) assign any parent
             * buffers as required.
             */
            status = fbe_mirror_recovery_verify_setup_overlay_buffers(parent_read_fruts_p,
                                                                      sg_p,
                                                                      num_sgs,
                                                                      memory_info_p,
                                                                      verify_start,
                                                                      verify_count);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                       "mirror : failed to setup buffers for sg status = 0x%x, siots_p = 0x%p\n",
                                     status, siots_p);
                return status;
            }

        } /* end else if this is the parent read position */

        /* Goto next verify read fruts position.
         */
        read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);
    
    } /* end while there are more verify read fruts */

    /* Debug code to prevent scatter gather overrun
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    status = fbe_raid_fruts_for_each(0xFFFF, read_fruts_p, 
                                     fbe_raid_fruts_validate_sgs, 0, 0);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to validate sgs of read_fruts_p 0x%p "
                             "for siots_p 0x%p. status = 0x%x\n",
                             read_fruts_p, siots_p, status);

        return  status;
    }
    /* Always return the execution status.
     */
    return(status);
}
/* end of fbe_mirror_recovery_verify_setup_sgs() */

/*!***************************************************************************
 *          fbe_mirror_verify_setup_sgs()
 *****************************************************************************
 *
 * @brief   Setup the sg lists for a mirror verify operation.
 *
 * @param   siots_p - this I/O.
 * @param   read_info_p - read information needed to validate
 * @param   memory_info_p - Pointer to memory request information
 *
 * @return fbe_status_t
 *
 * @author
 *  12/15/2009  Ron Proulx - Created from fbe_parity_verify_setup_sgs 
 * 
 *****************************************************************************/
static fbe_status_t fbe_mirror_verify_setup_sgs(fbe_raid_siots_t * siots_p,
                                                fbe_raid_fru_info_t *read_info_p,
                                                fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Need to use a different sg setup method based on algorithm.
     */
    switch(siots_p->algorithm)
    {
        case MIRROR_VR_BUF:
        case MIRROR_RD_VR:
            /* For recovery verify types of requests, we need to populate the 
             * parent buffers with the result of the recovered read data.
             * Also need to setup any write sgs for degraded positions (although
             * we will never write corrections or invalidations for a recovery
             * verify the xor library expects the proper sgs).
             */        
            status = fbe_mirror_recovery_verify_setup_sgs(siots_p,
                                                          read_info_p,
                                                          memory_info_p,
                                                          siots_p->parity_start,
                                                          siots_p->parity_count);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                 fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                      "mirror : failed to setup sgs for recovery verify: siots_p = 0x%p, "
                                      "status = 0x%x\n",
                                      siots_p, status);
                  return  status;
            }
            break;

        case MIRROR_VR:
        case MIRROR_VR_WR:
        case MIRROR_COPY_VR:
            /* Populate the read and write sgs with buffers.
             */
            status = fbe_mirror_read_setup_sgs_normal(siots_p, memory_info_p);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                 fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                      "mirror : failed to populate read sgs: siots_p = 0x%p, "
                                      "status = 0x%x\n",
                                      siots_p, status);
                  return  status;
            }
        
            /* Now setup any write sgs
             */
            status = fbe_mirror_write_setup_sgs_normal(siots_p, memory_info_p);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                 fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                      "mirror : failed to populate write sgs: siots_p = 0x%p, "
                                      "status = 0x%x\n",
                                      siots_p, status);
                  return  status;
            }
            break;

        default:
            /* Unsupported algorithm.
             */
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mirror: verify setup sgs unsupported algorithm: 0x%x\n",
                                 siots_p->algorithm);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    /* Always return the execution status.
     */
    return(status);
}
/******************************************
 * end fbe_mirror_verify_setup_sgs()
 ******************************************/

/*!**************************************************************
 * fbe_parity_verify_setup_sgs_for_next_pass()
 ****************************************************************
 * @brief
 *  Setup the sgs for another strip mine pass.
 *
 * @param siots_p - current I/O.               
 *
 * @return fbe_status_t
 *
 * @author
 *  05/03/2010  Ron Proulx  - Copied from fbe_parity_verify_setup_sgs_for_next_pass
 *
 ****************************************************************/
fbe_status_t fbe_mirror_verify_setup_sgs_for_next_pass(fbe_raid_siots_t * siots_p)
{
    /* We initialize status to failure since it is expected to be populated. */
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_fru_info_t     read_info[FBE_MIRROR_MAX_WIDTH];
    fbe_raid_fru_info_t     write_info[FBE_MIRROR_MAX_WIDTH];
    fbe_u16_t               num_sgs[FBE_RAID_MAX_SG_TYPE];
    fbe_raid_memory_info_t  memory_info;
    
    /* Zero the number of sgs by type
     */
    fbe_zero_memory(&num_sgs[0], (sizeof(fbe_u16_t) * FBE_RAID_MAX_SG_TYPE));

    /* Populate the fru info for this region.
     */
    status = fbe_mirror_verify_get_fru_info(siots_p, 
                                            &read_info[0],
                                            &write_info[0],
                                            &num_sgs[0],
                                            FBE_TRUE);
     if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
     {
          fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                               "mirror : failed to get fru info: siots_p = 0x%p, "
                               "status = 0x%x\n",
                               siots_p, status);
          return  status;
     }

    /* We don't re-size the sgls for either the read or write positions.
     * This is ok since we are not growing the size of the request and in-fact
     * we are shrinking the request size.  The sgls will be properly terminated. 
     */
    status = fbe_raid_siots_init_data_mem_info(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to initialize memory request info with status: 0x%x\n",
                             status);
        return(status); 
    }

    /* Now invoke the common method to plant the read buffers.
     */
    status = fbe_mirror_verify_setup_sgs(siots_p,
                                         &read_info[0],
                                         &memory_info);
     if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
     {
          fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                               "mirror : failed to setup sgs: siots_p = 0x%p status = 0x%x\n",
                               siots_p, status);
          return  status;
     }

    /* Assign buffers to write sgs.  We don't reset to the memory queue head 
     * since we want to continue from after the read buffer space.  The only
     * purpose of setting up the write sgs is that validation works and to
     * keep the xor code simple (always process the entire width).  It also
     * allows any error region information to be traced.
     */
    status = fbe_mirror_write_setup_sgs_normal(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
         fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                              "mirror: failed to setup write sgs for siots_p 0x%p status = 0x%x\n",
                              siots_p, status);
         return  status;
    }

    /* Now validate the sgs for this request
     */
    status = fbe_mirror_validate_fruts(siots_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {  
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to validate sgs with status: 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    } 

    return(status);
}
/*************************************************
 * end fbe_mirror_verify_setup_sgs_for_next_pass()
 *************************************************/

/*!**************************************************************
 *          fbe_mirror_verify_setup_resources()
 ****************************************************************
 * @brief
 *  Setup the newly allocated resources.
 *
 * @param siots_p - current I/O.
 * @param read_info_p - Pointer to array of per drive information               
 * @param write_info_p - Pointer to array of degraded information               
 *
 * @return fbe_status_t
 *
 * @author
 *  12/15/2009  Ron Proulx - Copied from fbe_parity_verify_set_resources
 *
 ****************************************************************/
fbe_status_t fbe_mirror_verify_setup_resources(fbe_raid_siots_t *siots_p, 
                                               fbe_raid_fru_info_t *read_info_p,
                                               fbe_raid_fru_info_t *write_info_p)
{
    /* We initialize status to failure since it is expected to be populated. */
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_memory_info_t  memory_info;
    fbe_raid_memory_info_t  data_memory_info;

    /* Initialize our memory request information
     */
    status = fbe_raid_siots_init_control_mem_info(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to apply buffer: siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);

        return  status;
    }

    status = fbe_raid_siots_init_data_mem_info(siots_p, &data_memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to init data memory info: status = 0x%x, siots_p = 0x%p\n",
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
    status = fbe_mirror_verify_setup_fruts(siots_p, 
                                           read_info_p,
                                           write_info_p,
                                           FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                           &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror:fail to setup fruts:siots=0x%p,stat=0x%x\n",
                             siots_p, status);

        return  status;
    }
    
    /* Initialize the RAID_VC_TS now that it is allocated.
     */
    status = fbe_raid_siots_init_vcts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {  
        fbe_raid_siots_trace(siots_p, 
                            FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                            "mirror: failed in vcts initialization for siots_p 0x%p, status = 0x%x\n",
                            siots_p, status);
        return  status;
    }

    /* Initialize the VR_TS.
     * We are allocating VR_TS structures WITH the fruts structures.
     */
    status = fbe_raid_siots_init_vrts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {  
        fbe_raid_siots_trace(siots_p, 
                            FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                            "mirror: failed in vrts initialization: siots_p 0x%p, status = 0x%x\n",
                            siots_p, status);
        return (status); 
    }

    /* Plant the allocated sgs into the locations calculated above.
     */
    status = fbe_raid_fru_info_array_allocate_sgs_sparse(siots_p, 
                                                         &memory_info, 
                                                         read_info_p, 
                                                         NULL, 
                                                         write_info_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {  
        fbe_raid_siots_trace(siots_p, 
                            FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                            "mirror: failed to allocate sgs: siots_p 0x%p, status = 0x%x\n",
                            siots_p, status);
        return (status); 
    }

    /* Assign buffers to sgs.
     * We reset the current ptr to the beginning since we always 
     * allocate buffers starting at the beginning of the memory. 
     */
    status = fbe_mirror_verify_setup_sgs(siots_p, read_info_p, &data_memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {  
        fbe_raid_siots_trace(siots_p, 
                            FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                            "mirror: failed to setup sgs: siots_p 0x%p, status = 0x%x\n",
                            siots_p, status);
        return (status); 
    }

    /* Now validate the sgs for this request
     */
    status = fbe_mirror_validate_fruts(siots_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {  
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to validate sgs with status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    } 
        
    return (status);
}
/******************************************
 * end fbe_mirror_verify_setup_resources()
 ******************************************/

/*!*****************************************************************
 *          fbe_mirror_verify_extent()
 *******************************************************************
 *
 * @brief   Verify an extent for this siots.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return - fbe_status_t
 *
 * @author
 *  12/18/2009  Ron Proulx - Created from fbe_parity_rebuild_extent 
 *
 *******************************************************************/
fbe_status_t fbe_mirror_verify_extent(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_xor_mirror_reconstruct_t verify;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* Initialize the sg desc and bitkey vectors.
     */
    status = fbe_mirror_setup_reconstruct_request(siots_p, &verify);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
          fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                               "mirror : failed to setup reconstruct request: siots_p = 0x%p, status = 0x%x\n",
                               siots_p, status);
          return  status;
    }

    /* Init the remaining parameters.  For mirrors the
     * extent is simply the siots parity_start and parity_blocks.
     */
    verify.start_lba = siots_p->parity_start;
    verify.blocks = siots_p->parity_count;
    if ( fbe_raid_geometry_is_sparing_type(raid_geometry_p) &&
        !fbe_raid_siots_is_metadata_operation(siots_p)         )
    {
        /* Don't check/generate lba stamps for a spare since the metadata is
         * not owned by this object.
         */
        verify.options = 0;
    }
    else
    {
        /* Else either not sparing type or a metadata request to a spare.
         */
        verify.options = FBE_XOR_OPTION_CHK_LBA_STAMPS;
    }
    verify.eboard_p = &siots_p->misc_ts.cxts.eboard;
    verify.eregions_p = &siots_p->vcts_ptr->error_regions;
    verify.error_counts_p = &siots_p->vcts_ptr->curr_pass_eboard;
    verify.raw_mirror_io_b = fbe_raid_geometry_is_raw_mirror(raid_geometry_p);

    /* The width MUST be the width of the raid group.
     */
    verify.width = fbe_raid_siots_get_width(siots_p);

    /* Set XOR debug options.
     */
    fbe_raid_xor_set_debug_options(siots_p, 
                                   &verify.options, 
                                   FBE_FALSE /* Don't generate error trace */);

    /* The current pass counts for the vcts should be cleared since
     * we are beginning another XOR.  Later on, the counts from this pass
     * will be added to the counts from other passes.
     */
    fbe_raid_siots_init_vcts_curr_pass( siots_p );

    /* Perform the verify.  
     */
    status = fbe_xor_lib_mirror_verify(&verify);
    return(status);
}
/*******************************
 * end fbe_mirror_verify_extent()
 *******************************/

/*!***************************************************************************
 *          fbe_mirror_verify_reduce_request_size()
 *****************************************************************************
 *
 * @brief   Update the siots request size for a verify request.  The request
 *          must still be aligned to the optimal block size.
 *
 * @param   siots_p - this I/O.
 *
 * @return  status - FBE_STATUS_OK - The request was modified as required
 *                   Other - Something is wrong with the request
 *
 * @author  
 *  02/09/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
static fbe_status_t fbe_mirror_verify_reduce_request_size(fbe_raid_siots_t *siots_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_block_size_t        optimal_block_size;
    fbe_block_count_t       reduced_block_count;

    /* First attempt is to dividet the request in half but the request must
     * remain a multiple of optimal block size.
     */
    fbe_raid_geometry_get_optimal_size(raid_geometry_p, &optimal_block_size);
    reduced_block_count = (siots_p->parity_count / 2);

    /* Currently we fail if we can't divide the request in half.
     */
    if ((reduced_block_count % optimal_block_size) != 0)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        /* Else update the siots request size.
         */
        siots_p->parity_count = reduced_block_count; 
    }

    /* Always return the execution status.
     */
    return(status);
}
/*************************************************
 * end fbe_mirror_verify_reduce_request_size() 
 ************************************************/

/*!***************************************************************************
 *          fbe_mirror_verify_limit_request()
 *****************************************************************************
 *
 * @brief   Limits the size of a mirror verify request.  There are
 *          (2) values that will limit the request size:
 *              1. Maximum per drive request
 *              2. Maximum amount of buffers that can be allocated
 *                 per request.
 *
 * @param   siots_p - this I/O.
 *
 * @return  status - FBE_STATUS_OK - The request was modified as required
 *                   Other - Something is wrong with the request
 *
 * @author  
 *  02/09/2010  Ron Proulx  - Created
 *
 *****************************************************************************/
fbe_status_t fbe_mirror_verify_limit_request(fbe_raid_siots_t *siots_p)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_u32_t           width = fbe_raid_siots_get_width(siots_p);
    
    /* Use the standard method to determine the page size.  For mirror verifys
     * we use the width for the number of fruts and the transfer size for the
     * number of blocks to allocate.
     */
    status = fbe_mirror_limit_request(siots_p,
                                      width,
                                      (width * siots_p->parity_count),
                                      fbe_mirror_verify_get_fru_info);

    /* Always return the execution status.
     */
    return(status);
}
/******************************************
 * end fbe_mirror_verify_limit_request()
 ******************************************/



/*!**************************************************************
 * fbe_mirror_report_corrupt_operations()
 ****************************************************************
 * @brief
 *  This function will report errors at the time of corrupt
 *  CRC as well as DATA operation.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return fbe_status_t
 *
 * @author
 *  03/08/11 - Created - Jyoti Ranjan
 ****************************************************************/
static fbe_status_t fbe_mirror_report_corrupt_operations(fbe_raid_siots_t * siots_p)
{
    fbe_u32_t region_index;
    fbe_u32_t error_code;
    fbe_u32_t vr_error_bits;
    fbe_u32_t array_pos;
    fbe_u16_t array_pos_mask;
    const fbe_xor_error_regions_t * regions_p;

    vr_error_bits = 0;
    regions_p = &siots_p->vcts_ptr->error_regions;

    for (array_pos = 0; array_pos < fbe_raid_siots_get_width(siots_p); array_pos++)
    {
        /* array_pos_mask has the current position in the error bitmask 
         * that we need to check below.
         */
        array_pos_mask = 1 << array_pos;

        /* traverse the xor error region for current disk position
         */
        for (region_index = 0; region_index < regions_p->next_region_index; ++region_index)
        {
            if ((regions_p->regions_array[region_index].error != 0) &&
                (regions_p->regions_array[region_index].pos_bitmask & array_pos_mask))
            {
                /* Determine the error reason code.
                 */
                error_code = regions_p->regions_array[region_index].error & FBE_XOR_ERR_TYPE_MASK;
                switch(error_code)
                {
                    case FBE_XOR_ERR_TYPE_CORRUPT_CRC:
                    case FBE_XOR_ERR_TYPE_CORRUPT_CRC_INJECTED:
                        vr_error_bits |= VR_CORRUPT_CRC;
                        break;

                    case FBE_XOR_ERR_TYPE_CORRUPT_DATA:
                    case FBE_XOR_ERR_TYPE_CORRUPT_DATA_INJECTED:
                        vr_error_bits |= VR_CORRUPT_DATA;
                        break;
                }

                /* We will log message even if drive is dead as we want to
                 * notify that correspondign sector of drive is lost.
                 */
                fbe_raid_send_unsol(siots_p,
                                    SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,
                                    array_pos,
                                    regions_p->regions_array[region_index].lba,
                                    regions_p->regions_array[region_index].blocks,
                                    vr_error_bits,
                                    fbe_raid_extra_info(siots_p));

            } /* end if ((regions_p->regions_array[region_idx].error != 0) ... */
        } /* end for (region_idx = 0; region_idx < regions_p->next_region_index; ++region_idx) */
    } /* end for (array_pos = 0; array_pos < fbe_raid_siots_get_width(siots_p); array_pos++) */ 

    return FBE_STATUS_OK;
}
/**********************************************
 * end fbe_mirror_report_corrupt_operations()
 **********************************************/


/*!***************************************************************
 *          fbe_mirror_report_errors()
 *****************************************************************
 *
 * @brief   Report any error encountered for a rebuild or verify
 *          request (errors are only reported of the verify count
 *          structure was supplied by the calling object).
 *
 * @param   siots_p - ptr to the fbe_raid_siots_t 
 * @param b_update_eboard - a flag indicating whether vp eboard has to
 *                          be updated or not with siot's vcts' overall 
 *                          eboard. We will do update only if flag
 *                          is FBE_TRUE.
 *
 * @return  fbe_status_t
 *
 * @author
 *      08/16/00  RG - Created.
 *      11/20/08  Pradnya Patankar - Modified for new event logging.
 *
 ****************************************************************/
fbe_status_t fbe_mirror_report_errors(fbe_raid_siots_t * siots_p, fbe_bool_t b_update_eboard)
{
    fbe_status_t status = FBE_STATUS_OK;
    const fbe_xor_error_regions_t * const regions_p = (siots_p->vcts_ptr == NULL) ? NULL : &siots_p->vcts_ptr->error_regions;

    /* We will do do error reporting using EBOARD only if we CAN NOT 
     * report using XOR error region.
     */
    if (regions_p != NULL)
    {
        /* Activate error reporting only if xor error region is not empty.
         */
        if (!FBE_XOR_ERROR_REGION_IS_EMPTY(regions_p))
        {
            if (fbe_raid_siots_is_corrupt_operation(siots_p))
            {
               /* We do report errors differently if siot's operation is  
                * corrupt CRC/Data. 
                */
                status = fbe_mirror_report_corrupt_operations(siots_p);
            }
            else
            {
                status = fbe_mirror_report_errors_from_error_region(siots_p);
            }
        }

        if (status != FBE_STATUS_OK) { return status; }

        /* We may have some errors which we went away after retry.
         * Also, we do not create any XOR error region for these
         * errors. However, we do log those errors.
         */
        status = fbe_mirror_report_retried_errors(siots_p);
        if (status != FBE_STATUS_OK) { return status; }
    }
    else
    {
        /* Report using eboard.
         */
        status = fbe_mirror_report_errors_from_eboard(siots_p);
    }

 
    /* Update the IOTS' error counts with the totals from this siots.
     * We will do so only if we are asked to do so.
     */
    if (b_update_eboard)
    {
        fbe_raid_siots_update_iots_vp_eboard( siots_p );
    }

    return status;
}
/************************************
 * end fbe_mirror_report_errors() 
 ***********************************/

/*!***************************************************************
 * fbe_mirror_report_errors_from_error_region()
 ****************************************************************
 * 
 * @brief   This function will report the errors using xor error 
 *          region in event log
 *
 * @param   siots_p - pointer to siots
 *
 * @return  fbe_status_t
 *
 * @author
 *   12/08/08  - Created for new event logging - Pradnya Patankar
 *   01/10/11  - Created for new event logging - Jyoti Ranjan
 ****************************************************************/
fbe_status_t fbe_mirror_report_errors_from_error_region(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    const fbe_xor_error_regions_t * const regions_p = &siots_p->vcts_ptr->error_regions;
    fbe_u32_t array_pos, actual_array_pos;
    fbe_u16_t array_pos_mask;
    fbe_u32_t region_idx;
    fbe_u32_t c_err_bits, uc_err_bits = 0;

    /* Report correctable and uncorrectable errors by scanning xor region for
     * each array position.
     */
    for (array_pos = 0; array_pos < fbe_raid_siots_get_width(siots_p); array_pos++)
    {
        array_pos_mask = 1 << array_pos;
        for (region_idx = 0; region_idx < regions_p->next_region_index;  ++region_idx)
        {
            if (regions_p->regions_array[region_idx].error != 0)
            {
                status = fbe_raid_get_error_bits_for_given_region(&regions_p->regions_array[region_idx],
                                                                  FBE_FALSE,
                                                                  array_pos_mask,
                                                                  FBE_TRUE,
                                                                  &c_err_bits);
                if (RAID_COND(status != FBE_STATUS_OK)) { return status; }

                status = fbe_raid_get_error_bits_for_given_region(&regions_p->regions_array[region_idx], 
                                                                  FBE_FALSE, 
                                                                  array_pos_mask,
                                                                  FBE_FALSE,
                                                                  &uc_err_bits);
                if (RAID_COND(status != FBE_STATUS_OK)) { return status; }

                if (regions_p->regions_array[region_idx].pos_bitmask & array_pos_mask)
                {
                    fbe_raid_geometry_t * raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

                    /* We are here as we have an xor error region for current array position.
                     */
                    if (c_err_bits || uc_err_bits)
                    {
                        /* Adjust the array position to be relative to the unit,rather than the 
                         * mirror if necessary.
                         */
                        if ((!fbe_raid_geometry_is_sparing_type(raid_geometry_p)) && 
                            (fbe_raid_geometry_is_raid10(raid_geometry_p)))
                        {
                            actual_array_pos = fbe_mirror_get_position_from_index(siots_p, array_pos);
                        }
                        else
                        {
                            actual_array_pos = array_pos;
                        }

                        if ((uc_err_bits != 0) &&
                            (regions_p->regions_array[region_idx].error & FBE_XOR_ERR_TYPE_UNCORRECTABLE))
                        {
                            status = fbe_mirror_log_uncorrectable_error(siots_p,
                                                                        uc_err_bits,
                                                                        region_idx,
                                                                        actual_array_pos,
                                                                        array_pos,
                                                                        regions_p->regions_array[region_idx].lba);

                            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                            {
                                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                                     "mirror: failed to log uncorrectable error: siots_p = 0x%p, status = 0x%x\n", 
                                                     siots_p ,status);
                                return status;
                            }
                        }

                        if (c_err_bits != 0)
                        {
                            /* We are here as we are having correctable errors but will report only of 
                             * we have correctable error bits as non-zero.
                             */
                            
                            status = fbe_mirror_log_correctable_error(siots_p,
                                                                      c_err_bits,
                                                                      region_idx,
                                                                      actual_array_pos,
                                                                      array_pos,
                                                                      regions_p->regions_array[region_idx].lba);
                            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                            {
                                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                                     "mirror: failed to log correctable error: status = 0x%x, siots_p = 0x%p\n",
                                                     status, siots_p);
                                return status;
                            }
                        } /* end if (c_err_bits != 0) */ 

                    }/* end if (c_err_bits || uc_err_bits) */
                }/* end  if (regions_p->regions_array[region_idx].pos_bitmask & array_pos_mask) */
            }/* if (regions_p->regions_array[region_idx].error != 0) */
        }/* end for(region...)*/
    }/* end for (array_pos = 0.....) */

    fbe_mirror_log_dead_peer_invalidation(siots_p);
    return status;
}
/***************************************************
 * end fbe_mirror_report_errors_from_error_region() 
 **************************************************/



/*!***************************************************************
 * fbe_mirror_report_retried_errors()
 *****************************************************************
 * @brief
 *  This function will record the errors whcih were detected but 
 *  did not surface again on retry.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return fbe_status_t
 *
 * @author
 *  02/10/11 - Created - Jyoti Ranjan
 *
 ****************************************************************/
static fbe_status_t fbe_mirror_report_retried_errors(fbe_raid_siots_t * siots_p)
{
    fbe_u32_t array_pos = 0;
    fbe_u16_t array_pos_mask = 0;
    fbe_u32_t width = fbe_raid_siots_get_width(siots_p);

    for (array_pos = 0; array_pos < width; array_pos++)
    {
        array_pos_mask = 1 << array_pos;

        if ((siots_p->vrts_ptr->retried_crc_bitmask & array_pos_mask ) != 0 )
        {
            /* If we are here then it means that we have to log
             * retry message for the current position.
             */
            fbe_raid_send_unsol(siots_p,
                                SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED,
                                array_pos,
                                siots_p->parity_start,
                                (fbe_u32_t)siots_p->parity_count,
                                fbe_raid_error_info(VR_CRC_RETRY),
                                fbe_raid_extra_info(siots_p));

        } /* end if ((siots_p->vrts_ptr->retried_crc_bitmask & array_pos_mask ) != 0 ) */ 
    } /* end for (array_pos = 0; array_pos < fbe_raid_siots_get_width(siots_p); array_pos++) */

    return FBE_STATUS_OK;
}
/*******************************************
 *end fbe_mirror_report_retried_errors() 
 ******************************************/

/*!*********************************************************************
 * fbe_mirror_log_dead_peer_invalidation()
 ***********************************************************************
 * @brief
 *  Report invalidated messages against a dead peer for stips having
 *  uncorrectable errors. Example: let us assume that dual mirror group's
 *  secondary disk is dead. So, any sector in primary disk will cause 
 *  uncorrectable. This functions however logs an uncorrectable messages 
 *  against a dead drive if correspnding strip(s) is/are uncorrectable.
 *
 * @param  siots_p    - pointer to siots
 *
 * @return fbe_status_t
 *
 * @author
 *  11/20/08 - Created - Jyoti Ranjan
 *
 ***********************************************************************/
static fbe_status_t fbe_mirror_log_dead_peer_invalidation(fbe_raid_siots_t * siots_p)
{
    fbe_u32_t region_index, scan_index;
    fbe_u16_t uc_strip_bitmask = 0;
    fbe_bool_t xor_region_traversed[FBE_XOR_MAX_ERROR_REGIONS] = { 0 };
    fbe_xor_error_regions_t * regions_p = NULL;
    fbe_xor_error_regions_t * scan_regions_p = NULL;
    fbe_u32_t xor_error_code; 
    
    regions_p = scan_regions_p = (siots_p->vcts_ptr == NULL) ? NULL : (&siots_p->vcts_ptr->error_regions);
    if ((regions_p == NULL) || FBE_XOR_ERROR_REGION_IS_EMPTY(regions_p))
    {
        return FBE_STATUS_OK;
    }

    
    /* We will do following:
     *   1. Walk through each xor error region to figure out if it can 
     *      cause uncorrectable strip(s) standalone or in combination 
     *      with other xor error region(s).
     *   2. If there is an uncorrectable strip(s) then log parity 
     *      invalidation message only if peer is dead.
     */
    for(region_index = 0; region_index < regions_p->next_region_index; ++region_index)
    {
         xor_error_code = regions_p->regions_array[region_index].error & FBE_XOR_ERR_TYPE_MASK;
         if ((siots_p->algorithm == MIRROR_RD || siots_p->algorithm == MIRROR_RD_VR) &&
             (xor_error_code == FBE_XOR_ERR_TYPE_CORRUPT_CRC || 
              xor_error_code == FBE_XOR_ERR_TYPE_CORRUPT_DATA ||
              xor_error_code == FBE_XOR_ERR_TYPE_RAID_CRC ||
              xor_error_code == FBE_XOR_ERR_TYPE_INVALIDATED))
         {
            /* If this is a read operation and the block has been invalidated 
             * already do not log anything. This will cause errors on  reads 
             * to be logged on the first encounter and no future ones. So.
             * give a pass to these cases. Also, mark this region as traversed
             * so that we no longer consider it in further reporting.
             */
            xor_region_traversed[region_index] = FBE_TRUE;
         }
        
        /* Flush-out previous state of uncorrectable as well correctable strip bitmask
         */
        uc_strip_bitmask = 0;

        if (xor_region_traversed[region_index] != FBE_TRUE)
        {
            if (regions_p->regions_array[region_index].error & FBE_XOR_ERR_TYPE_UNCORRECTABLE)
            {
                uc_strip_bitmask = regions_p->regions_array[region_index].pos_bitmask;
            }
            /* Scan through all regions to figure out all other error-regions 
             * collaboarting with current xor error region forms uncorrectable 
             * strips or not. In other words, ultimate objective is to find 
             * a chain of xor error regions causing strips to be uncorrectable.
             */
            for (scan_index = 0; scan_index < scan_regions_p->next_region_index; ++scan_index)
            {
                if ((scan_regions_p->regions_array[scan_index].lba == regions_p->regions_array[region_index].lba) &&
                    (scan_regions_p->regions_array[scan_index].blocks >= regions_p->regions_array[region_index].blocks))
                {
                    if (scan_regions_p->regions_array[scan_index].error & FBE_XOR_ERR_TYPE_UNCORRECTABLE)
                    {
                        uc_strip_bitmask = uc_strip_bitmask | scan_regions_p->regions_array[scan_index].pos_bitmask;
                    }

                    /* Drop this region from further scanning as we have already covered  
                     * all possible cases (correctable as well as uncorrectable) where
                     * it could have been contributed for a  given strip(s).
                     */
                    xor_region_traversed[scan_index] = FBE_TRUE;
                }
            } /* end for (scan_index = 0; scan_index < scan_region_p->next_region_index; ++scan_index) */
        } /* end if (xor_region_traversed[region_index] != FBE_TRUE) */

        /* By now, we must have traversed this region. So, update traverse list to reflect it.
         */
        xor_region_traversed[region_index] = FBE_TRUE;

        if (uc_strip_bitmask != 0)
        {
            fbe_u32_t position = 0;
            fbe_u32_t width = fbe_raid_siots_get_width(siots_p);

            for (position = 0; position < width; position++)
            {
                if (fbe_raid_siots_pos_is_dead(siots_p, position))
                {
                    fbe_raid_send_unsol(siots_p,
                                        SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR,
                                        position,
                                        regions_p->regions_array[region_index].lba,
                                        regions_p->regions_array[region_index].blocks,
                                        fbe_raid_error_info(0),
                                        fbe_raid_extra_info(siots_p));

                    fbe_raid_send_unsol(siots_p,
                                        SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED,
                                        position,
                                        regions_p->regions_array[region_index].lba,
                                        regions_p->regions_array[region_index].blocks,
                                        fbe_raid_error_info(0),
                                        fbe_raid_extra_info(siots_p));
                } /* end if (fbe_raid_siots_pos_is_dead(siots_p, pos)) */
            } /*  end  for (position = 0; position < width; position++) */ 
        } /* end if (uc_strip_bitmask != 0) */
    }  /* end for(region_index = 0; region_index < regions_p->next_region_index; ++region_index) */

    return FBE_STATUS_OK;
}
/***********************************************
 * end fbe_mirror_log_dead_peer_invalidation()
 ***********************************************/

/*!***************************************************************
 * fbe_mirror_report_errors_from_eboard()
 ****************************************************************
 * 
 * @brief   This function report errors in event log using eboard

 * @param   siots_p - pointer to siots
 *
 * @return  fbe_status_t
 *
 * @author
 *      08/16/00  RG - Created.
 *      12/08/08  PP - Created for new event logging.
 *      12/08/08  MA - Modified for BVA
 *
 ****************************************************************/
fbe_status_t fbe_mirror_report_errors_from_eboard(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t array_pos, actual_array_pos;
    fbe_u32_t sunburst = 0; 
    
   

    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* When we are logging the error from eboard, we will only have the 
     * fru position and there is not information available about the number of 
     * blocks, hence it is initialized to one.
     */
    fbe_u32_t blocks = FBE_RAID_MIN_ERROR_BLOCKS;

    /* Get the opcode from SIOTS
     */
    fbe_raid_siots_get_opcode(siots_p, &opcode);

    /* Report errors using eboard
     */
    for (array_pos = 0; array_pos < fbe_raid_siots_get_width(siots_p); array_pos++)
    {
        fbe_lba_t data_pba;
        fbe_u32_t c_err_bits = 0;
        fbe_u32_t uc_err_bits = 0;
        fbe_u16_t array_pos_mask = 1 << array_pos;        

        /* Adjust the array position to be relative to the unit,
         * rather than the mirror if necessary.
         */
        if ((!fbe_raid_geometry_is_sparing_type(raid_geometry_p)) && 
            (fbe_raid_geometry_is_raid10(raid_geometry_p)))
        {
            actual_array_pos = fbe_mirror_get_position_from_index(siots_p, array_pos);
        }
        else
        {
            actual_array_pos = array_pos;
        }

        /* Determine if there are correctable errors, since we need to log them.
         */
        c_err_bits = fbe_raid_get_correctable_status_bits(siots_p->vrts_ptr, array_pos_mask);
        if (c_err_bits != 0 )
        {
            /* Correctable errors.
             */
            data_pba = fbe_raid_get_region_lba_for_position(siots_p, array_pos, siots_p->parity_start);

            /* Correctable errors are reported from highest to lowest
             * priority.  Thus LBA stamp errors are the highest.
             */
            /* if we are doing BVA write then we will log all the correctable
             * errors as recontructed.
             */
            if (siots_p->algorithm == MIRROR_VR_WR)
            {
                sunburst = SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED;
            }
            else if ((c_err_bits & VR_REASON_MASK) == VR_LBA_STAMP)
            {
                /* We want to cause a phone-home for LBA stamp
                 * errors since it could indicate that the drive
                 * is doing something bad.
                 */
                sunburst = SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR;
            }
            else if (c_err_bits & VR_COH)
            {
                if(opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY)
                {
                    /* Coherency errors are expected when there is an incomplete write which
                     * would have kicked off a incomplete write verify.
                     * Log it as Info so that it does not call home but we have record that we
                     * hit this.
                     */
                    sunburst = SEP_INFO_CODE_RAID_EXPECTED_COHERENCY_ERROR;
                }
                else
                {
                    /* Any time we see a coherency error,
                     * we want to log a message of higher severity to cause a call home.
                     */
                    sunburst = SEP_ERROR_CODE_RAID_HOST_COHERENCY_ERROR;
                }
            }
            else if (c_err_bits & VR_UNEXPECTED_CRC)
            {
                if ((siots_p->vrts_ptr->eboard.crc_multi_and_lba_bitmap & (1 << array_pos)) && 
                    fbe_raid_library_get_encrypted()) {
                    /* Mark it as expected since if we see these the drive is not likely bad, 
                     * it is more likely a software error, from which the only recovery is to reconstruct. 
                     */
                    sunburst = SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED;
                    /* Do not report error trace if we may have read this area with a different key.
                     */
                    if (opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA){
                        fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                                    "unexpected multi-bit crc w/lba st error pos: %d lba: 0x%llx bl: 0x%x\n",
                                                    array_pos, data_pba, blocks);
                    }
                } else {
                    /* Any time we see a correctable unexpected CRC error, we want 
                     * to log a message of higher severity to cause a call home.
                     */
                    sunburst = SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR;
                }

                /* If an unexpected CRC error is found, notify the DH so it can
                 * properly handle the potential for a bad drive.
                 *
                 * If this is a BVA I/O or the LUN is scheduled for Background Verify,
                 * do not inform DH, as the CRC error might be due to an interrupted write,
                 * rather than a drive problem.
                 */

                /*! @todo
                 * Currently, there is function which is equivalent to
                 * RG_LUN_IN_BV(RG_GET_UNIT(siots_p). Need to add condition for that.
                 */

            } /* end else if ( err_bits & VR_UNEXPECTED_CRC ) */
            else
            {
                sunburst = SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED;
            }

            fbe_raid_send_unsol(siots_p,
                                sunburst,
                                actual_array_pos,
                                data_pba,
                                blocks,
                                fbe_raid_error_info(c_err_bits),
                                fbe_raid_extra_info(siots_p));

        } /* end if (c_err_bits != 0) */
        

        /* Determine if there are uncorrectable errors, since we need to log them.
         */
        uc_err_bits = fbe_raid_get_uncorrectable_status_bits( &siots_p->vrts_ptr->eboard, array_pos_mask );
        if (uc_err_bits != 0)
        {
            data_pba = fbe_raid_get_region_lba_for_position(siots_p, array_pos, siots_p->parity_start);

            /* If this is a read operation and the block has been invalidated
             * already do not log anything.  This will cause errors on  reads
             * to be logged on the first encounter and no future ones.  Other
             * operations log every time.
             */
            if ((siots_p->algorithm == MIRROR_RD || siots_p->algorithm == MIRROR_RD_VR) &&
                (uc_err_bits == VR_CORRUPT_CRC || uc_err_bits == VR_INVALID_CRC || uc_err_bits == VR_RAID_CRC))
            {
            }
            /* The operation is either the first read or not a read at all.
             * Log the uncorrectable error.
             * If this a BVA verify, then the error is expected and should 
             * be corrected by the subsequent write, so a lower severity
             * error is logged.
             */
            else if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE)
            {
                sunburst = SEP_INFO_RAID_HOST_CORRECTING_WITH_NEW_DATA;
                fbe_raid_send_unsol(siots_p,
                                    sunburst,
                                    array_pos,
                                    data_pba,
                                    blocks,
                                    fbe_raid_error_info(uc_err_bits),
                                    fbe_raid_extra_info(siots_p));
            }
            else
            {
                sunburst = SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR;
                fbe_raid_send_unsol(siots_p,
                                    sunburst,
                                    actual_array_pos,
                                    data_pba,
                                    blocks,
                                    fbe_raid_error_info(uc_err_bits),
                                    fbe_raid_extra_info(siots_p));

                /* Done logging the uncorrectable sectors,
                 * so it's time to log the sector invalidated messages.
                 */
                sunburst = SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED;

                /*! @todo Currently, there is function 
                 * which is equivalent to                      .
                 * !SKIP_CRC_DRIVE_KILLING(RG_GET_UNIT(siots_p). 
                 * Need to add condition for that              .
                 */

                fbe_raid_send_unsol(siots_p,
                                    sunburst,
                                    actual_array_pos,
                                    data_pba,
                                    blocks,
                                    fbe_raid_error_info(uc_err_bits),
                                    fbe_raid_extra_info(siots_p));
            } 
        }/* end  if (uc_err_bits != 0) */
    } /* End for all array positions. */

    return status;
}
/**********************************************
 * end fbe_mirror_report_errors_from_eboard()
 *********************************************/

/*!***************************************************************
 * fbe_mirror_log_correctable_error()
 ****************************************************************
 * 
 * @brief   This function will report the correctable error in ulog.
 *
 * @param   siots_p - pointer to siots
 * @param   c_err_bits       - bits explaining type and reason of error
 * @param   region_idx       - index in the FBE_XOR_ERROR_REGION array
 * @param   actual_array_pos - array position.
 * @param   relative_pos     - position relative to the raid group.
 * @param   data_pba         - PBA on which error has occured.
 *
 * @return  fbe_status_t
 *
 * @author
 *  11/20/08  - Created - Pradnya Patankar
 *
 ****************************************************************/
fbe_status_t fbe_mirror_log_correctable_error(fbe_raid_siots_t * siots_p,
                                              fbe_u32_t c_err_bits,
                                              fbe_u32_t region_idx,
                                              fbe_u16_t array_pos,
                                              fbe_u16_t relative_pos,
                                              fbe_lba_t data_pba)
{
    /* It is required for usurper command support.
     */
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u16_t error_type;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_u32_t sunburst = 0;
    const fbe_xor_error_regions_t * const regions_p = &siots_p->vcts_ptr->error_regions;

    fbe_raid_siots_get_opcode(siots_p, &opcode);
    error_type = regions_p->regions_array[region_idx].error;
 
    /* Correctable errors are reported from highest to lowest priority. 
     * Thus LBA stamp errors are the highest.
     *
     * However if this a BVA verify, then the error is expected 
     * and should be corrected by the subsequent write, so a lower
     * severity error is logged.
     */
    if (siots_p->algorithm == MIRROR_VR_WR )
    {
        sunburst = SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED;
    }
    else if (error_type == FBE_XOR_ERR_TYPE_LBA_STAMP) 
    {
        /* We want to cause a phone-home for LBA stamp errors 
         * since it could indicate that the drive is doing something bad.
         */
        sunburst = SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR;
    }
    else if (error_type == FBE_XOR_ERR_TYPE_COH) 
    {
        if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY)
        {
            /* Coherency errors are expected when there is an incomplete write which
             * would have kicked off a incomplete write verify.
             * Log it as Info so that it does not call home but we have record that we
             * hit this.
             */
            sunburst = SEP_INFO_CODE_RAID_EXPECTED_COHERENCY_ERROR;
        }
        else
        {
            /* Any time we see a coherency error, we want to log a message
             * of higher severity to cause a call home.
             */
            sunburst = SEP_ERROR_CODE_RAID_HOST_COHERENCY_ERROR;
        }
    }
    else if (FBE_XOR_ERROR_TYPE_IS_UNKNOWN_CRC(error_type))
    {
        if ((siots_p->vrts_ptr->eboard.crc_multi_and_lba_bitmap & (1 << array_pos)) && 
            fbe_raid_library_get_encrypted()) {
            /* Mark it as expected since if we see these the drive is not likely bad, 
             * it is more likely a software error, from which the only recovery is to reconstruct. 
             */
            sunburst = SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED;
            /* Do not report error trace if we may have read this area with a different key.
             */
            if (opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA){
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                            "unexpected multi-bit crc w/lba st error pos: %d lba: 0x%llx bl: 0x%x\n",
                                            array_pos,
                                            data_pba,
                                            regions_p->regions_array[region_idx].blocks);
            }
        } else {
            /* Any time we see a correctable unexpected CRC error,
             * we want to log a message of higher severity to cause a call home.
             */
            sunburst = SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR;
        }

        /* If an unexpected CRC error is found, notify the DH so it can
         * properly handle the potential for a bad drive.
         *
         * Since we will work off the raid group, send the position relative to
         * the RG at this level. This may differ from the absolute position if we
         * are dealing layers of RG's, as in RAID 10.
         *
         * If this is a BVA I/O or the LUN is scheduled for Background Verify,
         * do not inform DH, as the CRC error might be due to an interrupted write,
         * rather than a drive problem.
         */


         /*! @todo
          * Currently, there is function which is equivalent to
          * !SKIP_CRC_DRIVE_KILLING(RG_GET_UNIT(siots_p).
          * Need to add condition for that.
          */

    } /* end else if (FBE_XOR_ERROR_TYPE_IS_UNKNOWN_CRC(error_type)) */
    else
    {
        /* All other flavors of corrections are logged to the event log.
         * as sector reconstructed.
         */
        sunburst = SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED;
    }

    fbe_raid_send_unsol(siots_p,
                        sunburst,
                        array_pos,
                        data_pba,
                        regions_p->regions_array[region_idx].blocks,
                        fbe_raid_error_info(c_err_bits),
                        fbe_raid_extra_info(siots_p));

    return status;
}
/********************************************
 * end fbe_mirror_log_correctable_error()
 *******************************************/

/*!***************************************************************
 * fbe_mirror_log_uncorrectable_error()
 ****************************************************************
 *
 * @brief  This function will report the uncorrectable errors 
 *         in event log
 *
 * @param   siots_p     - pointer to siots
 * @param   uc_err_bits - error bits indicating reason and type of errors
 * @param   region_idx  - index in the FBE_XOR_ERROR_REGION array
 * @param   actual_array_pos - array position.
 * @param   relative_pos - position relative to the raid group
 * @param   data_pba    - PBA on which error has occured.
 *
 * @return  fbe_status_t.
 *
 * @author
 *  11/21/08  Created - Pradnya Patankar
 ****************************************************************/
fbe_status_t fbe_mirror_log_uncorrectable_error(fbe_raid_siots_t * siots_p,
                                                fbe_u32_t uc_err_bits,
                                                fbe_u32_t region_idx,
                                                fbe_u16_t array_pos,
                                                fbe_u16_t relative_pos,
                                                fbe_lba_t data_pba)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t sunburst = 0;
    fbe_payload_block_operation_opcode_t opcode;
    const fbe_xor_error_regions_t * const regions_p = &siots_p->vcts_ptr->error_regions;

    fbe_raid_siots_get_opcode(siots_p, &opcode);
 
    if ((siots_p->algorithm == MIRROR_RD || siots_p->algorithm == MIRROR_RD_VR) &&
        (uc_err_bits == VR_CORRUPT_CRC || uc_err_bits == VR_INVALID_CRC || uc_err_bits == VR_RAID_CRC))
    {
        /* If this is a read operation and the block has been invalidated 
         * already do not log anything. This will cause errors on  reads 
         * to be logged on the first encounter and no future ones. So.
         * give a pass to these cases.
         */
    }
    else
    {
        /* The operation is either the first read or not a read at all
         * Log the uncorrectable error.
         * If this a BVA verify, then the error is expected and should 
         * be corrected by the subsequent write, so a lower severity
         * error is logged.
         */
        if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE)
        {
            sunburst = SEP_INFO_RAID_HOST_CORRECTING_WITH_NEW_DATA;

            fbe_raid_send_unsol(siots_p,
                                sunburst,
                                array_pos,
                                data_pba,
                                regions_p->regions_array[region_idx].blocks,
                                fbe_raid_error_info(uc_err_bits),
                                fbe_raid_extra_info(siots_p));
        }
        else
        {
            BITS_32E error_type = regions_p->regions_array[region_idx].error;

            sunburst = SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR;

            if (FBE_XOR_ERROR_TYPE_IS_UNKNOWN_CRC(error_type))
            {
            } /* end if (FBE_XOR_ERROR_TYPE_IS_UNKNOWN_CRC(error_type)) */

            fbe_raid_send_unsol(siots_p,
                                sunburst,
                                array_pos,
                                data_pba,
                                regions_p->regions_array[region_idx].blocks,
                                fbe_raid_error_info(uc_err_bits),
                                fbe_raid_extra_info(siots_p));

            /* Also log sector invalidated message, which we log whenever we 
             * log uncorrectable sector message
             */
            sunburst = SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED;
            fbe_raid_send_unsol(siots_p,
                                sunburst,
                                array_pos,
                                data_pba,
                                regions_p->regions_array[region_idx].blocks,
                                fbe_raid_error_info(uc_err_bits),
                                fbe_raid_extra_info(siots_p));
        }/* end else if( siots_p->algorithm == MIRROR_WR_VR )*/
    }/* end else if ((siots_p->algorithm == MIRROR_RD ||....) */

    return status;
}
/**********************************************
 * end fbe_mirror_log_uncorrectable_error()
 *********************************************/

/*!***************************************************************
 * fbe_mirror_only_invalidated_in_error_regions()
 ****************************************************************
 *
 * @brief  Check if the error regions only contain invalidated sectors
 *
 * @param   siots_p     - pointer to siots
 *
 * @return  boolean 
 *
 * @author
 *  12/2014 Created - Deanna Heng
 ****************************************************************/
fbe_bool_t fbe_mirror_only_invalidated_in_error_regions(fbe_raid_siots_t * siots_p)
{
    fbe_u32_t region_idx;
    const fbe_xor_error_regions_t * regions_p = &siots_p->vcts_ptr->error_regions;
    fbe_u32_t error_code;

    for (region_idx = 0; region_idx < regions_p->next_region_index; ++region_idx)
    {
        if (regions_p->regions_array[region_idx].error & FBE_XOR_ERR_TYPE_UNCORRECTABLE)
        {
            error_code = regions_p->regions_array[region_idx].error & FBE_XOR_ERR_TYPE_MASK;
            if((siots_p->algorithm == MIRROR_RD || siots_p->algorithm == MIRROR_RD_VR) &&
               ((error_code == FBE_XOR_ERR_TYPE_RAID_CRC) || 
                (error_code == FBE_XOR_ERR_TYPE_INVALIDATED) || 
                (error_code == FBE_XOR_ERR_TYPE_CORRUPT_CRC)))
            {
                /* read on already invalidated data, do nothing*/
            }
            else
            {
                return FBE_FALSE;
            }
        }
        else
        {
            return FBE_FALSE;
        }
    }

    return FBE_TRUE;
                
}
/**********************************************
 * end fbe_mirror_only_invalidated_in_error_regions()
 *********************************************/

/*!***************************************************************
 * fbe_mirror_set_raw_mirror_status()
 ****************************************************************
 *
 * @brief  This function will check the siots eboard and set the
 *          block status and qualifier for a raw mirror.
 *
 * @param   siots_p     - pointer to siots
 * @param   xor_status  - status returned from XOR operations
 *
 * @return  none
 *
 * @author
 *  11/2011 Created - Susan Rundbaken
 ****************************************************************/
void fbe_mirror_set_raw_mirror_status(fbe_raid_siots_t * siots_p, fbe_xor_status_t xor_status)
{
    if (xor_status == FBE_XOR_STATUS_NO_ERROR)
    {
        if ((siots_p->misc_ts.cxts.eboard.c_rm_magic_bitmap) ||
            (siots_p->misc_ts.cxts.eboard.c_rm_seq_bitmap))
        {
            /* Correctable errors on raw mirror.
             * Set error status and return.
             */
            fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
            fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RAW_MIRROR_MISMATCH);

            return;
        }

        /* No raw mirror errors reported.
         * Set success status and return.
         */
        fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS);
        fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);

        return;
    }

    if (xor_status == FBE_XOR_STATUS_CHECKSUM_ERROR)
    {
        /* Indicate that recovery verify wasn't able to recover the data.
         */
        fbe_raid_siots_set_block_status(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_STATUS_MEDIA_ERROR);
        fbe_raid_siots_set_block_qualifier(siots_p, FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_MEDIA_ERROR_DATA_LOST);
    }

    return;
}
/**********************************************
 * end fbe_mirror_set_raw_mirror_status()
 *********************************************/
/*!**************************************************************
 * fbe_mirror_verify_get_write_bitmap()
 ****************************************************************
 * @brief
 *   Determine what our write bitmap should be for this verify.
 *   into account here.  The write bitmap is the positions which
 *   need to be written to correct errors and keep the stripe consistent.
 *  
 * @param siots_p - Siots to calculate the write bitmap for.
 * 
 * @return fbe_u16_t - Bitmask of positions we need to write.
 *
 ****************************************************************/

fbe_u16_t fbe_mirror_verify_get_write_bitmap(fbe_raid_siots_t * const siots_p)
{
    fbe_u16_t write_bitmap;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_u16_t degraded_bitmask;
    fbe_raid_position_bitmask_t soft_media_err_bitmap;

    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    /* Just write what we were asked to.
     */
    write_bitmap = siots_p->misc_ts.cxts.eboard.w_bitmap;

    if (siots_p->algorithm == MIRROR_VR)
    {
        /* If we are a verify, then it is OK to do remaps of soft media errors.
         * Also write out the positions that had soft media errors so these positions get 
         * remapped. 
         */
        fbe_raid_fruts_get_bitmap_for_status(read_fruts_p, &soft_media_err_bitmap, NULL, /* No counts */
                                             FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                             FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_REMAP_REQUIRED);
        write_bitmap |= soft_media_err_bitmap;
    }
    /* We cannot write write to dead disks since the drive is not there.
     */
    write_bitmap &= ~degraded_bitmask;
    return write_bitmap;
}
/*!*************************************
 * end fbe_mirror_verify_get_write_bitmap()
 **************************************/
/**************************************************
 * end file fbe_mirror_verify_util.c
 **************************************************/
