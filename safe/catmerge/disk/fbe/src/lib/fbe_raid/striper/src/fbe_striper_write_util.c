/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_striper_write_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains utility functions for the striper write state machine.
 *
 * @notes
 *
 * @author
 *   2009 created for logical package Rob Foley
 ***************************************************************************/

/*************************
 ** INCLUDE FILES       **
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_striper_io_private.h"

fbe_status_t fbe_striper_write_get_fruts_info(fbe_raid_siots_t * siots_p,
                                              fbe_raid_fru_info_t *read_p,
                                              fbe_raid_fru_info_t *write_p,
                                              fbe_block_count_t *read_blocks_p,
                                              fbe_u32_t *read_fruts_count_p);
fbe_status_t fbe_striper_write_calculate_num_sgs(fbe_raid_siots_t *siots_p,
                                                 fbe_u16_t *num_sgs_p,
                                                 fbe_raid_fru_info_t *read_p,
                                                 fbe_raid_fru_info_t *write_p,
                                                 fbe_bool_t b_generate);
fbe_status_t fbe_striper_write_setup_sgs(fbe_raid_siots_t * siots_p,
                                         fbe_raid_memory_info_t *memory_info_p);
fbe_status_t fbe_striper_write_setup_fruts(fbe_raid_siots_t * siots_p, 
                                            fbe_raid_fru_info_t *read_p,
                                            fbe_raid_fru_info_t *write_p,
                                            fbe_raid_memory_info_t *memory_info_p);
static fbe_status_t fbe_striper_write_generate_unaligned_fru_info_if_needed(fbe_raid_siots_t *siots_p,
                                                                            fbe_raid_fru_info_t *read_info_p,
                                                                            fbe_raid_fru_info_t *write_info_p);

/*!**************************************************************************
 * fbe_striper_write_calculate_memory_size()
 ****************************************************************************
 * @brief  This function calculates the total memory usage of the siots for
 *         this state machine.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts
 * @param read_info_p - Array of read information.
 * @param write_info_p - Array of write information.
 * 
 * @return fbe_status_t
 *
 **************************************************************************/
fbe_status_t fbe_striper_write_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                     fbe_raid_fru_info_t *read_info_p,
                                                     fbe_raid_fru_info_t *write_info_p)
{
    fbe_u16_t num_sgs[FBE_RAID_MAX_SG_TYPE];
    fbe_u32_t num_fruts = 0;
    fbe_u32_t num_read_fruts = 0;
    fbe_block_count_t read_blocks = 0;
    fbe_block_count_t total_blocks = 0;
    fbe_status_t status;

    fbe_zero_memory(&num_sgs[0], (sizeof(fbe_u16_t) * FBE_RAID_MAX_SG_TYPE));

    /* Then calculate how many buffers are needed.
     */
    if (fbe_raid_siots_get_raid_geometry(siots_p)->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        /* For R10, there is no need for pre-read at striper level. */
        status = fbe_striper_write_get_fruts_info(siots_p, NULL, write_info_p, NULL, NULL);
    }
    else
    {
        status = fbe_striper_write_get_fruts_info(siots_p, read_info_p, write_info_p, &read_blocks, &num_read_fruts);
    }

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to get fruts infor: status: 0x%x siots_p = 0x%p, "
                             "siots_p->start_lba = 0x%llx, siots_p->xfer_count = 0x%llx \n",
                             status, siots_p,
                 (unsigned long long)siots_p->start_lba,
                 (unsigned long long)siots_p->xfer_count);
        return status; 
    }

    /* We need one fruts for each write position and one for each read position.
     */
    num_fruts = siots_p->data_disks + num_read_fruts;

    if (fbe_raid_siots_is_buffered_op(siots_p))
    {
        /* Buffered transfer needs blocks allocated.
         */
        total_blocks = siots_p->xfer_count;
    }
        
    siots_p->total_blocks_to_allocate = total_blocks + read_blocks;
    fbe_raid_siots_set_optimal_page_size(siots_p,
                                         num_fruts, siots_p->total_blocks_to_allocate);

    /* Next calculate the number of sgs of each type.
     */
    status = fbe_striper_write_calculate_num_sgs(siots_p, 
                                                 &num_sgs[0], 
                                                 read_info_p,
                                                 write_info_p,
                                                 FBE_FALSE);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) 
    { 
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to calculate number of sgs: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return status; 
    }

    /* Take the information we calculated above and determine how this translates 
     * into a number of pages. 
     */
    status = fbe_raid_siots_calculate_num_pages(siots_p, num_fruts, &num_sgs[0],
                                                FBE_TRUE /* In-case recovery verify is required*/);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) 
    { 
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to calculated number of pages: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return status; 
    }

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_striper_write_calculate_memory_size()
 **************************************/
/*!**************************************************************
 * fbe_striper_write_setup_resources()
 ****************************************************************
 * @brief
 *  Setup the newly allocated resources.
 *
 * @param siots_p - current I/O.
 * @param read_p - Array of read information.
 * @param write_p - Array of write information.
 *
 * @return fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_striper_write_setup_resources(fbe_raid_siots_t * siots_p, 
                                               fbe_raid_fru_info_t *read_p,
                                               fbe_raid_fru_info_t *write_p)
{
    fbe_raid_memory_info_t  memory_info;
    fbe_raid_memory_info_t  data_memory_info;
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;

    /* Initialize our memory request information
     */
    status = fbe_raid_siots_init_control_mem_info(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to init memory info: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
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
    status = fbe_striper_write_setup_fruts(siots_p, read_p,write_p,
                                           &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper:fail to setup wt fruts:stat 0x%x,siots=0x%p\n",
                             status, siots_p);                              
        return (status);
    }

    /* Plant the nested siots in case it is needed for recovery verify.
     * We don't initialize it until it is required.
     */
    status = fbe_raid_siots_consume_nested_siots(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to consume nested siots: siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return (status);
    }
   
    /* Initialize the RAID_VC_TS now that it is allocated.
     */
    status = fbe_raid_siots_init_vcts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to init vcts: status: 0x%x, siots_p = 0x%p\n",
                             status, siots_p);                              
        return (status);
    }

    /* Initialize the VR_TS.
     * We are allocating VR_TS structures WITH the fruts structures.
     */
    status = fbe_raid_siots_init_vrts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to init vrts with status: 0x%x, siots_p = 0x%p\n",
                             status, siots_p);                               
        return (status);
    }

    /* Plant the allocated sgs into the locations calculated above.
     */
    status = fbe_raid_fru_info_array_allocate_sgs(siots_p, &memory_info,
                                                  read_p, NULL, write_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {  
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to allocate sgs with status: 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    }

    /* Assign buffers to sgs.
     * We reset the current ptr to the beginning since we always 
     * allocate buffers starting at the beginning of the memory. 
     */
    status = fbe_striper_write_setup_sgs(siots_p, &data_memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {  
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to setup sgs with status: 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    }

    /* Make sure the buffer memory we just used does not overlap into 
     * the area used for other resources. 
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_striper_write_setup_resources()
 ******************************************/
/****************************************************************
 * fbe_striper_write_get_fruts_info()
 ****************************************************************
 * @brief
 *  This function initializes the fruts structures for R0 read.
 *
 * @param siots_p - current I/O
 * @param read_info_p - Array of read information.
 * @param write_info_p - Array of write information.
 * @param read_blocks_p - Ptr to number of read blocks to allocate.
 * @param read_fruts_count_p - Ptr to number of read fruts to allocate.
 *
 * @return fbe_status_t
 *
 * @note    Assumues contents of read_blocks_p and read_fruts count are 0.
 *
 ****************************************************************/
fbe_status_t fbe_striper_write_get_fruts_info(fbe_raid_siots_t * siots_p,
                                              fbe_raid_fru_info_t *read_info_p,
                                              fbe_raid_fru_info_t *write_info_p,
                                              fbe_block_count_t *read_blocks_p,
                                              fbe_u32_t *read_fruts_count_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_u8_t               *position = &siots_p->geo.position[0];
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_lba_t               logical_parity_start = siots_p->geo.logical_parity_start;
    fbe_u32_t               width;
    fbe_u16_t               data_pos;
    fbe_lba_t               configured_capacity;

    /* Zero the write and if neccessary the pre-read fruts.
     */
    if (read_info_p != NULL) {
        fbe_zero_memory(read_info_p, sizeof(fbe_raid_fru_info_t) * siots_p->data_disks);
    }
    fbe_zero_memory(write_info_p, sizeof(fbe_raid_fru_info_t) * siots_p->data_disks);

    /* Get width, per-disk capacity and data_pos.
     */
    fbe_raid_geometry_get_width(raid_geometry_p, &width);
    fbe_raid_geometry_get_configured_capacity(raid_geometry_p, &configured_capacity);

    /* This is an Individual disk.  All we have to do is populate one position.
     */
    if (siots_p->data_disks == 1) {
        /* Fetch a fruts.
         */
        data_pos = siots_p->start_pos;
        write_info_p->lba = logical_parity_start + siots_p->geo.start_offset_rel_parity_stripe;
        write_info_p->blocks = siots_p->xfer_count;
        write_info_p->position = position[data_pos];

        /* setup the pre-read if necessary.
         */
        if ((read_info_p != NULL)                                           &&
            fbe_raid_geometry_is_position_4k_aligned(raid_geometry_p, 
                                                     write_info_p->position)    ) {
            /* Check and set the pre-read fru info as required.
             */
            status = fbe_striper_write_generate_unaligned_fru_info_if_needed(siots_p,
                                                                             read_info_p,
                                                                             write_info_p);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "striper: failed to generate unaligned fru info: status = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
                return status;
            }

            /* Update required pre-read fruts and blocks
             */
            if (read_info_p->blocks != 0) {
                if (read_blocks_p != NULL) {
                    *read_blocks_p += read_info_p->blocks;
                }
                if (read_fruts_count_p != NULL) {
                    *read_fruts_count_p += 1;
                }
            }
        }

        /* Validate the write blocks.
         */
        if (RAID_COND((write_info_p->lba + write_info_p->blocks) > 
                      (configured_capacity / width))) {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "striper: write blocks: %llx exceeds blocks limit per disks: %llx, siots_p = 0x%p\n",
                                 (unsigned long long)(write_info_p->lba + write_info_p->blocks),
                                 (unsigned long long)(configured_capacity / width),
                                 siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    } else {
        fbe_lba_t phys_blkoff;    /* block offset where access starts */
        fbe_block_count_t phys_blkcnt;    /* number of blocks accessed */
        fbe_lba_t parity_range_offset;
        fbe_block_count_t r1_blkcnt[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
        fbe_block_count_t r2_blkcnt[FBE_RAID_MAX_DISK_ARRAY_WIDTH]; /* block counts for pre-read */

        status = fbe_raid_geometry_calc_preread_blocks(siots_p->start_lba,
                                              &siots_p->geo,
                                              siots_p->xfer_count,
                                              fbe_raid_siots_get_blocks_per_element(siots_p),
                                              fbe_raid_siots_get_width(siots_p),
                                              siots_p->parity_start,
                                              siots_p->parity_count,
                                              r1_blkcnt,
                                              r2_blkcnt);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "striper: failed to calculate pre-read blocks: status = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return status;
        }

        if ((siots_p->data_disks == 1) ||
            (siots_p->geo.start_index == (fbe_raid_siots_get_width(siots_p) - 1))) {
            /* Parity range starts at first data range.
             */
            parity_range_offset = siots_p->geo.start_offset_rel_parity_stripe;
        } else {
            /* Parity range starts at previous element boundary from
             * the first data element.
             */
            parity_range_offset = 0;
        }

        /* For each fru doing i/o calculate the block offset and block count
         * for that fru.
         */
        for (data_pos = 0;
             data_pos < fbe_raid_siots_get_width(siots_p);
             data_pos++) {
            phys_blkcnt = siots_p->parity_count - r1_blkcnt[data_pos] - r2_blkcnt[data_pos];

            if (phys_blkcnt > 0) {
                /* First setup the write.
                 */
                phys_blkoff = logical_parity_start + parity_range_offset + r1_blkcnt[data_pos];
                write_info_p->lba = phys_blkoff;
                write_info_p->blocks = phys_blkcnt;
                write_info_p->position = position[data_pos];

                fbe_raid_geometry_get_configured_capacity(raid_geometry_p, &configured_capacity);
                fbe_raid_geometry_get_width(raid_geometry_p, &width);

                if (RAID_COND((write_info_p->lba + write_info_p->blocks) > 
                              (configured_capacity / width)))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "Write blocks: %llx exceeds blocks limit per disks: %llx\n",
                                        (unsigned long long)(write_info_p->lba + write_info_p->blocks),
                                        (unsigned long long)(configured_capacity / width));
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                /* setup the pre-read if necessary.
                 */
                if ((read_info_p != NULL)                                           &&
                    fbe_raid_geometry_is_position_4k_aligned(raid_geometry_p, 
                                                             write_info_p->position)    ) {
                    /* Check and set the pre-read fru info as required.
                     */
                    status = fbe_striper_write_generate_unaligned_fru_info_if_needed(siots_p,
                                                                                     read_info_p,
                                                                                     write_info_p);
                    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "striper: failed to generate unaligned fru info: status = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
                        return status;
                    }

                    /* Update required pre-read fruts and blocks
                     */
                    if (read_info_p->blocks != 0) {
                        if (read_blocks_p != NULL) {
                            *read_blocks_p += read_info_p->blocks;
                        }
                        if (read_fruts_count_p != NULL) {
                            *read_fruts_count_p += 1;
                        }

                        /* The read infos (like the read fruts) are sparsely populated.
                         */
                        read_info_p++;
                    }
                }

                /* The write infos (like the write fruts) are sparsely populated.
                 */
                write_info_p++;

            } /* end if there is data for this position */

        } /* for all possible data positions */

    } /* end else if there is more than one data disk */

    /* Return success
     */
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_striper_write_get_fruts_info()
 **************************************/

/***************************************************************************
 * fbe_striper_write_calculate_num_sgs()
 ***************************************************************************
 *
 * @brief
 *  This function is responsible for counting the S/G lists needed
 *  in the RAID 0 state machine.  
 *
 * @param siots_p - Pointer to current I/O.
 * @param num_sgs_p - Array of sgs we need to allocate, one array
 *                    entry per sg type.
 * @param read_p - Read information.
 * @param write_p - Write information.
 * @param b_generate - True if coming from generate.
 *                     We use this to determine which errors we trace.
 *
 * @return fbe_status_t
 *
 * @notes
 * 
 **************************************************************************/
fbe_status_t fbe_striper_write_calculate_num_sgs(fbe_raid_siots_t *siots_p,
                                                 fbe_u16_t *num_sgs_p,
                                                 fbe_raid_fru_info_t *read_p,
                                                 fbe_raid_fru_info_t *write_p,
                                                 fbe_bool_t b_generate)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u32_t               data_pos;
    fbe_u32_t               num_sg_elements[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_block_count_t       mem_left = 0;
    fbe_u32_t               start_pos;
    fbe_lba_t               elsz;
    fbe_u32_t               width = fbe_raid_siots_get_width(siots_p);

    if (RAID_COND(width == 0))
    {
         fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                              "striper: array width is 0, siots_p = 0x%p\n",
                              siots_p);
         return FBE_STATUS_GENERIC_FAILURE;
    }
    start_pos = siots_p->start_pos;

    /* Initialize the num_sg_elements to NULL
     */
    for (data_pos = 0; data_pos < width; data_pos++)
    {
        num_sg_elements[data_pos] = 0;
    }

    /* Element size when the width is 1 is the page size to avoid using 
     * too many sg's. A R0 that is a width of 1 isn't really striping 
     * anyway.
     */
    if (width == 1)
    {
        elsz = siots_p->data_page_size;
    }
    else
    {
        elsz = fbe_raid_siots_get_blocks_per_element(siots_p);
    }

    if (fbe_raid_siots_is_buffered_op(siots_p))
    {
        mem_left = fbe_raid_sg_scatter_fed_to_bed(siots_p->start_lba,
                                         siots_p->xfer_count,
                                         start_pos,
                                         width,
                                         elsz,
                                         (fbe_block_count_t)siots_p->data_page_size,
                                         mem_left,
                                         num_sg_elements);
    }
    else
    {
        fbe_sg_element_with_offset_t sg_desc;   /* track location in cache S/G list */

        status = fbe_raid_siots_setup_cached_sgd(siots_p, &sg_desc);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "striper: failed to setup cache sgd: status = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return status;
        }

        status = fbe_raid_scatter_cache_to_bed(siots_p->start_lba,
                                               siots_p->xfer_count,
                                               start_pos,
                                               width,
                                               elsz,
                                               &sg_desc,
                                               num_sg_elements);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "striper: failed to scatter cache: status = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return status;
        }

    }

    /* Now take the sgs we just counted and add them to the input counts.
     */
    for (data_pos = 0; data_pos < siots_p->data_disks; data_pos++)
    {
        /* First determine the write sg type so we know which type of sg we
         * need to increase the count of. 
         */
        if (write_p->blocks != 0)
        {
            /* If the write is unaligned, then add on a constant number of sgs.
             * We might need these as we setup the write SG if we have an extra pre or post area due to alignment. 
             */
            if (fbe_raid_geometry_position_needs_alignment(raid_geometry_p, write_p->position)){
                num_sg_elements[write_p->position] += FBE_RAID_EXTRA_ALIGNMENT_SGS;
            }
            write_p->sg_index = fbe_raid_memory_sg_count_index(num_sg_elements[write_p->position]);
            
            if (RAID_COND(write_p->sg_index >= FBE_RAID_MAX_SG_TYPE))
            {
                if (!b_generate)
                {
                    /*Split trace to two lines*/
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                         "striper: write sg index unexpected for write_p->position = %d>\n",
                                         write_p->position);
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                         "write_p = 0x%p, siots_p = 0x%p<\n", 
                                         write_p, siots_p);
                }
                return FBE_STATUS_INSUFFICIENT_RESOURCES;
            }
            num_sgs_p[write_p->sg_index]++;
        }
        write_p++;

        /* First determine the read sg type so we know which type of sg we
         * need to increase the count of. 
         */
        if (read_p->blocks != 0)
        {
            fbe_u32_t sg_count = 0;
            mem_left = fbe_raid_sg_count_uniform_blocks(read_p->blocks,
                                                        siots_p->data_page_size,
                                                        mem_left,
                                                        &sg_count);
            read_p->sg_index = fbe_raid_memory_sg_count_index(sg_count);
            
            if (RAID_COND(read_p->sg_index >= FBE_RAID_MAX_SG_TYPE))
            {
                if (!b_generate)
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                         "striper: read sg index %d unexpected for pos %d, "
                                         "sg_count = %d, read_p = 0x%p, siots_p = 0x%p\n", 
                                         read_p->sg_index, read_p->position, sg_count, read_p, siots_p);
                }
                return FBE_STATUS_INSUFFICIENT_RESOURCES;
            }
            num_sgs_p[read_p->sg_index]++;
        }
        read_p++;
    }
    return status;
}
/* fbe_striper_write_calculate_num_sgs() */

/*****************************************************************************
 *          fbe_striper_write_get_unaligned_write_for_position()
 ***************************************************************************** 
 * 
 * @brief   We have a pre-read for a position.  Locate the associated unalinged
 *          write fruts.
 * 
 * @param   siots_p - Pointer to siots
 * @param   read_fruts_p - Pointer to read fruts
 * @param   write_fruts_pp - Address of write fruts to upate
 *
 * @return fbe_status_t
 *****************************************************************************/
static fbe_status_t fbe_striper_write_get_unaligned_write_for_position(fbe_raid_siots_t *siots_p,
                                                                       fbe_raid_fruts_t *read_fruts_p,
                                                                       fbe_raid_fruts_t **write_fruts_pp)
{
    fbe_raid_fruts_t   *write_fruts_p = NULL;

    *write_fruts_pp = NULL;
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    while (write_fruts_p) {
        /* Validate that it is unaligned.
         */
        if (write_fruts_p->position == read_fruts_p->position) {
            if ((write_fruts_p->lba == read_fruts_p->lba)       &&
                (write_fruts_p->blocks == read_fruts_p->blocks)    ) {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "striper: get unaligned for pos: %d lba: 0x%llx blks: 0x%llx matches read.\n",
                                     read_fruts_p->position, write_fruts_p->lba, write_fruts_p->blocks);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Else update the result and return success.
             */
            *write_fruts_pp = write_fruts_p;
            return FBE_STATUS_OK;
        }

        /* Goto next.
         */
        write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);
    }

    /* Return failure.
     */
    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                         "striper: get unaligned for pos: %d lba: 0x%llx blks: 0x%llx not found.\n",
                         read_fruts_p->position, read_fruts_p->lba, read_fruts_p->blocks);
    return FBE_STATUS_GENERIC_FAILURE;
}
/********************************************************** 
 * end fbe_striper_write_get_unaligned_write_for_position()
 **********************************************************/

/*****************************************************************************
 *          fbe_striper_write_get_read_for_unaligned_write_position()
 ***************************************************************************** 
 * 
 * @brief   Check if there is a pre-read for this position
 * 
 * @param   siots_p - Pointer to siots
 * @param   write_fruts_p - Pointer to write fruts
 * @param   read_fruts_pp - Address of read fruts to update
 *
 * @return fbe_status_t
 *****************************************************************************/
static fbe_status_t fbe_striper_write_get_read_for_unaligned_write_position(fbe_raid_siots_t *siots_p,
                                                                       fbe_raid_fruts_t *write_fruts_p,
                                                                       fbe_raid_fruts_t **read_fruts_pp)
{
    fbe_raid_fruts_t   *read_fruts_p = NULL;

    *read_fruts_pp = NULL;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    while (read_fruts_p) {
        /* Validate that it is unaligned.
         */
        if (read_fruts_p->position == read_fruts_p->position) {
            if ((read_fruts_p->lba == write_fruts_p->write_preread_desc.lba)            &&
                (read_fruts_p->blocks == write_fruts_p->write_preread_desc.block_count)    ) {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "striper: get unaligned for pos: %d lba: 0x%llx blks: 0x%llx matches write.\n",
                                     read_fruts_p->position, read_fruts_p->lba, read_fruts_p->blocks);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Else update the result and return success.
             */
            *read_fruts_pp = read_fruts_p;
            return FBE_STATUS_OK;
        }

        /* Goto next.
         */
        read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);
    }

    /* Return failure.
     */
    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                         "striper: get unaligned for pos: %d lba: 0x%llx blks: 0x%llx not found.\n",
                         write_fruts_p->position, write_fruts_p->lba, write_fruts_p->blocks);
    return FBE_STATUS_GENERIC_FAILURE;
}
/**************************************************************** 
 * end fbe_striper_write_get_read_for_unaligned_write_position()
 ****************************************************************/

/***************************************************************************
 * fbe_striper_write_validate_sgs() 
 ***************************************************************************
 * @brief
 *  This function is responsible for verifying the
 *  consistency of striper sg lists.
 *
 * @param siots_p - pointer to SUB_IOTS data
 *
 * @return fbe_status_t
 *
 * @author
 *  04/28/2014  Ron Proulx  - Created.
 *
 **************************************************************************/
static fbe_status_t fbe_striper_write_validate_sgs(fbe_raid_siots_t *siots_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_block_count_t       curr_sg_blocks;
    fbe_u32_t               sgs = 0;
    fbe_raid_fruts_t       *read_fruts_p = NULL;
    fbe_raid_fruts_t       *write_fruts_p = NULL;
    fbe_sg_element_t       *sg_p = NULL;

    /* Validate the consistency of both write and optional read fruts.
     * For write fruts we will verify:
     *  1. That the sgl is correct
     *  2. If there is a read fruts that the number of write fruts blocks
     *     is the same as the read fruts.
     * For the the optional read fruts verify:
     *  1. That the sgl is correct.
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    while (write_fruts_p) {
        sgs = 0;
        fbe_raid_fruts_get_sg_ptr(write_fruts_p, &sg_p);
        status = fbe_raid_sg_count_sg_blocks(sg_p, &sgs, &curr_sg_blocks);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                        "striper: fail to count sg blocks: stat=0x%x,  siots=0x%p\n",
                                        status, siots_p);
                return status;
        }
        if (write_fruts_p->sg_id != NULL) {
            if (!fbe_raid_memory_header_is_valid((fbe_raid_memory_header_t*)write_fruts_p->sg_id)) {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "raid header invalid for fruts 0x%p, sg_id: 0x%px\n",
                                            write_fruts_p, write_fruts_p->sg_id);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        if (write_fruts_p->blocks == 0) {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "write blocks zero for fruts 0x%p\n", write_fruts_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (RAID_COND(curr_sg_blocks != write_fruts_p->blocks)) {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "striper: write sg blocks 0x%llx < fruts blocks 0x%llx siots=0x%p\n",
                                 (unsigned long long)curr_sg_blocks, (unsigned long long)write_fruts_p->blocks, siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
        /* If this position requires alignment, validate that it is aligned.
         * If the original request was not aligned validate that there is a
         * pre-read fruts.
         */
        if (fbe_raid_geometry_is_position_4k_aligned(raid_geometry_p, 
                                                     write_fruts_p->position)) {
            fbe_lba_t           write_lba = write_fruts_p->lba;
            fbe_block_count_t   write_blocks = write_fruts_p->blocks;

            /* Validate that the write is aligned.
             */
            fbe_raid_geometry_align_io(raid_geometry_p,
                                       &write_lba,
                                       &write_blocks);

            /* If the write is write is not aligned it is an error.
             */
            if ((write_lba != write_fruts_p->lba)      ||
                (write_blocks != write_fruts_p->blocks)   ) {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "striper: write pos: %d lba: 0x%llx blks: 0x%llx is not aligned.\n",
                                     write_fruts_p->position, write_fruts_p->lba, write_fruts_p->blocks);
                return FBE_STATUS_GENERIC_FAILURE;
            }
     
            /* If the original request was not aligned validate that there is
             * a pre-read fruts.
             */   
            if (write_fruts_p->write_preread_desc.block_count > 0) {
                write_lba = write_fruts_p->write_preread_desc.lba;
                write_blocks = write_fruts_p->write_preread_desc.block_count;

                /* Check if this position is unaligned.
                 */
                fbe_raid_geometry_align_io(raid_geometry_p,
                                           &write_lba,
                                           &write_blocks);

                /* If the original write request was unaligned.
                 */
                if ((write_lba != write_fruts_p->lba)      ||
                    (write_blocks != write_fruts_p->blocks)   ) {
                    /* We expect and pre-read fruts.
                     */
                    status = fbe_striper_write_get_read_for_unaligned_write_position(siots_p,
                                                                                 write_fruts_p,
                                                                                 &read_fruts_p);
                    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
                        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                                "striper: fail to get pre-read fruts: stat=0x%x,  siots=0x%p\n",
                                                status, siots_p);
                        return status;
                    }
                    fbe_raid_fruts_get_sg_ptr(read_fruts_p, &sg_p);
                    status = fbe_raid_sg_count_sg_blocks(sg_p, &sgs, &curr_sg_blocks);
                    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
                        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                        "striper: fail to count sg blocks: stat=0x%x,  siots=0x%p\n",
                                        status, siots_p);
                        return status;
                    }
                    if (RAID_COND(read_fruts_p->blocks != curr_sg_blocks)) {
                        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                            "read blocks not match sg blocks for fruts 0x%p blocks:0x%llx sg:0x%llx\n",
                                            read_fruts_p,
                                            (unsigned long long)read_fruts_p->blocks,
                                            (unsigned long long)curr_sg_blocks);
                        return FBE_STATUS_GENERIC_FAILURE;
                    }

                    /* Validate the read and write block count are the same.
                     */
                    if (RAID_COND(curr_sg_blocks != write_fruts_p->blocks)) {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "striper: read sg blocks 0x%llx < fruts blocks 0x%llx siots=0x%p\n",
                                     (unsigned long long)curr_sg_blocks, (unsigned long long)write_fruts_p->blocks, siots_p);
                        return FBE_STATUS_GENERIC_FAILURE;
                    }

                } /* end if original write was unaligned */

            } /* end if there is a pre-read descriptor */

        } /* end if this position needs to be aligned */ 

        /* Goto next.
         */
        write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);

    } /* end for all write fruts */

    return status;
}
/* end fbe_striper_write_validate_sgs() */

/****************************************************************
 * fbe_striper_write_setup_sgs()
 ****************************************************************
 * @brief
 *  This function initializes the S/G lists 
 *  allocated earlier and if neccessary initialize the pre-read
 *  fruts. (copied from r5_468_setup_sgs)
 *
 *  It is executed from R5 large read state machine 
 *  once memory has been allocated.
 *
 *  @param siots_p - Current I/O.
 *  @param memory_info_p - Pointer to memory request information
 *
 * @return fbe_status_t
 ****************************************************************/

fbe_status_t fbe_striper_write_setup_sgs(fbe_raid_siots_t * siots_p,
                                         fbe_raid_memory_info_t *memory_info_p)

{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_raid_fruts_t   *write_fruts_p = NULL;
    fbe_raid_fruts_t   *read_fruts_p = NULL;
    fbe_u32_t           data_pos;
    fbe_u32_t           index;
    fbe_u32_t           blkcnt;
    fbe_sg_element_t   *write_sgl_vp[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_sg_element_t   *preread_sgl_vp[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_u32_t           width = fbe_raid_siots_get_width(siots_p);
    fbe_sg_element_with_offset_t    write_data_sgd;

    /* Step 1:  Initalize the local resources.
     */
    for (index = 0; index < width; index++) {
        write_sgl_vp[index] = NULL;
        preread_sgl_vp[index] = NULL;
    }

    if (RAID_COND(siots_p->start_pos >= width)) {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: start_pos %d > width %d\n",
                             siots_p->start_pos, width);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    blkcnt = (fbe_u32_t)(siots_p->xfer_count);

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    while (write_fruts_p) {
        if (write_fruts_p->blocks == 0) {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "striper: write blocks is zero pos %d\n",
                                 write_fruts_p->position);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
        /* Recall that r5 expansion writes go through this
         * path and thus we need to map to a real data position.
         */
        data_pos = write_fruts_p->position;

        write_sgl_vp[data_pos] = fbe_raid_fruts_return_sg_ptr(write_fruts_p);

        write_sgl_vp[data_pos]->count = 0;
        write_sgl_vp[data_pos]->address = NULL;

        /* Goto next.
         */
        write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);
    }

    /* Assign data to the S/G lists used for the data pre-reads.
     * If there are extra pre-reads for 4k, then these will also be included in the write.
     */
    /* Phase 1: Plant any pre-reads.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    while(read_fruts_p != NULL) {
        fbe_sg_element_with_offset_t sg_descriptor;
        fbe_u32_t                   dest_byte_count =0;
        fbe_u16_t                   num_sg_elements;
        fbe_sg_element_t           *read_sgl_p = fbe_raid_fruts_return_sg_ptr(read_fruts_p);

        data_pos = read_fruts_p->position;
        if(!fbe_raid_get_checked_byte_count(read_fruts_p->blocks, &dest_byte_count)) {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "%s: byte count exceeding 32bit limit \n",
                                 __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE; 
        }

        /* Assign newly allocated memory for the whole verify region.
         */
        num_sg_elements = fbe_raid_sg_populate_with_memory(memory_info_p,
                                                           read_sgl_p, 
                                                           dest_byte_count);
        if (num_sg_elements == 0) {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "%s: failed to populate sg for: siots = 0x%p\n",
                                 __FUNCTION__,siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Get the write fruts associated with this pre-read.
         */
        status = fbe_striper_write_get_unaligned_write_for_position(siots_p, read_fruts_p, &write_fruts_p);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "%s: failed to located write fruts - status: 0x%x\n",
                                 __FUNCTION__, status);
            return status; 
        }

        /* ******** <---- Read start
         * *      *            /\
         * *      * <--- This middle area needs to be added to the write.
         * *      *            \/
         * ******** <---- Write start
         * *      *
         * *      *
         * The write lba at this point represents where the user data starts. 
         * Our read starts before the new write data                  .
         * We will need to write out these extra blocks to align to 4k. 
         * Copy these extra blocks from the read to write sg. 
         */
        fbe_sg_element_with_offset_init(&sg_descriptor, 0, read_sgl_p, NULL);
        status = fbe_raid_sg_clip_sg_list(&sg_descriptor,
                                          write_sgl_vp[data_pos],
                                          (fbe_u32_t)(write_fruts_p->lba - read_fruts_p->lba) * FBE_BE_BYTES_PER_BLOCK,
                                          &num_sg_elements);
        write_sgl_vp[data_pos] += num_sg_elements;

        /* Goto the next read fruts
        */
        read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);

    } /* while there are more read fruts to be planted */


    /* Phase 2: Setup the write sgl.
     */
    if (fbe_raid_siots_is_buffered_op(siots_p)) {
        fbe_status_t status;

        if(siots_p->xfer_count > FBE_U32_MAX) {
             fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                  "striper: siots xfer count 0x%llx > FBE_U32_MAX \n",
                                   (unsigned long long)siots_p->xfer_count);
             return FBE_STATUS_GENERIC_FAILURE; 
        }

        status = fbe_raid_scatter_sg_with_memory(siots_p->start_lba,
                                                 (fbe_u32_t)siots_p->xfer_count,
                                                 siots_p->start_pos,
                                                 width,
                                                 (width == 1) ? siots_p->data_page_size : 
                                                 fbe_raid_siots_get_blocks_per_element(siots_p),
                                                 memory_info_p,
                                                 write_sgl_vp);
         if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))  { 
             fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                  "striper: failed to scatter sg with memory: status = 0x%x, siots_p 0x%p\n",
                                  status, siots_p);
             return status; 
         }

    } else {
        /* Use the cache data to setup the write sgl for non-buffered IO.
         */
        status = fbe_raid_siots_setup_cached_sgd(siots_p, &write_data_sgd);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) 
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "striper: failed to setup cache sgd: status = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return status;
        }

        /* Element size when the width is 1 is page size to avoid using 
         * too many sg's. A R0 that is a width of 1 isn't really striping 
         * anyway.
         */
        status = fbe_raid_scatter_sg_to_bed(siots_p->start_lba,
                                            blkcnt,
                                            siots_p->start_pos,
                                            width,
                                            (width == 1) ? siots_p->data_page_size :                             
                                            fbe_raid_siots_get_blocks_per_element(siots_p),
                                            &write_data_sgd,
                                            write_sgl_vp);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))  {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "striper: failed to scatter sg to memory: siots_p = 0x%p, status = 0x%x\n",
                                 siots_p, status);
            return (status);
        }
    }  /* end else a non-buffered I/O */


    /* Phase 3: Use the pre-read sgs to complete the end of unaligned writes.
     */
    /* If there are extra pre-reads for 4k beyond the host write data,
     * then these will also be included in the write.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    while(read_fruts_p != NULL) {
        fbe_sg_element_with_offset_t sg_descriptor;
        fbe_u16_t           num_sg_elements;
        fbe_sg_element_t   *read_sgl_p = NULL;
        fbe_lba_t           read_end, write_end;
        fbe_u32_t           offset;

        /* Get the write fruts associated with this pre-read.
         */
        data_pos = read_fruts_p->position;
        status = fbe_striper_write_get_unaligned_write_for_position(siots_p, read_fruts_p, &write_fruts_p);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "%s: failed to located write fruts - status: 0x%x\n",
                                 __FUNCTION__, status);
            return status; 
        }
        read_end = read_fruts_p->lba + read_fruts_p->blocks - 1;
        write_end = write_fruts_p->lba + write_fruts_p->blocks - 1;

        /* If we need to add more read sgs to the end of the write.
         */
        if (read_end > write_end) {
            /* ******** <---- I/O start
            * *      *
            * *      *
            * ******** <---- Write end
            * *      *            /\
            * *      * <--- This middle area needs to be added to the write.
            * *      *            \/
            * ******** <---- Read end 
            *  
            * The write lba at this point represents where the user data starts 
            * Our read ends after the new write data ends. 
            * We will need to write out these extra blocks to align to 4k 
            * Copy these extra blocks from the read to write sg.
            */
            read_sgl_p = fbe_raid_fruts_return_sg_ptr(read_fruts_p);

            /* The point to copy from is the end of the write I/O. 
             * Calculate the offset into the read sg list from the beginning to where the write ends. 
             */
            offset = (fbe_u32_t)(write_end - read_fruts_p->lba + 1) * FBE_BE_BYTES_PER_BLOCK;
            fbe_sg_element_with_offset_init(&sg_descriptor, offset, read_sgl_p, NULL);
            fbe_raid_adjust_sg_desc(&sg_descriptor);
            status = fbe_raid_sg_clip_sg_list(&sg_descriptor,
                                              write_sgl_vp[data_pos],
                                              (fbe_u32_t)(read_end - write_end) * FBE_BE_BYTES_PER_BLOCK, /* Blocks to copy from read -> write */
                                              &num_sg_elements);
            write_sgl_vp[data_pos] += num_sg_elements;

        } /* end if we need to use pre-read data and end of write */

        /*! @note Now adjust the unaligned write to be aligned.
         */
        if (RAID_DBG_ENABLED) {
            write_fruts_p->write_preread_desc.lba = write_fruts_p->lba;
            write_fruts_p->write_preread_desc.block_count = write_fruts_p->blocks;
        }
        write_fruts_p->lba = read_fruts_p->lba;
        write_fruts_p->blocks = read_fruts_p->blocks;

        /* Goto the next pre-read fruts
         */
        read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);

    } /* end while there are more pre-read fruts to process */
    
    if (RAID_DBG_ENABLED) {
        /* Make sure our sgs are sane.
         */
        status = fbe_striper_write_validate_sgs(siots_p);
    }

    /* Return status
     */
    return status;
}
/* fbe_striper_write_setup_sgs() */

/*!***************************************************************
 * fbe_striper_write_setup_fruts()
 ****************************************************************
 * @brief
 *  This function will set up fruts for write.
 *
 * @param siots_p    - ptr to the fbe_raid_siots_t
 * @param read_p - Read info array.
 * @param write_p - Write info array.
 * @param memory_info_p - Pointer to memory request information
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_striper_write_setup_fruts(fbe_raid_siots_t * siots_p, 
                                           fbe_raid_fru_info_t *read_p,
                                           fbe_raid_fru_info_t *write_p,
                                           fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u16_t data_pos;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_payload_block_operation_opcode_t iots_opcode; 
    fbe_payload_block_operation_opcode_t write_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE;
    fbe_raid_fruts_t* write_fruts_ptr;

    fbe_raid_siots_get_opcode(siots_p, &iots_opcode);
    if (fbe_raid_geometry_is_raid10(raid_geometry_p))     
    {
        if (iots_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE)
        {
            /* Raid 10 BVAs will issue the verify write to the mirror.
             */
            write_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE;
        }
        if ((iots_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA))
        {
            /* Mirror needs to know it is corrupt CRC request */
            write_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA;
        }
        if ((iots_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE)){
            write_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE;
        }   
        if ((iots_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS)){
            write_opcode = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS;
        }       
    }

    /* Setup all the fruts from the fru_info
     */
    for (data_pos = 0; data_pos < siots_p->data_disks; data_pos++)
    {
        /* Initialize the fruts for writes.
         */
        status = fbe_raid_setup_fruts_from_fru_info(siots_p,
                                                    write_p,
                                                    &siots_p->write_fruts_head,
                                                    write_opcode,
                                                    memory_info_p);

        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
             fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                  "striper:fail to setup wt fruts:siots=0x%p,"
                                  "wt=0x%p,stat=0x%x\n",
                                  siots_p, write_p, status);
            return status;
        }

        /* Setup the read if we have one.
         */
        if ((read_p->blocks > 0) && (read_p->position == write_p->position)) 
        {
            status = fbe_raid_setup_fruts_from_fru_info(siots_p,
                                                    read_p,
                                                    &siots_p->read_fruts_head,
                                                    FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                                    memory_info_p);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "striper: setup read fruts failed for pos %d status %d, siots_p = 0x%p "
                                     "read_p = 0x%p\n",
                                     read_p->position, status, siots_p, read_p);
                return status;
            }

            /* The read infos (like the read fruts) are sparsely populated.
             */
            read_p++;
        }
        write_p++;
    }

    /* save the original write lba to write_preread_desc, latter xor will use that */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);
    while (write_fruts_ptr)
    {
        write_fruts_ptr->write_preread_desc.lba = write_fruts_ptr->lba;
        write_fruts_ptr->write_preread_desc.block_count = write_fruts_ptr->blocks;
        write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);
    } /* end while write_fruts_ptr*/

    return(status);
}
/* end of fbe_striper_write_setup_fruts() */

/*!***************************************************************************
 *          fbe_striper_write_generate_unaligned_fru_info_if_needed()
 *****************************************************************************
 * 
 * @brief   This function initializes the pre-read fru info for an unaligned 
 *          striper write position.
 *
 * @param   siots_p - Current sub request.
 * @param   read_info_p - Pointer to read fru info for this position
 * @param   write_info_p - Pointer to write fru info for this position
 *
 * @return  status - FBE_STATUS_OK - read fruts information successfully generated
 *                   FBE_STATUS_INSUFFICIENT_RESOURCES - Request is too large
 *                   other - Unexpected failure
 *
 * @author
 *  04/14/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_striper_write_generate_unaligned_fru_info_if_needed(fbe_raid_siots_t *siots_p,
                                                                            fbe_raid_fru_info_t *read_info_p,
                                                                            fbe_raid_fru_info_t *write_info_p)
{
    fbe_u32_t           width = fbe_raid_siots_get_width(siots_p);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_lba_t           read_lba = write_info_p->lba;
    fbe_block_count_t   read_blocks = write_info_p->blocks;

    /* If there is no read fru info then there is no need to check or generate
     * the pre-read info.
     */
    if (read_info_p == NULL) {
        return FBE_STATUS_OK;
    }

    /* Validate the write info.
     */
    if ((write_info_p == NULL)            ||
        (write_info_p->position >= width)    ) {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: write info: %p pos: %d is not valid\n",
                             write_info_p, (write_info_p) ? write_info_p->position : FBE_RAID_POSITION_INVALID);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the bitmask of those positions that need to be aligned.
     */
    if (fbe_raid_geometry_is_position_4k_aligned(raid_geometry_p, 
                                                 write_info_p->position)) {
        /* Check if this position is unaligned.
         */
        fbe_raid_geometry_align_io(raid_geometry_p,
                                   &read_lba,
                                   &read_blocks);
    }

    /* If the write needs alignment.
     */
    if ((read_lba != write_info_p->lba)      ||
        (read_blocks != write_info_p->blocks)   ) {
        /* Align pre-read request.  Set the sg index invalid
         */
        read_info_p->lba = read_lba;
        read_info_p->blocks = read_blocks;
        read_info_p->position = write_info_p->position;
        read_info_p->sg_index = FBE_RAID_SG_INDEX_INVALID;

        /*! @note Don't modify the write range until we have planted the sgs. 
         *        If we update the range before we are done we will loose the
         *        ability to determine if it is aligned or not.
         */
    }

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/***************************************************************
 * end fbe_striper_write_generate_unaligned_fru_info_if_needed()
 ***************************************************************/


/*************************
 * end file fbe_striper_write_util.c
 *************************/
