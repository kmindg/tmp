/****************************************************************
 *  Copyright (C)  EMC Corporation 2006
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 ****************************************************************/

/***************************************************************************
 * xor_raid6_reconstruct_2.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions to implement the reconstruct 2 state of the
 *  EVENODD algorithm which is used for RAID 6 functionality.
 *
 * @notes
 *
 * @author
 *
 * 04/06/06 - Created. NJI
 *
 **************************************************************************/

#include "fbe_xor_raid6.h"
#include "fbe_xor_raid6_util.h"
#include "fbe_xor_epu_tracing.h"

/***************************************************************************
 *  xor_rbld_stamp_one_parity()
 ***************************************************************************
 * @brief
 *   This funciton determines the time stamp & write stamp coherency
 *   across the stripe, the stamps in each data disk will be compared
 *   with this parity disk. If a problem is found, we'll mark the 
 *   proper uncorrectable bitmap.
 *
 * @param scratch_p - ptr to the scratch database area.
 * @param eboard_p - error board for marking different types 
 *                         of errors.
 * @param sector_p - vector of ptrs to sector contents.
 * @param bitkey - bitkey for stamps/error boards.
 * @param parity_bitmap - bitmap of the physical parity positions.
 * @param width - the width of the LUN.
 *   rebuild_pos[2],[I]  - indexes to be rebuilt, first position 
 *                         should always be valid.
 * @param parity_pos - the parity position we are checking for now.
 *
 *  @return
 *    FBE_TRUE:  if the reconstruct is successful.
 *    FBE_FALSE: if the reconstruct fails.
 *
 *  @author
 *    04/13/2006 - Created. NJI
 ***************************************************************************/
static fbe_bool_t fbe_xor_rbld_stamp_one_parity(fbe_xor_scratch_r6_t * const scratch_p, 
                                     fbe_xor_error_t * const eboard_p,
                                     fbe_sector_t * const sector_p[],
                                     const fbe_u16_t bitkey[], 
                                     const fbe_u16_t parity_bitmap,
                                     const fbe_u16_t width,
                                     const fbe_u16_t rebuild_pos[],
                                     const fbe_u16_t parity_pos,
                                     fbe_u16_t * const ret_ts_bitmap_p,
                                     fbe_u16_t * const ret_ws_bitmap_p)
{
    fbe_u16_t tstamp1; /* timestamp value if FBE_SECTOR_ALL_TSTAMPS is set */
    fbe_u16_t tstamp2; /* timestamp value if FBE_SECTOR_ALL_TSTAMPS isn't set */
    fbe_u16_t wstamp;  /* write stamp value from parity disk */

    /* 
     * bitmap of data disks not being rebuilt, 
     * doesn't contain any crc/stamp/media errors 
     */
    fbe_u16_t valid_data_disk_bitmap = 0; 
    fbe_u16_t ts_bitmap = 0; /* bitmap of bad timestamp disks */
    fbe_u16_t ws_bitmap = 0; /* bitmap of bad write stamp disks */
    fbe_u16_t bitkeys; /* union of bitmap of disks with bad stamps */
    fbe_bool_t can_rebuild;
    fbe_u16_t pos;

    /* Tracks if we have found a data drive that is valid.
     */
    fbe_bool_t b_found_data = FBE_FALSE;
    
    /*
     * Since we are comparing the data stamps with parity stamps,
     * make sure parity disk is available.
     */

    if (XOR_COND(parity_pos == FBE_XOR_INV_DRV_POS))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "parity_pos 0x%x == FBE_XOR_INV_DRV_POS 0x%x\n",
                              parity_pos,
                              FBE_XOR_INV_DRV_POS);

        return FBE_STATUS_GENERIC_FAILURE;
    }



    wstamp = sector_p[parity_pos]->write_stamp;
    tstamp1 = sector_p[parity_pos]->time_stamp;

    if (0 != (tstamp1 & FBE_SECTOR_ALL_TSTAMPS))
    {
        tstamp1 &= ~FBE_SECTOR_ALL_TSTAMPS;
        tstamp2 = tstamp1;
    }
    else
    {
        tstamp2 = FBE_SECTOR_INVALID_TSTAMP;
    }

    can_rebuild = FBE_TRUE;

    for (pos = 0; pos < width; pos++)
    {
        fbe_u16_t dkey;

        dkey = bitkey[pos];

        if (!(dkey & parity_bitmap)) 
        {
            /*
             * We're evaluating each valid data disk.
             */
            if (!(dkey & scratch_p->fatal_key))
            {
                fbe_sector_t *dblk_p;
                /* We found a data drive, so mark our boolean
                 * tracking this to FBE_TRUE.
                 */
                b_found_data = FBE_TRUE;
                
                /*
                 * Bits are set in the fatal key for these reasons:
                 * 1) initial rebuild position;
                 * 2) crc/media errors;
                 * 3) stamp errors.
                 *
                 * So these bits either exist from the entry of EPU,
                 * or were found in the previous passes through the 
                 * EPU loop.
                 */
                dblk_p = sector_p[pos];

                /* 
                 * Update valid_data_disk_bitmap with the current
                 * data disk.
                 */
                valid_data_disk_bitmap |= dkey;

                if ((dblk_p->time_stamp != tstamp1) &&
                    (dblk_p->time_stamp != tstamp2))
                {
                    /*
                     * Compare time stamp in this data block with
                     * possible values. If it doesn't match, then
                     * save this position in ts_bitmap indicating
                     * this is a mismatch of time stamps.
                     */
                    ts_bitmap |= dkey;
                }
                if (dblk_p->write_stamp != (wstamp & dkey))
                {
                    /*
                     * Compare write stamp in this data block with
                     * possible value. If it doesn't match, then
                     * save this position, since this is a mismatch
                     * of write stamp.
                     */
                    ws_bitmap |= dkey;
                }
            }
            else
            {
                /*
                 * If this is a fatal_blk, we are not going to
                 * evaluate the stamps, just skip it.
                 * We can have up to 2 fatal or rebuild positions.
                 */
            }
        }
        else
        {
            /*
             * We do not evaluate parity disk stamps since
             * we are comparing all stamps in data disks
             * to that in parity disk.
             */
        }
    } /* end of the for loop */

    /* Only perform the following check if data drives were
     * actually evaluated. Mismatches can only be detected
     * if we compared the data and parity stamps.
     */
    if (b_found_data)
    {
        /*
         * If stamps in all data disks mismatch the stamp
         * in parity, then set the ts_bitmap and ws_bitmap
         * to indicate that the parity is bad. 
         * Note the data stamps either matches each other
         * or not, but there is nothing more we can do.
         */
        if (ts_bitmap == valid_data_disk_bitmap)
        {
            ts_bitmap = bitkey[parity_pos];
        }
        if (ws_bitmap == valid_data_disk_bitmap)
        {
            ws_bitmap = bitkey[parity_pos];
        }
    }
    
    /*
     * Get the union uncorrectable bitmap for the new time stamp
     * and write stamp which mismatches this parity disk.
     */
    bitkeys = ts_bitmap | ws_bitmap;

    if (bitkeys != 0)
    {
        /*
         * No matter what, as long as we have a new error,
         * we have to switch to another state,
         * so always cannot rebuild from this state.
         * EPU will need to take this additional fatal error 
         * into account.
         */
        can_rebuild = FBE_FALSE;
    }
    else
    {
        /*
         * Since there's no stamp error, everything is good,
         * we can setup the stamps for the target disk being rebuilt.
         */
        fbe_sector_t *rblk_p;

        can_rebuild = FBE_TRUE;

        rblk_p = scratch_p->fatal_blk[0];

        /*
         * Setup the stamps for the 1st rebuild position.
         * shed_stamp is always the lba stamp for data disks in R6.
         * time_stamp is from the evaluation from above.
         * write_stamp is the bitkey position of this disk being
         * rebuilt.
         */
        rblk_p->lba_stamp = xorlib_generate_lba_stamp(scratch_p->seed, eboard_p->raid_group_offset);
        rblk_p->time_stamp = tstamp2;
        rblk_p->write_stamp = (wstamp & bitkey[rebuild_pos[0]]);

        /*
         * Need to check if we have a second disk being rebuilt.
         * If so, setup the stamps for this disk as well.
         */
        rblk_p = scratch_p->fatal_blk[1];
        if (rblk_p != NULL)
        {
            /*
             * Rebuild the stamps for the 2nd rebuild position.
             * It's very similar to the 1st rebuild position case.
             */
            rblk_p->lba_stamp = xorlib_generate_lba_stamp(scratch_p->seed, eboard_p->raid_group_offset);
            rblk_p->time_stamp = tstamp2;
            rblk_p->write_stamp = (wstamp & bitkey[rebuild_pos[1]]);
        }
    }

    /* Return both bitmaps.
     */
    *ret_ts_bitmap_p = ts_bitmap;
    *ret_ws_bitmap_p = ws_bitmap;
    
    /*
     * scratch_p->fatal_cnt has been updated,
     * so caller knows what to do next.
     */
    return can_rebuild;
}
/* fbe_xor_rbld_stamp_one_parity() */

/***************************************************************************
 *  xor_rbld_data_stamps()
 ***************************************************************************
 * @brief
 *   This function rebuilds stamps from all available parity disks.
 *

 * @param scratch_p - ptr to the scratch database area.
 * @param eboard_p - error board for marking different types 
 *                         of errors.
 * @param sector_p - vector of ptrs to sector contents.
 * @param bitkey - bitkey for stamps/error boards.
 * @param parity_bitmap - bitmap of the physical parity positions.
 * @param width - the width of the LUN.
 *   rebuild_pos[2],[I]  - physical indeces to be rebuilt, 
 *                         first position should always be valid.  
 *   parity_pos[2], [I]  - the 2 physical parity positions we are checking.
 * @param b_reconst_1 - FBE_TRUE if we are in reconstruct 1.
 *                         FBE_FALSE if reconstruct 2.
 * @param error_result_p - [O] - FBE_TRUE:  if no stamp errors found from all available parity disks.
 *                               FBE_FALSE: if there're stamp errors found.
 *
 *  @return fbe_status_t
 *
 *
 *  @notes
 *    This function assumes at least 1 parity disk is valid.  
 *
 *  @author
 *    04/13/2006 - Created. NJI
 *    03/22/2010 - Omer Miranda. Updated to use the new sector tracing facility.
 *    07/19/2012 - Vamsi V. Fail recons_1 rebuild if more than one stamp mismatch       
 *    11/20/2013 - Geng.Han. continue rebuild under the case where there is single fatal error on the data disk, 
 *                           an incomplete write on single parity and both parity disks are alive
 ***************************************************************************/
 fbe_status_t fbe_xor_rbld_data_stamps_r6(fbe_xor_scratch_r6_t * const scratch_p, 
                                          fbe_xor_error_t * const eboard_p,
                                          fbe_sector_t * const sector_p[],
                                          const fbe_u16_t bitkey[], 
                                          const fbe_u16_t parity_bitmap,
                                          const fbe_u16_t width,
                                          const fbe_u16_t rebuild_pos[],
                                          const fbe_u16_t parity_pos[],
                                          const fbe_bool_t b_reconst_1,
                                          fbe_bool_t * const error_result_p,
                                          fbe_bool_t * const needs_inv_p)
{
    fbe_bool_t status0 = FBE_FALSE;
    fbe_bool_t status1 = FBE_FALSE;
    fbe_u16_t row_ts_bitmap = 0;
    fbe_u16_t row_ws_bitmap = 0;
    fbe_u16_t diag_ts_bitmap = 0;
    fbe_u16_t diag_ws_bitmap = 0;
    fbe_bool_t b_rebuild_success = FBE_FALSE;
    fbe_u16_t diag_timestamp = 0;
    fbe_u16_t row_timestamp = 0;
    fbe_u16_t diag_writestamp = 0;
    fbe_u16_t row_writestamp = 0;
    fbe_u16_t ts_ws_mismatch_errs = 0;
    fbe_u32_t mismatch_count = 0;
    fbe_u32_t pos = 0;
    fbe_status_t fbe_status;
	fbe_bool_t b_needs_inv = FBE_FALSE;
    
    /* One parity should be alive if we are here.
     * In a free build, just return FBE_FALSE, since there is nothing to do.
     * In a checked build we assert here.
     */
    if ( (scratch_p->fatal_key & parity_bitmap) == parity_bitmap )
    {
        
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_CRITICAL,
                              "unexpected error : one parity drive must be alive"
                              "((scratch_p->fatal_key 0x%x & parity_bitmap 0x%x) == parity_bitmap)\n ",
                              scratch_p->fatal_key,
                              parity_bitmap);

        *error_result_p = FBE_FALSE;
        return FBE_STATUS_OK;
    }
    
    /*
     * Parity disk is used to verify the stamps in data disks,
     * so it has to be valid, and it cannot be the disk being
     * rebuilt.
     */
    if ((parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW] != FBE_XOR_INV_DRV_POS) &&
        ((scratch_p->fatal_key & bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]]) == 0))
    {
        /*
         * Rebuild the data stamps from the 1st parity disk.
         */
        status0 = fbe_xor_rbld_stamp_one_parity(scratch_p, eboard_p, 
                                            sector_p, bitkey, 
                                            parity_bitmap, 
                                            width, rebuild_pos, 
                                            parity_pos[0],
                                            &row_ts_bitmap, &row_ws_bitmap);

        row_timestamp = sector_p[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]]->time_stamp;
        row_writestamp = sector_p[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]]->write_stamp;
    }

    if ((parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG] != FBE_XOR_INV_DRV_POS) &&
        ((scratch_p->fatal_key & bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]]) == 0))
    {
        /*
         * Rebuild the data stamps from the 2nd parity disk.
         */
        status1 = fbe_xor_rbld_stamp_one_parity(scratch_p, eboard_p, 
                                            sector_p, bitkey, 
                                            parity_bitmap, 
                                            width, rebuild_pos, 
                                            parity_pos[1],
                                            &diag_ts_bitmap, &diag_ws_bitmap);

        diag_timestamp = sector_p[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]]->time_stamp;
        diag_writestamp = sector_p[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]]->write_stamp;
    }

    // in single fatal case: (for AR608994)
    // ts and ws on one parity matches all the alive data disks, but the other parity does not match
    // we only mark the mismatched parity as error
    if ((scratch_p->fatal_cnt == 1) && ((diag_ts_bitmap | diag_ws_bitmap) == 0) && ((row_ts_bitmap | row_ws_bitmap) != 0))
    {
        if (row_ts_bitmap != 0)
        {
            row_ts_bitmap = bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]];
            eboard_p->c_ts_bitmap |= row_ts_bitmap;
            fbe_status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_ts_bitmap,
                                                   FBE_XOR_ERR_TYPE_TS, 13, FBE_FALSE);
            if (fbe_status != FBE_STATUS_OK) 
            { 
                return fbe_status; 
            }
        }
        if (row_ws_bitmap != 0)
        {
            row_ws_bitmap = bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]];
            eboard_p->c_ws_bitmap |= row_ws_bitmap;
            fbe_status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_ws_bitmap,
                                                   FBE_XOR_ERR_TYPE_WS, 14, FBE_FALSE);
            if (fbe_status != FBE_STATUS_OK) 
            { 
                return fbe_status; 
            }
        }
        ts_ws_mismatch_errs |= (row_ts_bitmap | row_ws_bitmap);
    }
    else if ((scratch_p->fatal_cnt == 1) && ((row_ts_bitmap | row_ws_bitmap) == 0) && ((diag_ts_bitmap | diag_ws_bitmap) != 0))
    {
        if (diag_ts_bitmap != 0)
        {
            diag_ts_bitmap = bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]];
            eboard_p->c_ts_bitmap |= diag_ts_bitmap;
            fbe_status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_ts_bitmap,
                                                   FBE_XOR_ERR_TYPE_TS, 13, FBE_FALSE);
            if (fbe_status != FBE_STATUS_OK) 
            { 
                return fbe_status; 
            }
        }
        if (diag_ws_bitmap != 0)
        {
            diag_ws_bitmap = bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]];
            eboard_p->c_ws_bitmap |= diag_ws_bitmap;
            fbe_status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_ws_bitmap,
                                                   FBE_XOR_ERR_TYPE_WS, 14, FBE_FALSE);
            if (fbe_status != FBE_STATUS_OK) 
            { 
                return fbe_status; 
            }
        }
        ts_ws_mismatch_errs |= (diag_ts_bitmap | diag_ws_bitmap);
    }
    else
    {
        /* Compare the parity time stamps with each other and make sure they agree.
         * We should only try to make this check if both are there since
         * if one is dead, then one of them is always zero.
         */
        if ((scratch_p->fatal_key & parity_bitmap) == 0 && 
            ((diag_timestamp != row_timestamp) ||
             (diag_writestamp != row_writestamp)))
        {
            /* Lets look at the time stamps to see if there is a mismatch.
            */
            if (diag_timestamp != row_timestamp)
            {
                /* If the parity drive time stamps do not match
                 * match and we are in reconstruct 1 then we can fix these errors
                 * and continue with the reconstruct.
                 */
                if (b_reconst_1 &&
                    row_ts_bitmap == 0 && diag_ts_bitmap == 0 &&
                    row_ws_bitmap == 0 && diag_ws_bitmap == 0)
                {
                    /* The parity drive time stamps do not match, and yet the data positions
                     * show no issues with write or time stamps.
                     * This means that all the data drives are using write stamps.
                     * Change the parity time stamps to be the invalid R6 time stamp.
                     * Also mark a time stamp error.
                     */
                    if (row_timestamp != FBE_SECTOR_R6_INVALID_TSTAMP)
                    {
                        sector_p[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]]->time_stamp = 
                            FBE_SECTOR_R6_INVALID_TSTAMP;
                        eboard_p->c_ts_bitmap |= bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]];
                        fbe_status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_ts_bitmap,
                                                               FBE_XOR_ERR_TYPE_TS, 13, FBE_FALSE);
                        if (fbe_status != FBE_STATUS_OK) 
                        { 
                            return fbe_status; 
                        }
                    }
                    if (diag_timestamp != FBE_SECTOR_R6_INVALID_TSTAMP)
                    {
                        sector_p[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]]->time_stamp = 
                            FBE_SECTOR_R6_INVALID_TSTAMP;
                        eboard_p->c_ts_bitmap |= bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]];
                        fbe_status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_ts_bitmap,
                                                               FBE_XOR_ERR_TYPE_TS, 14, FBE_FALSE);
                        if (fbe_status != FBE_STATUS_OK) 
                        { 
                            return fbe_status; 
                        }
                    }
                }
                /* If the partner parity drive has marked itself as having a timestamp
                 * error lets believe it and not mark ourselves as having one.
                 */
                else if (((diag_ts_bitmap & bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]]) == 0) &&
                         ((row_ts_bitmap & bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]]) == 0))
                {
                    /* The time stamps do not match but we did not find a problem with the
                     * time stamps on the data drives.  Most likely we have a timestamp error
                     * that needs the dead drives to fix.  Mark the error in each tracking 
                     * bitmap so that the correct error will be reported.
                     */
                    row_ts_bitmap |= bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]];

                    diag_ts_bitmap |= bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]];

                    fbe_status = fbe_xor_sector_history_trace_vector_r6(scratch_p->seed,  /* Lba */
                                                                        (const fbe_sector_t * const *)sector_p, /* Sector vector to save. */
                                                                        bitkey, /* Bitmask of positions in group. */
                                                                        width, /* Width of group. */
                                                                        parity_pos,
                                                                        eboard_p->raid_group_object_id,
                                                                        eboard_p->raid_group_offset,
                                                                        "ts match", /* Error type. */
                                                                        FBE_SECTOR_TRACE_ERROR_LEVEL_ERROR, /* Trace level */
                                                                        FBE_SECTOR_TRACE_ERROR_FLAG_TS /* Trace error type */);

                    if (fbe_status != FBE_STATUS_OK) 
                    { 
                        return fbe_status; 
                    }
                }
                else if ((diag_ts_bitmap & bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]]) == 0)
                {
                    row_ts_bitmap |= bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]];
                }
                else if ((row_ts_bitmap & bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]]) == 0)
                {
                    diag_ts_bitmap |= bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]];
                }
            }

            /* Now lets look to see if the write stamps mismatch.
            */
            if (diag_writestamp != row_writestamp)
            {
                /* If the partner parity drive has marked itself as having a writestamp
                 * error lets believe it and not mark ourselves as having one.
                 */
                if ((diag_writestamp != row_writestamp) &&
                    ((diag_ws_bitmap & bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]]) == 0) &&
                    ((row_ws_bitmap & bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]]) == 0))
                {
                    /* The write stamps do not match but we did not find a problem with the
                     * write stamps on the data drives.  Most likely we have a write stamp error
                     * that needs the dead drives to fix.  Also we want to go through the code
                     * path below where row_ws_bitmap == diag_ws_bitmap so that both
                     * parities are marked as having errors.
                     */
                    row_ws_bitmap |= parity_bitmap;
                    diag_ws_bitmap |= parity_bitmap;

                    fbe_status = fbe_xor_sector_history_trace_vector_r6(scratch_p->seed,  /* Lba */
                                                                        (const fbe_sector_t * const *)sector_p, /* Sector vector to save. */
                                                                        bitkey, /* Bitmask of positions in group. */
                                                                        width, /* Width of group. */
                                                                        parity_pos,
                                                                        eboard_p->raid_group_object_id,
                                                                        eboard_p->raid_group_offset,
                                                                        "ws match", /* Error type. */
                                                                        FBE_SECTOR_TRACE_ERROR_LEVEL_ERROR, /* Trace level */
                                                                        FBE_SECTOR_TRACE_ERROR_FLAG_WS /* trace error type */);
                    if (fbe_status != FBE_STATUS_OK) 
                    { 
                        return fbe_status; 
                    }
                }
                else if ((diag_ws_bitmap & bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]]) == 0)
                {
                    row_ws_bitmap |= bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]];
                }
                else if ((row_ws_bitmap & bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]]) == 0)
                {
                    diag_ws_bitmap |= bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]];
                }
            }
        }

        if ( (row_ts_bitmap | row_ws_bitmap |
              diag_ts_bitmap | diag_ws_bitmap) == 0)
        {
            /* Nothing to do, no errors.
            */
            b_rebuild_success = FBE_TRUE;
        }
        else if ( (scratch_p->fatal_key & parity_bitmap) == 0 &&
                  ( row_ts_bitmap != diag_ts_bitmap ||
                    row_ws_bitmap != diag_ws_bitmap ) )
        {
            /* Both parities are alive (meaning that we can compare
             * the parity results to each other),
             * errors were found, and the bitmaps are not equal.
             */
            if (((row_ts_bitmap | row_ws_bitmap | diag_ts_bitmap | 
                  diag_ws_bitmap) & parity_bitmap) != 0)
            {
                if ((row_timestamp == diag_timestamp) &&
                    (row_ts_bitmap != diag_ts_bitmap))
                {
                    /* The timestamps on the parity drives match so it is unlikely that
                     * they are the problem.  The data drives that don't match the
                     * parities are probably wrong.
                     * Create a bitmask of remaining data drives that are probably
                     * the wrong ones.  Start with all the drives in the strip.
                     */
                    fbe_u16_t remaining_data_drive_bit = (0x1 << width) - 0x1; 

                    /* Remove the disks that match the parity timestamp.  Since each  
                     * parity put itself in its ts_bitmap the ts_bitmap includes all 
                     * the drives that matched the parity.
                     */
                    remaining_data_drive_bit ^= row_ts_bitmap | diag_ts_bitmap; 

                    /* Remove the dead drives, this leaves us with the remaining drives.
                    */
                    remaining_data_drive_bit ^= scratch_p->fatal_key; 

                    eboard_p->c_ts_bitmap |= remaining_data_drive_bit;
                    ts_ws_mismatch_errs |= remaining_data_drive_bit;
                    fbe_status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_ts_bitmap, 
                                                           FBE_XOR_ERR_TYPE_TS, 12, FBE_FALSE);    
                    if (fbe_status != FBE_STATUS_OK) 
                    { 
                        return fbe_status; 
                    }
                }
                else if (row_ts_bitmap != diag_ts_bitmap)
                {
                    /* One or both of the parities were identified as having an error.
                     * Update eboards that mismatch with parity.
                     */
                    eboard_p->c_ts_bitmap |= row_ts_bitmap | diag_ts_bitmap;
                    ts_ws_mismatch_errs |= row_ts_bitmap | diag_ts_bitmap;
                    fbe_status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_ts_bitmap,
                                                           FBE_XOR_ERR_TYPE_TS, 1, FBE_FALSE);
                    if (fbe_status != FBE_STATUS_OK) 
                    { 
                        return fbe_status; 
                    }
                }

                if ((row_writestamp == diag_writestamp) &&
                    (row_ws_bitmap != diag_ws_bitmap))
                {
                    /* The write stamps on the parity drives match so it is unlikely that
                     * they are the problem.  The data drives that don't match the
                     * parities are probably wrong.
                     * Create a bitmask of remaining data drives that are probably
                     * the wrong ones.  Start with all the drives in the strip.
                     */
                    fbe_u16_t remaining_data_drive_bit = (0x1 << width) - 0x1; 

                    /* Remove the disks that match the parity write stamp.  Since each  
                     * parity put itself in its ts_bitmap the ws_bitmap includes all 
                     * the drives that matched the parity.
                     */
                    remaining_data_drive_bit ^= row_ws_bitmap | diag_ws_bitmap; 

                    /* Remove the dead drives, this leaves us with the remaining drives.
                    */
                    remaining_data_drive_bit ^= scratch_p->fatal_key; 

                    eboard_p->c_ws_bitmap |= remaining_data_drive_bit;
                    ts_ws_mismatch_errs |= remaining_data_drive_bit;
                    fbe_status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_ws_bitmap, 
                                                           FBE_XOR_ERR_TYPE_WS, 7, FBE_FALSE);    
                    if (fbe_status != FBE_STATUS_OK) 
                    { 
                        return fbe_status; 
                    }
                }
                else if (row_ws_bitmap != diag_ws_bitmap) 
                {
                    /* One or both of the parities were identified as having an error.
                     * Update eboards that mismatch with parity.
                     */
                    eboard_p->c_ws_bitmap |= row_ws_bitmap | diag_ws_bitmap;
                    ts_ws_mismatch_errs |= row_ws_bitmap | diag_ws_bitmap;
                    fbe_status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_ws_bitmap, 
                                                           FBE_XOR_ERR_TYPE_WS, 0, FBE_FALSE);
                    if (fbe_status != FBE_STATUS_OK) 
                    { 
                        return fbe_status; 
                    }
                }

            }
            else
            {
                /* We assume both parities are bad in this case.
                */
                fbe_xor_scratch_r6_add_error(scratch_p, 
                                             bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]]);
                fbe_xor_scratch_r6_add_error(scratch_p, 
                                             bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_DIAG]]);

                /* Update eboards that mismatch with parity.
                */
                if ( row_ts_bitmap != diag_ts_bitmap )
                {
                    eboard_p->u_ts_bitmap |= parity_bitmap;
                    fbe_status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_ts_bitmap,
                                                           FBE_XOR_ERR_TYPE_TS, 2, FBE_TRUE);
                    if (fbe_status != FBE_STATUS_OK) 
                    { 
                        return fbe_status; 
                    }
                }
                if ( row_ws_bitmap != diag_ws_bitmap )
                {
                    eboard_p->u_ws_bitmap |= parity_bitmap;
                    fbe_status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_ws_bitmap, 
                                                           FBE_XOR_ERR_TYPE_WS, 1, FBE_TRUE);
                    if (fbe_status != FBE_STATUS_OK) 
                    { 
                        return fbe_status; 
                    }
                }
            }
            /* Rebuild failed since both parities look incorrect.
            */
        }
        else /* if ( (scratch_p->fatal_key & parity_bitmap) != 0 ||
              *      (row_ts_bitmap == diag_ts_bitmap &&
              *       row_ws_bitmap == diag_ws_bitmap) )
              */
        {
            
            /* Either one of the parity is down, meaning that we cannot
             * compare the parities to each other,
             * or bitmaps are equal, meaning that paritys are
             * consistent with each other.  We can add these errors to the eboard.
             */
            /* Depending on which is dead, use the appropriate bitmask.
            */
            if ( scratch_p->fatal_key &
                 bitkey[parity_pos[FBE_XOR_R6_PARITY_POSITION_ROW]] )
            {
                /* Row parity is dead, add the Diag bitmaps to the eboard.
                */
                eboard_p->c_ts_bitmap |= diag_ts_bitmap;
                fbe_status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_ts_bitmap,
                                                       FBE_XOR_ERR_TYPE_TS, 3, FBE_FALSE);
                if (fbe_status != FBE_STATUS_OK) 
                { 
                    return fbe_status; 
                }

                eboard_p->c_ws_bitmap |= diag_ws_bitmap;
                fbe_status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_ws_bitmap,
                                                       FBE_XOR_ERR_TYPE_WS, 2, FBE_FALSE);
                if (fbe_status != FBE_STATUS_OK) 
                { 
                    return fbe_status; 
                }
                ts_ws_mismatch_errs |= diag_ts_bitmap | diag_ws_bitmap;
            }
            else
            {
                /* Row parity is alive, so we can use it.
                 * If diagonal parity is alive, then the bitmasks are equal.
                 */
                eboard_p->c_ts_bitmap |= row_ts_bitmap;
                fbe_status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_ts_bitmap, 
                                                       FBE_XOR_ERR_TYPE_TS, 4, FBE_FALSE);
                if (fbe_status != FBE_STATUS_OK) { return fbe_status; }

                eboard_p->c_ws_bitmap |= row_ws_bitmap;
                fbe_status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->c_ws_bitmap, 
                                                       FBE_XOR_ERR_TYPE_WS, 8, FBE_FALSE);
                if (fbe_status != FBE_STATUS_OK) 
                { 
                    return fbe_status; 
                }
                ts_ws_mismatch_errs |= row_ts_bitmap | row_ws_bitmap;
            }
            /* Rebuild failed since we have ts or ws errors.
            */
        }
    }

    /* If we have mismatch errors, then sum them.
     */
    if ( ts_ws_mismatch_errs != 0 )
    {
        /* Count up how many mistach errors we have.
         */
        for (pos = 0; pos < width; pos++)
        {
            if (ts_ws_mismatch_errs & bitkey[pos])
            {
                /* Count all of the mismatch errors.
                 */
                mismatch_count++;
            }
        } /* End for all positions in width. */

    } /* End if stamp mismatch errors found. */
    
    /* If we are in reconstruct 1, we tolerate additional stamp errors
     * in order to attempt reconstruction.  If there are any
     * coherency errors then the reconstruct 1 is guaranteed to fail.
     */
    if (scratch_p->scratch_state == FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_1)
    {
        if (mismatch_count == 0)
        {
            /* We have either no mismatches and can continue in reconstruct 1 */
            b_rebuild_success = FBE_TRUE;
        }
        else if (scratch_p->fatal_cnt == 1)
        {
            if (mismatch_count == 1)
            {
                /* If we only have one error in reconstruct 1, add it to the scratch.
                 */
                fbe_xor_scratch_r6_add_error(scratch_p, ts_ws_mismatch_errs );
                scratch_p->stamp_mismatch_pos = ts_ws_mismatch_errs;

                if ( ts_ws_mismatch_errs & parity_bitmap )
                {
                    /* If the error was on parity, we should continue with reconstruct 1,
                     * and then go to reconstruct parity.
                     */
                    b_rebuild_success = FBE_TRUE;
                } /* End if the error was on parity. */
                else
                {
                    /* Return FBE_FALSE, since we need to go to reconstruct 2.
                     */
                    b_rebuild_success = FBE_FALSE;
                }
            }
            else
            {
                /* We have too many stamp errors for rebuilding the block
                 */
                b_rebuild_success = FBE_FALSE;
                b_needs_inv = FBE_TRUE;
            }
        }
        else if (scratch_p->fatal_cnt == 2)
        {
            /* We have too many stamp errors for rebuilding the blocks
             */
			b_rebuild_success = FBE_FALSE;
			b_needs_inv = FBE_TRUE;
        }/* end if no mismatches or too many. */

        if (b_rebuild_success == FBE_TRUE && 
            status0 == FBE_FALSE && status1 == FBE_FALSE)
        { 
            /* It is possible that we did not rebuild stamps
             * for this drive yet if both calls above to reconstruct stamps
             * failed for this drive.
             * There is an assumption that the stamps for this drive
             * have been reconstructed prior to us leaving.
             * Setup the stamps to default values.
             */
            fbe_sector_t *rblk_p = scratch_p->fatal_blk[0];
            
            /* Setup the stamps for the 1st rebuild position.
             * shed_stamp is set to our lba stamp since this is a data position.
             * time_stamp is invalid.
             * write_stamp is zero.
             */
            rblk_p->lba_stamp = xorlib_generate_lba_stamp(scratch_p->seed, eboard_p->raid_group_offset);
            rblk_p->time_stamp = FBE_SECTOR_INVALID_TSTAMP;
            rblk_p->write_stamp = 0;
        }/* End rebuild data stamps. */

    } /* end if reconstruct 1 */
    else if (scratch_p->scratch_state == FBE_XOR_SCRATCH_R6_STATE_RECONSTRUCT_2 &&
             mismatch_count > 0)
    {
        /* If we are in reconstruct 2 and there are any stamp errors,
         * we should fail the reconstruct, since there is no
         * guarantee we can reconstruct valid data.
         */
        b_rebuild_success = FBE_FALSE;
        b_needs_inv = FBE_TRUE;
    }
    
    /*
     * Returns FBE_TRUE if at least one rebuild stamp success.
     */
    *error_result_p = b_rebuild_success;
    *needs_inv_p = b_needs_inv;

    return FBE_STATUS_OK;
}
/* fbe_xor_rbld_data_stamps_r6() */


/***************************************************************************
 * fbe_xor_reconst_2_check_csum()
 ***************************************************************************
 * @brief
 *  This function checks the checksums on the 2 reconstructed blocks
 *  and handles the necessary errors.
 *
 * @param scratch_p - scratch containing row & diagonal syndroms.
 * @param eboard_p - error board for marking different types 
 * @param bitkey - bitkey for stamps/error boards.
 *   rebuild_pos[2], [I]  - physical indeces to be rebuilt, first 
 *                          position should always be valid.
 *   init_rpos[2],   [I] - The initial rebuild positions
 *                         to be rebuilt.
 *   parity_pos[2],  [I]  - the 2 physical parity positions.
 * @param rpos - Position being rebuilt, 0 or 1.
 & @param checksum_result_p [O] - checksume result of reconstructed blocks
 *
 * @return fbe_status_t
 *  
 *
 * @author
 *  08/04/06 - Created. Rob Foley
 ***************************************************************************/

static fbe_status_t fbe_xor_reconst_2_check_csum(fbe_xor_scratch_r6_t * const scratch_p,
                                               fbe_xor_error_t * const eboard_p,
                                               const fbe_u16_t bitkey[], 
                                               const fbe_u16_t rebuild_pos[],
                                               const fbe_u16_t init_rpos[],
                                               const fbe_u16_t parity_pos[],
                                               const fbe_u16_t rpos,
                                               fbe_bool_t *checksum_result_p)
{
    fbe_bool_t result = FBE_TRUE;
    fbe_status_t status = FBE_STATUS_OK;

    /* Compare the checksum from the data with the one from PoC
     * (Which is located in fatal_blk->crc).
     * If they don't match, we have a poc coherency error.
     */
    if (xorlib_cook_csum(xorlib_calc_csum(scratch_p->fatal_blk[rpos]->data_word),
                      (fbe_u32_t)scratch_p->seed) != scratch_p->fatal_blk[rpos]->crc )
    {
        /* Determine the types of checksum error.
         */
        status = fbe_xor_determine_csum_error( eboard_p, 
                                               scratch_p->fatal_blk[rpos], 
                                               bitkey[rebuild_pos[rpos]], 
                                               scratch_p->seed,
                                               0,
                                               0,
                                               FBE_TRUE);
        if (status != FBE_STATUS_OK) 
        {
            return status; 
        }

        /* Check if the sector has been invalidated on purpose.  If it has been
         * invalidated on purpose then report it as a crc error.  If not then
         * there is something wrong with the data so report it as a coherency
         * error.
         */
        if (((eboard_p->crc_invalid_bitmap & bitkey[rebuild_pos[rpos]]) != 0) ||
            ((eboard_p->crc_raid_bitmap & bitkey[rebuild_pos[rpos]]) != 0)    ||
            ((eboard_p->corrupt_crc_bitmap & bitkey[rebuild_pos[rpos]]) != 0)    )
        {
            /* This error is due to an invalidated sector mark it as a crc error.
             */
            eboard_p->u_crc_bitmap |= bitkey[rebuild_pos[rpos]];

            /* Correct any errors that could have caused us to get here.
             * Leave the crc error unchanged since it is still uncorrectable.
             */
            fbe_xor_correct_all_non_crc_one_pos(eboard_p, bitkey[rebuild_pos[rpos]]);
            
            /* We don't log errors for previously invalidated sectors. So pass
             * 0 for the bitmap.
             */
            status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, 0, 
                                               FBE_XOR_ERR_TYPE_CRC, (rpos == 0) ? 13 : 14, FBE_TRUE);
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }
        }
        else
        {
            if ( scratch_p->stamp_mismatch_pos != FBE_XOR_INV_DRV_POS &&
                 ((scratch_p->fatal_key & scratch_p->stamp_mismatch_pos) == scratch_p->stamp_mismatch_pos ) )
            {
                /* If we came from reconstruct 1 originally and also had a single stamp error,
                 * then we should not invalidate the stamp error position, since it might
                 * be due to an incomplete write.
                 * We only take this error out if it has not already been taken out by the
                 * first pass of reconstruct 2.
                 */
                status = fbe_xor_scratch_r6_remove_error(scratch_p, scratch_p->stamp_mismatch_pos);
                if (status != FBE_STATUS_OK) 
                { 
                    return status; 
                }
            }
            else
            {
                /* We found the checksum from data doesn't match the checksum
                 * from PoC from the parity disk, so set it as PoC coherency 
                 * error. 
                 * Since this is the rebuild 2 case, there's no much we can 
                 * do for the total of 3 errors.
                 * Set the status to be false so caller will exit gracefully.
                 */    
                eboard_p->u_poc_coh_bitmap |= bitkey[rebuild_pos[rpos]];
                status = FBE_XOR_EPU_TRC_LOG_ERROR(scratch_p, eboard_p, eboard_p->u_poc_coh_bitmap,
                                      FBE_XOR_ERR_TYPE_POC_COH, (rpos == 0) ? 5 : 6, FBE_TRUE);
                if (status != FBE_STATUS_OK) 
                { 
                    return status; 
                }
            }
        }
        result = FBE_FALSE;
    }
    else
    {
        /* We are able to rebuild this data disk, so
         * clean up all the flags here.
         */
        status = fbe_xor_scratch_r6_remove_error(scratch_p, bitkey[rebuild_pos[rpos]]);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
        fbe_xor_correct_all_one_pos(eboard_p, bitkey[rebuild_pos[rpos]]);
        eboard_p->m_bitmap |= bitkey[rebuild_pos[rpos]];

        /* Clear the c_crc_bitmap for the rebuild 
         * position since the rebuild succeeded.
         */
        if (init_rpos[0] == rebuild_pos[rpos] ||
            init_rpos[1] == rebuild_pos[rpos])
        {
            eboard_p->c_crc_bitmap &= ~bitkey[rebuild_pos[rpos]];
        }
    }

    *checksum_result_p = result;
    return FBE_STATUS_OK;
}
/* end fbe_xor_reconst_2_check_csum() */

/***************************************************************************
 * fbe_xor_eval_reconstruct_2()
 ***************************************************************************
 * @brief
 *  This function reconstructs 2 missing data disk sectors from parity,
 *  as well as their checksums from PoC in the parity disks.
 *

 * @param scratch_p - scratch containing row & diagonal syndroms.
 * @param data_pos_0 - 1st logical position of the data disk 
 *                          being reconstructed.
 * @param data_pos_1 - 2nd logical position of the data disk 
 *                          being reconstructed.
 * @param eboard_p - error board for marking different types 
 * @param bitkey - bitkey for stamps/error boards.
 *   rebuild_pos[2], [I]  - physical indeces to be rebuilt, first 
 *                          position should always be valid.
 *   init_rpos[2],   [I] - The initial rebuild positions
 *                         to be rebuilt.
 *   parity_pos[2],  [I]  - the 2 physical parity positions.
 * @param reconstruct_result_p    [O} - result of reconstructed data

 * @return fbe_status_t
 *
 * @notes
 *  Please refer to page 196 in <<EVENODD: An Efficient Scheme for 
 *  Tolerating Double Disk Failures in RAID Architectures>> 
 *  by Mario Blaum, 1994.
 *
 * Basically there're 3 steps,
 *
 *  1. set s = mod(-(data_pos_1 - data_pos_0) - 1);
 *     for 0 <= l <= (m-1), set a[m-1, l] = 0;
 *  2. set data_symbol[1][s] = sydnrome_symbol_1[mod(j+s)] ^
 *                            data_symbol[0][mod(s+j-i)];
 *     set data_symbol[0][s] = data_syndrome[0][s] ^ 
 *                            data_symbol[1][s];
 *  3. set s = mod(-(data_pos_1 - data_pos_0) - 1);
 *     go back to step 2, loop untill the end.
 *
 *
 *  This function assumes the syndromes and S value has been 
 *  calculated already, but the syndromes don't contain the
 *  S value in them.
 *
 * @author
 *  03/24/06 - Created. NJI
 ***************************************************************************/
 fbe_status_t fbe_xor_eval_reconstruct_2(fbe_xor_scratch_r6_t * const scratch_p,
                            const fbe_u16_t data_pos_0,
                            const fbe_u16_t data_pos_1,
                            fbe_xor_error_t * const eboard_p,
                            const fbe_u16_t bitkey[], 
                            const fbe_u16_t rebuild_pos[],
                            const fbe_u16_t init_rpos[],
                            const fbe_u16_t parity_pos[],
                            fbe_bool_t *reconstruct_result_p)
{
    /* 
     * Data & parity syndromes calculated from all available disks.
     */
    fbe_u32_t *data_syndrome[2]; 
    fbe_u32_t csum_syndrome[2] = {scratch_p->checksum_row_syndrome,
                                scratch_p->checksum_diag_syndrome}; 
    fbe_status_t fbe_status;
    fbe_bool_t check_csum;
    
    /* 
     * These are the variables used to keep the data and checksum
     * gets reconstructed for the 2 faulted disks.
     * For data we start with the location of first data symbol.
     * Then we use them to offset other symbols in the block.
     * For checksum, each bit represents 1 symbol.
     */
    fbe_u32_t *first_data_symbol[2];
    fbe_u32_t block_csum[2] = {0};

    /*
     * Symbol indeces used to reconstruct the data and checksum.
     */ 
    fbe_u32_t symbol_index[2];

    /* 
     * Each time we work on 1 data symbol and 1 checksum symbol.
     * The following variables are the symbols
     * we are currently working on.
     */
    fbe_u32_t *data_symbol[2];
    fbe_u32_t csum_symbol[2];

    /* 
     * Intermediate variables used for data & checksum reconstruct. 
     */
    fbe_u32_t csum_bit_copy = 0;
    fbe_u32_t *data_symbol_copy;
    fbe_u32_t dummy_data_symbol[FBE_XOR_WORDS_PER_SYMBOL]={0};

    /*
     * The holder for the data s value which is calculated
     * from row and diagonal parity disks.
     */
    fbe_u32_t *s_value_holder;

    /*
     * Status return to the caller.
     */
    fbe_bool_t status = FBE_TRUE;
    
    /*
     * Find the location where we'll save the rebuilt data.
     */
    first_data_symbol[0] = &scratch_p->fatal_blk[0]->data_word[0];
    first_data_symbol[1] = &scratch_p->fatal_blk[1]->data_word[0];

    /*
     * If the s value for parity of checksum is 1,
     * we need to apply 1 to all 16 bits for diagonal
     * checksum syndrome.
     */
    if (scratch_p->s_value_for_checksum == 1)
    {
        csum_syndrome[1] ^= 0x1ffff;
    }

    /*
     * Step 1 in page 196, get the s.
     */
    symbol_index[0] = FBE_XOR_MOD_VALUE_M(data_pos_0 - data_pos_1 - 1 + 
                                      FBE_XOR_EVENODD_M_VALUE, 
                                      FBE_XOR_EVENODD_M_VALUE - 1); 
    symbol_index[1] = FBE_XOR_MOD_VALUE_M(symbol_index[0] + data_pos_1, 
                                      FBE_XOR_EVENODD_M_VALUE - 1);

    do
    {
        /* 
         * Step 2 in page 196.
         */

        /*
         * The 1st part is for data reconstruction.
         */
        data_syndrome[1] = &scratch_p->diag_syndrome->syndrome_symbol[symbol_index[1]].data_symbol[0];

        /*
         * Always starts from the begining of the symbol (8 integers).
         */
        data_symbol_copy = &dummy_data_symbol[0]; 
        data_syndrome[0] = &scratch_p->row_syndrome->syndrome_symbol[symbol_index[0]].data_symbol[0];
        data_symbol[1]   = first_data_symbol[1] + 
                           symbol_index[0] * FBE_XOR_WORDS_PER_SYMBOL; 
        data_symbol[0]   = first_data_symbol[0] + 
                           symbol_index[0] * FBE_XOR_WORDS_PER_SYMBOL;
        s_value_holder   = &scratch_p->s_value->data_symbol[0];

        /* 
         * Reconstruct the data from both parities.
         */
        XORLIB_REPEAT8(( *data_symbol[1]     = *data_syndrome[1]++ ^ 
                                            *s_value_holder++ ^ 
                                            *data_symbol_copy, 
                      *data_symbol_copy++ = *data_symbol[0]++ = 
                                            *data_syndrome[0]++ ^ 
                                            *data_symbol[1]++ ));

        /*
         * The 2nd part is for checksum reconstruction.
         */

        /*
         * Reconstruct checksums from PoC, since they are
         * initialized as 0, we just need to set the bit 
         * if it's 1. This is still step 2 from page 196.
         * But not sure this is a good way.
         *
         * Note here bit[16-k] in the syndrome is the kth
         * symbol in the evenodd array, 
         * bit[15-s] is the sth bit in the checksum since
         * the MSB bit of checksum is the 0th symbol in
         * the evenodd array.
         */
        csum_symbol[1] = 
            FBE_XOR_IS_BITSET(csum_syndrome[1], 16 - symbol_index[1]) ^ 
            csum_bit_copy;       
        if (csum_symbol[1] == 1) 
        {
            block_csum[1] = 
                FBE_XOR_SET_BIT(block_csum[1], 15 - symbol_index[0]);
        }
        csum_bit_copy = 
            csum_symbol[0] = 
                FBE_XOR_IS_BITSET(csum_syndrome[0], 16 - symbol_index[0]) ^ 
                csum_symbol[1];   
        if (csum_symbol[0]) 
        {
            block_csum[0] = 
                FBE_XOR_SET_BIT(block_csum[0], 15 - symbol_index[0]);
        }

        /* 
         * Step 3 in page 196.
         */
        symbol_index[0] = FBE_XOR_MOD_VALUE_M(symbol_index[0] - 
                                          data_pos_1 + data_pos_0 + 
                                          FBE_XOR_EVENODD_M_VALUE, 
                                          FBE_XOR_EVENODD_M_VALUE - 1); 
        symbol_index[1] = FBE_XOR_MOD_VALUE_M(data_pos_1 + symbol_index[0],
                                          FBE_XOR_EVENODD_M_VALUE - 1);
    }
    while(symbol_index[0] != (FBE_XOR_EVENODD_M_VALUE - 1));


    scratch_p->fatal_blk[0]->crc = block_csum[0];
    scratch_p->fatal_blk[1]->crc = block_csum[1];

    /* Now check the checksum and finish up.
     */
    fbe_status = fbe_xor_reconst_2_check_csum(scratch_p, eboard_p, 
                                       bitkey, rebuild_pos, 
                                       init_rpos, parity_pos,
                                       0 /* rebuild position 0 */,
                                       &check_csum);

    if (fbe_status != FBE_STATUS_OK) 
    { 
        *reconstruct_result_p = status;
        return fbe_status; 
    }

    status &= check_csum;

    /* Continue with the second pass if the second rebuild position
     * is still in the fatal key.
     * If pass 1 failed and there were stamp errors, then
     * we might take the second rebuild position out of the fatal key.
     */
    if (bitkey[rebuild_pos[1]] & scratch_p->fatal_key)
    {
         fbe_status = fbe_xor_reconst_2_check_csum(scratch_p, eboard_p, 
                                           bitkey, rebuild_pos, 
                                           init_rpos, parity_pos,
                                           1 /* rebuild position 1 */,
                                           &check_csum);
         if (fbe_status != FBE_STATUS_OK) 
         { 
             return status; 
         }
         status &= check_csum;
    }
    
    *reconstruct_result_p = status;
    return FBE_STATUS_OK;
}
/* fbe_xor_eval_reconstruct_2() */


/***************************************************************************
 * fbe_xor_calc_data_syndrome()
 ***************************************************************************
 * @brief
 *  This function updates both row and diagonal syndromes from the
 *  given data disk sector, as well as the 2 checksum syndromes.
 *

 * @param scratch_p - scratch containing row  and diagonal syndroms.
 * @param col_index - column index of the EVENODD array, which is the
 * @param sector_p - sector of the data disk.
 *
 * @return
 *   Uncooked checksum.
 *
 * @notes
 *
 * @author
 *  04/05/06 - Created. NJI
 ***************************************************************************/
fbe_u32_t fbe_xor_calc_data_syndrome(fbe_xor_scratch_r6_t * const scratch_p, 
                               const fbe_u32_t col_index, 
                               fbe_sector_t * const sector_p)
{
    fbe_u32_t checksum = 0x0;

    /*
     * Holds the pointer to the top row syndrome symbol.
     */
    fbe_xor_r6_syndrome_t *tgt_row_ptr_holder = scratch_p->row_syndrome;
    /*
     * Holds the pointer to the top diagonal syndrome symbol.
     */
    fbe_xor_r6_syndrome_t *tgt_diagonal_ptr_holder = scratch_p->diag_syndrome;
    fbe_u32_t *tgt_row_ptr, *tgt_diag_ptr, *src_ptr;
    fbe_u32_t row_index, diag_symbol_index;

    /*
     * Init the target pointer for row syndrome.
     */
    tgt_row_ptr = (fbe_u32_t *)tgt_row_ptr_holder;

    /*
     * The data in the sector contains the info we need to update
     * the syndromes.
     */
    src_ptr = sector_p->data_word;

    /* 
     * Update both row and diagonal syndrome symbols from the 16 
     * data symbols from the data sector.
     * For row syndrome, we will never need to change 17th symbol.
     * So it's always 0s.
     * For diagonal syndrome, based on the data disk position,
     * we don't need to change one of the diagonal syndrome symbol.
     */
    XORLIB_PASSES16(row_index, FBE_WORDS_PER_BLOCK)
    {
        /*
         * Find the diagonal syndrome symbol needs to be updated.
         */
        diag_symbol_index = FBE_XOR_MOD_VALUE_M(row_index + col_index, FBE_XOR_EVENODD_M_VALUE-1);        
        tgt_diag_ptr = &tgt_diagonal_ptr_holder->syndrome_symbol[diag_symbol_index].data_symbol[0];

        if (scratch_p->initialized)
        {
            /*
             * If syndromes have be initialized, update both syndromes
             * by xoring the data symbol into row and diagonal 
             * syndromes.
             */
            XORLIB_REPEAT8( (checksum ^= *src_ptr, *tgt_row_ptr++ ^= *src_ptr, *tgt_diag_ptr++ ^= *src_ptr++) );
        }
        else
        {
            /*
             * If the 1st data sector in the array we're working on, 
             * then scratch_p hasn't be initialized.
             * So assign the data into both syndromes.
             */
            XORLIB_REPEAT8( (checksum ^= *src_ptr, *tgt_row_ptr++ = *src_ptr, *tgt_diag_ptr++ = *src_ptr++) );
        }
    }

    /* 
     * We have an imaginary 17th data symbol of 0s for this sector_p,
     * the coresponding row & diagonal syndrome symbol will be set 
     * below.
     */
    if (scratch_p->initialized == FBE_FALSE )
    {
        /*
         * For row syndrome, it's always the 17th symbol never
         * needs to be updated. So just initialize it to be 0.
         */
        tgt_row_ptr = &tgt_row_ptr_holder->syndrome_symbol[FBE_XOR_EVENODD_M_VALUE - 1].data_symbol[0];

        /*
         * Data sector contains 16 symbols, while each syndrome is
         * 17 syndromes. There's 1 diagonal syndrome symbol will
         * be initialized to be 0s. 
         */
        diag_symbol_index = FBE_XOR_MOD_VALUE_M(col_index - 1 + FBE_XOR_EVENODD_M_VALUE, FBE_XOR_EVENODD_M_VALUE - 1);        
        tgt_diag_ptr = &tgt_diagonal_ptr_holder->syndrome_symbol[diag_symbol_index].data_symbol[0];
        
        /*
         * Assign 0s into the proper row and diagonal syndrome symbol.
         */
        XORLIB_REPEAT8( (*tgt_row_ptr++ = 0, *tgt_diag_ptr++ = 0) );

        /* 
         * Assign the checksum syndromes, bit 0 of checksum_syndrome
         * is A[0][col], bit 1 is A[1][col]...
         * Since crc only has 16 bits, A[16][col] is always 0 
         * representing 17th symbol for row parity syndrome.
         * For diagonal syndrome, we need to make sure it's 17 bits.
         */
        scratch_p->checksum_row_syndrome = sector_p->crc << 1;
        scratch_p->checksum_diag_syndrome = (0x1ffff) & FBE_XOR_EVENODD_SYNDROME_DIAGONAL_MANGLE(sector_p->crc, col_index);
 
        scratch_p->initialized = FBE_TRUE;
    }
    else
    {
        /*
         * Update the checksum syndromes.
         */
        scratch_p->checksum_row_syndrome ^= sector_p->crc << 1;
        scratch_p->checksum_diag_syndrome ^= (0x1ffff) & FBE_XOR_EVENODD_SYNDROME_DIAGONAL_MANGLE(sector_p->crc, col_index);
    }

    return (checksum);
} 
/* fbe_xor_calc_data_syndrome() */

/***************************************************************************
 * fbe_xor_calc_parity_syndrome()
 ***************************************************************************
 * @brief
 *  This function calculates the syndrome from parity disk,
 *  as well as the checksum syndrome.
 *

 * @param scratch_p - scratch containing row  and diagonal syndroms
 *   parity_index,[I]  - 0 for row parity, 1 for diagonal parity
 * @param sector_p - sector of the data disk
 *
 * @return
 *   Uncooked checksum.
 *
 * @notes
 *   We always assume caller will call this for row parity 1st,
 *   then diagonal parity.
 *
 * @author
 *  04/10/06 - Created. NJI
 ***************************************************************************/
fbe_u32_t fbe_xor_calc_parity_syndrome(fbe_xor_scratch_r6_t * const scratch_p, 
                                 const fbe_xor_r6_parity_positions_t parity_index,
                                 fbe_sector_t * const sector_p)
{
    fbe_u32_t checksum = 0x0;

    /*
     * Holds the pointer to the top row/diagonal syndrome symbol.
     */
    fbe_xor_r6_syndrome_t *tgt_ptr_holder;
    fbe_u32_t *tgt_ptr, *src_ptr, *s_value_holder;
    fbe_u32_t row_index, symbol_index;

    if (XOR_COND((parity_index < FBE_XOR_R6_PARITY_POSITION_ROW) ||
                 (parity_index > FBE_XOR_R6_PARITY_POSITION_DIAG)))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "unexpected RAID-6 error : parity index = 0x%x,"
                              "row positin = 0x%x, diagonal position = 0x%x\n",
                              parity_index,
                              FBE_XOR_R6_PARITY_POSITION_ROW,
                              FBE_XOR_R6_PARITY_POSITION_DIAG);

        return FBE_STATUS_GENERIC_FAILURE;
    }


    /* 
     * Set up the pointer to the top of syndrome symbol
     * based on the parity type.
     */
    if (parity_index == FBE_XOR_R6_PARITY_POSITION_ROW)
    {
        tgt_ptr_holder = scratch_p->row_syndrome;
    }
    else if (parity_index == FBE_XOR_R6_PARITY_POSITION_DIAG)
    {
        tgt_ptr_holder = scratch_p->diag_syndrome;
    }
    
    /*
     * Init the target pointer for to the top of row/diagonal syndrome.
     */
    tgt_ptr = (fbe_u32_t *)tgt_ptr_holder;

    /*
     * Get the top of the data sector.
     */
    src_ptr = sector_p->data_word;

    /*
     * Init the src_ptr for data s value calculation.
     */
    s_value_holder = &scratch_p->s_value->data_symbol[0];

    /*
     * For reconstruct 2 data disk, S value is xored from both parities.
     * Assuming row parity is always evaluated before diagonal parity,
     * We initialize the S value to be 0 1st in the case of row parity.
     * Note here we are assuming this function will be called for 
     * row parity first.
     */
    if (parity_index == FBE_XOR_R6_PARITY_POSITION_ROW)
    {
        XORLIB_REPEAT8( (*s_value_holder++ = 0) );
   
        /* 
         * Init the s value for parity of checksum as well.
         */
        scratch_p->s_value_for_checksum = 0;
    }

    /* 
     * Each parity sector has 16 data symbols, update the coresponding
     * row/diagonal syndrome symbols.
     */
    XORLIB_PASSES16(row_index, FBE_WORDS_PER_BLOCK)
    {
        /*
         * Init the s value pointer holder
         */
        s_value_holder = &scratch_p->s_value->data_symbol[0];

        if (scratch_p->initialized)
        {
            XORLIB_REPEAT8( (checksum ^= *src_ptr, *tgt_ptr++ ^= *src_ptr, *s_value_holder++ ^= *src_ptr++) );
        }
        else
        {
            XORLIB_REPEAT8( (checksum ^= *src_ptr, *tgt_ptr++ = *src_ptr, *s_value_holder++ ^= *src_ptr++) );
        }
    }

    /* 
     * If the lun has 4 disks, and the 1st 2 data disks are gone, 
     * we only have 2 parity disks left,
     * then we need to init from parity disks.
     */
    if (scratch_p->initialized == FBE_FALSE)
    {
        /* 
         * We have an imagnary 17th symbol of 0s for this sector_p,
         * the coresponding 17th syndrome symbol will be set 
         * during initizalization.
         */
        tgt_ptr = &tgt_ptr_holder->syndrome_symbol[FBE_XOR_EVENODD_M_VALUE - 1].data_symbol[0];
      
        XORLIB_REPEAT8( (*tgt_ptr++ = 0) );

        /*
         * Since we could enter here for the 2nd loop,
         * always reset the checksum syndromes in the beginning.
         */
        if (parity_index == FBE_XOR_R6_PARITY_POSITION_ROW)
        {
            scratch_p->checksum_row_syndrome = 0;
        }
        else if (parity_index == FBE_XOR_R6_PARITY_POSITION_DIAG)
        {
            scratch_p->checksum_diag_syndrome = 0;

            /*
             * For row parity, we know we need to call this function
             * for diagonal parity. So we only set initialized to be FBE_TRUE
             * when both parity sectors have been updated.
             */
            scratch_p->initialized = FBE_TRUE;
        }
    }

    /*
     * Calcuate the S value for checksum syndrome.
     * shed_stamp contains 16 bits representing 16 symbols,
     * so we update the s_value from each symbol/bit.
     */ 
    for (symbol_index = 0; symbol_index < (FBE_XOR_EVENODD_M_VALUE - 1); symbol_index++)
    {
        if (sector_p->lba_stamp & (1 << symbol_index)) 
        {
            scratch_p->s_value_for_checksum ^= 1;
        }
    }

    /* 
     * Setup the checksum syndrome.
     */   
    if (parity_index == FBE_XOR_R6_PARITY_POSITION_ROW)
    {
        scratch_p->checksum_row_syndrome ^= sector_p->lba_stamp << 1; 
    }
    else if (parity_index == FBE_XOR_R6_PARITY_POSITION_DIAG)
    {
        scratch_p->checksum_diag_syndrome ^= sector_p->lba_stamp << 1;
    }

    return (checksum);
}
/* fbe_xor_calc_parity_syndrome() */

/* end xor_raid6_reconstruct_2.c */
