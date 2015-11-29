/****************************************************************
 *  Copyright (C)  EMC Corporation 2006
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 ****************************************************************/

/***************************************************************************
 * xor_raid6_syndrome_verify.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions to implement the verify state of the EVENODD
 *  algorithm which is used for RAID 6 functionality.
 *
 * @notes
 *
 * @author
 *
 * 05/02/06 - Created. RCL
 *
 **************************************************************************/

#include "fbe_xor_raid6.h"
#include "fbe_xor_raid6_util.h"
#include "fbe_xor_epu_tracing.h"

/****************************************
 * static FUNCTIONS
 ****************************************/

static fbe_bool_t fbe_xor_raid6_determine_coh_err(fbe_xor_scratch_r6_t * const scratch_p,
                                       const fbe_u16_t data_pos[],
                                       fbe_u16_t * physical_pos,
                                       const fbe_u16_t width);

/***************************************************************************
 *  fbe_xor_eval_verify()
 ***************************************************************************
 * @brief
 *   This function verifies the coherency of the strip based on the syndromes
 *   that have already been calculated.  This algorithm will detect if there 
 *   is one symbol that has an error.  If an error is detected then the scratch
 *   state will be setup to reconstruct that sector.
 *
 * @param scratch_p        - ptr to the scratch database area.
 * @param eboard_p         - error board for marking different types of errors.
 * @param bitkey           - bitkey for stamps/error boards.
 * @param data_pos         - array of logical data positions.
 * @param parity_pos       - array of parity positions.
 * @param width            - width of strip being verified.
 * @param rebuild_pos[2]   - indexes to be rebuilt, first position should always 
 *                            be valid.
 * @param verify_result_p  - FBE_TRUE if the strip passes the verify.
 *                           FBE_FALSE if there is a sector that needs to be rebuilt.
 *
 * @return fbe_status_t
 
 *
 *  @notes
 *    A strip will pass the verify if the horizontal syndrome is all zeros and
 *    the every symbol in the diagonal syndrome is equal to the S value.  If
 *    the strip does not pass the verify the logical sector that contains the
 *    error can be determined by shifting and comparing the syndromes.  It is
 *    also assumed that the syndrome size is 17 symbols.  This allows the use
 *    of 0x1ffff as the all ones syndrome.
 *
 *  @author
 *    05/02/2006 - Created. RCL
 ***************************************************************************/
 fbe_status_t fbe_xor_eval_verify(fbe_xor_scratch_r6_t * const scratch_p,
                     fbe_xor_error_t * const eboard_p,
                     const fbe_u16_t bitkey[],
                     fbe_u16_t data_pos[],
                     const fbe_u16_t parity_pos[],
                     const fbe_u16_t width,
                     fbe_bool_t *verify_result_p)
{
    fbe_bool_t verify_results;    /* Did the data pass the verify. */
    fbe_bool_t chksum_verify_results = FBE_TRUE; /* Did the checksums pass the verify. */

    /* Does the row syndrome of the data show an error.
     */
    fbe_bool_t row_syndrome_good = FBE_TRUE; 
    /* Does the diagonal syndrome of the data show an error. 
     */
    fbe_bool_t diag_syndrome_good = FBE_TRUE; 
    /* Does the row syndrome of POC show an error. 
     */
    fbe_bool_t row_chksum_syndrome_good = FBE_FALSE; 
    /* Does the diagonal syndrome of POC show an error. 
     */
    fbe_bool_t diag_chksum_syndrome_good = FBE_FALSE; 
    fbe_u16_t row_index;

    fbe_u32_t * row_syndrome_ptr;
    fbe_u32_t * diag_syndrome_ptr;
    fbe_u32_t * s_value_ptr;
    fbe_status_t status;

    /* If we are in verify we shouldn't have any errors.
     */
    if (XOR_COND(scratch_p->fatal_key != 0x0))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "scratch_p->fatal_key 0x%x != 0x0\n",
                              scratch_p->fatal_key);

        return FBE_STATUS_GENERIC_FAILURE;
    }


    /* Mark the scratch so that we know we have gone through the verify path.
     */
    scratch_p->strip_verified = FBE_TRUE;

    /* Get the syndromes from the scratch.
     */
    row_syndrome_ptr = scratch_p->row_syndrome->syndrome_symbol->data_symbol;
    diag_syndrome_ptr = scratch_p->diag_syndrome->syndrome_symbol->data_symbol;

    /* Check if every symbol in the row syndrome is zero, and if every symbol
     * in the diagonal syndrome is the S value.
     */
    for (row_index = 0; row_index < FBE_XOR_EVENODD_M_VALUE; row_index++)
    {
        /* Since each diagonal symbol should be equal to the S value, just
         * assume the first diagonal symbol is it.  We can't use the calculated
         * S value yet because there may be an error in the parity blocks.
         */
        s_value_ptr = scratch_p->diag_syndrome->syndrome_symbol->data_symbol;

        XORLIB_REPEAT8( ((*row_syndrome_ptr++ != 0x0)                      
                      ? row_syndrome_good = FBE_FALSE                       
                      : 0 ,
                      ( (*diag_syndrome_ptr++ ^ *s_value_ptr++) != 0x0) 
                        ? diag_syndrome_good = FBE_FALSE                    
                        : 0 ) );

    }

    /* If the only problem is in the row syndrome then something is wrong with
     * the row parity, set it to be rebuilt.
     */
    if (!row_syndrome_good && diag_syndrome_good)
    {        
        /* Set the scratch to the initialized value because it will be set
         * correctly on the next pass through the loop.
         */
        scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_NOT_SET;
        fbe_xor_scratch_r6_add_error(scratch_p, bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]]);

        /* Set an uncorrectable coherency error so the block gets rebuilt.
         */
        eboard_p->u_coh_bitmap |= bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]];
        status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_coh_bitmap,
                                           FBE_XOR_ERR_TYPE_COH, 7, FBE_TRUE);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }

        verify_results = FBE_FALSE;
    }
    /* If the only problem is in the diagonal syndrome then something is wrong
     * with the diagonal parity, set it to be rebuilt.
     */
    else if (row_syndrome_good && !diag_syndrome_good)
    {
        /* Set the scratch to the initialized value because it will be set
         * correctly on the next pass through the loop.
         */
        scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_NOT_SET;
        fbe_xor_scratch_r6_add_error(scratch_p, bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]]);

        /* Set an uncorrectable coherency error so the block gets rebuilt.
         */
        eboard_p->u_coh_bitmap |= bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]];
        status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_coh_bitmap, 
                                           FBE_XOR_ERR_TYPE_COH, 8, FBE_TRUE);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }

        verify_results = FBE_FALSE;
    }
    /* If there is a problem with both then a data block is bad, lets find it
     * if we can and set it to be rebuilt.
     */
    else if (!row_syndrome_good && !diag_syndrome_good)
    {
        /* Disable the code that reconstructs a data position on coherency error since it is not 
         * able to determine that it is reconstructing the correct data. 
         */
#if 0
        fbe_u16_t phys_rebuild_pos;
        if (fbe_xor_raid6_determine_coh_err(scratch_p,
                                        data_pos,
                                        &phys_rebuild_pos,
                                        width))
        {
            fbe_xor_scratch_r6_add_error(scratch_p, bitkey[phys_rebuild_pos]);

            /* Set an uncorrectable coherency error so the block gets rebuilt.
             * Clear checksum errors for this position.
             */
            eboard_p->u_coh_bitmap |= bitkey[phys_rebuild_pos];
            eboard_p->crc_invalid_bitmap &= ~bitkey[phys_rebuild_pos];
            eboard_p->crc_raid_bitmap &= ~bitkey[phys_rebuild_pos];
            eboard_p->corrupt_crc_bitmap &= ~bitkey[phys_rebuild_pos];
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_coh_bitmap, 
                                               FBE_XOR_ERR_TYPE_COH, 9, FBE_TRUE);
            if (status != FBE_STATUS_OK) 
            {
                return status; 
            }
        }
        else
#endif
        {
            /* We must have run into more than one coherency error. Rebuilding all parity
             * will make the strip consistent and that is the only step we can take since
             * we can not determine where the error is.
             */
            fbe_xor_scratch_r6_add_error(scratch_p, bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]]);
            fbe_xor_scratch_r6_add_error(scratch_p, bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]]);

            /* Set an uncorrectable coherency error so the block gets rebuilt.
             */
            eboard_p->u_coh_bitmap |= bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]];
            eboard_p->u_coh_bitmap |= bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]];
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_coh_bitmap,
                                               FBE_XOR_ERR_TYPE_COH, 10, FBE_TRUE);
            if (status != FBE_STATUS_OK) 
            {
                return status; 
            }
        }

        /* Set the scratch to the initialized value because it will be set
         * correctly on the next pass through the loop.
         */
        scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_NOT_SET;
        verify_results = FBE_FALSE;
    }
    /* Everything passed so just return true.
     */
    else /* (row_syndrome_good && diag_syndrome_good) */
    {
        verify_results = FBE_TRUE;
    } /* end if else (row_syndrome_good && diag_syndrome_good) */

    /* Now check the coherency of the checksums.
     */
    /* If the row syndrome of the checksums is all zeroes it is good.
     */
    if (scratch_p->checksum_row_syndrome == 0x0)
    {
        row_chksum_syndrome_good = FBE_TRUE;
    }
    /* If the diagonal syndrome is all zeroes or all ones it is good.
     */
    if ((scratch_p->checksum_diag_syndrome == 0x0) ||
         (scratch_p->checksum_diag_syndrome == 0x1ffff))
    {
        diag_chksum_syndrome_good = FBE_TRUE;
    }

    /* If the only problem is in the row syndrome then something is wrong with
     * the row parity, set it to be rebuilt.
     */
    if (!row_chksum_syndrome_good && diag_chksum_syndrome_good)
    {        
        /* Set the scratch to the initialized value because it will be set
         * correctly on the next pass through the loop.
         */
        scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_NOT_SET;
        fbe_xor_scratch_r6_add_error(scratch_p, bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]]);        

        /* Set an uncorrectable poc coherency error so the block gets rebuilt.
         */
        eboard_p->u_poc_coh_bitmap |= bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]];
        status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_poc_coh_bitmap,
                                           FBE_XOR_ERR_TYPE_POC_COH, 9, FBE_TRUE);
        if (status != FBE_STATUS_OK) 
        {
            return status;
        }

        chksum_verify_results = FBE_FALSE;
    }
    /* If the only problem is in the diagonal syndrome then something is wrong
     * with the diagonal parity, set it to be rebuilt.
     */
    else if (row_chksum_syndrome_good && !diag_chksum_syndrome_good)
    {
        /* Set the scratch to the initialized value because it will be set
         * correctly on the next pass through the loop.
         */
        scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_NOT_SET;
        fbe_xor_scratch_r6_add_error(scratch_p, bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]]);        

        /* Set an uncorrectable poc coherency error so the block gets rebuilt.
         */
        eboard_p->u_poc_coh_bitmap |= bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]];
        status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_poc_coh_bitmap,
                                           FBE_XOR_ERR_TYPE_POC_COH, 10, FBE_TRUE);
        if (status != FBE_STATUS_OK) 
        { 
            return status;
        }

        chksum_verify_results = FBE_FALSE;
    }
    /* If there is a problem with both then a data block is bad, lets find it
     * if we can and set it to be rebuilt.
     */
    else if (!row_chksum_syndrome_good && !diag_chksum_syndrome_good)
    {
        /* Disable the code that reconstructs a data position on coherency error since it is not 
         * able to determine that it is reconstructing the correct data. 
         */
#if 0
        fbe_u16_t column_index;

        /* Lets find the error if we can, the same logic applies here as does in 
         * the function fbe_xor_raid6_determine_coh_err.  It is easier to perform
         * the checks right here since the symbol size of the parity of checksums
         * is one bit.
         */
        for (column_index = 0; column_index < FBE_XOR_EVENODD_M_VALUE; column_index++)
        {
            /* If the row syndrome equals the diagonal syndrome or the compliment
             * of the diagonal syndrome then we have found the error.
             */
            if ((scratch_p->checksum_row_syndrome == 
                 scratch_p->checksum_diag_syndrome) ||
                (scratch_p->checksum_row_syndrome == 
                 (scratch_p->checksum_diag_syndrome ^ 0x1ffff)))
            {
                /* Found the logical position of the error, now find the physical
                 * position so we can rebuild it.
                 */
                for (phys_rebuild_pos = 0; phys_rebuild_pos < width; phys_rebuild_pos++)
                {
                    if (data_pos[phys_rebuild_pos] == column_index)
                    {
                        /* Set the scratch to the initialized value because it will be set
                         * correctly on the next pass through the loop.
                         */
                        scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_NOT_SET;
                        fbe_xor_scratch_r6_add_error(scratch_p, bitkey[phys_rebuild_pos]); 

                        /* Set an uncorrectable poc coherency error so the block gets rebuilt.
                         * Clear any checksum error for this position.
                         */
                        eboard_p->u_poc_coh_bitmap |= bitkey[phys_rebuild_pos];
                        eboard_p->crc_invalid_bitmap &= ~bitkey[phys_rebuild_pos];
                        eboard_p->crc_raid_bitmap &= ~bitkey[phys_rebuild_pos];
                        eboard_p->corrupt_crc_bitmap &= ~bitkey[phys_rebuild_pos];                        
                        status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_poc_coh_bitmap,
                                                           FBE_XOR_ERR_TYPE_POC_COH, 11, FBE_TRUE);
                        if (status != FBE_STATUS_OK)
                        {
                            return status; 
                        }

                        chksum_verify_results = FBE_FALSE;
                        break;
                    }    /* end if data_pos */
                }    /* end for phys_rebuild_pos */
                break;
            }    /* end if syndromes match */

            /* Didn't find the error, cyclically shift the bits.
             */
            FBE_XOR_CYCLIC_SHIFT(scratch_p->checksum_row_syndrome);
        }    /* end for column_index */
#endif
        if (chksum_verify_results == FBE_TRUE)
        {
            /* We must have run into more than one coherency error. Rebuild all parity
             * will make the strip consistent and that is the only step we can take since
             * we can not determine where the error is.
             */
            scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_NOT_SET;
            fbe_xor_scratch_r6_add_error(scratch_p, bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]]);
            fbe_xor_scratch_r6_add_error(scratch_p, bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]]);

            /* Set an uncorrectable coherency error so the block gets rebuilt.
             */
            eboard_p->u_poc_coh_bitmap |= bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]];
            eboard_p->u_poc_coh_bitmap |= bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]];
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_poc_coh_bitmap,
                                               FBE_XOR_ERR_TYPE_POC_COH, 12, FBE_TRUE);
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }

            chksum_verify_results = FBE_FALSE;
        }
    }
    /* Everything passed so just return true.
     */
    else /* (row_chksum_syndrome_good && diag_chksum_syndrome_good) */
    {
        chksum_verify_results = FBE_TRUE;
    } /* end if else (row_chksum_syndrome_good && diag_chksum_syndrome_good) */

    if (verify_results && chksum_verify_results)
    {
        scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_DONE;
    }

    *verify_result_p = (verify_results & chksum_verify_results);
    return FBE_STATUS_OK;
} /* end fbe_xor_eval_verify() */

/***************************************************************************
 *  fbe_xor_raid6_determine_coh_err()
 ***************************************************************************
 * @brief
 *   This function determines the location of the coherency error if possible
 *   from the syndromes.
 *

 * @param scratch_p - ptr to the scratch database area.
 * @param data_pos - array of logical data positions.
 *   physical_pos   [O]  - the location of the error.  If the return from this
 *                         function is FBE_FALSE this value will be meaningless.
 *   width          [I]  - width of strip being verified.
 *
 *  @return
 *    FBE_TRUE if the physical location of the error is found.
 *    FBE_FALSE if the physical location of the error can not be found.
 *
 *  @notes
 *    The location of the error can be determined by cyclically shifting the 
 *    row syndrome until it exactly matches the diagonal syndrome XORed with 
 *    the S value.  The number of shifts required to get the match is the
 *    logical index of the block in error.
 *
 *    A cyclic shift is when an array [a[0], a[1],..., a[n-1], a[n]] is shifted
 *    to [a[n], a[0], a[1],..., a[n-1]].
 *
 *  @author
 *    05/02/2006 - Created. RCL
 ***************************************************************************/
fbe_bool_t fbe_xor_raid6_determine_coh_err(fbe_xor_scratch_r6_t * const scratch_p,
                                 const fbe_u16_t data_pos[],
                                 fbe_u16_t * physical_pos,
                                 const fbe_u16_t width)
{
    fbe_bool_t error_found;
    fbe_u16_t row_index;
    fbe_u16_t column_index;
    fbe_xor_r6_symbol_size_t syndrome_symbol_holder;

    fbe_u32_t * row_syndrome_ptr;
    fbe_u32_t * diag_syndrome_ptr;
    fbe_u32_t * s_value_ptr;

    for (column_index = 0; column_index < FBE_XOR_EVENODD_M_VALUE; column_index++)
    {
        /* Get the syndromes from the scratch.
         */
        row_syndrome_ptr = scratch_p->row_syndrome->syndrome_symbol->data_symbol;
        diag_syndrome_ptr = scratch_p->diag_syndrome->syndrome_symbol->data_symbol;

        /* Init the return value to FBE_TRUE and change it when we find the first 
         * error.
         */
        error_found = FBE_TRUE;
        
        /* Compare the syndromes.
         */
        for (row_index = 0; row_index < FBE_XOR_EVENODD_M_VALUE; row_index++)
        {
            /* Reset the pointer to the top of the S value.
             */
            s_value_ptr = scratch_p->s_value->data_symbol;

            /* Check that the row syndrome equals the diagonal syndrome XOR S value.
             * Since we are looking for a complete match, the first differenc tells
             * us this is not what we are looking for.
             */
            XORLIB_REPEAT8(
                ( (*row_syndrome_ptr++ != (*diag_syndrome_ptr++ ^ *s_value_ptr++))  
                             ? error_found = FBE_FALSE                                  
                             : 0 ) );

            if (error_found == FBE_FALSE)
            {
                /* We found a mismatch in the syndromes so lets break out of this
                 * loop and try the next shift.
                 */
                break;
            }
        } /* end for row_index */

        /* We made it through the syndromes and did not find a mismatch.  Therefore
         * they are the same and this is what we are looking for.
         */
        if (error_found == FBE_TRUE)
        {
            break;
        }

        /* Cyclically shift the row syndrome and try again.
         */
        syndrome_symbol_holder = 
            scratch_p->row_syndrome->syndrome_symbol[FBE_XOR_EVENODD_M_VALUE - 1];

        for (row_index = FBE_XOR_EVENODD_M_VALUE - 1; row_index > 0; row_index--)
        {
            scratch_p->row_syndrome->syndrome_symbol[row_index] =
                scratch_p->row_syndrome->syndrome_symbol[row_index - 1];
        }

        scratch_p->row_syndrome->syndrome_symbol[0] = syndrome_symbol_holder;

    } /* end for column_index */

    /* If we found an error determine its physical position.
     */
    if (error_found == FBE_TRUE)
    {
        for (*physical_pos = 0; *physical_pos < width; (*physical_pos)++)
        {
            if (data_pos[*physical_pos] == column_index)
            {
                break;
            }
        }
    }

    return(error_found);
}
/* end xor_raid6_syndrome_verify.c */
