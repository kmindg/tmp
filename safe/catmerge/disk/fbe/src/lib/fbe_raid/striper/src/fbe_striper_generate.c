/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_striper_generate.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the generate code for stripers.
 *
 * @version
 *   7/22/2009:  Created. rfoley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_striper_io_private.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/


/*****************************************************
 * STATIC FUNCTIONS 
 *****************************************************/
static fbe_status_t fbe_striper_gen_verify(fbe_raid_siots_t * siots_p);
static fbe_status_t fbe_striper_gen_read_write(fbe_raid_siots_t * siots_p);
static fbe_status_t fbe_striper_gen_zero(fbe_raid_siots_t * siots_p);
static fbe_status_t fbe_striper_generate_check_zeroed(fbe_raid_siots_t * const siots_p);
static fbe_u32_t r0_rb_limit_blocks_degraded(fbe_raid_siots_t * const siots_p,
                                          const fbe_u32_t array_width,
                                          const fbe_u16_t mirror_number,
                                          const fbe_u32_t blocks);
static fbe_u32_t r0_get_max_window_size(fbe_raid_siots_t *const siots_p,
                                       fbe_lba_t lba,
                                       fbe_u32_t flags);
static fbe_u32_t r0_limit_blocks_degraded(fbe_raid_siots_t *const siots_p,
                                       const fbe_u16_t array_width,
                                         fbe_u32_t blocks);

/****************************************************************
 * fbe_striper_generate_start()
 ****************************************************************
 *  @brief
 *    This is the entry point for the generate function.
 *    Determine the geometry and start the lock.
 *

 *    fbe_raid_siots_t *siots_ptr - ptr to new request.
 *
 *  @return VALUE:
 *    We return FBE_RAID_STATE_STATUS_EXECUTING.
 *
 *  @author
 *    1/18/00 - Created. Kevin Schleicher
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_striper_generate_start(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_payload_block_operation_opcode_t opcode;
    fbe_object_id_t object_id = fbe_raid_geometry_get_object_id(raid_geometry_p);

    fbe_raid_siots_get_opcode(siots_p, &opcode);

    siots_p->parity_pos = 0;

    /* Calculate appropriate siots request size and update iots.
     */
    if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS) ||
        (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_ZEROS))
    {
        status = fbe_striper_gen_read_write(siots_p);
    }
    else if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY) ||
             (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY) ||
             (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY) ||
             (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY) ||
             (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_SYSTEM_VERIFY) ||             
             (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_SPECIFIC_AREA)  ||             
             (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA))
             
    {
        /* Verify and remap have the same generate function.
         */
        status = fbe_striper_gen_verify(siots_p);
    }
    else if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO) ||
             (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_UNMARK_ZERO) ||
             (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_ZERO))
    {
        status = fbe_striper_gen_zero(siots_p);
    }
    else if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED)
    {
        status = fbe_striper_generate_check_zeroed(siots_p);
    }
    else
    {   
        /* If the requested block size or requested optimum size are zero then
         * return an error. 
         */
        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                "striper: %d unsupported opcode 0x%x\n", 
                                object_id, opcode);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_invalid_opcode);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    /* Check status from the above call to generate.
     */
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: unexpected generate error %d algorithm: 0x%x\n", 
                             status, siots_p->algorithm);
        fbe_raid_siots_transition(siots_p, fbe_raid_siots_unexpected_error);
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Verify that generate has set some important values.
     */
    if (RAID_COND(RG_NO_ALG == siots_p->algorithm))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "Striper: Unexpected siots_p->algorithm == RG_NO_ALG\n");
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    /* Uses Function Table found in RGDB to determine generate_start function.
     */
    if (RAID_COND(siots_p->common.state == (fbe_raid_common_state_t)fbe_striper_generate_start))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                      "Striper: Unexpected siots_p->common.state == fbe_striper_generate_start\n");
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }

    if(RAID_COND(((fbe_u32_t) - 1) == siots_p->drive_operations))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "parity: siots_p->drive_operations == INVALID\n");
        return FBE_RAID_STATE_STATUS_EXECUTING;
    }
    if (fbe_raid_geometry_is_raid0(raid_geometry_p)) {
        fbe_raid_siots_log_traffic(siots_p, "gen start");
    }

    return FBE_RAID_STATE_STATUS_EXECUTING;
}
/*******************************
 * end fbe_striper_generate_start()
 *******************************/

/****************************************************************
 * fbe_striper_gen_verify()
 ****************************************************************
 * @brief
 *  Generate a RAID 0 verify request by filling out 
 *  the siots.
 *
 * @param siots_p - Current sub request.
 *
 * @return fbe_status_t
 * 
 ****************************************************************/
static fbe_status_t fbe_striper_gen_verify(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_block_count_t blocks;
    /* This array width is the entire lun width for this
     * striped mirror unit.  Normally, at the striper
     * level we deal with width/2 for striped mirrors,
     * but in this case we use the total width.
     */
    fbe_u16_t array_width;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u32_t optimal_size;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_iots_t *iots_p = NULL;
#if 0
    fbe_bool_t fbe_lock_state = FBE_FALSE;
#endif
    fbe_block_count_t max_blocks_per_drive;

    iots_p = fbe_raid_siots_get_iots(siots_p);

    if(RAID_COND(NULL == iots_p))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "Striper: iots_p == NULL");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_raid_siots_get_optimal_size(siots_p, &optimal_size);
    fbe_raid_siots_get_opcode(siots_p, &opcode);

    /* Background requests are not allowed or expected for striped mirrors
     * because all background operations are performed at the mirror level.
     */
    if (RAID_COND(fbe_raid_geometry_is_raid10(raid_geometry_p) == FBE_TRUE))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: iots opcode: 0x%x lba: 0x%llx blks: 0x%llx background op for RAID-10 not allowed",
                                            opcode,
					    (unsigned long long)iots_p->lba,
					    (unsigned long long)iots_p->blocks);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Fetch the geometry information first.
     */
    status = fbe_striper_get_physical_geometry(raid_geometry_p,
                                               siots_p->start_lba,
                                               &siots_p->geo,
                                               &array_width);
    if (status != FBE_STATUS_OK)
    {
        /* There was an invalid parameter in the siots, return error now.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: invalid parameter from get phy geometry status: 0x%x",
                                            status);     
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(RAID_COND( 0 != (siots_p->geo.start_offset_rel_parity_stripe + 
                        siots_p->geo.blocks_remaining_in_parity) % 
                        (fbe_raid_siots_get_blocks_per_element(siots_p))))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                       "SIOTS not aligned with element size, start offset: %llx"
                                       "block remaining in parity: 0x%llx, element size 0x%x",
                                       (unsigned long long)siots_p->geo.start_offset_rel_parity_stripe,
                                       (unsigned long long)siots_p->geo.blocks_remaining_in_parity,
                                       fbe_raid_siots_get_blocks_per_element(siots_p));
        return FBE_STATUS_GENERIC_FAILURE;;
    }

    siots_p->parity_pos = fbe_raid_siots_get_parity_pos(siots_p);
    if (RAID_COND(siots_p->parity_pos == FBE_RAID_INVALID_PARITY_POSITION))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper : siots_p->parity_pos 0x%x == FBE_RAID_INVALID_PARITY_POSITION\n",
                                            siots_p->parity_pos);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    siots_p->start_pos = fbe_raid_siots_get_start_pos(siots_p);
    siots_p->data_disks = fbe_raid_siots_get_width(siots_p);

    /* Limit the request to maximum blocks per request.
     */
    blocks = FBE_MIN(siots_p->xfer_count, siots_p->geo.max_blocks);
    siots_p->xfer_count = siots_p->parity_count = blocks;
    siots_p->parity_start = siots_p->start_lba;

    /* Now limit it to the max `per drive' limit.
     */
    fbe_raid_geometry_get_max_blocks_per_drive(raid_geometry_p, &max_blocks_per_drive);
    siots_p->xfer_count = FBE_MIN(siots_p->xfer_count, max_blocks_per_drive);
    siots_p->parity_count = siots_p->xfer_count;

#if 0
    /* Do not allow writes that involve two parity regions.
     */
    {
        LOCK_REQUEST lock_ptr = fbe_raid_siots_get_iots(siots_p)->lock_ptr;
        fbe_lock_state = (lock_ptr == NULL) || ((lock_ptr->flags & LOCK_FLAG_HOLE) == 0) ||
                         ((lock_ptr->mid_start == 0) && (lock_ptr->mid_end == 0));
        if(RAID_COND(!fbe_lock_state))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "Striper: Write involve two parity regions"
                                            "lock_ptr %p, lock flags %d, lock start %llx"
                                            "lock end %llx, lock mid start %llx, lock mid end %llx",
                                            lock_ptr, lock_ptr->flags, lock_ptr->start, lock_ptr->end,
                                            lock_ptr->mid_start, lock_ptr->mid_end);
            return FBE_STATUS_GENERIC_FAILURE;;
        }
    }
#endif    

    /* This routine only handles aligned requests.
     * We only validate the request in checked builds.
     */
    if(RAID_COND(((siots_p->start_lba  % optimal_size) != 0) ||
                 ((siots_p->xfer_count % optimal_size) != 0)))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                              FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                              "Striper: request not aligned to optimal block size"
                              "start_lba %llx, xfer_count 0x%llx, optimal_size 0x%x",
                              (unsigned long long)siots_p->start_lba,
			  (unsigned long long)siots_p->xfer_count,
			  optimal_size);
        return FBE_STATUS_GENERIC_FAILURE;;
    }

    /* Adjust the iots to account for this request
     */
    fbe_raid_iots_dec_blocks(fbe_raid_siots_get_iots(siots_p), siots_p->xfer_count);
    siots_p->drive_operations = fbe_raid_siots_get_width(siots_p);

    /* Assign algorithm and the entry state.
     */
    siots_p->algorithm = RAID0_VR;
    siots_p->retry_count = FBE_RAID_DEFAULT_RETRY_COUNT;
    fbe_raid_siots_transition(siots_p, fbe_striper_verify_state0);

    return FBE_STATUS_OK;
}
/* end fbe_striper_gen_verify() */

/*!**************************************************************
 * fbe_striper_read_write_calculate()
 ****************************************************************
 * @brief
 *  Calculate some generic parameters for a read or write operation.
 *
 * @param siots_p - this I/O.
 * @param blocks - Number of blocks we are generating.
 *
 * @return fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_striper_read_write_calculate(fbe_raid_siots_t *siots_p,
                                              fbe_block_count_t blocks)
{
    fbe_raid_extent_t parity_range[FBE_RAID_MAX_PARITY_EXTENTS];
    fbe_u16_t data_disks;
    fbe_lba_t blocks_per_element;
    fbe_u16_t array_width;
    fbe_lba_t start_lba;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    start_lba = siots_p->start_lba;
    array_width = fbe_raid_siots_get_width(siots_p);
    if (RAID_COND(array_width == 0))
    {
         fbe_raid_siots_set_unexpected_error(siots_p,
                                             __LINE__, "fbe_striper_rd_wt_calc" ,
                                             "striper: siots_p = 0x%p, array width == 0\n",
                                             siots_p);
         return FBE_STATUS_GENERIC_FAILURE;
    }
    blocks_per_element = fbe_raid_siots_get_blocks_per_element(siots_p);
    siots_p->xfer_count = blocks;

    siots_p->data_disks = (fbe_u32_t)((blocks + (start_lba % blocks_per_element) +
                                       (blocks_per_element - 1)) / blocks_per_element);

    if ((siots_p->data_disks) > array_width)
    {
        siots_p->data_disks = array_width;
    }
    /* Fetch the parity range of this request
     */
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    fbe_raid_get_stripe_range(siots_p->start_lba,
                              siots_p->xfer_count,
                              fbe_raid_siots_get_blocks_per_element(siots_p),
                              data_disks,
                              FBE_RAID_MAX_PARITY_EXTENTS,    /* max extents */
                              parity_range);

    /* Fill out the parity range from the
     * values returned by fbe_raid_get_stripe_range
     */
    siots_p->parity_start = parity_range[0].start_lba;

    if (parity_range[1].size != 0)
    {
        /* Discontinuous parity range.
         * For parity count, we just take the entire parity range.
         */
        fbe_lba_t end_pt = (parity_range[1].start_lba +
                            parity_range[1].size - 1);
        siots_p->parity_count =(fbe_u32_t)( end_pt - parity_range[0].start_lba + 1);
    }
    else
    {
        siots_p->parity_count = parity_range[0].size;

        if (RAID_COND(siots_p->parity_count > siots_p->xfer_count))
        {
            fbe_raid_siots_set_unexpected_error(siots_p,
                           __LINE__, "fbe_striper_rd_wt_calc" ,
                           "Striper: siots_p->parity_count 0x%llx > siots_p->xfer_count 0x%llx\n",
                           (unsigned long long)siots_p->parity_count,
			   (unsigned long long)siots_p->xfer_count);
            return FBE_STATUS_GENERIC_FAILURE;
        }

    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_striper_read_write_calculate()
 ******************************************/

/****************************************************************
 * fbe_striper_limit_needs_alignment_write_request()
 ****************************************************************
 * @brief
 *  The write request may need to be limited to prevent overlap
 *  with the pre-read of other requests.
 *
 * @param   siots_p - Current sub request.
 * @param   blocks_p - Address of blocks to update
 *
 * @return  status
 *
 ****************************************************************/
static fbe_status_t fbe_striper_limit_needs_alignment_write_request(fbe_raid_siots_t *siots_p,
                                                                    fbe_block_count_t *blocks_p)
{
    fbe_status_t        status;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_raid_iots_t     *iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_block_count_t   blocks_remaining = 0;

    /*! @note Striper doesn't lock siots.  If another siots is required and
     *        the request is not aligned then we must reduce the request at the
     *        end so that the next siots doesn't overlap with pre-read.
     */
    fbe_raid_iots_get_blocks_remaining(iots_p, &blocks_remaining);
    if ((siots_p->xfer_count < blocks_remaining)                  &&
        fbe_raid_geometry_io_needs_alignment(raid_geometry_p,
                                             siots_p->start_lba,
                                             siots_p->xfer_count)     ) {
        /* Adjust the request. 
         */
        fbe_raid_siots_log_traffic(siots_p, "striper align write");
        status = fbe_raid_generate_align_io_end_reduce(raid_geometry_p->element_size,
                                                       siots_p->start_lba,
                                                       blocks_p);
        if (status != FBE_STATUS_OK) { 
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper limit needs align: align end failed with 0x%x\n",
                                            status);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Re-calculate the request
         */
        status = fbe_striper_read_write_calculate(siots_p, *blocks_p);
        if (status != FBE_STATUS_OK) { 
            fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper limit needs align: calculate failed with 0x%x\n",
                                            status);
            return FBE_STATUS_GENERIC_FAILURE; 
        }
    }

    return FBE_STATUS_OK;
}
/***********************************************************
 * end fbe_fbe_striper_limit_needs_alignment_write_request()
 ***********************************************************/

/*!**************************************************************
 * fbe_striper_get_max_read_write_blocks()
 ****************************************************************
 * @brief
 *  Determine the max block count for this read or write.
 *
 * @param siots_p - this I/O.
 * @param blocks_p - Number of blocks we can generate.
 *
 * @return fbe_status_t   
 *
 ****************************************************************/

fbe_status_t fbe_striper_get_max_read_write_blocks(fbe_raid_siots_t * siots_p,
                                                   fbe_block_count_t *blocks_p)
{
    fbe_status_t status;
    fbe_status_t return_status;
    fbe_block_count_t blocks = siots_p->xfer_count;
    fbe_raid_fru_info_t read_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1]; /* Add one for terminator */
    fbe_u16_t num_sgs[FBE_RAID_MAX_SG_TYPE];

    /* Use standard method to determine page size.
     */
    fbe_raid_siots_set_optimal_page_size(siots_p, 
                                         siots_p->data_disks, 
                                         (siots_p->data_disks * siots_p->parity_count));

    /* We should optimize so that we only do the below code 
     * for cases where the I/O exceeds a certain max size. 
     * This way for relatively small I/O that we know we can do we will 
     * not incurr the overhead for the below. 
     */
    if (fbe_raid_siots_is_small_request(siots_p) == FBE_TRUE)
    {
        return FBE_STATUS_OK;
    }
    
    do
    {
        if (siots_p->algorithm == RAID0_RD)
        {
            /* Next calculate the number of sgs of each type.
             */ 
            fbe_raid_fru_info_init_arrays(fbe_raid_siots_get_width(siots_p),
                                          &read_info[0], NULL, NULL);
            status = fbe_striper_read_get_fruts_info(siots_p, 
                                                     &read_info[0]);
            if (status != FBE_STATUS_OK) { return status; }
            status = fbe_striper_read_calculate_num_sgs(siots_p, 
                                                        &num_sgs[0], 
                                                        &read_info[0],
                                                        FBE_TRUE);

            /* If the request exceeds the sgl limit or the per-drive 
             * transfer limit split the request in half.
             */
            status = fbe_raid_siots_check_fru_request_limit(siots_p,
                                                            &read_info[0],
                                                            NULL,
                                                            NULL,
                                                            status);
        }
        else if (siots_p->algorithm == RAID0_WR)
        {
            /* Else if it is a write type of request use the write info also
             */
            fbe_raid_fru_info_t write_info[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];

            fbe_raid_fru_info_init_arrays(fbe_raid_siots_get_width(siots_p),
                                          &read_info[0],&write_info[0],NULL);

            if (fbe_raid_siots_get_raid_geometry(siots_p)->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
            {
                /* For R10, there is no need for pre-read at striper level. */
                status = fbe_striper_write_get_fruts_info(siots_p, NULL, &write_info[0], NULL, NULL);
            }
            else
            {
                status = fbe_striper_write_get_fruts_info(siots_p, &read_info[0], &write_info[0], NULL, NULL);
            }

            if (status != FBE_STATUS_OK) { return status; }
            /* Next calculate the number of sgs of each type.
             */ 
            status = fbe_striper_write_calculate_num_sgs(siots_p, 
                                                         &num_sgs[0], 
                                                         &read_info[0],
                                                         &write_info[0],
                                                         FBE_TRUE);

            /* If the request exceeds the sgl limit or the per-drive 
             * transfer limit split the request in half.
             */
            status = fbe_raid_siots_check_fru_request_limit(siots_p,
                                                            &read_info[0],
                                                            &write_info[0],
                                                            NULL,
                                                            status);
        }
        else
        {
             fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                  "striper: unexpected algorithm %d\n", 
                                  siots_p->algorithm);
             return FBE_STATUS_GENERIC_FAILURE;
        }
        
        if (status == FBE_STATUS_INSUFFICIENT_RESOURCES)
        {
            /* Exceeded sg limits, cut it in half.
             */
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_DEBUG_HIGH, 
                                 "lba: 0x%llx blocks: 0x%llx reduced to blocks: 0x%llx \n",
                                 (unsigned long long)siots_p->start_lba,
				 (unsigned long long)blocks,
				 (unsigned long long)(blocks / 2)); 
            blocks = blocks / 2;
            status = fbe_striper_read_write_calculate(siots_p, blocks);
            if (status != FBE_STATUS_OK) { return status; }

            /* Update status so that we re-validate the new request
             */
            status = FBE_STATUS_INSUFFICIENT_RESOURCES;
        }

    }
    while ((status == FBE_STATUS_INSUFFICIENT_RESOURCES) &&
           (blocks != 0));
    *blocks_p = blocks;

    if ((blocks == 0) || (status != FBE_STATUS_OK))
    {
        /* Something is wrong, we did not think we could generate any blocks.
         */
        return_status = FBE_STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        return_status = FBE_STATUS_OK;
    }
    return return_status;
}
/******************************************
 * end fbe_striper_get_max_read_write_blocks()
 ******************************************/


/****************************************************************
 * fbe_striper_gen_read_write()
 ****************************************************************
 * @brief
 *  Generate a RAID 0 read or write request by filling out the 
 *  siots.
 *
 * @param siots_p - Current sub request.
 *
 * @return fbe_status_t State status: executing, waiting
 *
 * @notes
 *  Modified the code to allow larger I/O sizes on the backend.
 *
 ****************************************************************/
static fbe_status_t fbe_striper_gen_read_write(fbe_raid_siots_t * siots_p)
{
    fbe_lba_t blocks_per_element;
    fbe_u16_t array_width;
    fbe_lba_t start_lba;
    fbe_block_count_t blocks;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_raid_iots_t *iots_p = NULL;
    fbe_block_count_t iots_blocks;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_status_t status;

    blocks = siots_p->xfer_count;

    iots_p = fbe_raid_siots_get_iots(siots_p);

    if (RAID_ERROR_COND(NULL == iots_p))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "Striper: iots_p == NULL");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_raid_siots_get_blocks(siots_p, &iots_blocks);
    fbe_raid_siots_get_opcode(siots_p, &opcode);

    /* Fetch the geometry information first.
     */
    status = fbe_striper_get_geometry(raid_geometry_p, siots_p->start_lba, &siots_p->geo);
    if (RAID_COND(status != FBE_STATUS_OK))
    {
        /* There was an invalid parameter in the siots, return error now.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "Striper: invalid status 0x%x from get geometry", status);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    /* Assign algorithm and the entry state.
     */
    switch(opcode)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
            siots_p->algorithm = RAID0_RD;
            fbe_raid_siots_transition(siots_p, fbe_striper_read_state0);
            break;

        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_ZERO:
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_ZEROS:

            /* We always transition to the write state.  Even
             * if the request isn't immediately startable.
             */
            siots_p->algorithm = RAID0_WR;
            fbe_raid_siots_transition(siots_p, fbe_striper_write_state0);
            break;

        default:
            /* Unexpected opcode.
             */
            fbe_raid_siots_transition(siots_p, fbe_raid_siots_invalid_opcode);
            break;

    } /* end switch on opcode */

    siots_p->start_pos = fbe_raid_siots_get_start_pos(siots_p);
    siots_p->retry_count = FBE_RAID_DEFAULT_RETRY_COUNT;

    status = fbe_striper_read_write_calculate(siots_p, blocks);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper generate: calculate failed with %d\n",
                                            status);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    /* We are limited in how large a backend sg we can allocate.
     * Determine if this exceeds any of our backend limits. 
     */
    status = fbe_striper_get_max_read_write_blocks(siots_p, &blocks);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper generate: fetch of max blocks failed with %d\n",
                                            status);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    /* Round the end back to element (aligned) boundary.
     */
    if ((siots_p->algorithm == RAID0_WR)                          &&
        fbe_raid_geometry_io_needs_alignment(raid_geometry_p,
                                             siots_p->start_lba,
                                             siots_p->xfer_count)     )
    {
        status = fbe_striper_limit_needs_alignment_write_request(siots_p, &blocks);
        if (status != FBE_STATUS_OK) 
        {
            return status;
        }
    }

    /* Init local storage variables.
     * The IO should not be greater than the max allowed.
     */
    start_lba = siots_p->start_lba;
    array_width = fbe_raid_siots_get_width(siots_p);
    blocks_per_element = fbe_raid_siots_get_blocks_per_element(siots_p);

    /* First limit the request to the geometry abstraction's value.
     */
    blocks =  FBE_MIN(blocks, siots_p->geo.max_blocks);

    if (RAID_COND( 0 == blocks ))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, 
                                            FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "Striper: blocks == 0");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Adjust the iots to account for this request
     */
    fbe_raid_iots_dec_blocks(fbe_raid_siots_get_iots(siots_p), siots_p->xfer_count);
    siots_p->drive_operations = siots_p->data_disks;
    /* Return the state status.
     */
    return FBE_STATUS_OK;
}
/* end fbe_striper_gen_read_write() */
/****************************************************************
 * fbe_striper_gen_zero()
 ****************************************************************
 * @brief
 *  Generate a RAID 0 zero request by filling out the siots.
 *
 * @param siots_p - Current sub request.
 *
 * @return
 *   fbe_status_t
 *
 * @author
 *   5/04/04 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_striper_gen_zero(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_block_count_t blocks_per_element;
    fbe_u16_t array_width;
    fbe_lba_t start_lba;
    fbe_lba_t end_lba;
    fbe_block_count_t blocks;
    fbe_bool_t b_is_aligned;
    fbe_raid_geometry_t *geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_payload_block_operation_opcode_t opcode;
    fbe_u16_t data_disks;
    fbe_block_count_t stripe_size;

    fbe_raid_siots_get_opcode(siots_p, &opcode);

    /* Fetch the geometry information first.
     * Note that our window size is essentially
     * unlimited since we're just zeroing.
     */
    status = fbe_striper_get_geometry(geometry_p, siots_p->start_lba, &siots_p->geo);
    if (status != FBE_STATUS_OK)
    {
        /* There was an invalid parameter in the siots, return error now.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "Striper: zero invalid status 0x%x from get geometry", status);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    siots_p->start_pos = fbe_raid_siots_get_start_pos(siots_p);
    siots_p->parity_pos = FBE_RAID_INVALID_POS;
    siots_p->retry_count = FBE_RAID_DEFAULT_RETRY_COUNT;

    /* Init local storage variables.
     */
    blocks = siots_p->xfer_count;
    start_lba = siots_p->start_lba;
    end_lba = start_lba + blocks - 1;
    array_width = fbe_raid_siots_get_width(siots_p);
    blocks_per_element = (fbe_block_count_t)fbe_raid_siots_get_blocks_per_element(siots_p);
    
    /* If we are not aligned to the optimal block size we need pre-reads.
     */
    b_is_aligned = fbe_raid_is_aligned_to_optimal_block_size(siots_p->start_lba,
                                                             blocks,
                                                             geometry_p->optimal_block_size);
    fbe_raid_geometry_get_data_disks(geometry_p, &data_disks);
    stripe_size = data_disks * blocks_per_element;
    if ((opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_UNMARK_ZERO) &&
		((b_is_aligned == FBE_FALSE) || (siots_p->start_lba % stripe_size) != 0 ||
         ((siots_p->start_lba + siots_p->xfer_count) % stripe_size) != 0 ))
    { 
        /* For unmark_zero we should be aligned.
         */
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: opcode: 0x%x generate not aligned.\n",
                             opcode);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* If we are a Raid 0 and not aligned to the optimal block size, use the regular write
     * path since pre-reads are needed. 
     * Raid 10s will allow the mirror to handle misalignment. 
     */
    if ((geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID0) &&
        (b_is_aligned == FBE_FALSE))
    {

        /* Perform a normal write. If we are large enough then try some optimization.
         */
        if (siots_p->xfer_count > (stripe_size * FBE_STRIPER_MIN_ZERO_STRIPE_OPTIMIZE))
        {
            /* If the start is aligned, then cut it off after the max even number of stripes.
             * This will allow us to do a write same for the aligned portion                .
             */
            if (((siots_p->start_lba % geometry_p->optimal_block_size) == 0) &&
                (siots_p->xfer_count > stripe_size))
            {
                siots_p->xfer_count -= (fbe_block_count_t)(((siots_p->start_lba + siots_p->xfer_count) % stripe_size));
            }
            else 
            {
                /* We know there is at least one full stripe.
                 * Limit to the end of the stripe. 
                 * This will allow subsequent requests to be stripe aligned at the beginning.
                 *  
                 * +----+  +----+  +----+  +----+  +----+
                 * |    |  |    |  |    |  |    |  |xxxx|
                 * |    |  |    |  |    |  |xxxx|  |xxxx|
                 * |    |  |    |  |    |  |xxxx|  |xxxx|
                 * +----+  +----+  +----+  +----+  +----+ <----- Cut here
                 * +----+  +----+  +----+  +----+  +----+ <----- This request is stripe
                 * |xxxx|  |xxxx|  |xxxx|  |xxxx|  |xxxx|        aligned at the beginning
                 * |xxxx|  |xxxx|  |xxxx|  |xxxx|  |xxxx|
                 * |xxxx|  |xxxx|  |xxxx|  |xxxx|  |xxxx|
                 * +----+  +----+  +----+  +----+  +----+ <----- We will also cut here.
                 * +----+  +----+  +----+  +----+  +----+        So the middle piece is
                 * |xxxx|  |xxxx|  |xxxx|  |xxxx|  |    |        stripe aligned.
                 * |xxxx|  |xxxx|  |xxxx|  |xxxx|  |    |
                 * |xxxx|  |xxxx|  |xxxx|  |    |  |    |
                 * +----+  +----+  +----+  +----+  +----+
                 *  
                 *  
                 */
                siots_p->xfer_count = (fbe_block_count_t)FBE_MIN(siots_p->xfer_count, 
                                              stripe_size - (siots_p->start_lba % stripe_size));
            }
        }
        /* Clear this flag since the below code will re-determine the 
         * I/O size and will re-determine if this needs to be set. 
         */
        return fbe_striper_gen_read_write(siots_p);
    }
    else
    {
        /* Fetch the parity range of this request
         */
        fbe_raid_extent_t parity_range[FBE_RAID_MAX_PARITY_EXTENTS];
        fbe_u16_t data_disks;

        /* Update the info in fbe_raid_siots_t.
         */
        if (RAID_COND( 0 == blocks ))
        {
            fbe_raid_siots_set_unexpected_error(siots_p, 
                                                FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                                "Striper: blocks == 0");
            return FBE_STATUS_GENERIC_FAILURE;
        }

        siots_p->xfer_count = blocks;

        siots_p->data_disks = (fbe_u32_t)((blocks + (start_lba % blocks_per_element) +
                                         (blocks_per_element - 1)) / blocks_per_element);

        if ((siots_p->data_disks) > array_width)
        {
            siots_p->data_disks = array_width;
        }

        /* Now update data disks based on striper type.
         */
        fbe_raid_geometry_get_data_disks(geometry_p, &data_disks);
        fbe_raid_get_stripe_range(siots_p->start_lba,
                                  siots_p->xfer_count,
                                  fbe_raid_siots_get_blocks_per_element(siots_p),
                                  data_disks,
                                  FBE_RAID_MAX_PARITY_EXTENTS, /* max extents */
                                  parity_range);

        /* Fill out the parity range from the
         * values returned by fbe_raid_get_stripe_range
         */
        siots_p->parity_start = parity_range[0].start_lba;

        if (parity_range[1].size != 0)
        {
            /* Discontinuous parity range.
             * For parity count, we just take the entire parity range.
             */
            fbe_lba_t end_pt = (parity_range[1].start_lba +
                              parity_range[1].size - 1);
            siots_p->parity_count =(fbe_u32_t)( end_pt - parity_range[0].start_lba + 1);
        }
        else
        {
            siots_p->parity_count = parity_range[0].size;

            if (RAID_COND(siots_p->parity_count > siots_p->xfer_count))
            {
                fbe_raid_siots_set_unexpected_error(siots_p,
                           FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                           "Striper: siots_p->parity_count 0x%llx > siots_p->xfer_count 0x%llx\n",
                           (unsigned long long)siots_p->parity_count,
			   (unsigned long long)siots_p->xfer_count);
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }

    /* Adjust the iots to account for this request
     */
    fbe_raid_iots_dec_blocks(fbe_raid_siots_get_iots(siots_p), siots_p->xfer_count);
    siots_p->drive_operations = siots_p->data_disks;
    
    siots_p->algorithm = RG_ZERO;
    fbe_raid_siots_transition(siots_p, fbe_striper_zero_state0);
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_striper_gen_zero()
 *****************************************/
#if 0
/****************************************************************
 * r0_rb_limit_blocks_degraded()
 ****************************************************************
 * @brief
 *  For this striper, determine the number of blocks we are
 *  allowed to generate to this mirrored pair.
 *

 * @param siots_p - Current siots.
 * @param array_width - Array width of this request.
 * @param mirror_index - The number of the mirrored pair to
 *                       get the min RB offset for.
 * @param blocks - Current blocks for this rb request.
 *
 * @return
 *  Returns the new blocks value for this rb request limited
 *  based on rb checkpoints in this mirror.
 *
 * @author
 *  02/15/07 - Created.  Rob Foley
 *
 ****************************************************************/

static fbe_u32_t r0_rb_limit_blocks_degraded( fbe_raid_siots_t * const siots_p,
                                           const fbe_u32_t array_width,
                                           const fbe_u16_t mirror_index,
                                           const fbe_u32_t input_blocks )
{
    fbe_u32_t blocks = input_blocks; /* Limited blocks value to return. */
    fbe_lba_t physical_end = 0;        /* End addr of LUN partition on RG. */
    fbe_u16_t mirror_pos;            /* Position within this mirror. */
    
    /* The physical end address is the pba on the RAID group
     * where this LUN ends.
     */
    physical_end = dba_unit_get_physical_end_address(adm_handle, RG_GET_UNIT(siots_p));
    
    /* Loop over all the disks in this mirrored pair.
     * mirror_pos is the mirrored pair position.
     */
    for ( mirror_pos = 0; 
          mirror_pos < RG_MIRROR_ARRAY_WIDTH(fbe_raid_siots_get_raid_geometry(siots_p)); 
          mirror_pos++ )
    {
        fbe_u16_t array_pos; /* Position relative to the array. */
        fbe_lba_t address_offset = rg_siots_get_addr_offset(siots_p);
        fbe_lba_t rb_offset;

        /* Convert the position within the mirror to the position within the array.
         */
        if ( RG_STRIPED_MIRROR_REDUNDANCY(fbe_raid_siots_get_raid_geometry(siots_p)) == 1 )
        {
            /* In N-way the array position and position are the same.
             */
            array_pos = mirror_pos;
        }
        else
        {
            /* Non N-way we only have a primary and secondary.
             */
            array_pos = (mirror_pos == 0) ? mirror_index : mirror_index + (array_width / 2);
        }
        /* Use the array position to get this mirror position's rebuild checkpoint.
         */
        rb_offset = RG_RB_CHKPT(fbe_raid_siots_get_raid_geometry(siots_p),RG_GET_UNIT(siots_p), array_pos);
        
        /* Found an offset less than minimum. Save position and offset.
         */
        if ( rb_offset < physical_end )
        {
            /* We know this position is degraded, now limit the 
             * request based on the rb checkpoint.
             */
            if ( rb_offset <= address_offset )
            {
                /* Offset is below address offset.
                 * Rebuild is in another lun in this RG.
                 */
                rb_offset = 0;
            }
            else if ( !RG_IS_MIRROR_GROUP(RG_SUB_IOTS_RGDB(siots_p)) )
            {
                /* Offset is within this lun.
                 * Just normalize the rb_offset to be sans address offset,
                 * so we have a unit relative physical offset.
                 */
                rb_offset = (rb_offset - address_offset);
            }

            if ( siots_p->start_lba >= rb_offset )
            {
                fbe_bool_t rebuild_log_needed = FBE_FALSE;
                fbe_bool_t set_clean_flag = FBE_FALSE;
                fbe_u16_t dead_pos = array_pos;
                
                /* Check to see if the degraded FRU has a rebuild log and if so, if
                 * the I/O hits a clean and/or dirty region of the FRU.
                 * The block count and degraded offset are adjusted to account for
                 * this.  dead_pos may be set to RG_INVALID_DEAD_POS if the area is
                 * completely clean.
                 */
                blocks = rg_rl_limit_physical( siots_p->start_lba,
                                               blocks,
                                               &dead_pos,
                                               &rb_offset,
                                               (fbe_u32_t)fbe_raid_siots_get_blocks_per_element(siots_p),
                                               RG_GET_IORB(siots_p)->opcode,
                                               RG_GET_UNIT(siots_p),
                                               &rebuild_log_needed,
                                               &set_clean_flag );
            }

            /* If we are spanning the rebuild checkpoint, then break up
             * this request into non-degraded and degraded pieces.
             * This is necessary so that any equalize request will
             * be completely non-degraded or completely degraded.
             */
            if ( siots_p->start_lba < rb_offset &&
                 siots_p->start_lba + blocks > rb_offset )
            {
                /* Set xfer_count to the non-degraded portion.
                 * The next time we generate, we'll pick up the 
                 * degraded portion.
                 */
                blocks = (fbe_u32_t)(rb_offset - siots_p->start_lba);
            }
        } /* end if rb_offset < physical_end. */

    } /* for all positions in the mirrored pair. */
    return blocks;
}
/***************************************
 * End of r0_rb_limit_blocks_degraded()
 ***************************************/
/**************************************************************************
 * r0_get_max_window_size()
 **************************************************************************
 *
 * @brief
 *  Calculates the maximum window size in blocks 
 *  to allow larger I/O sizes on the backend.
 *

 * @param siots_p - Current SIOTS.
 * @param lba - Host address.
 * @param flags - Valid flags are
 *                       LU_RAID_EXPANSION and LU_RAID_USE_PHYSICAL.
 *    
 * @return VALUES:
 * @param fbe_u32_t - Returns max window size in blocks.
 *
 * @author
 *  05/08/06  SM - Created
 *
 ***************************************************************************/
fbe_u32_t r0_get_max_window_size(fbe_raid_siots_t *const siots_p,
                               fbe_lba_t lba,
                               fbe_u32_t flags)
{
    fbe_u32_t array_width;            /* Array width. */
    fbe_u32_t blocks_per_element;     /* Number of blocks per element. */
    fbe_u32_t blocks;                 /* Maximum blocks to be used in backend. 
                                     */
    fbe_u32_t unit = fbe_raid_siots_get_iots(siots_p)->unit;
    fbe_bool_t host_transfer = RG_HOST_OP(siots_p);
    fbe_bool_t backfill = RG_BACKFILL_OP(siots_p);
   
    if (flags & LU_RAID_EXPANSION) 
    {
        /* When LU_RAID_EXPANSION flag is set we are
         * forced to use the new width .
         */
        array_width = dba_unit_get_num_frus(adm_handle, unit);
    }
    else if (flags & LU_RAID_USE_PHYSICAL)
    {
        /* We are forced to use the physical checkpoint
         * instead of the logical.
         * Since the physical checkpoint is relative to the new address offset
         * we need to add this to the given unit relative pba to compare apples
         * to apples.
         */
        if (RAID_COND(!dba_unit_is_expanding(adm_handle, unit)))
        {
            fbe_raid_siots_set_unexpected_error(siots_p,
                           FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                           "Striper: Unexpected - unit is not expanding , unit %d", unit);
            return FBE_RAID_STATE_STATUS_EXECUTING;
        }

        array_width = DBA_LU_ARRAY_WIDTH_PHYS(adm_handle,unit,
                                              dba_unit_get_address_offset
                                              (adm_handle,unit));
    }
    else
    {
        /* Array width already includes intelligence 
         * about expansion checkpoint.
         */
        array_width = DBA_LU_ARRAY_WIDTH(adm_handle, unit, lba);
    }

    /* Limit the width the number of data disks since this could be a r10.
     */
    array_width = dba_unit_get_data_disks(dba_unit_get_type(adm_handle, unit), 
                                          array_width);

    blocks_per_element = dba_unit_get_sectors_per_element(adm_handle, unit);

    /* Calculating backend limits for the current IO based on the lun 
     * bound on raid group. We considered here max backend limit as 
     * 1024(FBE_RAID_MAX_BACKEND_IO_BLOCKS) blocks.
     */
    blocks = array_width * FBE_RAID_MAX_BACKEND_IO_BLOCKS;

    /* Limit this request.
     */
    blocks = rg_get_transfer_limit(unit,
                                   array_width,
                                   blocks,
                                   lba,
                                   host_transfer,
                                   backfill,
                                   RAID0_HOST_IO_CHECK_COUNT);

    if (blocks_per_element > RG_MAX_ELEMENT_SIZE)
    {
        /* Limit request for luns of large element size (>128).
         */
        fbe_u32_t data_remainder;
        /* Don't access more than RG_MAX_ELEMENT_SIZE sectors per drive.
         */
        data_remainder = (fbe_u32_t)(blocks_per_element - (lba % blocks_per_element));
        if (data_remainder > RG_MAX_ELEMENT_SIZE)
        {
            /* We have to limit ourselves to only 
             * RG_MAX_ELEMENT_SIZE sectors per drive.
             */
            data_remainder = RG_MAX_ELEMENT_SIZE;
        }
        else
        {
            data_remainder += FBE_MIN(RG_MAX_ELEMENT_SIZE,blocks_per_element - data_remainder);
        }
        blocks = FBE_MIN(blocks, data_remainder);
    }

    /* For small element sizes, it is possible that if our blocks count is 
     * too high, then we may try to allocate too many SG elements.
     * The function rg_get_max_blocks() is get called to prevent this.
     */
    if(blocks_per_element < BM_STD_BUFFER_SIZE)
    {
        blocks = rg_get_max_blocks(siots_p,
                                   lba,
                                   blocks,
                                   array_width,
                                   FBE_TRUE);
    }
    return blocks;
}
/**************************************************************************
 * end of r0_get_max_window_size()
 **************************************************************************/
/****************************************************************
 * r0_limit_blocks_degraded()
 ****************************************************************
 * @brief
 *  Check the degraded situation of this striped mirror.
 *  The purpose of this function is to help break the siots
 *  across rebuild checkpoints.  This is done by
 *  returning the max blocks we can generate.
 *

 * @param siots_p - Current siots.
 * @param array_width - Array width of this request.
 * @param block_limit - Current block limit for this request.
 *
 * @return
 *  Returns the max number of blocks 1..block_limit
 *  that this siots is allowed to generate.
 *
 * @author
 *  02/05/07 - Created.  Rob Foley
 *
 ****************************************************************/
static fbe_u32_t r0_limit_blocks_degraded( fbe_raid_siots_t *const siots_p,
                                        const fbe_u16_t array_width,
                                        fbe_u32_t block_limit )
{
    fbe_u16_t pos;
    fbe_lba_t physical_end;   /* End addr of LUN partition on RG. */
    fbe_lba_t address_offset; /* Start addr of LUN partition on RG. */
    address_offset = rg_siots_get_addr_offset(siots_p);

    /* The physical end address is the pba on the RAID group
     * where this LUN ends.
     * Take off the address offset, since the degraded lba we 
     * compare with does not have 
     */
    physical_end = dba_unit_get_physical_end_address(adm_handle, RG_GET_UNIT(siots_p));
    
    /* Loop over all the drives and limit the request based on 
     * any degraded positions that we find.
     */
    for (pos = 0; pos < array_width; pos++)
    {
        /* Get the current degraded offset for this array position.
         */
        fbe_lba_t degraded_offset;
        degraded_offset = RG_RB_CHKPT(fbe_raid_siots_get_raid_geometry(siots_p), 
                                      RG_GET_UNIT(siots_p), 
                                      pos);

        /* If the degraded_offset is within the LUN, then this
         * position must be degraded.
         * Let's see if we need to limit the I/O based on this.
         */
        if ( degraded_offset < physical_end )
        {                
            /* This array position is degraded.
             * Now limit the request based on the rebuild checkpoint and 
             * rebuild log regions.
             */
            fbe_bool_t rebuild_log_needed = FBE_FALSE;
            fbe_bool_t set_clean_flag = FBE_FALSE;
            fbe_u32_t new_blocks; /* New blocks returned by rg_calc_degraded_range(). */
            fbe_u16_t degraded_pos = RG_INVALID_DEAD_POS;
            
            /* Adjust the degraded offset by address offset if necessary.
             */
            if (degraded_offset <= address_offset)
            {
                /* Offset is below address offset.
                 * Rebuild is in another lun in this RG.
                 */
                degraded_offset = 0;
            }
            else
            {
                /* Offset is within this lun.
                 * Just normalize the degraded_offset to be sans address offset,
                 * so we have a unit relative physical offset.
                 */
                degraded_offset = (degraded_offset - address_offset);
            }
            
            /* Copy pos to degraded_pos, since the below function might
             * change this variable.
             */
            degraded_pos = pos;
            
            /* Make sure the degraded position was set.
             */
            if (RAID_COND(RG_INVALID_DEAD_POS == degraded_pos))
            {
                fbe_raid_siots_set_unexpected_error(siots_p,
                           FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                           "Striper: degraded_pos != RG_INVALID_DEAD_POS");
                return FBE_RAID_STATE_STATUS_EXECUTING;
            }

            /* Determine the new limit for the I/O based on this position.
             */
            new_blocks = rg_calc_degraded_range( siots_p->start_lba,
                                                 block_limit,
                                                 &degraded_pos,
                                                 RG_INVALID_FIELD /* Parity position not used. */,
                                                 siots_p->start_pos,
                                                 degraded_offset,
                                                 fbe_raid_siots_get_width(siots_p),
                                                 (fbe_u16_t) fbe_raid_siots_get_blocks_per_element(siots_p),
                                                 (fbe_u32_t) RG_GET_IORB(siots_p)->opcode,
                                                 fbe_raid_siots_get_iots(siots_p)->unit,
                                                 &rebuild_log_needed,
                                                 &set_clean_flag,
                                                 0 /* Number of parity positions. */ );

            /* Limit the block_limit based on the new block limit. 
             */
            block_limit = FBE_MIN(block_limit, new_blocks);

        } /* End degraded_offset is not RG_INVALID_DEGRADED_OFFSET. */
    } /* End for all positions. */

    /* Since this is a striper, there are no dead positions.
     * The above code might have set the dead position, so reset it now.
     */
    rg_siots_dead_pos_set(siots_p, RG_FIRST_DEAD_POS, RG_INVALID_DEAD_POS );
    
    return block_limit;
}
/*******************************
 * end r0_limit_blocks_degraded()
 *******************************/
#endif // commented out


/*!**************************************************************************
 * fbe_striper_generate_validate_siots()
 ****************************************************************************
 * @brief  Validate the fields of the siots.
 *
 * @param siots_p - Pointer to this I/O.
 * 
 * @return FBE_STATUS_OK on success, FBE_STATUS_GENERIC_FAILURE on failure.
 *
 **************************************************************************/

fbe_status_t fbe_striper_generate_validate_siots(fbe_raid_siots_t *siots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u16_t width = fbe_raid_siots_get_width(siots_p);

    if (RAID_COND(siots_p->start_pos >= width))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "striper: error start position: 0x%x width: 0x%x\n",
                             siots_p->start_pos, width);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (RAID_COND(siots_p->data_disks > width))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "striper: data_disks %d is greater than width. %d\n", 
                             siots_p->data_disks, width);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (RAID_COND(siots_p->data_disks == 0))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "striper: data_disks is zero. %d\n", siots_p->data_disks);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (RAID_COND(siots_p->xfer_count == 0))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "striper: xfer_count is unexpected. %llu\n",
			     (unsigned long long)siots_p->xfer_count);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (RAID_COND(siots_p->start_pos != siots_p->geo.position[siots_p->start_pos]))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "start position %d not equal to position[start_pos] %d\n", 
                             siots_p->start_pos, siots_p->geo.position[siots_p->start_pos]);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (RAID_COND(width == 0))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "striper: width is unexpected. %d\n", width);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (RAID_COND(siots_p->data_disks > FBE_RAID_MAX_DISK_ARRAY_WIDTH))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "striper: data_disks is unexpected. %d\n", siots_p->data_disks);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    if (RAID_COND(siots_p->parity_count == 0))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "striper: parity_count is zero\n");
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
/**************************************
 * end fbe_striper_generate_validate_siots()
 **************************************/
/****************************************************************
 * fbe_striper_generate_check_zeroed()
 ****************************************************************
 * @brief
 *  Generate a striper check zero request.
 *  
 * @param siots_p - Current sub request. 
 *
 * @return fbe_status_t
 *
 ****************************************************************/
static fbe_status_t fbe_striper_generate_check_zeroed(fbe_raid_siots_t * const siots_p)
{
    fbe_status_t status;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u16_t array_width;
    fbe_raid_iots_t *iots_p = fbe_raid_siots_get_iots(siots_p);

    fbe_raid_siots_get_opcode(siots_p, &opcode);

     /* Fetch the geometry information first.
     */
    status = fbe_striper_get_physical_geometry(raid_geometry_p,
                                               siots_p->start_lba,
                                               &siots_p->geo,
                                               &array_width);
    if (status != FBE_STATUS_OK)
    {
        /* There was an invalid parameter in the siots, return error now.
         */
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: invalid parameter from get phy geometry status: 0x%x",
                                            status);     
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(RAID_COND( 0!= (siots_p->geo.start_offset_rel_parity_stripe + 
                       siots_p->geo.blocks_remaining_in_parity) % 
                       (fbe_raid_siots_get_blocks_per_element(siots_p))))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity:  SIOTS not aligned with element size, start offset: %llx"
                             "block remaining in parity: %llu, element size %d\n",
                             (unsigned long long)siots_p->geo.start_offset_rel_parity_stripe,
                             (unsigned long long)siots_p->geo.blocks_remaining_in_parity,
                             fbe_raid_siots_get_blocks_per_element(siots_p));
        return FBE_STATUS_GENERIC_FAILURE;
    }

    siots_p->parity_pos = fbe_raid_siots_get_parity_pos(siots_p);
    if (RAID_COND(siots_p->parity_pos == FBE_RAID_INVALID_PARITY_POSITION))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: siots_p->parity_pos 0x%x == FBE_RAID_INVALID_PARITY_POSITION\n",
                             siots_p->parity_pos);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Background requests are not allowed or expected for striped mirrors
     * because all background operations are performed at the mirror level.
     */
    if (RAID_COND(fbe_raid_geometry_is_raid10(raid_geometry_p) == FBE_TRUE))
    {
        fbe_raid_siots_set_unexpected_error(siots_p, FBE_RAID_SIOTS_TRACE_UNEXPECTED_ERROR_PARAMS,
                                            "striper: iots opcode: 0x%x lba: 0x%llx blks: 0x%llx background op for RAID-10 not allowed",
                                            opcode,
					    (unsigned long long)iots_p->lba,
					    (unsigned long long)iots_p->blocks);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    siots_p->start_pos = fbe_raid_siots_get_start_pos(siots_p);
    siots_p->data_disks = fbe_raid_siots_get_width(siots_p);

    /* Update the info in fbe_raid_siots_t.
     */
    siots_p->algorithm = RG_CHECK_ZEROED;
    siots_p->retry_count = FBE_RAID_DEFAULT_RETRY_COUNT;

    fbe_raid_siots_transition(siots_p, fbe_raid_check_zeroed_state0);

    /* Setup the parity start and parity count.
     */
    siots_p->parity_start = siots_p->start_lba;
    siots_p->parity_count = siots_p->xfer_count;
    siots_p->geo.logical_parity_count = siots_p->parity_count;

    /* Adjust the iots to account for this request
     */
    fbe_raid_iots_dec_blocks(fbe_raid_siots_get_iots(siots_p), siots_p->xfer_count);

    /* Drive operations are just the width.
     */
    siots_p->drive_operations = fbe_raid_siots_get_width(siots_p);
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_parity_generate_check_zeroed()
 *****************************************/
/*************************
 * end file fbe_striper_generate.c
 *************************/


