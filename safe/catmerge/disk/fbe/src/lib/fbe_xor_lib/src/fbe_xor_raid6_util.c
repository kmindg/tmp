/****************************************************************
 *  Copyright (C)  EMC Corporation 2006
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 ****************************************************************/

/***************************************************************************
 * xor_raid6_util.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions to implement the EVENODD algorithm
 *  which is used for RAID 6 functionality.
 *
 * @notes
 *
 * @author
 *
 * 03/16/06 - Created. RCL
 *
 **************************************************************************/

#include "fbe_xor_raid6.h"
#include "fbe_xor_raid6_util.h"
#include "fbe_xor_sector_history.h"
#include "fbe_xor_epu_tracing.h"

/* Globals need for eval_parity_unit_r6.
 */
fbe_xor_globals_r6_t fbe_xor_raid6_globals;

/* Global defining strings for each error for tracing.
 */
const char *fbe_xor_error_type_strings[] = { FBE_XOR_ERROR_TYPE_STRINGS };

/***************************************************************************
 *  fbe_xor_get_error_type_string
 ***************************************************************************
 * @brief
 *  This function returns a string representation of the input error type.
 *

 * @param err_type - Error type to return a string for.
 *      
 * @return
 *  The String representing the input error type.
 *
 * @notes
 *
 * @author
 *  02/01/08 - Created. Rob Foley
 ***************************************************************************/

const char * fbe_xor_get_error_type_string(const fbe_xor_error_type_t err_type)
{
    const char *err_type_string_p;

    if (err_type > FBE_XOR_ERR_TYPE_MAX)
    {
        /* This error type is invalid and does not have
         * any corresponding string in xor_error_type_strings.
         */
        return "Invalid xor error type";
    }

    /* Get our error string from the global array of error type strings.
     */
    err_type_string_p = fbe_xor_error_type_strings[err_type];
    
    return err_type_string_p;
}
/* end fbe_xor_get_error_type_string() */

/***************************************************************************
 *  fbe_xor_calc_csum_and_init_r6_parity_non_mmx()
 ***************************************************************************
 * @brief
 *  This function calculates a "raw" checksum (unseeded, unfolded) for
 *  data within a sector using the "classic" checksum algorithm; this is
 *  the first pass over the data so the parity blocks are initialized
 *  based on the data.
 *

 * @param src_ptr - ptr to first word of source sector data
 * @param tgt_row_ptr - ptr to first word of target sector of row parity
 * @param tgt_diagonal_ptr - ptr to first word of target sector of diagonal 
 *                         parity
 * @param column_index - The index of the disk passed in.
 *      
 * @return
 *  The "raw" checksum for the data in the sector.
 *
 * @notes
 *
 * @author
 *  03/16/06 - Created. RCL
 ***************************************************************************/

fbe_u32_t fbe_xor_calc_csum_and_init_r6_parity_non_mmx(const fbe_u32_t * src_ptr,
                                         fbe_u32_t * tgt_row_ptr,
                                         fbe_u32_t * tgt_diagonal_ptr,
                                         const fbe_u32_t column_index)
{
    fbe_u32_t checksum = 0x0;
    fbe_u16_t row_index;
    fbe_xor_r6_symbol_size_t s_value;
    fbe_u32_t * s_value_ptr;

    /* Holds the pointer to the top diagonal parity block.
     */
    fbe_xor_r6_symbol_size_t * tgt_diagonal_ptr_holder;

    tgt_diagonal_ptr_holder = (fbe_xor_r6_symbol_size_t*) tgt_diagonal_ptr;
    s_value_ptr = (fbe_u32_t*) &s_value;

    /* Find the component of the S value in this particular column.  There is
     * no component of the S value contained in the first column. 
     */
    if (column_index == 0)
    {
        /* Since this is the first column there is no s value component, so we
         * will just initialize the s value to zero for each of the words that
         * make up this symbol.
         */
        XORLIB_REPEAT8((*s_value_ptr++ = 0x0));
    }
    else
    {
        /* Since this is not the first column find the S value component which
         * is the ( FBE_XOR_EVENODD_M_VALUE - column_index - 1 ) symbol.
         */
        s_value = *(((fbe_xor_r6_symbol_size_t*)src_ptr) + 
                    ( FBE_XOR_EVENODD_M_VALUE - column_index - 1 ) );
    }

    /* For each data symbol do three things.
     * 1) Update the checksum.
     * 2) Set the correct diagonal parity symbol.
     * 3) Set the correct row parity symbol.
     */
    XORLIB_PASSES16(row_index, FBE_WORDS_PER_BLOCK)
    {
        /* This loop will process one symbol.  First find the correct position
         * in the diagonal parity block for this symbol and find the top of the
         * S value.
         */
        tgt_diagonal_ptr = &((tgt_diagonal_ptr_holder + 
                                     FBE_XOR_MOD_VALUE_M((row_index + column_index),
                                                      column_index-1))->data_symbol[0]);

        s_value_ptr = (fbe_u32_t*) &s_value;

        if( ((row_index + column_index) % FBE_XOR_EVENODD_M_VALUE) !=
            (FBE_XOR_EVENODD_M_VALUE - 1))
        {
            XORLIB_REPEAT8((checksum ^= *src_ptr, 
                        *tgt_diagonal_ptr++ = *src_ptr ^ *s_value_ptr++, 
                        *tgt_row_ptr++ = *src_ptr++));
        }
        else
        {
            /* If the symbol is the S value component then the source data 
             * is not added.
             */
            XORLIB_REPEAT8((checksum ^= *src_ptr, 
                        *tgt_diagonal_ptr++ = *s_value_ptr++, 
                        *tgt_row_ptr++ = *src_ptr++));

        }
    }

    return (checksum);
} /* end fbe_xor_calc_csum_and_init_r6_parity_non_mmx */

/***************************************************************************
 * fbe_xor_calc_csum_and_update_r6_parity_non_mmx()
 ***************************************************************************
 * @brief
 *  This function calculates a "raw" checksum (unseeded, unfolded) for
 *      data within a sector using the "classic" checksum algorithm; the
 *  parity values are updated based on the data.
 *

 * @param src_ptr - ptr to first word of source sector data
 * @param tgt_row_ptr - ptr to first word of target sector of row parity
 * @param tgt_diagonal_ptr - ptr to first word of target sector of diagonal
 *                         parity
 * @param column_index - The index of the disk passed in.
 *
 * @return
 *  The "raw" checksum for the data in the sector.
 *
 * @notes
 *
 * @author
 *  03/16/06 - Created. RCL
 ***************************************************************************/

fbe_u32_t fbe_xor_calc_csum_and_update_r6_parity_non_mmx(const fbe_u32_t * src_ptr,
                                           fbe_u32_t * tgt_row_ptr,
                                           fbe_u32_t * tgt_diagonal_ptr,
                                           const fbe_u32_t column_index)
{
    fbe_u32_t checksum = 0x0;
    fbe_u16_t row_index;
    fbe_xor_r6_symbol_size_t s_value;
    fbe_u32_t * s_value_ptr;

    /* Holds the pointer to the top diagonal parity block.
     */
    fbe_xor_r6_symbol_size_t * tgt_diagonal_ptr_holder;

    tgt_diagonal_ptr_holder = (fbe_xor_r6_symbol_size_t*) tgt_diagonal_ptr;
    s_value_ptr = (fbe_u32_t*) &s_value;

    /* Find the component of the S value in this particular column.  There is
     * no component of the S value contained in the first column. 
     */
    if (column_index == 0)
    {
        /* Since this is the first column there is no s value component, so we
         * will just initialize the s value to zero for each of the words that
         * make up this symbol.
         */
        XORLIB_REPEAT8((*s_value_ptr++ = 0x0));
    }
    else
    {
        /* Since this is not the first column find the S value component which
         * is the ( FBE_XOR_EVENODD_M_VALUE - column_index - 1 ) symbol.
         */
        s_value = *(((fbe_xor_r6_symbol_size_t*)src_ptr) + 
                     ( FBE_XOR_EVENODD_M_VALUE - column_index - 1 ) );
    }

    /* For each data symbol do three things.
     * 1) Update the checksum.
     * 2) Update the correct diagonal parity symbol.
     * 3) Update the correct row parity symbol.
     */
    XORLIB_PASSES16(row_index, FBE_WORDS_PER_BLOCK)
    {
        /* This loop will process one symbol.  First find the correct position
         * in the diagonal parity block for this symbol and find the top of the
         * S value.
         */
        tgt_diagonal_ptr = (fbe_u32_t*) (tgt_diagonal_ptr_holder + 
                                  FBE_XOR_MOD_VALUE_M((row_index + column_index), 
                                                   column_index-1));

        s_value_ptr = (fbe_u32_t*) &s_value;

        if( ((row_index + column_index) % FBE_XOR_EVENODD_M_VALUE) !=
            (FBE_XOR_EVENODD_M_VALUE - 1) )
        {
            XORLIB_REPEAT8((checksum ^= *src_ptr, 
                        *tgt_diagonal_ptr++ ^= *src_ptr ^ *s_value_ptr++, 
                        *tgt_row_ptr++ ^= *src_ptr++));
        }
        else
        {
            /* If the symbol is the S value component then the source data is 
             * not added.
             */
            XORLIB_REPEAT8((checksum ^= *src_ptr, 
                        *tgt_diagonal_ptr++ ^= *s_value_ptr++, 
                        *tgt_row_ptr++ ^= *src_ptr++));

        }
    }

    return (checksum);
} /* end fbe_xor_calc_csum_and_update_r6_parity_non_mmx */

/***************************************************************************
 * fbe_xor_calc_parity_of_csum_init()
 ***************************************************************************
 * @brief
 *  This function initializes and calculates the row and diagonal parity
 *  of the 16 bit checksum value.
 *

 * @param src_ptr - ptr to source data
 * @param tgt_row_ptr - ptr to row parity
 * @param tgt_diagonal_ptr - ptr to diagonal parity
 * @param column_index - The index of the disk passed in.
 *      
 * @return
 *  None.
 *
 * @notes
 *
 * @author
 *  03/16/06 - Created. RCL
 ***************************************************************************/

void fbe_xor_calc_parity_of_csum_init(const fbe_u16_t * const src_ptr,
                                  fbe_u16_t * const tgt_row_ptr,
                                  fbe_u16_t * const tgt_diagonal_ptr,
                                  const fbe_u32_t column_index)
{

    fbe_u16_t s_value = 0x0;    

    /* If the S value is set in this column set the S value to all ones.
     * This will allow us to xor in one S value to the whole diagonal
     * parity column instead of 16 individual bits. The first column does 
     * not contain an S value component.
     */
    if ((column_index != 0) && 
        (FBE_XOR_IS_BITSET(*src_ptr, column_index - 1)))
    {
        s_value = 0xffff;
    }

    /* Since this is the first time through set the parity data to the data
     * symbol instead of XORing it.
     */
    *tgt_row_ptr = *src_ptr;

    /* Since this is the first time through set the parity data to the data
     * correct diagonal symbol instead of XORing it.
     */
    *tgt_diagonal_ptr = FBE_XOR_EVENODD_DIAGONAL_MANGLE(*src_ptr, column_index) 
                         ^ s_value;

    return;
} /* end fbe_xor_calc_parity_of_csum_init */

/***************************************************************************
 * fbe_xor_calc_parity_of_csum_update()
 ***************************************************************************
 * @brief
 *  This function calculates the row and diagonal parity of the 16 bit
 *  checksum value.
 *

 * @param src_ptr - ptr to source data
 * @param tgt_row_ptr - ptr to row parity
 * @param tgt_diagonal_ptr - ptr to diagonal parity
 * @param column_index - The index of the disk passed in.
 *      
 * @return
 *  None.
 *
 * @notes
 *
 * @author
 *  03/16/06 - Created. RCL
 ***************************************************************************/

void fbe_xor_calc_parity_of_csum_update(const fbe_u16_t * const src_ptr,
                                    fbe_u16_t * const tgt_row_ptr,
                                    fbe_u16_t * const tgt_diagonal_ptr,
                                    const fbe_u32_t column_index)
{

    fbe_u16_t s_value = 0x0;    

    /* If the S value is set in this column set the S value to all ones.
     * This will allow us to xor in one S value to the whole diagonal
     * parity column instead of 16 individual bits. The first column does 
     * not contain an S value component.
     */
    if ((column_index != 0) && 
        (FBE_XOR_IS_BITSET(*src_ptr, column_index - 1)))
    {
        s_value = 0xffff;
    }

    /* Update the row parity symbol with the data symbol.
     */
    *tgt_row_ptr ^= *src_ptr;

    /* Update the correct diagonal parity symbol with the current data symbol.
     */
    *tgt_diagonal_ptr ^= FBE_XOR_EVENODD_DIAGONAL_MANGLE(*src_ptr, column_index) 
                          ^ s_value;

    return;
} /* end fbe_xor_calc_parity_of_csum_update */

/***************************************************************************
 * xor_calc_parity_of_csum_update_and_cpy()
 ***************************************************************************
 * DESCRIPTION:
 *  This function updates the row and diagonal parity of the 16 bit
 *  checksum value from temp buffers and writes the result to row and
 *  diagonal parity byffers.
 *
 * PARAMETERS:
 *  src_ptr, [I] - ptr to source data
 *  tgt_temp_row_ptr, [IO] - ptr to row parity
 *  tgt_temp_diagonal_ptr, [IO] - ptr to diagonal parity
 *  column_index, [I] - The index of the disk passed in
 *  tgt_row_ptr, [IO] - ptr to row parity
 *  tgt_diagonal_ptr, [IO] - ptr to diagonal parity
 *      
 * RETURNS:
 *  None.
 *
 * NOTES:
 *
 * HISTORY:
 *  10/xx/13 - Created. Vamsi Vankamamidi
 ***************************************************************************/

void fbe_xor_calc_parity_of_csum_update_and_cpy (const fbe_u16_t * const src_ptr,
                                                 const fbe_u16_t * const tgt_temp_row_ptr,
                                                 const fbe_u16_t * const tgt_temp_diagonal_ptr,
                                                 const fbe_u32_t column_index,
                                                 fbe_u16_t * const tgt_row_ptr,
                                                 fbe_u16_t * const tgt_diagonal_ptr)
{

    UINT_16E s_value = 0x0;    

    /* If the S value is set in this column set the S value to all ones.
     * This will allow us to xor in one S value to the whole diagonal
     * parity column instead of 16 individual bits. The first column does 
     * not contain an S value component.
     */
    if ((column_index != 0) && 
        (FBE_XOR_IS_BITSET(*src_ptr, column_index - 1)))
    {
        s_value = 0xffff;
    }

    /* Update the row parity symbol with the data symbol.
     */
    *tgt_row_ptr = *tgt_temp_row_ptr ^ *src_ptr;

    /* Update the correct diagonal parity symbol with the current data symbol.
     */
    *tgt_diagonal_ptr = *tgt_temp_diagonal_ptr ^ FBE_XOR_EVENODD_DIAGONAL_MANGLE(*src_ptr, column_index) 
                          ^ s_value;

    return;
} /* end xor_calc_parity_of_csum_update_and_cpy */

/***************************************************************************
 * fbe_xor_init_scratch_r6()
 ***************************************************************************
 * @brief
 *  This routine is used to initialize the "scratch" data-base used to 
 *  process Raid6 LUs.
 *
 * @param scratch_p - ptr to a Scratch database area.     
 * @param sector_p - vector of ptrs to sector contents.
 * @param bitkey - bitkey for stamps/error boards.
 * @param width -  number of sectors in strip.
 * @param seed - lba of this strip.
 * @param parity_bitmap - The position bitmask of parity positions.
 * @param ppos - The parity position array.
 * @param rpos - The rebuild position array.
 * @param eboard_p - The error board.
 * @param initial_rebuild_position - Array of positions that were originally
 *                                   marked as needing a rebuild.
 *
 * @return
 *  None.
 *
 * @notes
 *
 * @author
 *  04/04/2006 - Created.  RCL
 ***************************************************************************/

fbe_status_t fbe_xor_init_scratch_r6(fbe_xor_scratch_r6_t *scratch_p,
                                     fbe_xor_r6_scratch_data_t *scratch_data_p,
                                     fbe_sector_t * sector_p[],
                                     fbe_u16_t bitkey[],
                                     fbe_u16_t width,
                                     fbe_lba_t seed,
                                     fbe_u16_t parity_bitmap,
                                     fbe_u16_t ppos[],
                                     fbe_u16_t rpos[],
                                     fbe_xor_error_t * eboard_p,
                                     fbe_u16_t initial_rebuild_position[])
{
    fbe_u16_t local_invalid_bitmap;
    fbe_u16_t index;

    /* Initialize the fatal_key now, we will init the rest of the scratch later.
     */
    scratch_p->fatal_key = 0;

    /* Determine media error bitmap.
     * If there is a media errored position, then
     * we clear its bit from the mask below in
     * fbe_xor_determine_media_errors(),
     * and that function requires this value.
     *
     * Note that all uncorrectables at this point
     * are due to media errors.
     *
     * If there is a rebuild position, then
     * clear its bit from the mask.
     */
    scratch_p->media_err_bitmap = eboard_p->hard_media_err_bitmap;
    scratch_p->retry_err_bitmap = eboard_p->retry_err_bitmap;

    if ( rpos[0] != FBE_XOR_INV_DRV_POS)
    {
        /* Since we really have a rebuild position,
         * clear its bit from the media error bitmap,
         * so that we will not report this position with errors.
         */
        scratch_p->media_err_bitmap &= ~(bitkey[rpos[0]]);
        scratch_p->retry_err_bitmap &= ~(bitkey[rpos[0]]);
        initial_rebuild_position[0] = rpos[0];

        /* We are rebuilding the "rpos" position.
         * Mark this position as invalid, so we will
         * 1) rebuild this array position
         * 2) NOT report a CRC error.
         */
        scratch_p->fatal_key |= bitkey[rpos[0]];

        if ( rpos[1] != FBE_XOR_INV_DRV_POS)
        {
            /* Since we really have a rebuild position,
             * clear its bit from the media error bitmap,
             * so that we will not report this position with errors.
             */
            scratch_p->media_err_bitmap &= ~(bitkey[rpos[1]]);
            scratch_p->retry_err_bitmap &= ~(bitkey[rpos[1]]);
            initial_rebuild_position[1] = rpos[1];

            /* We are rebuilding the "rpos" position.
             * Mark this position as invalid, so we will
             * 1) rebuild this array position
             * 2) NOT report a CRC error.
             */
            scratch_p->fatal_key |= bitkey[rpos[1]];
        }
    }
        
    /* Add uncorrectable crc and stamp errors to fatal key.
     */
    scratch_p->fatal_key |= scratch_p->media_err_bitmap;
    scratch_p->fatal_key |= eboard_p->retry_err_bitmap;
    scratch_p->fatal_key |= eboard_p->no_data_err_bitmap;

    local_invalid_bitmap = scratch_p->fatal_key;

    scratch_p->initialized =
        scratch_p->xor_required = FBE_FALSE,
        scratch_p->running_csum = 0,
        scratch_p->fatal_cnt = 0,
        scratch_p->fatal_blk[0] = 
        scratch_p->fatal_blk[1] = NULL,
        scratch_p->row_syndrome = &(scratch_data_p->scratch_syndromes[0]),
        scratch_p->diag_syndrome = &(scratch_data_p->scratch_syndromes[1]),
        scratch_p->seed = seed,
        scratch_p->offset = eboard_p->raid_group_offset,
        scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_NOT_SET,
        scratch_p->s_value = &(scratch_data_p->scratch_S_value),
        scratch_p->checksum_row_syndrome = 0,
        scratch_p->checksum_diag_syndrome = 0,
        scratch_p->strip_verified = FBE_FALSE,
        scratch_p->s_value_for_checksum = 0,
        scratch_p->stamp_mismatch_pos = FBE_XOR_INV_DRV_POS,
        scratch_p->traced_pos_bitmap = 0,
        scratch_p->width = width;

    /* Setup parity positions.
     */
    for (index = 0; index < FBE_XOR_NUM_PARITY_POS; index++)
    {
        scratch_p->ppos[index] = ppos[index];
    }
    
    /* Setup the array of sector pointers and array
     * of position to postion bitmask.
     */
    for (index = 0; index < width; index++)
    {
        scratch_p->sector_p[index] = sector_p[index];
        scratch_p->pos_bitmask[index] = bitkey[index];
    }

    /* Count the number of 'known' errors...
     */
    while ( (width-- > 0) && (local_invalid_bitmap != 0))
    {
        if (0 != (scratch_p->fatal_key & bitkey[width]))
        {
            /* If there are more than 2 fatal errors we can't fix anything
             * so we don't need to keep track of their locations.
             */
            if (scratch_p->fatal_cnt < 2)
            {
                scratch_p->fatal_blk[scratch_p->fatal_cnt] = sector_p[width];
            }
            scratch_p->fatal_cnt++;
            local_invalid_bitmap &= ~bitkey[width];
        }
    }

    return FBE_STATUS_OK;
} /* xor_init_scratch() */

/***************************************************************************
 *  fbe_xor_reset_rebuild_positions_r6()
 ***************************************************************************
 *  @brief
 *    This routine is used to ensure the scratch and rebuild_position tracking
 *    variables are maintained correctly after new errors have been found or
 *    some errors have been corrected.  If one data disk and one parity are
 *    dead then the data position should be in the first rebuild position and
 *    the parity disk should be in the second.
 *

 * @param scratch_p - ptr to a Scratch database area.     
 * @param sector_p - vector of ptrs to sector contents.
 * @param bitkey - bitkey for stamps/error boards.
 * @param width -  number of sectors in strip.
 * @param parity_pos - array of parity positions.
 * @param rebuild_pos - array of rebuild positions.
 *
 *  @return
 *    None.
 *
 *  @notes
 *
 *  @author
 *    05/03/2006 - Created.  RCL
 ***************************************************************************/

fbe_status_t fbe_xor_reset_rebuild_positions_r6(fbe_xor_scratch_r6_t * scratch_p,
                                    fbe_sector_t * sector_p[],
                                    fbe_u16_t bitkey[],
                                    fbe_u16_t width,
                                    fbe_u16_t parity_pos[],
                                    fbe_u16_t rebuild_pos[],
                                    fbe_u16_t data_position[])
{
    fbe_u16_t errors_found = 0;
    fbe_u16_t column_index = 0;
    fbe_u16_t width_index = 0;
    fbe_xor_r6_parity_positions_t parity_index;
    scratch_p->fatal_blk[0] = NULL;
    scratch_p->fatal_blk[1] = NULL;
    scratch_p->s_value_for_checksum = 0;

    /* Make sure we don't leave junk info behind.
     */
    rebuild_pos[0] = FBE_XOR_INV_DRV_POS;
    rebuild_pos[1] = FBE_XOR_INV_DRV_POS;

    /* Count the number of errors in the data positions.
     */
    for (column_index = 0; column_index < width - 2; column_index++)
    {
        /* We need to order the rebuild positions by data position.  Find
         * the corresponding physical position for each data position.
         */
        for (width_index = 0; width_index < width; width_index++)
        {
            if (data_position[width_index] == column_index)
            {
                break;
            }
        }

        /* If this position is in the fatal key update the error tracking
         * structures.
         */
        if ((0 != (scratch_p->fatal_key & bitkey[width_index])) &&
            (width_index != parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]) &&
            (width_index != parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]))
        {
            /* We only track 2 errors since that is the most we can fix.  If
             * there are more than 2 errors forget updating the tracking
             * structure the fatal key is enough.
             */
            if (errors_found < 2)
            {
                scratch_p->fatal_blk[errors_found] = sector_p[width_index];
                rebuild_pos[errors_found] = width_index;
            }
            errors_found++;
        }
    }

    /* Count the number of errors in the parity positions.
     */
   for (parity_index = FBE_XOR_R6_PARITY_POSITION_ROW; parity_index < 2; parity_index++)
   {
        if (0 != (scratch_p->fatal_key & bitkey[parity_pos[parity_index]]))
        {
            if (errors_found < 2)
            {
                scratch_p->fatal_blk[errors_found] = sector_p[parity_pos[parity_index]];
                rebuild_pos[errors_found] = parity_pos[parity_index];
            }
            errors_found++;
        }
   }

   /* Set the scratch to uninitialized so we don't reuse leftover data.
    */
   scratch_p->initialized = FBE_FALSE;

   /* Make sure we found the error we think we have.
    */
   if (XOR_COND(scratch_p->fatal_cnt != errors_found))
   {
       fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                             "scratch_p->fatal_cnt 0x%x != errors_found 0x%x\n",
                             scratch_p->fatal_cnt,
                             errors_found);

       return FBE_STATUS_GENERIC_FAILURE;
   }

    
   return FBE_STATUS_OK;
} /* fbe_xor_reset_rebuild_positions_r6() */

/***************************************************************************
 *  fbe_xor_eval_data_sector_r6()
 ***************************************************************************
 * @brief
 *   This routine is used for Raid6 LUs to determine if the content of the 
 *   data sector is 'internally consistent', i.e. the checksum attached to
 *   the data is correct, and that the timstamp, writestamp and shedstamp 
 *   values are structured in accordance with FLARE policy.  In addition to 
 *   those tasks depending on the state of the scratch the syndromes will be
 *   partially calculated or the necessary reconstruction tasks will be 
 *   preformed.
 *

 * @param scratch_p - ptr to the scratch database area.
 * @param eboard_p - error board for marking different types of errors.
 * @param dblk - ptr to data sector.
 * @param dkey - bitkey for data sector.
 * @param data_position - the logical position of the drive within the strip.
 *   rpos[],        [I] - array of the rebuild positions.  (up to 2) (logical)
 * @param parity_bitmap - bitmap of parity positions
 * @param error_result_p =o] - FBE_TRUE if there are no errors enocuntered.
 *                             FBE_FALSE if an error is encountered.
 *
 * @return fbe_status_t
 *
 *  @author
 *    04/04/2006 - Created. RCL
 ***************************************************************************/
fbe_status_t fbe_xor_eval_data_sector_r6(fbe_xor_scratch_r6_t * scratch_p,
                                        fbe_xor_error_t * eboard_p,
                                        fbe_sector_t * dblk_p,
                                        fbe_u16_t dkey,
                                        fbe_u32_t data_position,
                                        fbe_u16_t rpos[],
                                        fbe_u16_t parity_bitmap,
                                        fbe_bool_t * const error_result_p)
{
    fbe_u32_t checksum = 0x0;
    fbe_u16_t cooked_csum = 0;
    fbe_bool_t no_errors = FBE_TRUE;
    fbe_bool_t b_eval_stamps = FBE_TRUE;
    fbe_status_t status;
    
    if ((FBE_XOR_IS_SCRATCH_VERIFY_STATE(scratch_p, parity_bitmap, scratch_p->fatal_key)) && 
        (scratch_p->scratch_state == FBE_XOR_SCRATCH_R6_STATE_VERIFY))
    {
        /* Update the syndromes with the data from this disk.
         */
        checksum = fbe_xor_calc_data_syndrome(scratch_p, data_position, dblk_p);
    }
    else if ((FBE_XOR_IS_SCRATCH_RECONSTRUCT_1_STATE(scratch_p, parity_bitmap, scratch_p->fatal_key)) && 
            (scratch_p->scratch_state == FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_1))
    {
        /* Calculate rebuild position (=rpos[0]) from row parity and diagonal 
         * parity.  This should return the uncooked checksum of the data block.
         */
        status = fbe_xor_calc_csum_and_reconstruct_1(scratch_p,
                                                   dblk_p,
                                                   data_position,
                                                   rpos[0],
                                                   &checksum);
        if (status != FBE_STATUS_OK) 
        {
            return status;
        }
    }
    else if ((FBE_XOR_IS_SCRATCH_RECONSTRUCT_2_STATE(scratch_p, parity_bitmap, scratch_p->fatal_key)) && 
            (scratch_p->scratch_state == FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_2))
    {
        checksum = fbe_xor_calc_data_syndrome(scratch_p, data_position, dblk_p);
    }
    else if ((FBE_XOR_IS_SCRATCH_RECONSTRUCT_P_STATE(scratch_p, parity_bitmap, scratch_p->fatal_key)) && 
            (scratch_p->scratch_state == FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_P))
    {
        /* Calculate the parity for the missing parity drive(s).  This should
         * return the uncooked checksum of the data block.
         */
         status  = fbe_xor_calc_csum_and_reconstruct_parity(scratch_p,
                                                        dblk_p,
                                                        data_position,
                                                        rpos,
                                                        parity_bitmap,
                                                        &checksum);
         if (status != FBE_STATUS_OK) 
         { 
             return status;
         }
    }
    else
    {
        /* We picked up another fatal error along the way, just check the 
         * checksums and we will go through the loop again later.
         */
        checksum = xorlib_calc_csum(dblk_p->data_word);
    } /* end if scratch state */


    if (dblk_p->crc != (cooked_csum = xorlib_cook_csum(checksum, (fbe_u32_t)scratch_p->seed)))
    {
        /* Determine the types of errors.
         */
        status = fbe_xor_determine_csum_error( eboard_p, dblk_p, dkey, scratch_p->seed,
                                               dblk_p->crc, cooked_csum, FBE_TRUE);
        if (status != FBE_STATUS_OK) 
        { 
            return status;
        }

        /* Check if the sector has been invalidated on purpose.  If it has then
         * we don't want to add it as uncorrectable.
         */
        if (((eboard_p->crc_invalid_bitmap & dkey) == 0) &&
            ((eboard_p->crc_raid_bitmap & dkey) == 0)    &&
            ((eboard_p->corrupt_crc_bitmap & dkey) == 0)    )
        {
            /* Bad checksum for this sector's data.
             * Mark this as an uncorrectable checksum
             * error for now.
             */
            eboard_p->u_crc_bitmap |= dkey;
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_crc_bitmap, 
                                               FBE_XOR_ERR_TYPE_CRC, 17, FBE_TRUE);
            if (status != FBE_STATUS_OK)
            { 
                return status; 
            }
        
            /* We consider this a fatal hit, so do not evaluate the stamps.
             */
            b_eval_stamps = FBE_FALSE;
        }
    }
    else if (!xorlib_is_valid_lba_stamp(&dblk_p->lba_stamp, scratch_p->seed, scratch_p->offset))
    {
        /* Invalid lba stamp, report the same as a checksum error.
         */
        eboard_p->u_crc_bitmap |= dkey;
        eboard_p->crc_lba_stamp_bitmap |= dkey;

        /* Log the Error. 
         */
        status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->crc_lba_stamp_bitmap,
                                           FBE_XOR_ERR_TYPE_LBA_STAMP, 17, FBE_TRUE);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }

        /* We consider this a fatal hit, so do not evaluate the stamps.
         */
        b_eval_stamps = FBE_FALSE;
    }

    if (b_eval_stamps)
    {
        /* The checksum was okay, so let's look
         * at the rest of the metadata. We'll
         * start with the shed stamps.
         */
        if (0 != (dblk_p->time_stamp & FBE_SECTOR_ALL_TSTAMPS))
        {
            /* The ALL_TIMESTAMPS bit should never be set
             * on a data sector. For now, mark this as
             * an uncorrectable time stamp error.
             */
            eboard_p->u_ts_bitmap |= dkey;
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_ts_bitmap, 
                                               FBE_XOR_ERR_TYPE_TS, 5, FBE_TRUE);
            if (status != FBE_STATUS_OK) 
            { 
                return status;
            }
        }

        if (dblk_p->write_stamp == 0)
        {
            /* No writestamp bits are set.
             */
        }
        else if (dblk_p->write_stamp != dkey)
        {
            /* The wrong bit[s] are set in the writestamp.
             * For now, mark this as an uncorrectable write
             * stamp error.
             */
            eboard_p->u_ws_bitmap |= dkey;
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_ws_bitmap,
                                               FBE_XOR_ERR_TYPE_WS, 4, FBE_TRUE);
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }
        }
        else if (dblk_p->time_stamp != FBE_SECTOR_INVALID_TSTAMP)
        {
            /* The correct bit is set in the writestamp, but
             * the timestamp has not been invalidated accordingly.
             * This could go either way. For now, mark this as an
             * uncorrectable time stamp error.
             */
            eboard_p->u_ts_bitmap |= dkey;
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_ts_bitmap,
                                               FBE_XOR_ERR_TYPE_TS, 6, FBE_TRUE);
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }
        }
    } /* end if checksum matches */

    if (0 != (dkey & (eboard_p->u_crc_bitmap |
                      eboard_p->u_ts_bitmap |
                      eboard_p->u_ws_bitmap |
                      eboard_p->u_ss_bitmap)))
    {
        fbe_xor_scratch_r6_add_error(scratch_p, dkey);
        no_errors = FBE_FALSE;
    }

    *error_result_p = no_errors;
    return FBE_STATUS_OK;
}
/* fbe_xor_eval_data_sector_r6() */

/***************************************************************************
 *  fbe_xor_eval_parity_sector_r6()
 ***************************************************************************
 * @brief
 *   This routine is used for Raid6 LUs to determine if the content of the 
 *   parity sector is 'internally consistent', i.e. the checksum attached to
 *   the data is correct, and that the timstamp and writestamp values are 
 *   structured in accordance with FLARE policy.  In addition to those tasks
 *   depending on the state of the scratch the syndromes will be partially 
 *   calculated or the necessary reconstruction tasks will be preformed.
 *

 * @param scratch_p - ptr to the scratch database area.
 * @param eboard_p - error board for marking different types of errors.
 * @param pblk_p - ptr to parity sector.
 * @param pkey - bitkey for parity sector.
 * @param width - number of sectors in the strip.
 * @param pindex - parity position index, 0 = row parity sector
 *                                               1 = diagonal parity sector
 * @param parity_bitmap - bitmap of parity positions
 *   rpos[],        [I] - arrray of the rebuild positions.  (up to 2)
 * @param error_result_p - [O] FBE_TRUE if there are no errors enocuntered.
 *                             FBE_FALSE if an error is encountered.
 *
 * @return fbe_status_t
 *
 * @author
 *    04/04/2006 - Created. RCL
 ***************************************************************************/

fbe_status_t fbe_xor_eval_parity_sector_r6(fbe_xor_scratch_r6_t * scratch_p,
                               fbe_xor_error_t * eboard_p,
                               fbe_sector_t * pblk_p,
                               fbe_u16_t pkey,
                               fbe_u16_t width,
                               fbe_xor_r6_parity_positions_t pindex,
                               fbe_u16_t parity_bitmap,
                               fbe_u16_t rpos[],
                               fbe_bool_t * const error_result_p)
{
    fbe_u32_t checksum = 0x0;
    fbe_u16_t cooked_csum = 0;
    fbe_bool_t no_errors = FBE_TRUE;
    fbe_status_t status;

    if ((FBE_XOR_IS_SCRATCH_VERIFY_STATE(scratch_p, parity_bitmap, scratch_p->fatal_key)) && 
        (scratch_p->scratch_state == FBE_XOR_SCRATCH_R6_STATE_VERIFY))
    {
        /* Update the syndromes with the data from this parity sector.
         */
        checksum = fbe_xor_calc_parity_syndrome(scratch_p, pindex, pblk_p);

    }
    else if ((FBE_XOR_IS_SCRATCH_RECONSTRUCT_1_STATE(scratch_p, parity_bitmap, scratch_p->fatal_key)) && 
            (scratch_p->scratch_state == FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_1))
    {
        /* Calculate rebuild position(=rpos[0]) from row parity and diagonal 
         * parity. This should return the checksum of the parity block.
         */
        checksum = fbe_xor_calc_csum_and_parity_reconstruct_1(scratch_p,
                                                          pblk_p,
                                                          pindex,
                                                          rpos[0]);
    }
    else if ((FBE_XOR_IS_SCRATCH_RECONSTRUCT_2_STATE(scratch_p, parity_bitmap, scratch_p->fatal_key)) && 
            (scratch_p->scratch_state == FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_2))
    {
        checksum = fbe_xor_calc_parity_syndrome(scratch_p, pindex, pblk_p);
    }
    else if ((FBE_XOR_IS_SCRATCH_RECONSTRUCT_P_STATE(scratch_p, parity_bitmap, scratch_p->fatal_key)) && 
            (scratch_p->scratch_state == FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_P))
    {
        /* We are rebuilding a parity disk and it is not this one so all we 
         * have to do is validate this checksum since it is not needed for the 
         * rebuild calculation.
         */
        checksum = xorlib_calc_csum(pblk_p->data_word); 
    }
    else
    {
        /* We picked up another fatal error along the way, just check the 
         * checksums and we will go through the loop again later.
         */
        checksum = xorlib_calc_csum(pblk_p->data_word);
    } /* end if scratch state */

    if (pblk_p->crc != (cooked_csum = xorlib_cook_csum(checksum, (fbe_u32_t)scratch_p->seed)))
    {
        /* The checksum calculated for the parity data
         * doesn't match that attached to the sector. Mark
         * this as an uncorrectable checksum error for now.
         */
        eboard_p->u_crc_bitmap |= pkey;
        
        /* Determine the types of errors.
         */
        status = fbe_xor_determine_csum_error( eboard_p, pblk_p, pkey, scratch_p->seed,
                                               pblk_p->crc, cooked_csum, FBE_TRUE);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }

        /* Check if the sector has been invalidated on purpose.
         * If so, then clear the reason bits so that we will
         * actually try to fix this.
         */
        if (((eboard_p->crc_invalid_bitmap & pkey) != 0) ||
            ((eboard_p->crc_raid_bitmap & pkey) != 0)    ||
            ((eboard_p->corrupt_crc_bitmap & pkey) != 0)    )
        {
            /* Notice that we clear the bitmap before tracing the errors.
             * This is so that we DON'T log the sector.
             */
            eboard_p->crc_invalid_bitmap &= ~pkey;
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->crc_invalid_bitmap, 
                                               FBE_XOR_ERR_TYPE_INVALIDATED, 19, FBE_TRUE);
            if (status != FBE_STATUS_OK)
            { 
                return status; 
            }

            eboard_p->crc_raid_bitmap &= ~pkey;
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->crc_raid_bitmap, 
                                               FBE_XOR_ERR_TYPE_RAID_CRC, 19, FBE_TRUE);
            if (status != FBE_STATUS_OK)
            { 
                return status; 
            }

            eboard_p->corrupt_crc_bitmap &= ~pkey;
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->corrupt_crc_bitmap,
                                               FBE_XOR_ERR_TYPE_CRC, 19, FBE_TRUE);
            if (status != FBE_STATUS_OK) 
            { 
                return status;
            }
        }
        else
        {
            /* Trace this as an uncorrectable error.
             */
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_crc_bitmap, 
                                               FBE_XOR_ERR_TYPE_CRC, 18, FBE_TRUE);
            if (status != FBE_STATUS_OK) 
            {
                return status; 
            }
        }
    }
    else
    {
        fbe_u16_t tstamp,
          wstamp;

        wstamp = pblk_p->write_stamp,
            tstamp = pblk_p->time_stamp;

        /* The checksum was okay, so let's look at
         * the relative consistency of the stamps.
         */

        /* Now check the write stamp.
         */
        if (0 == wstamp)
        {
            /* No write stamps are set.
             */
        }

        else if ((0 != (wstamp >> width)) ||
                 (0 != (wstamp & parity_bitmap)))
        {
            /* The writestamp has bits set for non-existant
             * drives, or for either of the parity drives. For now,
             * mark this as an uncorrectable write stamp error.
             */
            eboard_p->u_ws_bitmap |= pkey;
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_ws_bitmap,
                                               FBE_XOR_ERR_TYPE_WS, 5, FBE_TRUE);
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }
        }

        else if (0 != (tstamp & FBE_SECTOR_ALL_TSTAMPS))
        {
            /* The ALL_TIMESTAMPS flag is set, but
             * there are valid bits set in the writestamp
             * as well.
             */
            eboard_p->u_ts_bitmap |= pkey;
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_ts_bitmap, 
                                               FBE_XOR_ERR_TYPE_TS, 7, FBE_TRUE);
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }
        }

        /* Now check the time stamp.
         */
        if ((tstamp == (FBE_SECTOR_ALL_TSTAMPS | FBE_SECTOR_INITIAL_TSTAMP)) ||
            (tstamp == (FBE_SECTOR_ALL_TSTAMPS | FBE_SECTOR_INVALID_TSTAMP)) ||
            (tstamp == (FBE_SECTOR_ALL_TSTAMPS | FBE_SECTOR_R6_INVALID_TSTAMP)))
        {
            /* If the stripe was written with all
             * timestamps, then neither of these
             * values are permitted.
             */
            eboard_p->u_ts_bitmap |= pkey;
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_ts_bitmap, 
                                               FBE_XOR_ERR_TYPE_TS, 8, FBE_TRUE);
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }
        }
        else if ( tstamp == FBE_SECTOR_INITIAL_TSTAMP )
        {
            /* Only a zeroed strip has an initial time stamp.
             * If we are here, then we should not be zeroed.
             * If we are zeroed, then this parity is not useful,
             * since the initial parity does not reflect the data.
             */
            eboard_p->u_ts_bitmap |= pkey;
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_ts_bitmap,
                                               FBE_XOR_ERR_TYPE_TS, 9, FBE_TRUE);
            if (status != FBE_STATUS_OK) 
            {
                return status; 
            }
        }
    } /* end if checksum matches */

    /* If there is an error with this driver update the eboard and the scratch.
     */
    if (0 != (pkey & (eboard_p->u_crc_bitmap |
                      eboard_p->u_coh_bitmap |
                      eboard_p->u_ts_bitmap |
                      eboard_p->u_ws_bitmap |
                      eboard_p->u_ss_bitmap)))
    {
        /* we always set no_errors to FALSE irrespective of completion status 
         * of fbe_xor_scratch_r6_add_fatal_block().
         */
        fbe_xor_scratch_r6_add_fatal_block(scratch_p, pkey, pblk_p); 
        no_errors = FBE_FALSE;
    }

    *error_result_p = no_errors;
    return FBE_STATUS_OK;
}
/* fbe_xor_eval_parity_sector_r6() */
fbe_status_t fbe_xor_trace_stamps_r6(fbe_xor_scratch_r6_t * const scratch_p, 
                                     fbe_xor_error_t * const eboard_p,
                                     fbe_sector_t *const sector_p[],
                                     const fbe_u16_t bitkey[],
                                     const fbe_u16_t width,
                                     const fbe_u16_t rebuild_pos[],
                                     const fbe_u16_t parity_pos[],
                                     const fbe_lba_t seed,
                                     fbe_xor_error_type_t error_type,
                                     fbe_u32_t occurrence_number,
                                     fbe_u16_t bitmask,
                                     fbe_char_t *string_p)
{
#if 0
    fbe_char_t ws_array[500];
    fbe_char_t ts_array[500];
    fbe_u16_t parity_bitmask = 
        bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]] | bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]];
    fbe_u16_t pos;
    ws_array[0] = 0;
    ts_array[0] = 0;
    if (scratch_p->traced_pos_bitmap == 0xFFFF)
    {
        return FBE_STATUS_OK;
    }
    scratch_p->traced_pos_bitmap = 0xFFFF;
    for (pos = 0; pos < width; pos++)
    {
        fbe_char_t tmp_string[10];
        fbe_sector_t *dblk_p;
        dblk_p = sector_p[pos];
        _snprintf(&tmp_string[0], 10, "%04x ", dblk_p->write_stamp);
        strcat(ws_array, tmp_string);
        _snprintf(&tmp_string[0], 10, "%04x ", dblk_p->time_stamp);
        strcat(ts_array, tmp_string);
    }
    fbe_base_library_trace(FBE_LIBRARY_ID_XOR, FBE_TRACE_LEVEL_WARNING, eboard_p->raid_group_object_id,
                           "%s err: 0x%x #: %d bm: 0x%x p_bm: 0x%x eb: 0x%x seed: 0x%llx\n", 
                           string_p, error_type, occurrence_number, bitmask, parity_bitmask, (fbe_u32_t)eboard_p, seed);
    fbe_base_library_trace(FBE_LIBRARY_ID_XOR, FBE_TRACE_LEVEL_WARNING, eboard_p->raid_group_object_id,
                           "eb: 0x%x ws: %s\n", (fbe_u32_t)eboard_p, ws_array);
    fbe_base_library_trace(FBE_LIBRARY_ID_XOR, FBE_TRACE_LEVEL_WARNING, eboard_p->raid_group_object_id,
                           "eb: 0x%x ts: %s\n", (fbe_u32_t)eboard_p, ts_array);
#endif
    return FBE_STATUS_OK;
}

/***************************************************************************
 *  fbe_xor_rbld_parity_stamps_r6()
 ***************************************************************************
 * @brief
 *  This routine is used for Raid6 LUs to check and correct 
 *  inconsistencies among stamp values in an otherwise valid strip. 
 *  This maximizes the probability that we will be able to reconstruct 
 *  the contents of lost sectors in the future.
 *

 * @param scratch_p - Pointer to scratch structure for this request
 * @param eboard_p - Error board for this request.
 * @param sector_p - Vector of sector ptrs.
 * @param bitkey - Bitkey vector for stamps/error boards.
 * @param width - The width of the LUN.
 *   rebuild_pos[2],[I]  - Indexes to be rebuilt, if any.
 *   parity_pos[2], [I]  - The parity positions vector.
 * @param seed - Seed for LBA stamping.
 * @param check_parity_stamps - True to check stamps, FBE_FALSE to just set them.
 *
 * @return fbe_status_t
 *  
 *
 * ASSUMPTIONS:
 *  We assume that this function is only called when we are 
 *  either rebuilding parity (check_parity_stamps == FBE_FALSE) 
 *  and need to reconstruct valid stamps.
 *  Or we are performing a verify (check_parity_stamps == FBE_TRUE),
 *  and need to fix up any inconsistencies in the parity stamps.
 *
 * @author
 *   05/01/2006 - Created. Rob Foley
 ***************************************************************************/
fbe_status_t fbe_xor_rbld_parity_stamps_r6(fbe_xor_scratch_r6_t * const scratch_p, 
                               fbe_xor_error_t * const eboard_p,
                               fbe_sector_t *const sector_p[],
                               const fbe_u16_t bitkey[],
                               const fbe_u16_t width,
                               const fbe_u16_t rebuild_pos[],
                               const fbe_u16_t parity_pos[],
                               const fbe_lba_t seed,
                               const fbe_bool_t check_parity_stamps)
{
    fbe_u16_t pos;             /* Position within the array. */
    fbe_u16_t timestamp_count; /* Count of timestamps found in strip. */
    fbe_u16_t time_stamp;
    fbe_u16_t write_stamp;
    fbe_status_t status;

    /* Initialize the timestamp to initial for now since
     * we don't yet know if there is a valid timestamp on a
     * data drive in this strip.
     */
    time_stamp = FBE_SECTOR_INITIAL_TSTAMP;

    /* Loop over every data drive, and break out when
     * we find the first real timestamp value.
     * We'll use this below as a starting point.
     */
    for (pos = 0; pos < width; pos++)
    {
        /* Ignore the parity positions since we will be
         * updating the parity stamps based on the data
         * stamp values.
         */
        if (pos != parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW] && 
            pos != parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG])
        {
            fbe_sector_t *dblk_p;

            dblk_p = sector_p[pos];

            if ((FBE_SECTOR_INVALID_TSTAMP != dblk_p->time_stamp) &&
                (FBE_SECTOR_INITIAL_TSTAMP != dblk_p->time_stamp))
            {
                /* If the stamps are not invalid and not initial,
                 * then they are "real" timestamps.
                 * Hold onto this stamp value and break out.
                 */
                time_stamp = dblk_p->time_stamp;
                break;
            }
        } /* End if not parity position. */
    } /* End for all data positions. */

    /* Loop over the data positions, correcting any out of
     * place time stamps and accumulating the write stamps.
     */
    write_stamp = 0;
    timestamp_count = 0;

    for (pos = 0; pos < width; pos++)
    {
        /* Ignore the parity positions since we will be
         * updating the parity stamps based on the data
         * stamp values.
         */
        if (pos != parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW] && 
            pos != parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG])
        {
            fbe_sector_t *dblk_p;
            fbe_u16_t pos_bitmask;

            dblk_p = sector_p[pos];
            pos_bitmask = 1 << pos;

            if (dblk_p->write_stamp != 0)
            {
                /* Accumulate the write stamp.
                 */
                write_stamp |= dblk_p->write_stamp;
            }

            else if (FBE_SECTOR_INVALID_TSTAMP == dblk_p->time_stamp)
            {
                /* No timestamp. Writestamp just happened to be zero.
                 */
            }
            else if (timestamp_count++, time_stamp != dblk_p->time_stamp)
            {
                /* We have mis-matched timestamp values in this
                 * stripe. This can occur when an MR3 is only
                 * partially written, the result of an unexpected
                 * shutdown condition. Replace the timestamp for
                 * this sector with the one selected. Mark this
                 * sector as having had its timestamp corrected.
                 */
                fbe_xor_trace_stamps_r6(scratch_p, eboard_p, sector_p, bitkey, width, rebuild_pos, parity_pos, 
                                        seed, FBE_XOR_ERR_TYPE_TS, 10, bitkey[pos], "data ts error");
                dblk_p->time_stamp = time_stamp;
                eboard_p->c_ts_bitmap |= bitkey[pos];
                status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_ts_bitmap,
                                                   FBE_XOR_ERR_TYPE_TS, 10, FBE_FALSE);
                if (status != FBE_STATUS_OK) 
                { 
                    return status; 
                }

                /* Set the lba stamp, since this position will
                 * now be written.
                 */
                dblk_p->lba_stamp = xorlib_generate_lba_stamp(seed, eboard_p->raid_group_offset);
            }
        } /* End if not parity position. */
    } /* End for all data positions. */

    if ( time_stamp == FBE_SECTOR_INVALID_TSTAMP)
    {
        /* R6 parity drives do not use the invalid timestamp,
         * they use a special R6 style invalid timestamp.
         */
        time_stamp = FBE_SECTOR_R6_INVALID_TSTAMP;
    }

    if (timestamp_count + 2 >= width)
    {
        /* We need to set the ALL_TIMESTAMPS bit.
         */
        time_stamp |= FBE_SECTOR_ALL_TSTAMPS;
    }
    
    /* Now check to make sure the stamps on the
     * parity sector agree with the stamps on
     * the data sectors.
     */
    {
        fbe_sector_t *pblk_p;
        fbe_xor_r6_parity_positions_t parity_idx;

        for (parity_idx = FBE_XOR_R6_PARITY_POSITION_ROW; 
             parity_idx <= FBE_XOR_R6_PARITY_POSITION_DIAG; 
             parity_idx++)
        {
            fbe_bool_t b_in_fatal_key = 
                scratch_p->fatal_key & bitkey[parity_pos[parity_idx]];
            
            pblk_p = sector_p[parity_pos[parity_idx]];

            if ( b_in_fatal_key == FBE_FALSE &&
                 time_stamp == FBE_SECTOR_R6_INVALID_TSTAMP && 
                 ((timestamp_count == 0) && !b_in_fatal_key) )
            {
                /* We found no 'real' timestamps and no occurrences
                 * of the initial binding value. The timestamp can
                 * be anything. Take the value directly from the
                 * parity, stripping off the ALL_TIMESTAMPS flag.
                 * That way we can keep from triggering needless
                 * 'corrected timestamp' errors.
                 */
                time_stamp = pblk_p->time_stamp & ~FBE_SECTOR_ALL_TSTAMPS;

            } /* End if b_in_fatal_key == FBE_FALSE */

        } /* End for all parity drives. */
        
        for (parity_idx = FBE_XOR_R6_PARITY_POSITION_ROW; 
             parity_idx <= FBE_XOR_R6_PARITY_POSITION_DIAG; 
             parity_idx++)
        {
            fbe_bool_t b_in_fatal_key = 
                scratch_p->fatal_key & bitkey[parity_pos[parity_idx]];
            
            pblk_p = sector_p[parity_pos[parity_idx]];
            
            /* We are rebuilding so we are setting stamps.
             */
            if (b_in_fatal_key)
            {
                /* We're in the fatal key, which means we want to reconstruct.
                 * Set the write and time stamps to the above values
                 * which we retrieved from our data drives.
                 */
                pblk_p->write_stamp = write_stamp;
                pblk_p->time_stamp = time_stamp;
            }
            else if (check_parity_stamps ||
                     pblk_p->write_stamp != write_stamp ||
                     pblk_p->time_stamp != time_stamp )
            {
                /* The parity is not being reconstructed, but the stamps are wrong.
                 * This is an valid in the cases where we are rebuilding the
                 * parity drive and the other parity drive has stamps that
                 * mismatch with the other data drives.
                 */
                if (pblk_p->time_stamp != time_stamp)
                {
                    /* The timestamp on the parity is not
                     * what it should be. Fix it and mark
                     * it as corrected.
                     */
                    fbe_xor_trace_stamps_r6(scratch_p, eboard_p, sector_p, bitkey, width, rebuild_pos, parity_pos, 
                                            seed, FBE_XOR_ERR_TYPE_TS, 11, bitkey[parity_pos[parity_idx]],
                                            "parity ts error");
                    fbe_base_library_trace(FBE_LIBRARY_ID_XOR, FBE_TRACE_LEVEL_WARNING, eboard_p->raid_group_object_id,
                                           "eb: 0x%x p_time_stamp: 0x%x time_stamp: 0x%x p_ws: 0x%x ws: 0x%x ts_c: 0x%x\n",
                                           (fbe_u32_t)eboard_p, pblk_p->time_stamp, time_stamp, pblk_p->write_stamp, write_stamp,
                                           timestamp_count);
                    pblk_p->time_stamp = time_stamp;
                    eboard_p->c_ts_bitmap |= bitkey[parity_pos[parity_idx]];
                    status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_ts_bitmap,
                                                       FBE_XOR_ERR_TYPE_TS, 11, FBE_FALSE);
                    if (status != FBE_STATUS_OK)
                    {
                        return status; 
                    }
                }
                
                if (pblk_p->write_stamp != write_stamp)
                {
                    /* The writestamp on the parity does not
                     * match what is on the data drives. Fix
                     * it and mark it as corrected.
                     */
                    fbe_xor_trace_stamps_r6(scratch_p, eboard_p, sector_p, bitkey, width, rebuild_pos, parity_pos, 
                                            seed, FBE_XOR_ERR_TYPE_WS, 6, bitkey[parity_pos[parity_idx]], 
                                            "parity ws error");
                    pblk_p->write_stamp = write_stamp;
                    eboard_p->c_ws_bitmap |= bitkey[parity_pos[parity_idx]];
                    status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_ws_bitmap, 
                                                       FBE_XOR_ERR_TYPE_WS, 6, FBE_FALSE);
                    if (status != FBE_STATUS_OK) 
                    { 
                        return status;
                    }
                }
            } /* end else check parity stamps or stamps wrong */
        } /* End for both parity positions */
    }
    return FBE_STATUS_OK;
}
/* end fbe_xor_rbld_parity_stamps_r6 */

/****************************************************************
 * fbe_xor_eval_parity_invalidate_r6()
 ****************************************************************
 * @brief
 *  This function will invalidate any uncorrectable data blocks 
 *  and reconstruct the parity blocks if necessary.
 *

 * @param media_err_bitmap - Bitmap of media err positions.
 * @param eboard_p - Error board for marking different
 *                  types of errors.
 * @param sector_p - Vector of sectors in the strip.
 * @param bitkey - Each index in this vector has a bitmask which tells
 *                  us which position a corresponding index in
 *                  the sector array has.                 
 *                  In other words, this is the bit position to 
 *                  modify within eboard_p to save the error type.
 * @param width - Disks in array.
 * @param seed - Seed of block, which is usually the lba.
 * @param parity_pos - Index containing parity info.
 * @param rebuild_pos - Index to be rebuilt, -1 if no rebuild required.
 * @param initial_rpos - Index of dead drives, -1 if no rebuild required.
 * @param data_position - Array of data positions for phys->log conversion.
 * @param b_final_recovery_attempt - FBE_TRUE: This is our final attempt to recover
 *                              FBE_FALSE: There are more retries
 *
 * @return fbe_status_t
 *  
 *
 * @author
 *  05/10/06 - Created. Rob Foley
 *
 ****************************************************************/
 fbe_status_t fbe_xor_eval_parity_invalidate_r6( fbe_xor_scratch_r6_t * const scratch_p,
                                             const fbe_u16_t media_err_bitmap,
                                             fbe_xor_error_t * const eboard_p,
                                             fbe_sector_t * const sector_p[],
                                             const fbe_u16_t bitkey[],
                                             const fbe_u16_t width,
                                             const fbe_lba_t seed,
                                             const fbe_u16_t parity_pos[],
                                             const fbe_u16_t rebuild_pos[],
                                             const fbe_u16_t initial_rpos[],
                                             const fbe_u16_t data_position[],
                                             const fbe_bool_t b_final_recovery_attempt,
                                             fbe_u16_t * const bitmask_p)
{
    fbe_u16_t invalid_bitmap;
    fbe_status_t status;

    /* err_bitmap contains a bit set if it is an uncorrectable error
     * that was not caused by media errors.
     */
    fbe_u16_t err_bitmap = 
        eboard_p->u_crc_bitmap | eboard_p->u_ts_bitmap | 
        eboard_p->u_ws_bitmap | eboard_p->u_ss_bitmap | 
        eboard_p->u_coh_bitmap | eboard_p->u_n_poc_coh_bitmap | 
        eboard_p->u_poc_coh_bitmap | eboard_p->u_coh_unk_bitmap;
     
    fbe_u16_t fatal_parity_bitkey;/* Hold a fatal parity bitkey. */
    fbe_u32_t parity_bitkey; /* Bitkey of parity positions. */ 
    fbe_u16_t in_fatal_key;  
    
    fbe_u16_t dead_bitkey = 0; /* Bitkey of all dead positions. */

  
    parity_bitkey = 
        bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]] | 
        bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]];
    
    /* Construct a dead_bitkey from the two dead positions.
     */
    if (initial_rpos[0] != FBE_XOR_INV_DRV_POS)
    {
        dead_bitkey |= bitkey[initial_rpos[0]];
        if (initial_rpos[1] != FBE_XOR_INV_DRV_POS)
        {
            dead_bitkey |= bitkey[initial_rpos[1]];
        }
    }

    invalid_bitmap = err_bitmap;
    
    /* Also take out any media errors, since these are included 
     * in the crc error bitmask too.
     */
    err_bitmap &= ~media_err_bitmap;

    /* Clear out any dead parities, since these will be
     * removed from the eboard below and thus will not
     * be retried by Raid.
     */
    err_bitmap &= ~(dead_bitkey & parity_bitkey);

    /* Remove the row and diagonal from the invalid bitmap.
     * We won't be invalidating these, but might be
     * reconstructing these.
     */
    invalid_bitmap &= ~bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]];
    invalid_bitmap &= ~bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]];

    /* Invalidate all the data sectors in the invalid_bitmap.
     */
    fbe_xor_invalidate_sectors(sector_p,
                           bitkey,
                           width,
                           seed,
                           invalid_bitmap,
                           eboard_p);
    
    /*! @todo Since we no longer invalidate parity, the only check required 
     *        to determine if parity needs to be reconstructed or not is if
     *        `invalid_bitmap' is not 0 we must reconstruct parity.  It maybe
     *        the current code always results in this but it would be a lot
     *        cleaner to simply use that check.
     */
    if ( ( b_final_recovery_attempt && err_bitmap != 0 ) || 
         ( err_bitmap == 0 ) )
    {
        
        /* We need to reconstruct parity here since
         * we will not be retrying this operation.
         */
        status = fbe_xor_reconstruct_parity_r6(sector_p,
                                               bitkey,
                                               width,
                                               seed,
                                               parity_pos,
                                               data_position);
        if (status != FBE_STATUS_OK) 
        { 
            return status;
        }
       
       /* The parity bits are not set as a fatal_key of scratch pad. 
        * And hence, store the fatal_key of scratch pad temporarily and pass
        * only the parity bit key as a fatal_key while invoking the function 
        * fbe_xor_rbld_parity_stamps_r6() at this point. Please refer remedy note 
        * of DIMS 214135 for further detail.
        */
       in_fatal_key = scratch_p->fatal_key;
       scratch_p->fatal_key = parity_bitkey;
       
       /* Fix up the parity stamps so they match the data.
        */
       status = fbe_xor_rbld_parity_stamps_r6(scratch_p,
                                                eboard_p,
                                                sector_p,
                                                bitkey,
                                                width,
                                                rebuild_pos,
                                                parity_pos,
                                                seed,
                                                FBE_FALSE /* Don't check parity stamps. */);
       if (status != FBE_STATUS_OK) 
       {
           return status;
       }
        
       scratch_p->fatal_key = in_fatal_key;


        /* Make sure we write out the parity blocks.
         */
        eboard_p->m_bitmap |= parity_bitkey;

    }

    /* Set state to done so we will exit from epu.
     */
    scratch_p->scratch_state = FBE_XOR_SCRATCH_R6_STATE_DONE; 
    
    /* Clean up the fatal key for parity, since we reconstructed parity.
     * Remove the parity errors from fatal key and fatal count.
     * We also add in the modified bitmap for parity since if we are
     * reconstructing, it needs to be in the modified bitmap.
     */
    fatal_parity_bitkey = bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]];
    if ( scratch_p->fatal_key & fatal_parity_bitkey )
    {
        status = fbe_xor_scratch_r6_remove_error(scratch_p, fatal_parity_bitkey );
        if (status != FBE_STATUS_OK) 
        { 
            return status;
        }
        eboard_p->m_bitmap |= fatal_parity_bitkey;

        /* Correct all uncorrectables for this parity position.
         * We can do this since parity does not have uncorrectables,
         * as parity always gets reconstructed.
         */
        fbe_xor_correct_all_one_pos(eboard_p, fatal_parity_bitkey);
    }
    fatal_parity_bitkey = bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]];
    if ( scratch_p->fatal_key & fatal_parity_bitkey )
    {
        status = fbe_xor_scratch_r6_remove_error(scratch_p, fatal_parity_bitkey);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
        eboard_p->m_bitmap |= fatal_parity_bitkey;

        /* Correct all uncorrectables for this parity position.
         * We can do this since parity does not have uncorrectables,
         * as parity always gets reconstructed.
         */
        fbe_xor_correct_all_one_pos(eboard_p, fatal_parity_bitkey);
    }
    if ( parity_bitkey & dead_bitkey )
    {
        /* Clear any correctable parity errors on dead drives
         * from both u and c crc bitmask.
         * Drives we are rebuilding should not have crc errors logged
         * against them in this case.
         */
        eboard_p->c_crc_bitmap &= ~(dead_bitkey & parity_bitkey);
        eboard_p->u_crc_bitmap &= ~(dead_bitkey & parity_bitkey);
    }

    *bitmask_p = invalid_bitmap;
    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_xor_eval_parity_invalidate_r6()
 ****************************************/

/****************************************************************
 * fbe_xor_reconstruct_parity_r6()
 ****************************************************************
 * @brief
 *  This function reconstructs parity for a raid 6.
 *  This function reconstructs the parity of checksums (shed stamp).
 *  This function only initializes the timestamps and writestamps to
 *  FBE_SECTOR_R6_INVALID_TSTAMP and 0 respectively.
 *

 * @param sector_p - Vector of sectors.
 * @param bitkey - Each index in this vector has a bitmask which tells
 *                  us which position a corresponding index in
 *                  the sector array has.                 
 *                  In other words, this is the bit position to 
 *                  modify within eboard_p to save the error type.
 * @param width - Disks in array.
 * @param seed - Seed of block, which is usually the lba.
 * @param parity_pos - Index containing parity info.
 * @param rebuild_pos - Index to be rebuilt, -1 if no rebuild required.
 * @param data_position - Vector of data position phys->log mapping.
 *
 * @return fbe_status_t
 *  .
 *
 * ASSUMPTIONS:
 *  This function does not set the parity write stamp and time stamp.
 *  We assume the caller to this function will be setting the
 *  time stamp and write stamp values.
 *
 * @author
 *  05/10/06 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_xor_reconstruct_parity_r6( fbe_sector_t * const sector_p[],
                                      const fbe_u16_t bitkey[],
                                      const fbe_u16_t width,
                                      const fbe_lba_t seed,
                                      const fbe_u16_t parity_pos[],
                                      const fbe_u16_t data_position[])
{
    fbe_u16_t parity_drives = (parity_pos[1] == FBE_XOR_INV_DRV_POS) ? 1 : 2;
    fbe_u32_t pcsum = 0;           /* Running checksum of row parity.  */
    fbe_u32_t data_csum;          /* The current data block checksum. */
    fbe_s32_t disk_index;           /* Index of a data position.        */
    fbe_sector_t *row_parity_blk_ptr;  /* Pointer of row parity block.     */
    fbe_sector_t *data_blk_ptr;        /* Pointer of data block.           */
    fbe_sector_t *diag_parity_blk_ptr; /* Pointer of diagonal parity block.*/
    fbe_bool_t is_first_disk = FBE_TRUE;   /* Is this the first data disk in the strip? */
        
    /* We only support more than 1 parity drives.
     */
    if (XOR_COND(parity_drives <= 1))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "xor: %s parity_drives 0x%x <= 1\n",
                              __FUNCTION__, parity_drives);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Init the row and diagonal block pointers.
     */
    row_parity_blk_ptr = sector_p[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]];
    diag_parity_blk_ptr = sector_p[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]];

    /* Loop over all drives, calculating the row and diagonal parity.
     * This includes calculations for the parity and the parity of checksums.
     */
    for (disk_index = 0; disk_index < width; disk_index++)
    {
        /* If this disk is a parity disk, skip it.
         */
        if ((disk_index != parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]) &&
            (disk_index != parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]))
        {
            /* Get a ptr to the data sector.
             */
            data_blk_ptr = sector_p[disk_index];

            /* Next, either init parity if we are the first drive,
             * or update parity if we are a later drive.
             */
            if (is_first_disk)
            {
                data_csum = xorlib_calc_csum_and_init_r6_parity(data_blk_ptr->data_word,
                                                             row_parity_blk_ptr->data_word,
                                                             diag_parity_blk_ptr->data_word,
                                                             data_position[disk_index]); 

                /* Update our running total for row parity, and seed checksum.
                 */
                pcsum ^= data_csum;

                /* Initializes and calculates the row and diagonal parity
                 * of the 16 bit checksum value.
                 */
                fbe_xor_calc_parity_of_csum_init(&data_blk_ptr->crc,
                                             &row_parity_blk_ptr->lba_stamp,
                                             &diag_parity_blk_ptr->lba_stamp,
                                             data_position[disk_index]);
                is_first_disk = FBE_FALSE;
            }
            else
            {
                data_csum = xorlib_calc_csum_and_update_r6_parity(data_blk_ptr->data_word,
                                                               row_parity_blk_ptr->data_word,
                                                               diag_parity_blk_ptr->data_word,
                                                               data_position[disk_index]);

                /* Update our running total for row parity, and seed checksum.
                 */
                pcsum ^= data_csum;
            
                /* Otherwise update the parity of checksums 
                 * to include data drive's checksum.
                 */
                fbe_xor_calc_parity_of_csum_update(&data_blk_ptr->crc,
                                               &row_parity_blk_ptr->lba_stamp,
                                               &diag_parity_blk_ptr->lba_stamp,
                                               data_position[disk_index]);
            } /* end if is_first_disk. */
        
            /* We intentionally do not check or set the data checksum here
             * as we are not concerned with the data drive stamps.
             */

        } /* end if not a parity disk. */

    } /* End loop over all drives. */

    /* Set up the metadata in the parity sector.
     * We intentionally use the invalid R6 timestamp so
     * that it will be obvious we have written this strip,
     * AND because we don't know how many other drives we
     * will be writing.
     */
    row_parity_blk_ptr->time_stamp = FBE_SECTOR_R6_INVALID_TSTAMP;
    row_parity_blk_ptr->write_stamp = 0;
            
    /* On R6, the parity of checksums (shed stamp) was computed above.
     * Next, set the row parity checksum
     */
    row_parity_blk_ptr->crc = xorlib_cook_csum(pcsum, (fbe_u32_t)seed);
            
    /* Next, initialize the ts and ws for diagonal parity.
     * Also calculate and set the checksum for diagonal parity.
     */
    diag_parity_blk_ptr->time_stamp = FBE_SECTOR_R6_INVALID_TSTAMP;
    diag_parity_blk_ptr->write_stamp = 0;
    diag_parity_blk_ptr->crc = xorlib_cook_csum(
        xorlib_calc_csum(diag_parity_blk_ptr->data_word), (fbe_u32_t)seed);

    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_xor_reconstruct_parity_r6()
 ****************************************/

/**********************************************************************
 * fbe_xor_r6_eval_parity_of_checksums()
 **********************************************************************
 * 
 * @brief
 *  This function determines the state of the parity of checksums
 *  and will fix up these checksums if we determine that they are
 *  in an initialized state.  
 *  An initialized stripe has data = zeros, crc = 0x5eed, 
 *  timestamp = 0x7FFF, writestamp = 0 and shedstamp = 0.
 *
 *  The diagonal and row parities do not always compute to zero for
 *  a initialized stripe.  Thus, before we attempt to update the
 *  stripe with a 468, we need to set the PoC to a known good state.
 *  Thus when we finish updating the stripe for the first time,
 *  it will be consistent.
 *

 * @param row_parity_blk_ptr - Ptr to row parity.
 * @param diag_parity_blk_ptr - Ptr to diag parity.
 * @param array_width - Total width of the unit.
 * @param parity_drives - Number of parities.
 *
 * @return
 *  FBE_TRUE on success
 *  FBE_FALSE on failure.  
 *  Failure means that we should first verify this strip since there
 *  is some inconsistency in the strip.
 *  
 * @author
 *  05/11/06 - Created. Rob Foley
 *  
 *********************************************************************/
fbe_bool_t fbe_xor_r6_eval_parity_of_checksums(fbe_sector_t * row_parity_blk_ptr,
                                     fbe_sector_t * diag_parity_blk_ptr,
                                     fbe_u16_t array_width,
                                     fbe_u16_t parity_drives)
{
    fbe_u32_t stamps_initted_count = 0; /* Count of stamps that look initialized. */
    fbe_bool_t b_status = FBE_TRUE; /* This is our return status. */
    
    /* Increment the stamps_initted_count once for each
     * initialized set of stamps we find.  These stamps indicate
     * that the stripe is completely freshly bound.
     * If the parity stamps are in this state, then we will need
     * to fix up the parity of checksums.
     */
    if (row_parity_blk_ptr->time_stamp == FBE_SECTOR_INVALID_TSTAMP &&
        row_parity_blk_ptr->crc == 0x5eed &&
        row_parity_blk_ptr->lba_stamp == 0 &&
        row_parity_blk_ptr->write_stamp == 0)
    {
        stamps_initted_count++;
    }
    if (diag_parity_blk_ptr->time_stamp == FBE_SECTOR_INVALID_TSTAMP &&
        diag_parity_blk_ptr->crc == 0x5eed &&
        diag_parity_blk_ptr->lba_stamp == 0 &&
        diag_parity_blk_ptr->write_stamp == 0)
    {
        stamps_initted_count++;
    }

    if ( stamps_initted_count < 2 &&
         (row_parity_blk_ptr->time_stamp == FBE_SECTOR_INVALID_TSTAMP ||
          diag_parity_blk_ptr->time_stamp == FBE_SECTOR_INVALID_TSTAMP))
    {
        /* At least one set of parity stamps is not initted, 
         * but we found an invalid timestamp.  Meaning that at least one
         * of row or parity looks initialized.  
         * Something is obviously wrong, so force a verify.
         * The idea here is that we do not want to update parity if it is
         * not in a known good state.
         * This check catches inconsistencies including when only one
         * parity block looks like it is initialized.
         */
        b_status = FBE_FALSE;
    }
    
    if (stamps_initted_count == 2)
    {
        /* We found two parity stamps that were initialized.
         * We need to update the Parity of Checksums in order to
         * make it look valid for zeroed data.
         * This way, the parity of checksums for row and diagonal parity
         * will be in a known good state before the 468 code starts updating
         * the parity of checksums.
         */

        /* Get the pre-calculated parity of checksum for a zeroed 
         * strip from the below constant array.  We index into the 
         * array by the number of data drives and -1 since this is zero based.
         * Set both row and diagonal parity of checksums.
         */
        row_parity_blk_ptr->lba_stamp = 
            fbe_xor_raid6_globals.xor_R6_init_row_poc[array_width - parity_drives - 1];

        diag_parity_blk_ptr->lba_stamp = 
            fbe_xor_raid6_globals.xor_R6_init_diag_poc[array_width - parity_drives - 1];
    }
    
    return b_status;
}
/* end fbe_xor_r6_eval_parity_of_checksums() */

/*****************************************************************
 * fbe_xor_init_raid6_globals()
 *****************************************************************
 * @brief
 *  This function performs initialization of the raid6 globals.
 *  At this time we only init the parity of checksums arrays.
 *
 * @param None. 
 *
 * @return FBE_STATUS_OK on success. FBE_STATUS_GENERIC_FAILURE otherwise.
 *
 * @author
 *  05/11/06 - Created. Rob Foley
 *
 *****************************************************************/
fbe_status_t fbe_xor_init_raid6_globals(void)
{
    fbe_u16_t data_index = 0;
    /* Checksum is used below to calculate poc.
     */
    fbe_u16_t checksum = 0x5eed;
    /* Setup the row and diagonal parity of checksums with
     * zero initially.  Then setup the poc with a call
     * to fbe_xor_calc_parity_of_csum_init().
     */
    fbe_u16_t row_poc = 0;
    fbe_u16_t diag_poc = 0;

    /* Initialize the Parity of Checksums for the first data position.
     */
    fbe_xor_raid6_globals.xor_R6_init_row_poc[data_index] = row_poc;
    fbe_xor_raid6_globals.xor_R6_init_diag_poc[data_index] = diag_poc;
    fbe_xor_calc_parity_of_csum_init(&checksum, &row_poc, &diag_poc, data_index);
        
    /* For the rest of the widths, calculate the parity of checksum,
     * and save it in our globals.
     * We only need to calculate data disks worth, which is max frus - 2.
     */
    for ( data_index = 1; data_index < FBE_XOR_MAX_FRUS - 2; data_index++ )
    {
        fbe_xor_calc_parity_of_csum_update(&checksum, &row_poc, &diag_poc, data_index );
        fbe_xor_raid6_globals.xor_R6_init_row_poc[data_index] = row_poc;
        fbe_xor_raid6_globals.xor_R6_init_diag_poc[data_index] = diag_poc;
    }
    return FBE_STATUS_OK;
}
/******************************
 * fbe_xor_init_raid6_globals()
 ******************************/

/************************************************************
 *  fbe_xor_sector_history_trace_vector_r6()
 ************************************************************
 *
 * @brief
 *  Save a copy of this sector to the ktrace.
 *

 * @param lba - Lba of block.
 * @param sector_p - vector of sectors to trace. We assume it
 *                              is already mapped in.
 * @param pos_bitmask - vector of positions
 *                              1 << position in redundancy group.
 *  ppos[2],            [I] - Parity positions for this array.
 * @param raid_group_object_id - The object identifier of the raid group that
 *                               encountered the error
 * @param raid_group_offset - The offset for the raid group in error
 * @param error_string_p - Error string to display in trace.
 * @param tracing_level - If this tracing level is enabled
 *                              then we will display the block.
 *                              Defined in flare_debug_tracing.h
 *
 * @notes
 *  It is recommended that any trace levels with this tracing 
 *  should NOT be enabled by default in free builds, since  
 *  this mode of tracing can get very verbose and could
 *  wrap the standard ktrace buffer.
 *
 * @return fbe_status_t
 *  
 *
 * History:
 *  08/22/05:  Rob Foley -- Created.
 *  03/22/10:  Omer Miranda -- Updated to use the new sector tracing facility.
 *
 ************************************************************/

fbe_status_t fbe_xor_sector_history_trace_vector_r6( const fbe_lba_t lba, 
                                         const fbe_sector_t * const sector_p[],
                                         const fbe_u16_t pos_bitmask[],
                                         const fbe_u16_t width,
                                         const fbe_u16_t ppos[],
                                         const fbe_object_id_t raid_group_object_id,
                                         const fbe_lba_t raid_group_offset, 
                                         char *error_string_p,
                                         const fbe_sector_trace_error_level_t error_level,
                                         const fbe_sector_trace_error_flags_t error_flag )
{
    fbe_status_t status;
    char *display_err_string_p;
    fbe_u32_t array_position;
    
    /* If the correct tracing level is not enabled, just return
     * since there is nothing for us to do.
     */
    if ( !(fbe_sector_trace_is_enabled(error_level, error_flag)) )
    {
        return FBE_STATUS_OK;
    }

    /* Report the error.
     */
    status = fbe_xor_report_error(error_string_p, error_level, error_flag);
    if (status != FBE_STATUS_OK)
    {
        /* Return the status
         */
        return status;
    }

    /* Loop over the entire width, tracing all.
     */
    for ( array_position = 0; array_position < width; array_position++ )
    {
         char error_string[FBE_XOR_SECTOR_HISTORY_ERROR_MAX_CHARS];

         display_err_string_p = error_string_p;

        if (((ppos[FBE_XOR_R6_PARITY_POSITION_ROW] != FBE_XOR_INV_DRV_POS) && 
            (ppos[FBE_XOR_R6_PARITY_POSITION_ROW] == array_position)) ||
            ((ppos[FBE_XOR_R6_PARITY_POSITION_DIAG] != FBE_XOR_INV_DRV_POS) && 
            (ppos[FBE_XOR_R6_PARITY_POSITION_DIAG] == array_position)))
        {

            /* For parity display a bit more with the error string.
             */
            display_err_string_p = (char *)error_string;

            sprintf((char*) error_string, "parity %s", error_string_p);
            
        }
        
        status = fbe_xor_trace_sector( lba,  /* Lba */
                                  pos_bitmask[array_position], /* Bitmask of position in group. */
                                  0, /* The number of crc bits in error isn't known */
                                  raid_group_object_id,
                                  raid_group_offset, 
                                  sector_p[array_position], /* Sector to save. */
                                  display_err_string_p, /* Error type. */ 
                                  error_level, /* Trace level */
                                  error_flag /* error type */);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
    }
    
    /* Notify the sector trace service that we are done processing this error.
     */
    status = fbe_xor_trace_sector_complete(error_level, error_flag);

    return status;
}
/****************************************
 * end fbe_xor_sector_history_trace_vector_r6()
 ****************************************/


/***************************************************
 * end xor_raid6_util.c 
 ***************************************************/
