/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_parity_rebuild_util.c
 ***************************************************************************
 *
 * @brief
 *   This file contains the utility functions of the
 *   RAID 5 Rebuild state machine.
 *
 * @author
 *   10/99: Created.  Rob Foley
 *   2009: Ported to FBE. Rob Foley
 *
 ***************************************************************************/

/*************************
 **    INCLUDE FILES    **
 *************************/

#include "fbe_raid_library_proto.h"
#include "fbe_parity_io_private.h"

/********************************
 *  static STRING LITERALS
 ********************************/

/*****************************************************
 *  static FUNCTIONS 
 *****************************************************/

fbe_status_t fbe_parity_rebuild_calculate_num_sgs(fbe_raid_siots_t *siots_p,
                                                  fbe_u16_t *num_sgs_p,
                                                  fbe_raid_fru_info_t *read_p,
                                                  fbe_raid_fru_info_t *write_p);
fbe_status_t fbe_parity_rebuild_setup_fruts(fbe_raid_siots_t * siots_p, 
                                            fbe_raid_fru_info_t *read_p,
                                            fbe_raid_fru_info_t *write_p,
                                            fbe_raid_memory_info_t *memory_info_p);
fbe_status_t fbe_parity_rebuild_setup_sgs(fbe_raid_siots_t * siots_p,
                                          fbe_raid_memory_info_t *memory_info_p);
fbe_status_t fbe_parity_rebuild_extent_setup(fbe_raid_siots_t * siots_p,
                                             fbe_xor_parity_reconstruct_t *rebuild_p);
/****************************
 *  GLOBAL VARIABLES
 ****************************/

/*************************************
 *  EXTERNAL GLOBAL VARIABLES
 *************************************/

/*!**************************************************************************
 * fbe_parity_rebuild_calculate_memory_size()
 ****************************************************************************
 * @brief  This function calculates the total memory usage of the siots for
 *         this state machine.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts
 * @param read_info_p - Array of info with lba, blocks for each read position.
 * @param write_info_p - Array of info with lba, blocks for each write position.
 * @param num_pages_to_allocate_p - Number of memory pages to allocate
 * 
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @author
 *  10/16/2009 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_parity_rebuild_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                      fbe_raid_fru_info_t *read_info_p,
                                                      fbe_raid_fru_info_t *write_info_p,
                                                      fbe_u32_t *num_pages_to_allocate_p)
{
    fbe_u16_t num_sgs[FBE_RAID_MAX_SG_TYPE];
    fbe_u32_t num_fruts = 0;
    fbe_block_count_t total_blocks;
    fbe_status_t status;

    fbe_zero_memory(&num_sgs[0], (sizeof(fbe_u16_t) * FBE_RAID_MAX_SG_TYPE));

    num_fruts = fbe_raid_siots_get_width(siots_p);

    total_blocks = fbe_raid_siots_get_width(siots_p) * siots_p->parity_count;

    siots_p->total_blocks_to_allocate = total_blocks;
    fbe_raid_siots_set_optimal_page_size(siots_p,
                                         num_fruts, siots_p->total_blocks_to_allocate);

    /* Next calculate the number of sgs of each type.
     */
    status = fbe_parity_rebuild_calculate_num_sgs(siots_p, 
                                                  &num_sgs[0], 
                                                  read_info_p,
                                                  write_info_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: rebuild calc num sgs failed with status = 0x%x, siots_p = 0x%p, "
                             "read_info_p = 0x%p, write_info_p = 0x%p\n",
                             status, siots_p, read_info_p, write_info_p);
        return (status); 
    }

    /* Take the information we calculated above and determine how this translates 
     * into a number of pages. 
     */
    status = fbe_raid_siots_calculate_num_pages(siots_p, num_fruts, &num_sgs[0],
                                                FBE_FALSE /* No nested siots required*/);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: calc num pages failed with status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return(status); 
    }
    *num_pages_to_allocate_p = siots_p->num_ctrl_pages;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_rebuild_calculate_memory_size()
 **************************************/

/*!**************************************************************
 * fbe_parity_rebuild_setup_resources()
 ****************************************************************
 * @brief
 *  Setup the newly allocated resources.
 *
 * @param siots_p - current I/O.
 * @param read_info_p - Array of info with lba, blocks for each read position.
 * @param write_info_p - Array of info with lba, blocks for each write position.
 *
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @author
 *  10/5/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_rebuild_setup_resources(fbe_raid_siots_t * siots_p, 
                                                fbe_raid_fru_info_t *read_p, 
                                                fbe_raid_fru_info_t *write_p)
{
    fbe_raid_memory_info_t  memory_info;
    fbe_status_t            status;
    fbe_raid_memory_info_t  data_memory_info;

    /* Initialize our memory request information
     */
    status = fbe_raid_siots_init_control_mem_info(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to init memory info: status = 0x%x, siots_p = 0x%p, "
                             "siots_p->start_lba = 0x%llx, siots_p->xfer_count = 0x%llx\n",
                             status, siots_p,
			     (unsigned long long)siots_p->start_lba,
			     (unsigned long long)siots_p->xfer_count);
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
    status = fbe_parity_rebuild_setup_fruts(siots_p, read_p, write_p,
                                            &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity:fail to setup fruts with stat=0x%x,siots=0x%p"
                             "read=0x%p,write=0x%p\n",
                             status, siots_p, read_p, write_p);
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
                                                  read_p, NULL, write_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: failed to init vrts: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return  status;
    }

    /* Assign buffers to sgs.
     * We reset the current ptr to the beginning since we always 
     * allocate buffers starting at the beginning of the memory. 
     */
    status = fbe_parity_rebuild_setup_sgs(siots_p, &data_memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {  
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity:468 fail to setup sgs with stat=0x%x,siots=0x%p\n",
                             status, siots_p);
        return (status); 
    }

    /* Make sure the buffer memory we just used does not overlap into 
     * the area used for other resources. 
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_parity_rebuild_setup_resources()
 ******************************************/
/*!**************************************************************************
 * fbe_parity_rebuild_get_fru_info()
 ***************************************************************************
 *
 * @brief
 *   This function is responsible for setting up all fruts'.
 *   This function is executed from the RAID 5 rebuild state machine.
 *
 * @param siots_p - pointer to SUB_IOTS data
 * @param read_info_p - Array of info with lba, blocks for each read position.
 * @param write_info_p - Array of info with lba, blocks for each write position.
 *
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @notes
 *  This function modifies the sub iots read and write fruts ptrs.
 *
 * @author
 *  9/13/99:  Rob Foley -- created.      
 *
 **************************************************************************/

fbe_status_t fbe_parity_rebuild_get_fru_info(fbe_raid_siots_t * siots_p, 
                                             fbe_raid_fru_info_t *read_p, 
                                             fbe_raid_fru_info_t *write_p)
{
    fbe_u32_t data_pos;    /* data position in array */
    fbe_u32_t found_count;
    fbe_block_count_t parity_range_offset;
    fbe_u8_t *position = siots_p->geo.position;
    fbe_lba_t logical_parity_start;
    fbe_raid_position_bitmask_t degraded_bitmask;

    logical_parity_start = siots_p->geo.logical_parity_start;

    parity_range_offset = siots_p->geo.start_offset_rel_parity_stripe;

    /* Validate the parity block count.
     */
    if (RAID_COND(siots_p->parity_count == 0))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: parity count == 0\n");

        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);
    data_pos = 0;

    for (found_count = 0;
        found_count < fbe_raid_siots_get_width(siots_p);
        found_count++)
    {
        /* Skip over the dead positions.
         */
        if ((1 << position[data_pos]) & degraded_bitmask)
        {
            data_pos++;
            continue;
        }

        if (RAID_COND(data_pos >= fbe_raid_siots_get_width(siots_p)))
        {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: data posistion 0x%x >= siots width 0x%x\n",
                                 data_pos,
                                 fbe_raid_siots_get_width(siots_p));

            return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_raid_fru_info_init(read_p,
                               logical_parity_start + parity_range_offset,
                               siots_p->parity_count,
                               position[data_pos]);

        /* Skip the dead disk.
         * We will add the dead disk fruts to the write list later.
         */
        data_pos++;
        read_p++;
    }

    /* Initialize the write_fruts for the dead drive.
     * This is the minimum we will be writing.
     */
    fbe_raid_fru_info_init(write_p,
                           logical_parity_start + parity_range_offset,
                           siots_p->parity_count,
                           siots_p->dead_pos);
    if (siots_p->dead_pos_2 != FBE_RAID_INVALID_DEAD_POS)
    {
        write_p++;
        fbe_raid_fru_info_init(write_p,
                               logical_parity_start + parity_range_offset,
                               siots_p->parity_count,
                               siots_p->dead_pos_2);
    }
    return FBE_STATUS_OK;
}
/*******************************
 * end  fbe_parity_rebuild_get_fru_info()
 *******************************/

/*!**************************************************************************
 * fbe_parity_rebuild_calculate_num_sgs()
 ***************************************************************************
 * @brief
 *   This function counts the number of each type of sg needed.
 *   This function is executed from the RAID 5 rebuild state machine.
 *
 * @param siots_p - current I/O 
 * @param num_sgs_p - Array of sgs of width FBE_RAID_MAX_SG_TYPE.
 *                    Number of sgs of each type to allocate.
 * @param read_info_p - Array of info with lba, blocks for each read position.
 * @param write_info_p - Array of info with lba, blocks for each write position.
 *
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @notes
 *   The rebuild fru is always first in the write fruts list.
 *
 * @author
 *  10/19/2009 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_parity_rebuild_calculate_num_sgs(fbe_raid_siots_t *siots_p,
                                                  fbe_u16_t *num_sgs_p,
                                                  fbe_raid_fru_info_t *read_p,
                                                  fbe_raid_fru_info_t *write_p)
{
    fbe_u32_t sg_count = 0;    /* S/G elements per data drive */
    fbe_block_count_t mem_left = 0;
    fbe_u32_t width;
    fbe_u32_t data_pos;
    fbe_u32_t rebuild_positions = fbe_raid_siots_get_degraded_count(siots_p);
    width = fbe_raid_siots_get_width(siots_p);

    if (RAID_COND(!fbe_raid_siots_validate_page_size(siots_p)))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: siots_p->page_size is invalid\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }


    if (RAID_COND(siots_p->algorithm != R5_RB))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: siots algorithm 0x%x is unexpected.\n",
                             siots_p->algorithm);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Determine the number of sgs needed to reference the buffers.
     */

    for (data_pos = 0; data_pos < width - rebuild_positions; data_pos++)
    {
        sg_count = 0;

        if (RAID_COND(read_p->blocks == 0))
        {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: read_p->blocks == 0\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (RAID_COND(read_p->position >= FBE_RAID_MAX_DISK_ARRAY_WIDTH))
        {
            fbe_raid_siots_trace(siots_p, 
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: read position 0x%x >= maximum disk array width 0x%x\n",
                                 read_p->position,
                                 FBE_RAID_MAX_DISK_ARRAY_WIDTH);

            return FBE_STATUS_GENERIC_FAILURE;
        }


        /* First calcualte the number of sgs elements for this read drive.
         */
        mem_left = fbe_raid_sg_count_uniform_blocks(read_p->blocks,
                                                    siots_p->data_page_size,
                                                    mem_left,
                                                    &sg_count);

        /* Add on another sg entry since we might need to split a buffer into more than
         * one sg if we are going to strip mine. 
         */
        if ((sg_count + 1 < FBE_RAID_MAX_SG_ELEMENTS))
        {
            sg_count++;
        }
        read_p->sg_index = fbe_raid_memory_sg_count_index(sg_count);

        /* First determine the read sg type so we know which type of sg we
         * need to increase the count of. 
         */
        if (RAID_COND(read_p->blocks == 0))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: read blocks %llu unexpected\n",
                                 (unsigned long long)read_p->blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (RAID_COND(read_p->sg_index >= FBE_RAID_MAX_SG_TYPE))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: read_p->sg_index 0x%x >= FBE_RAID_MAX_SG_TYPE\n",
                                 read_p->sg_index);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        num_sgs_p[read_p->sg_index]++;
        read_p++;
    }

    /* Count sgs for write also.
     */
    for (data_pos = 0; data_pos < rebuild_positions; data_pos++)
    {
        sg_count = 0;

        if (RAID_COND(write_p->blocks == 0))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: write_p->blocks == 0\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Calculate the number of sgs elements for this rebuild drive.
         */
        mem_left = fbe_raid_sg_count_uniform_blocks(write_p->blocks,
                                                    siots_p->data_page_size,
                                                    mem_left,
                                                    &sg_count);
        /* Determine which sg type to use. 
         * Add one to the totals for this type. 
         */
        write_p->sg_index = fbe_raid_memory_sg_count_index(sg_count);
        /* First determine the read sg type so we know which type of sg we
        * need to increase the count of. 
        */
        if (RAID_COND(write_p->blocks == 0))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: write blocks %llu unexpected\n",
                                 (unsigned long long)write_p->blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (RAID_COND(write_p->sg_index >= FBE_RAID_MAX_SG_TYPE))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: write sg_type %d unexpected\n",
                                 write_p->sg_index);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        num_sgs_p[write_p->sg_index]++;
        write_p ++;
    }

    return FBE_STATUS_OK;
}
/*******************************
 * end fbe_parity_rebuild_calculate_num_sgs()
 *******************************/

/*!**************************************************************************
 * fbe_parity_rebuild_setup_fruts()
 ***************************************************************************
 *
 * @brief
 *   This function is responsible for setting up all fruts'.
 *   This function is executed from the RAID 5 rebuild state machine.
 *
 * @param siots_p - pointer to I/O.
 * @param read_p - Array of info with lba, blocks for each read position.
 * @param write_p - Array of info with lba, blocks for each write position.
 * @param memory_info_p - Pointer to memory request information
 *
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @notes
 *  This function modifies the sub iots read and write fruts ptrs.
 *
 * @author
 *  10/19/2009 - Created. Rob Foley  
 *  
 **************************************************************************/

fbe_status_t fbe_parity_rebuild_setup_fruts(fbe_raid_siots_t * siots_p, 
                                            fbe_raid_fru_info_t *read_p,
                                            fbe_raid_fru_info_t *write_p,
                                            fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t status;
    fbe_u16_t data_pos;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_u32_t rebuild_positions = fbe_raid_siots_get_degraded_count(siots_p);
    fbe_u32_t width = fbe_raid_siots_get_width(siots_p);

    /* Validate the parity block count.
     */
    if (RAID_COND(siots_p->parity_count == 0))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: siots_p->parity_count == 0\n");

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*
     * Calculate the position of the data disk
     * in the array where the host write begins.
     */
    status = fbe_raid_siots_get_fruts_chain(siots_p, 
                                            &siots_p->read_fruts_head,
                                            fbe_raid_siots_get_width(siots_p) - rebuild_positions,
                                            memory_info_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: failed to get fruts chain: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return  status;
    }

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

    if (RAID_COND(read_fruts_ptr == NULL))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: read_fruts_ptr == NULL, sitos_p = 0x%p\n",
                             siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (data_pos = 0;
        data_pos < fbe_raid_siots_get_width(siots_p) - rebuild_positions;
        data_pos++)
    {
        status = fbe_raid_fruts_init_fruts(read_fruts_ptr,
                                           FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                           read_p->lba,
                                           read_p->blocks,
                                           read_p->position);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity : failed to init fruts for siots_p = 0x%p, read_fruts_ptr = 0x%p. "
                                 "status = 0x%x\n",
                                 siots_p, read_fruts_ptr, status);

            return  status;
        }

        /* Skip the dead disk.
         * We will add the dead disk fruts to the write list later.
         */
        read_p++;
        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
    }

    for (data_pos = 0; data_pos < rebuild_positions; data_pos++)
    {
        /* Initialize the write_fruts for the dead drive.
         * This is the minimum we will be writing.
         */
        write_fruts_ptr = fbe_raid_siots_get_next_fruts(siots_p,
                                                        &siots_p->write_fruts_head,
                                                        memory_info_p);

        if (RAID_COND(write_fruts_ptr == NULL))
        {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: write_fruts_ptr == NULL\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        status = fbe_raid_fruts_init_fruts(write_fruts_ptr,
                                           FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                           write_p->lba,
                                           write_p->blocks,
                                           write_p->position);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity : failed to init fruts for siots_p = 0x%p, "
                                 "write_fruts_ptr = 0x%p, status = 0x%x\n",
                                 siots_p, write_fruts_ptr, status);
            return  status;
        }
        write_p++;
    }

    if (fbe_queue_length(&siots_p->write_fruts_head) != rebuild_positions)
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: write fruts chain length %d != rebuild_positions %d\n",
                             fbe_queue_length(&siots_p->write_fruts_head),
                             rebuild_positions);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (fbe_queue_length(&siots_p->read_fruts_head) != (width - rebuild_positions))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: read fruts chain length %d != (width %d - rebuild_positions %d)\n",
                             fbe_queue_length(&siots_p->read_fruts_head), width, rebuild_positions);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/*******************************
 * end fbe_parity_rebuild_setup_fruts()
 *******************************/

/*!**************************************************************************
 * fbe_parity_rebuild_setup_sgs()
 ***************************************************************************
 * @brief 
 *  This function is responsible for initializing the S/G
 *  lists allocated earlier 
 *
 *  This function is executed from the RAID 5 rebuild state
 *  machine once resources are allocated.
 *
 * @param siots_p - pointer to I/O.
 * @param memory_info_p - Pointer to memory request information
 *
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @notes
 *  The rebuild fru is always first in the write fruts list.
 *
 * @author
 *  9/13/99:  Rob Foley -- Created.
 *
 **************************************************************************/
fbe_status_t fbe_parity_rebuild_setup_sgs(fbe_raid_siots_t * siots_p,
                                          fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t status;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_raid_fruts_t *write_fruts_p = NULL;
    fbe_sg_element_t *sgl_ptr = NULL;

    if (RAID_COND(siots_p->start_pos >= fbe_raid_siots_get_width(siots_p)))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: start pos  0x%x >= width 0x%x\n",
                             siots_p->start_pos,
                             fbe_raid_siots_get_width(siots_p));

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (RAID_COND(siots_p->xfer_count == 0))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: siots_p->xfer_count == 0\n");

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (RAID_COND(fbe_raid_siots_get_width(siots_p) < FBE_PARITY_MIN_WIDTH))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: siots width 0x%x < FBE_PARITY_MIN_WIDTH 0x%x\n",
                             fbe_raid_siots_get_width(siots_p),
                             FBE_PARITY_MIN_WIDTH);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    /* We should have at least one read fruts and one write fruts.
     */
    if (RAID_COND(read_fruts_p == NULL))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: read_fruts_p == NULL\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (RAID_COND(write_fruts_p == NULL))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: write_fruts_p == NULL\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Initialize all the read sgs.
     */
    while (read_fruts_p != NULL)
    {
        fbe_u16_t sg_total = 0;
        fbe_u32_t dest_byte_count =0;

        if (RAID_COND(read_fruts_p->sg_id == 0))
        {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: read_fruts_p->sg_id == 0\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (RAID_COND(read_fruts_p->blocks == 0))
        {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: read_fruts_p->blocks == 0\n");

            return FBE_STATUS_GENERIC_FAILURE;
        }

        sgl_ptr = fbe_raid_fruts_return_sg_ptr(read_fruts_p);

        if(!fbe_raid_get_checked_byte_count(read_fruts_p->blocks, &dest_byte_count))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                               "%s: byte count exceeding 32bit limit \n",
                                               __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE; 
        }
        sg_total= fbe_raid_sg_populate_with_memory(memory_info_p,
                                                                                     sgl_ptr, 
                                                                                     dest_byte_count);
        if (sg_total == 0)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                    "%s: failed to populate sg for: siots = 0x%p\n",
                     __FUNCTION__,siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);
    }

    /* Initialize the write sgs.
     */
    if (RAID_COND(write_fruts_p == NULL))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: write_fruts_p == NULL\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    while (write_fruts_p != NULL)
    {
        fbe_u16_t sg_total = 0;
        fbe_u32_t dest_byte_count =0;

        /* Fill out buffer.
         */
        sgl_ptr = fbe_raid_fruts_return_sg_ptr(write_fruts_p);

        if(!fbe_raid_get_checked_byte_count(write_fruts_p->blocks,&dest_byte_count))
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
        write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);
    }

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    /* Make sure each of the fruts chains is sane.
     */
    status = fbe_raid_fruts_for_each(FBE_RAID_FRU_POSITION_BITMASK, read_fruts_p, fbe_raid_fruts_validate_sgs, 0, 0);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to validate sgs of read_fruts_p 0x%p "
                             "for siots_p 0x%p; faield status = 0x%x\n",
                             read_fruts_p, siots_p, status);
        return  status;
    }

    status = fbe_raid_fruts_for_each(FBE_RAID_FRU_POSITION_BITMASK, write_fruts_p, fbe_raid_fruts_validate_sgs, 0, 0);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to validate sgs of write_fruts_p 0x%p "
                             "for siots_p 0x%p. status = 0x%x\n",
                             write_fruts_p, siots_p, status);
        return  status;
    }

    return FBE_STATUS_OK;
}
/*******************************
 * end fbe_parity_rebuild_setup_sgs()
 *******************************/

/*!***************************************************************
 *  fbe_parity_rebuild_extent()
 ****************************************************************
 * @brief
 *  Rebuild an extent for this siots.
 *
 * @param siots_p - Current I/O.
 *
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @author
 *  10/19/2009 - Rewrote. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_rebuild_extent(fbe_raid_siots_t * siots_p)
{
    fbe_xor_parity_reconstruct_t rebuild;
    fbe_status_t status;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_u32_t width = 0;

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    /* Initialize the sg desc and bitkey vectors.
     */
    status = fbe_parity_rebuild_extent_setup(siots_p, &rebuild);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: rebuild extent setup failed with status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return(status); 
    }

    status = fbe_raid_siots_parity_rebuild_pos_get( siots_p, &rebuild.parity_pos[0], &rebuild.rebuild_pos[0] );
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to get rebuild or parity position for siots_p = 0x%p, status = 0x%x\n",
                             siots_p, status);

        return  status;
    }

    /* Init the remaining parameters.
     */
    rebuild.seed = read_fruts_p->lba;
    rebuild.count = siots_p->parity_count;
    rebuild.eboard_p = &siots_p->misc_ts.cxts.eboard;
    rebuild.eregions_p = &siots_p->vcts_ptr->error_regions;
    rebuild.b_final_recovery_attempt = fbe_raid_siots_is_retry(siots_p);
    rebuild.option = 0;
    width = fbe_raid_siots_get_width(siots_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: got unexpected width (=0x%x) of raid group for siots_p = 0x%p\n",
                             width, siots_p);

        return  status;
    }
    fbe_raid_siots_get_no_data_bitmap(siots_p, &siots_p->misc_ts.cxts.eboard.no_data_err_bitmap);

    rebuild.data_disks = (width - fbe_raid_siots_parity_num_positions(siots_p));

    /* Set XOR debug options.
     */
    fbe_raid_xor_set_debug_options(siots_p, 
                                   &rebuild.option, 
                                   FBE_FALSE /* Don't generate error trace */);

    /* Perform the rebuild.  
     */
    status = fbe_xor_lib_parity_rebuild(&rebuild);

    return status;
}
/*******************************
 * end fbe_parity_rebuild_extent()
 *******************************/

/*!***************************************************************
 * fbe_parity_rebuild_extent_setup()
 ****************************************************************
 * @brief
 *   Setup bitkey, sgl, seed, and count vectors for an extent
 *   based rebuild.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 * @param rebuild_p - Ptr to struct to pass to xor.
 *
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @notes
 *  The rebuild position is always first in the write fruts list.
 *
 * @author
 *  11/16/99 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_rebuild_extent_setup(fbe_raid_siots_t * siots_p,
                                             fbe_xor_parity_reconstruct_t *rebuild_p)
{
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_raid_fruts_t *write_fruts_p = NULL;
    fbe_u32_t data_pos;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);

    /* First initialize the positions we are rebuilding from.
     */
    while (read_fruts_p != NULL)
    {
        if (RAID_COND(read_fruts_p->position >= fbe_raid_siots_get_width(siots_p)))
        {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: read_fruts_p->position 0x%x >= siots width 0x%x\n",
                                 read_fruts_p->position,
                                 fbe_raid_siots_get_width(siots_p));

            return FBE_STATUS_GENERIC_FAILURE;
        }

        rebuild_p->sg_p[read_fruts_p->position] = fbe_raid_fruts_return_sg_ptr(read_fruts_p);

        /* Determine the data position of this fru and save it for possible later
         * use in RAID 6 algorithms.
         */
        status = FBE_RAID_EXTENT_POSITION(read_fruts_p->position,
                                          siots_p->parity_pos,
                                          fbe_raid_siots_get_width(siots_p),
                                          fbe_raid_siots_parity_num_positions(siots_p),
                                          &data_pos);
        rebuild_p->data_position[read_fruts_p->position] = data_pos;
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: failed to convert position: status = 0x%x, "
                                                "siots_p = 0x%p\n",
                                                status, siots_p);
            return  status;
        }
        /* Init the bitkey for this position.
         */
        rebuild_p->bitkey[read_fruts_p->position] = (1 << read_fruts_p->position);
        read_fruts_p = fbe_raid_fruts_get_next(read_fruts_p);
    }

    while (write_fruts_p != NULL)
    {
        /* Next, setup the drive to be rebuilt.
         */
        if (RAID_COND(write_fruts_p->position >= fbe_raid_siots_get_width(siots_p)))
        {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: write_fruts_p->position 0x%x >= siots width 0x%x\n",
                                 write_fruts_p->position,
                                 fbe_raid_siots_get_width(siots_p));

            return FBE_STATUS_GENERIC_FAILURE;
        }

        rebuild_p->sg_p[write_fruts_p->position] = fbe_raid_fruts_return_sg_ptr(write_fruts_p);
        /* Determine the data position of this fru and save it for possible later
         * use in RAID 6 algorithms.
         */
        status = FBE_RAID_EXTENT_POSITION(write_fruts_p->position,
                                          siots_p->parity_pos,
                                          fbe_raid_siots_get_width(siots_p),
                                          fbe_raid_siots_parity_num_positions(siots_p),
                                          &data_pos);
        rebuild_p->data_position[write_fruts_p->position] = data_pos;
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity:  errored as position covnersion failed: status = 0x%x, "
                                                "write_fruts_p = 0x%p, siots_p = 0x%p\n",
                                                status, write_fruts_p, siots_p);
            return  status;
        }
        /* Initialize the bitkey for the rebuild position.
         */
        rebuild_p->bitkey[write_fruts_p->position] = (1 << write_fruts_p->position);

        write_fruts_p = fbe_raid_fruts_get_next(write_fruts_p);
    }
    return FBE_STATUS_OK;
}
/*******************************
 * end fbe_parity_rebuild_extent_setup()
 *******************************/
/*!***************************************************************
 * fbe_parity_rebuild_multiple_writes()
 ****************************************************************
 * @brief
 *  Start writes to more than just the rebuild position.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 * @param w_bitmap - bitmap of positions to write.
 *
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @notes
 *  The rebuild position is always first in the write fruts list.
 *
 * @author
 *  11/24/99 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_rebuild_multiple_writes(fbe_raid_siots_t * siots_p, fbe_u16_t w_bitmap)
{
    fbe_raid_fruts_t *write_fruts_p = NULL;
    fbe_raid_fruts_t *read_fruts_p = NULL;
    fbe_raid_position_bitmask_t rebuild_bitmask;
    fbe_raid_position_bitmask_t rl_bitmask;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_raid_siots_get_rebuild_bitmask(siots_p, &rebuild_bitmask);
    fbe_raid_siots_get_rebuild_logging_bitmask(siots_p, &rl_bitmask);


    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    /*************************************************************
     * The write_fruts has just the degraded fruts positions.
     * The read_fruts has just the several read fruts.
     * Append the read fruts to the write fruts,
     * and eliminate start the writes for the write bitmap.
     *************************************************************/
    if (RAID_COND(fbe_queue_length(&siots_p->write_fruts_head) != fbe_raid_get_bitcount(rebuild_bitmask | rl_bitmask)))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: write fruts chain length %d != bitcount(rebuild bits 0x%x | rl bits 0x%x)\n",
                             fbe_queue_length(&siots_p->write_fruts_head),
                             rebuild_bitmask, rl_bitmask);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (RAID_COND(read_fruts_p == NULL))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: read_fruts_p == NULL\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Append the read fruts to the write fruts.
     */
    while (!fbe_queue_is_empty(&siots_p->read_fruts_head))
    {
        fbe_raid_common_t *common_p = NULL;
        common_p = (fbe_raid_common_t *)fbe_queue_pop(&siots_p->read_fruts_head);
        fbe_raid_common_enqueue_tail(&siots_p->write_fruts_head, common_p);
    }

    /* Determine the count of writes to submit.
     */
    siots_p->wait_count = fbe_raid_get_bitcount(w_bitmap);

    /* Since we no longer invalidate parity, we could recreate a sector that
     * was previously invalidated without the need to update parity.  Thus
     * the minimum number of write positions is 1.
     */
    if (RAID_COND(siots_p->wait_count < 1))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: siots_p->wait_count 0x%llx <= 1\n",
                             siots_p->wait_count);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Slam the opcode to write, so our entire chain will be writes.
     */
    status = fbe_raid_fruts_for_each(FBE_RAID_FRU_POSITION_BITMASK,
                                     write_fruts_p,
                                     fbe_raid_fruts_set_opcode_success,
                                    (fbe_u64_t) FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID,
                                    (fbe_u64_t) NULL);

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed for fruts chain. write_fruts_p 0x%p "
                             "for siots_p 0x%p. status = 0x%x\n",
                             write_fruts_p, siots_p, status);

        return  status;
    }

    status = fbe_raid_fruts_for_each(w_bitmap,
                                   write_fruts_p,
                                   fbe_raid_fruts_set_opcode,
                                   (fbe_u64_t) FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                   (fbe_u64_t) NULL);

    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to set opcode for write_fruts_p 0x%p "
                             "for siots_p 0x%p; w_bitmap = 0x%x, opcode = 0x%x, status = 0x%x\n",
                             write_fruts_p, siots_p, w_bitmap, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE, status);
        return  status;
    }

    /* Start writes to all frus for our write bitmap.
     */
    status =  fbe_raid_fruts_for_each(w_bitmap,
                                   write_fruts_p,
                                   fbe_raid_fruts_retry,
                                   (fbe_u64_t) FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                   CSX_CAST_PTR_TO_PTRMAX(fbe_raid_fruts_evaluate));

    
    if (RAID_FRUTS_COND(status != FBE_STATUS_OK))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to retry write_fruts_p 0x%p for siots_p 0x%p. status = 0x%x\n",
                             write_fruts_p, siots_p, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_parity_rebuild_multiple_writes()
 *****************************************/

/*!***************************************************************
 * fbe_parity_rebuild_reinit()
 ****************************************************************
 * @brief
 *  Reinitialize the tracking structures for the next piece of
 *  a rebuild.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 *
 * @return fbe_status_t FBE_STATUS_OK on success
 *                      FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @notes
 *  The rebuild position is always first in the write fruts list.
 *
 * @author
 *  11/30/99 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_rebuild_reinit(fbe_raid_siots_t * siots_p)
{
    fbe_raid_fruts_t *cur_fruts_p;
    fbe_lba_t lba = siots_p->parity_start;
    fbe_block_count_t blocks = siots_p->parity_count;
    fbe_lba_t parity_range_offset;
    fbe_u32_t extent_pos;
    fbe_queue_element_t *element_p = NULL;
    fbe_raid_fruts_t *write_fruts_p = NULL;
    fbe_lba_t logical_parity_start;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_position_bitmask_t degraded_bitmask;
    fbe_raid_siots_get_degraded_bitmask(siots_p, &degraded_bitmask);

    logical_parity_start = siots_p->geo.logical_parity_start;

    /* Verify that we do not go beyond the range of parity locked
     * by the parent siots.
     */
    if (RAID_COND((siots_p->algorithm != R5_RB) &&
                  ((lba + blocks - 1) > (fbe_raid_siots_get_parent(siots_p)->parity_start +
                                         fbe_raid_siots_get_parent(siots_p)->parity_count - 1))))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity: %s: operating beyond range : siots_p->algorithm = 0x%x and"
                             "start lba = 0x%llx, block count = 0x%llx,"
                             "parent siots parity start = 0x%llx,"
                             "parent siots parity count = 0x%llx, siots_p = 0x%p\n",
                             __FUNCTION__,
                             siots_p->algorithm,
                             (unsigned long long)lba,
                             (unsigned long long)blocks,
                             (unsigned long long)fbe_raid_siots_get_parent(siots_p)->parity_start,
                             (unsigned long long)fbe_raid_siots_get_parent(siots_p)->parity_count,
                             siots_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    parity_range_offset = siots_p->geo.start_offset_rel_parity_stripe
                          + (lba - siots_p->start_lba);

    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_p);
    if (RAID_COND(write_fruts_p == NULL))
    {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: %s errored: write_fruts_p == NULL, siots_p = 0x%p\n",
                             __FUNCTION__,
                             siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*************************************************************
     * The write_fruts should have just the rebuild position's fruts
     * The read_fruts has just the several read fruts.
     * Make sure the write fruts has a single fruts only,
     * as we may have appended the read_fruts to write errored positions
     * during the last rebuild pass.
     *************************************************************/

    /* Put the read fruts back on the read chain.
     */
    element_p = fbe_queue_front(&siots_p->write_fruts_head);
    while (element_p != NULL)
    {
        fbe_raid_fruts_t *fruts_p = (fbe_raid_fruts_t*) element_p;
        fbe_queue_element_t *next_p = fbe_queue_next(&siots_p->write_fruts_head,
                                                     element_p);
        if (((1 << fruts_p->position) & degraded_bitmask) == 0)
        {
            fbe_queue_remove(element_p);
            fbe_raid_common_enqueue_tail(&siots_p->read_fruts_head, (fbe_raid_common_t*)element_p);
        }
        element_p = next_p;
    }

    siots_p->wait_count = 0;

    /*************************************************************
     * Loop through all the read fruts.
     * Re-init the fruts for each position.
     *************************************************************/
    fbe_raid_siots_get_read_fruts(siots_p, &cur_fruts_p);

    while (cur_fruts_p != NULL)
    {
        /* Initialize this fruts.
         */
        status = FBE_RAID_EXTENT_POSITION(cur_fruts_p->position,
                                          siots_p->parity_pos,
                                          fbe_raid_siots_get_width(siots_p),
                                          fbe_raid_siots_parity_num_positions(siots_p),
                                          &extent_pos);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "parity: failed to covert postion: status: 0x%x, siots_p = 0x%p\n",
                                                status, siots_p);
            return  status;
        }
        /* LBA to read is lba of parity range start on this position
         * plus our offset from the parity range.
         */
        lba = logical_parity_start + parity_range_offset;

        status = fbe_raid_fruts_init_request(cur_fruts_p,
                                             FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                             lba,
                                             blocks,
                                             cur_fruts_p->position);
        if ( RAID_COND(status != FBE_STATUS_OK) )
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: failed to init fruts: siots_p = 0x%p, fruts 0x%p, status 0x%x\n",
                                 siots_p, cur_fruts_p, status);
            return  status;
        }

        cur_fruts_p = fbe_raid_fruts_get_next(cur_fruts_p);
    }

    /* Reinit the write fruts chain also.
     */
    fbe_raid_siots_get_write_fruts(siots_p, &cur_fruts_p);

    while (cur_fruts_p != NULL)
    {
        /* LBA to write is lba of parity range start on this position
         * plus our offset from the parity range.
         */
        lba = logical_parity_start + parity_range_offset;

        /* Re-init the fruts to the same position.
         */
        status = fbe_raid_fruts_init_request(cur_fruts_p,
                                             FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE,
                                             lba,
                                             blocks,
                                             cur_fruts_p->position);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: failed to init fruts: siots_p = 0x%p, fruts = 0x%p status = 0x%x\n",
                                 siots_p, cur_fruts_p, status);
            return  status;
        }
        else if (RAID_COND(((1 << cur_fruts_p->position) & degraded_bitmask) == 0))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity: unexpected error: fruts = 0x%p, "
                                 "pos = %d, deg_bitmask = 0x%x\n", 
                                 cur_fruts_p, cur_fruts_p->position,
                                 degraded_bitmask);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        cur_fruts_p = fbe_raid_fruts_get_next(cur_fruts_p);
    }
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_parity_rebuild_reinit()
 *****************************************/

/***********
 * End fbe_parity_rebuild_util.c
 ***********/
