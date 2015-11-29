/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_xor.c
 ***************************************************************************
 *
 * @brief
 *  This file contains xor related functions of the raid library.
 *
 * @version
 *   8/10/2009:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static fbe_status_t fbe_raid_xor_zero_buffers(fbe_raid_siots_t * const siots_p);


/***************************************************************************
 *          fbe_raid_xor_set_debug_options()
 ***************************************************************************
 *
 * @brief   Set the XOR options flags with debug if neccessary
 *
 * @param   siots_p - Ptr to current fbe_raid_siots_t.
 * @param   option_p - Pointer to option flags to update
 * @param   b_generate_error_trace - FBE_TRUE - Generate error trace that may
 *                      panic system if enabled.
 *   
 * @return  None
 *
 * @notes
 *
 * @author
 *  03/17/2010  Ron Proulx  - Created
 *
 **************************************************************************/
void fbe_raid_xor_set_debug_options(fbe_raid_siots_t *const siots_p,
                                                         fbe_xor_option_t *option_p,
                                                         fbe_bool_t b_generate_error_trace)
{
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* First check if we should trace any errors in the xor library.
     */
    if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_XOR_ERROR_TRACING))
    {
        /* At a minimum we will trace xor errors.
         */
        *option_p |= FBE_XOR_OPTION_DEBUG;

        /* If `generate error trace' is set or this is a metadata request, set the
         * `debug checksum errors' flag.
         */
        if ((b_generate_error_trace == FBE_TRUE)                        ||
            (fbe_raid_siots_is_metadata_operation(siots_p) == FBE_TRUE)    )
        {
            *option_p |= FBE_XOR_OPTION_DEBUG_ERROR_CHECKSUM;
        }
    }

    /* Now check if we should validate that */
    if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_XOR_VALIDATE_DATA))
    {
        *option_p |= FBE_XOR_OPTION_VALIDATE_DATA;

        /* Check if we should validate the data pattern of valid sectors.
         */
        if ( fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_PATTERN_DATA_CHECKING) &&
            !fbe_raid_siots_is_metadata_operation(siots_p)                                                              )
        {
            *option_p |= FBE_XOR_OPTION_CHECK_DATA_PATTERN;
        }
    }
	
    if(fbe_raid_geometry_is_parity_type(raid_geometry_p) && ((fbe_raid_siots_is_bva_verify(siots_p) || fbe_raid_siots_is_marked_for_incomplete_write_verify(siots_p) || fbe_raid_siots_is_metadata_operation( siots_p))))
    {
       *option_p |= FBE_XOR_OPTION_BVA;
    }
	
    return;
}
/******************************************
 * end of fbe_raid_xor_set_debug_options()
 ******************************************/


/****************************************************************
 * r5_check_lba_stamp_prefetch()
 ****************************************************************
 * @brief
 *  Issue a prefetch for the case where we are only checking the
 *  lba stamp.  
 * 
 *  We only prefetch when we are only checking lba stamp, since if
 *  we're checking the entire checksum we will be reading the entire block
 *  anyway.
 *
 * @param siots_p - Current siots.               
 *
 * @return
 *  None.             
 *
 * @author
 *  3/25/2008 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_xor_check_lba_stamp_prefetch(fbe_raid_siots_t * const siots_p)
{
    fbe_raid_fruts_t *current_fruts_ptr;
    fbe_u16_t fruts_block_offset = 0; /* Offset into current fruts. */
    fbe_u16_t sg_offset = 0;          /* Offset into current sg.    */
    fbe_u16_t prefetch_block = 0;     /* Current block number being prefetched. */
    fbe_sg_element_t *sg_ptr = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_raid_siots_get_read_fruts(siots_p, &current_fruts_ptr);
    fbe_raid_fruts_get_sg_ptr(current_fruts_ptr, &sg_ptr);

    /* Prefetch the first few blocks in the sg list.
     */
    while (prefetch_block < FBE_XOR_LBA_STAMP_PREFETCH_BLOCKS &&
           prefetch_block < siots_p->xfer_count)
    {
        if (fruts_block_offset >= current_fruts_ptr->blocks)
        {
            /* If we reach the end of this fruts then increment to next 
             * fruts. 
             */
            sg_offset = 0;
            fruts_block_offset = 0;
            current_fruts_ptr = fbe_raid_fruts_get_next(current_fruts_ptr);
            fbe_raid_fruts_get_sg_ptr(current_fruts_ptr, &sg_ptr);
        }
        else if (sg_offset >= sg_ptr->count)
        {
            /* If we reached the end of this sg element, increment to next SG.
             */
            sg_ptr++;
            sg_offset = 0;
        }

        if(RAID_COND( (sg_ptr == NULL) ||
                      (sg_offset >= sg_ptr->count) ) )
        {
            /*Split trace to two lines*/
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid: sg_ptr 0x%p is NULL or sg_offset 0x%x >=\n",
                                 sg_ptr, sg_offset);
			fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "sg_ptr->count 0x%x; siots_p = 0x%p\n",
                                 sg_ptr->count, siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Execute the prefetch.
         */
        FBE_XOR_PREFETCH_LBA_STAMP((fbe_sector_t*)(sg_ptr->address + sg_offset));

        /* Increment our offsets and prefetch block.
         */
        fruts_block_offset++;
        sg_offset += FBE_BE_BYTES_PER_BLOCK;
        prefetch_block++;
    } /* end while prefetch block < read fruts blocks. */

    /* Pause to let prefetch start.
     */
    csx_p_atomic_crude_pause();
    return status;
}
/******************************************
 * end r5_check_lba_stamp_prefetch() 
 ******************************************/

/****************************************************************
 * fruts_check_lba_stamp_prefetch()
 ****************************************************************
 * @brief
 *  Issue a prefetch for the case where we are only checking the
 *  lba stamp.  
 * 
 *  We only prefetch when we are only checking lba stamp, since if
 *  we're checking the entire checksum we will be reading the entire block
 *  anyway.
 *
 * @param siots_p - Current siots.               
 *
 * @return
 *  None.             
 *
 * @author
 *  02/07/11 Created. Daniel Cummins
 *
 ****************************************************************/
static fbe_status_t fbe_raid_xor_fruts_check_lba_stamp_prefetch(fbe_raid_fruts_t * fruts_ptr)
{
	fbe_raid_siots_t *siots_p;
    fbe_u16_t fruts_block_offset = 0; /* Offset into current fruts. */
    fbe_u16_t sg_offset = 0;          /* Offset into current sg.    */
    fbe_u16_t prefetch_block = 0;     /* Current block number being prefetched. */
    fbe_sg_element_t *sg_ptr = NULL;
    fbe_status_t status = FBE_STATUS_OK;

	siots_p = fbe_raid_fruts_get_siots(fruts_ptr);

	if (!siots_p) 
	{
		fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                "%s get NULL pointer from fbe_raid_fruts_get_siots \n", __FUNCTION__);

		return FBE_STATUS_GENERIC_FAILURE;
	}

    fbe_raid_fruts_get_sg_ptr(fruts_ptr, &sg_ptr);

    /* Prefetch the first few blocks in the sg list.
     */
    while (prefetch_block < FBE_XOR_LBA_STAMP_PREFETCH_BLOCKS &&
           prefetch_block < siots_p->xfer_count)
    {
        if (fruts_block_offset >= fruts_ptr->blocks)
        {
            /* If we reach the end of this fruts then increment to next 
             * fruts. 
             */
            sg_offset = 0;
            fruts_block_offset = 0;
            fruts_ptr = fbe_raid_fruts_get_next(fruts_ptr);
            fbe_raid_fruts_get_sg_ptr(fruts_ptr, &sg_ptr);
        }
        else if (sg_offset >= sg_ptr->count)
        {
            /* If we reached the end of this sg element, increment to next SG.
             */
            sg_ptr++;
            sg_offset = 0;
        }

        if(RAID_COND( (sg_ptr == NULL) ||
                      (sg_offset >= sg_ptr->count) ) )
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid: sg_ptr 0x%p is NULL or sg_offset 0x%x >= sg_ptr->count 0x%x; siots_p = 0x%p\n",
                                 sg_ptr, sg_offset, sg_ptr->count, siots_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Execute the prefetch.
         */
        FBE_XOR_PREFETCH_LBA_STAMP((fbe_sector_t*)(sg_ptr->address + sg_offset));

        /* Increment our offsets and prefetch block.
         */
        fruts_block_offset++;
        sg_offset += FBE_BE_BYTES_PER_BLOCK;
        prefetch_block++;
    } /* end while prefetch block < read fruts blocks. */

    /* Pause to let prefetch start.
     */
    csx_p_atomic_crude_pause();
    return status;
}
/******************************************
 * end fruts_check_lba_stamp_prefetch()
 ******************************************/


/***************************************************************************
 * fbe_raid_xor_fruts_check_checksum()
 ***************************************************************************
 * @brief
 *  This function checks checksums for a given fruts chain
 *
 * @param siots_p - Ptr to current fbe_raid_siots_t.
 *        fruts_p - Ptr to fruts chain to check checksum and lba stamps.
 *        b_force_crc_check - FBE_TRUE if CRC must be checked.
 *                            FBE_FALSE if routine should use check crc flag 
 *                                      to determine if crc needs checked.
 *   
 * @return fbe_status_t
 *
 *  @notes this function currently does not check the LBA stamps.. it only
 *  checks checksums.  Need to rework this function to check for LBA stamps
 *  and checksums on data drives and checksums only on parity drives.
 * 
 * @author
 *  02/07/11 Created. Daniel Cummins
 *
 **************************************************************************/
fbe_status_t fbe_raid_xor_fruts_check_checksum(fbe_raid_fruts_t * fruts_ptr,
                                               fbe_bool_t b_force_crc_check)
{    
    fbe_raid_siots_t *siots_p;
    fbe_status_t status;
    fbe_u32_t fru_index = 0;
    fbe_xor_execute_stamps_t execute_stamps;

    siots_p = fbe_raid_fruts_get_siots(fruts_ptr);

    if (!siots_p) 
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* This prefetch will keep us two blocks ahead for lba stamp checking.
     * We only prefetch when we are only checking lba stamp, since if we're 
     * checking the entire checksum we will be reading the entire block anyway. 
     *  
     * Mirror read verify should not try to prefetch since it is a verify based 
     * on physical addresses and the below prefetch fn only deals with logical 
     * addresses. 
     */
    if ((b_force_crc_check == FBE_FALSE) &&
        !fbe_raid_siots_should_check_checksums(siots_p) &&
        siots_p->algorithm != MIRROR_RD_VR)
    {
        status = fbe_raid_xor_check_lba_stamp_prefetch(siots_p);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid: failed while pre-fetching lba stamp: status = 0x%x, siots_p = 0x%p\n",
                                  status, siots_p);
            return status;
        }
    } /* End if prefetching */

    fbe_xor_lib_execute_stamps_init(&execute_stamps);

    /*******************************************************
     * Check the checksums one fru at a time.
     * This is necessary since in this case, we do not have 
     * a single SG that references the entire range.
     *******************************************************/

    while (fruts_ptr != NULL)
    {
        fbe_sg_element_t *sg_p = NULL;
        fbe_raid_fruts_get_sg_ptr(fruts_ptr, &sg_p);
        execute_stamps.fru[fru_index].sg = sg_p;
        execute_stamps.fru[fru_index].seed = fruts_ptr->lba;
        execute_stamps.fru[fru_index].count = fruts_ptr->blocks;
        execute_stamps.fru[fru_index].offset = 0;
        execute_stamps.fru[fru_index].position_mask = (1 << fruts_ptr->position);
        fru_index++;

        fruts_ptr = fbe_raid_fruts_get_next(fruts_ptr);
    }

    /* Setup the drive count and eboard pointer.
     */
    execute_stamps.data_disks = fru_index;
    execute_stamps.array_width = fbe_raid_siots_get_width(siots_p);
    execute_stamps.eboard_p = fbe_raid_siots_get_eboard(siots_p);
    execute_stamps.eregions_p = (siots_p->vcts_ptr == NULL) ? NULL : &siots_p->vcts_ptr->error_regions;

    /* Always check LBA stamps.
     */
 //   execute_stamps.option = FBE_XOR_OPTION_CHK_LBA_STAMPS;
    execute_stamps.option = FBE_XOR_OPTION_CHK_CRC;

    /* Set XOR debug options.
     */
    fbe_raid_xor_set_debug_options(siots_p, 
                                   &execute_stamps.option, 
                                   FBE_FALSE /* Don't generate error trace*/);

    /* Send xor command now. 
     */ 
    status = fbe_xor_lib_execute_stamps(&execute_stamps); 

    /* Save the status for later use.
     */
    fbe_raid_siots_set_xor_command_status(siots_p, execute_stamps.status);
    return status;
}
/**************************************
 * end fbe_raid_xor_fruts_check_checksum()
 **************************************/

/*!***************************************************************************
 *          fbe_raid_xor_read_check_checksum()
 *****************************************************************************
 * @brief   This function checks checksums for a read operation.
 *
 * @param siots_p - Ptr to current fbe_raid_siots_t.
 * @param b_force_crc_check - FBE_TRUE if CRC must be checked.
 *                            FBE_FALSE if routine should use check crc flag 
 *                                      to determine if crc needs checked.
 * @param  b_should_check_stamps - FBE_TRUE - Check the lba, write and time stamps
 *                                 FBE_FALSE - There are cases (sparing) where
 *                                  we should not check the stamps.
 *   
 * @return fbe_status_t
 *
 * @notes
 *
 * @author
 *  03/18/08: Rob Foley Created.
 *
 *****************************************************************************/
fbe_status_t fbe_raid_xor_read_check_checksum(fbe_raid_siots_t * const siots_p,
                                              fbe_bool_t b_force_crc_check,
                                              fbe_bool_t b_should_check_stamps)
{
    fbe_status_t             status = FBE_STATUS_OK;
    fbe_raid_fruts_t        *read_fruts_ptr = NULL;
    fbe_u32_t                fru_index = 0;
    fbe_xor_execute_stamps_t execute_stamps;    

    /* This prefetch will keep us two blocks ahead for lba stamp checking.
     * We only prefetch when we are only checking lba stamp, since if we're 
     * checking the entire checksum we will be reading the entire block anyway. 
     *  
     * Mirror read verify should not try to prefetch since it is a verify based 
     * on physical addresses and the below prefetch fn only deals with logical 
     * addresses. 
     */
    if ( (b_should_check_stamps == FBE_TRUE)              &&
         (b_force_crc_check == FBE_FALSE)                 &&
        !(fbe_raid_siots_should_check_checksums(siots_p)) &&
         (siots_p->algorithm != MIRROR_RD_VR)                )
    {
        status = fbe_raid_xor_check_lba_stamp_prefetch(siots_p);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid: failed while pre-fetching lba stamp: status = 0x%x, siots_p = 0x%p\n",
                                  status, siots_p);
            return status;
        }
    } /* End if prefetching */

    fbe_xor_lib_execute_stamps_init(&execute_stamps);

    /*******************************************************
     * Check the checksums one fru at a time.
     * This is necessary since in this case, we do not have 
     * a single SG that references the entire range.
     *******************************************************/

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    while (read_fruts_ptr != NULL)
    {
        fbe_sg_element_t *sg_p = NULL;
        fbe_raid_fruts_get_sg_ptr(read_fruts_ptr, &sg_p);
        execute_stamps.fru[fru_index].sg = sg_p;
        execute_stamps.fru[fru_index].seed = read_fruts_ptr->lba;
        execute_stamps.fru[fru_index].count = read_fruts_ptr->blocks;
        execute_stamps.fru[fru_index].offset = 0;
        execute_stamps.fru[fru_index].position_mask = (1 << read_fruts_ptr->position);
        fru_index++;

        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
    }

    /* Setup the drive count and eboard pointer.
     */
    execute_stamps.data_disks = fru_index;
    execute_stamps.array_width = fbe_raid_siots_get_width(siots_p);
    execute_stamps.eboard_p = fbe_raid_siots_get_eboard(siots_p);
    execute_stamps.eregions_p = (siots_p->vcts_ptr == NULL) ? NULL : &siots_p->vcts_ptr->error_regions;

    /* Default is no options.
     */
    execute_stamps.option = 0;

    /* If requested to check the stamps set the check lba_stamp flag.
     */
    if (b_should_check_stamps == FBE_TRUE)
    {
        execute_stamps.option |= FBE_XOR_OPTION_CHK_LBA_STAMPS;
    }

    /* Check checksums if required.
     */
    if ((b_force_crc_check == FBE_TRUE)                ||
        fbe_raid_siots_should_check_checksums(siots_p)    )
    {
        execute_stamps.option |= FBE_XOR_OPTION_CHK_CRC;
    }

    /* Tell xor to invalidate bad blocks that it finds for the
     * appropriate algorithms.
     */
    if (fbe_raid_siots_should_invalidate_blocks(siots_p))
    {

        execute_stamps.option |= FBE_XOR_OPTION_INVALIDATE_BAD_BLOCKS;
    }

    /* Check if we need to do anything.
     */
    if (execute_stamps.option != 0)
    {
        /* Set XOR debug options.
         */
        fbe_raid_xor_set_debug_options(siots_p, 
                                   &execute_stamps.option, 
                                   FBE_FALSE /* Don't generate error trace*/);

        /* Send xor command now. 
         */ 
        status = fbe_xor_lib_execute_stamps(&execute_stamps); 
    }
    else
    {
        /* Else there was nothing to do so mark success.
         */
        status = FBE_STATUS_OK;
        execute_stamps.status = FBE_XOR_STATUS_NO_ERROR;
    }

    /* Save the status for later use.
     */
    fbe_raid_siots_set_xor_command_status(siots_p, execute_stamps.status);

    return status;
}
/**************************************
 * end fbe_raid_xor_read_check_checksum()
 **************************************/

/***************************************************************************
 * fbe_raid_xor_striper_write_check_checksum()
 ***************************************************************************
 * @brief
 *  This function either check checksums and sets lba stamps (cached)
 *  or it sets checksums and lba stamps (host op).
 *
 * @param siots_p - Ptr to current fbe_raid_siots_t.
 *   
 * @return fbe_status_t
 *
 * @note    None.
 *
 **************************************************************************/
fbe_status_t fbe_raid_xor_striper_write_check_checksum(fbe_raid_siots_t * const siots_p)
{
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_u32_t drive_count = 0;
    fbe_status_t status;
    fbe_xor_execute_stamps_t execute_stamps;

    fbe_xor_lib_execute_stamps_init(&execute_stamps);

    /*******************************************************
     * Check the checksums one fru at a time.
     * This is necessary since in this case, we do not have 
     * a single SG that references the entire range.
     *******************************************************/
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);

    while (write_fruts_ptr != NULL)
    {
        fbe_sg_element_t *sg_p = NULL;
        
        fbe_raid_fruts_get_sg_ptr(write_fruts_ptr, &sg_p);
        execute_stamps.fru[drive_count].sg = sg_p;
        execute_stamps.fru[drive_count].seed = write_fruts_ptr->lba;
        execute_stamps.fru[drive_count].count = write_fruts_ptr->blocks;
        execute_stamps.fru[drive_count].offset = 0;
        execute_stamps.fru[drive_count].position_mask = (1 << write_fruts_ptr->position);
        drive_count++;

        write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);
    }

    /* Setup the drive count and eboard pointer.
     */
    execute_stamps.data_disks = drive_count;
    execute_stamps.array_width = fbe_raid_siots_get_width(siots_p);
    execute_stamps.eboard_p = fbe_raid_siots_get_eboard(siots_p);
    execute_stamps.eregions_p = &siots_p->vcts_ptr->error_regions;

    /* All requests need to generate the lba stamps and set the `write request'
     * flag.
     */
    execute_stamps.option = FBE_XOR_OPTION_WRITE_REQUEST;

    /* Generate the LBA stamps
     */
    execute_stamps.option |= FBE_XOR_OPTION_GEN_LBA_STAMPS;

    /* Buffered ops need to append crc and lba stamp since the data just 
     * arrived. 
     */
    if (fbe_raid_siots_is_buffered_op(siots_p))
    {
        execute_stamps.option |= FBE_XOR_OPTION_GEN_CRC;
    }
    else 
    {
        fbe_payload_block_operation_opcode_t opcode;

        if (fbe_raid_siots_should_check_checksums(siots_p))
        {
            /* Cached ops need to check the crc if the cache told us to do so.
             */
            execute_stamps.option |= FBE_XOR_OPTION_CHK_CRC;
        }

        fbe_raid_siots_get_opcode(siots_p, &opcode);
        if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA)
        {
            /* This is a corrupt data, let XOR know*/
            execute_stamps.option |= FBE_XOR_OPTION_CORRUPT_DATA;
        }

        if (fbe_raid_siots_is_corrupt_crc(siots_p) == FBE_TRUE)
        {
            /* Need to log corrupted sectors
             */
            execute_stamps.option |= FBE_XOR_OPTION_CORRUPT_CRC;
        }
    }


    /* Send xor command now. 
     */ 
    status = fbe_xor_lib_execute_stamps(&execute_stamps); 

    /* Save the status for later use.
     */
    fbe_raid_siots_set_xor_command_status(siots_p, execute_stamps.status);

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************************
 * end fbe_raid_xor_striper_write_check_checksums()
 **************************************************/

/***************************************************************************
 * fbe_raid_xor_mirror_write_check_checksum()
 ***************************************************************************
 * @brief
 *  This function either check checksums and sets lba stamps (cached)
 *  or it sets checksums and lba stamps (host op).
 *
 * @param siots_p - Ptr to current fbe_raid_siots_t.
 * @param b_should_generate_stamps - FBE_TRUE - Generate the lba, time and write
 *                                      stamps.
 *                                   FBE_FALSE - Do not generate stamps.
 *   
 * @return fbe_status_t
 *
 * @author
 *  04/01/2014  Ron Proulx  - Created.
 *
 **************************************************************************/
fbe_status_t fbe_raid_xor_mirror_write_check_checksum(fbe_raid_siots_t * const siots_p,
                                                      const fbe_bool_t b_should_generate_stamps)
{
    fbe_status_t                    status;
    fbe_sg_element_with_offset_t    host_sg_desc;
    fbe_xor_execute_stamps_t        execute_stamps;
    fbe_payload_block_operation_opcode_t opcode;

    /* There are a few opcodes where we will buffer the transfer like zeros.
     */
    fbe_raid_siots_get_opcode(siots_p, &opcode);

    /* We don't check checksums for buffered (i.e. non-data or internally 
     * generated data) operations.
     */
    if (fbe_raid_siots_is_buffered_op(siots_p)) {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: xor check checksum buffered op: %d not supported \n",
                             opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the host sg and generate the correct offset for this siots.  
     * Xor accepts the offset.
     */
    status = fbe_raid_siots_setup_cached_sgd(siots_p, &host_sg_desc);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) {
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "mirror: xor check checksum setup sgd failed - status: 0x%x\n",
                             status);
        return status;
    }

    /* Initialize the stamp request.
     */
    fbe_xor_lib_execute_stamps_init(&execute_stamps);

    /* Initialize the execute stamps.  Always use position 0.
     */
    execute_stamps.fru[0].sg = host_sg_desc.sg_element;
    execute_stamps.fru[0].seed = siots_p->parity_start;
    execute_stamps.fru[0].count = siots_p->parity_count;
    execute_stamps.fru[0].offset = host_sg_desc.offset / FBE_BE_BYTES_PER_BLOCK; /* offset in blocks for this siots */
    execute_stamps.fru[0].position_mask = (1 << 0);

    /* Setup the drive count and eboard pointer.
     */
    execute_stamps.data_disks = 1;
    execute_stamps.array_width = fbe_raid_siots_get_width(siots_p);
    execute_stamps.eboard_p = fbe_raid_siots_get_eboard(siots_p);
    execute_stamps.eregions_p = &siots_p->vcts_ptr->error_regions;

    /* All requests need to generate the lba stamps and set the `write request'
     * flag.
     */
    execute_stamps.option = FBE_XOR_OPTION_WRITE_REQUEST;

    /* Only if requested should we generate the lba_stamp
     */
    if (b_should_generate_stamps == FBE_TRUE) {
        execute_stamps.option |= FBE_XOR_OPTION_GEN_LBA_STAMPS;
    }

    /* If required check checksums.
     */
    if (fbe_raid_siots_should_check_checksums(siots_p)) {
        /* Cached ops need to check the crc if the cache told us to do so.
         */
        execute_stamps.option |= FBE_XOR_OPTION_CHK_CRC;
    }

    /* This is a corrupt data, let XOR know*/
    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA) {

        execute_stamps.option |= FBE_XOR_OPTION_CORRUPT_DATA;
    }

    /* Need to log corrupted sectors
     */
    if (fbe_raid_siots_is_corrupt_crc(siots_p) == FBE_TRUE) {

        execute_stamps.option |= FBE_XOR_OPTION_CORRUPT_CRC;
    }

    /* Send xor command now. 
     */ 
    status = fbe_xor_lib_execute_stamps(&execute_stamps); 

    /* Save the status for later use.
     */
    fbe_raid_siots_set_xor_command_status(siots_p, execute_stamps.status);

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/************************************************
 * end fbe_raid_xor_mirror_write_check_checksum()
 ************************************************/


/*!***************************************************************************
 *          fbe_raid_xor_mirror_preread_check_checksum()
 *****************************************************************************
 * @brief   This function checks checksums for a preread operation.
 *
 * @param siots_p - Ptr to current fbe_raid_siots_t.
 * @param b_force_crc_check - FBE_TRUE if CRC must be checked.
 *                            FBE_FALSE if routine should use check crc flag 
 *                                      to determine if crc needs checked.
 * @param  b_should_check_stamps - FBE_TRUE - Check the lba, write and time stamps
 *                                 FBE_FALSE - There are cases (sparing) where
 *                                  we should not check the stamps.
 *   
 * @return fbe_status_t
 *
 * @notes
 *
 * @author
 *  06/03/14: Deanna Heng Created.
 *
 *****************************************************************************/
fbe_status_t fbe_raid_xor_mirror_preread_check_checksum(fbe_raid_siots_t * const siots_p,
                                              fbe_bool_t b_force_crc_check,
                                              fbe_bool_t b_should_check_stamps)
{
    fbe_status_t             status = FBE_STATUS_OK;
    fbe_raid_fruts_t        *read_fruts_ptr = NULL;
    fbe_u32_t                fru_index = 0;
    fbe_xor_execute_stamps_t execute_stamps;    

    /* This prefetch will keep us two blocks ahead for lba stamp checking.
     * We only prefetch when we are only checking lba stamp, since if we're 
     * checking the entire checksum we will be reading the entire block anyway. 
     *  
     * Mirror read verify should not try to prefetch since it is a verify based 
     * on physical addresses and the below prefetch fn only deals with logical 
     * addresses. 
     */
    if ( (b_should_check_stamps == FBE_TRUE)              &&
         (b_force_crc_check == FBE_FALSE)                 &&
        !(fbe_raid_siots_should_check_checksums(siots_p)) &&
         (siots_p->algorithm != MIRROR_RD_VR)                )
    {
        status = fbe_raid_xor_check_lba_stamp_prefetch(siots_p);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid: failed while pre-fetching lba stamp: status = 0x%x, siots_p = 0x%p\n",
                                  status, siots_p);
            return status;
        }
    } /* End if prefetching */

    /*   
     * Unaligned writes to 4k drives requires a preread. Currently,
     * this preread spans over the entire write region. Only check the
     * checksum for the preread region that will not be overwritten.
     *
     *   *********
     *   *       * <-- preread region
     *   *=======*
     *   *       * <-- write region
     *   *=======*
     *   *       * <-- preread region
     *   *********
     *
     */


    fbe_xor_lib_execute_stamps_init(&execute_stamps);
    /*******************************************************
     * Check the checksums one fru at a time.
     * This is necessary since in this case, we do not have 
     * a single SG that references the entire range.
     *******************************************************/

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    while (read_fruts_ptr != NULL)
    {
        fbe_u32_t preread1_count = (fbe_u32_t)(siots_p->parity_start - read_fruts_ptr->lba);
        fbe_lba_t preread2_seed = siots_p->parity_start + siots_p->parity_count;
        fbe_u32_t preread2_offset = (fbe_u32_t)(preread2_seed - read_fruts_ptr->lba);
        fbe_u32_t preread2_count =  (fbe_u32_t)(read_fruts_ptr->lba + read_fruts_ptr->blocks - preread2_seed);
        fbe_sg_element_t *sg_p = NULL;
        
        if (preread1_count > 0)
        {
            fbe_raid_fruts_get_sg_ptr(read_fruts_ptr, &sg_p);
            execute_stamps.fru[fru_index].sg = sg_p;
            execute_stamps.fru[fru_index].seed = read_fruts_ptr->lba;
            execute_stamps.fru[fru_index].count = preread1_count;
            execute_stamps.fru[fru_index].offset = 0;
            execute_stamps.fru[fru_index].position_mask = (1 << read_fruts_ptr->position);
            fru_index++;
        }

        if (preread2_count > 0)
        {
            fbe_sg_element_t *sg_p = NULL; 
            fbe_raid_fruts_get_sg_ptr(read_fruts_ptr, &sg_p);
            execute_stamps.fru[fru_index].sg = sg_p;
            execute_stamps.fru[fru_index].seed = preread2_seed;
            execute_stamps.fru[fru_index].count = preread2_count;
            execute_stamps.fru[fru_index].offset = preread2_offset;
            execute_stamps.fru[fru_index].position_mask = (1 << read_fruts_ptr->position);
            fru_index++;
        }
        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
    }

    /* Setup the drive count and eboard pointer.
     */
    execute_stamps.data_disks = fru_index;
    execute_stamps.array_width = fbe_raid_siots_get_width(siots_p);
    execute_stamps.eboard_p = fbe_raid_siots_get_eboard(siots_p);
    execute_stamps.eregions_p = (siots_p->vcts_ptr == NULL) ? NULL : &siots_p->vcts_ptr->error_regions;

    /* Default is no options.
     */
    execute_stamps.option = 0;

    /* If requested to check the stamps set the check lba_stamp flag.
     */
    if (b_should_check_stamps == FBE_TRUE)
    {
        execute_stamps.option |= FBE_XOR_OPTION_CHK_LBA_STAMPS;
    }

    /* Check checksums if required.
     */
    if ((b_force_crc_check == FBE_TRUE)                ||
        fbe_raid_siots_should_check_checksums(siots_p)    )
    {
        execute_stamps.option |= FBE_XOR_OPTION_CHK_CRC;
    }

    /* Tell xor to invalidate bad blocks that it finds for the
     * appropriate algorithms.
     */
    if (fbe_raid_siots_should_invalidate_blocks(siots_p))
    {
        execute_stamps.option |= FBE_XOR_OPTION_INVALIDATE_BAD_BLOCKS;
    }

    /* Check if we need to do anything.
     */
    if (execute_stamps.option != 0)
    {
        /* Set XOR debug options.
         */
        fbe_raid_xor_set_debug_options(siots_p, 
                                       &execute_stamps.option, 
                                       FBE_FALSE /* Don't generate error trace*/);

        /* Send xor command now. 
         */ 
        status = fbe_xor_lib_execute_stamps(&execute_stamps); 
    }
    else
    {
        /* Else there was nothing to do so mark success.
         */
        status = FBE_STATUS_OK;
        execute_stamps.status = FBE_XOR_STATUS_NO_ERROR;

    }

    /* Save the status for later use.
     */
    fbe_raid_siots_set_xor_command_status(siots_p, execute_stamps.status);
    

    return status;
}
/**************************************
 * end fbe_raid_xor_mirror_preread_check_checksum()
 **************************************/

/***************************************************************************
 * fbe_raid_xor_write_log_header_set_stamps()
 ***************************************************************************
 * @brief
 *  This function either check checksums and sets lba stamps (cached)
 *  or it sets checksums and lba stamps (host op),
 *  but skips over any dead positions to support write logging.
 *
 * @param siots_p - Ptr to current fbe_raid_siots_t.
 * @param b_first_only - FBE_TRUE - This is a mirror write so only check the
 *                                  first write fruts
 *                       FBE_FALSE - Check all write fruts
 *   
 * @return fbe_status_t
 *
 * @notes
 *
 **************************************************************************/

fbe_status_t fbe_raid_xor_write_log_header_set_stamps(fbe_raid_siots_t * const siots_p,
                                                         const fbe_bool_t b_first_only)
{
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_u32_t drive_count = 0;
    fbe_status_t status;
    fbe_xor_execute_stamps_t execute_stamps;

    fbe_xor_lib_execute_stamps_init(&execute_stamps);

    /*******************************************************
     * Check the checksums one fru at a time.
     * This is necessary since in this case, we do not have 
     * a single SG that references the entire range.
     *******************************************************/
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);

    while (write_fruts_ptr != NULL)
    {
        fbe_sg_element_t *sg_p = NULL;
        
        if ((write_fruts_ptr->position != siots_p->dead_pos) &&
            (write_fruts_ptr->position != siots_p->dead_pos_2))
        {
            fbe_raid_fruts_get_sg_ptr(write_fruts_ptr, &sg_p);
            execute_stamps.fru[drive_count].sg = sg_p;
            execute_stamps.fru[drive_count].seed = write_fruts_ptr->lba;
            execute_stamps.fru[drive_count].count = write_fruts_ptr->blocks;
            execute_stamps.fru[drive_count].offset = 0;
            execute_stamps.fru[drive_count].position_mask = (1 << write_fruts_ptr->position);
            drive_count++;

            /* If `first only' is set it indicates that all write fruts have
             * the same buffer.  Thus only need to check checksum and generate
             * LBA stamp for first write fruts.
             */
            if (b_first_only)
            {
                break;
            }
        }

        write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);
    }

    /* Setup the drive count and eboard pointer.
     */
    execute_stamps.data_disks = drive_count;
    execute_stamps.array_width = fbe_raid_siots_get_width(siots_p);
    execute_stamps.eboard_p = fbe_raid_siots_get_eboard(siots_p);
    execute_stamps.eregions_p = &siots_p->vcts_ptr->error_regions;

    /* All requests need to generate the lba stamps and set the `write request'
     * flag.
     */
    execute_stamps.option = (FBE_XOR_OPTION_GEN_LBA_STAMPS | FBE_XOR_OPTION_WRITE_REQUEST | FBE_XOR_OPTION_GEN_CRC);

    /* Send xor command now. 
     */ 
    status = fbe_xor_lib_execute_stamps(&execute_stamps); 

    /* Save the status for later use.
     */
    fbe_raid_siots_set_xor_command_status(siots_p, execute_stamps.status);

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/**************************************
 * end fbe_raid_xor_write_log_header_set_stamps()
 **************************************/

/***************************************************************************
 * fbe_raid_xor_check_checksum()
 ***************************************************************************
 * @brief
 *  This function checks checksums for a read operation.
 *
 * @param siots_p - Ptr to current fbe_raid_siots_t.
 * @param option - Option to check with (checksum or lba stamp or both).
 *   
 * @return fbe_status_t
 *
 * @notes
 *
 * @author
 *  1/26/2010 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_raid_xor_check_checksum(fbe_raid_siots_t * const siots_p,
                                         fbe_xor_option_t option)
{
    fbe_status_t status;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_u32_t fru_index = 0;
    fbe_xor_execute_stamps_t execute_stamps;

    fbe_xor_lib_execute_stamps_init(&execute_stamps);

    /*******************************************************
     * Check the checksums one fru at a time.
     * This is necessary since in this case, we do not have 
     * a single SG that references the entire range.
     *******************************************************/

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    while (read_fruts_ptr != NULL)
    {
        fbe_sg_element_t *sg_p = NULL;
        fbe_raid_fruts_get_sg_ptr(read_fruts_ptr, &sg_p);
        execute_stamps.fru[fru_index].sg = sg_p;
        execute_stamps.fru[fru_index].seed = read_fruts_ptr->lba;
        execute_stamps.fru[fru_index].count = read_fruts_ptr->blocks;
        execute_stamps.fru[fru_index].offset = 0;
        execute_stamps.fru[fru_index].position_mask = (1 << read_fruts_ptr->position);
        fru_index++;

        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
    }

    /* Setup the drive count and eboard pointer.
     */
    execute_stamps.data_disks = fru_index;
    execute_stamps.array_width = fbe_raid_siots_get_width(siots_p);
    execute_stamps.eboard_p = fbe_raid_siots_get_eboard(siots_p);
    execute_stamps.eregions_p = (siots_p->vcts_ptr == NULL) ? NULL : &siots_p->vcts_ptr->error_regions;

    /* Set the option that the caller specified.
     */
    execute_stamps.option = option;

    /* Send xor command now. 
     */ 
    status = fbe_xor_lib_execute_stamps(&execute_stamps); 

    /* Save the status for later use.
     */
    fbe_raid_siots_set_xor_command_status(siots_p, execute_stamps.status);
    return status;
}
/**************************************
 * end fbe_raid_xor_check_checksum()
 **************************************/

/***************************************************************************
 * fbe_raid_xor_check_iots_checksum()
 ***************************************************************************
 * @brief
 *  This function checks checksums for a read operation.
 *
 * @param siots_p - Ptr to current fbe_raid_siots_t.
 *   
 * @return fbe_status_t
 *
 * @notes
 *
 * @author
 *  2/9/2010 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_raid_xor_check_iots_checksum(fbe_raid_siots_t * const siots_p)
{
    fbe_status_t status;
    fbe_xor_execute_stamps_t execute_stamps;
    fbe_sg_element_t *sg_p = NULL;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_sg_element_with_offset_t sgd;

    fbe_xor_lib_execute_stamps_init(&execute_stamps);

    /*******************************************************
     * Check the checksums on this iots. 
     * Note we only check checksums not lba stamps.
     *******************************************************/

    fbe_raid_iots_get_sg_ptr(iots_p, &sg_p);

    if (iots_p->host_start_offset > FBE_U32_MAX)
    {
       /* not expecting host start offset to exceed 32bit limit here
        */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: host start offset: 0x%llx exceeding 32bit limit here, IOTS: 0x%p\n",
                             (unsigned long long)iots_p->host_start_offset,
			     iots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_sg_element_with_offset_init(&sgd, (fbe_u32_t)iots_p->host_start_offset, sg_p, 
                                    NULL /* default increment fn */);
    fbe_raid_adjust_sg_desc(&sgd);


    /* Technically we could process a request larger that 2TB of data but
     * it is not expected.
     */
    if (iots_p->blocks > FBE_U32_MAX)
    {
       /* not expecting host start offset to exceed 32bit limit here
        */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: iots blocks 0x%llx exceeding 32bit limit here, IOTS: 0x%p\n",
                             (unsigned long long)iots_p->blocks, iots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*******************************************************
     * Check the checksums on this siots. 
     * Note we only check checksums not lba stamps.
     *******************************************************/
    execute_stamps.fru[0].sg = sgd.sg_element;
    execute_stamps.fru[0].seed = iots_p->lba;
    execute_stamps.fru[0].count = iots_p->blocks;
    execute_stamps.fru[0].offset = sgd.offset;
    execute_stamps.fru[0].position_mask = 1;

    /* Setup the drive count and eboard pointer.
     */
    execute_stamps.data_disks = 1;
    execute_stamps.array_width = fbe_raid_siots_get_width(siots_p);
    execute_stamps.eboard_p = fbe_raid_siots_get_eboard(siots_p);
    execute_stamps.eregions_p = (siots_p->vcts_ptr == NULL) ? NULL : &siots_p->vcts_ptr->error_regions;

    /* Set the option that the caller specified.
     */
    execute_stamps.option = FBE_XOR_OPTION_CHK_CRC;

    /* Set XOR debug options:
     *  o FBE_XOR_OPTION_DEBUG - Trace checksum errors
     *  o FBE_XOR_OPTION_DEBUG_ERROR_CHECKSUM - Generate error trace on checksum error
     *  o FBE_XOR_OPTION_VALIDATE_DATA - Validate data
     */
    execute_stamps.option |= (FBE_XOR_OPTION_DEBUG | FBE_XOR_OPTION_DEBUG_ERROR_CHECKSUM | FBE_XOR_OPTION_VALIDATE_DATA);

    /* Send xor command now. 
     */ 
    status = fbe_xor_lib_execute_stamps(&execute_stamps); 

    /* Save the status for later use.
     */
    fbe_raid_siots_set_xor_command_status(siots_p, execute_stamps.status);
    return status;
}
/**************************************
 * end fbe_raid_xor_check_iots_checksum()
 **************************************/

/***************************************************************************
 * fbe_raid_xor_check_siots_checksum()
 ***************************************************************************
 * @brief
 *  This function checks checksums for a read operation.
 *  We check just the checksums of the data for this siots.
 *
 * @param siots_p - Ptr to current fbe_raid_siots_t.
 *   
 * @return fbe_status_t
 *
 * @notes
 *
 * @author
 *  5/25/2010 - Created. Rob Foley
 * 
 **************************************************************************/

fbe_status_t fbe_raid_xor_check_siots_checksum(fbe_raid_siots_t * const siots_p)
{
    fbe_status_t status;
    fbe_xor_execute_stamps_t execute_stamps;
    fbe_sg_element_with_offset_t sgd;    

    status = fbe_raid_siots_setup_cached_sgd(siots_p, &sgd);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: failed to setup cache sgd: status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return status;
    }

    fbe_xor_lib_execute_stamps_init(&execute_stamps);

    /* Technically we could process a request larger that 2TB of data but
     * it is not expected.
     */
    if (siots_p->xfer_count > FBE_U32_MAX)
    {
       /* not expecting host start offset to exceed 32bit limit here
        */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "raid: siots xfer_count  0x%llx exceeding 32bit limit here, SIOTS: 0x%p\n",
                             (unsigned long long)siots_p->xfer_count, siots_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*******************************************************
     * Check the checksums on this siots. 
     * Note we only check checksums not lba stamps.
     *******************************************************/
    execute_stamps.fru[0].sg = sgd.sg_element;
    execute_stamps.fru[0].seed = siots_p->start_lba;
    execute_stamps.fru[0].count = siots_p->xfer_count;
    execute_stamps.fru[0].offset = sgd.offset / FBE_BE_BYTES_PER_BLOCK; /* offset in blocks */
    execute_stamps.fru[0].position_mask = 1;

    /* Setup the drive count and eboard pointer.
     */
    execute_stamps.data_disks = 1;
    execute_stamps.array_width = fbe_raid_siots_get_width(siots_p);
    execute_stamps.eboard_p = fbe_raid_siots_get_eboard(siots_p);
    execute_stamps.eregions_p = (siots_p->vcts_ptr == NULL) ? NULL : &siots_p->vcts_ptr->error_regions;

    /* Check the checksums (to detect invalidated blocks) and set the
     * flag to indicate that this is a `logical' request (i.e. cannot
     * validate the seed written to invalidated sectors).
     */
    execute_stamps.option = (FBE_XOR_OPTION_CHK_CRC |FBE_XOR_OPTION_LOGICAL_REQUEST);

    /* Set XOR debug options.
     */
    fbe_raid_xor_set_debug_options(siots_p, 
                                   &execute_stamps.option, 
                                   FBE_FALSE /* Don't generate error trace */);

    /* Send xor command now. 
     */ 
    status = fbe_xor_lib_execute_stamps(&execute_stamps); 

    /* Save the status for later use.
     */
    fbe_raid_siots_set_xor_command_status(siots_p, execute_stamps.status);
    return status;
}
/**************************************
 * end fbe_raid_xor_check_siots_checksum()
 **************************************/

/***************************************************************************
 * fbe_raid_xor_zero_buffers()
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

static fbe_status_t fbe_raid_xor_zero_buffers(fbe_raid_siots_t * const siots_p)
{
    fbe_status_t status;
    fbe_raid_fruts_t *write_fruts_ptr = NULL;
    fbe_xor_zero_buffer_t zero_buffer;
    fbe_u32_t data_pos;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    zero_buffer.disks = siots_p->data_disks;
    zero_buffer.offset = fbe_raid_siots_get_eboard_offset(siots_p);
    zero_buffer.status = FBE_STATUS_INVALID;

    /* Fill out the zero buffer structure from the write fruts.
     */
    fbe_raid_siots_get_write_fruts(siots_p, &write_fruts_ptr);
    for (data_pos = 0; data_pos < siots_p->data_disks; data_pos++)
    {
        fbe_sg_element_t *sg_p = NULL;
        fbe_lba_t write_start_lba = write_fruts_ptr->write_preread_desc.lba;
        fbe_block_count_t write_block_count = write_fruts_ptr->write_preread_desc.block_count;
        fbe_raid_fruts_get_sg_ptr(write_fruts_ptr, &sg_p);

        if (write_start_lba > write_fruts_ptr->lba) {
            /* Our write sg list needs to get incremented to the point where the write data starts.
             */
            fbe_sg_element_with_offset_t sg_descriptor;
            fbe_sg_element_with_offset_init(&sg_descriptor, 
                                            (fbe_u32_t)(write_start_lba - write_fruts_ptr->lba) * FBE_BE_BYTES_PER_BLOCK, 
                                            sg_p, NULL);
            fbe_raid_adjust_sg_desc(&sg_descriptor);
            zero_buffer.fru[data_pos].sg_p = sg_descriptor.sg_element;
            zero_buffer.fru[data_pos].offset = sg_descriptor.offset / FBE_BE_BYTES_PER_BLOCK;
        } else {
            zero_buffer.fru[data_pos].sg_p = sg_p;
            zero_buffer.fru[data_pos].offset = 0;
        }
        zero_buffer.fru[data_pos].count = write_block_count;
        zero_buffer.fru[data_pos].seed = write_start_lba;

        write_fruts_ptr = fbe_raid_fruts_get_next(write_fruts_ptr);
    }

    if (fbe_raid_geometry_is_raw_mirror(raid_geometry_p))
    {
        status = fbe_xor_lib_zero_buffer_raw_mirror(&zero_buffer);
    }
    else
    {
        status = fbe_xor_lib_zero_buffer(&zero_buffer);
    }

    if (status != FBE_STATUS_OK)
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "raid: failed to zero buffer with status = 0x%x, siots_p = 0x%p\n",
                                    status, siots_p);
    }
    /* Save the status for later use.
     */
    fbe_raid_siots_set_xor_command_status(siots_p, zero_buffer.status);

    return status;
}
/**************************************
 * end fbe_raid_xor_zero_buffers()
 **************************************/

/***************************************************************************
 * fbe_raid_xor_get_buffered_data()
 ***************************************************************************
 * @brief
 *  This gets the data for a buffered operation.
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

fbe_status_t fbe_raid_xor_get_buffered_data(fbe_raid_siots_t * const siots_p)
{
    fbe_payload_block_operation_opcode_t opcode;
    fbe_status_t status;

    fbe_raid_siots_get_opcode(siots_p, &opcode);

    if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_ZEROS) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS))
    {
        status = fbe_raid_xor_zero_buffers(siots_p);
    }
    else if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME)
    {
        /*! @note We don't support write sames yet.
         * When it is supported it simply copies from the user block to the 
         * destination blocks. 
         */
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                    "raid: %s errored becuase of unexpected opcode %d\n", __FUNCTION__, opcode);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_raid_xor_get_buffered_data()
 **************************************/

/*!***************************************************************************
 *          fbe_raid_xor_check_lba_stamp()
 *****************************************************************************
 * @brief   This function checks lba stamps for a read operation.
 *
 * @param siots_p - Ptr to current fbe_raid_siots_t.
 *   
 * @return fbe_status_t
 *
 * @notes We assume that if any error occurs we will break out.
 *        This routine does not use eboards since it is optimized for performance.
 *
 * @author
 *  4/24/2012 - Created. Rob Foley
 *
 *****************************************************************************/
fbe_status_t fbe_raid_xor_check_lba_stamp(fbe_raid_siots_t * const siots_p)
{
    fbe_status_t             status = FBE_STATUS_OK;
    fbe_raid_fruts_t        *read_fruts_ptr = NULL;
    fbe_xor_execute_stamps_t execute_stamps;    
    fbe_xor_error_t *eboard_p = fbe_raid_siots_get_eboard(siots_p);
    fbe_lba_t offset = eboard_p->raid_group_offset;

    /* This prefetch will keep us two blocks ahead for lba stamp checking.
     * We only prefetch when we are only checking lba stamp, since if we're 
     * checking the entire checksum we will be reading the entire block anyway. 
     *  
     * Mirror read verify should not try to prefetch since it is a verify based 
     * on physical addresses and the below prefetch fn only deals with logical 
     * addresses. 
     */
    if (siots_p->algorithm != MIRROR_RD_VR)
    {
        status = fbe_raid_xor_check_lba_stamp_prefetch(siots_p);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid: failed while pre-fetching lba stamp: status = 0x%x, siots_p = 0x%p\n",
                                  status, siots_p);
            return status;
        }
    } /* End if prefetching */

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    while (read_fruts_ptr != NULL)
    {
        fbe_sg_element_t *sg_p = NULL;
        fbe_raid_fruts_get_sg_ptr(read_fruts_ptr, &sg_p);
        execute_stamps.fru[0].sg = sg_p;
        execute_stamps.fru[0].seed = read_fruts_ptr->lba + offset;
        execute_stamps.fru[0].count = read_fruts_ptr->blocks;

        status = fbe_xor_lib_check_lba_stamp(&execute_stamps);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid: failed while checking lba stamp: status = 0x%x, siots_p = 0x%p\n",
                                  status, siots_p);
            return status;
        }

        if (execute_stamps.status != FBE_XOR_STATUS_NO_ERROR)
        {
            break;
        }
        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
    }

    /* Save the status for later use.
     */
    fbe_raid_siots_set_xor_command_status(siots_p, execute_stamps.status);
    return status;
}
/**************************************
 * end fbe_raid_xor_check_lba_stamp()
 **************************************/
/*************************
 * end file fbe_raid_xor.c
 *************************/


