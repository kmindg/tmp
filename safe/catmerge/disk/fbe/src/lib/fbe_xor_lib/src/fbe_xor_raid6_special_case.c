/****************************************************************
 *  Copyright (C)  EMC Corporation 2006-2007
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 ****************************************************************/

/***************************************************************************
 * xor_raid6_special_case.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions to deal with the special cases that arise in
 *  RAID 6 operations.
 *
 * @notes
 *  Special Cases:
 *    - The strip has been zeroed, the data is zero and the stamps are in an
 *      initialized state.  The parity of checksums is incorrect so we need
 *      to handle it differently.
 *
 * @author
 *
 * 05/08/2006 - Created. RCL
 *
 **************************************************************************/

#include "fbe_xor_raid6.h"
#include "fbe_xor_raid6_util.h"
#include "fbe/fbe_sector.h"
#include "fbe_xor_epu_tracing.h"

/******************************
 * static FUNCTIONS
 ******************************/
static fbe_bool_t fbe_xor_raid6_is_data_block_zeroed( fbe_sector_t *const sector_p,
                                           fbe_bool_t *const b_fatal_p,
                                           const fbe_u16_t dkey,
                                           fbe_lba_t seed,
                                           fbe_lba_t offset);
static fbe_bool_t fbe_xor_raid6_is_parity_block_zeroed( const fbe_sector_t *const sector_p );

static fbe_bool_t fbe_xor_raid6_validate_data_stamps(fbe_sector_t * const dblk_p,
                                          const fbe_u16_t dkey,
                                          const fbe_lba_t seed,
                                          const fbe_lba_t offset);


/***************************************************************************
 *  fbe_xor_check_strip_just_bound()
 ***************************************************************************
 * @brief
 *   This routine determines if we think the strip is in a zeroed state.
 *   To be in this state means that the data is all zeros, the timestamp is
 *   0x7FFF, the CRC is 0x5eed, the shed stamp is 0x0, and the write stamp 
 *   is 0x0.
 *   In the context of the below, "fatal" means that there is no data
 *   for the given position either because it is dead or it is media errd.
 *
 * @param scratch_p - ptr to the scratch database area.
 * @param sector_p - ptr to the strip.
 * @param parity_pos - ptr to array of parity positions.
 * @param bitkey - bitkey for stamps/error boards.
 *   width          [I] - width of strip being checked.
 *
 *  @return
 *    FBE_TRUE if the strip is in a zeroed state.
 *    FBE_FALSE if the strip is in a normal state.
 *
 *  @author
 *    05/08/2006 - Created. RCL
 ***************************************************************************/
fbe_bool_t fbe_xor_check_strip_just_bound(const fbe_xor_scratch_r6_t * const scratch_p,
                                          fbe_sector_t * const sector_p[],
                                          const fbe_u16_t parity_pos[],
                                          const fbe_u16_t bitkey[],
                                          const fbe_u16_t width)
{
    /* The below variables are counts that we use to classify
     * the parity blocks.
     */
    fbe_u16_t parity_zeroed = 0;
    fbe_u16_t parity_not_zeroed = 0;
    fbe_u16_t parity_fatal = 0;

    fbe_xor_r6_parity_positions_t parity_index;
    fbe_bool_t b_zeroed = FBE_FALSE;
    
    /* Loop over every parity position, classifying each parity position.
     */
    for (parity_index = FBE_XOR_R6_PARITY_POSITION_ROW; parity_index < 2; parity_index++)
    {
        /* Classify a parity block as being either:
         * a) fatal if it is not there.
         * b) not zeroed if the block is not a zeroed block.
         * c) zeroed otherwise.
         */
        if ((scratch_p->fatal_key & bitkey[parity_pos[parity_index]]) != 0)
        {
            /* This parity block is fatal, so increment the counter.
             */
            parity_fatal++;
        }
        else if (!fbe_xor_raid6_is_parity_block_zeroed(sector_p[parity_pos[parity_index]]))
        {
            parity_not_zeroed++;
        }
        else
        {
            /* Zeroed. 
             */
            parity_zeroed++;
        }
    }/* End for loop over all parity positions. */

    /* Make sure the counts add up to 2 parities.
     */
    if (XOR_COND(parity_zeroed + parity_fatal + parity_not_zeroed != 2))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "parity_zeroed 0x%x + parity_fatal 0x%x + parity_not_zeroed 0x%x = 2\n",
                              parity_zeroed,
                              parity_fatal,
                              parity_not_zeroed);

        return FBE_STATUS_GENERIC_FAILURE;
    }
                


    /* If two parity not zeroed, we declare this not zeroed since we have agreement
     * between the two parities.
     */
    if (parity_not_zeroed == 2)
    {
        /* The case of both parity not zeroed is probably the most common
         * case since once we have written to a strip we will come here.
         * This is why this case is placed first.
         */
        if (XOR_COND(parity_zeroed != 0 || parity_fatal != 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "parity_zeroed  0x%x != 0 or parity_fatal 0x%x != 0\n",
                                  parity_zeroed,
                                  parity_fatal);

            return FBE_STATUS_GENERIC_FAILURE;
        }

    }
    /* If two parity are zeroed, then we declare this strip zeroed since
     * we have agreement between parities.
     */
    else if (parity_zeroed == 2)
    {
        /* Make sure the remaining counts are zero 
         * since we only have two parity.
         */
        if (XOR_COND((parity_not_zeroed != 0) || (parity_fatal != 0)))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "(parity_not_zeroed 0x%x != 0) || (parity_fatal 0x%x != 0)\n",
                                  parity_not_zeroed,
                                  parity_fatal);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        b_zeroed = FBE_TRUE;
    }
    /* If 1 fatal and 1 not zeroed, then we declare this not zeroed since the one
     * remaining parity says we are not zeroed.
     */
    else if (parity_not_zeroed == 1 && parity_fatal == 1)
    {
        /* Not zeroed, make sure the remaining count is zero 
         * since we only have two parity.
         */
        if (XOR_COND(parity_zeroed != 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "parity_zeroed 0x%x != 0\n",
                                  parity_zeroed);

            return FBE_STATUS_GENERIC_FAILURE;
        }
    }    
    /* For all other cases we now check the data sectors:
     * 1) 1 fatal parity      && 1 zeroed parity 
     *    0 data not zeroed && <= 1 data fatal
     *
     * 2) 1 not zeroed parity && 1 zeroed parity
     * 3) 2 fatal parity
     *   => Both of the above tolerate 0 non-zeroed or fatal data.
     */
    else
    {   
        fbe_u16_t sector_index;

        /* The below variables are counts that we use to classify the data blocks.
         */
        fbe_u16_t data_zeroed = 0;
        fbe_u16_t data_not_zeroed = 0;
        fbe_u16_t data_fatal = 0;

        /* Loop over all the data sectors, classifying each one.
         */
        for (sector_index = 0; sector_index < width; sector_index++)
        {
            /* Classify each data sector as either fatal,
             * not zeroed or zeroed.
             */
            if ( sector_index != parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW] &&
                 sector_index != parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG] )
            {
                fbe_bool_t b_fatal;
                
                if ((scratch_p->fatal_key & bitkey[sector_index]) != 0)
                {
                    data_fatal++;
                }
                else if (!fbe_xor_raid6_is_data_block_zeroed(sector_p[sector_index],
                                                         &b_fatal,
                                                         bitkey[sector_index],
                                                         scratch_p->seed,
                                                         scratch_p->offset) )
                {
                    if (b_fatal)
                    {
                        /* We will consider this block fatal,
                         * since it has a crc or stamp issue.
                         */
                        data_fatal++;
                    }
                    else
                    {
                        /* Block is not fatal, but not zeroed.
                         */
                        data_not_zeroed++;
                    }
                }
                else
                {
                    data_zeroed++;
                }
            } /* End if the sector is not a parity disk. */

            /* If we have more than one block fatal or more than 0
             * blocks not zeroed, break out.
             */
            if ( data_not_zeroed > 0 || data_fatal > 1 )
            {
                /* This is an optimization to get us out of the loop
                 * quicker if we know we aren't zeroed.
                 */
                break;
            }
        } /* End for loop over width. */
        
        /* If we have 1 fatal parity and 1 zeroed parity (0 not zeroed), 
         * then we will tolerate just 1 fatal data.
         * When we are rebuilding one data and one parity we want the strip
         * to remain zeroed.
         */
        if ( parity_zeroed == 1 && parity_fatal == 1 &&
             data_not_zeroed == 0 && data_fatal <= 1 )
        {
            /* We should not have any non-zeroed parity since the sum
             * of the three variables should be 2.
             */
            if (XOR_COND(parity_not_zeroed != 0))
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "parity_not_zeroed 0x%x != 0\n",
                                      parity_not_zeroed);

                return FBE_STATUS_GENERIC_FAILURE;
            }

            b_zeroed = FBE_TRUE;
        }
        /* This covers the case where one parity is zeroed and
         * one isn't zeroed (parity_fatal == 0).
         */
        else if ( parity_not_zeroed == 1 && parity_zeroed == 1 &&
                  data_not_zeroed == 0 && data_fatal <= 1 )
        {
            /* We assume we are zeroed here.  This allows us to
             * handle the case where we have a dead data drive and
             * a single media error/crc error/stamp error on parity.
             */
            if (XOR_COND(parity_fatal != 0))
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "parity_fatal 0x%x != 0\n",
                                      parity_fatal);

                return FBE_STATUS_GENERIC_FAILURE;
            }

            b_zeroed = FBE_TRUE; 
        }
        else if ( data_not_zeroed == 0 && data_fatal == 0 )
        {
            /* Otherwise we do not tolerate any non-zero or fatal data.
             * This covers the case where both parity are dead.
             */
            if (XOR_COND(parity_not_zeroed != 0 || parity_zeroed != 0 || parity_fatal != 2))
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "parity_not_zeroed != 0 || parity_zeroed != 0 || parity_fatal != 2\n");

                return FBE_STATUS_GENERIC_FAILURE;
            }
                
            b_zeroed = FBE_TRUE;
        }
        else
        {
            /* Not zeroed.
             */
        }
    } /* end else check data sectors. */

    return b_zeroed;
} /* fbe_xor_check_strip_just_bound() */

/***************************************************************************
 *  fbe_xor_raid6_verify_zeroed()
 ***************************************************************************
 * @brief
 *   This routine will verify that the whole strip contains the zeroed pattern
 *   of all zeros for data, the timestamp is 0x7fff, the CRC is 0x5eed, the 
 *   shed stamp is 0x0, and the write stamp is 0x0.
 *
 * @param scratch_p - ptr to the scratch database area.
 * @param sector_p - ptr to the strip.
 * @param bitkey - bitkey for stamps/error boards.
 *   width          [I] - width of strip being checked.
 * @param eboard_p - ptr to error reporting structure.
 * @param seed - Seed of current strip.
 * @param parity_pos[],  [I] - Parity position array.
 *
 *  @return fbe_status_t
 *
 *  @author
 *    05/08/2006 - Created. RCL
 ***************************************************************************/
fbe_status_t fbe_xor_raid6_verify_zeroed(fbe_xor_scratch_r6_t * const scratch_p,
                                         fbe_sector_t * const sector_p[],
                                         const fbe_u16_t bitkey[],
                                         const fbe_u16_t width,
                                         fbe_xor_error_t * const eboard_p,
                                         const fbe_lba_t seed,
                                         const fbe_u16_t parity_pos[])
{
    fbe_u16_t disk_index;
    fbe_u16_t row_index;
    fbe_u16_t cooked_csum = 0;
    fbe_u32_t * src_ptr;
    fbe_bool_t is_data_zeroes;
    fbe_status_t status;
    fbe_lba_t offset = scratch_p->offset;

    for (disk_index = 0; disk_index < width; disk_index++)
    {
        /* If this sector is up, make sure it is good.
         */
        if ((bitkey[disk_index] & scratch_p->fatal_key) == 0)
        {
            src_ptr = sector_p[disk_index]->data_word;

            is_data_zeroes = FBE_TRUE;

            XORLIB_PASSES16(row_index, FBE_WORDS_PER_BLOCK)
            {
                XORLIB_REPEAT8( ((*src_ptr++ != 0x0) 
                              ? is_data_zeroes = FBE_FALSE
                              : 0) );
                
            }
            
            /* CRC errors have higher priority than stamp errors.
             * If we have a CRC error, then don't look at the stamps or COH.
             * Stamp errors have a higher priority than COH errors, but
             * all stamp errors have the same priority.
             * If we find that any field does not match the zeroed pattern,
             * we will also mark this as a "zeroed" error by setting a bit
             * in the zeroed_bitmask.
             */
            if (sector_p[disk_index]->crc != 
                  (cooked_csum = xorlib_cook_csum(xorlib_calc_csum(sector_p[disk_index]->data_word),
                  (fbe_u32_t)seed)))
            {
                /* Add the details of errorneous sector detail in KTRACE if it not of 
                 * a known pattern. Examples of known patterns are:
                 *         1. Sector invalidated using rdt
                 *         2. Sector invalidated using ddt 
                 *         3. Sector invaldiated using rderr
                 * There is no need to log sector data for cases similar to above
                 * example.
                 */

                scratch_p->fatal_key |= bitkey[disk_index];
                eboard_p->c_crc_bitmap |= bitkey[disk_index];
                eboard_p->zeroed_bitmap |= bitkey[disk_index];

                /* Recognize the pattern so we can determine what kind of error this 
                 * is to avoid logging sector of known errorenous pattern.
                 *
                 * Note that fbe_xor_determine_csum_error() does not result in logging 
                 * of sector data in ktrace if lun is of RAID 6 type. It only initializes
                 * the bitmap vector used to identify the known CRC patterns in case
                 * of RAID 6.
                 */
                status = fbe_xor_determine_csum_error(eboard_p,
                                                      sector_p[disk_index],
                                                      bitkey[disk_index],
                                                      seed,
                                                      sector_p[disk_index]->crc,
                                                      cooked_csum,
                                                      FBE_TRUE);
                if (status != FBE_STATUS_OK) 
                { 
                    return status;
                }

                /* Log the sector detail if detected pattern is not a known pattern.
                 */
                status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p,
                                                   eboard_p,
                                                   eboard_p->c_crc_bitmap,
                                                   FBE_XOR_ERR_TYPE_CRC,
                                                   15,
                                                   FBE_FALSE);
                if (status != FBE_STATUS_OK) 
                { 
                    return status; 
                }
            }
            else if ((sector_p[disk_index]->write_stamp != 0x0) ||

                    /* Or it is a data position and has a invalid lba stamp. */
                      ((disk_index != parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]) && 
                        (disk_index != parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]) &&
                        (!xorlib_is_valid_lba_stamp(&sector_p[disk_index]->lba_stamp, seed, offset)))  ||

                    /* Or it is a parity position and doesn't have a shed stamp of 0. */
                      (((disk_index == parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]) || 
                        (disk_index == parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG])) &&
                       (sector_p[disk_index]->lba_stamp != 0)) ||

                    /* Or an invalid time stamp. */
                     (sector_p[disk_index]->time_stamp != FBE_SECTOR_INVALID_TSTAMP))
            {
                eboard_p->zeroed_bitmap |= bitkey[disk_index];

                if (sector_p[disk_index]->write_stamp != 0x0)
                {
                    scratch_p->fatal_key |= bitkey[disk_index];
                    eboard_p->c_ws_bitmap |= bitkey[disk_index];
                    status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_ws_bitmap, 
                                                       FBE_XOR_ERR_TYPE_WS, 3, FBE_FALSE);
                    if (status != FBE_STATUS_OK) 
                    { 
                        return status;
                    }
                }
                
                /* Check for a bad shed stamp. To do this, check if the LBA stamp is not legal, 
                 * or if its a parity drive, the shed is not 0. (0 is a legal LBA stamp)
                 */
                if ( /* A data position and has an invalid lba stamp. */
                      ((disk_index != parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]) && 
                        (disk_index != parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]) &&
                        (!xorlib_is_valid_lba_stamp(&sector_p[disk_index]->lba_stamp, seed, offset)))  ||

                     /* Or a Parity position and doesn't have a shed stamp of 0. */
                      (((disk_index == parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]) ||
                       (disk_index == parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG])) &&
                       (sector_p[disk_index]->lba_stamp != 0)))
                {
                    scratch_p->fatal_key |= bitkey[disk_index];

                    /* If it is a parity drive, log it as a poc error.
                     * If data drive, log as shed stamp error.
                     */
                    if ( disk_index == parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW] ||
                         disk_index == parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG] )
                    {
                        /* A parity position.
                         */
                        eboard_p->c_poc_coh_bitmap |= bitkey[disk_index];
                        status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_poc_coh_bitmap, 
                                                           FBE_XOR_ERR_TYPE_POC_COH, 8, FBE_FALSE);
                        if (status != FBE_STATUS_OK)
                        {
                            return status;
                        }
                    }
                    else
                    {
                        /* A data position, therefore this is a LBA stamp 
                         * error. Report as a checksum error.
                         */
                        eboard_p->c_crc_bitmap |= bitkey[disk_index];
                        eboard_p->crc_lba_stamp_bitmap |= bitkey[disk_index];
                        
                        /* Log the Error. 
                         */
                        status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->crc_lba_stamp_bitmap,
                                                           FBE_XOR_ERR_TYPE_LBA_STAMP, 15, FBE_FALSE);
                        if (status != FBE_STATUS_OK) 
                        {
                            return status;
                        }
                        eboard_p->zeroed_bitmap |= bitkey[disk_index];
                    }
                }

                if (sector_p[disk_index]->time_stamp != FBE_SECTOR_INVALID_TSTAMP)
                {
                    scratch_p->fatal_key |= bitkey[disk_index];
                    eboard_p->c_ts_bitmap |= bitkey[disk_index];
                    status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_ts_bitmap,
                                                       FBE_XOR_ERR_TYPE_TS, 10, FBE_FALSE);
                    if (status != FBE_STATUS_OK)
                    { 
                        return status;
                    }
                }
            }
            /* If the data was not all zeroes, mark it to be rebuilt.
             */
            else if (is_data_zeroes == FBE_FALSE)
            {
                scratch_p->fatal_key |= bitkey[disk_index];
                eboard_p->c_coh_bitmap |= bitkey[disk_index];
                status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_coh_bitmap,
                                                   FBE_XOR_ERR_TYPE_COH, 6, FBE_FALSE);
                if (status != FBE_STATUS_OK)
                { 
                    return status;
                }
                eboard_p->zeroed_bitmap |= bitkey[disk_index];
            }
        } /* end if sector is up. */

    } /* end for disk_index */

    return FBE_STATUS_OK;
} /* fbe_xor_raid6_verify_zeroed() */

/***************************************************************************
 *  fbe_xor_raid6_rebuild_zeroed()
 ***************************************************************************
 * @brief
 *   This routine sets all sectors marked with an error to the zeroed pattern
 *   of all zeros for data, the timestamp is 0x7fff, the CRC is 0x5eed, the 
 *   shed stamp is 0x0 if a parity position, the lba stamp if a data position
 *   and the write stamp is 0x0.
 *
 * @param scratch_p - ptr to the scratch database area.
 * @param sector_p - ptr to the strip.
 * @param bitkey - bitkey for stamps/error boards.
 * @param width - width of strip being checked.
 * @param parity_pos - Parity position array.
 * @param offset - The offset to be added to seed
 *
 *  @return
 *    None
 *
 *  @author
 *    05/08/2006 - Created. RCL
 ***************************************************************************/
void fbe_xor_raid6_rebuild_zeroed(fbe_xor_scratch_r6_t * const scratch_p,
                                  fbe_sector_t * const sector_p[],
                                  const fbe_u16_t bitkey[],
                                  const fbe_u16_t width,
                                  const fbe_u16_t parity_pos[],
                                  const fbe_lba_t offset)
{
    fbe_u16_t disk_index;
    fbe_u16_t row_index;
    fbe_u32_t * src_ptr;

    for (disk_index = 0; disk_index < width; disk_index++)
    {
        /* Check if this sector needs to be rebuilt, if so rebuild it.
         */
        if ((bitkey[disk_index] & scratch_p->fatal_key) != 0)
        {
            /* If we are rebuilding this sector to the zeroed state then set
             * the data to zeroes, the write stamp to 0x0, the shed stamp to 
             * 0x0, the crc to 0x5eed, and the time stamp to 0x7fff.
             */
            src_ptr = sector_p[disk_index]->data_word;

            XORLIB_PASSES16(row_index, FBE_WORDS_PER_BLOCK)
            {
                XORLIB_REPEAT8( (*src_ptr++ = 0x0) );
                
            }

            sector_p[disk_index]->write_stamp = 0x0;

            /* If a parity position set the shed stamp to 0.
             */
            if ((disk_index == parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]) ||
                (disk_index == parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]))
            {
                sector_p[disk_index]->lba_stamp = 0;
            }
            else
            {
                /* This is a data position, set the LBA stamp.
                 */
                sector_p[disk_index]->lba_stamp = xorlib_generate_lba_stamp(scratch_p->seed, offset);
            }

            sector_p[disk_index]->crc = 0x5eed;
            sector_p[disk_index]->time_stamp = FBE_SECTOR_INVALID_TSTAMP;
            scratch_p->fatal_key &= ~bitkey[disk_index];
        }

    }
    
    return;
} /* fbe_xor_raid6_rebuild_zeroed() */

/***************************************************************************
 *  fbe_xor_raid6_handle_zeroed_state()
 ***************************************************************************
 * @brief
 *   This function determines if the strip is in the zeroed state and verifies
 *   each sector.  If an error is found or a rebuild has been requested that
 *   is performed also.
 *

 * @param scratch_p - ptr to the scratch database area.
 * @param sector_p - ptr to the strip.
 * @param bitkey - bitkey for stamps/error boards.
 *   width          [I] - width of strip being checked.
 * @param eboard_p - error board for marking different types of errors.
 *   parity_pos[],  [I] - indexes containing parity info.
 *   rebuild_pos[]  [I] - indexes to be rebuilt, FBE_XOR_INV_DRV_POS in first 
 *                        position if no rebuild required.
 * @param seed - Seed of current strip.
 * @param zero_result_p  [O] - FBE_TRUE if the strip has been verified (and reconstructed).
 *                             FBE_FALSE the strip is not in the zeroed state.
 *
 * @return fbe_status_t
 *  
 *
 *  @author
 *    05/08/2006 - Created. RCL
 ***************************************************************************/
fbe_status_t fbe_xor_raid6_handle_zeroed_state(fbe_xor_scratch_r6_t * const scratch_p,
                                   fbe_sector_t * const sector_p[],
                                   const fbe_u16_t bitkey[],
                                   const fbe_u16_t width,
                                   fbe_xor_error_t *const eboard_p,
                                   const fbe_u16_t parity_pos[],
                                   fbe_u16_t rebuild_pos[],
                                   const fbe_lba_t seed,
                                   fbe_bool_t *zero_result_p)
{
    fbe_status_t status;
    fbe_bool_t is_strip_processed = FBE_FALSE;

    /* Check if this is the special case where the disks are just bound and
     * contain no data.
     */
    if (fbe_xor_check_strip_just_bound(scratch_p, sector_p, parity_pos, bitkey, width))
    {
        /* This strip is in the zeroed state.  Every disk will be checked for
         * this state and/or set to it.  If there are no errors make sure that 
         * everything is in the zeroed state.
         */
        status = fbe_xor_raid6_verify_zeroed(scratch_p, sector_p, bitkey, width, 
                                eboard_p, seed, parity_pos);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }

        /* If there are any disks marked in error just set them to the zeroed
         * pattern.
         */
        if (scratch_p->fatal_key != 0)
        {
            eboard_p->m_bitmap |= scratch_p->fatal_key;
            fbe_xor_raid6_rebuild_zeroed(scratch_p, sector_p, bitkey, width, parity_pos, eboard_p->raid_group_offset);
        }

        /* Clear out rebuild position u_crc_bitmap.
         */
        if (rebuild_pos[0] != FBE_XOR_INV_DRV_POS)
        {
            eboard_p->u_crc_bitmap &= ~(bitkey[rebuild_pos[0]]);
            if (rebuild_pos[1] != FBE_XOR_INV_DRV_POS)
            {
                eboard_p->u_crc_bitmap &= ~(bitkey[rebuild_pos[1]]);
            }
        }
        
        /* If there is a media error it is correctable.
         */
        eboard_p->u_crc_bitmap &= ~scratch_p->media_err_bitmap;
        eboard_p->c_crc_bitmap |= scratch_p->media_err_bitmap;
        if (scratch_p->media_err_bitmap)
        {
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_crc_bitmap,
                                               FBE_XOR_ERR_TYPE_HARD_MEDIA_ERR, 16, FBE_FALSE);
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }
        }

        /* If there is a retryable error it is correctable.
         * We do not mark it in any correctable bitmask since we do not want to log it. 
         */
        eboard_p->u_crc_bitmap &= ~scratch_p->retry_err_bitmap;
        eboard_p->zeroed_bitmap |= scratch_p->retry_err_bitmap;
        
        /* Were any checksum errors caused by media errors?
         *
         * This is information we need to setup in the eboard,
         * and this is the logical place to determine
         * which errors were the result of media errors.
         */
        status = fbe_xor_determine_media_errors( eboard_p, scratch_p->media_err_bitmap );
        if (status != FBE_STATUS_OK) 
        { 
            return status;
        }

        /* Any media errors we fixed here should also be noted as having
         * occurred in a zeroed strip.
         */
        eboard_p->zeroed_bitmap |= scratch_p->media_err_bitmap;
        
        if (XOR_COND(eboard_p->u_crc_bitmap != 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "eboard_p->u_crc_bitmap 0x%x != 0\n",
                                  eboard_p->u_crc_bitmap);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        is_strip_processed = FBE_TRUE;
    } /* end if fbe_xor_check_strip_just_bound */

    *zero_result_p = is_strip_processed;
    return FBE_STATUS_OK;
} /* fbe_xor_raid6_handle_zeroed_state() */

/***************************************************************************
 *  fbe_xor_raid6_is_parity_block_zeroed()
 ***************************************************************************
 * @brief
 *  This function determines if the block is in the zeroed state.
 *  Checks are made of the stamps as well as the words of the block itself.
 *

 * @param sector_p - Ptr to the sector to check.
 *
 * @return
 *  FBE_TRUE if the block is zeroed.
 *  FBE_FALSE if the block is not zeroed.
 *
 * ASSUMPTIONS:
 *     This MUST be a parity block. 
 *
 * @author
 *  07/10/2006 - Created. Rob Foley
 ***************************************************************************/ 
static fbe_bool_t fbe_xor_raid6_is_parity_block_zeroed( const fbe_sector_t *const sector_p )
{
    /* We initialize the return value to FBE_FALSE, and
     * set it to FBE_TRUE below if we find something zeroed.
     */
    fbe_bool_t b_zeroed = FBE_FALSE;
    
    if ((sector_p->time_stamp == FBE_SECTOR_INVALID_TSTAMP) &&
        (sector_p->crc == 0x5eed)&&
        (sector_p->write_stamp == 0x0) &&
        (sector_p->lba_stamp == 0x0))
    {
        fbe_u16_t row_index;
        const fbe_u32_t * src_ptr;
        fbe_bool_t b_is_data_zeroes = FBE_TRUE;
        
        /* If all the stamps look correct for this block, then
         * check the data of the block too.
         */
        src_ptr = (const fbe_u32_t*) sector_p->data_word;
        XORLIB_PASSES16(row_index, FBE_WORDS_PER_BLOCK)
        {
            XORLIB_REPEAT8( ((*src_ptr++ != 0x0) 
                          ? b_is_data_zeroes = FBE_FALSE
                          : 0) );
        }
        /* If the data is not all zeroes then set b_zeroed to false.
         */
        if (b_is_data_zeroes == FBE_TRUE)
        {
            b_zeroed = FBE_TRUE;
        }
    } /* end if stamps look zeroed. */
    else
    {
        /* Stamps do not look zeroed, mark block not zeroed.
         */
    }
    return b_zeroed;
} 
/* end fbe_xor_raid6_is_parity_block_zeroed() */

/***************************************************************************
 *  fbe_xor_raid6_is_data_block_zeroed()
 ***************************************************************************
 * @brief
 *  This function determines if the block is in the zeroed state.
 *  Checks are made of the stamps as well as the words of the block itself.
 *

 * @param sector_p - Ptr to the sector to check.
 * @param b_fatal_p - Ptr to boolean, FBE_TRUE if to be considered fatal,
 *                                   FBE_FALSE otherwise.
 * @param dkey - Bitmap for this data block.
 * @param seed - Seed for checking the LBA stamp.
 * @param offset - The raid group offset to be added to lba (seed)
 *
 *  ASSUMPTIONS:
 *      This MUST be a data block.
 *
 * @return
 *  FBE_TRUE if the block is zeroed.
 *  FBE_FALSE if the block is not zeroed.
 *
 * @author
 *  07/10/2006 - Created. Rob Foley
 ***************************************************************************/
static fbe_bool_t fbe_xor_raid6_is_data_block_zeroed( fbe_sector_t *const sector_p,
                                           fbe_bool_t *const b_fatal_p,
                                           const fbe_u16_t dkey,
                                           fbe_lba_t seed,
                                           fbe_lba_t offset)
{
    /* We initialize the return value to FBE_FALSE, and
     * set it to FBE_TRUE below if we find something zeroed.
     */
    fbe_bool_t b_zeroed = FBE_FALSE;
    fbe_u16_t row_index;
    const fbe_u32_t * src_ptr;
    fbe_bool_t b_is_data_zeroes = FBE_TRUE;
    fbe_u32_t csum = 0; /* For running csum. */
    
    if (XOR_COND(b_fatal_p == NULL))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "b_fatal_p == NULL\n");

        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    /* Init *b_fatal_p to FBE_TRUE.
     * We set it to FBE_FALSE below if it is not fatal.
     */
    *b_fatal_p = FBE_TRUE;

    /* Check the data of the block.
     */
    src_ptr = (const fbe_u32_t*) sector_p->data_word;
    XORLIB_PASSES16(row_index, FBE_WORDS_PER_BLOCK)
    {
        XORLIB_REPEAT8( (csum ^= *src_ptr, 
                      (*src_ptr++ != 0x0) 
                      ? b_is_data_zeroes = FBE_FALSE
                      : 0) );
    }

    if (b_is_data_zeroes == FBE_TRUE)
    {
        if ((sector_p->time_stamp == FBE_SECTOR_INVALID_TSTAMP) &&
            (sector_p->crc == 0x5eed) &&
            (sector_p->write_stamp == 0x0) &&
            (xorlib_is_valid_lba_stamp(&sector_p->lba_stamp, seed, offset)))
        {
            /* If the data is all zeroes and the stamps look zeroed,
             * then set b_zeroed to FBE_TRUE, since it matches our
             * requirements for being zeroed.
             */
            b_zeroed = FBE_TRUE;

            /* Not a fatal error since the block is zeroed.
             */
            *b_fatal_p = FBE_FALSE;

        } /* End if stamps zeroed. */
        else
        {
            /* When the data is zeros, we consider it a fatal error
             * if the stamps do not match the zeroed pattern.
             */
        }
    } /* end if b_is_data_zeroes == FBE_TRUE. */

    /* Else not zeroed.
     * If the sector checksum matches the calculated one, 
     * and the stamps are valid, then the block is not fatal.
     */
    else if ( sector_p->crc == xorlib_cook_csum(csum, 0 ) &&
              fbe_xor_raid6_validate_data_stamps( sector_p, dkey, seed, offset) )
    {
        /* Checksum and stamps are good, so not fatal, but not zeroed.
         */
        *b_fatal_p = FBE_FALSE;
    }
    return b_zeroed;
}
/* end fbe_xor_raid6_is_data_block_zeroed() */

/***************************************************************************
 *  fbe_xor_raid6_validate_data_stamps()
 ***************************************************************************
 * @brief
 *  This function determines if the data block stamps look correct.
 *

 * @param sector_p - Ptr to the data sector to check.
 * @param dkey - The bitkey for this data block.
 * @param seed - The seed for the LBA stamp.
 * @param offset - The raid group offset to add to seed
 *
 * @return
 *  FBE_TRUE if the block is valid.
 *  FBE_FALSE if the block is not valid.
 *
 * @author
 *  07/17/2006 - Created. Rob Foley
 ***************************************************************************/
static fbe_bool_t fbe_xor_raid6_validate_data_stamps(fbe_sector_t * const dblk_p,
                                          const fbe_u16_t dkey,
                                          const fbe_lba_t seed,
                                          const fbe_lba_t offset)
{
    fbe_bool_t b_valid = FBE_TRUE;
    
    if (!xorlib_is_valid_lba_stamp(&dblk_p->lba_stamp, seed, offset))
    {
        /* The LBA stamp is invalid.
         */
        b_valid = FBE_FALSE;
    }

    else if ((dblk_p->time_stamp & FBE_SECTOR_ALL_TSTAMPS) != 0)
    {
        /* The ALL_TIMESTAMPS bit should never be set
         * on a data sector.
         */
        b_valid = FBE_FALSE;
    }

    else if (dblk_p->write_stamp == 0)
    {
        /* No writestamp bits are set.
         */
    }

    else if (dblk_p->write_stamp != dkey)
    {
        /* The wrong bit[s] are set in the writestamp.
         */
        b_valid = FBE_FALSE;
    }

    else if (dblk_p->time_stamp != FBE_SECTOR_INVALID_TSTAMP)
    {
        /* The correct bit is set in the writestamp, but
         * the timestamp has not been invalidated accordingly.
         * This stamp looks incorrect.
         */
        b_valid = FBE_FALSE;
    }
    return b_valid;
}
/* end fbe_xor_raid6_validate_data_stamps() */

/* end xor_raid6_special_case.c */
