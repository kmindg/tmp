/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * fbe_parity_rcw_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions for the parity object's rcw algorithm.
 *
 * @author
 *  10/07/2013:  Deanna Heng, Rob Foley -- Created.
 *
 ***************************************************************************/

/*************************
 ** INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_parity_io_private.h"


fbe_status_t fbe_parity_rcw_setup_sgs(fbe_raid_siots_t * siots_p,
                                      fbe_raid_memory_info_t *memory_info_p,
                                      fbe_raid_fru_info_t *read_p,
                                        fbe_raid_fru_info_t *read2_p,
                                        fbe_raid_fru_info_t *write_p);
fbe_status_t fbe_parity_rcw_setup_fruts(fbe_raid_siots_t * siots_p, 
                                        fbe_raid_fru_info_t *read_p,
                                        fbe_raid_fru_info_t *read2_p,
                                        fbe_raid_fru_info_t *write_p,
                                        fbe_raid_memory_info_t *memory_info_p);
fbe_status_t fbe_parity_rcw_validate_fruts_sgs(fbe_raid_siots_t *siots_p);
static fbe_status_t fbe_parity_rcw_validate_sgs(fbe_raid_siots_t * siots_p);
fbe_block_count_t fbe_parity_rcw_count_buffers(fbe_raid_siots_t *siots_p);
void fbe_raid_siots_setup_rcw_preread_fru_info(fbe_raid_siots_t *siots_p,
                                               fbe_raid_fru_info_t * const read_fruts_ptr, 
                                               fbe_u32_t array_pos);


/*!**************************************************************************
 * fbe_parity_rcw_calculate_memory_size()
 ****************************************************************************
 * @brief  This function calculates the total memory usage of the siots for
 *         this state machine.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts
 * @param num_pages_to_allocate_p - Number of memory pages to allocate
 * 
 * @return None.
 *
 * @author
 *  1/21/2014 - Created. Deanna Heng
 *
 **************************************************************************/

fbe_status_t fbe_parity_rcw_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                  fbe_raid_fru_info_t *read_info_p,
                                                  fbe_raid_fru_info_t *read2_info_p,
                                                  fbe_raid_fru_info_t *write_info_p,
                                                  fbe_u32_t *num_pages_to_allocate_p)
{
    fbe_u16_t num_sgs[FBE_RAID_MAX_SG_TYPE];
    fbe_u32_t num_fruts = 0;
    fbe_u32_t read_fruts = 0;
    fbe_u32_t total_blocks = 0;
    fbe_status_t status;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    fbe_zero_memory(&num_sgs[0], (sizeof(fbe_u16_t) * FBE_RAID_MAX_SG_TYPE));
    /* Then calculate how many buffers are needed.
     */
    status = fbe_parity_rcw_get_fruts_info(siots_p, 
                                           read_info_p,
                                           read2_info_p,
                                           write_info_p,
                                           (fbe_block_count_t*)&total_blocks,
                                           &read_fruts);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        /*Split trace to two lines*/
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_rcw_calc_mem_size" ,
                             "parity:rcw fail get fru:stat 0x%x,"
                             "siots=0x%p>\n",
                             status, siots_p);
		fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_rcw_calc_mem_size" ,
                             "read_info=0x%p,write_info=0x%p\n",
                             read_info_p, write_info_p);
        return(status); 
    }

    /* First start off with 1 fruts for every data position we are writing 
     * and for every parity position, and one for any prereads
     */
    num_fruts = siots_p->data_disks + fbe_raid_siots_parity_num_positions(siots_p) + read_fruts;

    if (fbe_raid_siots_is_buffered_op(siots_p))
    {
        /* Buffered transfer needs blocks allocated.
         */
        total_blocks += (fbe_u32_t)siots_p->xfer_count;
    }

    /* Increase the total blocks allocated by 1 per write position for write log headers if required
     */
    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_WRITE_LOG_HEADER_REQD))
    {
         fbe_u32_t journal_log_hdr_blocks;
        fbe_raid_geometry_get_journal_log_hdr_blocks(raid_geometry_p, &journal_log_hdr_blocks);
        total_blocks += (siots_p->data_disks + fbe_raid_siots_parity_num_positions(siots_p)) * journal_log_hdr_blocks;
    }

    fbe_raid_siots_set_total_blocks_to_allocate(siots_p, total_blocks);
    fbe_raid_siots_set_optimal_page_size(siots_p,
                                         num_fruts, 
                                         siots_p->total_blocks_to_allocate);

    /* Next calculate the number of sgs of each type.
     */
    status = fbe_parity_rcw_calculate_num_sgs(siots_p, &num_sgs[0], read_info_p, read2_info_p, write_info_p, FBE_TRUE);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        /*Split trace to two lines*/
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_rcw_calc_mem_size",
                             "parity:rcw fail calc num sgs:stat 0x%x,siots=0x%p>\n",
                             status, siots_p);
		fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_rcw_calc_mem_size",
                             "read_info=0x%p,write_info=0x%p<\n",
                             read_info_p, write_info_p);
        return (status); 
    }

    /* Take the information we calculated above and determine how this translates 
     * into a number of pages. 
     */
    status = fbe_raid_siots_calculate_num_pages(siots_p, num_fruts,
                                                &num_sgs[0], FBE_TRUE /* In-case recovery verify is required*/);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_rcw_calc_mem_size" ,
                             "parity:rcw fail to calc num pages with stat 0x%x,siots=0x%p\n",
                             status, siots_p);
        return(status); 
    }
    *num_pages_to_allocate_p = siots_p->num_ctrl_pages;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_rcw_calculate_memory_size()
 **************************************/
/*!**************************************************************
 * fbe_parity_rcw_setup_resources()
 ****************************************************************
 * @brief
 *  Setup the newly allocated resources.
 *
 * @param   siots_p - current I/O.     
 * @param   read_p - Pointer to per position read information          
 * @param   write_p - Pointer to per position write information          
 *
 * @return fbe_status_t
 *
 * @author
 *  1/21/2014 - Created. Deanna Heng
 *
 ****************************************************************/

fbe_status_t fbe_parity_rcw_setup_resources(fbe_raid_siots_t * siots_p, 
                                            fbe_raid_fru_info_t *read_p,
                                            fbe_raid_fru_info_t *read2_p, 
                                            fbe_raid_fru_info_t *write_p)
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
    status = fbe_parity_rcw_setup_fruts(siots_p, read_p, read2_p, write_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity:fail to setup fruts for rcw opt:stat=0x%x,siots=0x%p\n",
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
                             "parity : failed to init vcts for siots_p = 0x%p, status = 0x%x\n",
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
                             "parity : failed to init vrts for siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return status;
    }
    /* Plant the allocated sgs into the locations calculated above.
     */
    status = fbe_raid_fru_info_array_allocate_sgs(siots_p, &memory_info,
                                                  read_p, read2_p, write_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {  
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to allocate sgs with status 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return(status); 
    }

    /* Assign buffers to sgs.
     * We reset the current ptr to the beginning since we always 
     * allocate buffers starting at the beginning of the memory. 
     */
    status = fbe_parity_rcw_setup_sgs(siots_p, &data_memory_info, read_p, read2_p, write_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {  
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity:rcw fail to setup sgs with stat 0x%x,siots=0x%p\n",
                             status, siots_p);
        return (status); 
    }

    /* Make sure the buffer memory we just used does not overlap into 
     * the area used for other resources. 
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_parity_rcw_setup_resources()
 ******************************************/

/****************************************************************
 * fbe_parity_rcw_count_buffers()
 ****************************************************************
 * @brief
 *  Count the number of buffers needed for an RCW.
 *
 * @param siots_p - The raid sub iots to count for.             
 *
 * @return
 *  Number of blocks worth of buffers required.    
 *
 * @author
 *  1/21/2014 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_block_count_t fbe_parity_rcw_count_buffers(fbe_raid_siots_t *siots_p)
{
    fbe_block_count_t blocks = 0;
    fbe_block_count_t pre_read_blocks =
    siots_p->parity_count * fbe_raid_siots_get_width(siots_p) - (fbe_u32_t)(siots_p->xfer_count);

    /* More setup may be required here.
     */
    if (fbe_raid_siots_is_buffered_op(siots_p))
    {
        /* Allocate a full stripe worth of buffers,
         * since we will be doing a host xfer.
         */
        blocks = siots_p->parity_count * fbe_raid_siots_get_width(siots_p);
    }
    else
    {
        /* Allocate only enough for the pre-reads,
         * since the write data is provided.
         */
        blocks += pre_read_blocks;
    }
    return blocks;
}

/*!***************************************************************
 * fbe_parity_rcw_get_fruts_info()
 ****************************************************************
 * @brief
 *  This function initializes the fru info.
 *
 * @param siots_p - Current sub request.
 * @param read_p - read info to calculate for this request.
 * @param write_p - write info to calculate for this request.
 * @param read_blocks_p - number of blocks being read.
 *
 * @return
 *  None
 *
 * @author
 *  1/21/2014 - Created. Deanna Heng
 *
 ****************************************************************/

fbe_status_t fbe_parity_rcw_get_fruts_info(fbe_raid_siots_t * siots_p,
                                           fbe_raid_fru_info_t *read_p,
                                           fbe_raid_fru_info_t *read2_p,
                                           fbe_raid_fru_info_t *write_p,
                                           fbe_block_count_t *read_blocks_p,
                                           fbe_u32_t *read_fruts_count_p)
{
    fbe_lba_t write_lba;         /* block offset where access starts */
    fbe_block_count_t  write_blocks;       /* number of blocks accessed */
    fbe_lba_t read_lba;
    fbe_block_count_t  read_blocks;       /* number of blocks read */
    fbe_u32_t  array_pos;         /* Actual array position of access */
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u16_t data_pos;          /* data position in array */
    fbe_block_count_t ret_read_blocks = 0;
    fbe_u32_t ret_read_fruts_count = 0;

    fbe_lba_t parity_range_offset;  /* Distance from parity boundary to parity_start */

    fbe_block_count_t r1_blkcnt[FBE_RAID_MAX_DISK_ARRAY_WIDTH];    /* block counts for pre-reads, */
    fbe_block_count_t r2_blkcnt[FBE_RAID_MAX_DISK_ARRAY_WIDTH];    /* distributed across the FRUs */

    fbe_u8_t *position = siots_p->geo.position;

    /* Check number of parity drives that are supported. R5=1, R6=2
     */
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_lba_t logical_parity_start = siots_p->geo.logical_parity_start;

    fbe_u32_t width = fbe_raid_siots_get_width(siots_p);
    fbe_zero_memory(read_p, sizeof(fbe_raid_fru_info_t) * width);
    fbe_zero_memory(read2_p, sizeof(fbe_raid_fru_info_t) * width);
    fbe_zero_memory(write_p, sizeof(fbe_raid_fru_info_t) * width);

    /* Leverage MR3 calculations to describe what
     * portions of stripe(s) are *not* being accessed.
     */
    status = fbe_raid_geometry_calc_preread_blocks(siots_p->start_lba,
                                                   &siots_p->geo,
                                                   siots_p->xfer_count,
                                                   (fbe_u32_t)fbe_raid_siots_get_blocks_per_element(siots_p),
                                                   fbe_raid_siots_get_width(siots_p) - parity_drives,
                                                   siots_p->parity_start,
                                                   siots_p->parity_count,
                                                   r1_blkcnt,
                                                   r2_blkcnt);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        /*Split trace to two lines*/
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity:fail calc pre-read blocks:status=0x%x>\n",
                             status);
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "siots=0x%p,r1_blkcnt = 0x%p,r2_blkcnt=0x%p<\n",
                             siots_p, r1_blkcnt, r2_blkcnt);
        return status;
    }

    /* Calculate the total offset from our parity range.
     */
    parity_range_offset = fbe_raid_siots_get_parity_start_offset(siots_p);

    /* Loop through all data positions, setting up
     * each fru info.
     */
    for (data_pos = 0; data_pos < (fbe_raid_siots_get_width(siots_p) - parity_drives); data_pos++)
    {
        /* The pre-reads should NOT exceed the parity range.  */
        if ((r1_blkcnt[data_pos] + r2_blkcnt[data_pos]) > siots_p->parity_count)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "pre-read range %llu %llu exceeds parity range %llu\n", 
                                 (unsigned long long)r1_blkcnt[data_pos],
				 (unsigned long long)r2_blkcnt[data_pos],
				 (unsigned long long)siots_p->parity_count);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Should not be processing parity position here */
        if (position[data_pos] == siots_p->parity_pos)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: position %d equal to parity\n", position[data_pos]);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        array_pos = position[data_pos];
        write_blocks = siots_p->parity_count - r1_blkcnt[data_pos] - r2_blkcnt[data_pos];

        /* If a write_blocks exists for this position,
         * fill out read/write fruts.
         */
        if (write_blocks)
        {

            /* Init current pre-read and write's offset and position.
             * Note that the extent structure is used to determine offset/position.
             */
            write_lba = logical_parity_start + parity_range_offset + r1_blkcnt[data_pos];

            /* Init the write fruts.
             */
            write_p->lba = write_lba;
            write_p->blocks = write_blocks;
            write_p->position = array_pos;
            fbe_raid_siots_setup_rcw_preread_fru_info(siots_p, write_p, array_pos);

            /* Tack on any prereads if necessary
             */
            if ((r1_blkcnt[data_pos] == 0) && 
                (write_p->lba != write_lba))
            {
                read_p->lba = write_p->lba;
                read_p->blocks = write_lba - write_p->lba;
                read_p->position = array_pos;
                ret_read_blocks += read_p->blocks;
                ret_read_fruts_count++;
                read_p++;
            }

            if ((r2_blkcnt[data_pos] == 0) &&
                (write_p->lba + write_p->blocks != write_lba + write_blocks))
            {
                read2_p->lba = write_lba + write_blocks;
                read2_p->blocks = (write_p->lba + write_p->blocks) - (write_lba +write_blocks);
                read2_p->position = array_pos;
                ret_read_blocks += read2_p->blocks;
                ret_read_fruts_count++;
                read2_p++;
            }

            write_p++;
        } 

        if (r1_blkcnt[data_pos] > 0)
        {
            /* initialize the fru info for the 1st pre-read.
             */
            read_blocks = r1_blkcnt[data_pos];
            read_lba = logical_parity_start + parity_range_offset;

            /* Check to see if we're pre-reading the entire 
             * locked area of the disk. Rather than perform a second
             * operation, just combine the two.
             */
            if (r2_blkcnt[data_pos] == siots_p->parity_count - read_blocks)
            {
                read_blocks = siots_p->parity_count;
                r2_blkcnt[data_pos] = 0;
            }
            /* Init the read info.
             */
            read_p->lba = read_lba;
            read_p->blocks = read_blocks;
            read_p->position = array_pos;

            ret_read_fruts_count++;
            ret_read_blocks += read_p->blocks;
            read_p++;
        }


        if (r2_blkcnt[data_pos] > 0)
        {
            /* initialize the fru info for the 2nd pre-read.
             */
            read_blocks = r2_blkcnt[data_pos];
            read_lba = logical_parity_start + parity_range_offset +
                       siots_p->parity_count - read_blocks;
            read2_p->lba = read_lba;
            read2_p->blocks = read_blocks;
            read2_p->position = array_pos;

            ret_read_fruts_count++;
            ret_read_blocks += read2_p->blocks;
            read2_p++;
        }
    }                           

    /* Init the fruts for pre-read/write
     * of parity in the final fbe_raid_fruts_t.
     */
    write_lba = logical_parity_start + parity_range_offset;
    write_blocks = siots_p->parity_count;


    array_pos = position[fbe_raid_siots_get_width(siots_p) - parity_drives];
    write_p->lba = write_lba;
    write_p->blocks = write_blocks;
    write_p->position = array_pos;

    read_p->lba = write_lba;
    read_p->blocks = write_blocks;
    read_p->position = array_pos;

    /* Align read and the write
     */
    fbe_raid_siots_setup_rcw_preread_fru_info(siots_p, read_p, array_pos);
    fbe_raid_siots_setup_rcw_preread_fru_info(siots_p, write_p, array_pos);

    ret_read_blocks += read_p->blocks;
    ret_read_fruts_count++;

    if (parity_drives == 2)
    {
        /* For RAID 6 initialize the 2nd parity drive.
         * We do not need to read from the 2nd parity drive
         * since we only need the stamps from the first parity drive
         */
        write_p++;
        array_pos = position[fbe_raid_siots_get_width(siots_p) - 1];
        write_p->lba = write_lba;
        write_p->blocks = write_blocks;
        write_p->position = array_pos;
        fbe_raid_siots_setup_rcw_preread_fru_info(siots_p, write_p, array_pos);
        if (write_lba != write_p->lba || 
            (write_lba + write_blocks != write_p->lba + write_p->blocks))
        {
            read_p++;
            read_p->lba = write_p->lba;
            read_p->blocks = write_p->blocks;
            read_p->position = array_pos;
            ret_read_blocks += read_p->blocks;
        }

        /* allocate blocks for the write */
        ret_read_blocks += write_p->blocks;

        

        /* If this is not a parity drive return error.
         */                             
        if (!fbe_raid_siots_is_parity_pos(siots_p, write_p->position))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: write position not parity position\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    if (read_blocks_p != NULL)
    {
        *read_blocks_p = ret_read_blocks;
    }
    if (read_fruts_count_p != NULL)
    {
        *read_fruts_count_p = ret_read_fruts_count;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_rcw_get_fruts_info() 
 **************************************/

/*!**************************************************************
 * fbe_raid_siots_setup_preread_fru_info()
 ****************************************************************
 * @brief   
 *   This is a helper function for setting up a 468 read fruts.   
 *  
 * @param read_fruts_ptr - The ptr to the read fruts to setup.
 * @param write_fruts_ptr - The ptr to the write fruts to use   
 *                          to help us setup the read fruts.
 * @param array_pos - The position in the LUN to set.
 *
 * @return  None
 *
 * @author
 *  11/21/2008 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_raid_siots_setup_rcw_preread_fru_info(fbe_raid_siots_t *siots_p,
                                                 fbe_raid_fru_info_t * const read_fruts_ptr, 
                                                 fbe_u32_t array_pos)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    if (!fbe_raid_geometry_position_needs_alignment(raid_geometry_p, array_pos)) {
        /* This is a normal write, so just init to cover the same range 
         * as the write. 
         */
    } else {
        fbe_raid_geometry_align_io(raid_geometry_p,
                                   &read_fruts_ptr->lba,
                                   &read_fruts_ptr->blocks);
    }
    return;
}

/*!***************************************************************
 * fbe_parity_rcw_setup_fruts()
 ****************************************************************
 * @brief
 *  This function will set up fruts for the state machine.
 *
 * @param siots_p   - ptr to the fbe_raid_siots_t
 * @param read_p    - pointer to read info
 * @param read2_p - Array of read2 information.
 * @param write_p   - pointer to write info
 * @param memory_info_p - Pointer to memory requet information
 *
 * @return fbe_status_t
 *
 * @author
 *  1/21/2014 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t fbe_parity_rcw_setup_fruts(fbe_raid_siots_t * siots_p, 
                                        fbe_raid_fru_info_t *read_p,
                                        fbe_raid_fru_info_t *read2_p,
                                        fbe_raid_fru_info_t *write_p,
                                        fbe_raid_memory_info_t *memory_info_p)
{
    
    fbe_u16_t data_pos;           /* data position in array */
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t capacity;
    fbe_u32_t width = 0;
    fbe_u8_t *position = siots_p->geo.position;
    fbe_raid_siots_get_capacity(siots_p, &capacity);

    /* Simply iterate over the width. 
     * Always setup a write. 
     * Setup a read if a read is present. 
     */
    width = fbe_raid_siots_get_width(siots_p);
    if (RAID_COND(width == 0))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_rcw_setup_fruts" ,
                             "parity_rcw_setup_fruts got unexpected width of RG. width = 0x%x\n",
                             width);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (data_pos = 0; data_pos < width ; data_pos++)
    {
        if ((read_p->blocks > 0) && (read_p->position == position[data_pos]))
        {
            if (RAID_COND((read_p->lba + read_p->blocks) > capacity))
            {
                fbe_raid_siots_trace(siots_p,
                                     FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_rcw_setup_fruts" ,
                                     "parity_rcw_setup_fruts:(read->lba 0x%llx+read->blocks 0x%llx)>capacity 0x%llx:siots=0x%p\n",
                                     (unsigned long long)read_p->lba,
				     (unsigned long long)read_p->blocks,
				     (unsigned long long)capacity,
                                     siots_p);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            status = fbe_raid_setup_fruts_from_fru_info(siots_p,
                                                        read_p,
                                                        &siots_p->read_fruts_head,
                                                        FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                                        memory_info_p);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            { 
                fbe_raid_siots_trace(siots_p,
                                     FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_rcw_setup_fruts" ,
                                     "parity:fail setup: mem_info=0x%p,"
                                     "read 0x%p,siots=0x%p,stat=0x%x\n",
                                     memory_info_p, read_p, siots_p, status);
                return status;
            }
            read_p++;
        }
        if ((read2_p->blocks > 0) && (read2_p->position == position[data_pos]))
        {
            if (RAID_COND((read2_p->lba + read2_p->blocks) > capacity))
            {
                fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_rcw_setup_fruts" ,
                                     "parity_rcw_setup_fruts fru info 0x%p lba %llx+blocks 0x%llx is beyond capacity %llx\n",
                                     read2_p,
                				     (unsigned long long)read2_p->lba,
                				     (unsigned long long)read2_p->blocks,
                				     (unsigned long long)capacity);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            status = fbe_raid_setup_fruts_from_fru_info(siots_p,
                                                        read2_p,
                                                        &siots_p->read2_fruts_head,
                                                        FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                                        memory_info_p);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            { 
                fbe_raid_siots_trace(siots_p,
                                     FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_rcw_setup_fruts" ,
                                     "parity:fail to setup fruts:read2=0x%p,siots=0x%p,"
                                     "data pos=0x%x, stat=0x%x\n",
                                     read2_p, siots_p, data_pos, status);
                return status;
            }
            read2_p++;
        }

        if (write_p->blocks > 0 && write_p->position == position[data_pos]) {
        
            if (RAID_COND((write_p->lba + write_p->blocks) > capacity))
            {
                fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_rcw_setup_fruts" ,
                                     "parity_rcw_setup_fruts, fru info 0x%p lba %llx+blocks 0x%llx is beyond capacity %llx\n",
                                     write_p, (unsigned long long)write_p->lba,
    				 (unsigned long long)write_p->blocks,
    				 (unsigned long long)capacity);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            status = fbe_raid_setup_fruts_from_fru_info(siots_p,
                                                        write_p,
                                                        &siots_p->write_fruts_head,
                                                        FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                                        memory_info_p);

            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            { 
                fbe_raid_siots_trace(siots_p,
                                     FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_rcw_setup_fruts" ,
                                     "parity:fail setup fru: mem_info=0x%p,write=0x%p,"
                                     "siots=0x%p,data pos=0x%x,stat=0x%x\n",
                                     memory_info_p, write_p, siots_p, data_pos, status);
                return status;
            }

            write_p++;
        }
    } /* While data_pos < width */

    return FBE_STATUS_OK;

}
/* end of fbe_parity_rcw_setup_fruts() */

/***************************************************************************
 * fbe_parity_rcw_calculate_num_sgs()
 ***************************************************************************
 * @brief
 *  This function counts the number of each type of sg needed.
 *  This function is executed from the RAID 5/RAID 3 rcw state machine.
 *
 * @param siots_p - Pointer to siots
 * @param num_sgs_p - Pointer of array of each sg type length FBE_RAID_MAX_SG_TYPE.
 * @param read_p - array of read info.
 * @param read2_p - Read 2 information.
 * @param write_p - array of write info.
 * @param b_generate - True if in generate and should not print certain errors.
 *
 * @return fbe_status_t
 *
 * @author
 *  1/21/2014 - Created. Deanna Heng
 *
 **************************************************************************/

fbe_status_t fbe_parity_rcw_calculate_num_sgs(fbe_raid_siots_t *siots_p,
                                              fbe_u16_t *num_sgs_p,
                                              fbe_raid_fru_info_t *read_p,
                                              fbe_raid_fru_info_t *read2_p,
                                              fbe_raid_fru_info_t *write_p,
                                              fbe_bool_t b_generate)
{
    fbe_u16_t data_pos;    /* data position in array */
    fbe_block_count_t mem_left = 0;    /* sectors unused in last buffer */
    fbe_block_count_t pr_mem_left = 0;    /* sectors unused in last buffer */
    fbe_u32_t sg_count_vec[FBE_RAID_MAX_DISK_ARRAY_WIDTH];    /* S/G elements per data drive */
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u16_t write_log_header_count = 0;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* Check number of parity drives that are supported. R5=1, R6=2
     */
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_u32_t width = fbe_raid_siots_get_width(siots_p);
    fbe_u32_t read_sg_count = 0;
    fbe_u32_t write_sg_count = 0;
    fbe_sg_element_with_offset_t sg_desc;    /* tracks current location of data in S/G list */

    /* Check if we need to allocate and plant write log header buffers
     */
    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_WRITE_LOG_HEADER_REQD))
    {
        fbe_u32_t journal_log_hdr_blocks;
        fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
        fbe_raid_geometry_get_journal_log_hdr_blocks(raid_geometry_p, &journal_log_hdr_blocks);

        /* Allow 2 sg entries for the write log header.
         */
        write_log_header_count = 2;

        /* Since we're going to plant all the write fruts write log header buffers first in fbe_parity_mr3_setup_sgs,
         *  we need to skip over them here so the rest of these plants end up in the same place relative to buffer
         *  boundaries.  Since mem_left just tracks boundaries, it's ok if it's equal to 0 or page_size.
         */
        mem_left = siots_p->data_page_size - (((siots_p->data_disks + parity_drives) * journal_log_hdr_blocks) % siots_p->data_page_size);
    }

    /* Clear out the vectors of sg sizes per position.
     */
    for (data_pos = 0; data_pos < (FBE_RAID_MAX_DISK_ARRAY_WIDTH); data_pos++)
    {
        sg_count_vec[data_pos] = 0;
    }

    /* Add the sg for the first pre-reads to the list of sg_ids.
     */
    while (read_p->blocks > 0 && read_p->position != siots_p->parity_pos)
    {
            fbe_u32_t current_data_pos;
            status = FBE_RAID_DATA_POSITION(read_p->position, siots_p->parity_pos,
                                            width, parity_drives, &current_data_pos);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                 fbe_raid_siots_object_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_rcw_calc_num_sgs",
                                             "parity position:fail to cvt pos: stat = 0x%x, "
                                             "siots = 0x%p, read = 0x%p, position 0x%x datapos: 0x%x\n",
                                             status, siots_p, read_p, read_p->position, data_pos);
                 return status;
            }
            read_sg_count = 0;
            mem_left = fbe_raid_sg_count_uniform_blocks(read_p->blocks,
                                                        siots_p->data_page_size,
                                                        mem_left,
                                                        &read_sg_count);

            status = fbe_raid_fru_info_set_sg_index(read_p, read_sg_count, !b_generate);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p,FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_rcw_calc_num_sgs", 
                                     "parity: fail to set sg index: stat= 0x%x, siots= 0x%p, "
                                     "read_p = 0x%p\n", 
                                     status, siots_p, read_p);
                return status;
            }

            num_sgs_p[read_p->sg_index]++;
            read_p++;
    }

    while (read2_p->blocks > 0 && read2_p->position != siots_p->parity_pos)
    {
            fbe_u32_t current_data_pos;
            status = FBE_RAID_DATA_POSITION(read2_p->position, siots_p->parity_pos,
                                               width, parity_drives, &current_data_pos);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_rcw_calc_num_sgs",
                                            "parity:fail to cvt pos: stat = 0x%x, "
                                            "siots = 0x%p, read2_p = 0x%p, position 0x%x\n",
                                            status, siots_p, read2_p, read2_p->position);
                 return status;
            }
            read_sg_count = 0;
            mem_left = fbe_raid_sg_count_uniform_blocks(read2_p->blocks,
                                                        siots_p->data_page_size,
                                                        mem_left,
                                                        &read_sg_count);

            status = fbe_raid_fru_info_set_sg_index(read2_p, read_sg_count, !b_generate);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p,
                                     FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_rcw_calc_num_sgs", 
                                     "parity:fail to set sg index:stat=0x%x,siots=0x%p, "
                                     "read2=0x%p\n", 
                                     status, siots_p, read2_p);

                return status;
            }

            num_sgs_p[read2_p->sg_index]++;
            read2_p++;
    }

    if (!fbe_raid_siots_is_buffered_op(siots_p))
    {
        fbe_u32_t data_pos;
        /* Setup the sg descriptor to point to the "data" we will use
         * for this operation. 
         */
        status = fbe_raid_siots_setup_sgd(siots_p, &sg_desc);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: failed to setup sg: status  = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return status;
        }
        /* Count write sgs for a normal rcw.
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


        status = fbe_raid_scatter_cache_to_bed(siots_p->start_lba,
                                               siots_p->xfer_count,
                                               data_pos,
                                               fbe_raid_siots_get_width(siots_p) - parity_drives,
                                               fbe_raid_siots_get_blocks_per_element(siots_p),
                                               &sg_desc,
                                               sg_count_vec);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: failed to scatter cache: status = 0x%x, siots_p = 0x%p "
                                 "data_pos = 0x%x\n",
                                 status, siots_p, data_pos);
            return status;
        }
    }
    else
    {
        fbe_u32_t data_pos;
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
        mem_left = fbe_raid_sg_scatter_fed_to_bed(siots_p->start_lba,
                                                  siots_p->xfer_count,
                                                  data_pos,
                                                  fbe_raid_siots_get_width(siots_p) - parity_drives,
                                                  fbe_raid_siots_get_blocks_per_element(siots_p),
                                                  (fbe_block_count_t)siots_p->data_page_size,
                                                  mem_left,
                                                  sg_count_vec);
    }

    /*
     * Stage 2: This operation must supply a set of S/G lists
     *          which reference buffers from the BM into which
     *          the older data residing on the disk may be
     *          pre-read, one list per disk.
     *
     *          These S/G lists will be referenced when 
     *          computing parity/writestamps from the pre-read data.
     */
    if (RAID_COND(write_p == NULL))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: write_p fruts is null. write: %p\n", 
                             write_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    while (write_p->blocks > 0 && write_p->position != siots_p->parity_pos)
    {
            fbe_u32_t current_data_pos;

            status = FBE_RAID_DATA_POSITION(write_p->position,
                                            siots_p->parity_pos,
                                            fbe_raid_siots_get_width(siots_p),
                                            parity_drives,
                                            &current_data_pos);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                            status, siots_p);
                return status;
            }
            if (current_data_pos >= (fbe_raid_siots_get_width(siots_p) - parity_drives))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity: data position %d unexpected %d %d\n", 
                                     current_data_pos, fbe_raid_siots_get_width(siots_p), parity_drives);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* If the write is unaligned, then add on a constant number of sgs.
             */
            if (fbe_raid_geometry_position_needs_alignment(raid_geometry_p, write_p->position)) {
                sg_count_vec[current_data_pos] += FBE_RAID_EXTRA_ALIGNMENT_SGS;
            }
    
            /* Put the totals we calculated above 
             * for the write data onto the sg_id list.
             */
            if (sg_count_vec[current_data_pos] == 0)
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity: sg_count_vec for data_pos: %d is zero. %p \n", data_pos, siots_p);
                return FBE_STATUS_GENERIC_FAILURE;
            }
    
            /* add header count to the sg count, in case we need to prepend a write log header */
            status = fbe_raid_fru_info_set_sg_index(write_p, sg_count_vec[current_data_pos] + write_log_header_count, !b_generate);
    
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p,
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity:fail to set sg index:stat=0x%x,siots=0x%p, "
                                     "write=0x%p\n", 
                                     status, siots_p, write_p);
    
                return status;
            }
    
            num_sgs_p[write_p->sg_index]++;
            write_p++;

    }    /* end while fruts position != parity_pos */


    /*
     * Stage 3: This operation must supply an S/G list which
     *          references buffers from the BM into which
     *          the old parity residing on the disk may be pre-read
     *
     *          We use a single S/G list first to read parity from the
     *          disk, then again to write the new parity onto the disk.
     *          
     */
    if (write_p->blocks < siots_p->parity_count)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: write blocks %llu not parity count %llu.\n", 
                             (unsigned long long)write_p->blocks,
			     (unsigned long long)siots_p->parity_count);
        return FBE_STATUS_GENERIC_FAILURE;        
    }

    /* read may also be aligned to 4k
     */
    if (read_p->blocks < siots_p->parity_count)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: read blocks %llu not parity count %llu.\n", 
                             (unsigned long long)read_p->blocks,
			     (unsigned long long)siots_p->parity_count);
        return FBE_STATUS_GENERIC_FAILURE;        
    }



    /* Next count the num blocks for the pre-read on parity
     */
    read_sg_count = 0;
    pr_mem_left = fbe_raid_sg_count_uniform_blocks(read_p->blocks,
                                                siots_p->data_page_size,
                                                mem_left,
                                                &read_sg_count);
    
    status = fbe_raid_fru_info_set_sg_index(read_p, read_sg_count, !b_generate);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity:fail to set sg index:stat=0x%x,siots=0x%p,"
                             "read=0x%p\n", 
                             status, siots_p, read_p);

       return status;
    }
    num_sgs_p[read_p->sg_index]++;

    write_sg_count = 0;
    mem_left = fbe_raid_sg_count_uniform_blocks(read_p->blocks,
                                                siots_p->data_page_size,
                                                mem_left,
                                                &write_sg_count);

    /* add header count to the sg count, in case we need to prepend a write log header */
    if (!fbe_raid_fru_info_set_sg_index(write_p, write_sg_count + write_log_header_count, !b_generate))
    {
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    num_sgs_p[write_p->sg_index]++;
    mem_left = pr_mem_left;

    /* If R6, handles stage 3 for diagonal parity.
     */                             
    if (parity_drives == 2)
    {
        read_p++;
        write_p++; 
   
        if (read_p != NULL && read_p->blocks > 0)
        {
            /* Next count the num blocks for the pre-read on parity
             */
            read_sg_count = 0;
            pr_mem_left = fbe_raid_sg_count_uniform_blocks(read_p->blocks,
                                                        siots_p->data_page_size,
                                                        mem_left,
                                                        &read_sg_count);
            
            status = fbe_raid_fru_info_set_sg_index(read_p, read_sg_count, !b_generate);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p,
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity:fail to set sg index:stat=0x%x,siots=0x%p,"
                                     "read=0x%p\n", 
                                     status, siots_p, read_p);

               return status;
            }
            num_sgs_p[read_p->sg_index]++;
        } 

        write_sg_count = 0;
        mem_left = fbe_raid_sg_count_uniform_blocks(write_p->blocks,
                                                    siots_p->data_page_size,
                                                    mem_left,
                                                    &write_sg_count);
        /* add header count to the sg count, in case we need to prepend a write log header */
        if (!fbe_raid_fru_info_set_sg_index(write_p, write_sg_count + write_log_header_count, !b_generate))
        {
            return FBE_STATUS_INSUFFICIENT_RESOURCES;
        }
        num_sgs_p[write_p->sg_index]++;

    }
    return status;
}
/*****************************************
 * fbe_parity_rcw_calculate_num_sgs
 *****************************************/
/***************************************************************************
 * fbe_parity_rcw_validate_sgs     
 ***************************************************************************
 * @brief
 *  This function is responsible for verifying the
 *  consistency of rcw sg lists
 *
 * @param siots_p - pointer to I/O.
 *
 * @return
 *  fbe_status_t
 *
 * @author
 *  1/21/2014 - Created. Deanna Heng
 *
 **************************************************************************/

fbe_status_t fbe_parity_rcw_validate_sgs(fbe_raid_siots_t * siots_p)
{
    fbe_block_count_t total_read_blocks = 0;
    fbe_block_count_t total_write_blocks = 0;
    fbe_block_count_t total_sg_blocks = 0;
    fbe_block_count_t curr_blocks;
    fbe_u32_t sgs = 0;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_fruts_t *read2_fruts_ptr = NULL;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    while (read_fruts_ptr)
    {
        sgs = 0;
        if (read_fruts_ptr->sg_id != NULL)
        {
            if (!fbe_raid_memory_header_is_valid((fbe_raid_memory_header_t*)read_fruts_ptr->sg_id))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "raid header invalid for fruts 0x%p, sg_id: 0x%p\n",
                                            read_fruts_ptr, read_fruts_ptr->sg_id);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        if (read_fruts_ptr->blocks == 0)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "rcw: read pos %d blocks is %llu\n",
                                 read_fruts_ptr->position,
                                 (unsigned long long)read_fruts_ptr->blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        status = fbe_raid_sg_count_sg_blocks(fbe_raid_fruts_return_sg_ptr(read_fruts_ptr), 
                                             &sgs,
                                             &curr_blocks);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: failed to count sg blocks: status = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return status;
        }

        total_read_blocks += curr_blocks;
        if (read_fruts_ptr->blocks != curr_blocks)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "rcw: read pos %d blocks is %llu != %llu\n",
                                 read_fruts_ptr->position,
                				 (unsigned long long)curr_blocks,
                				 (unsigned long long)read_fruts_ptr->blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
    }

    fbe_raid_siots_get_read2_fruts(siots_p, &read2_fruts_ptr);
    while (read2_fruts_ptr)
    {
        if (read2_fruts_ptr->sg_id != NULL)
        {
            if (!fbe_raid_memory_header_is_valid((fbe_raid_memory_header_t*)read2_fruts_ptr->sg_id))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_CRITICAL,
                                            "raid header invalid for read2 fruts 0x%p, sg_id: 0x%p\n",
                                            read2_fruts_ptr, read2_fruts_ptr->sg_id);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        sgs = 0;
        if (read2_fruts_ptr->blocks == 0)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "rcw: read2 pos %d blocks is %llu\n",
                                 read2_fruts_ptr->position,
				 (unsigned long long)read2_fruts_ptr->blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        status = fbe_raid_sg_count_sg_blocks(fbe_raid_fruts_return_sg_ptr(read2_fruts_ptr), 
                                             &sgs,
                                             &curr_blocks);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: failed to count sg blocks: status = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return status;
        }

        total_read_blocks += curr_blocks;
        if (read2_fruts_ptr->blocks != curr_blocks)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "rcw: read2 pos %d blocks is %llu != %llu\n",
                                 read2_fruts_ptr->position,
				 (unsigned long long)curr_blocks,
				 (unsigned long long)read2_fruts_ptr->blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        read2_fruts_ptr = fbe_raid_fruts_get_next(read2_fruts_ptr);
    }

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);
    while (write_fruts_ptr)
    {
        if (write_fruts_ptr->sg_id != NULL)
        {
            if (!fbe_raid_memory_header_is_valid((fbe_raid_memory_header_t*)write_fruts_ptr->sg_id))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "raid header invalid for write fruts 0x%p, sg_id: 0x%p\n",
                                            write_fruts_ptr, write_fruts_ptr->sg_id);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        sgs = 0;
        if (write_fruts_ptr->blocks == 0)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "rcw: write pos %d blocks is %llu\n",
                                 write_fruts_ptr->position,
				 (unsigned long long)write_fruts_ptr->blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        status = fbe_raid_sg_count_sg_blocks(fbe_raid_fruts_return_sg_ptr(write_fruts_ptr), 
                                             &sgs,
                                             &curr_blocks);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: failed to count sg blocks: status = 0x%x, siots_p = 0x%p\n",
                                status, siots_p);
            return status;
        }
        if (write_fruts_ptr->blocks != curr_blocks)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "rcw: write pos %d blocks is %llu != %llu\n",
                                 write_fruts_ptr->position,
				 (unsigned long long)curr_blocks,
				 (unsigned long long)write_fruts_ptr->blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        total_write_blocks += curr_blocks;
        write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);
    }

    /* 
     * Make sure the total write blocks is equal 
     * to the total data we are writing plus the size of the parity data. 
     */
    if (total_write_blocks < (siots_p->xfer_count + 
                              (siots_p->parity_count * fbe_raid_siots_parity_num_positions(siots_p))))
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                    "Not enough blocks found in write sgs %llu\n",
                                    (unsigned long long)total_sg_blocks);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Make sure the total fruts blocks is equal to the total data we are 
     * reading and writing which is (width +1)*parity count.  
     */
    total_sg_blocks = total_write_blocks + total_read_blocks;
    if (total_sg_blocks < (siots_p->parity_count + 
                            (fbe_raid_siots_get_width(siots_p) * siots_p->parity_count)))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "rcw: total blocks %llu < width %d * parity_count + parity_count %llu\n",
                             (unsigned long long)total_sg_blocks,
                             fbe_raid_siots_get_width(siots_p),
                             (unsigned long long)siots_p->parity_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    return status;
}

/***************************************************************************
 * fbe_parity_rcw_setup_parity_sgs()
 ***************************************************************************
 * @brief
 *  This function is responsible for initializing the S/G
 *  lists associated with the parity position(s).
 *
 *  This function is executed from the RAID 5/RAID 3 rcw state
 *  machine once resources (i.e. memory) are allocated.
 *
 * @param   siots_p - pointer to SUB_IOTS data
 * @param   memory_info_p - Pointer to memory request information
 * @param   read_fruts_ptr - Pointer to parity read fruts
 * @param   write_fruts_ptr - Pointer to parity write fruts
 * @param   read2_mem_pp - Address of pointer to second parity read buffer
 * @param   read2_mem_left_p - Pointer to bytes left in second parity read buffer
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  1/21/2014 - Created. Deanna Heng
 *
 **************************************************************************/

static fbe_status_t fbe_parity_rcw_setup_parity_sgs(fbe_raid_siots_t * siots_p,
                                                    fbe_raid_memory_info_t *memory_info_p,
                                                    fbe_raid_fruts_t *read_fruts_ptr,
                                                    fbe_raid_fruts_t *write_fruts_ptr,
                                                    fbe_raid_memory_element_t **read2_mem_pp,
                                                    fbe_u32_t *read2_mem_left_p)

{
    fbe_status_t            status;
    fbe_raid_memory_element_t *read_mem_p = NULL;  /* Saves read buffer information for parity position*/
    fbe_u32_t               read_mem_left = 0;
    fbe_sg_element_t       *sgl_ptr;
    fbe_u16_t sg_total = 0;
    fbe_u32_t dest_byte_count =0;

    /* Validate this is is a parity position
     */
    if (RAID_DBG_COND(!fbe_raid_siots_is_parity_pos(siots_p, read_fruts_ptr->position)))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "read pos %d != parity_pos %d\n", 
                             read_fruts_ptr->position, siots_p->parity_pos);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (RAID_DBG_COND(read_fruts_ptr->position != write_fruts_ptr->position))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "read pos %d != write pos %d\n", 
                             read_fruts_ptr->position, write_fruts_ptr->position);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (RAID_DBG_COND(write_fruts_ptr->blocks < read_fruts_ptr->blocks))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "write blocks %llx != read blocks %llx\n", 
                             (unsigned long long)write_fruts_ptr->blocks,
                             (unsigned long long)read_fruts_ptr->blocks);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (RAID_DBG_COND(read_fruts_ptr->lba != write_fruts_ptr->lba))
    {

        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "%s: write fruts lba 0x%llx != read fruts lba 0x%llx\n",
                             __FUNCTION__,
                             (unsigned long long)write_fruts_ptr->lba,
                             (unsigned long long)read_fruts_ptr->lba);
        return FBE_STATUS_GENERIC_FAILURE; 
    }


    /* Assign the read blocks from the beginning to the end.
     * We use a different sgd so that the write will start from the
     * right location.
     */
    sgl_ptr = fbe_raid_fruts_return_sg_ptr(read_fruts_ptr);

    /* Now do the read all the way to the end.  Save the current buffer
     * information since we use the same buffers for the parity write.
     */
    read_mem_p = fbe_raid_memory_info_get_current_element(memory_info_p);
    read_mem_left = fbe_raid_memory_info_get_bytes_remaining_in_page(memory_info_p);

    if(!fbe_raid_get_checked_byte_count(read_fruts_ptr->blocks, &dest_byte_count))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                           "%s: byte count exceeding 32bit limit \n",
                                           __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    sg_total = fbe_raid_sg_populate_with_memory(memory_info_p, sgl_ptr, dest_byte_count);
    if (sg_total == 0)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                "%s: failed to populate sg for: siots = 0x%p\n",
                 __FUNCTION__,siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (RAID_DBG_ENABLED)
    {
        status = fbe_raid_fruts_validate_sg(read_fruts_ptr);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity:fail to valid sg:siots=0x%p,rd_fruts=0x%p,stat=0x%x\n", 
                                 siots_p, read_fruts_ptr, status);
            return status;
        }
    }

    /* Save the current buffer information for the second parity position
     */
    *read2_mem_pp = fbe_raid_memory_info_get_current_element(memory_info_p);
    *read2_mem_left_p = fbe_raid_memory_info_get_bytes_remaining_in_page(memory_info_p);

    /* Now populate the parity write buffers with the read buffers.  We
     * need to reset the buffer pointers back to the read buffers.
     */
    sgl_ptr = fbe_raid_fruts_return_sg_ptr(write_fruts_ptr);
    fbe_raid_memory_info_set_current_element(memory_info_p, read_mem_p);
    fbe_raid_memory_info_set_bytes_remaining_in_page(memory_info_p, read_mem_left);

    if(!fbe_raid_get_checked_byte_count(write_fruts_ptr->blocks, &dest_byte_count))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                           "%s: byte count exceeding 32bit limit \n",
                                           __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    sg_total = fbe_raid_sg_populate_with_memory(memory_info_p, sgl_ptr, dest_byte_count);
    if (sg_total == 0)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                            "%s: failed to populate sg for: siots = 0x%p\n",
                                             __FUNCTION__,siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (RAID_DBG_ENABLED)
    {
        status = fbe_raid_fruts_validate_sg(write_fruts_ptr);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity:fail to valid sg:siots=0x%p,wt_fruts=0x%p,stat=0x%x\n", 
                                 siots_p, write_fruts_ptr, status);
            return status;
        }
    }

    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_parity_rcw_setup_parity_sgs()
 *****************************************/

/***************************************************************************
 * fbe_parity_rcw_setup_sgs()
 ***************************************************************************
 * @brief
 *  This function is responsible for initializing the S/G
 *  lists allocated earlier by bem_rcw_alloc_sgs(). A
 *  full description of these lists (and rcw) can be found
 *  in that code.
 *
 *  This function is executed from the RAID 5/RAID 3 rcw state
 *  machine once resources (i.e. memory) are allocated.
 *
 * @param   siots_p - pointer to SUB_IOTS data
 * @param   memory_info_p - Pointer to memory request information
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @author
 *  1/21/2014 - Created. Deanna Heng
 *
 **************************************************************************/

fbe_status_t fbe_parity_rcw_setup_sgs(fbe_raid_siots_t * siots_p,
                                      fbe_raid_memory_info_t *memory_info_p,
                                      fbe_raid_fru_info_t *read_p,
                                        fbe_raid_fru_info_t *read2_p,
                                        fbe_raid_fru_info_t *write_p)
{
    fbe_status_t            status;
    fbe_raid_fruts_t       *read_fruts_ptr = NULL;
    fbe_raid_fruts_t       *read2_fruts_ptr = NULL;
    fbe_raid_fruts_t       *write_fruts_ptr = NULL;
    fbe_raid_fruts_t       *current_write_fruts_ptr = NULL;
    fbe_sg_element_with_offset_t host_sgd;      /* Used to reference our allocated buffers */
    fbe_raid_memory_element_t *read2_mem_p = NULL;  /* Saves read buffer information for second parity position*/
    fbe_u32_t               read2_mem_left = 0;
    fbe_sg_element_t        *current_sg_ptr;
    fbe_u16_t data_pos;
    fbe_u32_t dest_byte_count =0;
    fbe_u16_t sg_total =0;

    /* The w_sgl_ptr is used to hold the pointers to the write sgs.
     */
    fbe_sg_element_t *w_sgl_ptr[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    /* The r2_sgl_ptr is used to hold the pointers to the read sgs.
     */
    fbe_sg_element_t *r2_sgl_ptr[FBE_RAID_MAX_DISK_ARRAY_WIDTH];

    /* Check number of parity drives that are supported. R5=1, R6=2
     */
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_u16_t data_disks = fbe_raid_siots_get_width(siots_p) - parity_drives;

    /* If write log header required, need to populate the first sg_list element with the first buffer
     * before doing the rest, or the first sg_list element will cover the header and more
     */
    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_WRITE_LOG_HEADER_REQD))
    {
        fbe_u16_t num_sg_elements;
        fbe_u32_t journal_log_hdr_blocks;
        fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
        fbe_raid_geometry_get_journal_log_hdr_blocks(raid_geometry_p, &journal_log_hdr_blocks);

        fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);

        while (write_fruts_ptr)
        {
            current_sg_ptr = fbe_raid_fruts_return_sg_ptr(write_fruts_ptr);
            num_sg_elements = fbe_raid_sg_populate_with_memory(memory_info_p, 
                                                               current_sg_ptr, 
                                                               journal_log_hdr_blocks * FBE_BE_BYTES_PER_BLOCK);
            if (num_sg_elements != 1)
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "parity: write log header num_sg_elements %d != 1, siots_p = 0x%p\n",
                                            num_sg_elements, siots_p);
            }

            write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);
        } /* end while write_fruts_ptr*/

        /* Now set up the write fruts chain to skip the newly attached header */
        fbe_raid_fruts_chain_set_prepend_header(&siots_p->write_fruts_head, FALSE);
    }

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

    /* Stage 1:
     *
     * Assign a portion of the buffers we've obtained
     * from the BM to the S/G lists for the pre-read
     * operations. The size and locations of these
     * S/G lists are preserved for later use...
     */
    for (data_pos = 0; data_pos < data_disks; data_pos++)
    {
        current_write_fruts_ptr = NULL;
        /* Obtain the locations of the S/G lists used
         * for the parity calculations. Portions of
         * these lists are copies of the S/G lists
         * used for the pre-reads.
         */
        if (write_fruts_ptr != NULL &&
            write_fruts_ptr->position != siots_p->parity_pos)
        {
            fbe_u32_t current_data_pos;
            status = FBE_RAID_DATA_POSITION(write_fruts_ptr->position,
                                            siots_p->parity_pos,
                                            fbe_raid_siots_get_width(siots_p),
                                            parity_drives,
                                            &current_data_pos);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                            status, siots_p);
                return status;
            }
            if (current_data_pos == data_pos)
            {
                w_sgl_ptr[data_pos] = fbe_raid_fruts_return_sg_ptr(write_fruts_ptr);
                write_fruts_ptr->write_preread_desc.lba = write_fruts_ptr->lba;
                write_fruts_ptr->write_preread_desc.block_count = write_fruts_ptr->blocks;
                current_write_fruts_ptr = write_fruts_ptr;
                write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr); 
            }
        }

        if (read_fruts_ptr != NULL &&
            read_fruts_ptr->position != siots_p->parity_pos)
        {
            fbe_u32_t current_data_pos;
            status = FBE_RAID_DATA_POSITION(read_fruts_ptr->position,
                                        siots_p->parity_pos,
                                        fbe_raid_siots_get_width(siots_p),
                                        parity_drives,
                                        &current_data_pos);

            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                            status, siots_p);
                return status;
            }
            if (current_data_pos == data_pos)
            {
                fbe_sg_element_t *sgl_ptr;

                /* Sectors are to be read from this disk
                 * to backfill into the first stripe. We
                 * can update the S/G lists for parity
                 * calculations immediately.
                 */
                sgl_ptr = fbe_raid_fruts_return_sg_ptr(read_fruts_ptr);

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
                                         "parity: failed to populate sg for siots_p = 0x%p\n",
                                         siots_p);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                if (current_write_fruts_ptr != NULL &&
                    read_fruts_ptr->lba + read_fruts_ptr->blocks != current_write_fruts_ptr->lba)
                {
                    fbe_sg_element_with_offset_t sg_descriptor;
                    fbe_u16_t num_sg_elements;
                    fbe_u32_t offset = 0;
                    fbe_u32_t copy_byte_count = 0;
                    fbe_u32_t copy_block_count = 0;
                    fbe_lba_t read_end_lba = read_fruts_ptr->lba + read_fruts_ptr->blocks;


                    /* ******** <---- Read start
                     * *      *   
                     * *      * <---- 4K boundary - Aligned Write start 
                     * *      *            /\
                     * *      * <--- This middle area needs to be added to the write.
                     * *      *            \/
                     * ******** <---- Write start / Read End
                     * *      *
                     * *      * 
                     * The write lba at this point represents where the user data starts. 
                     * Our read ends after the new write data                  .
                     * We will need to write out these extra blocks to align to 4k. 
                     * Copy these extra blocks from the read to write sg. 
                     */
                    offset = (fbe_u32_t)(current_write_fruts_ptr->lba - read_fruts_ptr->lba);
                    copy_block_count = (fbe_u32_t)(read_end_lba - current_write_fruts_ptr->lba);
                    copy_byte_count = copy_block_count * FBE_BE_BYTES_PER_BLOCK;
                    current_write_fruts_ptr->write_preread_desc.lba = read_end_lba;
                    current_write_fruts_ptr->write_preread_desc.block_count -= copy_block_count;

                    fbe_sg_element_with_offset_init(&sg_descriptor, offset * FBE_BE_BYTES_PER_BLOCK, sgl_ptr, NULL);
                    fbe_raid_adjust_sg_desc(&sg_descriptor);
                    status = fbe_raid_sg_clip_sg_list(&sg_descriptor,
                                                      w_sgl_ptr[data_pos],
                                                      copy_byte_count,
                                                      &num_sg_elements);
                    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                             "parity: failed to clip sg list: status = 0x%x, siots_p = 0x%p, "
                                             "parent_fruts_ptr = 0x%p\n",
                                             status, siots_p, read_fruts_ptr);
                        return status;
                    }
                    w_sgl_ptr[data_pos] += num_sg_elements;
                }
                read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
            }
        }
        
    } /* end for all data disk positions */

    fbe_raid_siots_get_read2_fruts(siots_p, &read2_fruts_ptr);

    for (data_pos = 0; data_pos < data_disks; data_pos++)
    {
        /* S/G lists from the second pre-read can't be copied
         * immediately, so we preserve their locations and
         * sizes to allow copying later.
         */
        r2_sgl_ptr[data_pos] = 0;
        if (read2_fruts_ptr != NULL)
        {
            fbe_u32_t current_data_pos;
            status = FBE_RAID_DATA_POSITION(read2_fruts_ptr->position,
                                            siots_p->parity_pos,
                                            fbe_raid_siots_get_width(siots_p),
                                            parity_drives,
                                            &current_data_pos);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "parity: failed to convert position: status = 0x%x, siots_p = 0x%p, "
                                            "read2_fruts_ptr = 0x%p\n",
                                            status, siots_p, read2_fruts_ptr);
                return status;
            }

            if (current_data_pos == data_pos)
            {
                /* Sectors are to be read from this disk
                 * to backfill into the final stripe. We
                 * can't update the S/G lists for parity
                 * calculations yet, so we'll save the
                 * size and locations.
                 */
                r2_sgl_ptr[data_pos] = fbe_raid_fruts_return_sg_ptr(read2_fruts_ptr);

                if(!fbe_raid_get_checked_byte_count(read2_fruts_ptr->blocks, &dest_byte_count))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                         "%s: byte count exceeding 32bit limit \n",
                                         __FUNCTION__);
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                         "%s: byte count exceeding 32bit limit \n",
                                         __FUNCTION__);
                    return FBE_STATUS_GENERIC_FAILURE; 
                }
                sg_total = fbe_raid_sg_populate_with_memory(memory_info_p,
                                                            r2_sgl_ptr[data_pos],
                                                            dest_byte_count);
                if (sg_total == 0)
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                         "parity: failed to populate sg for siots_p = 0x%p\n",
                                         siots_p);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                read2_fruts_ptr = fbe_raid_fruts_get_next(read2_fruts_ptr);
            }
        }
    }

     /* Buffered operations need to allocate their own space for 
     * the data we will be writing.  Fill in the write sgs with 
     * the memory we allocated. 
     */
    if (fbe_raid_siots_is_buffered_op(siots_p))
    {
        fbe_u32_t data_pos;
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
            return status;
        }

        if (RAID_DBG_COND(siots_p->xfer_count > FBE_U32_MAX))
        {
            /* we do not expect the blocks to scatter to exceed 32bit limit here
             */
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: xfer count 0x%llx exceeding 32bit limit , siots_p = 0x%p\n",
                                        (unsigned long long)siots_p->xfer_count,
					siots_p);
            
            return FBE_STATUS_GENERIC_FAILURE;
        }
        status = fbe_raid_scatter_sg_with_memory(siots_p->start_lba,
                                                 (fbe_u32_t)siots_p->xfer_count,
                                                 data_pos,
                                                 fbe_raid_siots_get_width(siots_p) - parity_drives,
                                                 fbe_raid_siots_get_blocks_per_element(siots_p),
                                                 memory_info_p,
                                                 w_sgl_ptr);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: failed to scatter sg with memory: status = 0x%x, siots_p 0x%p "
                                        "data_pos = 0x%x, memory_info_p = 0x%p, w_sgl_ptr = 0x%p\n",
                                        status, siots_p, data_pos, memory_info_p, w_sgl_ptr);
            return status;
        }
    }
    else
    {
        fbe_u32_t data_pos;
        /* Setup the sg descriptor to point to the "data" we will use
         * for this operation. 
         */
        status = fbe_raid_siots_setup_sgd(siots_p, &host_sgd);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: failed to setup sg: status = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return status;
        }
        /*
         * Stage 2: Assign data to the S/G lists used for the partial
         *           product calculations. This is either buffered data
         *           from the FED or cache page locations in the write
         *           cache.
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

        status = fbe_raid_scatter_sg_to_bed(siots_p->start_lba,
                                           (fbe_u32_t)(siots_p->xfer_count),
                                           data_pos,
                                           fbe_raid_siots_get_width(siots_p) - parity_drives,
                                           fbe_raid_siots_get_blocks_per_element(siots_p),
                                           &host_sgd,
                                           w_sgl_ptr);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: failed to scatter memory to sg : status: 0x%x, siots_p = 0x%p, "
                                        "data_pos = 0x%x, w_sgl_ptr = 0x%p\n",
                                        status, siots_p, data_pos, w_sgl_ptr);
            return (status);
        }
    }

    

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);
    fbe_raid_siots_get_read2_fruts(siots_p, &read2_fruts_ptr);
    for (data_pos = 0; data_pos < data_disks; data_pos++)
    {
        current_write_fruts_ptr = NULL;
        /* Obtain the locations of the S/G lists used
         * for the parity calculations. Portions of
         * these lists are copies of the S/G lists
         * used for the pre-reads.
         */
        if (write_fruts_ptr != NULL &&
            write_fruts_ptr->position != siots_p->parity_pos)
        {
            fbe_u32_t current_data_pos;
            status = FBE_RAID_DATA_POSITION(write_fruts_ptr->position,
                                            siots_p->parity_pos,
                                            fbe_raid_siots_get_width(siots_p),
                                            parity_drives,
                                            &current_data_pos);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                            status, siots_p);
                return status;
            }
            if (current_data_pos == data_pos)
            {
                current_write_fruts_ptr = write_fruts_ptr;
                write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);
            }
        }

        if (read2_fruts_ptr != NULL &&
            read2_fruts_ptr->position != siots_p->parity_pos)
        {
            fbe_u32_t current_data_pos;
            status = FBE_RAID_DATA_POSITION(read2_fruts_ptr->position,
                                            siots_p->parity_pos,
                                            fbe_raid_siots_get_width(siots_p),
                                            parity_drives,
                                            &current_data_pos);

            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                            status, siots_p);
                return status;
            }
            if (current_data_pos == data_pos)
            {


                if (current_write_fruts_ptr != NULL &&
                    current_write_fruts_ptr->lba + current_write_fruts_ptr->blocks != read2_fruts_ptr->lba) 
                {
                    fbe_sg_element_with_offset_t sg_descriptor;
                    fbe_u16_t num_sg_elements;
                    fbe_u32_t offset = 0;
                    fbe_u32_t copy_block_count = 0;
                    fbe_u32_t copy_byte_count = 0;
                    fbe_lba_t aligned_end = (fbe_u32_t)(current_write_fruts_ptr->lba + current_write_fruts_ptr->blocks);
                    copy_block_count = (fbe_u32_t)(aligned_end - read2_fruts_ptr->lba);
                    // adjust the original block count;
                    current_write_fruts_ptr->write_preread_desc.block_count -= copy_block_count;
                    copy_byte_count = copy_block_count * FBE_BE_BYTES_PER_BLOCK;
                    /* ******** <---- Write start
                     * *      *
                     * *      *
                     * ******** <---- Write End / Read Start
                     * *      *            /\
                     * *      * <--- This middle area needs to be added to the write.
                     * *      *            \/
                     * ******** <---- 4K Boundary - Aligned Write End
                     * *      *
                     * ******** <---- Read end 
                     *  
                     * The write lba at this point represents where the user data starts 
                     * Our read ends after the new write data ends. 
                     * We will need to write out these extra blocks to align to 4k 
                     * Copy these extra blocks from the read to write sg.
                     */
                    /* The point to copy from is the end of the write I/O. 
                     * Calculate the offset into the read sg list from the beginning to where the write ends. 
                     */
                    fbe_sg_element_with_offset_init(&sg_descriptor, offset, r2_sgl_ptr[data_pos], NULL);
                    fbe_raid_adjust_sg_desc(&sg_descriptor);
                    status = fbe_raid_sg_clip_sg_list(&sg_descriptor,
                                                      w_sgl_ptr[data_pos],
                                                      copy_byte_count,
                                                      &num_sg_elements);
                    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                                 "parity: failed to clip sg list: status = 0x%x, siots_p = 0x%p, "
                                                 "parent_fruts_ptr = 0x%p\n",
                                                 status, siots_p, read2_fruts_ptr);
                        return status;
                    }

                    w_sgl_ptr[data_pos] += num_sg_elements;
                }
                read2_fruts_ptr = fbe_raid_fruts_get_next(read2_fruts_ptr);
            }
        }

    } /* end for all data disk positions */



    /*
     * Stage 4: Assign data to the S/G list used for the parity pre-read.
     *          The same sgs and buffers are used for the parity
     *          pre-read AND the parity write.
     *          Note how the read fruit gets the same sg_id as the write.
     */

    /* Save the original size for comparison
     */
    write_fruts_ptr->write_preread_desc.lba = siots_p->parity_start;
    write_fruts_ptr->write_preread_desc.block_count = siots_p->parity_count;
    status = fbe_parity_rcw_setup_parity_sgs(siots_p,
                                             memory_info_p,
                                             read_fruts_ptr,
                                             write_fruts_ptr,
                                             &read2_mem_p,
                                             &read2_mem_left);

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        /*Split trace to two lines*/
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "parity:fail plant parity sgs:stat=0x%x,"
                                    "siots=0x%p>\n",
                                    status, siots_p);
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "mem_info=0x%p,read_fruts=0x%p,write_fruts=0x%p\n",
                                    memory_info_p, read_fruts_ptr, write_fruts_ptr);
        return (status);
    }

    /* If R6, handles stage 4 for diagonal parity.
     */                          
    if (parity_drives == 2)
    {
        fbe_sg_element_t *sgl_ptr;
        fbe_u16_t sg_total = 0;
        fbe_u32_t dest_byte_count =0;

        write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);
        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
        if (RAID_COND(write_fruts_ptr == NULL))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: rcw: write fruts is null\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        write_fruts_ptr->write_preread_desc.lba = siots_p->parity_start;
        write_fruts_ptr->write_preread_desc.block_count = siots_p->parity_count;

        /* A preread for the diagonal parity was necessary to
         * align it's write
         */
        if (read_fruts_ptr != NULL && read_fruts_ptr->blocks > 0)
        {
            status = fbe_parity_rcw_setup_parity_sgs(siots_p,
                                                     memory_info_p,
                                                     read_fruts_ptr,
                                                     write_fruts_ptr,
                                                     &read2_mem_p,
                                                     &read2_mem_left);

            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                /*Split trace to two lines*/
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "parity:fail plant parity sgs:stat=0x%x,"
                                            "siots=0x%p>\n",
                                            status, siots_p);
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "mem_info=0x%p,read_fruts=0x%p,write_fruts=0x%p\n",
                                            memory_info_p, read_fruts_ptr, write_fruts_ptr);
                return (status);
            }
        } 
        else 
        {
            if (RAID_COND(!fbe_raid_siots_is_parity_pos(siots_p, write_fruts_ptr->position)))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: rcw: write fruts pos %d is not a parity position %d\n",
                                     write_fruts_ptr->position, siots_p->parity_pos);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            if(!fbe_raid_get_checked_byte_count(write_fruts_ptr->blocks, &dest_byte_count))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "%s: byte count exceeding 32bit limit \n",
                                     __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE; 
            } 
             
            sgl_ptr = fbe_raid_fruts_return_sg_ptr(write_fruts_ptr);

            /* Now plant the second parity position
             */
            sg_total = fbe_raid_sg_populate_with_memory(memory_info_p,
                                                        sgl_ptr,
                                                        dest_byte_count);
            if (sg_total == 0)
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                         "parity: failed to populate sg for siots_p = 0x%p\n",
                                         siots_p);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    } /* end if second parity position */

    if (RAID_DBG_ENABLED)
    {
        /* Make sure our sgs are sane.
         */
        status = fbe_parity_rcw_validate_sgs(siots_p);
    }
    return status;
}
/*****************************************
 * fbe_parity_rcw_setup_sgs()
 *****************************************/

/****************************************************************
 * fbe_parity_rcw_setup_xor_vectors()
 ****************************************************************
 * @brief
 *  Setup the xor command's vectors for this rcw operation.
 *
 * @param siots_p - The siots to use when creating the xor command.
 * @param xor_write_p - The xor command to setup vectors in.
 *
 * @return fbe_status_t
 *
 * @author
 *  1/21/2014 - Created. Deanna Heng
 *
 ****************************************************************/
/*


                         *********************  -------
                         * write_offset      *     |   |- Aligned read  
     parity start----|-- *********************     |---
                     |   *                   *     |
                     |   *                   *     |
                     |   *  original write   *     |  aligned write
                     |   *                   *     | 
                     |   *                   *     |
     parity count   -|   *********************     |---
                     |   * write end offset  *     |   |- Aligned read2
                     |   ********************* --------


*/
 

fbe_status_t fbe_parity_rcw_setup_xor_vectors(fbe_raid_siots_t *const siots_p, 
                                              fbe_xor_rcw_write_t * const xor_write_p)
{
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_raid_fruts_t *read2_fruts_ptr = NULL;
    fbe_raid_fruts_t *current_write_fruts_ptr = NULL;
    fbe_u32_t parity_disks;
    fbe_u32_t fru_index = 0;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u32_t width;
    fbe_u8_t *positions = siots_p->geo.position;
    fbe_u32_t alignment = fbe_raid_geometry_get_default_optimal_block_size(raid_geometry_p);

    fbe_raid_geometry_get_width(raid_geometry_p, &width);
    fbe_raid_geometry_get_num_parity(raid_geometry_p, &parity_disks);
    
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    fbe_raid_siots_get_read2_fruts(siots_p, &read2_fruts_ptr);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);

    for (fru_index = 0; fru_index < fbe_raid_siots_get_width(siots_p); fru_index++)
    {
        fbe_u32_t data_pos;
        fbe_u32_t position = -1;
        current_write_fruts_ptr = NULL;
        xor_write_p->fru[fru_index].read_count = 0;
        xor_write_p->fru[fru_index].write_count = 0;
        xor_write_p->fru[fru_index].read2_count = 0;
        xor_write_p->fru[fru_index].write_offset = 0;
        xor_write_p->fru[fru_index].write_end_offset = 0;


        if (write_fruts_ptr != NULL && write_fruts_ptr->position == positions[fru_index])
        {
            fbe_lba_t aligned_end_lba = write_fruts_ptr->lba + write_fruts_ptr->blocks;
            fbe_lba_t original_end_lba = write_fruts_ptr->write_preread_desc.lba + write_fruts_ptr->write_preread_desc.block_count;

            xor_write_p->fru[fru_index].write_sg = fbe_raid_fruts_return_sg_ptr(write_fruts_ptr);
            xor_write_p->fru[fru_index].write_count = write_fruts_ptr->write_preread_desc.block_count;
            position = write_fruts_ptr->position;
            xor_write_p->fru[fru_index].write_offset = (fbe_u32_t)(write_fruts_ptr->write_preread_desc.lba - 
                                                                   write_fruts_ptr->lba);
            xor_write_p->fru[fru_index].write_end_offset = (fbe_u32_t)(aligned_end_lba - original_end_lba);
            if ((xor_write_p->fru[fru_index].write_offset >= alignment)       ||
                ((fbe_s32_t)xor_write_p->fru[fru_index].write_offset < 0)     ||
                (xor_write_p->fru[fru_index].write_end_offset >= alignment)   ||
                ((fbe_s32_t)xor_write_p->fru[fru_index].write_end_offset < 0)    ) {
                fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: invalid offset: write_offset=0x%x, write_end_offset=0x%x, siots_p=0x%p\n", 
                             xor_write_p->fru[fru_index].write_offset,
                             xor_write_p->fru[fru_index].write_end_offset, siots_p);
            }
            current_write_fruts_ptr = write_fruts_ptr;
            write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);
        }
        if (read_fruts_ptr != NULL && read_fruts_ptr->position == positions[fru_index])
        {
            xor_write_p->fru[fru_index].read_sg = fbe_raid_fruts_return_sg_ptr(read_fruts_ptr);
            xor_write_p->fru[fru_index].read_count = read_fruts_ptr->blocks;
            position = read_fruts_ptr->position;
            /* Do not xor the aligned prereads. 
             */
            if (current_write_fruts_ptr != NULL &&
                xor_write_p->fru[fru_index].write_offset != 0 &&
                current_write_fruts_ptr->lba < siots_p->parity_start)
            {
                /* write offset and read count of 0 indicates that we need to 
                 * check the checksum for the preread we will write out
                 */
                xor_write_p->fru[fru_index].read_count = 0;
            }
            else 
            {
                /* Adjust the sg_descriptor since the checksum will already be 
                 * checked on existing prereads
                 */
                fbe_sg_element_with_offset_t sg_descriptor;
                fbe_sg_element_with_offset_init(&sg_descriptor, 
                                                xor_write_p->fru[fru_index].write_offset * FBE_BE_BYTES_PER_BLOCK, 
                                                xor_write_p->fru[fru_index].write_sg, NULL);
                fbe_raid_adjust_sg_desc(&sg_descriptor);
                xor_write_p->fru[fru_index].write_sg = sg_descriptor.sg_element;
                xor_write_p->fru[fru_index].write_offset = sg_descriptor.offset / FBE_BE_BYTES_PER_BLOCK;
            }
            read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr); 
        }
        if (read2_fruts_ptr != NULL && read2_fruts_ptr->position == positions[fru_index])
        {
            xor_write_p->fru[fru_index].read2_sg = fbe_raid_fruts_return_sg_ptr(read2_fruts_ptr);
            xor_write_p->fru[fru_index].read2_count = read2_fruts_ptr->blocks;
            position = read2_fruts_ptr->position;

            /* Do not xor the aligned prereads. 
             */
            if (xor_write_p->fru[fru_index].write_end_offset != 0 &&
                current_write_fruts_ptr != NULL &&
                current_write_fruts_ptr->lba + current_write_fruts_ptr->blocks > siots_p->parity_start + siots_p->parity_count)
            {
                /* write end offset > 0 and read2 count of 0 indicates that we need to 
                 * check the checksum for the preread we will write out
                 */
                xor_write_p->fru[fru_index].read2_count = 0;
            } 
            else 
            {
                /* Adjust the sg_descriptor since the checksum will already be 
                 * checked on existing prereads
                 */
            }

            read2_fruts_ptr = fbe_raid_fruts_get_next(read2_fruts_ptr);   
        }

        /* Determine the data position of this fru and save it 
         * for possible later use in RAID 6 algorithms.
         */
        status = 
        FBE_RAID_EXTENT_POSITION(position,
                                 siots_p->parity_pos,
                                 fbe_raid_siots_get_width(siots_p),
                                 parity_disks,
                                 &data_pos);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: failed to convert position: pos = 0x%x, status = 0x%x, siots_p = 0x%p\n",
                                        position, status, siots_p);
            return status;
        }

        xor_write_p->fru[fru_index].seed = siots_p->parity_start;
        xor_write_p->fru[fru_index].count = siots_p->parity_count;
        xor_write_p->fru[fru_index].position_mask = (1 << position);
        xor_write_p->fru[fru_index].data_position = data_pos;

    }

    return FBE_STATUS_OK;
}
/******************************************
 * fbe_parity_rcw_setup_xor_vectors()
 ******************************************/

/****************************************************************
 * fbe_parity_rcw_setup_xor()
 ****************************************************************
 * @brief
 *  This function sets up the xor command for an rcw operation.
 *
 * @param siots_p - The fbe_raid_siots_t for this operation.
 * @param xor_write_p - The fbe_xor_command_t to setup.
 *
 * @return fbe_status_t
 *
 * @author
 *  1/21/2014 - Created. Deanna Heng
 *
 ****************************************************************/

fbe_status_t fbe_parity_rcw_setup_xor(fbe_raid_siots_t *const siots_p, 
                                      fbe_xor_rcw_write_t * const xor_write_p)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* Get number of parity that are supported. R5=1, R6=2 
     */
    fbe_u32_t parity_disks = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_u16_t data_disks = fbe_raid_siots_get_width(siots_p) - parity_disks;

    /* First init the command.
     */
    xor_write_p->status = FBE_XOR_STATUS_INVALID;

    /* Setup all the vectors in the xor command.
     */
    status = fbe_parity_rcw_setup_xor_vectors(siots_p, xor_write_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity:fail to setup xor vec:stat=0x%x,siots=0x%p,xor_wt=0x%p\n", 
                             status, siots_p, xor_write_p);

        return status;
    }

    /* Default option flags is FBE_XOR_OPTION_CHK_CRC.  We always check the
     * checksum of data read from disk for a write operation.
     */
    xor_write_p->option = FBE_XOR_OPTION_CHK_CRC;

    /* Degraded operations allow previously invalidated data
     */
    if ( siots_p->algorithm == R5_DEG_RCW )
    {
        /* Degraded RCW allows invalid sectors in pre-read data,
         * since we already performed a reconstruct.
         */
        xor_write_p->option |= FBE_XOR_OPTION_ALLOW_INVALIDS;
    }

    /* Setup the rest of the command including the parity and data disks and
     * offset.
     */
    xor_write_p->data_disks = data_disks;
    xor_write_p->parity_disks = parity_disks;
    xor_write_p->array_width = fbe_raid_siots_get_width(siots_p);
    xor_write_p->eboard_p = fbe_raid_siots_get_eboard(siots_p);
    xor_write_p->eregions_p = &siots_p->vcts_ptr->error_regions;
    xor_write_p->offset = fbe_raid_siots_get_eboard_offset(siots_p);
    return FBE_STATUS_OK;
}
/******************************************
 * fbe_parity_rcw_setup_xor()
 ******************************************/
/***************************************************************************
 *      fbe_parity_rcw_xor()
 ***************************************************************************
 * @brief
 *   This routine sets up S/G list pointers to interface with
 *   the XOR routine for parity operation for an rcw write.
 *
 * @param siots_p - pointer to fbe_raid_siots_t
 *
 * @return fbe_status_t
 *
 * @author
 *  1/21/2014 - Created. Deanna Heng
 *
 **************************************************************************/

fbe_status_t fbe_parity_rcw_xor(fbe_raid_siots_t * const siots_p)
{
    /* This is the command we will submit to xor.
     */
    fbe_xor_rcw_write_t xor_write;

    /* Status of starting this operation.
     */
    fbe_status_t status;

    /* Setup the xor command completely.
     */
    status = fbe_parity_rcw_setup_xor(siots_p, &xor_write);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: failed to setup xor command: status = 0x%x,  siots_p = 0x%p\n", 
                             status, siots_p);

        return status;
    }

    /* Send xor command now. 
     */ 
    status = fbe_xor_lib_parity_rcw_write(&xor_write);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: failed to send xor command: status = 0x%x, siots_p = 0x%p\n", 
                             status, siots_p);

        return status;
    }

    /* Save status.
     */
    fbe_raid_siots_set_xor_command_status(siots_p, xor_write.status);

    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_parity_rcw_xor
 *****************************************/

/****************************************************************
 *  fbe_parity_rcw_degraded_count()
 ****************************************************************
 * @brief
 *  This function counts the number of degraded positions in
 *  a rcw I/O.  
 *
 * @param siots_p - current siots
 *
 * @return
 *  fbe_bool_t FBE_TRUE - Preread degraded.
 *      FBE_FALSE - Preread is not degraded.
 *
 * ASSUMPTIONS:
 *  We assume this only use for Raid 6.
 *
 * @author
 *  1/21/2014 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_u16_t fbe_parity_rcw_degraded_count(fbe_raid_siots_t *const siots_p)
{
    fbe_raid_fruts_t *read_fruts_ptr = NULL;   /* Ptr to current fruts in read chain. */
    
    fbe_u32_t first_dead_pos = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS);
    fbe_u32_t second_dead_pos = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_SECOND_DEAD_POS);
    fbe_u16_t degraded_count = 0;
    
    if (first_dead_pos == FBE_RAID_INVALID_DEAD_POS)
    {
        /* There are no degraded positions and thus nothing for
         * us to do, so we will return.
         */
        return degraded_count;
    }
    
    /* Loop over siots_p->read_fruts_ptr and siots_p->reads_fruts_ptr
     * fruts in the chain and if any hit the dead position, then return FBE_TRUE
     * otherwise return FBE_FALSE.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    while (read_fruts_ptr)
    {   
        if ((read_fruts_ptr->position == first_dead_pos) ||
            (read_fruts_ptr->position == second_dead_pos))
        {
            /* Found the dead position in the preread.
             */
            degraded_count++;
        }
        
        read_fruts_ptr = fbe_raid_fruts_get_next( read_fruts_ptr );
    }
    
    return degraded_count;
}
/*****************************************
 * end fbe_parity_rcw_degraded_count
 *****************************************/
/*!**************************************************************
 * fbe_parity_rcw_handle_retry_error()
 ****************************************************************
 * @brief
 *  Handle a retry error status.
 *
 * @param siots_p - Current I/O.
 * @param eboard_p - This is the structure to save the results in.
 * @param read1_status - This is the first read status we need to handle.
 * @param read2_status - This is the 2nd read status we need to handle.
 *
 * @return fbe_status_t FBE_STATUS_OK if usurpers were sent
 *                      FBE_STATUS_GENERIC_FAILURE if no
 *                      timed out retries found in the chain
 *                      and thus no usurpers need to be sent.
 * @author
 *  1/21/2014 - Created. Deanna Heng
 *
 ****************************************************************/

fbe_status_t fbe_parity_rcw_handle_retry_error(fbe_raid_siots_t *siots_p,
                                               fbe_raid_fru_eboard_t * eboard_p,
                                               fbe_raid_fru_error_status_t read1_status, 
                                               fbe_raid_fru_error_status_t read2_status)
{   
    fbe_status_t status;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_raid_fruts_t *read2_fruts_p = NULL;

    /* Get totals for errored or dead pre-read drives.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_siots_get_read2_fruts(siots_p, &read2_fruts_p);

    fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_INFO,
                                 "parity:  mr3 r1_status: %d r2_status: %d count: 0x%x bitmap: 0x%x\n",
                                 read1_status, read2_status, eboard_p->retry_err_count, eboard_p->retry_err_bitmap);

    /* Send out retries.  We will stop retrying if we get aborted or 
     * our timeout expires or if an edge goes away. 
     */
    if (read1_status == FBE_RAID_FRU_ERROR_STATUS_RETRY)
    {
        fbe_raid_position_bitmask_t retry_bitmap;
         /* We might have gone degraded and have a retry.
          * In this case it is necessary to mark the degraded positions as nops since 
          * 1) we already marked the paged for these 
          * 2) the next time we run through this state we do not want to consider these failures 
          *    (and wait for continue) since we already got the continue. 
          */
        fbe_raid_fruts_set_degraded_nop(siots_p, read_fruts_p);
        fbe_raid_fruts_set_degraded_nop(siots_p, read2_fruts_p);

        /* For the case where we have read1 and read2 retries, we need to re-calculate 
         * the total counts, since our eboard only has one count per position. 
         */
        fbe_raid_fruts_get_chain_bitmap(read_fruts_p, &retry_bitmap);
        retry_bitmap &= eboard_p->retry_err_bitmap;
        eboard_p->retry_err_count = fbe_raid_get_bitcount(retry_bitmap);

        fbe_raid_fruts_get_chain_bitmap(read2_fruts_p, &retry_bitmap);
        retry_bitmap &= eboard_p->retry_err_bitmap;
        eboard_p->retry_err_count += fbe_raid_get_bitcount(retry_bitmap);

        fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_INFO, "parity mr3 retry %d reads", eboard_p->retry_err_count);

        status = fbe_raid_siots_retry_fruts_chain(siots_p, eboard_p, read_fruts_p);

        /* The eboard has the combined counts for read1 and read2.
         * Set to 0 so that we do not bump up the wait count further if we are sending 
         * out read2. 
         */
        eboard_p->retry_err_count = 0;
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity rcw: read retry %d errors (bitmap 0x%x) status: %d unexpected\n", 
                                                eboard_p->retry_err_count, eboard_p->retry_err_bitmap, status);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    if (read2_status == FBE_RAID_FRU_ERROR_STATUS_RETRY)
    {
        fbe_raid_position_bitmask_t active_bitmap = 0;
         /* We might have gone degraded and have a retry.
          * In this case it is necessary to mark the degraded positions as nops since 
          * 1) we already marked the paged for these 
          * 2) the next time we run through this state we do not want to consider these failures 
          *    (and wait for continue) since we already got the continue. 
          */
        fbe_raid_fruts_set_degraded_nop(siots_p, read_fruts_p);
        fbe_raid_fruts_set_degraded_nop(siots_p, read2_fruts_p);

        /* If there is anything active in this chain that needs a retry, then kick off 
         * the retry now. 
         */
        fbe_raid_fruts_get_chain_bitmap(read2_fruts_p, &active_bitmap);
        if (active_bitmap & eboard_p->retry_err_bitmap)
        {
            status = fbe_raid_siots_retry_fruts_chain(siots_p, eboard_p, read2_fruts_p);
        }
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_rcw_handle_retry_error()
 **************************************/

/*****************************************
 * end fbe_parity_rcw_util.c
 *****************************************/
