/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2007
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_parity_read_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains utility functions for the parity read State machines
 *
 * @author
 *  09/22/1999 Rob Foley Created.
 *
 ***************************************************************************/

/*************************
 **  INCLUDE FILES 
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_parity_io_private.h"

fbe_status_t fbe_parity_read_setup_sgs(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_parity_read_setup_fruts(fbe_raid_siots_t * siots_p, 
                                         fbe_raid_fru_info_t *read_p,
                                         fbe_u32_t opcode,
                                         fbe_raid_memory_info_t *memory_info_p);
fbe_status_t fbe_parity_read_validate_fruts_sgs(fbe_raid_siots_t *siots_p);

/*!**************************************************************************
 * fbe_parity_read_calculate_memory_size()
 ****************************************************************************
 * @brief  This function calculates the total memory usage of the siots for
 *         this state machine.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts
 * @param num_pages_to_allocate_p - Number of memory pages to allocate
 * 
 * @return None.
 *
 **************************************************************************/

fbe_status_t fbe_parity_read_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                   fbe_raid_fru_info_t *read_info_p,
                                                   fbe_u32_t *num_pages_to_allocate_p)
{
    fbe_u16_t num_sgs[FBE_RAID_MAX_SG_TYPE];
    fbe_u32_t num_fruts = 0;
    fbe_u32_t total_blocks = 0;
    fbe_status_t status;

    fbe_zero_memory(&num_sgs[0], (sizeof(fbe_u16_t) * FBE_RAID_MAX_SG_TYPE));
    /* Then calculate how many buffers are needed.
     */
    status = fbe_parity_read_get_fruts_info(siots_p, read_info_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: read get fru info failed with status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    }

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
    status = fbe_parity_read_calculate_num_sgs(siots_p, &num_sgs[0], read_info_p, FBE_TRUE);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: read calc num sgs failed with status = 0x%x, siots_p = 0x%p\n",
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
                             "parity: calc num pages failed with status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    }
    *num_pages_to_allocate_p = siots_p->num_ctrl_pages;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_read_calculate_memory_size()
 **************************************/

/*!**************************************************************
 * fbe_parity_read_setup_resources()
 ****************************************************************
 * @brief
 *  Setup the newly allocated resources.
 *
 * @param siots_p - current I/O.               
 *
 * @return fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_parity_read_setup_resources(fbe_raid_siots_t * siots_p, 
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
                             "parity: failed to init memory info with status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return(status); 
    }

    if ((siots_p->total_blocks_to_allocate * FBE_BE_BYTES_PER_BLOCK) > FBE_U32_MAX)
    {
        /* bytes to allocate crossing 32bit limit
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: bytes to allocate (0x%llx) crossing 32bit limit, siots_p = 0x%p\n",
                             (unsigned long long)siots_p->total_blocks_to_allocate, siots_p);
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    /* Skip over the area we will use for buffers.  This allows buffers to 
     * start aligned, which helps us to limit the number of sgs we allocate in 
     * some cases. 
     */
    status = fbe_raid_memory_apply_buffer_offset(&memory_info,
                                        (fbe_u32_t)(siots_p->total_blocks_to_allocate *
                                         FBE_BE_BYTES_PER_BLOCK));
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed tp apply offset to buffer for siots_p = 0x%p status = 0x%x\n",
                             siots_p, status);
        return  status;
    }

    /* Set up the FRU_TS.
     */
    status = fbe_parity_read_setup_fruts(siots_p, 
                                         read_p,
                                         FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                         &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: setup read fruts failed withstatus = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return(status); 
    }

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
                             "parity: failed to init vcts: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
       return  status;
    }

    /* Initialize the VR_TS.
     * We are allocating VR_TS structures WITH the fruts structures.
     */
    status = fbe_raid_siots_init_vrts(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: failed to init vrts: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
       return  status;
    }

    /* Plant the allocated sgs into the locations calculated above.
     */
    status = fbe_raid_fru_info_array_allocate_sgs(siots_p, &memory_info,
                                                  read_p, NULL, NULL);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: read failed to allocate sgs with status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status);
    }

    /* Assign buffers to sgs.
     */
    status = fbe_parity_read_setup_sgs(siots_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: read failed to setup sgs with status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return(status);
    }

    /* Make sure the buffer memory we just used does not overlap into 
     * the area used for other resources. 
     */
    return(status);
}
/******************************************
 * end fbe_parity_read_setup_resources()
 ******************************************/

/*!***************************************************************
 * fbe_parity_read_get_fruts_info()
 ****************************************************************
 * @brief
 *  This function initializes the fru info.
 *
 * @param siots_p - Current sub request.
 *
 * @return
 *  None
 *
 ****************************************************************/

fbe_status_t fbe_parity_read_get_fruts_info(fbe_raid_siots_t * siots_p,
                                              fbe_raid_fru_info_t *read_p)
{
    fbe_u16_t data_pos;
    fbe_u8_t *position = siots_p->geo.position;
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_lba_t logical_parity_start = siots_p->geo.logical_parity_start;
    fbe_lba_t phys_blkoff;    /* block offset where access starts */
    fbe_block_count_t parity_range_offset;    /* Offset from parity stripe to start of affected parity range. */
    fbe_block_count_t r1_blkcnt[FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1];
    fbe_block_count_t r2_blkcnt[FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1];    /* block counts for pre-read */
    fbe_status_t status = FBE_STATUS_OK;
    fbe_block_count_t phys_blkcnt;    /* number of blocks accessed */
    fbe_u32_t width;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_raid_geometry_get_width(raid_geometry_p, &width);
    
    /* Calculate the read areas on each fru for this multi drive I/O.
     * r1_blkcnt and r2_blkcnt contain the areas we will NOT read.
     */
    status = fbe_raid_geometry_calc_preread_blocks(siots_p->start_lba,
                                          &siots_p->geo,
                                          siots_p->xfer_count,
                                          fbe_raid_siots_get_blocks_per_element(siots_p),
                                          width - parity_drives,
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
                             "siots_p = 0x%p<\n",
                             siots_p);
       return  status;
    }

    /***************************************************
     * Determine the offset from the parity stripe to
     * the parity range.
     *
     * The calc_preread_blocks function determines
     * the pre-reads relative to the parity range
     * (ie. siots->parity_start, siots->parity count)
     *
     * The geometry information is all
     * relative to the parity stripe.
     *
     * Thus to get the start lba on a particular drive.
     * We need to calculate the offset from the 
     * parity STRIPE to the parity RANGE.
     *
     * Fru's start_lba = geo.extent[fru].start_lba +
     *                   parity range offset +
     *                   r1_blkcnt[fru]
     *
     *   -------- -------- -------- <-- Parity stripe-- 
     *  |        |        |        |                 /|\     
     *  |        |        |        |                  |      
     *  |        |        |        |    pstripe_offset|
     *  |        |        |        |                  |      
     *  |        |        |        |                  |    
     *  |        |        |        |    pstart_lba   \|/
     *  |        |        |ioioioio|<-- Parity range start of this I/O
     *  |        |        |ioioioio|       /|\       
     *  |        |        |ioioioio|        |      
     *  |        |        |ioioioio|  parity range offset        
     *  |        |        |ioioioio|        |      
     *  |        |        |ioioioio|       \|/       
     *  |        |ioioioio|ioioioio|<-- FRU I/O starts              
     *  |        |ioioioio|ioioioio|    fru_lba          
     *  |        |        |        |
     *  |________|________|________| 
     ****************************************************/
    if ((siots_p->data_disks == 1) ||
        (siots_p->geo.start_index == (width - (parity_drives + 1))))
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
        parity_range_offset = (siots_p->geo.start_offset_rel_parity_stripe / fbe_raid_siots_get_blocks_per_element(siots_p)) *
                     fbe_raid_siots_get_blocks_per_element(siots_p);
    }

    /* Initialize the fru info for data drives.
     */
    for ( data_pos = 0; 
          data_pos < (width - parity_drives);
          data_pos++)
    {
        phys_blkcnt = siots_p->parity_count - r1_blkcnt[data_pos] - r2_blkcnt[data_pos];

        if ((r1_blkcnt[data_pos] + r2_blkcnt[data_pos]) > siots_p->parity_count)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, "parity: \n");
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (phys_blkcnt > 0)
        {
            phys_blkoff = logical_parity_start + parity_range_offset + r1_blkcnt[data_pos];
            read_p->lba = phys_blkoff;
            read_p->blocks = phys_blkcnt;
            read_p->position = position[data_pos];

            if (read_p->position == siots_p->parity_pos)
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, "parity: \n");
                return FBE_STATUS_GENERIC_FAILURE;
            }

            read_p++;
        }
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_read_get_fruts_info
 **************************************/

/*!**************************************************************************
 * fbe_parity_read_calculate_num_sgs()
 ****************************************************************************
 * @brief  This function calculates the number of sg lists needed.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts.
 * @param num_sgs_p - array of all sg counts by type.
 * @param read_p - read fru information array.
 * @param b_log - true to log error for out of sgs case.
 * 
 * @return None.
 *
 * @author
 *  10/5/2009 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_parity_read_calculate_num_sgs(fbe_raid_siots_t *siots_p,
                                               fbe_u16_t *num_sgs_p,
                                               fbe_raid_fru_info_t *read_p,
                                               fbe_bool_t b_log)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t data_pos;
    fbe_u16_t pos;
    fbe_u16_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_sg_element_with_offset_t sg_desc;    /* track location in cache S/G list */
    fbe_u32_t data_disks = fbe_raid_siots_get_width(siots_p) - fbe_raid_siots_parity_num_positions(siots_p); 
    fbe_u32_t num_sg_elements[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
                              
    /* Page size must be set.
     */
    if (!fbe_raid_siots_validate_page_size(siots_p))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, "parity: page size invalid\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Initialize the num_sg_elements to NULL
     */
    fbe_zero_memory(num_sg_elements, sizeof(fbe_u32_t) * FBE_RAID_MAX_DISK_ARRAY_WIDTH);
    
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
        return  status;
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
                             "parity: failed to setup cache sg: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return  status;
    }

    /* Just do normal counting of sgs for the read.
     */
    status = fbe_raid_scatter_cache_to_bed(siots_p->start_lba,
                                           siots_p->xfer_count,
                                           data_pos,
                                           data_disks,
                                           fbe_raid_siots_get_blocks_per_element(siots_p),
                                           &sg_desc,
                                           num_sg_elements);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: failed to scatter cache: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return  status;
    }

    /* Now take the sgs we just counted and add them to the input counts.
     */
    for (pos = 0; pos < siots_p->data_disks; pos++)
    {
        status = FBE_RAID_DATA_POSITION(read_p->position,
                                        siots_p->parity_pos,
                                        fbe_raid_siots_get_width(siots_p),
                                        fbe_raid_siots_parity_num_positions(siots_p),
                                        &data_pos);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                        status, siots_p);
            return  status;
        }

        /* First determine the read sg type so we know which type of sg we
         * need to increase the count of. 
         */
        if (RAID_COND(read_p->blocks == 0))
        {
            if (b_log)
            {
                 fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                      "parity: read blocks is 0x%llx lba: %llx pos: %d\n",
                                      (unsigned long long)read_p->blocks,
				      (unsigned long long)read_p->lba,
				      read_p->position);
            }
            return FBE_STATUS_GENERIC_FAILURE;
        }

        read_p->sg_index = fbe_raid_memory_sg_count_index(num_sg_elements[data_pos]);
        /* Check if there is an error. 
         */
        if (RAID_COND(read_p->sg_index >= FBE_RAID_MAX_SG_TYPE))
        {
            fbe_raid_trace_error(FBE_RAID_TRACE_ERROR_PARAMS,
                                 "parity: sg index %d unexpected for pos %d sg_count: %d 0x%p \n", 
                                 read_p->sg_index, data_pos,
                                 num_sg_elements[data_pos], read_p);
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
        
        num_sgs_p[read_p->sg_index]++;
        read_p++;
    }
    return status;
}
/**************************************
 * end fbe_parity_read_calculate_num_sgs()
 **************************************/

/*!***************************************************************
 * fbe_parity_read_setup_fruts()
 ****************************************************************
 * @brief
 *  This function will set up fruts for a parity read
 *
 * @param siots_p    - ptr to the fbe_raid_siots_t
 * @param read_p    - Pointer to read fru information
 * @param opcode     - type of op to put in fruts.
 * @param memory_info_p - Pointer to memory request information
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_parity_read_setup_fruts(fbe_raid_siots_t * siots_p, 
                                         fbe_raid_fru_info_t *read_p,
                                         fbe_u32_t opcode,
                                         fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_u16_t data_pos = 0;

    /* Initialize the fruts for data drives.
     */
    while (data_pos < siots_p->data_disks)
    {
        read_fruts_ptr = fbe_raid_siots_get_next_fruts(siots_p,
                                                       &siots_p->read_fruts_head,
                                                       memory_info_p);
        status = fbe_raid_fruts_init_fruts(read_fruts_ptr,
                                           opcode,
                                           read_p->lba,
                                           read_p->blocks,
                                           read_p->position);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity : failed to init fruts for siots_p = 0x%p, status = 0x%x\n",
                                 siots_p, status);
            return  status;
        }

        if (read_p->blocks == 0)
        {
             fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                  "parity: verify read blocks is 0x%llx lba: %llx pos: %d\n",
                                  (unsigned long long)read_p->blocks,
				  (unsigned long long)read_p->lba,
				  read_p->position);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        read_p++;
        data_pos++;
    }
    return FBE_STATUS_OK;
}
/* end of fbe_parity_read_setup_fruts() */

/*!**************************************************************
 * fbe_parity_read_validate_fruts_sgs()
 ****************************************************************
 * @brief
 *  Make sure the sgs associated with this parity read
 *  are setup correctly.
 * 
 * @param siots_p - Current sub request.
 *
 * @return fbe_status_t
 *
 * @author
 *  9/4/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_read_validate_fruts_sgs(fbe_raid_siots_t *siots_p)
{
    fbe_status_t status;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

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
    return status;
}
/******************************************
 * end fbe_parity_read_validate_fruts_sgs()
 ******************************************/

/****************************************************************
 * fbe_parity_read_setup_sgs()
 ****************************************************************
 * @brief
 *  This function initializes the S/G lists
 *  allocated earlier by r5_rd_alloc_sg_lists().
 *
 *  It is executed from R5 large read state machine 
 *  once memory has been allocated.
 *
 *  @param siots_p - current I/O.
 *
 * @return fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_parity_read_setup_sgs(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_u32_t data_pos;
    fbe_u32_t blkcnt;
    fbe_sg_element_with_offset_t buf_sgd;
    fbe_sg_element_t *sgl_ptrv[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_u32_t data_disks = fbe_raid_siots_get_width(siots_p) - fbe_raid_siots_parity_num_positions(siots_p); 

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    if (read_fruts_ptr == NULL)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: read fruts is null\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* First get the ptrs to the sg list for each fruts.
     */
    while (read_fruts_ptr)
    {
        if (read_fruts_ptr->blocks == 0)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: verify read blocks is 0x%llx lba: %llx pos: %d\n",
                                  (unsigned long long)read_fruts_ptr->blocks,
				  (unsigned long long)read_fruts_ptr->lba,
				  read_fruts_ptr->position);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (read_fruts_ptr->position == siots_p->parity_pos)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: verify read position is parity pos: %d\n",
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
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                        status, siots_p);
            return  status;
        }

        sgl_ptrv[data_pos] = fbe_raid_fruts_return_sg_ptr(read_fruts_ptr);

        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
    }

    /* 
     * Stage 1: The operation originates from the write cache,
     *          a S/G list has been supplied which provides the 
     *          location of data in the cache pages.
     */
    blkcnt = (fbe_u32_t)(siots_p->xfer_count);

    /* Setup sg descriptor
     */
    status = fbe_raid_siots_setup_cached_sgd(siots_p, &buf_sgd);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: failed to setup cache: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return  status;
    }

    /* 
     * Stage 2: Assign data to the S/G lists used for the reads.
     */
    status = FBE_RAID_DATA_POSITION(siots_p->start_pos,
                                    siots_p->parity_pos,
                                    fbe_raid_siots_get_width(siots_p),
                                    fbe_raid_siots_parity_num_positions(siots_p),
                                    &data_pos);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                     status, siots_p);
        return  status;
    }

    status = fbe_raid_scatter_sg_to_bed(siots_p->start_lba,
                                        blkcnt,
                                        data_pos,
                                        data_disks,
                                        fbe_raid_siots_get_blocks_per_element(siots_p),
                                        &buf_sgd,
                                        sgl_ptrv);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity : failed to scatter memory to sg, siots_p = 0x%p, "
                             "status = 0x%x, sgl_ptrv = 0x%p, data_pos = 0x%x, blkcnt = 0x%x\n",
                             siots_p, status, sgl_ptrv, data_pos, blkcnt);
        return (status);
    }

    status = fbe_parity_read_validate_fruts_sgs(siots_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: failed to validate sgs: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return status;
    }

    return status;
}
/**************************************
 * end fbe_parity_read_setup_sgs()
 **************************************/

/***************************************************************************
 * fbe_parity_read_fruts_chain_touches_dead_pos()
 ***************************************************************************
 * @brief
 *  Determine if a fruts chain contains a dead position.
 *
 * @param siots_p - current sub request
 * @param fruts_p - fruts chain to check.
 *
 * @return
 *  fbe_bool_t - FBE_TRUE if chain contains dead pos, FBE_FALSE otherwise.
 *
 * @author
 *  08/09/05 - Created. Rob Foley
 *
 **************************************************************************/

fbe_bool_t fbe_parity_read_fruts_chain_touches_dead_pos( fbe_raid_siots_t * const siots_p, 
                                                         fbe_raid_fruts_t *const fruts_p )
{
    /* Try degraded read, if possible.
     */
    fbe_bool_t try_deg = FBE_FALSE;

    /* We can only try degraded read, 
     * when the dead fru has the data that we want to read,
     * otherwise, return error status.
     */
    if ((siots_p->dead_pos != (fbe_u16_t) - 1) && (siots_p->dead_pos != siots_p->parity_pos))
    {
        fbe_raid_fruts_t * current_fruts_ptr = fruts_p;
        
        while (current_fruts_ptr)
        {
            if (current_fruts_ptr->position == siots_p->dead_pos)
            {
                try_deg = FBE_TRUE;
                break;
            }

            current_fruts_ptr = fbe_raid_fruts_get_next(current_fruts_ptr);
        }
    }
    return try_deg;
}
/* end fbe_parity_read_fruts_chain_touches_dead_pos() */

/***************************************************
 * end fbe_parity_read_util.c 
 ***************************************************/
