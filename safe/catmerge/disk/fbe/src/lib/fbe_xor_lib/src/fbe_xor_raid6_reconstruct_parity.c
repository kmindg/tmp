/****************************************************************
 *  Copyright (C)  EMC Corporation 2006
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 ****************************************************************/

/***************************************************************************
 * xor_raid6_reconstruct_parity.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions to implement the reconstruct parity state of 
 *  the EVENODD algorithm which is used for RAID 6 functionality.
 *
 * @notes
 *
 * @author
 *
 * 04/10/06 - Created. RCL
 *
 **************************************************************************/

#include "fbe_xor_raid6.h"
#include "fbe_xor_raid6_util.h"
#include "fbe_xor_epu_tracing.h"


/* This fn will update the scratch error tracking, the modified bit in the 
 * eboard, and set the scratch state to done. 
 */
static fbe_status_t fbe_xor_reconstruct_parity_clean_up(fbe_xor_scratch_r6_t * m_scratch, 
                                                        fbe_u16_t m_bitkey,
                                                        fbe_xor_error_t * eboard_p)
{
    fbe_status_t status;
    status = fbe_xor_scratch_r6_remove_error(m_scratch, m_bitkey);
    if (status != FBE_STATUS_OK) { return status; }
    eboard_p->m_bitmap |= m_bitkey;
    m_scratch->scratch_state = FBE_XOR_SCRATCH_R6_STATE_DONE;
    return FBE_STATUS_OK;
}

/***************************************************************************
 *  fbe_xor_reconstruct_parity_double_check()
 ***************************************************************************
 * @brief
 *   This routine checks recalculated parity against the parity on disk to
 *  ensure that they match.  If they don't the new parity is put there.
 *
 * @param scratch_p - ptr to the scratch database area.
 * @param eboard_p - error board for marking different types of errors.
 * @param parity_index_bitmask - bitmap of the parity position being checked.
 * @param checksum - recalculated checksum of the parity position being checked.
 *   parity_pos[], [I] - The array of parity positions.
 *   parity_index [I] - parity index of the parity position being checked.
 *   sector_p[], [I]  - pointer to data sectors.
 *
 * @return fbe_status_t
 * 
 *
 *  @author
 *    04/21/2006 - Created. RCL
 ***************************************************************************/
static __forceinline fbe_status_t fbe_xor_reconstruct_parity_double_check(fbe_xor_scratch_r6_t * const scratch_p,
                                                           fbe_xor_error_t * const eboard_p,
                                                           const fbe_u16_t parity_index_bitmask,
                                                           fbe_u16_t checksum,
                                                           const fbe_u16_t parity_pos[],
                                                           const fbe_u16_t parity_index,
                                                           fbe_sector_t * const sector_p[])
{
    fbe_status_t status;
    fbe_bool_t existing_parity_is_good = FBE_TRUE;
    fbe_u32_t * existing_parity_ptr;
    fbe_u32_t * parity_ptr;
    fbe_u16_t row_index;
    fbe_sector_t * const existing_parity_sector_ptr = sector_p[parity_pos[parity_index]];
    fbe_sector_t *parity_sector_ptr = (fbe_sector_t*)
        scratch_p->diag_syndrome->syndrome_symbol[0].data_symbol;
    fbe_u16_t other_parity_index = (parity_index == FBE_XOR_R6_PARITY_POSITION_ROW) ?
        FBE_XOR_R6_PARITY_POSITION_DIAG : FBE_XOR_R6_PARITY_POSITION_ROW;
    
    /* The parity is not being rebuilt but we just recalculated it so lets 
     * double check that it is correct.
     */
    parity_ptr = scratch_p->diag_syndrome->syndrome_symbol[0].data_symbol;
    existing_parity_ptr = (fbe_u32_t *)existing_parity_sector_ptr;

    XORLIB_PASSES16(row_index, FBE_WORDS_PER_BLOCK)
    {
        XORLIB_REPEAT8(((*parity_ptr++ != *existing_parity_ptr++) 
                     ? existing_parity_is_good = FBE_FALSE 
                     : 0 ));
    }

    if (existing_parity_is_good == FBE_FALSE)
    {
        /* There was a mismatch with the parity calculated and the one on
         * the disk.  Lets use the new one since we know it matches the
         * data we have.
         */
        memcpy(existing_parity_sector_ptr, 
               scratch_p->diag_syndrome, 
               (FBE_XOR_R6_SYMBOLS_PER_SECTOR * sizeof(fbe_xor_r6_symbol_size_t)));

        existing_parity_sector_ptr->crc = checksum;

        /* Mark the error in the eboard so that we know this happened.
         */
        eboard_p->c_coh_bitmap |= parity_index_bitmask;
        status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_coh_bitmap,
                                           FBE_XOR_ERR_TYPE_COH, 5, FBE_FALSE);
        if (status != FBE_STATUS_OK) { return status; }

        /* Also reconstruct the POC from the POC we calculated.
         */
        existing_parity_sector_ptr->lba_stamp = parity_sector_ptr->lba_stamp;

        if ( existing_parity_sector_ptr->time_stamp != 
             sector_p[parity_pos[other_parity_index]]->time_stamp )
        {
            /* If we had the FBE_SECTOR_INITIAL_TSTAMP, then it means
             * we had the initial timestamp and are transitioning from
             * a zeroed stripe to a non-zeroed stripe.
             */
            existing_parity_sector_ptr->time_stamp = 
                sector_p[parity_pos[other_parity_index]]->time_stamp;
        }
    }
    else if ( existing_parity_sector_ptr->lba_stamp != parity_sector_ptr->lba_stamp )
    {
        /* existing_parity_is_good == FBE_TRUE
         * And the POC does not match the one we recalculated.
         *
         * Mark the error in the eboard so that we know this happened.
         */
        eboard_p->c_poc_coh_bitmap |= parity_index_bitmask;
        status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_poc_coh_bitmap,
                              FBE_XOR_ERR_TYPE_POC_COH, 7, FBE_FALSE);
        if (status != FBE_STATUS_OK) { return status; }

        /* Also reconstruct the POC from the POC we calculated.
         */
        existing_parity_sector_ptr->lba_stamp = parity_sector_ptr->lba_stamp;

        if ( existing_parity_sector_ptr->time_stamp != 
             sector_p[parity_pos[other_parity_index]]->time_stamp )
        {
            /* If we had the FBE_SECTOR_INITIAL_TSTAMP, then it means
             * we had the initial timestamp and are transitioning from
             * a zeroed stripe to a non-zeroed stripe.
             */
            existing_parity_sector_ptr->time_stamp = 
                sector_p[parity_pos[other_parity_index]]->time_stamp;
        }
    }
    return FBE_STATUS_OK;
}
/* End fbe_xor_reconstruct_parity_double_check */

/***************************************************************************
 *  fbe_xor_calc_csum_and_reconstruct_parity()
 ***************************************************************************
 * @brief
 *   This routine calculates the partial rebuild of the missing disk which is
 *   a parity disk.  If this is the first disk to be processed in the strip 
 *   then some initialization is required.  While the partial rebuild 
 *   calculations are being performed the checksum will be calculated for 
 *   the sector.
 *

 * @param scratch_p - ptr to the scratch database area.
 * @param data_blk_p - ptr to data sector.
 * @param column_index - the logical position of the drive within the strip.
 * @param rebuild_pos - the rebuild positions, there could be two.  This
 *                        value will be the logical index of the parity 
 *                        positions.
 *   parity_bitmap  [I] - bitmap of the parity positions.
 * @param uncooked_checksum_p [O] - uncooked checksum
 *
 *  @return fbe_status_t
 *    
 *
 *  @author
 *    04/21/2006 - Created. RCL
 ***************************************************************************/
fbe_status_t fbe_xor_calc_csum_and_reconstruct_parity(fbe_xor_scratch_r6_t * const scratch_p,
                                             fbe_sector_t * const data_blk_p,
                                             const fbe_u32_t column_index,
                                             const fbe_u16_t * rebuild_pos,
                                             const fbe_u16_t parity_bitmap,
                                             fbe_u32_t *uncooked_checksum_p)
{
    fbe_u32_t checksum = 0x0;
    fbe_u32_t * row_parity_ptr;
    fbe_u32_t * diag_parity_ptr;
    
    /* Only parity disks should be dead if we are in this state.
     */

    if (XOR_COND((scratch_p->fatal_key & ~parity_bitmap) != 0 ))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "(scratch_p->fatal_key 0x%x & ~parity_bitmap 0x%x) != 0 \n",
                              scratch_p->fatal_key,
                              parity_bitmap);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Determine which scratch buffer to use for the row parity recalculation.
     */
    if (rebuild_pos[0] == FBE_XOR_LOGICAL_ROW_PARITY_POSITION)
    {
        /* The first rebuild position is the row parity.
         */
        row_parity_ptr = scratch_p->fatal_blk[0]->data_word;
    }
    else if (rebuild_pos[1] == FBE_XOR_LOGICAL_ROW_PARITY_POSITION)
    {
        /* The second rebuild position is the row parity.
         */
        row_parity_ptr = scratch_p->fatal_blk[1]->data_word;
    }
    else
    {
        /* The row parity is not being rebuilt so just use some dummy space for
         * calculation and it will be discarded later.
         */
        row_parity_ptr = scratch_p->diag_syndrome->syndrome_symbol[0].data_symbol;
    }

    /* Determine which scratch buffer to use for the diagonal parity recalculation.
     */
    if (rebuild_pos[0] == FBE_XOR_LOGICAL_DIAG_PARITY_POSITION)
    {
        /* The first rebuild position is the diagonal parity.
         */
        diag_parity_ptr = scratch_p->fatal_blk[0]->data_word;
    }
    else if (rebuild_pos[1] == FBE_XOR_LOGICAL_DIAG_PARITY_POSITION)
    {
        /* The second rebuild position is the diagonal parity.
         */
        diag_parity_ptr = scratch_p->fatal_blk[1]->data_word;
    }
    else
    {
        /* The diagonal parity is not being rebuilt so just use some dummy space for
         * calculation and it will be discarded later.
         */
        diag_parity_ptr = scratch_p->diag_syndrome->syndrome_symbol[0].data_symbol;
    }

    /* Update the scratch with the partial parity calculations and the partial
     * parity calculations for the parity of checksums.
     */
    if (scratch_p->initialized)
    {
        checksum = xorlib_calc_csum_and_update_r6_parity(data_blk_p->data_word,
                                                      row_parity_ptr,
                                                      diag_parity_ptr,
                                                      column_index);


        fbe_xor_calc_parity_of_csum_update(&(data_blk_p->crc),
                                       &(((fbe_sector_t *)row_parity_ptr)->lba_stamp),
                                       &(((fbe_sector_t *)diag_parity_ptr)->lba_stamp),
                                       column_index);

    }
    else
    {
        checksum = xorlib_calc_csum_and_init_r6_parity(data_blk_p->data_word,
                                                    row_parity_ptr,
                                                    diag_parity_ptr,
                                                    column_index);

        fbe_xor_calc_parity_of_csum_init(&(data_blk_p->crc),
                                     &(((fbe_sector_t *)row_parity_ptr)->lba_stamp),
                                     &(((fbe_sector_t *)diag_parity_ptr)->lba_stamp),
                                     column_index);

        scratch_p->initialized = FBE_TRUE;
   
    }

    /* Return the uncooked checksum.
     */
    *uncooked_checksum_p = checksum;
    return FBE_STATUS_OK;


} /* end fbe_xor_calc_csum_and_reconstruct_parity() */

/***************************************************************************
 *  fbe_xor_eval_reconstruct_parity()
 ***************************************************************************
 * @brief
 *   This funciton calculates the checksums for the rebuilt parity positions
 *   and cleans up the eboard.
 *

 * @param scratch_p - ptr to the scratch database area.
 *   parity_pos[],  [I]  - parity positions
 * @param eboard_p - error board for marking different types of errors.
 * @param bitkey - bitkey for stamps/error boards.
 *   rebuild_pos[2],[I]  - indexes to be rebuilt, first position should always 
 *                         be valid.
 *   sector_p[],    [I]  - pointer to data sectors.
 * @param reconstruct_result_p [O] -  FBE_TRUE if the reconstruct is successful.
 *                                    FBE_FALSE if the reconstruct fails.
 *
 * @return fbe_status_t
 *
 *
 * @author
 *    04/21/2006 - Created. RCL
 ***************************************************************************/
fbe_status_t fbe_xor_eval_reconstruct_parity(fbe_xor_scratch_r6_t * const scratch_p,
                                 const fbe_u16_t parity_pos[],
                                 fbe_xor_error_t * const eboard_p,
                                 const fbe_u16_t bitkey[],
                                 const fbe_u16_t rebuild_pos[],
                                 fbe_sector_t * const sector_p[],
                                 fbe_bool_t *reconstruct_result_p)
{
    fbe_status_t status;
    fbe_u32_t * row_parity_ptr;
    fbe_u32_t * diag_parity_ptr;
    fbe_u32_t row_checksum = 0x0;
    fbe_u32_t diag_checksum = 0x0;
    fbe_u16_t row_index;
    fbe_bool_t return_value = FBE_FALSE;

    /* Only parity disks should be dead if we are in this state.
     */
    if (XOR_COND(((scratch_p->fatal_key & bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]]) == 0) && 
                 ((scratch_p->fatal_key & bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]])== 0)))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "unexpected error : scratch_p->fatal_key = 0x%x, "
                              "bitkey[parity_pos[%d]] = 0x%x, bitkey[parity_pos[%d]] = 0x%x\n",
                              scratch_p->fatal_key,
                              bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]],
                              FBE_XOR_R6_PARITY_POSITION_ROW,
                              bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]],
                              FBE_XOR_R6_PARITY_POSITION_DIAG);

        return FBE_STATUS_GENERIC_FAILURE;
    }


    /* Determine which scratch buffer to use for the row parity recalculation.
     */
    if (rebuild_pos[0] == parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW])
    {
        /* The first rebuild position is the row parity.
         */
        row_parity_ptr = scratch_p->fatal_blk[0]->data_word;
    }
    else if (rebuild_pos[1] == parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW])
    {
        /* The second rebuild position is the row parity.
         */
        row_parity_ptr = scratch_p->fatal_blk[1]->data_word;
    }
    else
    {
        /* The row parity is not being rebuilt so just use some dummy space for
         * calculation and it will be discarded later.
         */
        row_parity_ptr = scratch_p->diag_syndrome->syndrome_symbol[0].data_symbol;
    }

    /* Determine which scratch buffer to use for the diagonal parity recalculation.
     */
    if (rebuild_pos[0] == parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG])
    {
        /* The first rebuild position is the diagonal parity.
         */
        diag_parity_ptr = scratch_p->fatal_blk[0]->data_word;
    }
    else if (rebuild_pos[1] == parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG])
    {
        /* The second rebuild position is the diagonal parity.
         */
        diag_parity_ptr = scratch_p->fatal_blk[1]->data_word;
    }
    else
    {
        /* The diagonal parity is not being rebuilt so just use some dummy space for
         * calculation and it will be discarded later.
         */
        diag_parity_ptr = scratch_p->diag_syndrome->syndrome_symbol[0].data_symbol;
    }

    /* Calculate the checksums.
     */
    XORLIB_PASSES16(row_index, FBE_WORDS_PER_BLOCK)
    {
        XORLIB_REPEAT8( (row_checksum ^= *row_parity_ptr++, 
                      diag_checksum ^= *diag_parity_ptr++) );
    }

    /* Cleanup the eboard.
     */
    fbe_xor_raid6_correct_all(eboard_p);

    /* Cook the checksums calculated.
     */
    row_checksum = xorlib_cook_csum(row_checksum, (fbe_u32_t)scratch_p->seed);
    diag_checksum = xorlib_cook_csum(diag_checksum, (fbe_u32_t)scratch_p->seed);

    /* Determine where to put the reconstructed row parity.
     */
    if (rebuild_pos[0] == parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW])
    {
        /* The first rebuild position is the row parity update the checksum.
         * Remove the errror, set the modified bit, set the scratch state to
         * done, and return that the rebuild was successful.
         */
        scratch_p->fatal_blk[0]->crc = row_checksum;
        return_value = FBE_TRUE;
        status = fbe_xor_reconstruct_parity_clean_up(scratch_p,
                                    bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]],
                                    eboard_p);
        if (status != FBE_STATUS_OK) { return status; }
    }
    else if (rebuild_pos[1] == parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW])
    {
        /* The second rebuild position is the row parity update the checksum.
         * Remove the errror, set the modified bit, set the scratch state to
         * done, and return that the rebuild was successful.
         */
        scratch_p->fatal_blk[1]->crc = row_checksum;
        return_value = FBE_TRUE;
        status = fbe_xor_reconstruct_parity_clean_up(scratch_p,
                                    bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]],
                                    eboard_p);
        if (status != FBE_STATUS_OK) { return status; }
    }
    else
    {
        /* The row parity is not being rebuilt but we just recalculated it so
         * lets double check that it is correct.
         */
        status = fbe_xor_reconstruct_parity_double_check(scratch_p,
                                                         eboard_p, 
                                                         bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]], 
                                                         row_checksum,
                                                         parity_pos,
                                                         FBE_XOR_R6_PARITY_POSITION_ROW, 
                                                         sector_p);
         if (status != FBE_STATUS_OK) { return status; }
    }

    /* Determine where to put the reconstructed diagonal parity.
     */
    if (rebuild_pos[0] == parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG])
    {
        /* The first rebuild position is the diagonal parity update the checksum.
         * Remove the errror, set the modified bit, set the scratch state to
         * done, and return that the rebuild was successful.
         */
        scratch_p->fatal_blk[0]->crc = diag_checksum;
        return_value = FBE_TRUE;
        status = fbe_xor_reconstruct_parity_clean_up(scratch_p,
                                    bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]],
                                    eboard_p);
        if (status != FBE_STATUS_OK) { return status; }
    }
    else if (rebuild_pos[1] == parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG])
    {
        /* The second rebuild position is the diagonal parity update the checksum.
         * Remove the errror, set the modified bit, set the scratch state to
         * done, and return that the rebuild was successful.
         */
        scratch_p->fatal_blk[1]->crc = diag_checksum;
        return_value = FBE_TRUE;
        status = fbe_xor_reconstruct_parity_clean_up(scratch_p,
                                    bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]],
                                    eboard_p);
        if (status != FBE_STATUS_OK) { return status; }
    }
    else
    {
        /* The diagonal parity is not being rebuilt but we just recalculated it so
         * lets double check that it is correct.
         */
        status = fbe_xor_reconstruct_parity_double_check(scratch_p,
                                                         eboard_p, 
                                                         bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]], 
                                                         diag_checksum,
                                                         parity_pos,
                                                         FBE_XOR_R6_PARITY_POSITION_DIAG, 
                                                         sector_p);
        if (status != FBE_STATUS_OK) { return status; }
    }
    *reconstruct_result_p = return_value;
    return FBE_STATUS_OK;
} /* fbe_xor_eval_reconstruct_parity() */

/* end xor_raid6_reconstruct_parity.c */
