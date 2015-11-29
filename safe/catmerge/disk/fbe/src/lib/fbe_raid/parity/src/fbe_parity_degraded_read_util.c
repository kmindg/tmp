/***************************************************************************
 * Copyright (C) EMC Corporation 1989-2002,2004-2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_parity_degraded_read_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the state functions of the RAID 5 
 *  degraded read algorithm. This algorithm is selected only 
 *  when the read falls into the degraded region of a disk. 
 *
 * TABLE OF CONTENTS
 *   fbe_parity_degraded_read_calc_degraded_range
 *   fbe_parity_degraded_read_check_host_data
 *   fbe_parity_degraded_read_chk_xsums
 *   fbe_parity_degraded_read_count_buffers
 *   fbe_parity_degraded_read_get_dead_data_positions
 *   fbe_parity_degraded_read_get_degraded_blks
 *   fbe_parity_degraded_read_reconstruct
 *   fbe_parity_degraded_read_setup_fruts
 *   fbe_parity_degraded_read_setup_sgs
 *   fbe_parity_degraded_read_transition_setup
 *   fbe_parity_get_dead_data_position
 *
 * @notes
 *
 * @author
 *  10/99  Created.  JJ.
 ***************************************************************************/

/*************************
 ** INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_parity_io_private.h"


/********************************
 *      static STRING LITERALS
 ********************************/
static fbe_status_t fbe_parity_degraded_read_calc_degraded_range(fbe_raid_siots_t * siots_p,
                                                                 fbe_u16_t dead_data_pos_1,
                                                                 fbe_u16_t dead_data_pos_2,
                                                                 fbe_lba_t *dead_blk_off_p,
                                                                 fbe_block_count_t *dead_blk_cnt_p,
                                                                 fbe_block_count_t read1_blks[],
                                                                 fbe_block_count_t read2_blks[],
                                                                 fbe_block_count_t *r1_dead_pos);
static __inline fbe_u16_t fbe_parity_get_dead_data_position(fbe_raid_siots_t * siots_p, 
                                                            fbe_u16_t dead_pos);
fbe_status_t fbe_parity_degraded_read_calculate_num_sgs(fbe_raid_siots_t *siots_p,
                                                        fbe_u16_t *num_sgs_p,
                                                        fbe_raid_fru_info_t *read_p,
                                                        fbe_raid_fru_info_t *write_p);
fbe_status_t fbe_parity_degraded_read_get_fruts_info(fbe_raid_siots_t * siots_p,
                                                     fbe_raid_fru_info_t *read_p,
                                                     fbe_raid_fru_info_t *write_p,
                                                     fbe_block_count_t *blocks_p);
fbe_status_t fbe_parity_degraded_read_validate_fruts_sgs(fbe_raid_siots_t *siots_p);

/*!**************************************************************************
 * fbe_parity_degraded_read_calculate_memory_size()
 ****************************************************************************
 * @brief  This function calculates the total memory usage of the siots for
 *         this state machine.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts
 * @param read_info_p - Pointer to read info array.
 * @param write_info_p - Pointer to write info array.
 * @param num_pages_to_allocate_p - Number of memory pages to allocate
 * 
 * @return fbe_status_t
 *
 * @author
 *  9/15/2009 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_parity_degraded_read_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                            fbe_raid_fru_info_t *read_info_p,
                                                            fbe_raid_fru_info_t *write_info_p,
                                                            fbe_u32_t *num_pages_to_allocate_p)
{
    fbe_status_t status;
    fbe_u16_t num_sgs[FBE_RAID_MAX_SG_TYPE];
    fbe_u32_t num_fruts;
    fbe_block_count_t num_blocks = 0;

    fbe_zero_memory(&num_sgs[0], (sizeof(fbe_u16_t) * FBE_RAID_MAX_SG_TYPE));

    num_fruts = fbe_raid_siots_get_width(siots_p) + siots_p->data_disks;

    /* Then calculate how many buffers are needed.
     */
    status = fbe_parity_degraded_read_get_fruts_info(siots_p, 
                                                     read_info_p, write_info_p,
                                                     &num_blocks);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: degraded read get fru info failed with status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status);
    }

    siots_p->total_blocks_to_allocate = num_blocks;

    fbe_raid_siots_set_optimal_page_size(siots_p,
                                         num_fruts, 
                                         num_blocks);

    /* Next calculate the number of sgs of each type.
     */
    status = fbe_parity_degraded_read_calculate_num_sgs(siots_p, 
                                                        &num_sgs[0], 
                                                        read_info_p, write_info_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: degraded read get calc num sgs failed with status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status);
    }

    /* Take the information we calculated above and determine how this translates 
     * into a number of pages. 
     */
    status = fbe_raid_siots_calculate_num_pages(siots_p, num_fruts, &num_sgs[0],
                                                FBE_TRUE /* In-case recovery verify is required*/);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: failed to calculate num of pages: status = 0x%x, siots_p = 0x%p, num_blocks = 0x%llx\n",
                             status, siots_p, (unsigned long long)num_blocks);
        return (status);
    }
    *num_pages_to_allocate_p = siots_p->num_ctrl_pages;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_degraded_read_calculate_memory_size()
 **************************************/

/*!**************************************************************************
 * fbe_parity_degraded_read_calculate_num_sgs()
 ****************************************************************************
 * @brief  This function calculates the number of sg lists needed.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts
 * @param num_sgs_p - array of all sg counts by type
 * @param read_p - read fru information array
 * @param write_p - write fru information array
 * 
 * @return fbe_status_t
 *
 * @author
 *  9/15/2009 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_parity_degraded_read_calculate_num_sgs(fbe_raid_siots_t *siots_p,
                                                        fbe_u16_t *num_sgs_p,
                                                        fbe_raid_fru_info_t *read_p,
                                                        fbe_raid_fru_info_t *write_p)
{   
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t num_read_sg_elements[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_u32_t num_write_sg_elements[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_u32_t data_pos;
    fbe_u16_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_u16_t data_index;
    fbe_block_count_t mem_left = 0;
    fbe_raid_fru_info_t *original_read_p;

    /* Page size must be set.
     */
    if (!fbe_raid_siots_validate_page_size(siots_p))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, "parity: page size invalid\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Initialize the num_read_sg_elements to NULL
     */
    fbe_zero_memory(num_read_sg_elements, sizeof(fbe_u32_t) * FBE_RAID_MAX_DISK_ARRAY_WIDTH);
    fbe_zero_memory(num_write_sg_elements, sizeof(fbe_u32_t) * FBE_RAID_MAX_DISK_ARRAY_WIDTH);

    /* Store the top of the read info pointer.
     */
    original_read_p = read_p;

    /* First count the number of sg's required for the reconstruct, not including the data
     * being read by the host.
     */
    for (data_index = 0;
         data_index < fbe_raid_siots_get_width(siots_p);
         data_index++)
    {
        fbe_u32_t read_data_pos = FBE_RAID_INVALID_POS;
        fbe_u32_t write_data_pos = FBE_RAID_INVALID_POS;

        if (write_p->position != FBE_RAID_INVALID_POS)
        {
            /* This is a positions that has host data (all write fruts's
             * represent what the host is reading).
             * Fetch the read and write positions from the fruts info array.
             */
            status = FBE_RAID_DATA_POSITION(read_p->position,
                                            siots_p->parity_pos,
                                            fbe_raid_siots_get_width(siots_p),
                                            parity_drives,
                                            &read_data_pos);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                            status, siots_p);
                return status;
            }

            status = FBE_RAID_DATA_POSITION(write_p->position,
                                            siots_p->parity_pos,
                                            fbe_raid_siots_get_width(siots_p),
                                            parity_drives,
                                            &write_data_pos );
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                            status, siots_p);
                return status;
            }
        }

        if ((write_p->position == FBE_RAID_INVALID_POS) || 
            (read_data_pos < write_data_pos))
        {
            /* This data is for reconstruction only. If there was data for the host
             * to read the write info would have a position or the read and write positions would match.
             * This includes both parity positions. 
             */
            if (fbe_raid_siots_is_parity_pos(siots_p, read_p->position))
            {
                /* If we are parity we will put the parity drives in the 
                 * last few positions. 
                 * If we are diagonal parity, add one to the index. 
                 */
                data_pos = fbe_raid_siots_get_width(siots_p) - parity_drives;
                data_pos += (read_p->position != siots_p->parity_pos) ? 1 : 0;
            }
            else
            {
                /* Data position, retrieve the stripe relative position.
                 */
                status = FBE_RAID_DATA_POSITION(read_p->position,
                                            siots_p->parity_pos,
                                            fbe_raid_siots_get_width(siots_p),
                                            parity_drives,
                                            &data_pos);
                if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                {
                   fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                               "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                               status, siots_p);
                   return status;
                }

            }

            /* Now count the number of sg's needed for this position for the
             * reconstruct.
             */
            mem_left = fbe_raid_sg_count_uniform_blocks(read_p->blocks,
                                                        siots_p->data_page_size,
                                                        mem_left,
                                                        &num_read_sg_elements[data_pos]);
        }
        else
        {
            /* This position has a write info entry, which indicates the host has data to read
             * for this position. But we still might need extra data for reconstruction.
             */
            fbe_lba_t rd_end, wr_end;

            /* Retrieve the stripe relative data position for the reconstruct.
             */
            status = FBE_RAID_DATA_POSITION(read_p->position,
                                            siots_p->parity_pos,
                                            fbe_raid_siots_get_width(siots_p),
                                            parity_drives,
                                            &data_pos);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                            status, siots_p);
                return status;
            }

            /* Find the end address for the portion that needs reconstructed, and the portion the
             * host is reading.
             */
            rd_end = read_p->lba + read_p->blocks;
            wr_end = write_p->lba + write_p->blocks;

            if (read_p->lba < write_p->lba)
            {
                /* Reconstruct portion is less then what the host is reading, there sg's need allocated 
                 * for the reconstruct portion, which is the difference between the two lba's.
                 */
                mem_left = fbe_raid_sg_count_uniform_blocks((fbe_u32_t)(write_p->lba - read_p->lba),
                                                            siots_p->data_page_size,
                                                            mem_left,
                                                            &num_read_sg_elements[data_pos]);
            }

            if (rd_end > wr_end)
            {
                /* The end address of what is being reconstructed is greater then the host read address.
                 * SG's need allocated for these blocks. Write sg's are required only for buffers for
                 * reconstructs on a position after the host data.
                 */
                fbe_u32_t sg_size = 0;

                mem_left = fbe_raid_sg_count_uniform_blocks((fbe_u32_t)(rd_end - wr_end),
                                                            siots_p->data_page_size,
                                                            mem_left,
                                                            &sg_size);
                num_read_sg_elements[data_pos] += sg_size;
                num_write_sg_elements[data_pos] += sg_size;

                /* Next, determine the write sg type so we know which type of sg we
                 * need to increase the count of. 
                 */
                write_p->sg_index = fbe_raid_memory_sg_count_index(num_write_sg_elements[data_pos]);

                /* Check if there is an error. 
                 */
                if (RAID_COND(write_p->sg_index >= FBE_RAID_MAX_SG_TYPE))
                {
                    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                                "parity: sg index %d unexpected for pos %d sg_count: %d 0x%p \n", 
                                                write_p->sg_index, data_pos,
                                                num_write_sg_elements[data_pos], write_p);
                    return FBE_STATUS_INSUFFICIENT_RESOURCES;
                }

                num_sgs_p[write_p->sg_index]++;
            }

            /* Advance to the next write info position.
             */
            write_p++;
        }
        /* Advance to the next read info position.
         */
        read_p++;
    }

    /* Second count the number of sg's required for doing the read from the host, not
     * including the areas being read for the reconstruct.
     */
    if (siots_p->algorithm == R5_DEG_RD)
    {
        /* Retrieve the stripe relative position for the start position of the data being
         * read.
         */
        status = FBE_RAID_DATA_POSITION(siots_p->start_pos, 
                                        siots_p->parity_pos, 
                                        fbe_raid_siots_get_width(siots_p),
                                        parity_drives,
                                        &data_pos);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                        status, siots_p);
            return status;
        }

        /* Add the sgs for the host data to the list.
         * The numbers of sgs have included the extra reads for reconstruction already.
         */
        if (fbe_raid_siots_is_buffered_op(siots_p))
        {
            /* Buffered op.
             */
            mem_left = fbe_raid_sg_scatter_fed_to_bed(siots_p->start_lba,
                                                      siots_p->xfer_count,
                                                      data_pos,
                                                      fbe_raid_siots_get_width(siots_p) - parity_drives,
                                                      fbe_raid_siots_get_blocks_per_element(siots_p),
                                                      (fbe_block_count_t)siots_p->data_page_size,
                                                      mem_left,
                                                      num_read_sg_elements);
        }
        else
        {
            /* Normal operation.
             */
            fbe_sg_element_with_offset_t sg_desc;    /* track location in cache S/G list */

            /* Setup the SG Descriptor for a cache transfer.
             */
            status = fbe_raid_siots_setup_cached_sgd(siots_p, &sg_desc);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity: failed to setup cache sgd: status = 0x%x, siots_p = 0x%p\n",
                                     status, siots_p);
                return (status);
            }

            status = fbe_raid_scatter_cache_to_bed(siots_p->start_lba,
                                                   siots_p->xfer_count,
                                                   data_pos,
                                                   fbe_raid_siots_get_width(siots_p) - parity_drives,
                                                   fbe_raid_siots_get_blocks_per_element(siots_p),
                                                   &sg_desc,
                                                   num_read_sg_elements);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity: failed to scatter cache: status = 0x%x, siots_p = 0x%p, "
                                     "data_pos = 0x%x, num_read_sg_elements = 0x%p\n",
                                     status, siots_p, data_pos, num_read_sg_elements);
                return status;
            }
        }
    }
    else
    {
        /* Not a r5_deg_rd request, get the sgs's for the read from the parent siots.
         */
        fbe_raid_siots_t *parent_siots_p = fbe_raid_siots_get_parent(siots_p);
        fbe_raid_fruts_t *parent_fruts_ptr = NULL;
        fbe_raid_siots_get_read_fruts(parent_siots_p, &parent_fruts_ptr);

        while (parent_fruts_ptr)
        {
            fbe_sg_element_t *fruts_sgl_ptr;
            fbe_u16_t bytes_per_blk;
            fbe_sg_element_with_offset_t tmp_sgd;
            fbe_u16_t tmp_size = 0;

            fbe_raid_fruts_get_sg_ptr(parent_fruts_ptr, &fruts_sgl_ptr);

            bytes_per_blk = FBE_RAID_SECTOR_SIZE(fruts_sgl_ptr->address);

            tmp_sgd.offset = 0;
            tmp_sgd.sg_element = fruts_sgl_ptr;
            fbe_parity_rvr_count_assigned_sgs(parent_fruts_ptr->blocks,
                                              &tmp_sgd,
                                              bytes_per_blk,
                                              &tmp_size);
            status = FBE_RAID_DATA_POSITION(parent_fruts_ptr->position, 
                                            siots_p->parity_pos, 
                                            fbe_raid_siots_get_width(siots_p),
                                            parity_drives,
                                            &data_pos);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                            status, siots_p);
                return status;
            }
            num_read_sg_elements[data_pos] += tmp_size;

            parent_fruts_ptr = fbe_raid_fruts_get_next(parent_fruts_ptr);
        }
    } /* end else not R5_DEG_RD */

    /* Restore the read info pointer back the top if the list.
     */
    read_p = original_read_p;

    /* Now take the sgs we just counted and add them to the input counts.
     */
    for (data_pos = 0; data_pos < fbe_raid_siots_get_width(siots_p); data_pos++)
    {
        /* All data postions for a degraded read need to be read so should have at least one 
         * sg required to do the operation.
         */
        if (num_read_sg_elements[data_pos] == 0)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "read sg elements %d is zero.\n", num_read_sg_elements[data_pos]);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Determine the read sg type so we know which type of sg we
         * need to increase the count of. 
         */
        read_p->sg_index = fbe_raid_memory_sg_count_index(num_read_sg_elements[data_pos]);
        /* Check if there is an error. 
         */
        if (RAID_COND(read_p->sg_index >= FBE_RAID_MAX_SG_TYPE))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: sg index %d unexpected for pos %d sg_count: %d 0x%p \n", 
                                        read_p->sg_index, data_pos,
                                        num_read_sg_elements[data_pos], read_p);
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }

        num_sgs_p[read_p->sg_index]++;
        read_p++;

    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_degraded_read_calculate_num_sgs()
 **************************************/

/*!***************************************************************
 * fbe_parity_degraded_read_get_fruts_info()
 ****************************************************************
 * @brief
 *  This function initializes the fruts for a degraded read.
 *
 * @param siots_p - Current sub request.
 * @param read_p - read fru information array
 * @param write_p - write fru information array
 * @param read_blocks_p - number of blocks needed to allocate for read
 *
 * @return
 *  None
 *
 * @author
 *  04/04/06 - Rewrote for R6. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_degraded_read_get_fruts_info(fbe_raid_siots_t * siots_p,
                                                     fbe_raid_fru_info_t *read_p,
                                                     fbe_raid_fru_info_t *write_p,
                                                     fbe_block_count_t *read_blocks_p)
{
    fbe_block_count_t read_blocks = 0;
    fbe_status_t status = FBE_STATUS_OK;

    /* The range of parity accessed by this I/O might have an offset
     * from the beginning of the parity stripe.  We call this 
     * the parity_range_offset or the offset from the parity range.
     */
    fbe_lba_t parity_range_offset;

    /* Offset of the degraded/dead range from the start of unit.
     */
    fbe_lba_t relative_dead_blkoff;
    fbe_lba_t dead_phys_blkoff;    /* Offset of dead/deg, includes addr off.      */
    fbe_block_count_t dead_blkcnt;    /* Block count for dead/deg range.             */
    fbe_u16_t data_index;    /* Index used for looping over data positions. */
    fbe_block_count_t dead_log_off;    /* This is the first read for the dead pos.    */
    fbe_u16_t dead_data_pos;    /* 1st dead position. */
    fbe_u16_t dead_data_pos_2;    /* 2nd dead position. */

    /* Block counts for first stripe pre-read.
     */
    fbe_block_count_t r1_blkcnt[FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1];

    /* Block counts for last stripe pre-read.
     */
    fbe_block_count_t r2_blkcnt[FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1];
    fbe_u16_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_u8_t *position = siots_p->geo.position;
    fbe_lba_t read_pba;    /* Physical block offset where read starts.  */
    fbe_lba_t logical_parity_start = siots_p->geo.logical_parity_start;

    status = fbe_raid_geometry_calc_preread_blocks(siots_p->start_lba,
                                                   &siots_p->geo,
                                                   siots_p->xfer_count,
                                                   (fbe_u32_t)fbe_raid_siots_get_blocks_per_element(siots_p),
                                                   /* Number of data drives. 
                                                    */
                                                   fbe_raid_siots_get_width(siots_p) - parity_drives, 
                                                   siots_p->parity_start,
                                                   siots_p->parity_count,
                                                   r1_blkcnt,
                                                   r2_blkcnt);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        /*Split trace to two lines*/
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: fail calc pre-read blocks: status = 0x%x>\n",
                             status);
		fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "siots=0x%p,r1_blkcnt=0x%p,r2_blkcnt=0x%p<\n",
                             siots_p, r1_blkcnt, r2_blkcnt);
        return status;
    }

    /* Determine the range we need to rebuild on the dead drive.
     */
    parity_range_offset = fbe_raid_siots_get_parity_start_offset(siots_p);

    /* Get the dead data position(s).
     */
    fbe_parity_degraded_read_get_dead_data_positions(siots_p, &dead_data_pos, &dead_data_pos_2);

    /* We must have at least one dead data position.
     */
    if (dead_data_pos == FBE_RAID_INVALID_DEAD_POS)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "dead pos %d unexpected\n", dead_data_pos);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now calculate the range of the degraded area.
     * This function uses the dead_data_position to use
     * for r1, r1 blocks.
     */
    fbe_parity_degraded_read_calc_degraded_range(siots_p, 
                                                 dead_data_pos, dead_data_pos_2,
                                                 &dead_phys_blkoff, &dead_blkcnt, 
                                                 r1_blkcnt, r2_blkcnt, &dead_log_off );

    /* Save the above calculations for degraded blocks and count.
     * dead_phys_blkoff includes the address offset of the LUN.
     */
    siots_p->degraded_start = dead_phys_blkoff;
    siots_p->degraded_count = dead_blkcnt;

    /* The read pba is the same as the parity start.
     */
    read_pba = logical_parity_start + parity_range_offset;

    /* Determine a relative dead block offset.
     */
    relative_dead_blkoff = parity_range_offset + dead_log_off;
    if (relative_dead_blkoff < (siots_p->parity_start - siots_p->geo.logical_parity_start))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "relative_dead offset %llx < parity_start %llx - parity_offset %llx\n",
                             (unsigned long long)relative_dead_blkoff,
			     (unsigned long long)siots_p->parity_start,
			     (unsigned long long)siots_p->geo.logical_parity_start);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Use the first write_fruts_ptr to hold the reconstruction data of the dead drive.
     * Make sure the start of this I/O on the dead drive is within our parity range.
     */
    if ((dead_phys_blkoff - logical_parity_start) < parity_range_offset)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "dead offset %llx - parity_start %llx < pr_offset %llx\n",
                             (unsigned long long)dead_phys_blkoff,
			     (unsigned long long)logical_parity_start,
			     (unsigned long long)parity_range_offset);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (data_index = 0;
         data_index < fbe_raid_siots_get_width(siots_p) - parity_drives;
         data_index++)
    {
        fbe_lba_t phys_blkoff;    /* Physical block offset where access starts.  */
        fbe_block_count_t phys_blkcnt;    /* Blocks accessed. */
        fbe_lba_t relative_blkoff;    /* Offset from parity for a position. */

        /* First initialize the read fruts to the entire parity range.
         * We need to read the entire parity range so we can determine the exact
         * range of errors using the fbe_xor_error_t_REGION.   We'll log these to
         * the event log in r5_vr_report_errors().
         * We also need to read the entire parity range since on Raid 6,
         * if we are singly degraded, then we need the entire parity range
         * to handle errors that are in the host data, but outside 
         * the area of reconstruction denoted by degraded_start and degraded_count.
         */
        read_p->lba = read_pba;
        read_p->blocks = siots_p->parity_count;
        read_p->position = position[data_index];
        read_blocks += read_p->blocks;

        /* Determine relative block offset/count for this fru from parity.
         */
        relative_blkoff = parity_range_offset + r1_blkcnt[data_index];

        if (relative_blkoff < (siots_p->parity_start - siots_p->geo.logical_parity_start))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "relative_offset %llx < parity_start %llx - parity_offset %llx\n",
                                 (unsigned long long)relative_blkoff,
				 (unsigned long long)siots_p->parity_start,
				 (unsigned long long)siots_p->geo.logical_parity_start);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        phys_blkcnt = siots_p->parity_count - r1_blkcnt[data_index] - r2_blkcnt[data_index];
        
        if (phys_blkcnt == 0)
        {
            /* There is no host data for this position and thus no 
             * write fruts is needed.  We are only reading this position so that
             * we can reconstruct the dead drive's data.
             */
        }
        else
        {
            /* There is at least 1 block that the host is reading on
             * this fru which will be contained in the write fruts.
             * There might be other blocks on this fru that
             * will be only read in order to reconstruct.
             *
             * The read fruts will contain the combination of host data
             * and data required to reconstruct.
             */
            phys_blkoff = logical_parity_start + relative_blkoff;

            if ((phys_blkoff - logical_parity_start) < parity_range_offset)
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "offset %llx - parity_start %llx < pr_offset %llx\n",
                                     (unsigned long long)phys_blkoff,
				     (unsigned long long)logical_parity_start,
				     (unsigned long long)parity_range_offset);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            write_p->lba = phys_blkoff;
            write_p->blocks = phys_blkcnt;
            write_p->position = position[data_index];

            /* Set the index to invalid since not all write info structures
             * will have an sg index.
             */
            write_p->sg_index = FBE_RAID_SG_INDEX_INVALID;
            write_p++;
        }
        read_p++;
    }

    /* Initialize parity drive, which will read in the data for reconstruction.
     * R6 always initializes this drive even when it is dead.
     */
    read_p->lba = read_pba;
    read_p->blocks = siots_p->parity_count;
    read_p->position = position[fbe_raid_siots_get_width(siots_p) - parity_drives];
    read_blocks += read_p->blocks;
    read_p++;

    if (parity_drives > 1)
    {
        /* For RAID 6 initialize the 2nd parity drive.
         */
        read_p->lba = read_pba;
        read_p->blocks = siots_p->parity_count;
        read_p->position = position[fbe_raid_siots_get_width(siots_p) - 1];
        read_blocks += read_p->blocks;
        read_p++;
    }

    /* return number of needed blocks to allocate for read.
     */
    *read_blocks_p = read_blocks;

    /* Add a terminator.
     */
    read_p->blocks = 0;
    read_p->lba = 0;
    read_p->position = FBE_RAID_INVALID_POS;
    write_p->blocks = 0;
    write_p->lba = 0;
    write_p->position = FBE_RAID_INVALID_POS;
    return FBE_STATUS_OK;
}
/* end of fbe_parity_degraded_read_get_fruts_info() */

/*!***************************************************************
 * fbe_parity_degraded_read_setup_fruts()
 ****************************************************************
 * @brief
 *  This function initializes the fruts for a degraded read.
 *
 * @param siots_p - Current sub request.
 * @param memory_info_p - pointer to memory request information
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  04/04/06 - Rewrote for R6. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_degraded_read_setup_fruts(fbe_raid_siots_t * siots_p,
                                                  fbe_raid_memory_info_t *memory_info_p)
{
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_status_t status;

    /* The range of parity accessed by this I/O might have an offset
     * from the beginning of the parity stripe.  We call this 
     * the parity_range_offset or the offset from the parity range.
     */
    fbe_lba_t parity_range_offset;

    /* Offset of the degraded/dead range from the start of unit.
     */
    fbe_lba_t relative_dead_blkoff;
    fbe_lba_t dead_phys_blkoff;    /* Offset of dead/deg, includes addr off.      */
    fbe_block_count_t dead_blkcnt;    /* Block count for dead/deg range.             */
    fbe_u16_t data_index;    /* Index used for looping over data positions. */
    fbe_block_count_t dead_log_off;    /* This is the first read for the dead pos.    */
    fbe_u16_t dead_data_pos;    /* 1st dead position. */
    fbe_u16_t dead_data_pos_2;    /* 2nd dead position. */

    /* Block counts for first stripe pre-read.
     */
    fbe_block_count_t r1_blkcnt[FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1];

    /* Block counts for last stripe pre-read.
     */
    fbe_block_count_t r2_blkcnt[FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1];
    fbe_u16_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_u8_t *position = siots_p->geo.position;
    fbe_lba_t read_pba;    /* Physical block offset where read starts.  */
    fbe_lba_t logical_parity_start = siots_p->geo.logical_parity_start;

    status = fbe_raid_geometry_calc_preread_blocks(siots_p->start_lba,
                                          &siots_p->geo,
                                          siots_p->xfer_count,
                                          (fbe_u32_t)fbe_raid_siots_get_blocks_per_element(siots_p),
                                          /* Number of data drives. 
                                           */
                                          fbe_raid_siots_get_width(siots_p) - parity_drives, 
                                          siots_p->parity_start,
                                          siots_p->parity_count,
                                          r1_blkcnt,
                                          r2_blkcnt);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        /*Split trace to two lines*/
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: fail calc pre-read blocks: status = 0x%x>\n",
                             status);
	fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "siots_p = 0x%p, r1_blkcnt = 0x%p, r2_blkcnt = 0x%p<\n",
                             siots_p, r1_blkcnt, r2_blkcnt);
        return status;
    }

    /* Determine the range we need to rebuild on the dead drive.
     */
    parity_range_offset = fbe_raid_siots_get_parity_start_offset(siots_p);

    /* Get the dead data position(s).
     */
    fbe_parity_degraded_read_get_dead_data_positions(siots_p, &dead_data_pos, &dead_data_pos_2);

    /* We must have at least one dead data position.
     */
    if (dead_data_pos == FBE_RAID_INVALID_DEAD_POS)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "dead pos %d unexpected\n", dead_data_pos);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now calculate the range of the degraded area.
     * This function uses the dead_data_position to use
     * for r1, r1 blocks.
     */
    status = fbe_parity_degraded_read_calc_degraded_range(siots_p, 
                                                          dead_data_pos, dead_data_pos_2,
                                                          &dead_phys_blkoff, &dead_blkcnt, 
                                                          r1_blkcnt, r2_blkcnt, &dead_log_off );
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: degraded read calc degraded range failed with status 0x%x,\n",
                             status);
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "siots_p = 0x%p, dead_data_pos1 = 0x%x, dead_data_pos2 = 0x%x\n",
                             siots_p, dead_data_pos, dead_data_pos_2);
        return (status);
    }

    /* Save the above calculations for degraded blocks and count.
     * dead_phys_blkoff includes the address offset of the LUN.
     */
    siots_p->degraded_start = dead_phys_blkoff;
    siots_p->degraded_count = dead_blkcnt;

    /* The read pba is the same as the parity start.
     */
    read_pba = logical_parity_start + parity_range_offset;

    /* Determine a relative dead block offset.
     */
    relative_dead_blkoff = parity_range_offset + dead_log_off;
    if (relative_dead_blkoff < (siots_p->parity_start - siots_p->geo.logical_parity_start))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "relative_dead offset %llx < parity_start %llx - parity_offset %llx\n",
                             (unsigned long long)relative_dead_blkoff,
			     (unsigned long long)siots_p->parity_start,
			     (unsigned long long)siots_p->geo.logical_parity_start);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Assign the fruts chains.
     * The read chain includes the entire width since we will reconstruct using all drives.
     * The write chain only includes drives that contain host data.
     */
    /* Fetch a read fruts's.
     */
    status = fbe_raid_siots_get_fruts_chain(siots_p, 
                                            &siots_p->read_fruts_head,
                                            fbe_raid_siots_get_width(siots_p),
                                            memory_info_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: get read fruts chain failed with status 0x%x, siots_p = 0x%p, "
                             "memory_info_p = 0x%p\n",
                             status, siots_p, memory_info_p);
        return (status);
    }

    /* Fetch the write fruts's.
     */
    status = fbe_raid_siots_get_fruts_chain(siots_p, 
                                            &siots_p->write_fruts_head,
                                            siots_p->data_disks,
                                            memory_info_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: get write fruts chain failed with status 0x%x, siots_p = 0x%p, "
                             "memory_info_p = 0x%p\n",
                             status, siots_p, memory_info_p);
        return (status);
    }

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

    /* Use the first write_fruts_ptr to hold the reconstruction data of the dead drive.
     * Make sure the start of this I/O on the dead drive is within our parity range.
     */
    if ((dead_phys_blkoff - logical_parity_start) < parity_range_offset)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "dead offset %llx - parity_start %llx < pr_offset %llx\n",
                             (unsigned long long)dead_phys_blkoff,
			     (unsigned long long)logical_parity_start,
			     (unsigned long long)parity_range_offset);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (data_index = 0;
         data_index < fbe_raid_siots_get_width(siots_p) - parity_drives;
         data_index++)
    {
        fbe_lba_t phys_blkoff;    /* Physical block offset where access starts.  */
        fbe_block_count_t phys_blkcnt;    /* Blocks accessed. */
        fbe_lba_t relative_blkoff;    /* Offset from parity for a position. */

        /* First initialize the read fruts to the entire parity range.
         * We need to read the entire parity range so we can determine the exact
         * range of errors using the fbe_xor_error_t_REGION.   We'll log these to
         * the event log in r5_vr_report_errors().
         * We also need to read the entire parity range since on Raid 6,
         * if we are singly degraded, then we need the entire parity range
         * to handle errors that are in the host data, but outside 
         * the area of reconstruction denoted by degraded_start and degraded_count.
         */
        status = fbe_raid_fruts_init_fruts(read_fruts_ptr,
                                           FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                           read_pba,
                                           siots_p->parity_count,
                                           position[data_index]);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity : failed to init fruts for siots_p = 0x%p, status = 0x%x, "
                                 "read_pba = 0x%llx\n",
                                 siots_p, status, (unsigned long long)read_pba);
            return (status);
        }

        /* Determine relative block offset/count for this fru from parity.
         */
        relative_blkoff = parity_range_offset + r1_blkcnt[data_index];

        if (relative_blkoff < (siots_p->parity_start - siots_p->geo.logical_parity_start))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "relative_offset %llx < parity_start %llx - parity_offset %llx\n",
                                 (unsigned long long)relative_blkoff,
				 (unsigned long long)siots_p->parity_start,
				 (unsigned long long)siots_p->geo.logical_parity_start);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        phys_blkcnt = siots_p->parity_count - r1_blkcnt[data_index] - r2_blkcnt[data_index];

        if (phys_blkcnt == 0)
        {
            /* There is no host data for this position and thus no 
             * write fruts is needed.  We are only reading this position so that
             * we can reconstruct the dead drive's data.
             */
        }
        else
        {
            /* There is at least 1 block that the host is reading on
             * this fru which will be contained in the write fruts.
             * There might be other blocks on this fru that
             * will be only read in order to reconstruct.
             *
             * The read fruts will contain the combination of host data
             * and data required to reconstruct.
             */
            phys_blkoff = logical_parity_start + relative_blkoff;

            if ((phys_blkoff - logical_parity_start) < parity_range_offset)
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "offset %llx - parity_start %llx < pr_offset %llx\n",
                                     (unsigned long long)phys_blkoff,
				     (unsigned long long)logical_parity_start,
				     (unsigned long long)parity_range_offset);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Setup the write fruts to reference the Host I/O for this position.
             */
            status = fbe_raid_fruts_init_fruts(write_fruts_ptr,
                                               FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID,
                                               phys_blkoff,
                                               phys_blkcnt,
                                               position[data_index]);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
             
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity : failed to init fruts for siots_p = 0x%p, status = 0x%x,\n",
                                     siots_p, status);
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "write_fruts_ptr = 0x%p, phys_blkoff = 0x%llx, phys_blkcnt = 0x%llx\n",
                                     write_fruts_ptr,
				     (unsigned long long)phys_blkoff,
				     (unsigned long long)phys_blkcnt);
                return status;
            }
            write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);
        }

        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
    }

    /* Initialize parity drive, which will read in the data for reconstruction.
     * R6 always initializes this drive even when it is dead.
     */
    status = fbe_raid_fruts_init_fruts(read_fruts_ptr,
                                       FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                       read_pba,
                                       siots_p->parity_count,
                                       position[fbe_raid_siots_get_width(siots_p) - parity_drives]);
    if  (RAID_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity : failed to init fruts for siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return status;
    }

    if (parity_drives > 1)
    {
        /* For RAID 6 initialize the 2nd parity drive.
         */
        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);

        status = fbe_raid_fruts_init_fruts(read_fruts_ptr,
                                           FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                           read_pba,
                                           siots_p->parity_count,
                                           position[fbe_raid_siots_get_width(siots_p) - 1]);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
           fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                "parity : failed to init fruts for siots_p = 0x%p, status = 0x%x, "
                                "read_pba = 0x%llx\n",
                                siots_p, status, (unsigned long long)read_pba);
            return status;
        }
    }
    return FBE_STATUS_OK;
}
/* end of fbe_parity_degraded_read_setup_fruts() */

/*!**************************************************************
 * fbe_parity_degraded_read_setup_resources()
 ****************************************************************
 * @brief
 *  Setup the newly allocated resources.
 *
 * @param siots_p - current I/O.               
 * @param read_info_p - Pointer to read info array.
 * @param write_info_p - Pointer to write info array.
 *
 * @return None.   
 *
 * @author
 *  9/15/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_degraded_read_setup_resources(fbe_raid_siots_t * siots_p,
                                                      fbe_raid_fru_info_t *read_info_p,
                                                      fbe_raid_fru_info_t *write_info_p)
{
    fbe_raid_fruts_t       *read_fruts_ptr = NULL;
    fbe_raid_memory_info_t  memory_info;
    fbe_raid_memory_info_t  data_memory_info;
    fbe_status_t            status;

    /* Initialize our memory request information
     */
    status = fbe_raid_siots_init_control_mem_info(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to init memory info with status 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return(status); 
    }
    status = fbe_raid_siots_init_data_mem_info(siots_p, &data_memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: failed to init data memory info: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    }

    /* Set up the FRU_TS.
     */
    status = fbe_parity_degraded_read_setup_fruts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: read setup fruts failed with status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return(status);
    }

    /* Set NOPs for fruts that are degraded.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    fbe_raid_fruts_set_degraded_nop(siots_p, read_fruts_ptr);

    /* Plant the nested siots in case it is needed for recovery verify.
     * We don't initialize it until it is required.
     */
    status = fbe_raid_siots_consume_nested_siots(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to consume nested siots: siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return (status);
    }
    
    /* Initialize the RAID_VC_TS now that it is allocated.
     */
    status = fbe_raid_siots_init_vcts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity :failed to init vcts for siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return status;
    }

    /* Initialize the VR_TS.
     * We are allocating VR_TS structures WITH the fruts structures.
     */
    status = fbe_raid_siots_init_vrts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity : failed to init vrts for siots_p = 0x%p. status = 0x%x\n",
                             siots_p, status);
        return status;
    }

    /* Plant the allocated sgs into the locations calculated above.
     */
    status = fbe_raid_fru_info_array_allocate_sgs(siots_p, &memory_info,
                                                  read_info_p, NULL, write_info_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: read failed to allocate sgs with status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status);
    }

    /* Assign buffers to sgs.
     * We reset the current ptr to the beginning since we always 
     * allocate buffers starting at the beginning of the memory. 
     */
    status = fbe_parity_degraded_read_setup_sgs(siots_p, &data_memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: failed to setup sgs: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return status;
    }

    /* Make sure the buffer memory we just used does not overlap into 
     * the area used for other resources. 
     */
    status = fbe_parity_degraded_read_validate_fruts_sgs(siots_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: failed to validate sgs: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return status;
    }

    return status;
}
/******************************************
 * end fbe_parity_degraded_read_setup_resources()
 ******************************************/

/*!***************************************************************
 * fbe_parity_degraded_read_count_buffers()
 ****************************************************************
 * @brief
 *  This function will count the amount of buffers for degraded read.
 *
 * @param siots_p    - ptr to the fbe_raid_siots_t
 *
 * @return
 *
 * @author
 *  10/25/99 - Created. JJ
 *
 ****************************************************************/
fbe_block_count_t  fbe_parity_degraded_read_count_buffers(fbe_raid_siots_t * siots_p)
{
    fbe_block_count_t blks_required = 0;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

    while (read_fruts_ptr)
    {
        blks_required += read_fruts_ptr->blocks;

        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
    }
    return blks_required;
}
/* end of fbe_parity_degraded_read_count_buffers() */

/*!***************************************************************
 * fbe_parity_degraded_read_setup_sgs()
 ****************************************************************
 * @brief
 *  This function sets up the sgs for the degraded read.
 *
 * @param siots_p    - ptr to the fbe_raid_siots_t
 * @param memory_info_p - Pointer to memory request information
 *
 * @return fbe_status_t
 *
 * @author
 *  10/25/99 - Created. JJ
 *  06/29/01 - Added vault support. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_parity_degraded_read_setup_sgs(fbe_raid_siots_t * siots_p,
                                                fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t data_pos;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_sg_element_with_offset_t tmp_sgd;

    /* sgl_ptrv contains a vector of pointers to the read_fruts sg_ids.
     * the dead position's sg_id is also included in the sgl_ptrv.
     *
     * sgl_ptrv2 contains a vector of pointers to the write_fruts' sg_ids.
     * the write_fruts' sgs only contain buffers for reconstruct
     * on a position after the host data.
     */
    fbe_sg_element_t *sgl_ptrv[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_sg_element_t *sgl_ptrv2[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_u16_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);

    /* Initialize the sgl_ptrv to NULL first. 
     */
    for (data_pos = 0; data_pos < FBE_RAID_MAX_DISK_ARRAY_WIDTH; data_pos++)
    {
        sgl_ptrv[data_pos] = sgl_ptrv2[data_pos] = NULL;
    }

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);
    
    while (write_fruts_ptr)
    {
        status = FBE_RAID_DATA_POSITION(write_fruts_ptr->position, 
                                        siots_p->parity_pos, 
                                        fbe_raid_siots_get_width(siots_p),
                                        fbe_raid_siots_parity_num_positions(siots_p),
                                        &data_pos);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
             fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "parity: failed to convert position: status = 0x%x, siots_p = 0x%p, "
                                         "write_fruts_ptr = 0x%p\n",
                                         status, siots_p, write_fruts_ptr);
             return status;
        }
        if (write_fruts_ptr->sg_id != 0)
        {
            fbe_raid_fruts_get_sg_ptr(write_fruts_ptr, &sgl_ptrv2[data_pos]);
        }

        write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);
    }

    /* Reset the write fruts pointer back to the top of the list since we
     * just went through the entire list.
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);

    /* 1. Assign a portion of the buffers that we've obtained from the BM
     * to the S/G lists for the extra read operations.  
     * The size and locations of these S/G lists are preserved for later use...
     */
    while (read_fruts_ptr)
    {
        fbe_u32_t read_data_pos = FBE_RAID_INVALID_POS;
        fbe_u32_t write_data_pos = FBE_RAID_INVALID_POS;
        if ( write_fruts_ptr != NULL) 
        {
            status = FBE_RAID_DATA_POSITION(read_fruts_ptr->position,
                                            siots_p->parity_pos,
                                            fbe_raid_siots_get_width(siots_p),
                                            fbe_raid_siots_parity_num_positions(siots_p),
                                            &read_data_pos);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "parity: failed to convert position: status = 0x%x, siots_p = 0x%p, "
                                            "read_fruts_ptr = 0x%p\n",
                                            status, siots_p, read_fruts_ptr);
                return status;
            }
            status = FBE_RAID_DATA_POSITION(write_fruts_ptr->position,
                                            siots_p->parity_pos,
                                            fbe_raid_siots_get_width(siots_p),
                                            fbe_raid_siots_parity_num_positions(siots_p),
                                            &write_data_pos);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "parity: failed to convert position: status = 0x%x, siots_p = 0x%p, "
                                            "write_fruts_ptr = 0x%p",
                                            status, siots_p, write_fruts_ptr);
                return status;
            }
        }
        if ((write_fruts_ptr == NULL) || (read_data_pos < write_data_pos))
        {
            /* There is no host data on this position, so
             * simply put buffers on this read fruts.
             */
            fbe_sg_element_t *sgl_ptr = NULL;
            fbe_u16_t sg_total = 0;
            fbe_u32_t dest_byte_count =0;

            fbe_raid_fruts_get_sg_ptr(read_fruts_ptr, &sgl_ptr);

            if(!fbe_raid_get_checked_byte_count(read_fruts_ptr->blocks, &dest_byte_count))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "%s: byte count exceeding 32bit limit \n",
                                     __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE; 
            }
            sg_total = fbe_raid_sg_populate_with_memory(memory_info_p,
                                                        sgl_ptr,
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
            /* We still might need extra data for reconstruction.
             */
            fbe_lba_t rd_end;
            fbe_lba_t wr_end;

            if (fbe_raid_siots_is_parity_pos(siots_p, read_fruts_ptr->position))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "read position %d is parity\n",
                                     read_fruts_ptr->position);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            status = FBE_RAID_DATA_POSITION(read_fruts_ptr->position,
                                            siots_p->parity_pos,
                                            fbe_raid_siots_get_width(siots_p),
                                            fbe_raid_siots_parity_num_positions(siots_p),
                                            &data_pos);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity: failed to convert position: status = 0x%x, siots_p = 0x%p, "
                                     "read_fruts_ptr = 0x%p\n",
                                     status, siots_p, read_fruts_ptr);
                return status;
            }

            /* Save the read sg ptr in the sgl_ptrv for this data position 
             */
            fbe_raid_fruts_get_sg_ptr(read_fruts_ptr, &sgl_ptrv[data_pos]);

            rd_end = read_fruts_ptr->lba + read_fruts_ptr->blocks;
            wr_end = write_fruts_ptr->lba + write_fruts_ptr->blocks;

            if (read_fruts_ptr->lba < write_fruts_ptr->lba)
            {
                fbe_u16_t sg_total = 0;
                fbe_u32_t dest_byte_count =0;

                if(!fbe_raid_get_checked_byte_count((write_fruts_ptr->lba - read_fruts_ptr->lba), 
                                                                                &dest_byte_count))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                                       "%s: byte count exceeding 32bit limit \n",
                                                       __FUNCTION__);
                    return FBE_STATUS_GENERIC_FAILURE; 
                }
                /* The read starts before the write on this position.
                 * Just copy some buffers into the sgl_ptrv[] for this read_fruts_ptr
                 */
                sg_total = fbe_raid_sg_populate_with_memory(memory_info_p,
                                                            sgl_ptrv[data_pos],
                                                            dest_byte_count);
                if (sg_total == 0)
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                            "%s: failed to populate sg for: siots = 0x%p\n",
                             __FUNCTION__,siots_p);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                /* Adjust the sg to point to the start of host data.
                 */
                while ((++sgl_ptrv[data_pos])->count != 0);
            }

            if (rd_end > wr_end)
            {
                fbe_u16_t sg_total = 0;
                fbe_u32_t dest_byte_count =0;

                if(!fbe_raid_get_checked_byte_count((rd_end - wr_end), &dest_byte_count))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                                       "%s: byte count exceeding 32bit limit \n",
                                                       __FUNCTION__);
                    return FBE_STATUS_GENERIC_FAILURE; 
                }

                /* The read ends after the write, so put some buffers into the write fruts
                 * to hold these "after host data" reconstruct buffers.
                 */
                sg_total = fbe_raid_sg_populate_with_memory(memory_info_p,
                                                            sgl_ptrv2[data_pos],
                                                            dest_byte_count);
                if (sg_total == 0)
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                            "%s: failed to populate sg for: siots = 0x%p\n",
                             __FUNCTION__,siots_p);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            }
            write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);
        }
        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
    }

    if (siots_p->algorithm == R5_DEG_RD)
    {
        fbe_u32_t data_pos;
        /* The offset is the offset of this SIOTS from the beginning of the transfer, 
         * which also take into the account the host start offset provided by the 
         * sender. The sg_element is the sg list the cache driver sent to us.
         */
        status = fbe_raid_siots_setup_cached_sgd(siots_p, &tmp_sgd);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: failed to setup cache sgs: status = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return status;
        }
        status = FBE_RAID_DATA_POSITION(siots_p->start_pos, 
                                        siots_p->parity_pos, 
                                        fbe_raid_siots_get_width(siots_p),
                                        fbe_raid_siots_parity_num_positions(siots_p),
                                        &data_pos);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
             fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                  "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                  status, siots_p);
             return status;
        }

        if (siots_p->xfer_count > FBE_U32_MAX)
        {
             fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                  "parity: blocks to scatter 0x%llx exceeding 32bit limit, siots_p = 0x%p\n",
                                    (unsigned long long)siots_p->xfer_count,
				    siots_p);
             return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Put the host data into the read_fruts' sg lists.
         */
        status = fbe_raid_scatter_sg_to_bed(siots_p->start_lba,
                                            (fbe_u32_t)siots_p->xfer_count,
                                            data_pos,
                                            fbe_raid_siots_get_width(siots_p) - parity_drives,
                                            fbe_raid_siots_get_blocks_per_element(siots_p),
                                            &tmp_sgd,
                                            sgl_ptrv);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: failed to scater memory to sg: status = 0x%x, siots_p 0x%p\n",
                                 status, siots_p);
            return status;
        }
    }
    else
    {
        fbe_u16_t sg = 0;
        /* Assign the buffers from the read.
         */
        fbe_raid_siots_t *parent_siots_p = fbe_raid_siots_get_parent(siots_p);
        fbe_raid_fruts_t *parent_fruts_ptr = NULL;
        fbe_raid_siots_get_read_fruts(parent_siots_p, &parent_fruts_ptr);
        while (parent_fruts_ptr)
        {
            fbe_sg_element_t *fruts_sgl_ptr;
            fbe_u16_t bytes_per_blk;
            
            fbe_raid_fruts_get_sg_ptr(parent_fruts_ptr, &fruts_sgl_ptr);

            bytes_per_blk = FBE_RAID_SECTOR_SIZE(fruts_sgl_ptr->address);

            fbe_sg_element_with_offset_init(&tmp_sgd,
                                            0, /* offset */
                                            fruts_sgl_ptr,
                                            NULL /* default get next ptr */);

            status = FBE_RAID_DATA_POSITION(parent_fruts_ptr->position, 
                                        siots_p->parity_pos, 
                                        fbe_raid_siots_get_width(siots_p),
                                        fbe_raid_siots_parity_num_positions(siots_p),
                                        &data_pos);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                     status, siots_p);
                return status;
            }

            if ((parent_fruts_ptr->blocks * bytes_per_blk) > FBE_U32_MAX)
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity: byte count 0x%llx exceeding 32bit limit, siots_p = 0x%p\n",
                                     (unsigned long long)(parent_fruts_ptr->blocks * bytes_per_blk), siots_p);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            status  = fbe_raid_sg_clip_sg_list(&tmp_sgd, sgl_ptrv[data_pos], 
                                               (fbe_u32_t)(parent_fruts_ptr->blocks * bytes_per_blk),
                                               &sg);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity: failed to clip sg list: status = 0x%x, siots_p = 0x%p, "
                                     "parent_fruts_ptr = 0x%p\n",
                                     status, siots_p, parent_fruts_ptr);
                return status;
            }

            while ((++sgl_ptrv[data_pos])->count != 0);

            parent_fruts_ptr = fbe_raid_fruts_get_next(parent_fruts_ptr);
        }
    }

    for (data_pos = 0; data_pos < fbe_raid_siots_get_width(siots_p) - parity_drives; data_pos++)
    {
        fbe_sg_element_t *sgl_ptr;

        /* we may have buffers *after* the host data to assign
         * for the reconstruction.
         * above we saved these sgs in the sgs of the write_fruts'.
         *
         * Copy the "after host data" buffers for reconstruction
         * on this postion into the read fruts sg list.
         */
        if ((sgl_ptr = sgl_ptrv2[data_pos]) != NULL)
        {
            (void) fbe_raid_copy_sg_list(sgl_ptr, sgl_ptrv[data_pos]);
        }
    }

    /* Debug code to prevent scatter gather overrun
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    status = fbe_raid_fruts_for_each(0xFFFF, read_fruts_ptr, 
                      fbe_raid_fruts_validate_sgs, 0, 0);
    
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to validate sgs of read_fruts_ptr = 0x%p "
                             "for siots_p = 0x%p. status = 0x%x\n",
                             read_fruts_ptr, siots_p, status);
        return status;
    }

    return FBE_STATUS_OK;
}
/* end of fbe_parity_degraded_read_setup_sgs() */

/****************************************************************
 * fbe_parity_degraded_read_reconstruct()
 ****************************************************************
 * @brief
 *   This function attempts to reconstruct the data for the
 *   array position indicated in siots_p->dead_pos.
 *   It is assumed that the caller will not return data
 *   to the host if the return value is non-NULL.
 *   
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return
 *   This function returns the uncorrectable error bitmap.
 *   The caller is expected to interpret the uncorrectable
 *   bitmap and NOT return data to the host if
 *   we see uncorrectable errors.
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_degraded_read_reconstruct(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_xor_parity_reconstruct_t reconstruct;
    fbe_u16_t array_pos;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_u16_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_lba_t logical_parity_start = siots_p->geo.logical_parity_start;

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

    /* Initialize sgl_v to point to the starting point of reconstruction data.
     */
    while (read_fruts_ptr)
    {
        fbe_u32_t data_pos;
        array_pos = read_fruts_ptr->position;

        /* We do not have an offset since we are reading and verifying the 
         * entire parity stripe.
         */
        fbe_raid_fruts_get_sg_ptr(read_fruts_ptr, &reconstruct.sg_p[array_pos]);
        reconstruct.offset[array_pos] = 0;

        /* Determine the data position of this fru and save it for possible later
         * use in RAID 6 algorithms.
         */
        status = FBE_RAID_EXTENT_POSITION(read_fruts_ptr->position,
                                          siots_p->parity_pos,
                                          fbe_raid_siots_get_width(siots_p),
                                          fbe_raid_siots_parity_num_positions(siots_p),
                                          &data_pos);
        reconstruct.data_position[array_pos] = data_pos;
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_set_unexpected_error(siots_p,
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: got error in getting extent position. status: 0x%x, "
                                                "siots_p 0x%p\n",
                                                status, siots_p);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }
        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
    } /* end while (read_fruts_ptr) */ 

    for (array_pos = 0; array_pos < fbe_raid_siots_get_width(siots_p); array_pos++)
    {
        reconstruct.bitkey[array_pos] = 1 << array_pos;
    }

    reconstruct.seed = logical_parity_start + fbe_raid_siots_get_parity_start_offset(siots_p);

    if (siots_p->parity_count > FBE_U32_MAX)
    {
        fbe_raid_siots_set_unexpected_error(siots_p,
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: parity count 0x%llx exceeding 32bit limit here, "
                                            "siots_p 0x%p\n",
                                            (unsigned long long)siots_p->parity_count, siots_p);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    reconstruct.count = (fbe_u32_t)siots_p->parity_count;
    reconstruct.data_disks = (fbe_raid_siots_get_width(siots_p) - parity_drives);
    reconstruct.option = 0;
    /* Yes if we already retried, no if not yet retried. 
     */
    reconstruct.b_final_recovery_attempt = fbe_raid_siots_is_retry(siots_p);

    status = fbe_raid_siots_parity_rebuild_pos_get(siots_p, &reconstruct.parity_pos[0], &reconstruct.rebuild_pos[0]);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
         fbe_raid_siots_set_unexpected_error(siots_p, 
                                             FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS, 
                                             "parity: %s got unexpected error for sitos_p 0x%p, status = 0x%x \n", 
                                             __FUNCTION__, siots_p, status);
         return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    reconstruct.eboard_p = &siots_p->misc_ts.cxts.eboard;
    reconstruct.eregions_p = &siots_p->vcts_ptr->error_regions;

    /* Set XOR debug options.
     */
    fbe_raid_xor_set_debug_options(siots_p, 
                                   &reconstruct.option, 
                                   FBE_FALSE /* Don't generate error trace */);
    /* Perform the reconstruct.
     */
    fbe_xor_lib_parity_reconstruct(&reconstruct);

    return FBE_RAID_STATE_STATUS_EXECUTING;
}/* end of fbe_parity_degraded_read_reconstruct() */

/*!***************************************************************
 * fbe_parity_degraded_read_transition_setup()
 ****************************************************************
 * @brief
 *  This function will set up the nested sub_iots 
 *  when we come to degraded read for a normal read.
 *
 * @param siots_p    - ptr to the fbe_raid_siots_t
 *
 * @return fbe_status_ok
 *
 * @author
 *  11/1/99 - Created. JJ
 *
 ****************************************************************/
fbe_status_t fbe_parity_degraded_read_transition_setup(fbe_raid_siots_t * siots_p)
{
    fbe_raid_siots_t *parent_siots_p = fbe_raid_siots_get_parent(siots_p);

    /* Fill in the geometry information.
     */
    siots_p->geo = parent_siots_p->geo;

    siots_p->start_lba = parent_siots_p->start_lba,
        siots_p->xfer_count = parent_siots_p->xfer_count,
        siots_p->parity_pos = parent_siots_p->parity_pos,
        siots_p->start_pos = parent_siots_p->start_pos,
        siots_p->data_disks = parent_siots_p->data_disks,
        siots_p->dead_pos = parent_siots_p->dead_pos,
        siots_p->dead_pos_2 = parent_siots_p->dead_pos_2,
        siots_p->parity_start = parent_siots_p->parity_start,
        siots_p->parity_count = parent_siots_p->parity_count;

    if(RAID_COND((parent_siots_p->algorithm != R5_RD) && (parent_siots_p->algorithm != R5_SM_RD)))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: (parent_siots_p->algorithm = 0x%x, which is not neither"
                                            "equal to R5_RD nor equal to R5_SM_RD\n",
                                            parent_siots_p->algorithm);
        return (FBE_STATUS_GENERIC_FAILURE);
    }

    siots_p->algorithm = parent_siots_p->algorithm;

    return FBE_STATUS_OK;

}
/* end of fbe_parity_degraded_read_transition_setup() */
/*!***************************************************************
 * fbe_parity_get_dead_data_position()
 ****************************************************************
 * @brief
 *  This function returns the dead data position for either
 *  FBE_RAID_FIRST_DEAD_POS or FBE_RAID_SECOND_DEAD_POS.
 *
 * @param siots_p - Current sub request.
 * @param dead_pos - either FBE_RAID_FIRST_DEAD_POS or FBE_RAID_SECOND_DEAD_POS
 *
 * @return
 *  The logical data position of this logical dead position.
 *  Returns FBE_RAID_INVALID_DEAD_POS if the input dead_pos is not degraded.
 *
 * @author
 *  04/04/06 - Created. Rob Foley
 *
 ****************************************************************/
static __inline fbe_u16_t fbe_parity_get_dead_data_position(fbe_raid_siots_t * siots_p, 
                                                     fbe_u16_t dead_pos)
{
    fbe_u32_t dead_data_pos;
    fbe_u16_t position;
    
    /* First get the physical position of this logical dead position.
     */
    position = fbe_raid_siots_dead_pos_get(siots_p, dead_pos);
    
    if (position == FBE_RAID_INVALID_DEAD_POS)
    {
        /* This dead position is not degraded,
         * just return invalid.
         */
        dead_data_pos = FBE_RAID_INVALID_DEAD_POS;
    }
    else
    {
        fbe_status_t status;
        /* Next, map this physical position into a logical data position.
         */    
        status = FBE_RAID_DATA_POSITION(position,
                                         siots_p->parity_pos,
                                         fbe_raid_siots_get_width(siots_p),
                                         fbe_raid_siots_parity_num_positions(siots_p),
                                         &dead_data_pos);
        if (RAID_COND(status != FBE_STATUS_OK))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return FBE_RAID_INVALID_DEAD_POS;
        }
    }
    return dead_data_pos;
}
/*******************************
 * end fbe_parity_get_dead_data_position()
 *******************************/
/*!***************************************************************
 * fbe_parity_degraded_read_get_dead_data_positions()
 ****************************************************************
 * @brief
 *  This function calculates and returns both dead data positions.
 *  Note that if either the first or second dead position
 *  is a parity position, then we will not return it in either of
 *  the dead_data_pos inputs since this is not what drd needs.
 *  
 * @param siots_p - Current sub request.
 * @param dead_data_pos_1 - Ptr to first dead data position.
 * @param dead_data_pos_2 - Ptr to second dead data position.
 *
 * @return
 *  In the dead_data_pos_1 and dead_data_pos_2, returns either
 *  FBE_RAID_INVALID_DEAD_POS if this position is not degraded or
 *  returns a valid data position 0..width - parity drives.
 *
 * @author
 *  04/04/06 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_parity_degraded_read_get_dead_data_positions( fbe_raid_siots_t * siots_p,
                                                               fbe_u16_t *dead_data_pos_1,
                                                               fbe_u16_t *dead_data_pos_2 )
{
    fbe_u16_t position;
    
    /* First get the physical position of the first dead position.
     */
    position = fbe_raid_siots_dead_pos_get( siots_p, FBE_RAID_FIRST_DEAD_POS );
    
    if (position != FBE_RAID_INVALID_DEAD_POS &&
        !fbe_raid_siots_is_parity_pos(siots_p, position))
    {
        /* The first dead position is valid and is not a parity drive.
         * Map the first dead position to a data position.
         */
        *dead_data_pos_1 = fbe_parity_get_dead_data_position(siots_p, FBE_RAID_FIRST_DEAD_POS);

        /* First get the physical position of the second dead position.
         */
        position = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS);

        /* If this is valid and not a parity drive, then map it to a data position.
         */
        if (position != FBE_RAID_INVALID_DEAD_POS &&
            !fbe_raid_siots_is_parity_pos(siots_p, position))
        {
            /* The second dead position is valid and is not a parity drive.
             * Map the first dead position to a data position.
             */
            *dead_data_pos_2 = fbe_parity_get_dead_data_position(siots_p, FBE_RAID_SECOND_DEAD_POS);
        }
        else
        {
            /* Second dead data position is not valid.
             */
            *dead_data_pos_2 = FBE_RAID_INVALID_DEAD_POS;
        }
    }
    else
    {
        /* There is no second dead data position.
         */
        *dead_data_pos_2 = FBE_RAID_INVALID_DEAD_POS;
        
        /* Next get the physical position of the 2nd dead position.
         */
        position = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS);

        /* If this is valid and not a parity drive, then map it to a data position.
         */
        if (position != FBE_RAID_INVALID_DEAD_POS &&
            !fbe_raid_siots_is_parity_pos(siots_p, position))
        {
            *dead_data_pos_1 = fbe_parity_get_dead_data_position(siots_p, FBE_RAID_SECOND_DEAD_POS);
        }
        else
        {
            *dead_data_pos_1 = FBE_RAID_INVALID_DEAD_POS;
        }
    }
    return FBE_STATUS_OK;
}
/* end fbe_parity_degraded_read_get_dead_data_positions */
/*!***************************************************************
 * fbe_parity_degraded_read_calc_degraded_range()
 ****************************************************************
 * @brief
 *  This function calculates the degraded range of a drd by
 *  either returning the degraded range of the single degraded
 *  drive or (For R6 when there are 2 dead drives) calculating 
 *  the range for both degraded drives.
 *  
 * @param siots_p - Current sub request.
 * @param dead_data_pos_1 - First dead data position.
 * @param dead_data_pos_2 - Second dead data position.
 * @param dead_phy_off_p - Ptr to offset of degraded range
 *                          from the start of the LUN.
 * @param dead_blk_cnt_p - Ptr to size of the degraded range.
 * @param read1_blks - Array of read 1 blks from calc mr3 prerd.
 * @param read2_blks - Array of read 2 blks from calc mr3 prerd.
 * @param dead_log_off_p - Ptr to dead position read 1 blocks.
 *
 * @return
 *  The dead_phy_off_p and dead_blk_cnt_p will be filled in 
 *  with the correct degraded range for this I/O.
 *  The read1_blks and read2_blks might also be adjusted.
 *
 * ASSUMPTIONS:
 *  We assume dead_data_pos_1 is valid.
 *  The dead_data_pos_2 is allowed to be invalid or valid.
 *  The rest of the parameters must be non-null.
 *
 * @author
 *  04/04/06 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_parity_degraded_read_calc_degraded_range(fbe_raid_siots_t * siots_p,
                                         fbe_u16_t dead_data_pos_1,
                                         fbe_u16_t dead_data_pos_2,
                                         fbe_lba_t *dead_phy_off_p,
                                         fbe_block_count_t *dead_blk_cnt_p,
                                         fbe_block_count_t read1_blks[],
                                         fbe_block_count_t read2_blks[],
                                         fbe_block_count_t *dead_log_off_p)
                                 
{
    fbe_lba_t deg_range_end_1; /* End of first dead degraded range. */
    
    fbe_lba_t parity_range_offset;
    fbe_lba_t logical_parity_start = siots_p->geo.logical_parity_start;
    parity_range_offset = fbe_raid_siots_get_parity_start_offset(siots_p);

    /* Validate input params.
     */
    if (dead_data_pos_1 == FBE_RAID_INVALID_DEAD_POS)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "dead pos 1 is %d unexpectedly\n",
                             dead_data_pos_1);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if ((dead_phy_off_p == NULL) || (dead_blk_cnt_p == NULL))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "dead_phy_off_p %p dead_blk_cnt_p %p\n",
                             dead_phy_off_p, dead_blk_cnt_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* By default use the first dead position's r1 blocks,
     * since come of the time we only have one dead position.
     */
    *dead_log_off_p = read1_blks[dead_data_pos_1];

    /* First calculate the offset, blocks and range end for the
     * first dead position.
     */
    *dead_phy_off_p = logical_parity_start + 
        parity_range_offset + read1_blks[dead_data_pos_1];
    
    *dead_blk_cnt_p = siots_p->parity_count - 
        read1_blks[dead_data_pos_1] - read2_blks[dead_data_pos_1];

    if (*dead_blk_cnt_p == 0)
    {
        /* If the first dead blocks is zero then it means we are
         * a raid 6, there is a second dead position,
         * and the read I/O does not hit the first dead position,
         * only the second dead position.
         */
        if (!fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "dead block count %llu but not raid 6\n", 
                                 (unsigned long long)(*dead_blk_cnt_p));
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (dead_data_pos_2 == FBE_RAID_INVALID_DEAD_POS)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "dead pos 2 is %d unexpectedly\n",
                                 dead_data_pos_2);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        *dead_phy_off_p = logical_parity_start + 
            parity_range_offset +  read1_blks[dead_data_pos_2];

        *dead_blk_cnt_p = siots_p->parity_count - 
            read1_blks[dead_data_pos_2] - read2_blks[dead_data_pos_2];

        /* The logical offset comes from the 2nd dead data pos too.
         */
        *dead_log_off_p = read1_blks[dead_data_pos_2];
        return FBE_STATUS_OK;
    }

    if (*dead_blk_cnt_p > siots_p->parity_count)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "dead block count %llu > parity count %llu\n", 
                             (unsigned long long)(*dead_blk_cnt_p),
			     (unsigned long long)siots_p->parity_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    deg_range_end_1 = *dead_phy_off_p + *dead_blk_cnt_p - 1;
        
    /* If there is a second degraded position, then we will take the 2nd
     * degraded range into account.
     */
    if (dead_data_pos_2 != FBE_RAID_INVALID_DEAD_POS)
    {
        fbe_lba_t dead_blk_offset_2; /* Degraded offset for 2nd dead pos. */
        fbe_block_count_t dead_blk_count_2; /* Degraded blocks for 2nd dead pos. */
        fbe_lba_t deg_range_end_2; /* End of first dead degraded range. */
        
        /* Calculate the offset, count and range end for the 2nd dead pos.
         */
        dead_blk_offset_2 = logical_parity_start + 
            parity_range_offset +  read1_blks[dead_data_pos_2];
        
        dead_blk_count_2 = siots_p->parity_count - 
            read1_blks[dead_data_pos_2] - read2_blks[dead_data_pos_2];

        if (dead_blk_count_2 == 0)
        {
            /* The read I/O doesn't touch the 2nd drive.
             * Just return since we will just use the first drive
             * for the degraded range.
             * Make sure we are returning with a sane block count.
             */
            if (*dead_blk_cnt_p == 0)
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "dead block count is %llu\n",
				     (unsigned long long)(*dead_blk_cnt_p));
                return FBE_STATUS_GENERIC_FAILURE;
            }
            return FBE_STATUS_OK;
        }
        
        deg_range_end_2 = dead_blk_offset_2 + dead_blk_count_2 - 1;
        
        /* Decide which range starts first and use that one.
         */
        if ( dead_blk_offset_2 < *dead_phy_off_p )
        {
            /* The second range starts sooner, use it and
             * adjust the read 1 blocks to account for this.
             */
            *dead_phy_off_p = dead_blk_offset_2;
            *dead_log_off_p = read1_blks[dead_data_pos_2];
        }

        /* Re-calculate the range size using the one that ends later.
         */
        if (deg_range_end_2 > deg_range_end_1)
        {
            /* Second one ends later.
             */
            *dead_blk_cnt_p = deg_range_end_2 - *dead_phy_off_p + 1;
        }
        else
        {
            /* First one ends later.
             */
            *dead_blk_cnt_p = deg_range_end_1 - *dead_phy_off_p + 1;
        }
    }
    return FBE_STATUS_OK;
}
/* end fbe_parity_degraded_read_calc_degraded_range() */

/*!**************************************************************
 * fbe_parity_degraded_read_validate_fruts_sgs()
 ****************************************************************
 * @brief
 *  Make sure the sgs associated with this parity degraded read
 *  are setup correctly.
 * 
 * @param siots_p - Current sub request.
 *
 * @return fbe_status_t
 *
 * @author
 *  2/15/2011 - Created. Kevin Schleicher
 *
 ****************************************************************/
fbe_status_t fbe_parity_degraded_read_validate_fruts_sgs(fbe_raid_siots_t *siots_p)
{
    fbe_status_t status;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);

    if (read_fruts_ptr == NULL)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: read fruts is null\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_raid_fruts_chain_validate_sg(read_fruts_ptr);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity:fail to valid sg:stat=0x%x,siots=0x%p"
                             "rd_fruts=0x%p\n",
                             status, siots_p, read_fruts_ptr);
        return status;
    }

    if (write_fruts_ptr == NULL)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: write fruts is null\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* The sg on the write fruts can not be validated since the sg can be NULL
     * and if its not NULL it will not match the fruts block size since only the portion
     * past the host read has sg's allocated for it.
     */
    
    return status;
}
/******************************************
 * end fbe_degraded_parity_read_validate_fruts_sgs()
 ******************************************/


/*****************************************
 * End of fbe_parity_degraded_read_util.c
 *****************************************/
