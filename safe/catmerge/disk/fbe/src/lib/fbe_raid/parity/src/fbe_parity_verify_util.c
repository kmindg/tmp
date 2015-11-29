/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * fbe_parity_verify_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the utility functions of the RAID 5 
 *  normal and recovery verify algorithms. The normal verify algorithm is 
 *  selected when there is a verify request from the verify process.
 *  The recovery verify algorithm is selected only when the read or write 
 *  (468 or MR3) has encountered hard errors or checksum errors 
 *  on the read data. 
 *
 *
 * @notes
 *      At initial entry, we assume that either a fbe_raid_siots_t or a nested fbe_raid_siots_t 
 *      has been allocated.
 *
 *      While traversing through this state machine,
 *      the following SIOTS fields contain the
 *      following values.  3/13/00 - Rob Foley
 *
 *         start_lba - unit relative physical address where the entire
 *                     verify region begins
 *
 *         xfer_count - the total vertical blocks remaining in
 *                      the entire verify region
 *
 *         parity_start - unit relative physical address where
 *                        the current verify pass begins
 *
 *         parity_count - count of vertical blocks to verify
 *                        in the current verify pass
 *
 *         
 *         during strip mining, because we make several
 *         passes to do the verify,
 *         parity_start may not equal start_lba
 *         and xfer_count may not equal parity_count
 *
 * @author
 *    2000 Created Rob Foley.
 *
 ***************************************************************************/

/*************************
 ** INCLUDE FILES
 *************************/
#include "fbe_raid_library_proto.h"
#include "fbe_parity_io_private.h"
#include "fbe_raid_library.h"
#include "fbe_raid_error.h"
#include "fbe/fbe_event_log_utils.h"
#include "fbe/fbe_api_lun_interface.h"

/********************************
 *      static STRING LITERALS
 ********************************/

/*****************************************************
 *      static FUNCTIONS DECLARED GLOBALLY FOR VISIBILITY
 *****************************************************/
static fbe_status_t fbe_parity_verify_strip_setup(fbe_raid_siots_t * siots_p,
                              fbe_xor_parity_reconstruct_t *verify_p);
static fbe_status_t fbe_parity_recovery_verify_xor_fill(fbe_raid_siots_t * siots_p,
                                                        fbe_xor_recovery_verify_t *xor_recovery_verify_p);
static fbe_status_t fbe_parity_recovery_verify_fill_move(fbe_raid_siots_t * siots_p,
                                                         fbe_xor_mem_move_t *mem_move_p);
static fbe_bool_t r5_is_coh_correctable(const fbe_raid_siots_t * const siots_p,
                                 const fbe_u32_t parity_pos);
static fbe_bool_t fbe_parity_log_u_error(fbe_raid_siots_t * const siots_p, 
                             const fbe_u16_t array_pos, 
                             const fbe_lba_t parity_start_pba,
                             fbe_u32_t * const error_bits_p);
static fbe_status_t fbe_parity_log_parity_drive_invalidation(fbe_raid_siots_t * siots_p);
fbe_status_t fbe_parity_verify_count_allocated_buffers(fbe_raid_fruts_t * parent_fruts_ptr,
                                                    fbe_raid_siots_t * siots_p,
                                                    fbe_lba_t verify_start,
                                                    fbe_block_count_t verify_count,
                                                    fbe_s32_t *block_allocated_p);
fbe_status_t fbe_parity_recovery_verify_count_buffers(fbe_raid_siots_t * siots_p, 
                                                   fbe_lba_t verify_start, 
                                                   fbe_block_count_t verify_count,
                                                   fbe_block_count_t *block_required_p);
fbe_status_t fbe_parity_verify_setup_sgs(fbe_raid_siots_t * siots_p,
                                         fbe_raid_memory_info_t *memory_info_p);
fbe_status_t fbe_parity_recovery_verify_setup_sgs(fbe_raid_siots_t * siots_p,
                                                  fbe_raid_memory_info_t *memory_info_p, 
                                                  fbe_lba_t verify_start, 
                                                  fbe_block_count_t verify_count);
fbe_status_t fbe_parity_verify_calculate_num_sgs(fbe_raid_siots_t *siots_p,
                                                 fbe_u16_t *num_sgs_p,
                                                 fbe_raid_fru_info_t *read_p);
fbe_status_t fbe_parity_verify_count_sgs(fbe_raid_siots_t * siots_p, fbe_raid_fru_info_t *read_p);
fbe_status_t fbe_parity_recovery_verify_count_max_sgs(fbe_raid_siots_t * siots_p, fbe_raid_fru_info_t *read_p);
fbe_status_t fbe_parity_recovery_verify_count_sgs(fbe_raid_siots_t * siots_p, fbe_raid_fru_info_t *read_p);
fbe_status_t fbe_parity_verify_setup_fruts(fbe_raid_siots_t * siots_p, 
                                           fbe_raid_fru_info_t *read_p,
                                           fbe_u32_t opcode,
                                           fbe_raid_memory_info_t *memory_info_p);
fbe_status_t fbe_raid_send_unsol(fbe_raid_siots_t *siots_p,
                                 fbe_u32_t error_code,
                                 fbe_u32_t fru_pos,
                                 fbe_lba_t lba,
                                 fbe_u32_t blocks,
                                 fbe_u32_t error_info,
                                 fbe_u32_t extra_info );
fbe_u32_t fbe_raid_count_no_of_ones_in_bitmask(fbe_u16_t bitmask);
static fbe_bool_t fbe_parity_can_log_u_errors_as_per_eboard(fbe_raid_siots_t * siots_p);
static fbe_bool_t fbe_parity_can_log_uncorrectable_from_error_region(fbe_raid_siots_t * siots_p, 
                                                                     fbe_u32_t region_idx);
static fbe_bool_t fbe_parity_can_log_u_error_for_given_pos(fbe_raid_siots_t *const siots_p, 
                                                           const fbe_u16_t array_pos, 
                                                           const fbe_lba_t parity_start_pba,
                                                           fbe_u32_t * const error_bits_p);
static fbe_status_t fbe_parity_report_retried_errors(fbe_raid_siots_t * siots_p);
static fbe_status_t fbe_parity_report_corrupt_operations(fbe_raid_siots_t * siots_p);

/****************************
 *      GLOBAL VARIABLES
 ****************************/

/*************************************
 *      EXTERNAL GLOBAL VARIABLES
 *************************************/


/*!**************************************************************
 * fbe_parity_verify_validate()
 ****************************************************************
 * @brief
 *  Validate the algorithm and some initial parameters to
 *  the verify state machine.
 *
 * @param  siots_p - current I/O.               
 *
 * @return FBE_TRUE on success, FBE_FALSE on failure. 
 *
 ****************************************************************/

fbe_bool_t fbe_parity_verify_validate(fbe_raid_siots_t * siots_p)
{
    fbe_bool_t b_return; 
    fbe_raid_geometry_t *geo_p = fbe_raid_siots_get_raid_geometry(siots_p);

    /* First make sure we support the algorithm.
     */
    switch (siots_p->algorithm)
    {
        case R5_VR:
        case R5_RD_VR:
        case R5_468_VR:
        case R5_MR3_VR:
        case R5_RCW_VR:
        case R5_DEG_VR:
        case R5_DEG_RVR:
        case R5_DRD_VR:
        case RG_FLUSH_JOURNAL_VR:
            b_return = FBE_TRUE;
            break;
        default:
            b_return = FBE_FALSE;
            break;
    };

    /* Make sure the verify request is a multiple of the optimal block size.
     * Anyone calling this code should align the request to the optimal 
     * block size.  One exception is degraded verify since it goes through this 
     * path for I/O and we do not align the request. 
     */
    if (siots_p->algorithm != R5_DEG_VR)
    {
        if ( (siots_p->parity_count % geo_p->optimal_block_size) != 0 )
        {
            b_return = FBE_FALSE;
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "%s parity count not multiple of opt block size %llx %x\n", 
                                 __FUNCTION__,
				 (unsigned long long)siots_p->parity_count,
				 geo_p->optimal_block_size);
        }
        if ( ((siots_p->parity_start + siots_p->parity_count) % 
              geo_p->optimal_block_size) != 0)
        {   
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity end not multiple of opt block size %llx %llx %x\n", 
                                 (unsigned long long)siots_p->parity_start, 
                                 (unsigned long long)siots_p->parity_count,
				 geo_p->optimal_block_size);
            b_return = FBE_FALSE;
        }
    }
    if (b_return)
    {
         if (!fbe_parity_generate_validate_siots(siots_p))
         {
             fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                  "parity: verify generate validate failed\n");
             b_return = FBE_FALSE;
         }
    }
    return b_return;
}
/******************************************
 * end fbe_parity_verify_validate()
 ******************************************/

/*!***************************************************************
 * fbe_parity_verify_get_fruts_info()
 ****************************************************************
 * @brief
 *  This function initializes the fru info for verify.
 *
 * @param siots_p - Current sub request.
 *
 * @return
 *  None
 *
 * @author
 *  10/15/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_verify_get_fruts_info(fbe_raid_siots_t * siots_p,
                                              fbe_raid_fru_info_t *read_p)
{
    fbe_status_t status;
    fbe_block_count_t parity_range_offset;
    fbe_u16_t data_pos;
    fbe_u8_t *position = siots_p->geo.position;
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);
    fbe_lba_t logical_parity_start = siots_p->geo.logical_parity_start;
    fbe_block_count_t verify_count;

    status = fbe_parity_verify_get_verify_count(siots_p, &verify_count);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: get verify count failed with status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);                               
        return (status);
    }
    
    parity_range_offset = siots_p->geo.start_offset_rel_parity_stripe;

    /* Initialize the fru info for data drives.
     */
    for (data_pos = 0; data_pos < siots_p->data_disks; data_pos++)
    {
        read_p->lba = logical_parity_start + parity_range_offset;
        read_p->blocks = verify_count;
        read_p->position = position[data_pos];
        read_p++;
    }

    /* Initialize the fru info for parity drive.
     */
    read_p->lba = logical_parity_start + parity_range_offset;
    read_p->blocks = verify_count;
    read_p->position = position[fbe_raid_siots_get_width(siots_p) - parity_drives];
    read_p++;

    if (parity_drives > 1)
    {
        /* Initialize the R6 Diagonal Parity Drive.
         */
         read_p->lba = logical_parity_start + parity_range_offset;
         read_p->blocks = verify_count;
         read_p->position = position[fbe_raid_siots_get_width(siots_p) - 1];
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_verify_get_fruts_info
 **************************************/
/*!**************************************************************************
 * fbe_parity_verify_calculate_memory_size()
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
 *  9/15/2009 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_parity_verify_calculate_memory_size(fbe_raid_siots_t *siots_p,
                                                     fbe_raid_fru_info_t *read_info_p,
                                                     fbe_u32_t *num_pages_to_allocate_p)
{
    fbe_u16_t num_sgs[FBE_RAID_MAX_SG_TYPE];
    fbe_u32_t num_fruts = 0;
    fbe_block_count_t total_blocks;

    fbe_status_t status;

    fbe_zero_memory(&num_sgs[0], (sizeof(fbe_u16_t) * FBE_RAID_MAX_SG_TYPE));
    /* Then calculate how many buffers are needed.
     */
    status = fbe_parity_verify_get_fruts_info(siots_p, read_info_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: verify get fruts info failed with status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);                               
        return (status);
    }

    num_fruts = fbe_raid_siots_get_width(siots_p);

    if (siots_p->algorithm == R5_VR)
    {
        total_blocks = fbe_raid_siots_get_width(siots_p) * siots_p->parity_count;
    }
    else if (siots_p->parity_count != siots_p->xfer_count)
    {
        /* Count max amount of buffers.
         */
        total_blocks = ( (fbe_block_count_t)(fbe_raid_siots_get_width(siots_p)) ) * fbe_raid_siots_get_region_size(siots_p);
    }
    else
    {
        /* Count exact amount of buffers.
         * We cannot always do this.
         */
        status = fbe_parity_recovery_verify_count_buffers(siots_p, 
                                                          siots_p->parity_start, 
                                                          siots_p->parity_count,
                                                          &total_blocks);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity:failed to count buffers : status = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);

            return status;
        }
    }
        
    siots_p->total_blocks_to_allocate = total_blocks;
    fbe_raid_siots_set_optimal_page_size(siots_p,
                                         num_fruts, siots_p->total_blocks_to_allocate);

    /* Next calculate the number of sgs of each type.
     */
    status = fbe_parity_verify_calculate_num_sgs(siots_p, 
                                                 &num_sgs[0], 
                                                 read_info_p);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    { 
         fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                              "parity: failed to calculate number of sgs: status = 0x%x, siots_p = 0x%p\n",
                              status, siots_p);

         return status;
    }

    /* Take the information we calculated above and determine how this translates 
     * into a number of pages. 
     */
    status = fbe_raid_siots_calculate_num_pages(siots_p, num_fruts, &num_sgs[0],
                                                FBE_FALSE /* No nested siots is required*/);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    { 
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                              "parity: failed to calculate number of pages: status = 0x%x, siots_p = 0x%p\n",
                              status, siots_p);

        return status;
    }
    *num_pages_to_allocate_p = siots_p->num_ctrl_pages;
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_verify_calculate_memory_size()
 **************************************/

/*!**************************************************************************
 * fbe_parity_verify_calculate_num_sgs()
 ****************************************************************************
 * @brief  This function calculates the number of sg lists needed.
 *
 * @param siots_p - Pointer to siots that contains the allocated fruts
 * @param page_size_p - Size of each page we expect to use.
 * @param num_pages_to_allocate_p - Number of memory pages to allocate
 * 
 * @return None.
 *
 * @author
 *  10/5/2009 - Created. Rob Foley
 *
 **************************************************************************/

fbe_status_t fbe_parity_verify_calculate_num_sgs(fbe_raid_siots_t *siots_p,
                                                 fbe_u16_t *num_sgs_p,
                                                 fbe_raid_fru_info_t *read_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u16_t data_pos;
    fbe_u16_t width = fbe_raid_siots_get_width(siots_p);

    /* Page size must be set.
     */
    if (!fbe_raid_siots_validate_page_size(siots_p))
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Count and allocate sgs first.
     */
    if (siots_p->algorithm == R5_VR)
    {
        /* Count the number of sgs for normal verify.
         */
        status = fbe_parity_verify_count_sgs(siots_p, read_p);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: verify count sgs failed with status = 0x%x, siots_p = 0x%p, "
                                 "read_p = 0x%p\n",
                                 status, siots_p, read_p);                               
            return (status);
        }
    }
    else
    {
        if (siots_p->parity_count != siots_p->xfer_count)
        {
            /* We are strip mining.  Count and allocate max amount of sgs.
             */
            status = fbe_parity_recovery_verify_count_max_sgs(siots_p, read_p);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                /*Split trace to two lines*/
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity:recovery verify count max sgs fail,"
                             "stat=0x%x>\n",
                             status);  
				fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "siots_p=0x%p,read_p=0x%p<\n",
                             siots_p, read_p);   
                return (status);
            }
        }
        else
        {
            /* Count exact amount of sgs.
             * We can only do this while we are verify the whole range,
             * rather than strip-mined.
             */
            status = fbe_parity_recovery_verify_count_sgs(siots_p, read_p);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                /*Split trace to two lines*/
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity:recovery verify count sgs fail,"
                             "stat=0x%x>\n",
                             status);  
				fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "siots_p=0x%p,read_p=0x%p<\n",
                             siots_p, read_p);                             
                return (status);
            }
        }
    }

    /* Now take the sgs we just counted and add them to the input counts.
     */
    for (data_pos = 0; data_pos < width; data_pos++)
    {
        /* First determine the read sg type so we know which type of sg we
         * need to increase the count of. 
         */
        if (read_p->blocks == 0)
        {
             fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                  "parity: verify read blocks is 0x%llx lba: %llx pos: %d\n",
                                  (unsigned long long)read_p->blocks,
				  (unsigned long long)read_p->lba,
				  read_p->position);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (read_p->sg_index >= FBE_RAID_MAX_SG_TYPE)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: verify read sg index is %d for pos %d lba: %llx bl: 0x%llx\n",
                                 read_p->sg_index, read_p->position,
				 (unsigned long long)read_p->lba,
				 (unsigned long long)read_p->blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        num_sgs_p[read_p->sg_index]++;
        read_p++;
    }
    return status;
}
/**************************************
 * end fbe_parity_verify_calculate_num_sgs()
 **************************************/
/*!***************************************************************
 * fbe_parity_verify_setup_fruts()
 ****************************************************************
 * @brief
 *  This function will set up fruts for external verify.
 *
 * @param siots_p    - ptr to the fbe_raid_siots_t
 * @param verify_count - number of blocks to verify.
 * @param opcode     - type of op to put in fruts.
 * @param memory_info_p - Pointer to memory request information
 *
 * @return fbe_status_t
 *
 * @author
 *  8/30/99 - Created. JJ
 *  10/20/99 - Modified for both normal and recovery verify.
 *
 ****************************************************************/
fbe_status_t fbe_parity_verify_setup_fruts(fbe_raid_siots_t * siots_p, 
                                           fbe_raid_fru_info_t *read_p,
                                           fbe_u32_t opcode,
                                           fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_u16_t data_pos;
    fbe_u16_t width = fbe_raid_siots_get_width(siots_p);

    /* Initialize the fruts for data drives.
     */
    for (data_pos = 0; data_pos < width; data_pos++)
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
                                 "parity : failed to init fruts 0x%p for siots_p = 0x%p, status = 0x%x\n",
                                 read_fruts_ptr, siots_p, status);
             return status;
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
    }
    return FBE_STATUS_OK;
}
/* end of fbe_parity_verify_setup_fruts() */

/*!**************************************************************
 * fbe_parity_verify_count_sgs()
 ****************************************************************
 * @brief
 *  Count how many sgs we need for the verify operation.
 *
 * @param siots_p - this I/O.               
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_parity_verify_count_sgs(fbe_raid_siots_t * siots_p,
                                         fbe_raid_fru_info_t *read_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_block_count_t mem_left = 0;
    fbe_u32_t width;
    fbe_u32_t data_pos;

    width = fbe_raid_siots_get_width(siots_p);

    for (data_pos = 0; data_pos < width; data_pos++)
    {
        fbe_u32_t sg_count = 0;

        mem_left = fbe_raid_sg_count_uniform_blocks(read_p->blocks,
                                                    siots_p->data_page_size,
                                                    mem_left,
                                                    &sg_count);

        /* Always add on an sg since if we drop down to a region mode, 
         * then it is possible we will need an extra sg to account for region alignment. 
         */
        if ((sg_count + 1 < FBE_RAID_MAX_SG_ELEMENTS))
        {
            sg_count++;
        }
        /* Save the type of sg we need.
         */
        status = fbe_raid_fru_info_set_sg_index(read_p, sg_count, FBE_TRUE /* yes log */);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity:fail to set sg index:stat=0x%x,siots=0x%p, "
                                 "read=0x%p\n", 
                                 status, siots_p, read_p);

             return status;
        }
        read_p++;
    }
    return FBE_STATUS_OK;
} 
/******************************************
 * end fbe_parity_verify_count_sgs()
 ******************************************/

/*!***************************************************************
 * fbe_parity_recovery_verify_count_max_sgs()
 ****************************************************************
 * @brief
 *  This function will count the max amount of sgs in the recovery verify.
 *
 * @param siots_p- ptr to the fbe_raid_siots_t 
 *
 * @return fbe_status_t
 *
 * @author
 *  9/21/99 - Created. JJ
 *
 ****************************************************************/
fbe_status_t fbe_parity_recovery_verify_count_max_sgs(fbe_raid_siots_t * siots_p,
                                                      fbe_raid_fru_info_t *read_p)
{
    fbe_u32_t data_pos;
    fbe_u32_t width;
    fbe_status_t status = FBE_STATUS_OK;

    width = fbe_raid_siots_get_width(siots_p);

    for (data_pos = 0; data_pos < width; data_pos++)
    {
        status = fbe_raid_fru_info_set_sg_index(read_p, FBE_RAID_MAX_SG_DATA_ELEMENTS, FBE_TRUE /* yes log */);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity:fail to set sg index:stat=0x%x,siots=0x%p,read=0x%p\n", 
                                 status, siots_p, read_p);

             return status;
        }

        read_p++;
    }
    return FBE_STATUS_OK;
} /* end of fbe_parity_recovery_verify_count_max_sgs() */

/*!***************************************************************
 * fbe_parity_recovery_verify_count_sgs()
 ****************************************************************
 * @brief
 *  This function will count the exact amount of sgs
 *  in the recovery verify.
 *
 * @param siots_p    - ptr to the fbe_raid_siots_t
 *
 * @return fbe_status_t
 *
 * @author
 *  8/30/99 - Created. JJ
 *
 ****************************************************************/

fbe_status_t fbe_parity_recovery_verify_count_sgs(fbe_raid_siots_t * siots_p, 
                                                  fbe_raid_fru_info_t *read_p)
{
    fbe_lba_t verify_start = siots_p->parity_start; 
    fbe_block_count_t verify_count = siots_p->parity_count;
    fbe_raid_siots_t *parent_siots_p = fbe_raid_siots_get_parent(siots_p);
    fbe_raid_fruts_t *parent_read_fruts_ptr = NULL;
    fbe_raid_fruts_t *parent_read2_fruts_ptr = NULL;
    fbe_u32_t sg_size;
    fbe_block_count_t mem_left = 0;
    fbe_u32_t data_pos;
    fbe_lba_t logical_parity_start = siots_p->geo.logical_parity_start;
    fbe_u32_t width;
    fbe_sg_element_t *overlay_sg_ptr = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_block_count_t blks_assigned = 0;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);


    width = fbe_raid_siots_get_width(siots_p);
    fbe_raid_siots_get_read_fruts(parent_siots_p, &parent_read_fruts_ptr);
    fbe_raid_siots_get_read2_fruts(parent_siots_p, &parent_read2_fruts_ptr);

    for (data_pos = 0; data_pos < width; data_pos++)
    {
        sg_size = 0;
        blks_assigned = 0;
        
        if (parent_read_fruts_ptr && parent_read_fruts_ptr->position == read_p->position) 
        {
            fbe_lba_t relative_fruts_lba,   /* The Parity relative FRUTS lba. */
                      end_lba,
                      verify_end;
            fbe_block_count_t fruts_blocks; /* The first fruts block count.   */
            fbe_u32_t tmp_size = 0;

            if (verify_start < siots_p->start_lba)
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: verify start less than start_lba %llx lba: %llx\n",
                                     (unsigned long long)verify_start,
                                     (unsigned long long)siots_p->start_lba);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Since the fruts lba is a physical lba, and the
             * verify_start is a parity relative logical address,
             * map the fruts lba to a parity logical address.
             */
            relative_fruts_lba = logical_parity_start;
            status = fbe_raid_map_pba_to_lba_relative(parent_read_fruts_ptr->lba,
                                                      parent_read_fruts_ptr->position,
                                                      siots_p->parity_pos,
                                                      siots_p->start_lba,
                                                      fbe_raid_siots_get_width(siots_p),
                                                      &relative_fruts_lba,
                                                      fbe_raid_siots_get_parity_stripe_offset(siots_p),
                                                      fbe_raid_siots_parity_num_positions(siots_p));
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                /*Split trace to two lines*/
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity:fail map pba to lba: status = 0x%x, "
                                     "siots_p = 0x%p>\n",
                                     status, siots_p);
				fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parent_read_fruts_ptr = 0x%p<\n",
                                     parent_read_fruts_ptr);
                return status;
            }
            fruts_blocks = parent_read_fruts_ptr->blocks;
            end_lba = relative_fruts_lba + fruts_blocks - 1;
            verify_end = verify_start + verify_count - 1;


            fbe_raid_fruts_get_sg_ptr(parent_read_fruts_ptr, &overlay_sg_ptr);
            mem_left = fbe_raid_determine_overlay_sgs(relative_fruts_lba,
                                                      fruts_blocks,
                                                      siots_p->data_page_size,
                                                      overlay_sg_ptr,
                                                      verify_start,
                                                      verify_count,
                                                      mem_left,
                                                      &blks_assigned,
                                                      &tmp_size);
            sg_size += tmp_size;

            parent_read_fruts_ptr = fbe_raid_fruts_get_next(parent_read_fruts_ptr);
        } 

        if (parent_read2_fruts_ptr && parent_read2_fruts_ptr->position == read_p->position) 
        {
            /* If there are blocks remaining to be overlayed, then
             * we will check the 2nd pre-read
             * or just assign the remaining blocks.
             */
            if (blks_assigned < verify_count)
            {
                fbe_block_count_t extra_blocks = verify_count - blks_assigned;
                fbe_u32_t tmp_size = 0;
                fbe_block_count_t more_blks = 0;
                fbe_lba_t relative_fruts2_lba, end_lba, verify_end;
                fbe_block_count_t fruts_blocks;

                if (verify_start < siots_p->start_lba)
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "parity: verify start less than start_lba %llx lba: %llx\n",
                                         (unsigned long long)verify_start, (unsigned long long)siots_p->start_lba);
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                /* Since the fruts lba is a physical lba, and the
                 * verify_start is a parity relative logical address,
                 * map the fruts lba to a parity logical address.
                 */
                relative_fruts2_lba = logical_parity_start;
                status = fbe_raid_map_pba_to_lba_relative(parent_read2_fruts_ptr->lba,
                                                          parent_read2_fruts_ptr->position,
                                                          siots_p->parity_pos,
                                                          siots_p->start_lba,
                                                          fbe_raid_siots_get_width(siots_p),
                                                          &relative_fruts2_lba,
                                                          fbe_raid_siots_get_parity_stripe_offset(siots_p),
                                                          fbe_raid_siots_parity_num_positions(siots_p));
                if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                {
                    /*Split trace to two lines*/
		            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
		                                 "parity:fail map pba to lba: status = 0x%x, "
		                                 "siots_p = 0x%p>\n",
		                                 status, siots_p);
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
		                                 "parent_read2_fruts_ptr = 0x%p<\n",
		                                 parent_read2_fruts_ptr);
                    return status;
                }

                fruts_blocks = parent_read2_fruts_ptr->blocks;
                end_lba = relative_fruts2_lba + fruts_blocks - 1;
                verify_end = verify_start + verify_count - 1;

                /* There's only a parent_read2_fruts_ptr for this position
                 */
                if ((blks_assigned == 0) &&
                    (siots_p->algorithm != R5_MR3_VR) && 
                    (siots_p->algorithm != R5_DEG_VR) && 
                    (siots_p->algorithm != R5_DEG_RVR) &&
                    (siots_p->algorithm != R5_RCW_VR))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "parity: algorithm: not expected %d\n", siots_p->algorithm);
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                fbe_raid_fruts_get_sg_ptr(parent_read2_fruts_ptr, &overlay_sg_ptr);
                mem_left = fbe_raid_determine_overlay_sgs(relative_fruts2_lba,
                                                          parent_read2_fruts_ptr->blocks,
                                                          siots_p->data_page_size,
                                                          overlay_sg_ptr,
                                                          verify_start + blks_assigned,
                                                          extra_blocks,
                                                          mem_left,
                                                          &more_blks,
                                                          &tmp_size);


                if ((more_blks != extra_blocks) && !fbe_raid_geometry_needs_alignment(raid_geometry_p))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "parity: more_blks != extra_blocks 0x%llx 0x%llx\n",
                                         (unsigned long long)more_blks,
                                         (unsigned long long)extra_blocks);
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                sg_size += tmp_size;
                blks_assigned += more_blks;
            } 
            else if (blks_assigned != verify_count) 
            {
            
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: blks_assigned != verify_count 0x%llx 0x%llx\n",
                                     (unsigned long long)blks_assigned,
                                     (unsigned long long)verify_count);
                return FBE_STATUS_GENERIC_FAILURE;
            } 

            parent_read2_fruts_ptr = fbe_raid_fruts_get_next(parent_read2_fruts_ptr);
        }

        /* Allocate new buffers for the remaining blocks
         */
        if (blks_assigned < verify_count) 
        {
            fbe_u32_t tmp_size = 0;
            fbe_block_count_t extra_blocks = verify_count - blks_assigned;

            /* SG to point to the will-be allocated new buffers.
             */
            mem_left = fbe_raid_sg_count_uniform_blocks(extra_blocks,
                                                        siots_p->data_page_size,
                                                        mem_left,
                                                        &tmp_size);

           sg_size += tmp_size;
        }

        /* Always add on an sg since if we drop down to a region mode, 
         * then it is possible we will need an extra sg to account for region alignment. 
         */
        if ((sg_size + 1 < FBE_RAID_MAX_SG_ELEMENTS))
        {
            sg_size++;
        }

        status = fbe_raid_fru_info_set_sg_index(read_p, sg_size, FBE_TRUE /* yes log */);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p,
                                 FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity:fail to set sg index:stat=0x%x,siots=0x%p, "
                                 "read=0x%p\n", 
                                 status, siots_p, read_p);

            return status;
        }
        read_p++;
    }

    if (parent_siots_p->algorithm != R5_DEG_468)
    {
        if ((parent_read_fruts_ptr != NULL) &&
            (!fbe_raid_siots_is_parity_pos(siots_p, parent_read_fruts_ptr->position)))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: unexpected read fruts position 0x%p %d %d\n",
                                 parent_read_fruts_ptr, parent_read_fruts_ptr->position, siots_p->parity_pos);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if ((parent_read_fruts_ptr != NULL) || (parent_read2_fruts_ptr != NULL))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: unexpected read fruts not null 0x%p 0x%p %d \n",
                                 parent_read_fruts_ptr, parent_read2_fruts_ptr, siots_p->parity_pos);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return status;
}
/***********************************************
 *end of fbe_parity_recovery_verify_count_sgs() 
 ***********************************************/

/*!**************************************************************
 * fbe_parity_verify_setup_sgs_normal()
 ****************************************************************
 * @brief
 *  Setup the sg lists for the verify operation.
 *
 * @param siots_p - this I/O.     
 * @param memory_info_p - Pointer to memory request information          
 *
 * @return fbe_status_t
 *
 * @author
 *  8/18/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_verify_setup_sgs_normal(fbe_raid_siots_t * siots_p,
                                                fbe_raid_memory_info_t *memory_info_p)
{
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_status_t status;

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

    while (read_fruts_ptr != NULL)
    {
        fbe_sg_element_t *sgl_ptr;
        fbe_u16_t  sg_total =0;
        fbe_u32_t dest_byte_count =0;

        sgl_ptr = fbe_raid_fruts_return_sg_ptr(read_fruts_ptr);

        if(!fbe_raid_get_checked_byte_count(read_fruts_ptr->blocks, &dest_byte_count))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                               "%s: byte count exceeding 32bit limit \n",
                                               __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE; 
        }
        /* Assign newly allocated memory for the whole verify region.
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
        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
    }

    /* Debug code to detect scatter gather overrun.
     * Typically this will get caught when we try to free the 
     * sg lists to the BM, but it is better to try and catch it sooner. 
     */
    status = fbe_raid_fruts_for_each(FBE_RAID_FRU_POSITION_BITMASK, read_fruts_ptr, fbe_raid_fruts_validate_sgs, 0, 0);

    return status;
}
/******************************************
 * end fbe_parity_verify_setup_sgs_normal()
 ******************************************/

/*!**************************************************************
 * fbe_parity_verify_setup_sgs()
 ****************************************************************
 * @brief
 *  Setup the sg lists for the verify operation.
 *
 * @param siots_p - this I/O.      
 * @param memory_info_p - Pointer to memory request information         
 *
 * @return fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_parity_verify_setup_sgs(fbe_raid_siots_t * siots_p,
                                         fbe_raid_memory_info_t *memory_info_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    if (siots_p->algorithm == R5_VR)
    {
        /* Set up the sgs.
         */
        status = fbe_parity_verify_setup_sgs_normal(siots_p, memory_info_p);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) 
        { 
            /*Split trace to two lines*/
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity:fail setup sgs,status=0x%x>\n",
                                 status);
			fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "siots=0x%p,mem_info=0x%p<\n",
                                 siots_p, memory_info_p);
            return (status);
        }
    }
    else
    {
        /* Set up the sgs.
         */
        status = fbe_parity_recovery_verify_setup_sgs(siots_p,
                                                      memory_info_p,
                                                      siots_p->parity_start,
                                                      siots_p->parity_count);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) 
        { 
            /*Split trace to two lines*/
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "parity:fail setup sgs,status=0x%x>\n",
                                 status);
			fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "siots=0x%p,mem_info=0x%p<\n",
                                 siots_p, memory_info_p);
            return (status);
        }           
    }

    return status;
}
/******************************************
 * end fbe_parity_verify_setup_sgs()
 ******************************************/

/*!***************************************************************
 * fbe_parity_recovery_verify_setup_geometry()     
 ****************************************************************
 * @brief
 *  This function set up the geometry information for 
 *  the sub_iots in the recovery verify.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 * @param b_align - FBE_TRUE to align to optimal size, FBE_FALSE to not align.
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_parity_recovery_verify_setup_geometry(fbe_raid_siots_t * siots_p,
                                                       fbe_bool_t b_align)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_siots_t *parent_siots_p = fbe_raid_siots_get_parent(siots_p);
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_lba_t end_lba = parent_siots_p->parity_start + parent_siots_p->parity_count - 1;
    fbe_u32_t data_pos;

    /***************************************************
     * We always verify the parity range.
     ***************************************************/
    siots_p->start_lba = parent_siots_p->parity_start;

    /* If requested round the parity start back to the optimal block boundary.
     */
    fbe_raid_geometry_align_lock_request(raid_geometry_p,
                                         &siots_p->start_lba,
                                         &end_lba);

    siots_p->parity_start = siots_p->start_lba;
    siots_p->parity_count =
        siots_p->xfer_count = (fbe_u32_t)(end_lba - siots_p->start_lba + 1);

    fbe_parity_get_physical_geometry(raid_geometry_p,
                                     siots_p->parity_start,
                                     &siots_p->geo);

    if (parent_siots_p->parity_pos != fbe_raid_siots_get_parity_pos(siots_p))
    {
	fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_recov_verify_setup_geom" ,
						"parity:parent_siots->parity_pos 0x%x!=fbe_raid_siots_get_parity_pos(siots)0x%x\n",
						parent_siots_p->parity_pos, fbe_raid_siots_get_parity_pos(siots_p));
        return FBE_STATUS_GENERIC_FAILURE;
    }

    siots_p->parity_pos = fbe_raid_siots_get_parity_pos(siots_p);
    if (RAID_COND(siots_p->parity_pos == FBE_RAID_INVALID_PARITY_POSITION))
    {
	fbe_raid_siots_trace(siots_p, 
						FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_recov_verify_setup_geom" ,
						"parity:siots_p->parity_pos 0x%x=FBE_RAID_INVALID_PARITY_POSITION\n",
						siots_p->parity_pos);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    siots_p->start_pos = fbe_raid_siots_get_start_pos(siots_p);
    siots_p->data_disks = fbe_raid_siots_get_width(siots_p) - fbe_raid_siots_parity_num_positions(siots_p);

    status = FBE_RAID_DATA_POSITION(siots_p->start_pos,
                                siots_p->parity_pos,
                                fbe_raid_siots_get_width(siots_p),
                                fbe_raid_siots_parity_num_positions(siots_p),
                                &data_pos);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_object_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_recov_verify_setup_geom",
                                    "parity:fail to CVT data pos: stat: 0x%x, siots=0x%p\n",
                                    status, siots_p);
        return status;
    }
    if (data_pos != 0)
    {
        fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_recov_verify_setup_geom" ,
                             "parity:data pos unexpect %d start:%d "
                             "parity:%d width:%d #parity:%d\n",
                             data_pos,
                             siots_p->start_pos,
                             siots_p->parity_pos,
                             fbe_raid_siots_get_width(siots_p),
                             fbe_raid_siots_parity_num_positions(siots_p));
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/* end of fbe_parity_recovery_verify_setup_geometry() */

/*!***************************************************************
 *                            r5_rvr_error_fruts()
 ****************************************************************
 *      @brief
 *              This function will get the erred fruts.
 *
 *  siots_p    - ptr to the fbe_raid_siots_t 
 *
 *      @return VALUE:
 *
 *      @author
 *        9/30/99 - Created. JJ
 *
 ****************************************************************/
fbe_raid_fruts_t *r5_rvr_error_fruts(fbe_raid_fruts_t * fruts_ptr, fbe_u16_t error_bitmap)
{
    while (fruts_ptr != NULL)
    {
        if (error_bitmap & (1 << fruts_ptr->position))
        {
            break;
        }

        fruts_ptr = fbe_raid_fruts_get_next(fruts_ptr);
    }

    return fruts_ptr;

}                               /* end of r5_rvr_hard_media_error_fruts() */




/*!***************************************************************
 * fbe_raid_determine_overlay_sgs
 ****************************************************************
 * @brief
 *  This function determines the number of blocks and sgs
 *  required to fill gaps in the difference between a base region
 *  and an overlay region.
 *  We calculate the blocks and sgs for an overlay region.
 *
 * @param lba - Relative start LBA of base region.
 * @param blocks - Blocks of base region
 * @param overlay_fruts_p - Fruts which has sgs that will be
 *                          copied into this fruts sg.
 * @param overlay_start - Relative start LBA of base region
 * @param overlay_count - Blocks in overlay region
 * @param mem_left - Mem remaining in a standard buffer block
 * @param blocks_ptr - New blocks needed to perform overlay
 * @param sg_size_ptr - Vector of sg sizes needed for overlay.
 *
 * @return
 *  Blocks leftover in a standard buffer after performing overlay.
 *
 * @author
 *  3/12/00 - Rob Foley Created from r5_rvr_count_fruts_sgs()
 *                Commented code.
 *
 ****************************************************************/

fbe_block_count_t fbe_raid_determine_overlay_sgs(fbe_lba_t lba,
                                             fbe_block_count_t blocks,
                                             fbe_block_count_t buffer_blocks,
                                             fbe_sg_element_t *overlay_sg_ptr,
                                             fbe_lba_t overlay_start,
                                             fbe_block_count_t overlay_count,
                                             fbe_block_count_t mem_left,
                                             fbe_block_count_t * blocks_ptr,
                                             fbe_u32_t * sg_size_ptr)
{
    fbe_lba_t end_lba, overlay_end;

    /* Determine the end of the region to 
     * BE overlayed.
     */
    end_lba = lba + blocks - 1;

    /* Determine end of region we are overlaying.
     */
    overlay_end = overlay_start + overlay_count - 1;

    if (overlay_end < lba)
    {
        /***************************************************
         *
         * the start of the base.
         * Just count sgs for entire overlay region.
         *
         *
         *        ****** <-- overlay begins
         *        *    * 
         *        *    * <-- overlay ends
         *        *bbbb* <-- base begins
         *        *bbbb* 
         *        *bbbb* <-- base ends
         *        ******
         ***************************************************/
        mem_left = fbe_raid_sg_count_uniform_blocks(overlay_count,
                                              buffer_blocks,
                                              mem_left,
                                              sg_size_ptr);

        /* Incrment blocks with entire overlay count.
         */
        *blocks_ptr = overlay_count;
    }
    else
    {
        /* Some sort of overlap here.
         */
        fbe_block_count_t assigned_blocks,
          extra_blocks;
        fbe_u16_t bytes_per_blk;
        fbe_sg_element_t *sg_ptr = overlay_sg_ptr;
        fbe_sg_element_with_offset_t sgd;

        assigned_blocks = extra_blocks = 0;
        
        bytes_per_blk = FBE_RAID_SECTOR_SIZE(sg_ptr->address);

        if ((overlay_start >= lba) && (overlay_start <= end_lba))
        {
            /***************************************************
             *
             * The overlay is entirely within the
             * base.
             *
             *        ****** <-- base begins
             *        *BBBB* 
             *        *oBBo* <-- overlay begins
             *        *oBBo*
             *        *oBBo* <-- overlay ends
             *        *BBBB*
             *        ****** <-- base ends
             ****************************************************/

            /* We just need to count the sgs of the already assigned
             * blocks within the base region.
             */
            assigned_blocks = FBE_MIN(overlay_count,
                                  (fbe_u32_t)(end_lba - overlay_start + 1));

            /* The sgd offset is
             * the difference between where the base
             * begins and where the overlay begins.
             */
            sgd.offset = (fbe_u32_t)(overlay_start - lba) * bytes_per_blk,
                sgd.sg_element = sg_ptr;
        }
        else if (overlay_start < lba)
        {

            /***************************************************
             *
             * The overlay region straddles the
             * beginning of the base region
             *
             *        ****** <-- overlay begins
             *        *oooo* 
             *        *oBBo* <-- base begins
             *        *oBBo*
             *        *oBBo* <-- overlay ends
             *        *BBBB*
             *        *BBBB* <-- base ends
             *        ****** 
             ****************************************************/

            /* We need extra sgs for the difference between
             * where overlay region begins and
             * where base region begins.
             */
            extra_blocks = (fbe_u32_t) (lba - overlay_start);

            mem_left = fbe_raid_sg_count_uniform_blocks(extra_blocks,
                                                  buffer_blocks,
                                                  mem_left,
                                                  sg_size_ptr);
            /* Also need to count the sgs of the already
             * assigned blocks in the base region.
             */
            assigned_blocks = FBE_MIN(blocks,
                                  (fbe_u32_t)(overlay_end - lba + 1));

            /* Since we will start counting base region sgs
             * at the beginning of the base region,
             * no offset is necessary.
             */
            sgd.offset = 0,
                sgd.sg_element = sg_ptr;
        }

        if (assigned_blocks > 0)
        {
            fbe_u16_t additional_sgs = 0;

            /* Count the sgs already assigned in the
             * base region, which will be overlayed.
             */
            fbe_parity_rvr_count_assigned_sgs(assigned_blocks,
                                      &sgd,
                                      bytes_per_blk,
                                      &additional_sgs);

            /* Add the additional sgs to the value passed in.
             */
            *sg_size_ptr += additional_sgs;
        }

        *blocks_ptr = assigned_blocks + extra_blocks;
    }

    return mem_left;
}                               /* end of fbe_raid_determine_overlay_sgs() */
/*!***************************************************************
 *                            fbe_parity_rvr_count_assigned_sgs()
 ****************************************************************
 *      @brief
 *              This function will count the number of sgs assigned 
 *      by parent siots.
 *

 *              siots_p    - ptr to the fbe_raid_siots_t
 *
 *      @return VALUE:
 *
 *      @author
 *        8/30/99 - Created. JJ
 *
 ****************************************************************/
void fbe_parity_rvr_count_assigned_sgs(fbe_block_count_t blks_to_count,
                                        fbe_sg_element_with_offset_t * sg_desc_ptr,
                                        fbe_u32_t blks_per_buffer,
                                        fbe_u16_t * sg_size_ptr)
{
    fbe_u32_t blocks;

    while (sg_desc_ptr->offset > (fbe_s32_t) sg_desc_ptr->sg_element->count)
    {
        sg_desc_ptr->offset -= (sg_desc_ptr->sg_element++)->count;
    }

    while ((*sg_size_ptr)++, blks_to_count > (blocks = ((sg_desc_ptr->sg_element->count - sg_desc_ptr->offset) / blks_per_buffer)))
    {
        blks_to_count -= blocks,
            sg_desc_ptr->offset = 0, sg_desc_ptr->sg_element++;
    }

    if (sg_desc_ptr->sg_element->address == NULL)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_PARITY,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "parity: sg is null addr:0x%p count:0x%x\n",
                                 sg_desc_ptr->sg_element->address,
                                 sg_desc_ptr->sg_element->count);
        //return FBE_STATUS_GENERIC_FAILURE;
    }

    return;

}
/* end of fbe_parity_rvr_count_assigned_sgs() */

/*!***************************************************************
 * fbe_parity_recovery_verify_count_buffers()
 ****************************************************************
 * @brief
 *  This function will count how many buffers are required
 *  for a recovery verify.
 *  Since we will be re-using the parent siots buffers,
 *  we will only allocate what we need.
 *  Count how many buffers we need and then subtract off the
 *  already allocated parent buffers.
 *
 * @param siots_p    - ptr to the fbe_raid_siots_t
 *
 * @return  fbe_status_t
 *
 * @author
 *  8/30/99 - Created. JJ
 *
 ****************************************************************/
fbe_status_t fbe_parity_recovery_verify_count_buffers(fbe_raid_siots_t * siots_p, 
                                                   fbe_lba_t verify_start, 
                                                   fbe_block_count_t verify_count,
                                                   fbe_block_count_t *block_required_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_s32_t blks_allocated = 0;
    fbe_raid_fruts_t *parent_read_fruts_ptr = NULL;
    fbe_raid_fruts_t *parent_read2_fruts_ptr = NULL;
    fbe_u32_t parity_drives = fbe_raid_siots_parity_num_positions(siots_p);

    fbe_raid_siots_get_read_fruts(siots_p, &parent_read_fruts_ptr);
    fbe_raid_siots_get_read2_fruts(siots_p, &parent_read2_fruts_ptr);

    if (parent_read_fruts_ptr)
    {
        status = fbe_parity_verify_count_allocated_buffers(parent_read_fruts_ptr, siots_p, 
                                                           verify_start, verify_count,
                                                           &blks_allocated);

        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: failed to count allocated buffers: status = 0x%x, siots_p = 0x%p, "
                                 "parent_read_fruts_ptr = 0x%p, verify_count = 0x%llx\n",
                                 status, siots_p, parent_read_fruts_ptr,
				 (unsigned long long)verify_count);
            return status;
        }
    }

    if (parent_read2_fruts_ptr)
    {
        fbe_s32_t blks_allocated_old = blks_allocated;
        if ((siots_p->algorithm != R5_MR3_VR) && 
            (siots_p->algorithm != R5_DEG_VR) && 
            (siots_p->algorithm != R5_DEG_RVR) &&
            (siots_p->algorithm != R5_RCW_VR))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: algorithm: not expected %d\n", siots_p->algorithm);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        status = fbe_parity_verify_count_allocated_buffers(parent_read2_fruts_ptr, siots_p, 
                                                           verify_start, verify_count, &blks_allocated);
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: failed to count allocated buffers: status = 0x%x, siots_p = 0x%p, "
                                 "parent_read2_fruts_ptr = 0x%p, verify_start = 0x%llx, verify_count = 0x%llx\n",
                                 status, siots_p, parent_read2_fruts_ptr,
				 (unsigned long long)verify_start,
				 (unsigned long long)verify_count);
             return status;
        }
        blks_allocated = blks_allocated + blks_allocated_old;
    }

    *block_required_p = verify_count * (siots_p->data_disks + parity_drives) - blks_allocated;

    return status;

}
/* end of fbe_parity_recovery_verify_count_buffers() */

/*!***************************************************************
 * fbe_parity_verify_count_allocated_buffers()
 ***************************************************************** 
 * @brief 
 *  This function will count the number of buffers that were
 *  allocated by the parent.
 * 
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return fbe_status_t
 *
 * @author
 *  8/30/99 - Created. JJ
 *
 ****************************************************************/
fbe_status_t fbe_parity_verify_count_allocated_buffers(fbe_raid_fruts_t * parent_fruts_ptr,
                                                    fbe_raid_siots_t * siots_p,
                                                    fbe_lba_t verify_start,
                                                    fbe_block_count_t verify_count,
                                                    fbe_s32_t *block_allocated_p)
{
    fbe_lba_t verify_end = verify_start + verify_count - 1;
    fbe_lba_t logical_parity_start = siots_p->geo.logical_parity_start;
    fbe_status_t status = FBE_STATUS_OK;
    while (parent_fruts_ptr)
    {
        fbe_lba_t end_lba;
        fbe_lba_t normalized_fruts_lba;
        if (verify_start < siots_p->start_lba)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: verify start less than start_lba %llx lba: %llx\n",
                                 (unsigned long long)verify_start,
				 (unsigned long long)siots_p->start_lba);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Since the fruts lba is a fru relative lba,
         * we need to translate it to a logical relative lba.
         */
        normalized_fruts_lba = logical_parity_start;
        status = fbe_raid_map_pba_to_lba_relative(parent_fruts_ptr->lba,
                                       parent_fruts_ptr->position,
                                       siots_p->parity_pos,
                                       siots_p->start_lba,
                                       fbe_raid_siots_get_width(siots_p),
                                       &normalized_fruts_lba,
                                       fbe_raid_siots_get_parity_stripe_offset(siots_p),
                                       fbe_raid_siots_parity_num_positions(siots_p));
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: failed to pba to lba: status = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return status;
        }

        end_lba = normalized_fruts_lba + parent_fruts_ptr->blocks - 1;

        if (!((end_lba < verify_start) ||
              (normalized_fruts_lba > verify_end)))
        {
            *block_allocated_p += (fbe_u32_t)(FBE_MIN(verify_end, end_lba)
                - FBE_MAX(normalized_fruts_lba, verify_start) + 1);
        }

        parent_fruts_ptr = fbe_raid_fruts_get_next(parent_fruts_ptr);
    }

    return status;
}
/* end of fbe_parity_verify_count_allocated_buffers() */

/*!**************************************************************
 * fbe_parity_verify_setup_resources()
 ****************************************************************
 * @brief
 *  Setup the newly allocated resources.
 *
 * @param siots_p - current I/O.               
 *
 * @return fbe_status_t
 *
 * @author
 *  10/5/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_verify_setup_resources(fbe_raid_siots_t * siots_p, 
                                               fbe_raid_fru_info_t *read_p)
{
    fbe_raid_memory_info_t  memory_info; 
    fbe_raid_memory_info_t  data_memory_info;
    fbe_status_t            status;

    /* Initialize our memory request information
     */
    status = fbe_raid_siots_init_control_mem_info(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to initialize memory request info with status = 0x%x, siots_p = 0x%p\n",
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
    status = fbe_parity_verify_setup_fruts(siots_p, 
                                           read_p,
                                           FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                           &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity:fail to setup fruts with stat=0x%x,siots=0x%p,read=0x%p\n",
                             status, siots_p, read_p);
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
                                                  read_p, NULL, NULL);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    { 
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity:fail alloc sg.siots=0x%p,stat=0x%x,read=0x%p\n",
                             siots_p, status, read_p);
        return status;
    }

    /* Assign buffers to sgs.
     */
    status = fbe_parity_verify_setup_sgs(siots_p, &data_memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) 
    { 
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                            "parity:fail setup sgs for siots=0x%p,stat=0x%x\n",
                            siots_p, status);
        return status;
    }

    /* Make sure the buffer memory we just used does not overlap into 
     * the area used for other resources. 
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_parity_verify_setup_resources()
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
 *  1/26/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_verify_setup_sgs_for_next_pass(fbe_raid_siots_t * siots_p)
{
    fbe_raid_memory_info_t  memory_info;
    fbe_status_t            status;

    /* Initialize our memory request information
     */
    status = fbe_raid_siots_init_data_mem_info(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to initialize memory request info with status = 0x%x, siots_p = 0x%p\n",
                             status, siots_p);
        return(status); 
    }

    /* Assign buffers to sgs.
     * We reset the current ptr to the beginning since we always 
     * allocate buffers starting at the beginning of the memory. 
     */
    status = fbe_parity_verify_setup_sgs(siots_p, &memory_info);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status)) 
    { 
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "parity:failed to setup sgs,stat=0x%x,siots=0x%p\n",
                             status, siots_p);
        return (status); 
    }

    /* Make sure the buffer memory we just used does not overlap into 
     * the area used for other resources. 
     */
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_parity_verify_setup_sgs_for_next_pass()
 ******************************************/

/*!***************************************************************
 * fbe_parity_recovery_verify_setup_sgs()
 ****************************************************************
 * @brief
 *  This function will setup the sg lists.
 *
 * @param siots_p    - ptr to the fbe_raid_siots_t
 * @param memory_info_p - Pointer to memory request information
 * @param verify_start - Verify start lba
 * @parma verify_count - Veriyf block count (for each position)
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_parity_recovery_verify_setup_sgs(fbe_raid_siots_t * siots_p,
                                                  fbe_raid_memory_info_t *memory_info_p, 
                                                  fbe_lba_t verify_start, 
                                                  fbe_block_count_t verify_count)
{
    fbe_raid_siots_t *parent_siots_p = fbe_raid_siots_get_parent(siots_p);
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_fruts_t *parent_read_fruts_ptr = NULL;
    fbe_raid_fruts_t *parent_read2_fruts_ptr = NULL;
    fbe_lba_t logical_parity_start = siots_p->geo.logical_parity_start;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_sg_element_t *overlay_sg_ptr = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    fbe_raid_siots_get_read_fruts(parent_siots_p, &parent_read_fruts_ptr);
    fbe_raid_siots_get_read2_fruts(parent_siots_p, &parent_read2_fruts_ptr);

    while (read_fruts_ptr)
    {
        fbe_sg_element_t *sgl_ptr;
        fbe_u16_t sg_total = 0;
        fbe_u32_t dest_byte_count =0;
        fbe_block_count_t blks_assigned = 0;

        sgl_ptr = fbe_raid_fruts_return_sg_ptr(read_fruts_ptr);

        if (parent_read_fruts_ptr && parent_read_fruts_ptr->position == read_fruts_ptr->position) 
        {
            fbe_lba_t relative_fruts_lba,   /* The Parity relative FRUTS lba. */
                  end_lba,
                  verify_end;
            fbe_block_count_t fruts_blocks; /* The first fruts block count.   */

            if (verify_start < siots_p->start_lba)
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: verify start less than start_lba %llx lba: %llx\n",
                                     (unsigned long long)verify_start,
                                     (unsigned long long)siots_p->start_lba);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Since the fruts lba is a physical lba, and the
             * verify_start is a parity relative logical address,
             * map the fruts lba to a parity logical address.
             */
            relative_fruts_lba = logical_parity_start;
            status = fbe_raid_map_pba_to_lba_relative(parent_read_fruts_ptr->lba,
                                               parent_read_fruts_ptr->position,
                                               siots_p->parity_pos,
                                               siots_p->start_lba,
                                               fbe_raid_siots_get_width(siots_p),
                                               &relative_fruts_lba,
                                               fbe_raid_siots_get_parity_stripe_offset(siots_p),
                                               fbe_raid_siots_parity_num_positions(siots_p));
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity:fail map pba to lba:stat=0x%x,siots=0x%p\n",
                                     status, siots_p);
                return status;
            }

            fruts_blocks = parent_read_fruts_ptr->blocks;
            end_lba = relative_fruts_lba + fruts_blocks - 1;
            verify_end = verify_start + verify_count - 1;

            /* Fill in the FRUTS' sg list by copying the sgs
             * from the already allocated buffers in the
             * parent's read_fruts.
             */
            fbe_raid_fruts_get_sg_ptr(parent_read_fruts_ptr, &overlay_sg_ptr);
            status = fbe_raid_verify_setup_overlay_sgs(relative_fruts_lba,
                                                       fruts_blocks,
                                                       overlay_sg_ptr,
                                                       sgl_ptr,
                                                       memory_info_p,
                                                       verify_start,
                                                       verify_count,
                                                       &blks_assigned);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: failed to overlay sgs : status = 0x%x, siots_p = 0x%p\n",
                                     status, siots_p);
                return status;
            }
            parent_read_fruts_ptr = fbe_raid_fruts_get_next(parent_read_fruts_ptr);
        }

        if (parent_read2_fruts_ptr && parent_read2_fruts_ptr->position == read_fruts_ptr->position) 
        {
            if (blks_assigned < verify_count)
            {
                fbe_block_count_t extra_blocks = verify_count - blks_assigned;
                fbe_block_count_t  more_blks = 0;
                fbe_lba_t relative_fruts2_lba;
                fbe_block_count_t fruts_blocks;
                /* Adjust the sgl_ptr.
                 */
                if (blks_assigned)
                {
                     while ((++sgl_ptr)->count != 0);
                }
    
                if (verify_start < siots_p->start_lba)
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "parity: verify start less than start_lba %llx lba: %llx\n",
                                         (unsigned long long)verify_start,
                                         (unsigned long long)siots_p->start_lba);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
    
                /* Since the fruts lba is a physical lba, and the
                 * verify_start is a parity relative logical address,
                 * map the fruts lba to a parity logical address.
                 */
                relative_fruts2_lba = logical_parity_start;
                status = fbe_raid_map_pba_to_lba_relative(parent_read2_fruts_ptr->lba,
                                                          parent_read2_fruts_ptr->position,
                                                          siots_p->parity_pos,
                                                          siots_p->start_lba,
                                                          fbe_raid_siots_get_width(siots_p),
                                                          &relative_fruts2_lba,
                                                          fbe_raid_siots_get_parity_stripe_offset(siots_p),
                                                          fbe_raid_siots_parity_num_positions(siots_p));
                if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "parity:fail map pba to lba:stat=0x%x,siots=0x%p\n",
                                         status, siots_p);
                    return status;
                }

                fruts_blocks = parent_read2_fruts_ptr->blocks;
                /* There's only a parent_read2_fruts_ptr for this position
                 */
                if ((blks_assigned == 0) &&
                    (siots_p->algorithm != R5_MR3_VR) && 
                    (siots_p->algorithm != R5_DEG_VR) && 
                    (siots_p->algorithm != R5_DEG_RVR) &&
                    (siots_p->algorithm != R5_RCW_VR))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "parity: algorithm: not expected %d\n", siots_p->algorithm);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                /* Fill in the FRUTS' sg list by copying the sgs
                 * from the already allocated buffers in the
                 * parent's read2_fruts.
                 */
                fbe_raid_fruts_get_sg_ptr(parent_read2_fruts_ptr, &overlay_sg_ptr);
                status = fbe_raid_verify_setup_overlay_sgs(relative_fruts2_lba,
                                                           (fbe_block_count_t)parent_read2_fruts_ptr->blocks,
                                                           overlay_sg_ptr,
                                                           sgl_ptr,
                                                           memory_info_p,
                                                           verify_start + blks_assigned,
                                                           extra_blocks,
                                                           &more_blks);
                if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "parity: failed to overlay sgs : status = 0x%x, siots_p = 0x%p "
                                         "relative_fruts2_lba = 0x%llx, overlay_sg_ptr = 0x%p, memory_info_p = 0x%p\n",
                                         status, siots_p, (unsigned long long)relative_fruts2_lba, overlay_sg_ptr, memory_info_p);
                    return status;
                }

                if ((more_blks != extra_blocks) && !fbe_raid_geometry_needs_alignment(raid_geometry_p))
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                         "parity: more_blks > extra_blocks 0x%llx 0x%llx\n",
                                         (unsigned long long)more_blks,
                                         (unsigned long long)extra_blocks);
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                blks_assigned += more_blks;
            } 
            else if (blks_assigned != verify_count)
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: blks_assigned != verify_count 0x%llx 0x%llx\n",
                                     (unsigned long long)blks_assigned,
                                     (unsigned long long)verify_count);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            parent_read2_fruts_ptr = fbe_raid_fruts_get_next(parent_read2_fruts_ptr);
        }

        /* Assign new buffers for the remaining blocks
         */
        if (blks_assigned < verify_count) {
            fbe_block_count_t extra_blocks = verify_count - blks_assigned;
            /* Adjust the sgl_ptr.
             */
            if (blks_assigned)
            {
                while ((++sgl_ptr)->count != 0);
            }
            if(!fbe_raid_get_checked_byte_count(extra_blocks, &dest_byte_count))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "%s: byte count exceeding 32bit limit \n",
                                      __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE; 
            }

            /* Assign new buffers.
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

        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
    }


    if (((parent_read_fruts_ptr != NULL) && (parent_read_fruts_ptr->position != siots_p->parity_pos)) || 
        ((parent_read_fruts_ptr == NULL) && (parent_read2_fruts_ptr != NULL)))
	{
        if (parent_read_fruts_ptr != NULL)
        {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: 0x%p 0x%p %d %d \n",
                             parent_read_fruts_ptr, parent_read2_fruts_ptr,
                             parent_read_fruts_ptr->position, siots_p->parity_pos);
        }
        else
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: 0x%p 0x%p %d \n",
                             parent_read_fruts_ptr, parent_read2_fruts_ptr,
                             siots_p->parity_pos);
        }
        return FBE_STATUS_GENERIC_FAILURE;
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
                             "parity: failed to validate sgs: read_fruts_ptr 0x%p "
                             "siots_p = 0x%p. status = 0x%x\n",
                             read_fruts_ptr, siots_p, status);
        return status;
    }

    return status;
}/* end of fbe_parity_recovery_verify_setup_sgs() */

/*!***************************************************************
 * fbe_raid_verify_setup_overlay_sgs()
 ****************************************************************
 * @brief
 *  This function will set up the sgs in the fruts.
 *
 * @param base_lba - lba of fruts to copy from.
 * @param base_blocks - blocks of fruts to copy from.
 * @param base_sg_ptr - sg ptr of fruts to copy from.
 * @param sgl_ptr - destination sg list.
 * @param memory_info_p - Pointer to memory request information
 * @param verify_start - lba of destination fruts.
 * @param verify_count - blocks of destination fruts.
 * @param blocks_p - blocks overlayed in sgs
 * 
 * @return fbe_status_t
 *
 ****************************************************************/
 fbe_status_t fbe_raid_verify_setup_overlay_sgs(fbe_lba_t base_lba,
                                                fbe_block_count_t base_blocks,
                                                fbe_sg_element_t *base_sg_ptr,
                                                fbe_sg_element_t * sgl_ptr,
                                                fbe_raid_memory_info_t *memory_info_p,
                                                fbe_lba_t verify_start,
                                                fbe_block_count_t verify_count,
                                                fbe_block_count_t * blocks_p)
{
    fbe_block_count_t blks_assigned;
    fbe_lba_t end_lba, verify_end;
    fbe_u16_t sg_total = 0;
    fbe_u32_t dest_byte_count =0;
    fbe_status_t status = FBE_STATUS_OK;
    blks_assigned = 0;

    end_lba = base_lba + base_blocks - 1,
        verify_end = verify_start + verify_count - 1;

    if (verify_end < base_lba)
    {
        if(!fbe_raid_get_checked_byte_count(verify_count, &dest_byte_count))
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "%s: byte count exceeding 32bit limit \n",
                                   __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE; 
        }
        /* Assign newly allocated memory for the whole verify region.
         */
        sg_total = fbe_raid_sg_populate_with_memory(memory_info_p,
                                         sgl_ptr, 
                                         dest_byte_count);
        if (sg_total == 0)
        {
            fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                   "%s: failed to populate sg for: memory_info_p = 0x%p\n",
                                   __FUNCTION__,memory_info_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        blks_assigned = verify_count;
    }
    else
    {
        fbe_sg_element_t *parent_sgl_ptr;
        fbe_s64_t assigned_blocks,
          extra_blocks;
        fbe_sg_element_with_offset_t tmp_sgd;

        parent_sgl_ptr = base_sg_ptr;

        assigned_blocks = extra_blocks = 0;

        if ((verify_start >= base_lba) && (verify_start <= end_lba))
        {
            assigned_blocks = FBE_MIN(verify_count, (end_lba - verify_start + 1));

            fbe_sg_element_with_offset_init(&tmp_sgd, 
                                            (fbe_u32_t)(verify_start - base_lba) * FBE_BE_BYTES_PER_BLOCK, /* offset*/ 
                                            parent_sgl_ptr,
                                            NULL /* use default get next sg fn */);
        }
        else if (verify_start < base_lba)
        {
            /* Assign new buffers first,
             * then assign the ones from parent.
             */
            extra_blocks = base_lba - verify_start;

            if(!fbe_raid_get_checked_byte_count(extra_blocks, &dest_byte_count))
            {
                fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                       "%s: byte count exceeding 32bit limit \n",
                                       __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE; 
            }
            sg_total = fbe_raid_sg_populate_with_memory(memory_info_p,
                                                        sgl_ptr, 
                                                        dest_byte_count);
            if (sg_total == 0)
            {
                fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                       "%s: failed to populate sg for: memory_info_p = 0x%p\n",
                                       __FUNCTION__,memory_info_p);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Adjust the sgl_ptr.
             */
            while ((++sgl_ptr)->count != 0);

            assigned_blocks = FBE_MIN(base_blocks, (verify_end - base_lba + 1));
            
            fbe_sg_element_with_offset_init(&tmp_sgd, 0, /* offset*/ parent_sgl_ptr,
                                            NULL /* use default get next sg fn */);
        }

        if (assigned_blocks > 0)
        {
            fbe_u16_t sg_init = 0;

            if((assigned_blocks * FBE_BE_BYTES_PER_BLOCK) > FBE_U32_MAX)
            {
                fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                       "parity: %s assigned blocks crossing 32bit limit 0x%llx\n",
                                       __FUNCTION__, (unsigned long long)(assigned_blocks * FBE_BE_BYTES_PER_BLOCK));
                *blocks_p = 0;
                return FBE_STATUS_GENERIC_FAILURE;
            }
            status = fbe_raid_sg_clip_sg_list(&tmp_sgd, 
                                                                  sgl_ptr, 
                                                                  (fbe_u32_t)(assigned_blocks * FBE_BE_BYTES_PER_BLOCK),
                                                                  &sg_init);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                       "parity: %s failed to clip sg list : status = 0x%x, sgl_ptr = 0x%p\n",
                                       __FUNCTION__, 
                                       status, sgl_ptr);
                *blocks_p = 0;
                return status;
            }
        }

        blks_assigned = extra_blocks + assigned_blocks;
    }

    *blocks_p = blks_assigned;
    return FBE_STATUS_OK;

}
/*****************************************
 * end of fbe_raid_verify_setup_overlay_sgs()
 *****************************************/
/*!***************************************************************
 * fbe_parity_verify_strip()
 ****************************************************************
 * @brief
 *  This function will verify the contents by sector strip.
 *
 * @param siots_p    - ptr to the fbe_raid_siots_t
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_parity_verify_strip(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status;
    fbe_xor_parity_reconstruct_t verify;
    fbe_raid_fruts_t *read_fruts_p = NULL;

    /* Initialize per position entries.
     */
    status = fbe_parity_verify_strip_setup(siots_p, &verify);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
         fbe_raid_siots_trace(siots_p,
                              FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                              "parity: failed to set up for verify operation: status = 0x%x, "
                              "siots_p = 0x%p\n", 
                              status, siots_p);

        return status;
    }

    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_p);

    /* The seed is a physical address.
     */
    verify.seed = read_fruts_p->lba;
    verify.count = siots_p->parity_count;
    verify.data_disks = (fbe_raid_siots_get_width(siots_p) - 
                         fbe_raid_siots_parity_num_positions(siots_p));
    verify.eboard_p = &siots_p->misc_ts.cxts.eboard;
    verify.error_counts_p = &siots_p->vcts_ptr->curr_pass_eboard;
    verify.eregions_p = &siots_p->vcts_ptr->error_regions;
    verify.status = FBE_XOR_STATUS_INVALID;
    verify.option = 0;

    /* Yes if we already retried or if we are not going to retry (single region), 
     * no if not yet retried.
     */
    if (fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE) ||
        fbe_raid_siots_is_retry(siots_p))
    {
        verify.b_final_recovery_attempt = FBE_TRUE;
    }
    else
    {
        verify.b_final_recovery_attempt = FBE_FALSE;
    }
    
    status = fbe_raid_siots_parity_rebuild_pos_get( siots_p, 
                                           &verify.parity_pos[0], 
                                           &verify.rebuild_pos[0] );
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
         fbe_raid_siots_trace(siots_p,
                              FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                              "parity: failed to get rebuild position: status = 0x%x, "
                              "siots_p = 0x%p\n", 
                              status, siots_p);

        return status;
    }

    verify.b_preserve_shed_data = FBE_FALSE;

    /* The current pass counts for the vcts should be cleared since
     * we are beginning another XOR.  Later on, the counts from this pass
     * will be added to the counts from other passes.
     */
    fbe_raid_siots_init_vcts_curr_pass( siots_p );

    fbe_raid_fruts_get_retry_err_bitmap(read_fruts_p, &verify.eboard_p->retry_err_bitmap);
    if ((siots_p->dead_pos != FBE_RAID_INVALID_DEAD_POS) &&
        (fbe_raid_get_bitcount(verify.eboard_p->retry_err_bitmap) > 1))
    {
        fbe_raid_position_bitmask_t deg_bitmask;
        fbe_raid_siots_get_degraded_bitmask(siots_p, &deg_bitmask);
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "unexpected retry bitmask: 0x%x hme bitmask: 0x%x dead_bitmask: 0x%x\n",
                             verify.eboard_p->retry_err_bitmap, verify.eboard_p->hard_media_err_bitmap,
                             deg_bitmask);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Set XOR debug options.
     */
    fbe_raid_xor_set_debug_options(siots_p, 
                                   &verify.option, 
                                   FBE_FALSE /* Don't generate error trace */);

    /* Perform the verify operation.
     */
    status = fbe_xor_lib_parity_verify(&verify);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_position_bitmask_t deg_bitmask;
        fbe_raid_siots_get_degraded_bitmask(siots_p, &deg_bitmask);
        fbe_raid_siots_trace(siots_p,
                             FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: failed to perform xor operation: verify unexpected error 0x%x retry bitmask: 0x%x hme bitmask: 0x%x dead_bitmask: 0x%x\n",
                             status, verify.eboard_p->retry_err_bitmap, verify.eboard_p->hard_media_err_bitmap,
                             deg_bitmask);
        return status;
    }
    return status;
}
/**************************************
 * end fbe_parity_verify_strip()
 **************************************/

/*!***************************************************************
 * fbe_parity_verify_strip_setup()
 ****************************************************************
 * @brief
 *   Verify an extent for this siots.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t 
 * @param bitkey_v - the vec of positions
 * @param sgd_v - the vec of sg descriptors
 *   data_position_v [I]  - the vec of the data positions
 *
 * @return fbe_status_t
 *
 *  @notes
 *   The verify position is always first in the write fruts list.
 *
 ****************************************************************/
static fbe_status_t fbe_parity_verify_strip_setup(fbe_raid_siots_t * siots_p,
                                                  fbe_xor_parity_reconstruct_t *verify_p)
{
    fbe_raid_fruts_t *fruts_p = NULL;
    fbe_u32_t data_pos;
    /* First initialize the positions we are verifying from.
     */
    fbe_raid_siots_get_read_fruts(siots_p, &fruts_p);

    while (fruts_p != NULL)
    {
        fbe_status_t status = FBE_STATUS_OK;
        if (fruts_p->position >= fbe_raid_siots_get_width(siots_p))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: read position >= width %d %d\n",
                                 fruts_p->position, fbe_raid_siots_get_width(siots_p));
            return FBE_STATUS_GENERIC_FAILURE;
        }
        verify_p->sg_p[fruts_p->position] = fbe_raid_fruts_return_sg_ptr(fruts_p);

        /* Determine the data position of this fru and save it for possible later
         * use in RAID 6 algorithms.
         */
        status = FBE_RAID_EXTENT_POSITION(fruts_p->position,
                                          siots_p->parity_pos,
                                          fbe_raid_siots_get_width(siots_p),
                                          fbe_raid_siots_parity_num_positions(siots_p),
                                          &data_pos);
        verify_p->data_position[fruts_p->position] = data_pos;
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: failed to convert position: status = 0x%x, siots_p = 0x%p, "
                                        "fruts_p = 0x%p\n",
                                        status, siots_p, fruts_p);
            return status;
        }
        /* Init the bitkey for this position.
         */
        verify_p->bitkey[fruts_p->position] = (1 << fruts_p->position);
        fruts_p = fbe_raid_fruts_get_next(fruts_p);
    }

#if 0
    if (siots_p->algorithm == R5_CORRUPT_CRC)
    {
        fbe_lba_t min_rebuild_offset = rg_min_rb_offset(siots_p, fbe_raid_siots_get_width(siots_p));
 
        *cnt_vec =
            ((siots_p->dead_pos == (fbe_u16_t) - 1) || ((siots_p->parity_start + siots_p->parity_count - 1) < min_rebuild_offset))
            ? siots_p->parity_count
            : (fbe_u32_t)(min_rebuild_offset - siots_p->parity_start);

    }
#endif
    return FBE_STATUS_OK;
}
/*******************************
 * end fbe_parity_verify_strip_setup()
 *******************************/
/*!***************************************************************
 * fbe_parity_recovery_verify_copy_user_data()
 ****************************************************************
 * @brief
 *  This function will copy the user data for the write into the stripe
 *  that we just verified.
 *
 * @param siots_p    - ptr to the fbe_raid_siots_t
 *
 * @return fbe_status_t
 ****************************************************************/
fbe_status_t fbe_parity_recovery_verify_copy_user_data(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_xor_mem_move_t xor_mem_move;

    status = fbe_parity_recovery_verify_fill_move(siots_p, &xor_mem_move);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_recov_verify_cp_userdata" ,
                             "recovery verify setup fail for stios=0x%p with status 0x%x\n",
                             siots_p, status);

        return status;
    }

    /* By default the xor options is 0.
     */
    xor_mem_move.option = 0;

    /* Only check the checksums if requested to do so*/
    if (fbe_raid_siots_should_check_checksums(siots_p) == FBE_TRUE)
    {
        xor_mem_move.option |= FBE_XOR_OPTION_CHK_CRC;
    }

    /* Init the uncorrectable error bitmap from the
     * value passed in by the caller.
     * sub_iots_ptr->misc_ts.cxts.eboard.u_bitmap =
     *     sub_iots_ptr->hard_media_err_bitmap;
     */

    /* Copy the user data using the xor library.
     */
    status = fbe_xor_lib_parity_verify_copy_user_data(&xor_mem_move);

    /* Save status.
     */
    fbe_raid_siots_set_xor_command_status(siots_p, xor_mem_move.status);
    return status;
}
/*****************************************************
 * end of fbe_parity_recovery_verify_copy_user_data() 
 ****************************************************/

/*!***************************************************************
 * fbe_parity_recovery_verify_construct_parity()
 ****************************************************************
 * @brief
 *  This will do a data move to put new user data from the parent
 *  into the stripe to write out.
 *
 * @param siots_p    - ptr to the fbe_raid_siots_t
 *
 * @return fbe_status_t
 *
 * @author
 *  6/26/00 - Created. Kevin Schleicher
 *
 ****************************************************************/
fbe_status_t fbe_parity_recovery_verify_construct_parity(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_xor_recovery_verify_t xor_recovery_verify;
    fbe_u32_t width = 0;

    status = fbe_parity_recovery_verify_xor_fill(siots_p, &xor_recovery_verify);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
	fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_recov_verify_construct_parity" ,
					"parity: call to recovery_verify_xor_fill fail:siots_p:0x%p,stat:0x%x\n",
					siots_p, status);

        return status;
    }

    /* By default there are no options.
     */
    xor_recovery_verify.option = 0;

    if (fbe_raid_siots_should_check_checksums(siots_p) == FBE_TRUE)
    {
        xor_recovery_verify.option |= FBE_XOR_OPTION_CHK_CRC;
    }

    /* Init the uncorrectable error bitmap from the
     * value passed in by the caller.
     * sub_iots_ptr->misc_ts.cxts.eboard.u_bitmap =
     *     sub_iots_ptr->hard_media_err_bitmap;
     */
    status = fbe_raid_siots_parity_rebuild_pos_get( siots_p, 
                                           &xor_recovery_verify.parity_pos[0], 
                                           &xor_recovery_verify.rebuild_pos[0] );
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_recov_verify_construct_parity" ,
                             "parity: fail to get parity or rebuild pos:siots_p=0x%p,stat=0x%x\n",
                             siots_p, status);
        return status;
    }

    width = fbe_raid_siots_get_width(siots_p);
    if (RAID_COND(width == 0))
    {
        fbe_raid_siots_trace(siots_p, 
                             FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_recov_verify_construct_parity" ,
                             "parity:got unexpect width of rg. width=0x%x\n",
                             width);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Populate the remaining fields of the reconstruct request.
     */
    xor_recovery_verify.array_width = width;
    xor_recovery_verify.eboard_p = fbe_raid_siots_get_eboard(siots_p);
    xor_recovery_verify.data_disks = (width - fbe_raid_siots_parity_num_positions(siots_p));
    xor_recovery_verify.eregions_p = &siots_p->vcts_ptr->error_regions;

    /* Copy the user data into the stripe.
     */
    status = fbe_xor_lib_parity_recovery_verify_const_parity(&xor_recovery_verify);

    return status;
}                               
/* end of fbe_parity_recovery_verify_construct_parity() */

/*!***************************************************************
 * fbe_parity_recovery_verify_xor_fill()
 ****************************************************************
 * @brief
 *  This function will calculate the information
 *  needed for in the vectors for rvr xor and fill them in.
 *
 * @param siots_p         - ptr to the fbe_raid_siots_t 
 * @param xor_recovery_verify_p - ptr to the vectors to fill in 
 *          check_csum      - flag to check the csum
 *          vector_size     - ptr to location to store count of vectors
 *          move_op         - flag to indicate that this is a move op. 
 *
 * @return fbe_status_t
 *
 * @author
 *  7/12/00 - Created. Kevin Schleicher
 *
 ****************************************************************/
static fbe_status_t fbe_parity_recovery_verify_xor_fill(fbe_raid_siots_t * siots_p,
                                                        fbe_xor_recovery_verify_t *xor_recovery_verify_p)
{
    fbe_lba_t verify_start, verify_end;
    fbe_block_count_t verify_count;
    fbe_u32_t fru_cnt;
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    verify_start = siots_p->parity_start;
    verify_count = siots_p->parity_count;
    verify_end = verify_start + verify_count - 1;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);

    /* Only one seed/count for entire operation.
     */
    xor_recovery_verify_p->seed = read_fruts_ptr->lba;
    xor_recovery_verify_p->offset = siots_p->misc_ts.cxts.eboard.raid_group_offset;
    xor_recovery_verify_p->count = siots_p->parity_count;

    for (fru_cnt = 0; fru_cnt < fbe_raid_siots_get_width(siots_p); fru_cnt++)
    {
        fbe_u32_t data_pos;
        xor_recovery_verify_p->fru[fru_cnt].bitkey_p = (1 << read_fruts_ptr->position);
        fbe_raid_fruts_get_sg_ptr(read_fruts_ptr, &xor_recovery_verify_p->fru[fru_cnt].sg_p);
        xor_recovery_verify_p->fru[fru_cnt].offset = 0;
        status = 
            FBE_RAID_EXTENT_POSITION(read_fruts_ptr->position,
                                     siots_p->parity_pos,
                                     fbe_raid_siots_get_width(siots_p),
                                     fbe_raid_siots_parity_num_positions(siots_p),
                                     &data_pos);
        xor_recovery_verify_p->fru[fru_cnt].data_position = data_pos;
        if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                        "parity: failed to convert position: status = 0x%x, siots_p = 0x%p, "
                                        "read_fruts_ptr = 0x%p\n",
                                        status, siots_p, read_fruts_ptr);
            return status;
        }
        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
    } /* end of for */
    return FBE_STATUS_OK;
}
/* end fbe_parity_recovery_verify_xor_fill() */

/*!***************************************************************
 * fbe_parity_recovery_verify_fill_move()
 ****************************************************************
 * @brief
 *  This function will calculate the information
 *  needed for in the vectors for rvr xor and fill them in.
 *
 * @param siots_p         - ptr to the fbe_raid_siots_t 
 * @param xor_recovery_verify_p - ptr to the vectors to fill in 
 *          check_csum      - flag to check the csum
 *          vector_size     - ptr to location to store count of vectors
 *          move_op         - flag to indicate that this is a move op. 
 *
 * @return fbe_status_t
 *
 * @author
 *  7/12/00 - Created. Kevin Schleicher
 *
 ****************************************************************/
static fbe_status_t fbe_parity_recovery_verify_fill_move(fbe_raid_siots_t * siots_p,
                                                         fbe_xor_mem_move_t *mem_move_p)
{
    fbe_lba_t verify_start, verify_end;
    fbe_block_count_t verify_count;
    fbe_raid_siots_t *parent_siots_p = fbe_raid_siots_get_parent(siots_p);
    fbe_raid_fruts_t *read_fruts_ptr = NULL;
    fbe_raid_fruts_t *parent_wfruts_p = NULL;
    fbe_raid_fruts_t *parent_rfruts_p = NULL;
    fbe_raid_fruts_t *parent_r2fruts_p = NULL;
    fbe_lba_t logical_parity_start = siots_p->geo.logical_parity_start;
    fbe_u32_t total_disks = 0;
    fbe_u32_t fru_cnt;
    fbe_status_t status = FBE_STATUS_OK;
    verify_start = siots_p->parity_start;
    verify_count = siots_p->parity_count;
    verify_end = verify_start + verify_count - 1;
    fbe_raid_siots_get_read_fruts(siots_p, &read_fruts_ptr);
    fbe_raid_siots_get_read_fruts(parent_siots_p, &parent_rfruts_p);
    fbe_raid_siots_get_read2_fruts(parent_siots_p, &parent_r2fruts_p);
    fbe_raid_siots_get_write_fruts(parent_siots_p, &parent_wfruts_p);

    for (fru_cnt = 0; fru_cnt < fbe_raid_siots_get_width(siots_p); fru_cnt++)
    {
        /* Look at the write data on data disks.
         * For parity, we'll reconstruct anyway.
         */
        if ((read_fruts_ptr->position != siots_p->parity_pos) &&
            (parent_wfruts_p) &&
            (parent_wfruts_p->position == read_fruts_ptr->position))
        {
            fbe_lba_t parity_rel_parent_fru_lba,
            write_start,
            write_end;
            fbe_block_count_t write_blocks;

            if (verify_start < siots_p->start_lba)
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: verify start less than start_lba %llx lba: %llx\n",
                                     (unsigned long long)verify_start,
				     (unsigned long long)siots_p->start_lba);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Since the fruts lba is a physical lba, and the
             * verify_start is a parity relative logical address,
             * map the fruts lba to a parity logical address.
             */
            parity_rel_parent_fru_lba = logical_parity_start;
            status = fbe_raid_map_pba_to_lba_relative(parent_wfruts_p->lba,
                                                      parent_wfruts_p->position,
                                                      siots_p->parity_pos,
                                                      siots_p->start_lba,
                                                      fbe_raid_siots_get_width(siots_p),
                                                      &parity_rel_parent_fru_lba,
                                                      fbe_raid_siots_get_parity_stripe_offset(siots_p),
                                                      fbe_raid_siots_parity_num_positions(siots_p));
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                /*Split trace to two lines*/
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity:fail map pba to lba:stat=0x%x,siots=0x%p>\n",
                                     status, siots_p);
				fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parent_wfruts_p=0x%p<\n",
                                     parent_wfruts_p);
                return status;
            }
            write_start = parity_rel_parent_fru_lba,
            write_end = parity_rel_parent_fru_lba + parent_wfruts_p->blocks - 1,
            write_blocks = parent_wfruts_p->blocks;

            if (RAID_COND(write_end == FBE_LBA_INVALID))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, "write_end == FBE_LBA_INVALID");
                return FBE_STATUS_GENERIC_FAILURE;
            }

            if (siots_p->algorithm == R5_MR3_VR)
            {
                /* We have to remove the pre-read portion.
                 */
                if ((parent_rfruts_p) && (parent_rfruts_p->position == read_fruts_ptr->position))
                {
                    /* Subtract pre-read data from the top.
                     */
                    write_start += parent_rfruts_p->blocks;
                    write_blocks -= parent_rfruts_p->blocks;

                    /* If there is totally no pre-read data, 
                     * then write_start must be greater than write_end, 
                     * i.e. write_start > write_end,
                     * we must exclude this situation below.
                     */

                    parent_rfruts_p = fbe_raid_fruts_get_next(parent_rfruts_p);
                }
                if ((parent_r2fruts_p) && (parent_r2fruts_p->position == read_fruts_ptr->position))
                {
                    fbe_lba_t relative_fruts_lba;
                    relative_fruts_lba = logical_parity_start;
                    /* Map the physical address to a parity relative address */
                    status = fbe_raid_map_pba_to_lba_relative(parent_r2fruts_p->lba,
                                                              parent_r2fruts_p->position,
                                                              siots_p->parity_pos,
                                                              siots_p->start_lba,
                                                              fbe_raid_siots_get_width(siots_p),
                                                              &relative_fruts_lba,
                                                              fbe_raid_siots_get_parity_stripe_offset(siots_p),
                                                              fbe_raid_siots_parity_num_positions(siots_p) );
                    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity:fail map pba to lba:stat=0x%x,siots=0x%p>\n",
                                     status, siots_p);
			fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
		                             "parent_r2fruts_p=0x%p<\n",
		                             parent_r2fruts_p);
                        return status;
                    }

                    if(RAID_COND(write_start > relative_fruts_lba))
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                             "parity: write_start 0x%llx > relative_fruts_lba 0x%llx\n",
                                             (unsigned long long)write_start,
					     (unsigned long long)relative_fruts_lba);
                        return FBE_STATUS_GENERIC_FAILURE;
                    }
                    /* Subtract pre-read data from the bottom.
                     */
                    write_end -= parent_r2fruts_p->blocks;
                    write_blocks -= parent_r2fruts_p->blocks;

                    parent_r2fruts_p = fbe_raid_fruts_get_next(parent_r2fruts_p);
                }
            }

            /* There might be some host/cache data that falls into this verify range.
             */
            if ((write_blocks > 0) &&
                (!((write_start > write_end) ||
                   (write_end < verify_start) ||
                   (write_start > verify_end))))
            {
                fbe_u32_t source_offset;
                fbe_u32_t dest_offset;

                /* Initialize.
                 */
                fbe_raid_fruts_get_sg_ptr(parent_wfruts_p, &mem_move_p->fru[total_disks].source_sg_p);
                fbe_raid_fruts_get_sg_ptr(read_fruts_ptr, &mem_move_p->fru[total_disks].dest_sg_p);

                if (siots_p->algorithm == R5_MR3_VR)
                {
                    /* Adjust the source sgl to point to the write data.
                     */
                    mem_move_p->fru[total_disks].source_offset = 
                    fbe_raid_get_sg_ptr_offset(&mem_move_p->fru[total_disks].source_sg_p,
                                               (fbe_u32_t)(write_start - parity_rel_parent_fru_lba));

                    if (RAID_COND(mem_move_p->fru[total_disks].source_offset != 0))
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, "mem_move_p->fru[total_disks].source_offset %x != 0",
                                             mem_move_p->fru[total_disks].source_offset);
                        return FBE_STATUS_GENERIC_FAILURE;
                    }
                }

                if (verify_start <= write_start)
                {
                    /* Host data starts below the verify range.
                     * (or they are equal)
                     * Advance the destination, which is the
                     * verifies sgl to line up with the
                     * hosts sgl.
                     */
                    source_offset = 0;
                    dest_offset = (fbe_u32_t)(write_start - verify_start);

                }
                else
                {
                    /* The host data starts above the verify range.
                     * Advance the source, which is the hosts sgl to
                     * line up with the verifies sgl.
                     */
                    source_offset = (fbe_u32_t)(verify_start - write_start);
                    dest_offset = 0;

                }
                mem_move_p->fru[total_disks].source_offset = 
                fbe_raid_get_sg_ptr_offset(&mem_move_p->fru[total_disks].source_sg_p, 
                                           source_offset);
                mem_move_p->fru[total_disks].dest_offset = 
                fbe_raid_get_sg_ptr_offset(&mem_move_p->fru[total_disks].dest_sg_p, 
                                           dest_offset);

                /* The count is the end of the verify, or the end of
                 * the host data, whichever is smaller minus the
                 * start of the verify region or the start of the host
                 * data, whichever is greater. plus 1.
                 */
                mem_move_p->fru[total_disks].count = 
                (fbe_u32_t) (FBE_MIN(verify_end, write_end)
                             - FBE_MAX(write_start, verify_start) + 1);

                /* Seed for the data disks is always the read fruts lba +
                 * the offset the write is going to. (calculated above)
                 */
                mem_move_p->fru[total_disks].seed = read_fruts_ptr->lba + dest_offset;

                /* Increment the vector_size for the number of entries we have
                 * in the vectors.
                 */
                total_disks++;
            }
            parent_wfruts_p = fbe_raid_fruts_get_next(parent_wfruts_p);
        }
        read_fruts_ptr = fbe_raid_fruts_get_next(read_fruts_ptr);
    }                           /* end of for */

    mem_move_p->disks = total_disks;
    mem_move_p->offset = fbe_raid_siots_get_eboard_offset(siots_p);
    mem_move_p->eboard_p = &siots_p->misc_ts.cxts.eboard; 
    return status;
}
/* end fbe_parity_recovery_verify_fill_move() */

/*!***************************************************************
 * fbe_parity_record_errors()
 ****************************************************************
 * @brief
 *  This function will record the errors in verify.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 *
 * @return VALUE:
 *  FBE_RAID_STATE_STATUS_DONE - No Error.
 *  FBE_RAID_STATE_STATUS_WAITING - If retries were performed.
 *
 * @author
 *  06/23/00 - Rewrote for XOR Driver. Rob Foley
 *
 ****************************************************************/

fbe_raid_state_status_t fbe_parity_record_errors(fbe_raid_siots_t * const siots_p, 
                                                 fbe_lba_t verify_address)
{
    fbe_raid_state_status_t return_status = FBE_RAID_STATE_STATUS_DONE;
    fbe_u16_t array_pos;
    fbe_u16_t write_bitmap = 0;

    /* Verify requests should always
     * record errors *within* the verify range.
     */
    if (verify_address < siots_p->start_lba)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "parity: verify start less than start_lba %llx lba: %llx alg: 0x%x\n",
                             (unsigned long long)verify_address,
			     (unsigned long long)siots_p->start_lba,
			     siots_p->algorithm);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    {
        fbe_u16_t check_bits = 0;

        /* Determine which bit positions to ignore.
         */
        for (array_pos = 0; array_pos < fbe_raid_siots_get_width(siots_p); array_pos++)
        {
            fbe_u16_t bitkey = 1 << array_pos;

            if ((siots_p->algorithm != R5_RD_VR) && (bitkey & write_bitmap))
            {
                /* Data in this portion will be covered by host data,
                 * so we don't need to record any errors detected here.
                 */
                continue;
            }
            else
            {
                check_bits |= bitkey;
            }
        }

        /* Record errors for relevant array positions.
         */
        return_status = fbe_raid_siots_record_errors(siots_p,
                                         fbe_raid_siots_get_width(siots_p),
                                         (fbe_xor_error_t *) & siots_p->misc_ts.cxts.eboard,
                                         check_bits,
                                         FBE_TRUE,
                                         FBE_TRUE /* Yes also validate. */);
    }

    return return_status;
}
/* end of fbe_parity_record_errors() */


/*!**************************************************************
 * fbe_parity_report_corrupt_operations()
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
static fbe_status_t fbe_parity_report_corrupt_operations(fbe_raid_siots_t * siots_p)
{
    fbe_u32_t region_index;
    fbe_u32_t error_code;
    fbe_u32_t vr_error_bits;
    fbe_u32_t array_pos;
    fbe_u16_t array_pos_mask;
    fbe_u32_t sunburst;    
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

                if (!fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)))
                {
                      sunburst = fbe_raid_siots_is_parity_pos(siots_p, array_pos) 
                                      ? SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED
                                      : SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED;
                }
                else
                {
                    if (fbe_raid_siots_is_parity_pos(siots_p, array_pos) != FBE_FALSE) 
                    {
                        sunburst = (fbe_raid_siots_pos_is_dead(siots_p, array_pos) != FBE_TRUE) 
                                          ? SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED 
                                          : SEP_ERROR_CODE_RAID_HOST_PARITY_INVALIDATED;
                    }
                    else
                    {
                        sunburst = SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED;
                    }
                }

                /* We will log message even if drive is dead as we want to
                 * notify that correspondign sector of drive is lost.
                 */
                fbe_raid_send_unsol(siots_p,
                                    sunburst,
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
 * end fbe_parity_report_corrupt_operations()
 **********************************************/

/*!***************************************************************
 * fbe_parity_report_errors()
 ****************************************************************
 * @brief
 *  This function will record the errors encountered in verify
 *  to the event log.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 * @param b_update_eboard - a flag indicating whether vp eboard has to
 *                          be updated or not with siot's vcts' overall 
 *                          eboard. We will do update only if flag
 *                          is FBE_TRUE.
 *
 * @return fbe_status_t
 *
 * @author
 *  11/12/99 - Created. JJ
 *  06/23/00 - Fixed Header, added LBA Logging Rob Foley
 *  11/20/08 - Modified error reporting in event logs. PP
 *
 ****************************************************************/
fbe_status_t fbe_parity_report_errors(fbe_raid_siots_t * siots_p, fbe_bool_t b_update_eboard)
{
    fbe_status_t status = FBE_STATUS_OK;

    /* We will do do error reporting using EBOARD only if we CAN NOT 
     * report using XOR error region.
     */
    if (siots_p->vcts_ptr != NULL)
    {
        const fbe_xor_error_regions_t * const regions_p = &siots_p->vcts_ptr->error_regions;

        /* Activate error reporting only if xor error region is not empty.
         */
        if (!FBE_XOR_ERROR_REGION_IS_EMPTY(regions_p))
        {
            if (fbe_raid_siots_is_corrupt_operation(siots_p))
            {
               /* We do report errors differently if siot's operation is  
                * corrupt CRC/Data. 
                */
                status = fbe_parity_report_corrupt_operations(siots_p);
            }
            else
            {
                status = fbe_parity_report_errors_from_error_region(siots_p);
            }

            if (status != FBE_STATUS_OK) { return status; }
        }
        
        
        /* We may have some errors which we went away after retry.
         * Also, we do not create any XOR error region for these
         * errors. However we do log those errors.
         */
        status = fbe_parity_report_retried_errors(siots_p);
        if (status != FBE_STATUS_OK) { return status; }
    }
    else
    {
        /* Report using eboard.
         */
        status = fbe_parity_report_errors_from_eboard(siots_p);
    }

    /* Update the IOTS' error counts with the totals from this siots.
     * We will do so only if we are asked to do so.
     */
    if (b_update_eboard)
    {
        fbe_raid_siots_update_iots_vp_eboard( siots_p );
    }

    return FBE_STATUS_OK;
}
/************************************
 *end fbe_parity_report_errors() 
 ************************************/

/*!***************************************************************
 * fbe_parity_only_invalidated_in_error_regions()
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
fbe_bool_t fbe_parity_only_invalidated_in_error_regions(fbe_raid_siots_t * siots_p)
{
    fbe_u32_t region_idx;
    const fbe_xor_error_regions_t * regions_p = &siots_p->vcts_ptr->error_regions;

    for (region_idx = 0; region_idx < regions_p->next_region_index; ++region_idx)
    {
        if (regions_p->regions_array[region_idx].error != 0)
        {
            if(fbe_parity_can_log_uncorrectable_from_error_region(siots_p, region_idx))
            {
                return FBE_FALSE;
            }
        }
    }

    return FBE_TRUE;
                
}
/*******************************************
 *end fbe_parity_only_invalidated_in_error_regions() 
 ******************************************/

/*!***************************************************************
 * fbe_parity_report_retried_errors()
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
static fbe_status_t fbe_parity_report_retried_errors(fbe_raid_siots_t * siots_p)
{
    fbe_u32_t array_pos = 0;
    fbe_u16_t array_pos_mask = 0;
    fbe_u32_t sunburst = 0;
    fbe_u32_t width = fbe_raid_siots_get_width(siots_p);

    for (array_pos = 0; array_pos < width; array_pos++)
    {
        array_pos_mask = 1 << array_pos;

        if ((siots_p->vrts_ptr->retried_crc_bitmask & array_pos_mask ) != 0 )
        {
            /* If we are here then it means that we have to log
             * retry message for the current position.
             */
            sunburst = fbe_raid_siots_is_parity_pos(siots_p, array_pos)
                            ? SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED
                            : SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED;

            fbe_raid_send_unsol(siots_p,
                                sunburst,
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
 *end fbe_parity_report_retried_errors() 
 ******************************************/


/*!***************************************************************
 * fbe_parity_report_media_errors()
 ****************************************************************
 * @brief
 *  This function will report media errors in event log
 *
 * @param siots_p - pointer to the fbe_raid_siots_t
 * @param fruts_p - pointer to fru ts chain.
 * @param correctable - FBE_TRUE means these should be reported as
 *                      correctable errors, FBE_FALSE to report
 *                      as uncorrectable errors.
 *
 * @return fbe_status_t
 *
 * @author
 *  01/17/11 - Created - Jyoti Ranjan
 *
 ****************************************************************/
fbe_status_t fbe_parity_report_media_errors(fbe_raid_siots_t * siots_p,
                                            fbe_raid_fruts_t * const fruts_p,
                                            const fbe_bool_t correctable)
{
    fbe_status_t status = FBE_STATUS_OK;

     /* Determine if hard errors were taken.
     */
    (void) fbe_raid_fruts_get_media_err_bitmap(fruts_p, &siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap);

    if ( siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap != 0 )
    {
        /* Put the media errors into the VRTS since this is what report
         * errors uses.  Use the appropriate bitmap depending on whether or
         * not we were asked to log as correctable or not.
         */
        if ( correctable )
        {
            siots_p->vrts_ptr->eboard.c_crc_bitmap |= siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap;
        }
        else
        {
            siots_p->vrts_ptr->eboard.u_crc_bitmap |= siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap;
        }

        /* Also setup the media err bitmap.
         */
        siots_p->vrts_ptr->eboard.media_err_bitmap |= siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap;
    }

    /* Report these errors to the event log.
     */
    status = fbe_parity_report_errors( siots_p, FBE_TRUE );

    return status;
}
/*******************************************
 * fbe_parity_report_media_errors()
 ******************************************/

/*!***********************************************************
 * fbe_parity_report_errors_from_error_region()
 *************************************************************
 * @brief
 *  This function is used to report event log messages using
 *  XOR region.

 * @param   siots_p   - pointer to siots
 *
 * @return  fbe_status_t
 *
 * @author
 *  12/08/08 - Created -. Pradnya Patankar
 *  01/06/11 - Modified - Jyoti Ranjan
 *
 ************************************************************/
fbe_status_t fbe_parity_report_errors_from_error_region(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t array_pos;
    fbe_u16_t array_pos_mask;
    fbe_bool_t b_uc_err_logged = FBE_FALSE;
    fbe_bool_t b_check_parity_invalidated = FBE_FALSE;
    fbe_u32_t c_err_bits; 
    fbe_u32_t region_idx = 0;
    const fbe_xor_error_regions_t * regions_p = &siots_p->vcts_ptr->error_regions;
    
    /* Report correctable and/or uncorrectable messages by going over
     * each disk position and see whether there is any XOR error region
     * entry applicable to it or not. If so, then report event log
     * messages (if other conditions are satisfied).
     */
    for (array_pos = 0; array_pos < fbe_raid_siots_get_width(siots_p); array_pos++)
    {
        /* array_pos_mask has the current position in the error bitmask 
         * that we need to check below.
         */
        array_pos_mask = 1 << array_pos;
        
        /* Here we log the errors reported in the fbe_xor_error_region_t
         * only hence we will traverse the fbe_xor_error_region_t for given 
         * disk position.
         */
        for (region_idx = 0; region_idx < regions_p->next_region_index; ++region_idx)
        {
            if (regions_p->regions_array[region_idx].error != 0)
            {
                /* Note that checking FBE_XOR_ERR_TYPE_UNCORRECTABLE for this region and
                 * the code in fbe_parity_log_uncorrectable_error should prevent us from
                 * logging such uncorrectables for regions that do not merit them.
                 */
                if ((regions_p->regions_array[region_idx].pos_bitmask & array_pos_mask) ||
                    (fbe_raid_siots_pos_is_dead(siots_p, array_pos) && 
                           ((siots_p->misc_ts.cxts.eboard.w_bitmap & array_pos_mask) && 
                            !(siots_p->misc_ts.cxts.eboard.hard_media_err_bitmap & array_pos_mask))))
                {
                    /*
                     * This is not one of the disks that we made the error region for, 
                     * but another disk (call it disk A) that might be affected by it. 
                     * In this case, if multiple error regions contain errors at the 
                     * same pba but different drives (none of which are drive A), we 
                     * don't want to look at disk A twice.  We also don't want to look
                     * at disk A here if it is flagged with an uncorrectable at this
                     * pba in some other error region.
                     */
                    if (!(regions_p->regions_array[region_idx].pos_bitmask & array_pos_mask) )
                    {
                        int another_index;
                        fbe_bool_t skip = FBE_FALSE;

                        /* The code commented out below may be useful in the future.  
                         * It was not needed the last time this function was reworked. --HEW
                         */
/*
                        for (another_index = 0; another_index < region_idx; ++another_index)
                        {
                            if( (regions_p->regions_array[region_idx].lba == 
                                 regions_p->regions_array[another_index].lba)  &&
                                (local_u_bitmask[another_index] & array_pos_mask) )
                            {
                                /* We've already logged a "secondary" uncorrectable for this disk
                                 * at this PBA. We don't need to consider it here.
                                 */
                              /*  skip = FBE_TRUE;
                            }
                        }
*/
                       /*
                        * Check if this disk and lba have a "real" uncorrectable in another region
                        * If so, don't bother with this "secondary error" here.
                        */
                       for (another_index = 0; another_index < regions_p->next_region_index; ++another_index)
                       {
                           if( (regions_p->regions_array[another_index].pos_bitmask & array_pos_mask) &&
                               (regions_p->regions_array[another_index].error & FBE_XOR_ERR_TYPE_UNCORRECTABLE) )
                           {
                               skip = FBE_TRUE;
                           }
                       }
                    
                       if( skip ) 
                       {
                           /* Skip to next region
                            */
                           continue;
                       }
                    } /* end if (!(regions_p->regions_array[region_idx].pos_bitmask & array_pos_mask)) */

                    if (regions_p->regions_array[region_idx].error & FBE_XOR_ERR_TYPE_UNCORRECTABLE)
                    {
                        /* Current region is uncorrectable in nature.
                         */
                        status = fbe_parity_log_uncorrectable_errors_from_error_region(siots_p,
                                                                                region_idx,
                                                                                array_pos,
                                                                                &b_uc_err_logged);
                        if (RAID_COND(status != FBE_STATUS_OK))
                        {
                            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                                 "parity: failed to log uncorrectable error: status = 0x%x, siots_p = 0x%p\n", 
                                                 status, siots_p);
                            return FBE_STATUS_GENERIC_FAILURE;
                        }

                        if (b_uc_err_logged != FBE_FALSE)
                        {
                            b_check_parity_invalidated = FBE_TRUE;
                        }
                    } /* End if (regions_p->regions_array[region_idx].error & FBE_XOR_ERR_TYPE_UNCORRECTABLE) */ 
                    else
                    {
                        /* Current region is of correctable in nature. So, report onlt correctable 
                         * event log messages only.
                         */
                        status = fbe_raid_get_error_bits_for_given_region(&regions_p->regions_array[region_idx], 
                                                                          FBE_FALSE, 
                                                                          array_pos_mask,
                                                                          FBE_TRUE,
                                                                          &c_err_bits);
                        if (RAID_COND(status != FBE_STATUS_OK))
                        {
                            /* We are here as we tried to extract error-bits 
                             * for unexpected xor error code.
                             */
                            return status;
                        }

                        if (c_err_bits != 0)
                        {
                            status = fbe_parity_log_correctable_errors_from_error_region(siots_p,
                                                                                  region_idx,
                                                                                  array_pos,
                                                                                  c_err_bits);
                            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
                            {
                                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                                     "parity: failed to log correctable error: status = 0x%x, "
                                                     "siots_p = 0x%p, array_pos = 0x%x, region_idx = 0x%x\n", 
                                                     status, siots_p, array_pos, region_idx);
                                return status;
                            }
                        } /* end if (c_err_bits != 0) */ 
                    } /* else-if if (regions_p->regions_array[region_idx].error & FBE_XOR_ERR_TYPE_UNCORRECTABLE) */
                } /* end if ((regions_p->regions_array[region_idx].pos_bit ... */
            } /* end if (region_p->error!=0) ...*/
        }/* end for(region_p) ... */
    }/* end for (array_pos) ... */

    /* Check for parity invalidation.
     */
    if (b_check_parity_invalidated)
    { 
        /* We are here means: there are uncorrectable errors and these
         * have potential to cause parity invalidation
         */
       fbe_parity_log_parity_drive_invalidation(siots_p);
    }

    return status;

} 
/***************************************************
 * end fbe_parity_report_errors_from_error_region() 
 ***************************************************/



/*!************************************************************************
 * fbe_parity_can_log_u_errors_as_per_eboard()
 *************************************************************************
 * @brief
 *  This function will determine whether we should log an uncorrectable
 *  message as per information available in eboard or not.
 *
 * @param   siots_p - pointer to the fbe_raid_siots_t
 *
 * @return  return FBE_TRUE if uncorrectable errors has to be logged 
            else FBE_FALSE.
 *
 * @author
 *  01/25/11 - Re-wrote - Jyoti Ranjan
 *
 *************************************************************************/
static fbe_bool_t fbe_parity_can_log_u_errors_as_per_eboard(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t b_uc_stripe_exists = FBE_FALSE;
    fbe_u16_t array_pos;        
    fbe_block_count_t parity_range_offset;
    fbe_lba_t parity_start_pba;    
    fbe_lba_t address_offset;    
    fbe_u32_t err_bits; 

    status = fbe_raid_siots_get_raid_group_offset(siots_p, &address_offset);
    if (status != FBE_STATUS_OK) { return FBE_FALSE; }
    parity_range_offset = siots_p->geo.start_offset_rel_parity_stripe;
    parity_start_pba = address_offset + parity_range_offset;

    /* First check to see if we have any probability of logging 
     * uncorrectables errors.
     */
    for (array_pos = 0; array_pos < fbe_raid_siots_get_width(siots_p); array_pos++)
    {
        /* If the position is not dead, just see if we should log the error
         * by relying on fbe_parity_can_log_u_error_for_given_pos().
         *
         * We log uncorrectable onliy if one of the following condition is satisified:
         *      1. If the position is dead then error bits must be NON-ZERO
         *      2. If other NON-DEAD positions has an error to be logged
         *         as per fbe_parity_can_log_u_error_for_given_pos().
         *
         * This helps handle the case where we are filtering out the errors 
         * on other positions and do not need to log the uncorrectable
         * errors on dead positions.
         */
        fbe_bool_t b_log_u_error = fbe_parity_can_log_u_error_for_given_pos(siots_p, array_pos, parity_start_pba, &err_bits);

        if ((!fbe_raid_siots_pos_is_dead(siots_p, array_pos) && b_log_u_error) ||
            (fbe_raid_siots_pos_is_dead(siots_p, array_pos) && err_bits != 0))
        {
            b_uc_stripe_exists = FBE_TRUE;
            break;
        }
    }

    return b_uc_stripe_exists;
}
/*********************************************
 * end fbe_parity_can_log_u_errors_as_per_eboard()
 ********************************************/


/*!************************************************************************
 * fbe_parity_report_errors_from_eboard()
 *************************************************************************
 * @brief
 *  This function will record the errors encountered in verify as per 
 *  information available in eboard.
 *
 * @param   siots_p - pointer to the fbe_raid_siots_t
 *
 * @return   fbe_status_t
 *
 * @author
 *  12/08/08 - Created - Pradnya Patankar
 *  01/27/11 - Modifed - Jyoti Ranjan
 *
 *************************************************************************/
fbe_status_t fbe_parity_report_errors_from_eboard(fbe_raid_siots_t * siots_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t parity_start_pba;
    fbe_lba_t address_offset;
    fbe_bool_t data_invalid, parity_reconstruct;
    fbe_lba_t data_invalid_pba; 
    fbe_u32_t array_pos;
    fbe_bool_t b_log_uc_stripe = FBE_FALSE;


    /* When we are logging the error from eboard, we will only have the 
     * fru position and there is not information available about the number of 
     * blocks, hence it is initialized to one.
     */
    fbe_u32_t blocks = FBE_RAID_MIN_ERROR_BLOCKS;

    fbe_bool_t b_row_reconstructed = FBE_FALSE;
    fbe_bool_t b_diag_reconstructed = FBE_FALSE;
    fbe_u32_t err_bits; 

    data_invalid = parity_reconstruct = FBE_FALSE;

    /* Set this to an invalid LBA. Below we will set this to the minimum of
     * data_invalid_pba and data_pba, which is the current error pba.
     */
    data_invalid_pba = MAX_SUPPORTED_CAPACITY;


    
    status = fbe_raid_siots_get_raid_group_offset(siots_p, &address_offset);
    if (status != FBE_STATUS_OK) { return status; }
    parity_start_pba = siots_p->geo.start_offset_rel_parity_stripe + address_offset;
    b_log_uc_stripe = fbe_parity_can_log_u_errors_as_per_eboard(siots_p);

    for (array_pos = 0; array_pos < fbe_raid_siots_get_width(siots_p); array_pos++)
    {
        /* array_pos_mask has the current position in the
         * error bitmask that we need to check below.
         */
        fbe_u16_t array_pos_mask = 1 << array_pos;


        /* FBE_TRUE to log uncorrectables for this position.  FBE_FALSE otherwise.
         */
        fbe_bool_t b_log_uc_pos = FBE_FALSE;
        
        /* Determine if there are correctable errors, since we need to log them.
         */
        err_bits = fbe_raid_get_correctable_status_bits(siots_p->vrts_ptr,
                                                  array_pos_mask);
        
        if (err_bits != 0)
        {
            status = fbe_parity_log_correct_errors_from_eboard(siots_p,
                                                 array_pos,
                                                 parity_start_pba,
                                                 err_bits,
                                                 &b_row_reconstructed,
                                                 &b_diag_reconstructed);
        }

        if (b_log_uc_stripe)
        {
            /* If we already passed the check above, then 
             * determine if we have uncorrectables.
             */
            b_log_uc_pos = fbe_parity_can_log_u_error_for_given_pos(siots_p, array_pos, parity_start_pba, &err_bits);
        }
        
        status = fbe_parity_log_uncorrectable_errors_from_eboard(siots_p,
                                                                 b_log_uc_pos,
                                                                 b_log_uc_stripe,
                                                                 array_pos,
                                                                 parity_start_pba,
                                                                 err_bits,
                                                                 &data_invalid,
                                                                 &parity_reconstruct,
                                                                 &data_invalid_pba);

    } /* end for all array positions. */

    /* In the case of invalidation we no longer invalidate parity.  Therefore
     * report the parity sector as reconstructed.
     */
    if (!fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)) &&
        (parity_reconstruct == FBE_FALSE) && (data_invalid == FBE_TRUE))
    {
        fbe_raid_send_unsol(siots_p,
                            SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED,
                            siots_p->parity_pos,
                            data_invalid_pba,
                            blocks,
                            fbe_raid_error_info(err_bits),
                            fbe_raid_extra_info(siots_p));
    }
    else if (fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)) &&
             data_invalid == FBE_TRUE)
    {
        /* On R6, If either parity drive is alive,
         * then log a sector reconstructed message if we found an
         * uncorrectable parity sector.
         * We do this since R6 does not invalidate parity.
         */
        if (b_row_reconstructed == FBE_FALSE &&
            !fbe_raid_siots_pos_is_dead(siots_p, fbe_raid_siots_get_row_parity_pos(siots_p)))
        {
            fbe_raid_send_unsol(siots_p,
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED,
                                fbe_raid_siots_get_row_parity_pos(siots_p),
                                data_invalid_pba,
                                blocks,
                                fbe_raid_error_info(err_bits),
                                fbe_raid_extra_info(siots_p));

        }
        if (b_diag_reconstructed == FBE_FALSE &&
            !fbe_raid_siots_pos_is_dead(siots_p, fbe_raid_siots_dp_pos(siots_p)))
        {
            fbe_raid_send_unsol(siots_p,
                                SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED,
                                fbe_raid_siots_dp_pos(siots_p),
                                data_invalid_pba,
                                blocks,
                                fbe_raid_error_info(err_bits),
                                fbe_raid_extra_info(siots_p));
        }
    }
    return status;
}
/*********************************************
 * end fbe_parity_report_errors_from_eboard() 
 ********************************************/


/*!************************************************************************
 * fbe_parity_log_correct_errors_from_eboard()
 *************************************************************************
 * @brief
 *  This function will record the correctable errors encountered as per 
 *  information available in eboard.
 *
 * @param   siots_p - ptr to the fbe_raid_siots_t
 * @param   array_pos - array position
 * @param   parity_start_pba - Start PBA for this parity stripe
 * @param   err_bits - correctable error bits.
 * @param   b_row_reconstructed - flag that tells whether we want to reconstruct the 
 *                        row parity or not.
 * @param   b_diag_reconstructed - flag that tells whether we want to reconstruct the 
 *                        diagonal parity or not.
 *
 * @return   fbe_status_t
 *
 * @author
 *  11/12/99 - Created. JJ
 *  12/08/08 - Modified for error reporting in event logs. Pradnya Patankar
 *
 * @notes
 *  If there is not VCTS allocated , we will use the eboard to report error same 
 *  as the old error reporting mechanism.
 *
 ********************************************************************************/
fbe_status_t fbe_parity_log_correct_errors_from_eboard(fbe_raid_siots_t * siots_p,
                                                       fbe_u32_t array_pos,
                                                       fbe_lba_t parity_start_pba,
                                                       fbe_u32_t err_bits,
                                                       fbe_bool_t* b_row_reconstructed,
                                                       fbe_bool_t* b_diag_reconstructed)
{
    fbe_lba_t data_pba;    /* Pba to report for this position. */

       
    fbe_payload_block_operation_opcode_t opcode;
    fbe_status_t status = FBE_STATUS_OK;
    /* Correctable errors.
     */
    fbe_u32_t sunburst = 0;
    /* When we are logging the error from eboard, we will only have the 
     * fru position and there is not information available about the number of 
     * blocks, hence it is initialized to one.
     */
    fbe_u32_t blocks = FBE_RAID_MIN_ERROR_BLOCKS;

    /* Get the opcode from SIOTS
     */
    fbe_raid_siots_get_opcode(siots_p, &opcode);

    data_pba = fbe_raid_get_region_lba_for_position(siots_p, array_pos, parity_start_pba);

    /* Correctable errors are reported from highest to lowest
     * priority. Thus LBA stamp errors are the highest.
     */
    if ((err_bits & VR_REASON_MASK) == VR_LBA_STAMP)
    {
        /* We want to cause a phone-home for LBA stamp
         * errors since it could indicate that the drive
         * is doing something bad.
         */
        sunburst = SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR;
    }
    else if ((err_bits & (VR_COH | VR_POC | VR_N_POC)) &&
            !fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)))
    {
        if (fbe_raid_siots_is_marked_for_incomplete_write_verify(siots_p) ||
            fbe_raid_siots_is_metadata_operation(siots_p))
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
            /* This coherency error is not correctable because we were not 
             * able to identify the precise block in error.
             * We will log the coherency error event log, which causes a call home.
             * Raid 6 is allowed to have correctable coherency errors, and will
             * log correctable coherency errors below as sector reconstructed.
             */
            sunburst = SEP_ERROR_CODE_RAID_HOST_COHERENCY_ERROR;
        }
    }
    else if ( err_bits & VR_UNEXPECTED_CRC )
    {
        if ((siots_p->vrts_ptr->eboard.crc_multi_and_lba_bitmap & (1 << array_pos)) && 
            fbe_raid_library_get_encrypted()) {
            /* Mark it as expected since if we see these the drive is not likely bad, 
             * it is more likely a software error, from which the only recovery is to reconstruct. 
             */
            sunburst = fbe_raid_siots_is_parity_pos(siots_p, array_pos)
                       ? SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED
                       : SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED;

            /* Do not report error trace if we may have read this area with a different key.
             */
            if (opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA){ 
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                            "unexpected multi-bit crc w/lba st error pos: %d lba: 0x%llx bl: 0x%x\n",
                                            array_pos, data_pba, blocks);
            }
        } else {
            /* Any time we see a correctable unexpected CRC error,
             * we want to log a message of higher severity to cause a call home.
             */
            sunburst = (array_pos == siots_p->parity_pos)
                       ? SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR
                       : SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR;
        }
    }
    else
    {
        /* All other flavors of corrections are logged to the event log.
         */
        sunburst = fbe_raid_siots_is_parity_pos(siots_p, array_pos)
                    ? SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED
                    : SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED;
    }
    
    /* For Raid 6 keep track of parity being reconstructed so that if 
     * data sectors are invalidated, we don't log the same message below.
     */
    if ( fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)) )
    {
        if (fbe_raid_siots_get_row_parity_pos(siots_p) == array_pos)
        {
            *b_row_reconstructed = FBE_TRUE;
        }
        else if (fbe_raid_siots_dp_pos(siots_p) == array_pos)
        {
            *b_diag_reconstructed = FBE_TRUE;
        }
    }

    fbe_raid_send_unsol(siots_p,
                        sunburst,
                        array_pos,
                        data_pba,
                        blocks,
                        fbe_raid_error_info(err_bits),
                        fbe_raid_extra_info(siots_p));

    return status;

}/* end of fbe_parity_log_correct_errors_from_eboard() */

/*!************************************************************************
 * fbe_parity_log_uncorrectable_errors_from_eboard()
 *************************************************************************
 * @brief
 *  This function will record the uncorrectable errors encountered in verify
 *  and that are present in eboard but not present in the fbe_xor_error_region_t 
 *  (may be due to degraded lun) to the Flare ulog.
 *

 * @param   siots_p - ptr to the fbe_raid_siots_t
 * @param   b_log_uc_pos - flag that tells we want to log uncorrectable for this 
 *                     position or not.
 * @param   b_log_uc_stripe - flag that tells we want to log uncorrectable for this 
 *                     parity stripe or not.
 * @param   array_pos - array position
 * @param   parity_start_pba - Start PBA for this parity stripe
 * @param   err_bits - correctable error bits.
 * @param   data_invalid - flag that tells whether we want to invalidate the 
 *                     data or not.
 * @param   parity_reconstruct - flag that tells whether we want to reconstruct parity or not
 * @param   data_invalid_pba - PBA to log.for invalidation.
 *
 * @return   fbe_status_t
 *
 * @author
 *  11/12/99 - Created. JJ
 *  12/08/08 - Modified for error reporting in event logs. PP
 *
 * @notes
 *  If there is not VCTS allocated , we will use the eboard to report error same 
 *  as the old error reporting mechanism.
 *
 *************************************************************************/
fbe_status_t fbe_parity_log_uncorrectable_errors_from_eboard(fbe_raid_siots_t * siots_p,
                                                             fbe_bool_t b_log_uc_pos,
                                                             fbe_bool_t b_log_uc_stripe,
                                                             fbe_u32_t array_pos,
                                                             fbe_lba_t parity_start_pba,
                                                             fbe_u32_t err_bits,
                                                             fbe_bool_t* data_invalid,
                                                             fbe_bool_t* parity_reconstruct,
                                                             fbe_lba_t* data_invalid_pba)
{
    fbe_lba_t data_pba;    
    fbe_u16_t error_type;
    fbe_payload_block_operation_opcode_t opcode;
    
    /* When we are logging the error from eboard, we will only have the 
     * fru position and there is not information available about the number of 
     * blocks, hence it is initialized to one.
     */
    fbe_u32_t blocks = FBE_RAID_MIN_ERROR_BLOCKS;
    fbe_bool_t  b_is_parity_pos;

    /* Determine if this is a parity position or not.
     */
    b_is_parity_pos = fbe_raid_siots_is_parity_pos(siots_p, array_pos);

    /* Get the opcode from SIOTS
     */
    fbe_raid_siots_get_opcode(siots_p, &opcode);

    /* If we have uncorrectable coherency errors on parity, then 
     * we will log them with the coherency event log message.
     * This is the only case on Raid 6 where we log the
     * coherency event log message.
     */
    if (  b_log_uc_pos &&
          b_is_parity_pos                           &&
            (err_bits & (VR_COH | VR_POC | VR_N_POC )) )
    {
        fbe_u32_t error_code;
        /* Log this uncorrectable coherency error on parity as a 
         * coherency error instead of as an uncorrectable,
         * since we never invalidate parity on Raid 6.
         * This error was reported as an uncorrectable coherency error, since
         * it is the one case on Raid 6 where we do not know
         * where the parity is and thus will rebuild parity.
         */
        data_pba = fbe_raid_get_region_lba_for_position( siots_p, array_pos, parity_start_pba );
        if(fbe_raid_siots_is_marked_for_incomplete_write_verify(siots_p) ||
           fbe_raid_siots_is_metadata_operation(siots_p))
        {
            /* Note the fact that the coherency error was found when an incomplete write verify was
             * outstanding 
             */
            error_code = SEP_INFO_CODE_RAID_EXPECTED_COHERENCY_ERROR;
        }
        else
        {
            error_code = SEP_ERROR_CODE_RAID_HOST_COHERENCY_ERROR;
        }
        fbe_raid_send_unsol(siots_p,
                            error_code,
                            array_pos,
                            data_pba,
                            blocks,
                            fbe_raid_error_info(err_bits),
                            fbe_raid_extra_info(siots_p));

    }
    /* Determine if there are uncorrectable errors, since we need to log them.
     */
    else if (b_log_uc_stripe && b_log_uc_pos && fbe_raid_siots_is_metadata_operation(siots_p))
    {
        data_pba = fbe_raid_get_region_lba_for_position( siots_p, array_pos, parity_start_pba );
        fbe_raid_send_unsol(siots_p,
                            SEP_INFO_CODE_RAID_EXPECTED_COHERENCY_ERROR,
                            array_pos,
                            data_pba,
                            blocks,
                            fbe_raid_error_info(err_bits),
                            fbe_raid_extra_info(siots_p));
    }
    else if (b_log_uc_stripe && b_log_uc_pos)
    {
        fbe_u32_t sunburst = 0;
        data_pba = fbe_raid_get_region_lba_for_position( siots_p, array_pos, parity_start_pba );

        /* The operation is either the first read or not a read at all.  
         * Log the uncorrectable error.
         */
        if (b_is_parity_pos)
        {
            *parity_reconstruct = FBE_TRUE;
            sunburst = SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_PARITY_SECTOR;
        }
        else if ((err_bits & VR_INVALID_CRC) &&
                 fbe_raid_geometry_is_vault(fbe_raid_siots_get_raid_geometry(siots_p)))
        {
            /* Simply skip logging the error. We do not log these errors on the vault,
             * since the vault writes invalid blocks normally.
             */
            return FBE_STATUS_OK;
        }
        else
        {
            *data_invalid = FBE_TRUE;
            sunburst = SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR;
        }
        
        fbe_raid_send_unsol(siots_p,
                            sunburst,
                            array_pos,
                            data_pba,
                            blocks,
                            fbe_raid_error_info(err_bits),
                            fbe_raid_extra_info(siots_p));

        /* Log either the parity sector reconstruction or the data sector
         * invalidation.
         */
        sunburst = (b_is_parity_pos == FBE_TRUE)
                    ? SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED
                    : SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED;
        if (RAID_COND(b_is_parity_pos == FBE_TRUE) && (*parity_reconstruct != FBE_TRUE))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, "*parity_reconstruct %d != FBE_TRUE",
                                 *parity_reconstruct);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_raid_send_unsol(siots_p,
                            sunburst,
                            array_pos,
                            data_pba,
                            blocks,
                            fbe_raid_error_info(err_bits),
                            fbe_raid_extra_info(siots_p));

        /*! @todo Currently, there is 
         * function which is equivalent to   .
         * !RG_LUN_IN_BV(RG_GET_UNIT(siots_p). 
         * Need to add condition for that    .
         */
        error_type = ((err_bits & VR_REASON_MASK) == VR_MULTI_BIT_CRC) ?
                        FBE_XOR_ERR_TYPE_MULTI_BIT_CRC:
                        FBE_XOR_ERR_TYPE_SINGLE_BIT_CRC;

        
        

        /* Un-correctable errors.
         * First log uncorrectable errors, then log invalidations.
         */
        *data_invalid_pba = FBE_MIN(*data_invalid_pba, data_pba );

    } /* end if fbe_parity_log_u_error() */
    return FBE_STATUS_OK;
}
/********************************************************
 * end fbe_parity_log_uncorrectable_errors_from_eboard() 
 ********************************************************/

/*!*******************************************************************
 * fbe_parity_log_correctable_errors_from_error_region()
 ********************************************************************
 * @brief
 *  This function will record the correctable error encountered in 
 *  verify to the Flare ulog.
 *

 * @param   siots_p - ptr to the fbe_raid_siots_t
 * @param   region_idx - index to access the loaction in the 
 *                  fbe_xor_error_region_t.
 * @param   array_pos - array position for which we want to log the error
 * @param   err_bits - bits showing disk position on which correctable 
 *                 error is present
 *
 * @return   fbe_status_t
 *
 * @author
 *  11/20/08 - Created for new event logging. Refactored the code PP
 *  12/05/08 - Modified for Background Verify Avoidance.   MA
 ********************************************************************/
fbe_status_t fbe_parity_log_correctable_errors_from_error_region(fbe_raid_siots_t * siots_p,
                                                          fbe_u32_t region_idx,
                                                          fbe_u16_t array_pos,
                                                          fbe_u32_t err_bits)
{
    fbe_u32_t error_type;
    fbe_u16_t array_pos_mask;
    fbe_u32_t blocks;
    fbe_u32_t sunburst = 0;
    fbe_payload_block_operation_opcode_t opcode;
    
    fbe_status_t status = FBE_STATUS_OK;

    
    const fbe_xor_error_regions_t * const regions_p = &siots_p->vcts_ptr->error_regions;

    array_pos_mask = 1 << array_pos;
    blocks = regions_p->regions_array[region_idx].blocks;

    /* Get the opcode from SIOTS
     */
    fbe_raid_siots_get_opcode(siots_p, &opcode);

    if (!fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)))
    {
        error_type = regions_p->regions_array[region_idx].error;
    }
    else
    {
        if (regions_p->regions_array[region_idx].error & FBE_XOR_ERR_TYPE_ZEROED)
        {
            /* If the error has occured on the freshly bound LUN or zeroed LUN,
             * then we will get the error along with FBE_XOR_ERR_TYPE_ZEROED.
             * to indicate that we have not done any IO on lun.
             * that is, if we get the ckecksum error then the error 
             * will be FBE_XOR_ERR_TYPE_ZEROED = 0x4000,
             * FBE_XOR_ERR_TYPE_TS = 0x0A . The error will be 0x400A.
             * So to get the exact error type we and it with FBE_XOR_ERR_TYPE_ZEROED.
             */
            error_type = regions_p->regions_array[region_idx].error ^ FBE_XOR_ERR_TYPE_ZEROED;
        }
        else
        {
            error_type = regions_p->regions_array[region_idx].error;
        }
    }
        
    /* Correctable errors are reported from highest to lowest priority.
     * Thus LBA stamp errors are the highest.
     */
    if (error_type == FBE_XOR_ERR_TYPE_LBA_STAMP) 
    {
        /* We want to cause a phone-home for LBA stamp errors 
         * since it could indicate that the drive is doing something bad.
         */
        sunburst = SEP_ERROR_CODE_RAID_HOST_LBA_STAMP_ERROR;
    }
    else if (((error_type == FBE_XOR_ERR_TYPE_COH)||
              (error_type == FBE_XOR_ERR_TYPE_1POC)||
              (error_type == FBE_XOR_ERR_TYPE_N_POC_COH))&&
              !fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)))
    {
        if (fbe_raid_siots_is_marked_for_incomplete_write_verify(siots_p) ||
            fbe_raid_siots_is_metadata_operation(siots_p))
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
            /* This coherency error is not correctable because we were not able 
             * to identify the precise block in error.We will log the coherency 
             * error event log, which causes a call home.
             * Raid 6 is allowed to have correctable coherency errors, and will 
             * log correctable coherency errors below as sector reconstructed.
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
            sunburst = fbe_raid_siots_is_parity_pos(siots_p, array_pos)
                       ? SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED
                       : SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED;

            /* Do not report error trace if we may have read this area with a different key.
             */
            if (opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA){
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                            "unexpected multi-bit crc w/lba st error pos: %d lba: 0x%llx bl: 0x%x\n",
                                            array_pos,
                                            regions_p->regions_array[region_idx].lba,
                                            regions_p->regions_array[region_idx].blocks);
            }
        } else {
            /* Any time we see a correctable unexpected CRC error,
             * we want to log a message of higher severity to cause a call home.
             */
            sunburst = fbe_raid_siots_is_parity_pos(siots_p,array_pos)
                       ? SEP_ERROR_CODE_RAID_HOST_PARITY_CHECKSUM_ERROR
                       : SEP_ERROR_CODE_RAID_HOST_DATA_CHECKSUM_ERROR;
        }

        /* Adjust the reason code in the error bits, based on the error informantion
         * from the error_region data.  We need to do this because the error bits are 
         * derived from the eboard which is shared by all error regions in the SIOTS.
         */ 
        fbe_raid_region_fix_crc_reason(error_type, &err_bits);

        /* If an unexpected CRC error is found, notify the DH so it can
         * properly handle the potential for a bad drive.
         * If this is a BVA I/O or the LUN is scheduled for Background Verify,
         * do not inform DH, as the CRC error might be due to an interrupted write,
         * rather than a drive problem.
         * Do not notify DH if the drive is already marked dead. This could
         * unncessarily kill a rebuilding drive.  DIMS 232965.
         */
        /*! @todo Currently, there is 
         * function which is equivalent to  .
         * RG_LUN_IN_BV(RG_GET_UNIT(siots_p). 
         * Need to add condition for that   .
         */
        
    }
    else
    {
         /* All other flavors of corrections are logged to the event log.
          */
         sunburst = fbe_raid_siots_is_parity_pos(siots_p,array_pos)
                        ? SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED
                        : SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED;
    }

     /* If we are doing BVA write and we are able to correct the error
      * during this write with new data then will log the message for
      * sector reconrtuction.
      */
     if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE)
     {
         status = fbe_parity_bva_log_error_in_write(siots_p, 
                                                    region_idx,
                                                    array_pos,
                                                    sunburst,
                                                    err_bits);
         if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
         {
            fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, 
				     "parity_correct_err_in_err_area" , 
                                 "parity: failed to log error: status = 0x%x, siots_p = 0x%p\n",
                                 status, siots_p);
            return status;
         }
     }
    else
    {
        fbe_raid_send_unsol(siots_p,
                            sunburst,
                            array_pos,
                            regions_p->regions_array[region_idx].lba,
                            regions_p->regions_array[region_idx].blocks,
                            fbe_raid_error_info(err_bits),
                            fbe_raid_extra_info(siots_p));
    }
    return status;
}
/*******************************************************
 * end fbe_parity_log_correctable_errors_from_error_region() 
 ******************************************************/



/*!***************************************************************
 * fbe_parity_can_log_uncorrectable_from_error_region()
 ****************************************************************
 * @brief
 *  This function will determine whether we should log uncorrectable
 *  message or not against a given region.
 *
 * @param   siots_p         - pointer to the fbe_raid_siots_t
 * @param   region_idx      - index to access the loaction in the fbe_xor_error_region_t.
 *
 * @return  FBE_TRUE if uncorrectable message can be logged else FBE_FALSE 
 *
 * @author
 *  01/27/11 - Created - Jyoti Ranjan
 *
 ****************************************************************/
 static fbe_bool_t fbe_parity_can_log_uncorrectable_from_error_region(fbe_raid_siots_t * siots_p,
                                                                      fbe_u32_t region_index)
 {
    fbe_bool_t b_can_log_uc_error = FBE_FALSE;
    const fbe_xor_error_regions_t * scan_regions_p = &siots_p->vcts_ptr->error_regions;
    fbe_u32_t error_code;
    
    error_code = scan_regions_p->regions_array[region_index].error & FBE_XOR_ERR_TYPE_MASK;
    if ((error_code != FBE_XOR_ERR_TYPE_RAID_CRC) && 
        (error_code != FBE_XOR_ERR_TYPE_INVALIDATED) && 
        (error_code != FBE_XOR_ERR_TYPE_CORRUPT_CRC))
    {
        /* If we are here then we will always log 
         * uncorrectable message.
         */
        b_can_log_uc_error = FBE_TRUE;
    }
    else if (fbe_raid_siots_is_background_request(siots_p) == FBE_TRUE)
    {
        /* For background operations we must report `data lost' invalidated
         * sectors.
         */
        if (fbe_raid_siots_is_background_request(siots_p) == FBE_TRUE)
        {
            /* We must report `data lost' invalidated as an error.
             */
            b_can_log_uc_error = FBE_TRUE;
        }
    }
    else
    {    
        /* Wait a minute! We may not log uncorrectable message if following 
         * conditions are satisifed together:
         *     1. strip is having only corrupt CRC invalidated sectors 
         *     2. Current operation is read operation
         *     3. Strip does not contain other uncorrectable errors 
         *
         *      
         * It is so because blocks had been already invalidated in past and 
         * reported earlier. And, we did not find something new. Other
         * operations log every time primarily so that the BRT will
         * have the necessary entries in the logs.
         */
        if ((siots_p->algorithm == R5_DEG_RD || 
             siots_p->algorithm == R5_RD_VR  ||
             siots_p->algorithm == R5_RD || 
             siots_p->algorithm == R5_SM_RD || 
             siots_p->algorithm == R5_468_VR || 
             siots_p->algorithm == R5_MR3_VR ||
             siots_p->algorithm == R5_RCW_VR ))
        {
            fbe_u32_t scan_index = 0;
            fbe_u32_t curr_error_code = 0; 

            /* Determine if we have any other XOR error region entry (apart 
             * from given one) which is having uncorrectable errors not because 
             * of known invalid pattern.
             */
            for (scan_index = 0; scan_index < scan_regions_p->next_region_index; ++scan_index)
            {
                if ((scan_index != region_index) &&
                    (scan_regions_p->regions_array[scan_index].lba == scan_regions_p->regions_array[region_index].lba) &&
                    (scan_regions_p->regions_array[scan_index].blocks == scan_regions_p->regions_array[region_index].blocks) &&
                    (scan_regions_p->regions_array[scan_index].error & FBE_XOR_ERR_TYPE_UNCORRECTABLE))
                {
                    curr_error_code = scan_regions_p->regions_array[scan_index].error & FBE_XOR_ERR_TYPE_MASK;
                    if ((curr_error_code != FBE_XOR_ERR_TYPE_RAID_CRC) ||
                        (curr_error_code != FBE_XOR_ERR_TYPE_INVALIDATED) ||
                        (curr_error_code != FBE_XOR_ERR_TYPE_CORRUPT_CRC))
                    {
                        /* We have some uncorrectable sectors in strip(s) which
                         * is not because of known invalid pattern.
                         */
                        b_can_log_uc_error = FBE_TRUE;
                        break;
                    }
                } /* end if ((scan_index != region_index) ... */
            } /* end for (scan_index = 0; scan_index < scan_regions_p->next_region_index; ++scan_index) */
        } /* end if ((siots_p->algorithm == R5_DEG_RD) ... ) */
        else
        {
            /* For all other siots's algorithmic operation, we will always 
             * log messages.
             */
            b_can_log_uc_error = FBE_TRUE;
        }
    } /* end if ((xor_error_code != FBE_XOR_ERR_TYPE_RAID_CRC) || (xor_error_code != FBE_XOR_ERR_TYPE_CORRUPT_CRC)) */


    return b_can_log_uc_error;
 }
 /**********************************************************
  * end fbe_parity_can_log_uncorrectable_from_error_region
  *********************************************************/

/*!***************************************************************
 * fbe_parity_log_uncorrectable_errors_from_error_region()
 ****************************************************************
 * @brief
 *  This function will record the uncorrectable error encountered in 
 *  verify to the event log
 *

 * @param   siots_p         - pointer to the fbe_raid_siots_t
 * @param   region_index    - index to access the loaction in the fbe_xor_error_region_t.
 * @param   array_pos       - array position for which we want to log the error
 * @param   b_uc_error_logged_p - pointer to return a value indicating whether uncorrectable error
 *                                were logged or not. Returns FBE_TRUE if an uncorrectable sector 
 *                                or parity sector was logged. Otherwise, it return FBE_FALSE
 *
 * @return   fbe_status_t 
 *
 * @author
 *  11/20/08 - Created - Pradnya Patankar
 *  12/05/08 - Modified for Background Verify Avoidance.   MA
 ****************************************************************/
fbe_status_t fbe_parity_log_uncorrectable_errors_from_error_region(fbe_raid_siots_t * siots_p,
                                                                   fbe_u32_t region_index,
                                                                   fbe_u16_t array_pos,
                                                                   fbe_bool_t * b_uc_error_logged_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t sunburst = 0;
    fbe_u32_t uc_err_bits = 0;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_bool_t b_log_uc_stripe;
    fbe_bool_t b_is_parity_pos;
    fbe_bool_t b_parity_reconstructed = FBE_FALSE; 
    fbe_u16_t array_pos_mask = 1 << array_pos;
    const fbe_xor_error_regions_t * const regions_p =  &siots_p->vcts_ptr->error_regions;
    fbe_bool_t b_log_uc_pos = FBE_FALSE; /* indicates whether given position has uncorrectable error or not*/
    fbe_u16_t error_type = regions_p->regions_array[region_index].error;

    /* Determine if this is the parity position or not.
     */
    b_is_parity_pos = fbe_raid_siots_is_parity_pos(siots_p, array_pos);
     
    /* Determine opcode of the siots operation
     */
    fbe_raid_siots_get_opcode(siots_p, &opcode);

    /* Determine whether we do want to report uncorrectable message or not. 
     * For example: if strip is already invalidated in past with a known pattern, 
     * we do not want to report it again during read operation.
     */
    b_log_uc_stripe = fbe_parity_can_log_uncorrectable_from_error_region(siots_p, region_index);
    

    /* If an unexpected CRC error is found, notify PDO to handle it appropriately, which
     * can be a potential bad drive too.
     *
     * If this is a BVA I/O or the LUN is scheduled for Background Verify do not inform PDO,
     * as the CRC error might be due to an interrupted write, rather than a drive problem.
     *
     * Do not notify PDO if the drive is already marked dead. This could unncessarily kill 
     * a rebuilding drive. DIMS 232965.
     */
    if (b_log_uc_stripe && (FBE_XOR_ERROR_TYPE_IS_UNKNOWN_CRC(error_type)))
    {
         
    } /* end if ( FBE_XOR_ERROR_TYPE_IS_UNKNOWN_CRC(error_type)) */

    b_log_uc_pos = (regions_p->regions_array[region_index].pos_bitmask & array_pos_mask) ? FBE_TRUE : FBE_FALSE;
    if ((b_log_uc_stripe != FBE_TRUE) || (b_log_uc_pos != FBE_TRUE))
    {
        *b_uc_error_logged_p = FBE_FALSE;
        return FBE_STATUS_OK;
    }

    /* If we are here then it is certain that we have uncorrectable error
     * for given position and we have to log it. Determine the uncorrectable 
     * status bits for given xor error region. Also modify it if required 
     * for specific cases (mentioned below)
     */
    
    status = fbe_raid_get_error_bits_for_given_region(&regions_p->regions_array[region_index],
                                                      FBE_FALSE,
                                                      array_pos_mask,
                                                      FBE_FALSE,
                                                      &uc_err_bits);
    if (RAID_COND(status != FBE_STATUS_OK)) { return status; }
    
    /* Modify error bits as per reason mentioned below.
     */
    if (fbe_raid_siots_pos_is_dead(siots_p, array_pos)) 
    {
        if (!fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)))
        {
            /* For Raid5/Raid3 only:
             * The extended status for an uncorrectable on the dead position
             * should always be zero, since the uncorrectable on the
             * dead position is by definition caused by an error on a
             * different position.  This is not true for some R6 COH errors.
             */
            uc_err_bits = 0;
        }
        else 
        {
            /* If this is a rebuilding drive in a RAID 6 LUN a CRC error 
             * might be logged against it.  Since this error is always the
             * result of an error on another drive we don't want to output
             * this error.
             */
            uc_err_bits &= ~VR_UNEXPECTED_CRC;
        }
    }


    /* If we have uncorrectable coherency errors on parity, then we will 
     * log them with the coherency event log message..This is the only 
     * case on Raid 6 where we log the coherency event log message.
     *
     * Please note that FBE_XOR_ERR_TYPE_COH or FBE_XOR_ERR_TYPE_1POC or 
     * FBE_XOR_ERR_TYPE_N_POC_COH meant only for RAID 6.
     */
    if (fbe_raid_siots_is_parity_pos(siots_p, array_pos) &&           
        ( ((error_type & FBE_XOR_ERR_TYPE_MASK) == FBE_XOR_ERR_TYPE_COH) || 
          ((error_type & FBE_XOR_ERR_TYPE_MASK) == FBE_XOR_ERR_TYPE_1POC) || 
          ((error_type & FBE_XOR_ERR_TYPE_MASK) == FBE_XOR_ERR_TYPE_N_POC_COH) || 
          ((error_type & FBE_XOR_ERR_TYPE_MASK) == FBE_XOR_ERR_TYPE_POC_COH) ) )
    {
        /* Log this uncorrectable coherency error on parity as a coherency 
         * error instead of as an uncorrectable,since we never invalidate 
         * parity on Raid 6. This error was reported  as an uncorrectable 
         * coherency error, since it is the one case on Raid 6 where we do not
         * know where the parity is and thus will rebuild parity.
         * However if this is a BVA verify and if the sectors with errors are
         * reconstructed during BVA write then report it as reconstructed.
         */
        if(fbe_raid_siots_is_marked_for_incomplete_write_verify(siots_p) ||
           fbe_raid_siots_is_metadata_operation(siots_p))
        {
            /* Note the fact that the coherency error was found when an incomplete write verify was
             * outstanding 
             */
            sunburst = SEP_INFO_CODE_RAID_EXPECTED_COHERENCY_ERROR;
        }
        else
        {
            sunburst = SEP_ERROR_CODE_RAID_HOST_COHERENCY_ERROR;
        }
        if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE)
        {
            status = fbe_parity_bva_log_error_in_write(siots_p, 
                                                       region_index,
                                                       array_pos,
                                                       sunburst,
                                                       uc_err_bits);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, 
					  "parity_uncorrect_err_in_err_area" , 
                                     "parity: failed to log error: siots_p = 0x%p, status = 0x%x \n",
                                     siots_p, 
                                     status);
                return status;
            }
        }
        else
        {
            fbe_raid_send_unsol(siots_p,
                                sunburst,
                                array_pos,
                                regions_p->regions_array[region_index].lba,
                                regions_p->regions_array[region_index].blocks,
                                fbe_raid_error_info(uc_err_bits),
                                fbe_raid_extra_info(siots_p));

            *b_uc_error_logged_p = FBE_TRUE;
        }/* end if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE) */
    } /* end if (fbe_raid_siots_is_parity_pos(siots_p, array_pos) && ((error_type == FBE_XOR_ERR_TYPE_COH) ... */
    else
    {
         /* The operation is either the first read or not a read at all.
          *
          * Log the uncorrectable error.
          * If this is a BVA verify and if the sectors with errors are
          * reconstructed during BVA write then report it as reconstructed.
          * with new data
          */
        if (b_is_parity_pos == FBE_TRUE)
        {
            b_parity_reconstructed = FBE_TRUE;
            sunburst = SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_PARITY_SECTOR;
        }

        else if ((uc_err_bits & VR_INVALID_CRC) &&
                 fbe_raid_geometry_is_vault(fbe_raid_siots_get_raid_geometry(siots_p)))
        {
            /* Simply skip logging the error. We do not log these errors on the vault,
             * since the vault writes invalid blocks normally.
             */
            return FBE_STATUS_OK;
        }
        else
        {
            sunburst = SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR;
        }

        /* Check for BVA write
         */
         if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE)
         {
             status = fbe_parity_bva_log_error_in_write(siots_p, 
                                                        region_index,
                                                        array_pos,
                                                        sunburst,
                                                        uc_err_bits);
             if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
             {
                 fbe_raid_siots_trace(siots_p, FBE_TRACE_LEVEL_ERROR, __LINE__, 
				 	  "parity_uncorrect_err_in_err_area" , 
                                      "parity: failed to log error: status = 0x%x, siots_p = 0x%p\n",
                                      status, siots_p);
                 return status;
             }
         }
         else if (fbe_raid_siots_is_metadata_operation(siots_p))
         {
             fbe_raid_send_unsol(siots_p,
                                 SEP_INFO_CODE_RAID_EXPECTED_COHERENCY_ERROR,
                                 array_pos,
                                 regions_p->regions_array[region_index].lba,
                                 regions_p->regions_array[region_index].blocks,
                                 fbe_raid_error_info(uc_err_bits),
                                 fbe_raid_extra_info(siots_p));
             *b_uc_error_logged_p = FBE_TRUE;
         }
         else
         {
             /* Log an uncorrectable sector message
              */
             fbe_raid_send_unsol(siots_p,
                                 sunburst,
                                 array_pos,
                                 regions_p->regions_array[region_index].lba,
                                 regions_p->regions_array[region_index].blocks,
                                 fbe_raid_error_info(uc_err_bits),
                                 fbe_raid_extra_info(siots_p));

             /* Log sector invalidated message. Please note that we do 
              * log different sunburst code for parity and data position
              * while dumping invalidated message.
              */
             sunburst = (b_is_parity_pos == FBE_TRUE) 
                             ? SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED
                             : SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED;
             if ((b_is_parity_pos == FBE_TRUE) && (b_parity_reconstructed != FBE_TRUE))
             {
                 /* Something went wrong otherwise we should not have been here.
                  */
                 fbe_raid_siots_trace(siots_p, 
                                      FBE_TRACE_LEVEL_ERROR, __LINE__, "parity_uncorrect_err_in_err_area" , 
                                      "parity: (array_pos 0x%x == siots_p->parity_pos 0x%x) && (b_parity_reconstructed != FBE_TRUE)",
                                      array_pos, siots_p->parity_pos);
                 return FBE_STATUS_GENERIC_FAILURE;
             }

             fbe_raid_send_unsol(siots_p,
                                 sunburst,
                                 array_pos,
                                 regions_p->regions_array[region_index].lba,
                                 regions_p->regions_array[region_index].blocks,
                                 fbe_raid_error_info(uc_err_bits),
                                 fbe_raid_extra_info(siots_p));

             *b_uc_error_logged_p = FBE_TRUE;
        }
    } /* else end-if (fbe_raid_siots_is_parity_pos(siots_p, array_pos) && ((error_type == FBE_XOR_ERR_TYPE_COH) ... */

    /* We have scanned all regions and reported messages. But there may
     * be cases where we might have missed to log parity invalidated 
     * messages. Please log them too.
     */
    
    return FBE_STATUS_OK;
}
/**************************************************************
 * end fbe_parity_log_uncorrectable_errors_from_error_region()
 **************************************************************/



/*!*********************************************************************
 * fbe_parity_log_parity_drive_invalidation()
 ***********************************************************************
 * @brief
 *  Report parity invalidated messages in event log only if it is
 *  to be reported but has not been reported so far.
 *
 * @param  siots_p           - pointer to siots
 *
 * @return fbe_status_t
 *
 * @author
 *  11/20/08 - Created - Pradnya Patankar
 *  01/09/11 - Rewrote - Jyoti Ranjan
 *  09/28/2012 - Remove dead drive error, now generates its own error region - Dave Agans
 *
 * @note:
 * The function is supposed to be called only while reporting event
 * log messages using XOR error region.
 ***********************************************************************/
static fbe_status_t fbe_parity_log_parity_drive_invalidation(fbe_raid_siots_t * siots_p)
{
    fbe_u32_t region_index, scan_index;
    fbe_u16_t uc_strip_bitmask = 0;
    fbe_u16_t c_strip_bitmask = 0;
    fbe_bool_t xor_region_traversed[FBE_XOR_MAX_ERROR_REGIONS] = { 0 };
    fbe_xor_error_regions_t * regions_p = NULL;
    fbe_xor_error_regions_t * scan_regions_p = NULL;
    fbe_u32_t sunburst = 0;

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
     *      invalidation message if it is not already done by other code.
     */
    for(region_index = 0; region_index < regions_p->next_region_index; ++region_index)
    {
        /* Flush-out previous state of uncorrectable as well correctable strip bitmask
         */
        uc_strip_bitmask = c_strip_bitmask = 0;
 
        if (xor_region_traversed[region_index] != FBE_TRUE)
        {
            if (   (regions_p->regions_array[region_index].error & FBE_XOR_ERR_TYPE_UNCORRECTABLE)
                   && ((regions_p->regions_array[region_index].error & FBE_XOR_ERR_TYPE_MASK) != FBE_XOR_ERR_TYPE_INVALIDATED)
                   && ((regions_p->regions_array[region_index].error & FBE_XOR_ERR_TYPE_MASK) != FBE_XOR_ERR_TYPE_RAID_CRC)
                   && ((regions_p->regions_array[region_index].error & FBE_XOR_ERR_TYPE_MASK) != FBE_XOR_ERR_TYPE_CORRUPT_CRC))
            {
                uc_strip_bitmask = regions_p->regions_array[region_index].pos_bitmask;
            }
            else
            {
                c_strip_bitmask = regions_p->regions_array[region_index].pos_bitmask;
            }

            /* Scan through all regions to figure out if any other error-regions 
             * collaborating with current xor error region form uncorrectable 
             * strips or not. In other words, ultimate objective is to find 
             * a chain of xor error regions causing strips to be uncorrectable.
             */
            for (scan_index = 0; scan_index < scan_regions_p->next_region_index; ++scan_index)
            {
                if ((scan_regions_p->regions_array[scan_index].lba == regions_p->regions_array[region_index].lba) &&
                    (scan_regions_p->regions_array[scan_index].blocks >= regions_p->regions_array[region_index].blocks))
                {
                    if (   (regions_p->regions_array[scan_index].error & FBE_XOR_ERR_TYPE_UNCORRECTABLE)
                           && ((regions_p->regions_array[scan_index].error & FBE_XOR_ERR_TYPE_MASK) != FBE_XOR_ERR_TYPE_INVALIDATED)
                           && ((regions_p->regions_array[scan_index].error & FBE_XOR_ERR_TYPE_MASK) != FBE_XOR_ERR_TYPE_RAID_CRC)
                           && ((regions_p->regions_array[scan_index].error & FBE_XOR_ERR_TYPE_MASK) != FBE_XOR_ERR_TYPE_CORRUPT_CRC))
                    {
                        uc_strip_bitmask = uc_strip_bitmask | scan_regions_p->regions_array[scan_index].pos_bitmask;
                    }
                    else
                    {
                        c_strip_bitmask = c_strip_bitmask | scan_regions_p->regions_array[scan_index].pos_bitmask;
                    }

                    /* Drop this region from further scanning as we have already covered  
                     * all possible cases (correctable as well as uncorrectable) where
                     * it could have been contributed for a  given strip(s).
                     */
                    xor_region_traversed[scan_index] = FBE_TRUE;
                }
            } /* end for (scan_index = 0; scan_index < scan_region_p->next_region_index; ++scan_index) */
        }

        /* By now, we must have traversed this region. So, update traverse list to reflect it.
         */
        xor_region_traversed[region_index] = FBE_TRUE;


        /* At this point of time, we can investigate uncorrectable bitmask
         * to figure out whether current xor error regions can cause 
         * uncorrectable strip(s) in association with other xor error region in
         * list. So, we will log parity invalidation messages only if following
         * conditions are satisfied:
         *      1. Current xor region can cause uncorrectable strip(s) either 
         *         standalone or in association with other error regions.
         *      2. Uncorrectable strip (if it exists) has not already
         *         caused parity invalidation messages logged.
         */
        /* Log parity reconstructed messages, if there are uncorrectable strip(s) and 
         * parity invalidation message have not been logged already.
         */
        if (uc_strip_bitmask != 0)
        {
            if (!fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)))
            {
                fbe_u16_t parity_bitmask = (1 << siots_p->parity_pos);
                fbe_bool_t b_parity_invalidated = FBE_TRUE;

                b_parity_invalidated = (uc_strip_bitmask & parity_bitmask) || (c_strip_bitmask & parity_bitmask); 
                if (b_parity_invalidated != FBE_TRUE)
                {
                     fbe_raid_send_unsol(siots_p,
                                         SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED,
                                         siots_p->parity_pos,
                                         regions_p->regions_array[region_index].lba,
                                         regions_p->regions_array[region_index].blocks,
                                         fbe_raid_error_info(0),
                                         fbe_raid_extra_info(siots_p));
                }
            } /* end  if (!fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p))) */
            else
            {
                /* We are here as its a RAID 6 group. 
                 */
                fbe_u32_t rp_pos, dp_pos;
                fbe_u16_t rp_bitmask, dp_bitmask;
                fbe_bool_t b_rp_invalidated, b_dp_invalidated;

                rp_pos = fbe_raid_siots_get_row_parity_pos(siots_p);
                dp_pos = fbe_raid_siots_dp_pos(siots_p);
                rp_bitmask = (1 << rp_pos);
                dp_bitmask = (1 << dp_pos);

                b_rp_invalidated = (uc_strip_bitmask & rp_bitmask) || (c_strip_bitmask & rp_bitmask);
                b_dp_invalidated = (uc_strip_bitmask & dp_bitmask) || (c_strip_bitmask & dp_bitmask);


                sunburst = SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED;

                if (b_rp_invalidated != FBE_TRUE)
                {
                     fbe_raid_send_unsol(siots_p,
                                         sunburst,
                                         rp_pos,
                                         regions_p->regions_array[region_index].lba,
                                         regions_p->regions_array[region_index].blocks,
                                         fbe_raid_error_info(0),
                                         fbe_raid_extra_info(siots_p));
                }

                sunburst = SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED;

                if (b_dp_invalidated != FBE_TRUE)
                {
                     fbe_raid_send_unsol(siots_p,
                                         sunburst,
                                         dp_pos,
                                         regions_p->regions_array[region_index].lba,
                                         regions_p->regions_array[region_index].blocks,
                                         fbe_raid_error_info(0),
                                         fbe_raid_extra_info(siots_p));
                }
            } /* end else if (!fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p))) */
        } /* end if (uc_strip_bitmask != 0) */ 
    } /* end for(region_index = 0; region_index < regions_p->next_region_index; ++region_index) */

    return FBE_STATUS_OK;
}
/***************************************************
 * end of fbe_parity_log_parity_drive_invalidation()
 ***************************************************/

/*!***************************************************************
 * fbe_raid_count_no_of_ones_in_bitmask()
 ****************************************************************
 * @brief
 *  This function will count the number of bits set in the bitmask 
 *  
 *  bitmask [I] - bitmask for which we want to count no of bits set.
 *
 * @return VALUE:
 *  bit_count - number of ones in the bitmask.
 *
 * @author
 *  11/20/08 - Created        PP
 *
 ****************************************************************/
fbe_u32_t fbe_raid_count_no_of_ones_in_bitmask(fbe_u16_t bitmask)
{
    fbe_u32_t bit_count = 0;
    /* Count the number of 1's in the bitmask to find out 
     * number of data disks with an error.
     */
    while (bitmask)
    { 
        bit_count++;
        bitmask = bitmask & (bitmask - 1);
    }
    return bit_count;
}
/************************************************
 * end of fbe_raid_count_no_of_ones_in_bitmask()
 ************************************************/

/*!***************************************************************
 * fbe_parity_can_log_u_error_for_given_pos()
 ****************************************************************
 * @brief
 *  This function determines if we are allowed to log any
 *  uncorrectable errors for -GIVEN POSITION-. Eboard is used to 
 *  determine whether uncorrectable logging is allowed or not.
 *

 * @param siots_p - ptr to the fbe_raid_siots_t
 * @param array_pos - Parity position.
 * @param parity_start_pba - Address for the start of parity.
 * @param error_bits_p - Ptr to the error bits to return.
 *
 * @return VALUE:
 *  fbe_bool_t - FBE_TRUE if an uncorrectable is allowed to be logged.
 *
 * ASSUMPTIONS:
 * 
 * @author
 *  05/16/07 - Created. Rob Foley.
 ****************************************************************/
static fbe_bool_t fbe_parity_can_log_u_error_for_given_pos(fbe_raid_siots_t *const siots_p, 
                                                           const fbe_u16_t array_pos, 
                                                           const fbe_lba_t parity_start_pba,
                                                           fbe_u32_t * const error_bits_p)
{
    fbe_u32_t err_bits; /* Error bits to report for this position. */

    /* array_pos_mask has the current position in the
     * error bitmask that we need to check below.
     */
    fbe_u16_t array_pos_mask = 1 << array_pos;

    /* FBE_TRUE if we should log the erorr, FBE_FALSE otherwise.
     */
    fbe_bool_t b_log = FBE_FALSE;

    /* Consult eboard to figure out whether there is uncorrectable 
     * status bits or not.
     */    
    err_bits = fbe_raid_get_uncorrectable_status_bits( &siots_p->vrts_ptr->eboard, array_pos_mask );
        
    if ( err_bits != 0 )
    {
        fbe_u32_t dead_position = fbe_raid_siots_dead_pos_get(siots_p, FBE_RAID_FIRST_DEAD_POS);

        if (!fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)) &&
            (dead_position != FBE_RAID_INVALID_DEAD_POS) &&
            (dead_position == array_pos))
        {
            /* For Raid5/Raid3 only:
             * The extended status for an uncorrectable on the dead position
             * should always be zero, since the uncorrectable on the
             * dead position is by definition caused by an error on a
             * different position.  This is not true for some R6 COH errors.
             */
            err_bits = 0;
        }
        else if (fbe_raid_geometry_is_raid6(fbe_raid_siots_get_raid_geometry(siots_p)) &&
                 fbe_raid_siots_pos_is_dead(siots_p, array_pos))
        {
            /* If this is a rebuilding drive in a RAID 6 LUN a CRC error 
             * might be logged against it.  Since this error is always the
             * result of an error on another drive we don't want to output
             * this error.
             */
            err_bits &= ~VR_UNEXPECTED_CRC;
        }
            
        /* If this is a read or write operation and the block has been invalidated
         * already do not log anything.  This will cause errors on  reads
         * to be logged on the first encounter and no future ones.  Other
         * operations log every time primarily so that the BRT will
         * have the necessary entries in the logs.
         */
        if ((siots_p->algorithm == R5_DEG_RD ||
             siots_p->algorithm == R5_RD_VR  ||
             siots_p->algorithm == R5_RD ||
             siots_p->algorithm == R5_SM_RD  || 
             siots_p->algorithm == R5_468_VR || 
             siots_p->algorithm == R5_MR3_VR ||
             siots_p->algorithm == R5_RCW_VR ) &&
            (err_bits == VR_CORRUPT_CRC ||
             err_bits == VR_INVALID_CRC))
        {
            /* This is the case where we do not need to log.
             */
        }
        else
        {
            /* In this case we should log the error.
             */
            b_log = FBE_TRUE;
        }
    }

    /* Set the error bits into the passed in ptr.
     */
    *error_bits_p = err_bits;
    
    return b_log;
}
/*********************************************
 * fbe_parity_can_log_u_error_for_given_pos()
 ********************************************/

/*!**************************************************************
 * fbe_parity_verify_get_write_bitmap()
 ****************************************************************
 * @brief
 *   Determine what our write bitmap should be for this verify.
 *   We take degraded positions and shed data on parity
 *   into account here.  The write bitmap is the positions which
 *   need to be written to correct errors and keep the stripe consistent.
 *  
 * @param siots_p - Siots to calculate the write bitmap for.
 * 
 * @return fbe_u16_t - Bitmask of positions we need to write.
 *
 ****************************************************************/

fbe_u16_t fbe_parity_verify_get_write_bitmap(fbe_raid_siots_t * const siots_p)
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

    if (siots_p->algorithm == R5_VR)
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
 * end fbe_parity_verify_get_write_bitmap()
 **************************************/

/*!*************************************************************************
 * fbe_parity_bva_log_error_in_write()
 **************************************************************************
 * @brief
 *        This function checks if the given PBA is reconstructed
 *        during BVA write or not.This function is called for given
 *        disk position and hence we will restrict ourself for the 
 *        given disk.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 * @param region_idx - index to access the location in the 
 *                    fbe_xor_error_region_t.
 * @param array_pos - array position for which we want to log the error
 * @param sunburst - sunburst error code to be used for the error passed
 * @param err_bits - bits showing disk position on which correctable 
 * 
 * @return   fbe_status_t
 *
 * @note
 *       This function has limitations as it uses fbe_xor_error_region_t for 
 *       finding the errored sector. We can report only those PBAs which
 *       are logged in the fbe_xor_error_region_t
 *
 * @author
 * created 17/10/2008 - Mahesh Agarkar
 *************************************************************************/
 fbe_status_t fbe_parity_bva_log_error_in_write(fbe_raid_siots_t *siots_p,
                                fbe_u32_t region_idx,
                                fbe_u32_t array_pos,
                                fbe_u32_t sunburst,
                                fbe_u32_t err_bits)
{
    fbe_bool_t b_err_corr = FBE_FALSE;
    fbe_lba_t error_start = FBE_RAID_INVALID_LBA;
    fbe_lba_t error_end = FBE_RAID_INVALID_LBA;
    fbe_lba_t mid_start = FBE_RAID_INVALID_LBA;
    fbe_lba_t mid_end = FBE_RAID_INVALID_LBA;
    fbe_lba_t error_pba = FBE_RAID_INVALID_LBA;
    fbe_status_t status = FBE_STATUS_OK;
    /* Number of errored blocks.
     */
    fbe_u32_t blocks_corrected = 0;

    fbe_xor_error_regions_t * regions_p = &siots_p->vcts_ptr->error_regions;
    fbe_xor_error_region_t * err_region = &regions_p->regions_array[region_idx];
    fbe_u32_t end_pba = (fbe_u32_t) (err_region->lba + err_region->blocks -1);

    for (error_pba = err_region->lba ; error_pba < (end_pba + 1); error_pba++)
    {
        if(!fbe_raid_siots_is_parity_pos(siots_p,array_pos))
        {
            status = fbe_parity_bva_is_err_on_data_overwritten(siots_p,
                                                               error_pba,
                                                               array_pos,
                                                               &b_err_corr);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity: unexpected error: status = 0x%x, siots_p = 0x%p, "
                                     "error_pba = 0x%llx, array_pos = 0x%d\n",
                                     status, siots_p,
				     (unsigned long long)error_pba, array_pos);
                return (status);
            }
        }
        else
        {
            /* it is a parity disk 
             */
            status = fbe_parity_bva_is_err_on_parity_overwritten(siots_p,
                                                             error_pba,
                                                             array_pos,
                                                             &b_err_corr);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity: unexpected error: status = 0x%x, siots_p = 0x%p, "
                                     "error_pba = 0x%llx, array_pos = 0x%d\n",
                                     status, siots_p,
				     (unsigned long long)error_pba, array_pos);
                return (status);
            }
        }
        if(b_err_corr == FBE_FALSE)
        {
            if(error_start == FBE_RAID_INVALID_LBA)
            {
                error_start = error_end = error_pba;
            }
            else
            {
                error_end = error_pba;
            }
        }
        else if ( error_start != FBE_RAID_INVALID_LBA)
        {
            /* we are able to correct some sectors in between the error region
             * Here mid_start and mid_end will always give us the corrected PBAs
             */
            if(mid_end == FBE_RAID_INVALID_LBA)
            {
                mid_start = mid_end = error_pba;
            }
            else
            {
                mid_end = error_pba;
            }
        }/* end else if(b_err_corr == FBE_FALSE)*/
    } /* end for (count = 0; count < region_p->count; count++)*/

    /* let us report the error messages now 
     */
    if((error_start == FBE_RAID_INVALID_LBA) && (error_end == FBE_RAID_INVALID_LBA))
    {
        /* We are able to correct all the errors in this error region 
         * and hence we will clear the bitmask in the error region 
         */
        fbe_u16_t bitmask = 1 << array_pos;
        err_region->pos_bitmask &= (~ (bitmask));
        fbe_parity_bva_report_sectors_reconstructed(siots_p,
                                            err_region->lba,
                                            sunburst,
                                            err_bits,
                                            array_pos,
                                            FBE_TRUE,
                                            err_region->blocks);
    return status;
    }

    if(error_start != err_region->lba)
    {
        blocks_corrected = (fbe_u32_t)(error_start - err_region->lba);
        /* log that we are able to correct this sector
         */
        fbe_parity_bva_report_sectors_reconstructed(siots_p,
                                            err_region->lba,
                                            sunburst,
                                            err_bits,
                                            array_pos,
                                            FBE_TRUE,
                                            blocks_corrected);

        /* we could correct some sectors in the begining of this
         * error region. We will update the error region accordingly.
         */
         err_region->lba = error_start;
         err_region->blocks = (fbe_u16_t) (end_pba - error_start + 1);

        /* log message for sector not corrected during BVA write
         */
        fbe_parity_bva_report_sectors_reconstructed(siots_p,
                                            error_start,
                                            sunburst,
                                            err_bits,
                                            array_pos,
                                            FBE_FALSE,
                                            err_region->blocks);
    }
    else
    {
        /* for given error region we can have some sectors reconstructed
         * during BVA and some are not. We may need to log more than one
         * in that case messages.
         */
        fbe_parity_bva_report_sectors_reconstructed(siots_p,
                                            error_start,
                                            sunburst,
                                            err_bits,
                                            array_pos,
                                            FBE_FALSE,
                                            (fbe_u32_t)(error_end - error_start + 1));

        if (error_end < end_pba)
        {
            /* We are able to reconstruct some part in the lower side 
             * of the error region. log the message for reconstructed
             * PBA during BVA write.and also update the error region
             * accordingly.
             */
            err_region->blocks = (fbe_u16_t) (error_end - error_start + 1);
            blocks_corrected = (fbe_u32_t)(error_end - error_start);
            fbe_parity_bva_report_sectors_reconstructed(siots_p,
                                                error_end,
                                                sunburst,
                                                err_bits,
                                                array_pos,
                                                FBE_TRUE,
                                                blocks_corrected);
        }

        else if (mid_start != FBE_RAID_INVALID_LBA)
        {
            /* in this case we are correcting the sectors in between the 
             * error region blocks as follows:
             *       |----------|PBA X
             *       |   error  |<- sectors with error 
             *       |----------|PBA Y
             *       |//////////| <--- sector corrected during BVA
             *       |----------|PBA Z
             *       |   error  |
             * We will log three messages here as follows:
             *  (1) error_start :- (sector with error and not corrected)
             *  (2) mid_start   :- (sector corrected)
             *  (3) mid_end + 1 :- (sector with error and not corrected)
             * message for (1) is already logged above, now report the 
             * remaining two here
             */
            blocks_corrected = (fbe_u32_t) (mid_end - mid_start);
            fbe_parity_bva_report_sectors_reconstructed(siots_p,
                                                mid_start,
                                                sunburst,
                                                err_bits,
                                                array_pos,
                                                FBE_TRUE,
                                                blocks_corrected);
            /* log message for sector not corrected during BVA write
             */
            fbe_parity_bva_report_sectors_reconstructed(siots_p,
                                                (mid_end + 1),
                                                sunburst,
                                                err_bits,
                                                array_pos,
                                                FBE_FALSE,
                                                (fbe_u32_t)(error_end - mid_end));
        }/* end         else if (mid_start != 0)*/
    } /* end if(error_start != err_region->lba) */
    return status;
}
/**************************************************************************
 * end of fbe_parity_bva_log_error_in_write()
 **************************************************************************/

/*!*************************************************************************
 * fbe_parity_bva_is_err_on_data_overwritten()
 **************************************************************************
 * @brief:
 *        This function will be called for BVA to check whether the
 *        errored LBA is residing in the WRITE region.on given data
 *        disk.
 *
 * @param siots_p - ptr to the fbe_raid_siots_t
 * @param error_pba - the pba to be verify whether corrected or not
 * @param array_pos - array position for which we want to log the error
 * 
 * @return   fbe_status_t
 *
 * @author   
 * created 17/10/2008 - MA
 * 
 ***************************************************************************/
fbe_status_t fbe_parity_bva_is_err_on_data_overwritten(fbe_raid_siots_t * siots_p,
                                       fbe_lba_t error_pba,
                                       fbe_u32_t array_pos,
                                       fbe_bool_t* b_err_corr)
{
    fbe_lba_t error_lba;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t start_lba = 0;
    fbe_lba_t end_lba = 0;

    /* make sure we are not doing calculation for parity postion
     * and it is a NESTED SIOTS
     */
    if (RAID_COND((!( siots_p->common.flags & FBE_RAID_COMMON_FLAG_TYPE_NEST_SIOTS)) ||
                  (fbe_raid_siots_is_parity_pos(siots_p,array_pos))))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "siots_p->common.flags 0x%x is not set to RAID_TYPE_NEST_SIOTS and array pos 0x%x is not parity position. \n",
                             siots_p->common.flags,
                             array_pos );
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Here, we need the start LBA of the whole request.
     * Previously we used to get it through IORB.
     * Now it is packets->lba.
     */
    start_lba = fbe_raid_siots_get_iots(siots_p)->packet_lba;
    end_lba = (fbe_raid_siots_get_iots(siots_p)->packet_lba) + (fbe_raid_siots_get_iots(siots_p)->packet_blocks) - 1;

    status = fbe_raid_convert_pba_to_lba(siots_p, error_pba, array_pos, &error_lba);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: siots_p: 0x%p error_pba: 0x%llx array_pos: %d failed status: 0x%x\n",
                             siots_p, (unsigned long long)error_pba, array_pos,
			     status);
        return status;
    }

    /* Now check if it is correctable using the write data.
     */
    if ((error_lba >= start_lba) && (error_lba <= end_lba))
    {
        *b_err_corr = FBE_TRUE;
    }
    else
    {
        /* This LBA does not reside in the WRITE region of BVA
         */
        *b_err_corr = FBE_FALSE;
     }
     return status;
}
/**************************************************************************
 * end of fbe_parity_bva_is_err_on_data_overwritten()
 **************************************************************************/

/*!*************************************************************************
 * fbe_parity_bva_is_err_on_parity_overwritten()
 **************************************************************************
 * @brief
 *       This function will be called for BVA to check whether the
 *       errored sector on parity will be reconstructed or not
 *
 * @param   siots_p - ptr to the fbe_raid_siots_t
 * @param   error_pba - index to access the loaction in the 
 * @param   array_pos - array position for which we want to log the error
 *
 * @note  This error is logged against parity drive. 
 *        If it is correctable and even if a single block is written 
 *        with new data during BVA write along the same PBA on any of 
 *        the data disks then this parity sector will be reconstructed.
 *        But if we are left with even a single error on this strip then
 *        we can not claim that the parity is reconstructed.
 *
 * @return   fbe_status_t
 *
 * @author
 *  created 17/10/2008 - MA
 ***************************************************************************/
fbe_status_t fbe_parity_bva_is_err_on_parity_overwritten(fbe_raid_siots_t *siots_p,
                                         fbe_lba_t error_pba,
                                         fbe_u32_t array_pos,
                                         fbe_bool_t* b_err_corr)
{
    fbe_bool_t corr_data = FBE_FALSE;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t disk_pos;
    fbe_u32_t region_idx;
    fbe_raid_geometry_t *raid_geometry_p;
    fbe_u32_t num_frus;
    fbe_raid_group_type_t raid_type;

    fbe_u16_t lu_width;
    fbe_xor_error_regions_t *regions_p;
    fbe_u32_t data_count = 0;
    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_raid_geometry_get_width(raid_geometry_p, &num_frus);

    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);

    fbe_raid_geometry_get_data_disks(raid_geometry_p, &lu_width);

    regions_p = &siots_p->vcts_ptr->error_regions;

    for(disk_pos = 0 ; disk_pos < num_frus; disk_pos++)
    {
        fbe_u16_t pos_bitmask = (1 << disk_pos);

        if (!fbe_raid_siots_is_parity_pos(siots_p,disk_pos))
        {
            status = fbe_parity_bva_is_err_on_data_overwritten(siots_p,
                                                               error_pba,
                                                               disk_pos,
                                                               &corr_data);
            if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
            {
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                     "parity: unexpected error: status = 0x%x, siots_p = 0x%p, "
                                     "error_pba = 0x%llx, array_pos = 0x%d\n",
                                     status, siots_p,
				     (unsigned long long)error_pba, disk_pos);
                return (status);
            }

            if(corr_data == FBE_TRUE)
            {
                /* we are able to perform write here during BVA
                 */
                data_count++;
            }
            else
            {
                /* we need check if we are having any error on this PBA. We
                 * will written FBE_FALSE as we are not overwriting this one 
                 */

                for (region_idx = 0; 
                     region_idx < regions_p->next_region_index; 
                     ++region_idx)
                {
                    fbe_xor_error_region_t * err_region = 
                                      &regions_p->regions_array[region_idx];

                    fbe_lba_t err_reg_end = err_region->lba + err_region->blocks - 1;

                    if((err_region->pos_bitmask & pos_bitmask) && 
                       ((error_pba >= err_region->lba) && 
                        (error_pba <= err_reg_end)))
                    {
                        /* We are having an error and we failed to overwrite 
                         * this parity sector during BVA.
                         */
                        *b_err_corr = FBE_FALSE;
                    }/* end if((err_region->pos_bitmask ..)*/

                }/* end for (region_idx = 0;  */
            }/* end else if(r5_bva_is_err_on_data_overwritten ..)*/
        }/* end if (!fbe_raid_siots_is_parity_pos(siots_p,disk_pos))*/
    }/* for(disk_pos = 0 ; disk_pos < num_frus; disk_pos++)*/

    if(data_count > 0)
    {
        /* If we do not have any error on this STRIP and if we are writing
         * new data during BVA on at least one sector on this STRIP we 
         * we will claim that we are able to reconstruct this parity sector
         * during BVA
         */
        *b_err_corr = FBE_TRUE;
    }
    else 
    {
        *b_err_corr = FBE_FALSE;
    }
    return status;
}
/**************************************************************************
 * end fbe_parity_bva_is_err_on_parity_overwritten()
 **************************************************************************/

/*!***************************************************************************
 *   fbe_raid_convert_pba_to_lba()
 *****************************************************************************
 * @brief
 *       This function converts PBA on given disk to corresponding 
 *       LBA 
 *
 * @param   siots_p - fbe_raid_siots_t pointer
 * @param   pba - PBA to be converted to LBA
 * @param   disk - disk position for which PBA is given
 * @param   lba_for_pba_p - Pointer to lba for this pba to update
 *
 * @return  status - Typically FBE_STATUS_OK
 *
 * @note    The lba value in a fruts does NOT include the raid group address
 *          offset (the raid group offset is on the edge BELOW the raid group)
 *
 * @author
 *  11/11/2008 - created Mahesh Aagarkar
 *****************************************************************************/
fbe_status_t fbe_raid_convert_pba_to_lba(fbe_raid_siots_t * siots_p, 
                                         fbe_lba_t disk_pba, 
                                         fbe_u32_t array_pos,
                                         fbe_lba_t *lba_for_pba_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_lba_t               return_lba = FBE_LBA_INVALID;       /* actual LBA */
    fbe_raid_group_type_t   raid_type;
    fbe_u32_t               width = fbe_raid_siots_get_width(siots_p);
    fbe_element_size_t      element_size;  /* no of sectors per element */
    fbe_u16_t               data_disks;
    fbe_lba_t               raid_group_offset;
    fbe_u32_t               parity_pos;
    fbe_raid_geometry_t    *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_bool_t              b_allow_full_journal_access = FBE_FALSE;

    /* Initialize the return value to invalid
     */
    *lba_for_pba_p = return_lba;

    /*! Get the raid group offset raid no longer has the concept of lun
     *  offset (the lun offset is added to the lba before raid receives the
     *  request).
     */
    status = fbe_raid_siots_get_raid_group_offset(siots_p, &raid_group_offset);
    if (RAID_COND_STATUS((status != FBE_STATUS_OK), status))
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: failed to get raid group offset: status = 0x%x; siots_p = 0x%p\n",
                             status, siots_p);
        return status;
    }

    /* Now check if the disk_pba (i.e. the fruts lba value) exceeds the per 
     * position extent range.
     */
    if (siots_p->algorithm == RG_ZERO)
    {
        b_allow_full_journal_access = FBE_TRUE;
    }
    if (fbe_raid_geometry_does_request_exceed_extent(raid_geometry_p, disk_pba, 1, b_allow_full_journal_access) == FBE_TRUE)
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                             "siots_p: 0x%p lba 0x%llx blocks: 0x%x is beyond capacity \n",
                             siots_p, (unsigned long long)disk_pba, 1);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    /* if we are a parirty group then make sure that we are not doing 
     * the conversion for parity position
     */
    if(fbe_raid_geometry_is_parity_type(raid_geometry_p))
    {
        if (RAID_COND(fbe_raid_siots_is_parity_pos(siots_p, array_pos)))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid: siots_p: 0x%p array pos: %d is parity pos \n",
                                 siots_p, array_pos);
            return FBE_STATUS_GENERIC_FAILURE; 
        }

        /* considering that R5/R6 are right symmetric with rotating parity  we 
         * need to adjust the array_pos here for LBA calculation.This array_pos
         * is used to calculate the exact LBA
         */
        parity_pos = fbe_raid_siots_get_parity_pos(siots_p);
        if (RAID_COND(siots_p->parity_pos == FBE_RAID_INVALID_PARITY_POSITION))
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: siots_p->parity_pos 0x%x == FBE_RAID_INVALID_PARITY_POSITION\n",
                                 siots_p->parity_pos);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        array_pos = (width - array_pos - 1) + parity_pos;
        if( array_pos >= width)
        {
            array_pos = array_pos - width;
        }
    }
    fbe_raid_geometry_get_element_size(raid_geometry_p, &element_size);

    /* find the first LBA on the element on first disk of unit this element
     * is part of the data stripe in which our LBA will reside.
     */
    return_lba = (disk_pba / element_size) * (data_disks * element_size);

    /* add on the leftover blocks in starting element to get the LBA
     * corresponding to the first disk in unit
     */
    return_lba += (disk_pba % element_size);

    /* now add elements to find the exact LBA on given disk 
     */
    return_lba += (element_size * array_pos);

    /* Update the return value
     */
    *lba_for_pba_p = return_lba;

    return FBE_STATUS_OK;
}
/**************************************************************************
    end of fbe_raid_convert_pba_to_lba()
 **************************************************************************/

/*!*********************************************************************
    fbe_parity_bva_report_sectors_reconstructed()
 **********************************************************************
 * @brief
         This function reports errors during BVA verify for 
 *       parity groups (like R5,R3 and R6)
 *
 * @param   siots_p - fbe_raid_siots_t pointer
 * @param   data_pba - PBA aginst which error will be logged
 * @param   err_bits - for extended status calculations
 * @param   array_pos - disk position in the unit.
 * @param   corrected - if it is FBE_TRUE means we are to reconstruct the 
 *                    sector with error.
 *
 * @return   NONE
 *
 * @author
 *  02/12/2008 - created MA
 *********************************************************************/
void fbe_parity_bva_report_sectors_reconstructed(fbe_raid_siots_t * siots_p, 
                                                 fbe_lba_t data_pba,
                                                 fbe_u32_t sunburst,
                                                 fbe_u32_t err_bits,
                                                 fbe_u32_t array_pos,
                                                 fbe_bool_t corrected,
                                                 fbe_u32_t blocks)
{
    if (corrected)
    {
        /* We are able to reconstruct the sector during BVA write
         * report it.
         */
        if ((sunburst == SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_PARITY_SECTOR) ||
            (sunburst == SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR))
        {
            sunburst = SEP_INFO_RAID_HOST_CORRECTING_WITH_NEW_DATA;
        }
        else
        {
            /* we might have corrected the coherency error too
             */
            sunburst = SEP_INFO_RAID_HOST_SECTOR_RECONSTRUCTED;
        }
        fbe_raid_send_unsol(siots_p,
                            sunburst,
                            array_pos,
                            data_pba,
                            blocks,
                            fbe_raid_error_info(err_bits),
                            fbe_raid_extra_info(siots_p));
    } /* end if (corrected) */
    else
    {
        /* we will proceed with the normal error reporting as
         * we are not able to reconstruct the given PBA during 
         * BVA write
         */
        fbe_raid_send_unsol(siots_p,
                            sunburst,
                            array_pos,
                            data_pba,
                            blocks,
                            fbe_raid_error_info(err_bits),
                            fbe_raid_extra_info(siots_p));

        /* if we are having uncorrectable sector then we need to invalidate
         * it like we do it in normal error reporting.
         */
        if ((sunburst == SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_PARITY_SECTOR) ||
            (sunburst == SEP_ERROR_CODE_RAID_HOST_UNCORRECTABLE_SECTOR))
        {
            sunburst = (array_pos == siots_p->parity_pos)
                        ? SEP_INFO_RAID_HOST_PARITY_SECTOR_RECONSTRUCTED
                        : SEP_ERROR_CODE_RAID_HOST_SECTOR_INVALIDATED;

            fbe_raid_send_unsol(siots_p,
                                sunburst,
                                array_pos,
                                data_pba,
                                blocks,
                                fbe_raid_error_info(err_bits),
                                fbe_raid_extra_info(siots_p));
        }
    } /* end else-if (corrected) */

    return;
} 
/*********************************************************
    end of fbe_parity_bva_report_sectors_reconstructed()
 ********************************************************/

/*!**************************************************************
 * fbe_parity_verify_get_verify_count()
 ****************************************************************
 * @brief
 *  Get the number of blocks we will verify in each pass of this
 *  state machine.
 *
 * @param siots_p - this i/o.
 * @param verify_count_p - output verify count.
 *
 * @return fbe_status_t   
 *
 * @author
 *  10/12/2009 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_parity_verify_get_verify_count(fbe_raid_siots_t *siots_p,
                                                fbe_block_count_t *verify_count_p)
{
    fbe_block_count_t verify_count;

    /* Decide the verify_size for this sub-verify
     * Verify Start is relative to each fru position.
     */
    if ( siots_p->algorithm == R5_VR ||
         siots_p->algorithm == R5_DEG_VR ||
         siots_p->algorithm == R5_DEG_RVR ||
         siots_p->algorithm == R5_RD_VR)
    {
        /* In normal verify, generating function cut the verify into strips.
         * Similarly for degraded verify we assume the size of the
         * region to be verified is small enough.  We do not break it up
         * further for degraded verify since this is normal I/O path
         * for degraded write.
         */
        verify_count = siots_p->xfer_count;
    }
    else
    {
        /* In recovery verify we start off with a strip/region mining mode so that we do
         * not allocate too many buffers. 
         */
        verify_count = siots_p->xfer_count;
#if 0
        fbe_payload_block_operation_opcode_t opcode;
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_REGION_MODE);

        fbe_raid_siots_get_opcode(siots_p, &opcode);
        if ( opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY )
        {
            /* This is a BVA verify. Since a media error is NOT expected,
             * we can use bigger strips. 
             */
            verify_count = siots_p->xfer_count;
        }
        else
        {
            verify_count = fbe_raid_siots_get_parity_count_for_region_mining(siots_p);
            if(verify_count== 0)
            { 
                fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: verify count is zero\n");
                return(FBE_RAID_STATE_STATUS_EXECUTING);
            }
        }
        if (verify_count == 0)
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "parity: verify count is zero\n");
            return FBE_STATUS_GENERIC_FAILURE;
        }
        siots_p->parity_count = verify_count;
#endif

        /* If we are here we are doing error recovery. Set the single error recovery flag
         * since we already have an error and do not want to short ciruit any of the error 
         * handling for efficiency. 
         */
        fbe_raid_siots_set_flag(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_ERROR_RECOVERY);
    }
    *verify_count_p = verify_count;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_parity_verify_get_verify_count()
 ******************************************/
#if 0
/*!**************************************************************
 * fbe_parity_verify_retry_errors()
 ****************************************************************
 * @brief
 *  Determine if retries are needed and retry now.
 *
 * @param siots_p - Current I/O            
 *
 * @return FBE_TRUE if errors are being retried.
 *        FBE_FALSE otherwise.
 *
 * @author
 *  4/26/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_parity_verify_retry_errors(fbe_raid_siots_t * const siots_p,
                                          fbe_raid_fru_eboard_t *eboard_p)
{
    fbe_u32_t error_count;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u16_t parity_disks;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_raid_geometry_get_parity_disks(raid_geometry_p, &parity_disks);
    error_count = eboard->hard_media_err_count + fbe_raid_siots_get_degraded_count(siots_p) +
                  eboard->retry_err_bitmap;

    if ((error_count > parity_disks) && 
        fbe_raid_siots_is_flag_set(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_ERROR_RECOVERY))
    {
        /* We are still trying to recover from a small number of errors.
         * But we have too many errors to recover from, 
         * so we reset all the hard or retry errored fruts and try again.
         */
        fbe_raid_siots_clear_flag(siots_p, FBE_RAID_SIOTS_FLAG_SINGLE_ERROR_RECOVERY);
        siots_p->wait_count = eboard->retry_err_count + eboard->hard_media_err_count;
        if (siots_p->wait_count == 0)
        {
            /* Not sure what happened here.
             */
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                 "no wait count for fru status: %d retry_bitmap: 0x%x hme_bitmap: 0x%x\n",
                                 siots_p->wait_count, eboard->retry_err_bitmap, eboard->hard_media_err_bitmap);
        }
        else
        {
            fbe_raid_fruts_eboard_save_err_counts(&eboard, siots_p);
            status = fbe_raid_fruts_for_each(eboard->retry_err_bitmap | eboard->hard_media_err_bitmap,
                                           read_fruts_p,
                                           fbe_raid_fruts_reset,
                                           (fbe_u64_t) siots_p->parity_count,
                                           (fbe_u64_t) FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ);
                    
            if (RAID_COND(status != FBE_STATUS_OK))
            {
                fbe_raid_siots_trace(siots_p, 
                                     FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                     "parity: failed with unexpected error: read_fruts_ptr 0x%p "
                                     "siots_p 0x%p. status = 0x%x\n",
                                     read_fruts_ptr, siots_p, status);
                return FBE_FALSE;
            }

            return FBE_TRUE;
        }
    }
    return FBE_FALSE;
}
/******************************************
 * end fbe_parity_verify_retry_errors()
 ******************************************/
#endif
/*****************************************
 * End of fbe_parity_verify_util.c
 *****************************************/

