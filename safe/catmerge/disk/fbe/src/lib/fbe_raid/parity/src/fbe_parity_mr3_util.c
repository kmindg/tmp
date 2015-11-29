/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_parity_mr3_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions that are called by the r5 MR3 state machine.
 *
 * @notes
 *
 * @author
 *  09/22/1999 Created. Rob Foley.
 *
 ***************************************************************************/

/*************************
 ** INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_parity_io_private.h"

/********************************
 * static FUNCTIONS
 ********************************/
fbe_block_count_t fbe_parity_mr3_count_buffers(fbe_raid_siots_t *siots_p);
fbe_status_t fbe_parity_mr3_validate_sgs(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_parity_mr3_setup_xor(fbe_raid_siots_t *const siots_p, 
                                      fbe_xor_mr3_write_t * const xor_write_p);
fbe_status_t fbe_parity_mr3_setup_xor_vectors(fbe_raid_siots_t *const siots_p, 
                                              fbe_xor_mr3_write_t * const xor_write_p);
fbe_status_t fbe_parity_mr3_setup_sgs(fbe_raid_siots_t * siots_p,
                                      fbe_raid_memory_info_t *memory_info_p);
fbe_status_t fbe_parity_mr3_setup_fruts(fbe_raid_siots_t * siots_p, 
                                        fbe_raid_fru_info_t *read_p,
                                        fbe_raid_fru_info_t *read2_p,
                                        fbe_raid_fru_info_t *write_p,
                                        fbe_raid_memory_info_t *memory_info_p);
fbe_status_t fbe_parity_mr3_validate_fruts_sgs(fbe_raid_siots_t *siots_p);

/*!**************************************************************************
 * fbe_parity_mr3_calculate_memory_size()
 ****************************************************************************
 * @brief  This function calculates the total memory usage of the siots for
 *         this state machine.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts
 * @param read_info_p - Array of read information.
 * @param read2_info_p - Array of read2 information.
 * @param write_info_p - Array of write information.
 * 
 * @return None.
 *
 **************************************************************************/

fbe_status_t fbe_parity_mr3_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                  fbe_raid_fru_info_t *read_info_p,
                                                  fbe_raid_fru_info_t *read2_info_p,
                                                  fbe_raid_fru_info_t *write_info_p)
{
    fbe_u16_t num_sgs[FBE_RAID_MAX_SG_TYPE];
    fbe_u32_t num_fruts = 0;
    fbe_block_count_t read_blocks = 0;
    fbe_u32_t read_fruts = 0;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_zero_memory(&num_sgs[0], (sizeof(fbe_u16_t) * FBE_RAID_MAX_SG_TYPE));

    /* Then calculate how many buffers are needed.
     */
    status = fbe_parity_mr3_get_fruts_info(siots_p, read_info_p, read2_info_p, write_info_p, &read_blocks, &read_fruts);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    { 
        /*Split trace to two lines*/
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity:fail get fru info:siots=0x%p,stat=0x%x>\n",
                             siots_p, status);
		fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "read_info=0x%p,read2_info=0x%p,write_info=0x%p<\n",
                             read_info_p, read2_info_p, write_info_p);
        return status; 
    }

    /* First start off with 1 fruts for every data position we are writing 
     * and one for any prereads. 
     */
    num_fruts = siots_p->drive_operations + read_fruts;
        
    siots_p->total_blocks_to_allocate = fbe_parity_mr3_count_buffers(siots_p);

    /* Increase the total blocks allocated by 1 per write position for write log headers if required
     */
    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_WRITE_LOG_HEADER_REQD))
    {
        fbe_u32_t journal_log_hdr_blocks;
        fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

        fbe_raid_geometry_get_journal_log_hdr_blocks(raid_geometry_p, &journal_log_hdr_blocks);
        siots_p->total_blocks_to_allocate += fbe_raid_siots_get_width(siots_p) * journal_log_hdr_blocks;
    }
        
    fbe_raid_siots_set_optimal_page_size(siots_p,
                                         num_fruts, siots_p->total_blocks_to_allocate);

    /* Next calculate the number of sgs of each type.
     */
    status = fbe_parity_mr3_calculate_num_sgs(siots_p, &num_sgs[0], 
                                              read_info_p, read2_info_p, write_info_p,
                                              FBE_TRUE);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    { 
        /*Split trace to two lines*/
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity:fail cal num of sgs: status = 0x%x, siots_p = 0x%p>\n",
                             status, siots_p);
		fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "read_info=0x%p,read2_info=0x%p,write_info=0x%p<\n",
                             read_info_p, read2_info_p, write_info_p);
        return status; 
    }

    /* Take the information we calculated above and determine how this translates 
     * into a number of pages. 
     */
    status = fbe_raid_siots_calculate_num_pages(siots_p, num_fruts, &num_sgs[0],
                                                FBE_TRUE /* In-case recovery verify is required*/);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    { 
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: failed to calculate num of pages: siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return status;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_mr3_calculate_memory_size()
 **************************************/
/*!**************************************************************
 * fbe_parity_mr3_setup_resources()
 ****************************************************************
 * @brief
 *  Setup the newly allocated resources.
 *
 * @param siots_p - current I/O.
 * @param read_info_p - Array of read information.
 * @param read2_info_p - Array of read2 information.
 * @param write_info_p - Array of write information.
 *
 * @return fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_parity_mr3_setup_resources(fbe_raid_siots_t * siots_p, 
                                            fbe_raid_fru_info_t *read_p,
                                            fbe_raid_fru_info_t *read2_p,
                                            fbe_raid_fru_info_t *write_p)
{
    fbe_raid_memory_info_t memory_info;
    fbe_raid_memory_info_t data_memory_info;
    fbe_status_t status;
    fbe_raid_fru_info_t *current_read_p = NULL;
    fbe_raid_fru_info_t *current_read2_p = NULL;

    /* Initialize our memory request information
     */
    status = fbe_raid_siots_init_control_mem_info(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to ctrl init memory info with status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return (status); 
    }

    status = fbe_raid_siots_init_data_mem_info(siots_p, &data_memory_info);

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity:  failed to init data for siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return status;
    }

    /* Set up the FRU_TS.
     */
    status = fbe_parity_mr3_setup_fruts(siots_p, read_p, read2_p, write_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity:mr3 fail to setup frut with stat=0x%x,siots=0x%p\n",
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
                             "parity: failed to init vrts with status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);                                
        return(status);
    }

    if (read_p->blocks > 0)
    {
        current_read_p = read_p;
    }
    if (read2_p->blocks > 0)
    {
        current_read2_p = read2_p;
    }
    /* Plant the allocated sgs into the locations calculated above.
     * Only pass in read or read2 if we actually allocated something for these. 
     */
    status = fbe_raid_fru_info_array_allocate_sgs(siots_p, &memory_info,
                                                  current_read_p, current_read2_p, write_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to allocate sgs for siots_p = 0x%p with status  = 0x%x; "
                             "current_read_p = 0x%p, current_read2_p = 0x%p, write_p = 0x%p\n",
                             siots_p, status, current_read_p, current_read2_p, write_p);
        return status;
    }

    /* Assign buffers to sgs.
     * We reset the current ptr to the beginning since we always 
     * allocate buffers starting at the beginning of the memory. 
     */
    status = fbe_parity_mr3_setup_sgs(siots_p, &data_memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to setup sgs for siots_p = 0x%p with status 0x%x\n",
                             siots_p, status);
        return status;
    }

    /* Make sure the buffer memory we just used does not overlap into 
     * the area used for other resources. 
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_parity_mr3_setup_resources()
 ******************************************/

/*!***************************************************************
 * fbe_parity_mr3_get_fruts_info()
 ****************************************************************
 * @brief
 *  This function initializes the fru info.
 *
 * @param siots_p - Current sub request.
 * @param read_info_p - Array of read information.
 * @param read2_info_p - Array of read2 information.
 * @param read_blocks_p - number of blocks being read.
 * @param read_fruts_count_p - count of read fruts needed.
 *
 * @return fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_parity_mr3_get_fruts_info(fbe_raid_siots_t * siots_p,
                                           fbe_raid_fru_info_t *read_p,
                                           fbe_raid_fru_info_t *read2_p,
                                           fbe_raid_fru_info_t *write_p,
                                           fbe_block_count_t *read_blocks_p,
                                           fbe_u32_t *read_fruts_count_p)
{
    fbe_lba_t write_lba;
    fbe_block_count_t write_blocks;
    fbe_lba_t read_lba;
    fbe_block_count_t read_blocks;
    fbe_u32_t  array_pos;         /* Actual array position of access */
    fbe_u16_t data_pos;          /* data position in array */
    fbe_lba_t parity_range_offset;  /* Distance from parity boundary to parity_start */
    fbe_block_count_t r1_blkcnt[FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1];    /* block counts for pre-reads, */
    fbe_block_count_t r2_blkcnt[FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1];    /* distributed across the FRUs */
    fbe_u8_t *position = siots_p->geo.position;
    fbe_block_count_t ret_read_blocks = 0;
    fbe_u32_t ret_read_fruts_count = 0;
    fbe_status_t status = FBE_STATUS_OK;
    /* Check number of parity drives that are supported. R5=1, R6=2
     */
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_lba_t logical_parity_start = siots_p->geo.logical_parity_start;
    fbe_u32_t width = fbe_raid_siots_get_width(siots_p);

    fbe_zero_memory(read_p, sizeof(fbe_raid_fru_info_t) * (width - parity_drives));
    fbe_zero_memory(read2_p, sizeof(fbe_raid_fru_info_t) * (width - parity_drives));

    /* Leverage MR3 calculations to describe what
     * portions of stripe(s) are *not* being accessed.
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
                             "siots=0x%p,r1_blkcnt=0x%p,r2_blkcnt=0x%p<\n",
                             siots_p, r1_blkcnt, r2_blkcnt);
        return status;
    }

    /* Calculate the total offset from our parity range.
     */
    parity_range_offset = fbe_raid_siots_get_parity_start_offset(siots_p);

    /* Loop through all data positions, setting up
     * each fru info.
     */
    for (data_pos = 0; data_pos < (width - parity_drives); data_pos++)
    {
        /* Validate array position. */
        if (data_pos >= (width - parity_drives))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: %s failed as data_pos (%d) >= (width (%d) - parity_drives (%d)) : siots_p = 0x%p\n", 
                                 __FUNCTION__,
                                 data_pos, width, parity_drives,
                                 siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* The pre-reads should NOT exceed the parity range.  */
        if ((fbe_u32_t)(r1_blkcnt[data_pos] + r2_blkcnt[data_pos]) > siots_p->parity_count)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: %s failed for siots_p = 0x%p:"
                                 "r1_blkcnt[data_pos] (0x%llx) + r2_blkcnt[data_pos] (0x%llx)) > siots_p->parity_count (0x%llx)\n", 
                                 __FUNCTION__, siots_p,
				 (unsigned long long)r1_blkcnt[data_pos],
				 (unsigned long long)r2_blkcnt[data_pos],
				 (unsigned long long)siots_p->parity_count);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        array_pos = position[data_pos];
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

        write_blocks = siots_p->parity_count;
        write_lba = logical_parity_start + parity_range_offset;

        /* Should not be processing parity position here */
        if (position[data_pos] == siots_p->parity_pos)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: position %d equal to parity\n", position[data_pos]);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Validate the block count. */
        if (write_blocks == 0)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: write blocks is zero\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Init the write fruts.
         */
        write_p->lba = write_lba;
        write_p->blocks = write_blocks;
        write_p->position = array_pos;
        write_p++;
    }

    /* Init the fruts for pre-read/write
     * of parity in the final fbe_raid_fruts_t.
     */
    write_lba = logical_parity_start + parity_range_offset;
    write_blocks = siots_p->parity_count;
    array_pos = position[width - parity_drives];
    write_p->lba = write_lba;
    write_p->blocks = write_blocks;
    write_p->position = array_pos;

    if (parity_drives == 2)
    {
        /* For RAID 6 initialize the 2nd parity drive.
         */
        read_p++;
        write_p++;

        write_lba = logical_parity_start + parity_range_offset;
        array_pos = position[fbe_raid_siots_get_width(siots_p) - 1];
        write_p->lba = write_lba;
        write_p->blocks = write_blocks;
        write_p->position = array_pos;

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
 * end fbe_parity_mr3_get_fruts_info() 
 **************************************/
/***************************************************************************
 * fbe_parity_mr3_setup_fruts()
 ***************************************************************************
 * @brief 
 *  This function initializes the fruts structures for a
 *  RAID 5/RAID 3 MR3.
 *
 * @param siots_p - pointer to SIOTS data
 * @param read_info_p - Array of read information.
 * @param read2_info_p - Array of read2 information.
 * @param write_info_p - Array of write information
 * @param memory_info_p - Pointer to memory request information
 *
 * @return fbe_status_t
 *
 **************************************************************************/

fbe_status_t fbe_parity_mr3_setup_fruts(fbe_raid_siots_t * siots_p, 
                                        fbe_raid_fru_info_t *read_p,
                                        fbe_raid_fru_info_t *read2_p,
                                        fbe_raid_fru_info_t *write_p,
                                        fbe_raid_memory_info_t *memory_info_p)
{
    fbe_u16_t data_pos;           /* data position in array */
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u16_t data_disks;
    fbe_lba_t capacity;
    fbe_u32_t width = 0;

    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);
    fbe_raid_siots_get_capacity(siots_p, &capacity);

    /* Simply iterate over the width. 
     * Always setup a write. 
     * Setup a read if a read is present. 
     */
    width = fbe_raid_siots_get_width(siots_p);
    if (RAID_COND(width == 0))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_mr3_setup_fruts" ,
                             "parity_mr3_setup_fruts got unexpect width of RG. width = 0x%x\n",
                             width);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (data_pos = 0;
         data_pos < width;
         data_pos++)
    {
        if ((read_p->blocks > 0) && (read_p->position == write_p->position))
        {
            if (RAID_COND((read_p->lba + read_p->blocks) > capacity))
            {
                fbe_raid_siots_trace(siots_p,
                                     FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_mr3_setup_fruts" ,
                                     "parity_mr3_setup_fruts:(read->lba 0x%llx+read->blocks 0x%llx)>capacity 0x%llx:siots=0x%p\n",
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
                                     FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_mr3_setup_fruts" ,
                                     "parity:fail setup: mem_info=0x%p,"
                                     "read 0x%p,siots=0x%p,stat=0x%x\n",
                                     memory_info_p, read_p, siots_p, status);
                return status;
            }
            read_p++;
        }
        if ((read2_p->blocks > 0) && (read2_p->position == write_p->position))
        {
            if (RAID_COND((read2_p->lba + read2_p->blocks) > capacity))
            {
                fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_mr3_setup_fruts" ,
                                     "parity_mr3_setup_fruts fru info 0x%p lba %llx+blocks 0x%llx is beyond capacity %llx\n",
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
                                     FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_mr3_setup_fruts" ,
                                     "parity:fail to setup fruts:read2=0x%p,siots=0x%p,"
                                     "data pos=0x%x, stat=0x%x\n",
                                     read2_p, siots_p, data_pos, status);
                return status;
            }
            read2_p++;
        }
        if (RAID_COND((write_p->lba + write_p->blocks) > capacity))
        {
            fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_mr3_setup_fruts" ,
                                 "parity_mr3_setup_fruts, fru info 0x%p lba %llx+blocks 0x%llx is beyond capacity %llx\n",
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
                                 FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_mr3_setup_fruts" ,
                                 "parity:fail setup fru: mem_info=0x%p,write=0x%p,"
                                 "siots=0x%p,data pos=0x%x,stat=0x%x\n",
                                 memory_info_p, write_p, siots_p, data_pos, status);
            return status;
        }
        write_p++;
    } /* While data_pos < width */

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_mr3_setup_fruts()
 **************************************/

/***************************************************************************
 * fbe_parity_mr3_calculate_num_sgs()
 ***************************************************************************
 * @brief
 *  This function is responsible for counting the S/G lists needed
 *  in the RAID 5/RAID 3 State Machines.
 *  
 *  MR3 writes out entire stripes, and so does not pre-read parity; parity
 *  is instead calculated directly from the data written. If the primary
 *  data does not begin or end on a stripe boundary, data not being
 *  overwritten in leading and trailing stripes will be back-filled by
 *  pre-read operations. Pre-read data will be re-written with the primary
 *  data, so that a consistent timetamp can be assigned. MR3 is typically
 *  employed when all the data disks are to be written to.
 *
 * @param siots_p - pointer to SUB_IOTS data
 * @param num_sgs_p - Array of sgs we need to allocate, one array
 *                    entry per sg type.
 * @param read_p - Read information.
 * @param read2_p - Read 2 information.
 * @param write_p - Write information.
 * @param b_generate - true if coming from generate.  We need to know
 *                     since we do not log errors in this case.
 *
 * @return fbe_status_t
 *
 * @notes
 *
 **************************************************************************/

fbe_status_t fbe_parity_mr3_calculate_num_sgs(fbe_raid_siots_t *siots_p,
                                              fbe_u16_t *num_sgs_p,
                                              fbe_raid_fru_info_t *read_p,
                                              fbe_raid_fru_info_t *read2_p,
                                              fbe_raid_fru_info_t *write_p,
                                              fbe_bool_t b_generate)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_block_count_t mem_left = 0;
    /* Used for total count of SG entries needed for write operation.
     * Note this is a data_disk position and does not include
     * the parity disk.
     */
    fbe_u32_t sg_count_vec[FBE_RAID_MAX_DISK_ARRAY_WIDTH];
    fbe_u32_t read_sg_count;
    fbe_u32_t write_sg_count;
    fbe_u32_t vec_pos;
    fbe_u32_t data_position;
    
    fbe_sg_element_with_offset_t sg_desc;    /* tracks location in cache S/G list */
    fbe_u32_t width = fbe_raid_siots_get_width(siots_p);
    fbe_u16_t data_disks = width - fbe_raid_siots_parity_num_positions(siots_p);
    fbe_u16_t write_log_header_count = 0;

    /* Check number of parity drives that are supported. R5=1, R6=2
     */
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);

    /* Check if we need to allocate and plant write log header buffers
     */
    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_WRITE_LOG_HEADER_REQD))
    {
        fbe_u32_t journal_log_hdr_blocks;
        fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

        fbe_raid_geometry_get_journal_log_hdr_blocks(raid_geometry_p, &journal_log_hdr_blocks);
        write_log_header_count = 2;

        /* Since we're going to plant all the write fruts write log header buffers first in fbe_parity_mr3_setup_sgs,
         *  we need to skip over them here so the rest of these plants end up in the same place relative to buffer
         *  boundaries.  Since mem_left just tracks boundaries, it's ok if it's equal to 0 or page_size.
         */
        mem_left = siots_p->data_page_size - ((width  * journal_log_hdr_blocks) % siots_p->data_page_size);
    }

    /* Init total SGs for write to zero.
     */
    for (vec_pos = 0; vec_pos < FBE_RAID_MAX_DISK_ARRAY_WIDTH; vec_pos++)
    {
        sg_count_vec[vec_pos] = 0;
    }

    /* The sg_id_list is a list of Scatter Gather ID addresses.
     * Each sg_id_list[] entry is a pointer to the head of a list for an SG type.
     * Each element in the list points to the next sg_id address, and
     * the last entry in the list is NULL.
     *
     * Construct the sg_id_list for each type by determining the type
     * of sg we need for each disk op, and pushing the address of
     * this sg_id (from the siots, fruts, or fedts) onto the list of sg ids.
     *
     * Only sg ids needed for FED xfer, pre-read and write need to be added.
     *
     * When we are done, the head of the sg_id list for each type will be in
     * siots_p->sg_id_list[type].
     * By traversing this completed list, we can determine the number of
     * SGs we need of each SG type.
     */

    /* Add the sg for the first pre-reads to the list of sg_ids.
     */
    for (data_position = 0; data_position < (width - parity_drives); data_position++)
    {
        if (read_p->blocks > 0)
        {
            fbe_u32_t current_data_pos;
            status = FBE_RAID_DATA_POSITION(read_p->position, siots_p->parity_pos,
                                            width, parity_drives, &current_data_pos);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                 fbe_raid_siots_object_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_mr3_calc_num_sgs",
                                             "parity:fail to cvt pos: stat = 0x%x, "
                                             "siots = 0x%p, read = 0x%p\n",
                                             status, siots_p, read_p);
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
                fbe_raid_siots_trace(siots_p,FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_mr3_calc_num_sgs", 
                                     "parity: fail to set sg index: stat= 0x%x, siots= 0x%p, "
                                     "read_p = 0x%p\n", 
                                     status, siots_p, read_p);
                return status;
            }
            /* Add the count to the write count, since the write sg will also
             * contain a copy of this sg. 
             */
            sg_count_vec[current_data_pos] += read_sg_count;
            /* Save the type of sg we need for this read.
             */
            num_sgs_p[read_p->sg_index]++;
            read_p++;
        }
    }

    for (data_position = 0; data_position < (width - parity_drives); data_position++)
    {
        if (read2_p->blocks > 0)
        {
            fbe_u32_t current_data_pos;
            status = FBE_RAID_DATA_POSITION(read2_p->position, siots_p->parity_pos,
                                                width, parity_drives, &current_data_pos);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_object_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_mr3_calc_num_sgs",
                                            "parity:fail to cvt pos: stat = 0x%x, "
                                            "siots = 0x%p, read = 0x%p\n",
                                            status, siots_p, read_p);
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
                                     FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_mr3_calc_num_sgs", 
                                     "parity:fail to set sg index:stat=0x%x,siots=0x%p, "
                                     "read2=0x%p\n", 
                                     status, siots_p, read2_p);

                return status;
            }
            /* Add the count to the write count, since the write sg will also
             * contain a copy of this sg. 
             */
            sg_count_vec[current_data_pos] += read_sg_count;
            /* Save the type of sg we need for this read.
             */
            num_sgs_p[read2_p->sg_index]++;
            read2_p++;
        }
    }

    /* Buffered ops will use their own buffers for the data we are writing.
     */
    if (fbe_raid_siots_is_buffered_op(siots_p))
    {
        fbe_u32_t current_data_pos;
        status = FBE_RAID_DATA_POSITION(siots_p->start_pos,
                                        siots_p->parity_pos,
                                        width,
                                        parity_drives,
                                        &current_data_pos);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
             fbe_raid_siots_object_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_mr3_calc_num_sgs",
                                         "parity:fail to cvt pos: status = 0x%x, siots_p = 0x%p\n",
                                         status, siots_p);
            return status;
        }
        mem_left = fbe_raid_sg_scatter_fed_to_bed(siots_p->start_lba,
                                                  siots_p->xfer_count,
                                                  current_data_pos,
                                                  data_disks,
                                                  fbe_raid_siots_get_blocks_per_element(siots_p),
                                                  (fbe_block_count_t)siots_p->data_page_size,
                                                  mem_left,
                                                  sg_count_vec);
    }
    else
    {
        fbe_u32_t current_data_pos;
        /* Setup the sg descriptor to point to the "data" we will use
         * for this operation. 
         */
        status = fbe_raid_siots_setup_sgd(siots_p, &sg_desc);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_mr3_calc_num_sgs", 
                                 "parity:fail to setup cache sg:stat=0x%x,siots=0x%p\n",
                                 status, siots_p);
            return status;
        }
        status = FBE_RAID_DATA_POSITION(siots_p->start_pos,
                                        siots_p->parity_pos,
                                        width,
                                        parity_drives,
                                        &current_data_pos);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_mr3_calc_num_sgs",
                                        "parity:fail to cvt pos: stat=0x%x, siots=0x%p\n",
                                        status, siots_p);
            return status;
        }

        /*
         * The data is already in the S/G list supplied
         * with the IOTS. We will use these locations directly, i.e. no
         * explicit "fetch" will be performed.
         *
         * However, we must still produce the set of S/G lists
         * which partition this new data by destination disk.
         *
         * Note that the sg_count_vec already includes counts of S/G elements
         * for pre-read data, so we will add to these counts.
         */
        status = fbe_raid_scatter_cache_to_bed(siots_p->start_lba,
                                              siots_p->xfer_count,
                                              current_data_pos,
                                              data_disks,
                                              fbe_raid_siots_get_blocks_per_element(siots_p),
                                              &sg_desc,
                                              sg_count_vec);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_mr3_calc_num_sgs", 
                                 "parity:fail to scatter cache to sg bed:stat=0x%x,siots=0x%p, "
                                 "cur_data_pos=0x%x\n",
                                 status, siots_p, current_data_pos);
            return status;
        }
    }
    /* We have now finished finding the number of SG entries needed
     * for the write on each position.
     * Add the the SG ID address (from the write_fruts) to the sg_id_list.
     * Note we are passing in the sg_count_vec which includes
     * the sgs needed for pre-reads and for (host or cache) data.
     */
    for (data_position = 0; data_position < data_disks; data_position++)
    {
        fbe_u32_t current_data_pos;
        if (RAID_COND(write_p->position == siots_p->parity_pos))
        {
            fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_mr3_calc_num_sgs", 
                                 "parity: write pos %d is parity\n", write_p->position);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        status = FBE_RAID_DATA_POSITION(write_p->position,
                                        siots_p->parity_pos,
                                        width,
                                        parity_drives,
                                        &current_data_pos);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_mr3_calc_num_sgs",
                                        "parity:fail to cvt pos:stat=0x%x, siots=0x%p,write=0x%p\n",
                                        status, siots_p, write_p);
           return status;
        }
        if (RAID_COND(current_data_pos >= (width - parity_drives)))
        {
            fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_mr3_calc_num_sgs", 
                                 "parity:cur_data_pos 0x%x >= (width 0x%x - parity_drives 0x%x)\n", 
                                 current_data_pos, width, parity_drives);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        
        /* add header count to the sg count, in case we need to prepend a write log header */
        status = fbe_raid_fru_info_set_sg_index(write_p, sg_count_vec[data_position] + write_log_header_count, !b_generate);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_mr3_calc_num_sgs", 
                                 "parity:fail to set sg index:stat=0x%x,siots_p=0x%p, "
                                 "write=0x%p\n", 
                                 status, siots_p, write_p);

            return status;
        }
        num_sgs_p[write_p->sg_index]++;
        write_p++;
    }

    /* Now push the sgid address for the parity drive(s).
     */
    write_sg_count = 0;
    mem_left = fbe_raid_sg_count_uniform_blocks(write_p->blocks,
                                                siots_p->data_page_size,
                                                mem_left,
                                                &write_sg_count);

    
    /* add header count to the sg count, in case we need to prepend a write log header */
    status = fbe_raid_fru_info_set_sg_index(write_p, write_sg_count + write_log_header_count, !b_generate);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_mr3_calc_num_sgs", 
                             "parity: fail to set sg index:stat=0x%x,siots=0x%p, "
                             "write=0x%p\n", 
                             status, siots_p, write_p);

        return status;
    }
    num_sgs_p[write_p->sg_index]++;

    /* If RAID 6, push the sgid address for the second parity drives.
     */
    if (fbe_raid_siots_parity_num_positions(siots_p)== 2)
    {
        write_p++;
        write_sg_count = 0;
        mem_left = fbe_raid_sg_count_uniform_blocks(write_p->blocks,
                                                    siots_p->data_page_size,
                                                    mem_left,
                                                    &write_sg_count);

        /* add header count to the sg count, in case we need to prepend a write log header */
        status = fbe_raid_fru_info_set_sg_index(write_p, write_sg_count + write_log_header_count, !b_generate);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
             fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_mr3_calc_num_sgs", 
                                  "parity:fail to set sg index:stat=0x%x,siots=0x%p, "
                                  "write=0x%p\n", 
                                  status, siots_p, write_p);

            return status;
        }
        num_sgs_p[write_p->sg_index]++;
        write_p++;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_mr3_calculate_num_sgs()
 **************************************/
/****************************************************************
 * fbe_parity_mr3_count_buffers()
 ****************************************************************
 * @brief
 *  Count the number of buffers needed for an MR3.
 *
 * @param siots_p - The raid sub iots to count for.             
 *
 * @return
 *  Number of blocks worth of buffers required.    
 *
 ****************************************************************/
fbe_block_count_t fbe_parity_mr3_count_buffers(fbe_raid_siots_t *siots_p)
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
/******************************************
 * end fbe_parity_mr3_count_buffers()
 ******************************************/

/***************************************************************************
 * fbe_parity_mr3_setup_sgs()
 ***************************************************************************
 *
 * @brief
 *  This function is responsible for initializing the S/G lists
 *  allocated earlier by fbe_parity_mr3_alloc_sgs(). 
 *
 *  This function is executed from the RAID 5/RAID 3 MR3 state machine
 *  once memory has been allocated.
 *
 * @param siots_p - pointer to SUB_IOTS data
 * @param memory_info_p - Pointer to memory request information
 *
 * @return fbe_status_t
 *
 * @notes
 *  The implementation of this function is closely tied with that
 *  of fbe_parity_mr3_calculate_num_sgs().
 *  This function returns an error if it finds anything unexpected.
 *
 **************************************************************************/

fbe_status_t fbe_parity_mr3_setup_sgs(fbe_raid_siots_t * siots_p,
                                      fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t status;
    fbe_u16_t data_pos;
    fbe_block_count_t blkcnt;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_fruts_t *read2_fruts_ptr = NULL;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_u32_t dest_byte_count =0;
    fbe_u16_t sg_total =0;
    
    /* This array contains the sg list position in the
     * write data.  This is a "data" relative position.
     * This array does not include parity.
     */
    fbe_sg_element_t *mr3_sgl_ptr[FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1];
    fbe_sg_element_t *read2_sgl_ptr[FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1];
    /* Check number of parity that are supported. R5=1, R6=2 
     */
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_u16_t data_disks = fbe_raid_siots_get_width(siots_p) - parity_drives;
    fbe_sg_element_with_offset_t sg_desc;

    /* If write log header required, need to populate the first sg_list element with the first buffer
     * before doing the rest, or the first sg_list element will cover the header and more
     */
    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_WRITE_LOG_HEADER_REQD))
    {
        fbe_sg_element_t   *current_sg_ptr;
        fbe_u16_t           num_sg_elements;
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

    /* Now do the rest of the buffers */

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
        /* Obtain the locations of the S/G lists used
         * for the parity calculations. Portions of
         * these lists are copies of the S/G lists
         * used for the pre-reads.
         */
        mr3_sgl_ptr[data_pos] = fbe_raid_fruts_return_sg_ptr(write_fruts_ptr);
        if (read_fruts_ptr != NULL)
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

                blkcnt = read_fruts_ptr->blocks;

                if(!fbe_raid_get_checked_byte_count(blkcnt, &dest_byte_count))
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
                mr3_sgl_ptr[data_pos] = fbe_raid_copy_sg_list(sgl_ptr, mr3_sgl_ptr[data_pos]);
                read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
            }
        }
        write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);
    } /* end for all data disk positions */

    fbe_raid_siots_get_read2_fruts(siots_p, &read2_fruts_ptr);

    for (data_pos = 0; data_pos < data_disks; data_pos++)
    {
        /* S/G lists from the second pre-read can't be copied
         * immediately, so we preserve their locations and
         * sizes to allow copying later.
         */
        read2_sgl_ptr[data_pos] = 0;
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
                read2_sgl_ptr[data_pos] = fbe_raid_fruts_return_sg_ptr(read2_fruts_ptr);

                blkcnt = read2_fruts_ptr->blocks;

                if(!fbe_raid_get_checked_byte_count(blkcnt, &dest_byte_count))
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
                                                             read2_sgl_ptr[data_pos],
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

    if (RAID_COND((read_fruts_ptr != NULL) || (read2_fruts_ptr != NULL)))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: mr3: too many fruts read_fruts: %p read2_fruts: %p\n",
                             read_fruts_ptr, read2_fruts_ptr);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Stage 2a:
     * Buffered operations need to allocate their own space for the data we will be
     * writing.  Fill in the write sgs with the memory we allocated.
     */
    if (fbe_raid_siots_is_buffered_op(siots_p))
    {
        fbe_u32_t current_data_pos;
        status = FBE_RAID_DATA_POSITION(siots_p->start_pos,
                                        siots_p->parity_pos,
                                        fbe_raid_siots_get_width(siots_p),
                                        fbe_raid_siots_parity_num_positions(siots_p),
                                        &current_data_pos);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                        status, siots_p);
            return status;
        }

        if(siots_p->xfer_count > FBE_U32_MAX)
       {
            /* we do not expect blocks to scatter to exceed 32bit limit
             */
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: blocks to scatter (0x%llx) exceeding 32bit limit, siot_p = 0x%p\n",
                                        (unsigned long long)siots_p->xfer_count,
					siots_p);
            return FBE_STATUS_GENERIC_FAILURE; 
        }
        status = fbe_raid_scatter_sg_with_memory(siots_p->start_lba,
                                                 (fbe_u32_t)siots_p->xfer_count,
                                                 current_data_pos,
                                                 data_disks,
                                                 fbe_raid_siots_get_blocks_per_element(siots_p),
                                                 memory_info_p,
                                                 mr3_sgl_ptr);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p,FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: scatter sg with memeory failed with status: 0x%x, memory_info_p = 0x%p, "
                                 "mr3_sgl_ptr = 0x%p, siots_p =0x%p\n",
                                 status, memory_info_p, mr3_sgl_ptr, siots_p);
            return (status);
        }
    }
    else
    {
        fbe_u32_t current_data_pos;
        /* If the operation originates from the write cache, a
         * S/G list has been supplied which provides the location
         * of data in the cache pages.
         */
        blkcnt = siots_p->xfer_count;

        /* Setup the sg descriptor to point to the "data" we will use
         * for this operation. 
         */
        status = fbe_raid_siots_setup_sgd(siots_p, &sg_desc);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: failed to setup sg: status = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return status;
        }
        status = FBE_RAID_DATA_POSITION(siots_p->start_pos,
                                        siots_p->parity_pos,
                                        fbe_raid_siots_get_width(siots_p),
                                        fbe_raid_siots_parity_num_positions(siots_p),
                                        &current_data_pos);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                   status, siots_p);
            return status;
        }

        if(blkcnt > FBE_U32_MAX)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: block count crossing 32 bit limit = 0x%llx, siots_p = 0x%p\n",
                                 (unsigned long long)blkcnt, siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        status = fbe_raid_scatter_sg_to_bed(siots_p->start_lba,
                                   (fbe_u32_t)blkcnt,
                                   current_data_pos,
                                   data_disks,
                                   fbe_raid_siots_get_blocks_per_element(siots_p),
                                   &sg_desc,
                                   mr3_sgl_ptr);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: failed to scatter sg: status = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return (status);
        }
    }
    /* Stage 2c:
     *
     * Append the S/G lists from the second pre-reads
     * to the ends of the S/G lists for the parity
     * calculations.
     */
    blkcnt = siots_p->parity_count;

    fbe_raid_siots_get_read2_fruts(siots_p, &read2_fruts_ptr);

    for (data_pos = 0; data_pos < data_disks; data_pos++)
    {
        fbe_sg_element_t *sgl_ptr;
        fbe_u32_t current_data_pos;
        if ((sgl_ptr = read2_sgl_ptr[data_pos]) != NULL)
        {
            if (RAID_COND(read2_fruts_ptr == NULL))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: mr3: read2 %d  is null\n", data_pos);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            status = FBE_RAID_DATA_POSITION(read2_fruts_ptr->position, 
                                                siots_p->parity_pos, 
                                                fbe_raid_siots_get_width(siots_p),
                                                parity_drives,
                                                &current_data_pos);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                 fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                        "parity: failed to convert position: status = 0x%x, siots_p = 0x%p\n",
                                        status, siots_p);
                 return status;
            }
            if (RAID_COND(current_data_pos != data_pos))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: mr3: current_data_pos 0x%x!= data_pos 0x%x\n", 
                                     current_data_pos, data_pos);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            (void) fbe_raid_copy_sg_list(sgl_ptr, mr3_sgl_ptr[data_pos]);
            read2_fruts_ptr = fbe_raid_fruts_get_next(read2_fruts_ptr);
        }
    }

    /*
     * Stage 3:
     *
     * We fill the S/G list to write the new parity drive(s).
     */
    {
        fbe_sg_element_t *sgl_ptr;

        if (RAID_COND(write_fruts_ptr == NULL))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: mr3: write fruts is null\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (RAID_COND(!fbe_raid_siots_is_parity_pos(siots_p, write_fruts_ptr->position)))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: mr3: write fruts pos %d is not a parity position %d\n",
                                 write_fruts_ptr->position, siots_p->parity_pos);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
        sgl_ptr = fbe_raid_fruts_return_sg_ptr(write_fruts_ptr);

        if(!fbe_raid_get_checked_byte_count(blkcnt, &dest_byte_count))
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
        /* Handle the second parity drives for R6.
         */                       
        if (parity_drives == 2)
        {
            write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);

            if (RAID_COND(write_fruts_ptr == NULL))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: mr3: write fruts is null\n");
                return FBE_STATUS_GENERIC_FAILURE;
            }
            if (RAID_COND(!fbe_raid_siots_is_parity_pos(siots_p, write_fruts_ptr->position)))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: mr3: write fruts pos %d is not a parity position %d\n",
                                     write_fruts_ptr->position, siots_p->parity_pos);
                return FBE_STATUS_GENERIC_FAILURE;
            }
         
            sgl_ptr = fbe_raid_fruts_return_sg_ptr(write_fruts_ptr);

            if(!fbe_raid_get_checked_byte_count(blkcnt, &dest_byte_count))
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
        }                      
    }
    /* Make sure all the sg lists are well formed for the fruts.
     */
    status = fbe_parity_mr3_validate_sgs(siots_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity:mr3 fail to valid sgs:stat=0x%x,siots=0x%p\n",
                             status, siots_p);
        return status;
    }

    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_parity_mr3_setup_sgs()
 *****************************************/

/***************************************************************************
 * fbe_parity_mr3_validate_sgs     
 ***************************************************************************
 * @brief
 *  This function is responsible for verifying the
 *  consistency of mr3 sg lists
 *
 * @param siots_p - pointer to I/O.
 *
 * @return
 *  fbe_status_t
 *
 **************************************************************************/

fbe_status_t fbe_parity_mr3_validate_sgs(fbe_raid_siots_t * siots_p)
{
    fbe_block_count_t total_read_blocks = 0;
    fbe_block_count_t total_write_blocks = 0;
    fbe_block_count_t curr_blocks;
    fbe_u32_t sgs = 0;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    while (read_fruts_ptr)
    {
        if (!fbe_raid_memory_header_is_valid((fbe_raid_memory_header_t*)read_fruts_ptr->sg_id))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "raid header invalid for fruts 0x%p, sg_id: 0x%p\n",
                                        read_fruts_ptr, read_fruts_ptr->sg_id);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        sgs = 0;
        if (read_fruts_ptr->blocks == 0)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mr3: read pos %d blocks is %llu\n",
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
                                 "mr3: read pos %d blocks is %llu != %llu\n",
                                 read_fruts_ptr->position,
				 (unsigned long long)curr_blocks,
				 (unsigned long long)read_fruts_ptr->blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
    }

    fbe_raid_siots_get_read2_fruts(siots_p, &read_fruts_ptr);
    while (read_fruts_ptr)
    {
        if (!fbe_raid_memory_header_is_valid((fbe_raid_memory_header_t*)read_fruts_ptr->sg_id))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "raid header invalid for read2 fruts 0x%p, sg_id: 0x%p\n",
                                        read_fruts_ptr, read_fruts_ptr->sg_id);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        sgs = 0;
        if (read_fruts_ptr->blocks == 0)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mr3: read2 pos %d blocks is %llu\n",
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
                                 "mr3: read2 pos %d blocks is %llu != %llu\n",
                                 read_fruts_ptr->position,
				 (unsigned long long)curr_blocks,
				 (unsigned long long)read_fruts_ptr->blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
    }

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);
    while (write_fruts_ptr)
    {
        if (!fbe_raid_memory_header_is_valid((fbe_raid_memory_header_t*)write_fruts_ptr->sg_id))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "raid header invalid for write fruts 0x%p, sg_id: 0x%p\n",
                                        write_fruts_ptr, write_fruts_ptr->sg_id);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        sgs = 0;
        if (write_fruts_ptr->blocks == 0)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "mr3: write pos %d blocks is %llu\n",
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
                                 "mr3: write pos %d blocks is %llu != %llu\n",
                                 write_fruts_ptr->position,
				 (unsigned long long)curr_blocks,
				 (unsigned long long)write_fruts_ptr->blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        total_write_blocks += curr_blocks;
        write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);
    }

    /* We cannot be reading more than writing.
     */
    if (total_read_blocks > total_write_blocks)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mr3: read blocks %llu > write blocks %llu\n",
                             (unsigned long long)total_read_blocks,
			     (unsigned long long)total_write_blocks);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (total_write_blocks != (fbe_raid_siots_get_width(siots_p) * 
                               siots_p->parity_count))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mr3: write blocks %llu != width %d * parity_count %llu\n",
                             (unsigned long long)total_write_blocks,
			     fbe_raid_siots_get_width(siots_p),
                             (unsigned long long)siots_p->parity_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/* fbe_parity_mr3_validate_sgs() */

/***************************************************************************
 * fbe_parity_mr3_xor()
 ***************************************************************************
 * @brief
 *  This routine sets up S/G list pointers to interface with
 *  the XOR routine for parity operation for an MR3 write.
 *
 * @param siots_p - pointer to SUB_IOTS 
 *
 * @return
 *  fbe_status_t
 *  
 **************************************************************************/

fbe_status_t fbe_parity_mr3_xor(fbe_raid_siots_t * const siots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    /* This is the command we will submit to xor.
     */
    fbe_xor_mr3_write_t xor_mr3_write;

    /* Setup the xor command completely.
     */
    status = fbe_parity_mr3_setup_xor(siots_p, &xor_mr3_write);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to setup xor command for siots_p = 0x%p, status = 0x%x\n",
                             siots_p,status);
        return status;
    }
    /* Send xor command now. 
     */ 
    status = fbe_xor_lib_parity_mr3_write(&xor_mr3_write); 
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to send xor command for siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);
        return status;
    }
    /* Save status.
     */
    fbe_raid_siots_set_xor_command_status(siots_p, xor_mr3_write.status);

    return status;
}
/* end fbe_parity_mr3_xor() */

/****************************************************************
 * fbe_parity_mr3_setup_xor()
 ****************************************************************
 * @brief
 *  This function sets up the xor command for an MR3 operation.
 *
 * @param siots_p - The fbe_raid_siots_t for this operation. 
 * @param xor_write_p - The xor command to setup.
 *
 * @return fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_parity_mr3_setup_xor(fbe_raid_siots_t *const siots_p, 
                                      fbe_xor_mr3_write_t * const xor_write_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_block_operation_opcode_t opcode;

    /* Get number of parity that are supported. R5=1, R6=2 
     */
    fbe_u32_t parity_disks = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_u16_t data_disks = fbe_raid_siots_get_width(siots_p) - parity_disks;

    /* First init the command.
     */
    xor_write_p->status = FBE_XOR_STATUS_INVALID;

    /* Setup all the vectors in the xor command.
     */
    status = fbe_parity_mr3_setup_xor_vectors(siots_p, xor_write_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity:fail to setup xor vec,siots=0x%p,stat=0x%x,"
                             "xor_wt=0x%p\n",
                             siots_p, status, xor_write_p);

        return status;
    }

    /* Default option flags is FBE_XOR_OPTION_CHK_CRC.  We always check the
     * checksum of data read from disk for a write operation.
     */
    xor_write_p->option = FBE_XOR_OPTION_CHK_CRC;

    /* Degraded operations allow previously invalidated data
     */
    if ( siots_p->algorithm == R5_DEG_MR3 )
    {
        /* Degraded MR3 allows invalid sectors in pre-read data,
         * since we already performed a reconstruct.
         */
        xor_write_p->option |= FBE_XOR_OPTION_ALLOW_INVALIDS;
    }

    /* Let XOR know if this is a corrupt data for logging purposes. 
     * Use the opcode to check for the corrupt Data operation.  
     * If we are degraded, the algorithm may have changed.
     */
    fbe_raid_siots_get_opcode(siots_p, &opcode);
    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA )
    {
        xor_write_p->option |= FBE_XOR_OPTION_CORRUPT_DATA;
    }
    if (fbe_raid_siots_is_corrupt_crc(siots_p) == FBE_TRUE)
    {
        /* Need to log corrupted sectors
         */
        xor_write_p->option |= FBE_XOR_OPTION_CORRUPT_CRC;
    }

    /* Setup the rest of the command including the parity and data disks
     * and offset
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
 * fbe_parity_mr3_setup_xor()
 ******************************************/

/****************************************************************
 * fbe_parity_mr3_setup_xor_vectors()
 ****************************************************************
 * @brief
 *  Setup the xor command's vectors for this r5 mr3 xor_command_operation.
 *

 * @param siots_p - The siots to use when creating the xor command.
 * @param xor_write_p - The xor command to setup vectors in.
 *
 * @return fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_parity_mr3_setup_xor_vectors(fbe_raid_siots_t *const siots_p, 
                                              fbe_xor_mr3_write_t * const xor_write_p)
{
    fbe_raid_fruts_t *read_fruts_ptr;
    fbe_raid_fruts_t *write_fruts_ptr;
    fbe_raid_fruts_t *read2_fruts_ptr;
    fbe_s32_t fru_cnt;
    fbe_lba_t address_offset;
    fbe_u32_t fru_index = 0;
    fbe_status_t status = FBE_STATUS_OK;
    /* Get number of parity that are supported. R5=1, R6=2 
     */
    fbe_u32_t parity_disks = fbe_raid_siots_parity_num_positions(siots_p);

    /* Determine the address offset using the start lba of this siots. Since the siots is
     * entirely on one side of the expansion checkpoint, we can use a single address 
     * offsets when determining all disk I/Os. 
     */
    address_offset = 0;

    /* Load entries for the data disks.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    fbe_raid_siots_get_read2_fruts(siots_p, &read2_fruts_ptr);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);

    for (fru_cnt = fbe_raid_siots_get_width(siots_p); ;
         write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr))
    {
        fbe_lba_t lda;
        fbe_block_count_t count;
        fbe_u32_t data_pos;
        /* Obtain the address of the S/G list
         * for each FRU to be written.
         */
        xor_write_p->fru[fru_index].sg = fbe_raid_fruts_return_sg_ptr(write_fruts_ptr);
        xor_write_p->fru[fru_index].bitkey = (1 << write_fruts_ptr->position);

        /* Determine the data position of this fru and save it 
         * for possible later use in RAID 6 algorithms.
         */
        status = 
        FBE_RAID_EXTENT_POSITION(write_fruts_ptr->position,
                                 siots_p->parity_pos,
                                 fbe_raid_siots_get_width(siots_p),
                                 parity_disks,
                                 &data_pos);
        xor_write_p->fru[fru_index].data_position = data_pos;
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "parity: failed to convert position: status = 0x%x, siots_p = 0x%p, "
                                   "write_fruts_ptr = 0x%p\n",
                                   status, siots_p, write_fruts_ptr);
            return status;
        }
        /* Include the address offset for use by
         * lba stamping.
         */
        lda = siots_p->parity_start + address_offset;

        count = siots_p->parity_count;

        if (0 >= --fru_cnt)
        {
            /* Include the lda for last parity drive.
             * RAID 5/RAID 3 only has one parity drive.
             * RAID 6 has two parity drives;
             * the last one will be diagonal parity.
              */
            xor_write_p->fru[fru_index].start_lda = lda;
            xor_write_p->fru[fru_index].end_lda = lda + count;
            xor_write_p->fru[fru_index].count = count;
            break;
        }

        /* If there were pre-reads as part of the MR3, then
         * we need to know the lda and extent of the new
         * data.
         */
        if ((read_fruts_ptr != NULL) &&
            (read_fruts_ptr->position == write_fruts_ptr->position))
        {
            /* Data preceding the new data
             * was read from disk.
             */
            count -= read_fruts_ptr->blocks,
            lda += read_fruts_ptr->blocks,
            read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
        }

        if ((read2_fruts_ptr != NULL) &&
            (read2_fruts_ptr->position == write_fruts_ptr->position))
        {
            /* Data following the new data
             * was read from disk.
             */
            count -= read2_fruts_ptr->blocks;
            read2_fruts_ptr = fbe_raid_fruts_get_next(read2_fruts_ptr);
        }

        xor_write_p->fru[fru_index].start_lda = lda,
        xor_write_p->fru[fru_index].end_lda = lda + count;
        xor_write_p->fru[fru_index].count = siots_p->parity_count;
        fru_index++;
    }    /* for fru_cnt=width; fru_cnt >=0 */

    {
        fbe_bool_t valid_count = FBE_FALSE;

        for (fru_cnt = (fbe_raid_siots_get_width(siots_p) - parity_disks);
            fru_cnt >= 0;
            fru_cnt--)
        {
            if (xor_write_p->fru[fru_cnt].count > 0)
            {
                /* There must be at least one drive with
                 * host/cache data on it.
                 */
                valid_count = FBE_TRUE;
            }
            /* Verify that there is a valid SG
             * for every drive.
             */
            if (RAID_COND(xor_write_p->fru[fru_cnt].sg == NULL))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: mr3: read %d sg is null\n", fru_cnt);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            if (RAID_COND((*xor_write_p->fru[fru_cnt].sg).address == NULL))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: mr3: read %d sg address is null\n", fru_cnt);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            if (RAID_COND((*xor_write_p->fru[fru_cnt].sg).count == 0))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: mr3: read %d sg count is 0\n", fru_cnt);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
        if (RAID_COND(valid_count != FBE_TRUE))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: mr3: no valid counts set\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    
    return FBE_STATUS_OK;
}
/******************************************
 * fbe_parity_mr3_setup_xor_vectors()
 ******************************************/

/***************************************************************************
 * fbe_parity_mr3_get_buffered_data()
 ***************************************************************************
 * @brief
 *  This function zeros memory.
 *
 * @param siots_p - Ptr to current fbe_raid_siots_t.
 *   
 * @return fbe_status_t
 *
 * @notes
 *
 * @author
 *  3/4/2010 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_parity_mr3_get_buffered_data(fbe_raid_siots_t * const siots_p)
{
    fbe_status_t status;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_xor_zero_buffer_t zero_buffer;
    fbe_u32_t data_pos;
    fbe_u32_t fru_count = 0;
    fbe_block_count_t r1_blkcnt[FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1];    /* block counts for pre-reads, */
    fbe_block_count_t r2_blkcnt[FBE_RAID_MAX_DISK_ARRAY_WIDTH - 1];    /* distributed across the FRUs */
    fbe_block_count_t total_zeroed_blocks = 0;

    /* Check number of parity drives that are supported. R5=1, R6=2
     */
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_u32_t width = fbe_raid_siots_get_width(siots_p);

    /* Leverage MR3 calculations to describe what
     * portions of stripe(s) are *not* being accessed.
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
                             "parity:fail calc pre-read blocks:stat=0x%x>\n",
                             status);
	fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "siots=0x%p,r1_blkcnt=0x%p,r2_blkcnt=0x%p<\n",
                             siots_p, r1_blkcnt, r2_blkcnt);
        return status;
    }

    zero_buffer.status = FBE_STATUS_INVALID;

    /* Fill out the zero buffer structure from the write fruts.
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);
    for (data_pos = 0; data_pos < (width - parity_drives); data_pos++)
    {
        fbe_block_count_t total_pre_reads = (r1_blkcnt[data_pos] + r2_blkcnt[data_pos]);
        fbe_sg_element_t *sg_p = NULL;
        fbe_raid_fruts_get_sg_ptr(write_fruts_ptr, &sg_p);

        if (write_fruts_ptr->blocks > total_pre_reads)
        {
            /* We have data to zero.
             */
            zero_buffer.fru[fru_count].sg_p = sg_p;
            zero_buffer.fru[fru_count].count = write_fruts_ptr->blocks - total_pre_reads;
            zero_buffer.fru[fru_count].offset = r1_blkcnt[data_pos];
            zero_buffer.fru[fru_count].seed = write_fruts_ptr->blocks + r1_blkcnt[data_pos];
            total_zeroed_blocks += zero_buffer.fru[fru_count].count;
            fru_count++;
        }

        write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);
    }
    if (total_zeroed_blocks != siots_p->xfer_count)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "zeroed blocks %llx != xfer_count %llx\n",
                             (unsigned long long)total_zeroed_blocks,
			     (unsigned long long)siots_p->xfer_count);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    zero_buffer.disks = fru_count;
    zero_buffer.offset = fbe_raid_siots_get_eboard_offset(siots_p);

    status = fbe_xor_lib_zero_buffer(&zero_buffer);

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                               "parity: failed to zero buffer: siots_p = 0x%p, status = 0x%x\n",
                               siots_p, status);
    }
    /* Save the status for later use.
     */
    fbe_raid_siots_set_xor_command_status(siots_p, zero_buffer.status);
    return status;
}
/**************************************
 * end fbe_parity_mr3_get_buffered_data()
 **************************************/

/*!**************************************************************
 * fbe_parity_mr3_handle_timeout_error()
 ****************************************************************
 * @brief
 *  Send out the timeout usurper for the timed out retry errors
 *  in the current chain.
 * 
 *  This allows us to kick in rebuild logging and give up on retries.
 *
 * @param siots_p - Current I/O.            
 *
 * @return fbe_status_t FBE_STATUS_OK if usurpers were sent
 *                      FBE_STATUS_GENERIC_FAILURE if no
 *                      timed out retries found in the chain
 *                      and thus no usurpers need to be sent.
 *
 * @author
 *  2/11/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_mr3_handle_timeout_error(fbe_raid_siots_t *siots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_position_bitmask_t timeout1_bitmask = 0;
    fbe_raid_position_bitmask_t timeout2_bitmask = 0;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);    
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_raid_fruts_t *read2_fruts_p = NULL;

    /* Get totals for errored or dead pre-read drives.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_siots_get_read2_fruts(siots_p, &read2_fruts_p);

    /* Get the bitmask of timeouts from each chain.
     */
    fbe_raid_fruts_get_retry_timeout_err_bitmap(read_fruts_p, &timeout1_bitmask);
    fbe_raid_fruts_get_retry_timeout_err_bitmap(read2_fruts_p, &timeout2_bitmask);

    /* We will wait for all the usurpers from both chains.
     */
    siots_p->wait_count = fbe_raid_get_bitcount(timeout1_bitmask) + 
                          fbe_raid_get_bitcount(timeout2_bitmask);

    if (siots_p->wait_count != 0)
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_WARNING,
                                    "parity mr3 Send timeout usurper mask1: 0x%x mask2: 0x%x\n",
                                    timeout1_bitmask, timeout2_bitmask);
        
        /* If we have this error as a metadata operation, then we will always 
         * transition state, and when the usurpers are finished we will 
         * fail the I/O.  Metadata operations cannot wait for quiesce or 
         * deadlock will result. 
         */
        if (fbe_raid_siots_is_metadata_operation(siots_p))
        {
            fbe_raid_siots_transition(siots_p, fbe_raid_siots_dead_error);
        }

        /* Kick off the usurper packets to inform the lower level. 
         * When the packets complete, this siots will go waiting. 
         */
        if (timeout1_bitmask)
        {
            fbe_raid_fruts_chain_send_timeout_usurper(read_fruts_p, timeout1_bitmask);
        }
        if (timeout2_bitmask)
        {
            fbe_raid_fruts_chain_send_timeout_usurper(read2_fruts_p, timeout2_bitmask);
        }
        status = FBE_STATUS_OK;
    }
    else
    {
        /* No retries to send.
         */
        fbe_raid_library_trace_basic(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                     "%s no timeout errors rg: 0x%x bitmask1:0x%x bitmask2:0x%x\n",
                                     __FUNCTION__, fbe_raid_geometry_get_object_id(raid_geometry_p),
                                     timeout1_bitmask, timeout2_bitmask);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_parity_mr3_handle_timeout_error 
 **************************************/

/*!**************************************************************
 * fbe_parity_mr3_handle_retry_error()
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
 *  2/11/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_mr3_handle_retry_error(fbe_raid_siots_t *siots_p,
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
                                                "parity mr3: read retry %d errors (bitmap 0x%x) status: %d unexpected\n", 
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
 * end fbe_parity_mr3_handle_retry_error()
 **************************************/
/******************************
 * end fbe_parity_mr3_util.c
 ******************************/
