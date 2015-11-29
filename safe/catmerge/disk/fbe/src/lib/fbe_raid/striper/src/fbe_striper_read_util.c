/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_striper_read_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains utility functions for the fbe_striper_rd state machine.
 *
 * @notes
 *
 * @author
 *  8/3/2009 Rob Foley Created.
 *
 ***************************************************************************/

/*************************
 **  INCLUDE FILES 
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_striper_io_private.h"
#include "fbe_raid_library.h"


fbe_status_t fbe_striper_read_get_fruts_info(fbe_raid_siots_t * siots_p,
                                             fbe_raid_fru_info_t *read_p);
fbe_status_t fbe_striper_read_calculate_num_sgs(fbe_raid_siots_t *siots_p,
                                                fbe_u16_t *num_sgs_p,
                                                fbe_raid_fru_info_t *read_p,
                                                fbe_bool_t b_generate);
fbe_status_t fbe_striper_read_setup_sgs(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_striper_read_setup_fruts(fbe_raid_siots_t * siots_p, 
                                          fbe_raid_fru_info_t *read_p,
                                          fbe_u32_t opcode,
                                          fbe_raid_memory_info_t *memory_info_p);

/*!**************************************************************************
 * fbe_striper_read_calculate_memory_size()
 ****************************************************************************
 * @brief  This function calculates the total memory usage of the siots for
 *         this state machine.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts
 * @param read_p - Array of read information.
 * 
 * @return fbe_status_t
 *
 **************************************************************************/

fbe_status_t fbe_striper_read_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                    fbe_raid_fru_info_t *read_info_p)
{
    fbe_u16_t num_sgs[FBE_RAID_MAX_SG_TYPE];
    fbe_u32_t num_fruts = 0;
    fbe_u32_t total_blocks = 0;
    fbe_status_t status;

    fbe_zero_memory(&num_sgs[0], (sizeof(fbe_u16_t) * FBE_RAID_MAX_SG_TYPE));
    /* Then calculate how many buffers are needed.
     */
    status = fbe_striper_read_get_fruts_info(siots_p, read_info_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) { return status; }

    num_fruts = siots_p->data_disks;

    if (fbe_raid_siots_is_buffered_op(siots_p))
    {
        /* Buffered transfer needs blocks allocated.
         */
        total_blocks = (fbe_u32_t)siots_p->xfer_count;
    }
        
    siots_p->total_blocks_to_allocate = total_blocks;
    fbe_raid_siots_set_optimal_page_size(siots_p,
                                         num_fruts, siots_p->total_blocks_to_allocate);

    /* Next calculate the number of sgs of each type.
     */
    status = fbe_striper_read_calculate_num_sgs(siots_p, 
                                                &num_sgs[0], 
                                                read_info_p,
                                                FBE_FALSE);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) { return status; }

    /* Take the information we calculated above and determine how this translates 
     * into a number of pages. 
     */
    status = fbe_raid_siots_calculate_num_pages(siots_p, num_fruts, &num_sgs[0],
                                                FBE_TRUE /* In-case recovery verify is required*/);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) { return status; }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_striper_read_calculate_memory_size()
 **************************************/
/*!**************************************************************
 * fbe_striper_read_setup_resources()
 ****************************************************************
 * @brief
 *  Setup the newly allocated resources.
 *
 * @param siots_p - current I/O.
 * @param read_p - Array of read information.
 *
 * @return fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_striper_read_setup_resources(fbe_raid_siots_t * siots_p, 
                                              fbe_raid_fru_info_t *read_p)
{
    fbe_raid_memory_info_t  memory_info;
    fbe_status_t            status;

    /* Initialize our memory request information
     */
    status = fbe_raid_siots_init_control_mem_info(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to init memory info with status: 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    }

    /* Set up the FRU_TS.
     */
    status = fbe_striper_read_setup_fruts(siots_p, 
                                          read_p,
                                          FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                          &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: setup read fruts failed with status 0x%x, siots_p = 0x%p\n",
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
                             "striper: failed to init vcts with status 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return(status);
    }

    /* Initialize the VR_TS.
     * We are allocating VR_TS structures WITH the fruts structures.
     */
    status = fbe_raid_siots_init_vrts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to init vrts with status 0x%x, siots_p = 0x%p\n",
                             status, siots_p);                               
        return(status);
    }

    /* Plant the allocated sgs into the locations calculated above.
     */
    status = fbe_raid_fru_info_array_allocate_sgs(siots_p, &memory_info,
                                                  read_p, NULL, NULL);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to allocate sgs with status 0x%x, siots_p = 0x%p\n",
                             status, siots_p);                               
        return (status);
    }

    /* Assign buffers to sgs.
     * We reset the current ptr to the beginning since we always 
     * allocate buffers starting at the beginning of the memory. 
     */
    status = fbe_striper_read_setup_sgs(siots_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: read setup sgs failed with status 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status);
    }

    /* Make sure the buffer memory we just used does not overlap into 
     * the area used for other resources. 
     */
    return(status);
}
/******************************************
 * end fbe_striper_read_setup_resources()
 ******************************************/
/****************************************************************
 * fbe_striper_read_get_fruts_info()
 ****************************************************************
 * @brief
 *  This function initializes the fruts structures for R0 read.
 *
 * @param siots_p - current I/O
 * @param read_p - Array of read information.
 *
 * @return fbe_status_t
 *
 * @notes
 *
 * @author
 *  12/8/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_striper_read_get_fruts_info(fbe_raid_siots_t * siots_p,
                                             fbe_raid_fru_info_t *read_p)
{
    fbe_u8_t *position = &siots_p->geo.position[0];
    fbe_lba_t logical_parity_start;
    fbe_status_t status = FBE_STATUS_OK;
    logical_parity_start = siots_p->geo.logical_parity_start;

    if (siots_p->data_disks == 1)
    {
        /* This is an Individual disk.  All we have to do is init the
         * the FRUTS found in read_fruts_ptr.
         */
        fbe_u16_t data_pos = siots_p->start_pos;

        /* Our start pos should be our only position.
         */
        if (RAID_COND(siots_p->start_pos != position[data_pos]))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                        "Striper: Invalid start position: %d, data_pos: %d, position[data_pos] %d\n",
                        siots_p->start_pos, data_pos, position[data_pos]);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Fetch a fruts.
         */
        read_p->lba = logical_parity_start + siots_p->geo.start_offset_rel_parity_stripe;
        read_p->blocks = (fbe_u32_t)(siots_p->xfer_count);
        read_p->position = position[data_pos];
    }
    else
    {
        fbe_u16_t data_pos;
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
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "striper: failed to calculate pre-read blocks: status  = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return status;
        }
        if ((siots_p->data_disks == 1) ||
            (siots_p->geo.start_index == (fbe_raid_siots_get_width(siots_p) - 1)))
        {
            /* Parity range starts at first data range.
             */
            parity_range_offset = siots_p->geo.start_offset_rel_parity_stripe;
        }
        else
        {
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
             data_pos++)
        {
            phys_blkcnt = siots_p->parity_count - r1_blkcnt[data_pos] - r2_blkcnt[data_pos];

            if (phys_blkcnt > 0)
            {
                phys_blkoff = logical_parity_start + parity_range_offset + r1_blkcnt[data_pos];
                read_p->lba = phys_blkoff;
                read_p->blocks = phys_blkcnt;
                read_p->position = position[data_pos];
                read_p++;
            }
        }
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_striper_read_get_fruts_info()
 **************************************/

/*!**************************************************************************
 * fbe_striper_read_calculate_num_sgs()
 ****************************************************************************
 * @brief  This function calculates the number of sg lists needed.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts
 * @param num_sgs_p - Array of sgs we need to allocate, one array
 *                    entry per sg type.
 * @param read_p - Read information.
 * @param b_generate - True if coming from generate.
 *                     We use this to determine which errors we trace.
 * 
 * @return fbe_status_t
 *
 **************************************************************************/

fbe_status_t fbe_striper_read_calculate_num_sgs(fbe_raid_siots_t *siots_p,
                                                fbe_u16_t *num_sgs_p,
                                                fbe_raid_fru_info_t *read_p,
                                                fbe_bool_t b_generate)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u16_t data_pos;
    fbe_u16_t pos;
    fbe_u16_t width = fbe_raid_siots_get_width(siots_p);
    fbe_sg_element_with_offset_t sg_desc;    /* track location in cache S/G list */
    fbe_u32_t num_sg_elements[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_lba_t elsz;
                              
    /* Page size must be set.
     */
    if (RAID_COND(!fbe_raid_siots_validate_page_size(siots_p)))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, "striper: page size invalid\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Initialize the num_sg_elements to NULL
     */
    for (data_pos = 0; data_pos < width; data_pos++)
    {
        num_sg_elements[data_pos] = 0;
    }

    if (width == 1)
    {
        elsz = siots_p->data_page_size;
    }
    else
    {
        elsz = fbe_raid_siots_get_blocks_per_element(siots_p);
    }

    /* A cached transfer determines SGs per position
     * by striping and counting the
     * sg elements passed in by the cache.
     */
    /* Setup sg descriptor
     */
    status = fbe_raid_siots_setup_cached_sgd(siots_p, &sg_desc);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "striper: failed to setup cache sgd: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return status;
    }

    /* Just do normal counting of sgs for the read.
     */
    status = fbe_raid_scatter_cache_to_bed(siots_p->start_lba,
                                                                      siots_p->xfer_count,
                                                                      siots_p->start_pos,
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

    /* Now take the sgs we just counted and add them to the input counts.
     */
    for (pos = 0; pos < siots_p->data_disks; pos++)
    {
        /* First determine the read sg type so we know which type of sg we
         * need to increase the count of. 
         */
        if (read_p->blocks != 0)
        {
            read_p->sg_index = fbe_raid_memory_sg_count_index(num_sg_elements[read_p->position]);

            /* In generate we might expect to get sgs that are unexpected when 
             * we are trying to determine what size to generate. 
             */
            if (RAID_COND(read_p->sg_index >= FBE_RAID_MAX_SG_TYPE))
            {
                if (!b_generate)
                {
                    /*Split trace to two lines*/
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                         "striper: read sg index 0x%x unexpected for pos %d>\n", 
                                         read_p->sg_index, read_p->position);
					fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                         "read_p=0x%p,siots_p=0x%p<\n", 
                                         read_p, siots_p);
                }
                return FBE_STATUS_INSUFFICIENT_RESOURCES;
            }
            num_sgs_p[read_p->sg_index]++;
        }
        read_p++;
    }
    return status;
}
/**************************************
 * end fbe_striper_read_calculate_num_sgs()
 **************************************/

/****************************************************************
 * fbe_striper_read_setup_sgs()
 ****************************************************************
 * @brief
 *  This function initializes the S/G lists 
 *  allocated earlier by r5_rd_alloc_sg_lists().
 *
 *  It is executed from R5 large read state machine 
 *  once memory has been allocated.
 *
 * @param siots_p - current I/O
 *
 * @return fbe_status_t
 *
 * @author
 *  7/15/99 - Created. JJ
 *
 ****************************************************************/

fbe_status_t fbe_striper_read_setup_sgs(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_u32_t data_pos;
    fbe_u32_t blkcnt;
    fbe_sg_element_with_offset_t buf_sgd;
    fbe_sg_element_t *sgl_ptrv[FBE_RAID_GROUP_MAX_DISK_ARRAY_WIDTH];
    fbe_u32_t width = fbe_raid_siots_get_width(siots_p);

    if (RAID_COND(siots_p->start_pos > width))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "striper: start position %d > width %d: siots_p = 0x%p\n",
                             siots_p->start_pos,
                             width, siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

    if (RAID_COND(read_fruts_ptr == NULL))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "striper: read_fruts_ptr == NULL: siots_p = 0x%p\n",
                             siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    while (read_fruts_ptr != NULL)
    {
        if (RAID_COND(read_fruts_ptr->blocks == 0))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "striper: fruts block count <= 0: siots_p = 0x%p\n",
                                 siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        data_pos = read_fruts_ptr->position;

        sgl_ptrv[data_pos] = fbe_raid_fruts_return_sg_ptr(read_fruts_ptr);

        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
    }

    /* 
     * Stage 1: Assign buffers for data read from the disks
     *      if this operation originates from the FED; a 
     *      S/G lists was created specifically for this.
     *
     *      If the operation originates from the write cache,
     *      a S/G list has been supplied which provides the 
     *      location of data in the cache pages.
     */

    blkcnt = (fbe_u32_t)(siots_p->xfer_count);
    
    status = fbe_raid_siots_setup_cached_sgd(siots_p, &buf_sgd);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "striper: failed to setup cache sgd: status = 0x%x, siots_p = %p\n",
                             status, siots_p);
        return status;
    }

    /* 
     * Stage 2b: Assign data to the S/G lists used for the reads.
     */

    /* Element size when the width is 1 is siots_p->page_size to avoid
     * using too many sg's. A R0 that is a width of 1 isn't really striping anyway. 
     */
    status = fbe_raid_scatter_sg_to_bed(siots_p->start_lba,
                               blkcnt,
                               siots_p->start_pos,
                               width,
                               (width == 1) ? siots_p->data_page_size :                            
                               fbe_raid_siots_get_blocks_per_element(siots_p),
                               &buf_sgd,
                               sgl_ptrv);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "striper: failed to scatter sg: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status);
    }

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    status = fbe_raid_fruts_for_each(0xFFFF, read_fruts_ptr, 
                                     fbe_raid_fruts_validate_sgs, 0, 0);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: failed to validate sgs: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return status;
    }

    return status;
}
/**************************************
 * end fbe_striper_read_setup_sgs()
 **************************************/

/*!***************************************************************
 * fbe_striper_read_setup_fruts()
 ****************************************************************
 * @brief
 *  This function will set up fruts for external verify.
 *
 * @param siots_p    - ptr to the fbe_raid_siots_t
 * @param read_p - Read info array.
 * @param opcode - opcode to set in fruts.
 * @param memory_info_p - Pointer to memory request information
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_striper_read_setup_fruts(fbe_raid_siots_t * siots_p, 
                                          fbe_raid_fru_info_t *read_p,
                                          fbe_u32_t opcode,
                                          fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t status;
    fbe_u32_t data_pos = 0;

    /* Initialize the fruts for data drives.
     */
    while (data_pos < siots_p->data_disks)
    {
        status = fbe_raid_setup_fruts_from_fru_info(siots_p,
                                                    read_p,
                                                    &siots_p->read_fruts_head,
                                                    opcode,
                                                    memory_info_p);

        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
             fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                  "striper: setup fruts failed with status %d, siots_p = 0x%p\n",
                                  status, siots_p);
            return status;
        }
        read_p++;
        data_pos++;
    }
    return FBE_STATUS_OK;
}
/* end of fbe_striper_read_setup_fruts() */

/**************************************************
 * end file fbe_striper_read_util.c
 **************************************************/


