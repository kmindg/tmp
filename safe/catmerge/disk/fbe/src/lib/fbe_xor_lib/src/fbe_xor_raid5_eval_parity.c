/***************************************************************************
 * Copyright (C) EMC Corporation 1989-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_xor_raid5_eval_parity.c
 ***************************************************************************
 *
 * @brief
 *   Raid 5 and Raid 3 related functions are located in this file.
 * 
 * @notes
 *
 * @author
 *  11/4/2010 - Created. Rob Foley
 *
 ***************************************************************************/

/************************
 * INCLUDE FILES
 ************************/
#include "fbe_xor_private.h"
#include "fbe/fbe_library_interface.h"

/*****************************************************
 * GLOBALS
 *****************************************************/
static fbe_bool_t   b_xor_raid5_check_for_reconstructable_data = FBE_FALSE; /*! @todo Once we know that we aren't going to use stamps remove this. */

/*****************************************************
 * static FUNCTIONS
 *****************************************************/
static fbe_status_t fbe_xor_eval_parity_sector(fbe_xor_scratch_t * scratch_p,
                                               fbe_xor_error_t * eboard_p,
                                               fbe_sector_t * pblk_p,
                                               fbe_u16_t pkey,
                                               fbe_u16_t width,
                                               fbe_lba_t seed);
static fbe_status_t fbe_xor_eval_data_sector(fbe_xor_scratch_t * scratch_p,
                                             fbe_xor_error_t * eboard_p,
                                             fbe_sector_t * dblk_p,
                                             fbe_u16_t dkey,
                                             fbe_lba_t seed);
static fbe_status_t fbe_xor_eval_shed_sector(fbe_xor_scratch_t * scratch_p,
                                             fbe_xor_error_t * eboard_p,
                                             fbe_sector_t * sector_p[],
                                             fbe_u16_t bitkey[],
                                             fbe_u16_t ppos,
                                             fbe_u16_t spos,
                                             fbe_lba_t seed,
                                             fbe_bool_t b_preserve_shed_data,
                                             fbe_u16_t *u_bitmask_p);
static fbe_status_t fbe_xor_rebuild_sector(fbe_xor_scratch_t *scratch_p,
                                           fbe_xor_error_t *eboard_p);
static fbe_bool_t fbe_xor_rebuild_data_stamps(fbe_xor_scratch_t * scratch_p,
                                              fbe_xor_error_t * eboard_p,
                                              fbe_sector_t * sector_p[],
                                              fbe_u16_t bitkey[],
                                              fbe_u16_t width,
                                              fbe_u16_t ppos);
static void fbe_xor_rebuild_parity_stamps(fbe_xor_scratch_t * scratch_p,
                                          fbe_xor_error_t* eboard_p,
                                          fbe_sector_t* sector_p[],
                                          fbe_u16_t bitkey[],
                                          fbe_u16_t width,
                                          fbe_u16_t ppos,
                                          fbe_lba_t seed,
                                          fbe_bool_t CheckParity,
                                          fbe_sector_t* pblk_p);
static void fbe_xor_init_scratch(fbe_xor_scratch_t * scratch_p,
                                 fbe_sector_t * sector_p[],
                                 fbe_u16_t bitkey[],
                                 fbe_u16_t width,
                                 fbe_lba_t seed,
                                 fbe_xor_option_t option,
                                 fbe_u16_t ibitmap);
static fbe_u16_t fbe_xor_eval_parity_invalidate( fbe_xor_scratch_t *scratch_p,
                                                 fbe_u16_t invalid_bitmap, 
                                                 fbe_u16_t media_err_bitmap,
                                                 fbe_xor_error_t * eboard_p,
                                                 fbe_sector_t * sector_p[],
                                                 fbe_u16_t bitkey[],
                                                 fbe_u16_t width,
                                                 fbe_u16_t parity_pos,
                                                 fbe_u16_t rebuild_pos,
                                                 fbe_bool_t b_final_recovery_attempt);

/*****************************************************
 * FUNCTIONS
 *****************************************************/

/***************************************************************************
 *              fbe_xor_init_scratch()
 ***************************************************************************
 * @brief
 *  This routine is used to initialize the "scratch" data-
 *  base used to process Raid5 (or old Raid3) LUs.
 *
 *    scratch_p  - [I]   ptr to a Scratch database area.
 *    sector_p   - [I]   vector of ptrs to sector contents.
 *    bitkey- [I]   bitkey for stamps/error boards.
 *    width - [I]   number of sectors.
 *    seed  - [I]   seed value for checksum.
 *    ibitmap    - [I]   invalid sector bitmap.
 *
 * @return
 *    None.
 *
 * @notes
 *    If there are no know errors, the 'scratch' sector is used
 *    for constructing the 'horizontal' XOR of the data sectors,
 *    for comparison with parity. If an error is identified
 *    during processing, the content of this sector can be used
 *    directly for reconstruction.
 *
 *    If there are known errors, then the 'scratch' sector is
 *    ignored in favor of the sector to be reconstructed; this
 *    saves an additional copy operation.
 *
 * @author
 *    06-Nov-98  -SPN-   Redesigned to use checksum toolkit.
 ***************************************************************************/

static void fbe_xor_init_scratch(fbe_xor_scratch_t * scratch_p,
                                 fbe_sector_t * sector_p[],
                                 fbe_u16_t bitkey[],
                                 fbe_u16_t width,
                                 fbe_lba_t seed,
                                 fbe_xor_option_t option,
                                 fbe_u16_t ibitmap)
{
    scratch_p->initialized =
    scratch_p->xor_required = FBE_FALSE,
    scratch_p->running_csum = 0,
    scratch_p->fatal_cnt = 0,
    scratch_p->fatal_key = 0,
    scratch_p->fatal_blk = NULL,
    scratch_p->rbld_blk = &scratch_p->scratch_sector,
    scratch_p->seed = seed,
    scratch_p->b_valid_shed = FBE_FALSE;
    scratch_p->copy_required = FBE_FALSE;
    scratch_p->option = option;

    /* Count the number of 'known' errors...
     */
    while (width-- > 0)
    {
        if (0 != (ibitmap & bitkey[width]))
        {
            scratch_p->fatal_cnt++,
            scratch_p->fatal_key = bitkey[width],
                                   scratch_p->fatal_blk =
                                   scratch_p->rbld_blk = sector_p[width];
        }
    }

    return;
}  /* fbe_xor_init_scratch() */

/*****************************************************************************
 *          fbe_xor_raid5_rebuild_data_sector()
 ***************************************************************************** 
 * 
 * @brief   This method uses parity and the other data position to rebuild
 *          (reconstruct) (1) data position.  It checks if the the reconstructed
 *          data was the invalidated pattern and if so it re-invalidates the
 *          CRC.
 * 
 * @param   scratch_p - Pointer to parity `scratch' structure used for rebuild
 * @param   eboard_p  - error board for marking different types of errors.
 * @param   bitkey  - [I]   bitkey for stamps/error boards.
 * @param   ppos    - [I]   index containing parity info.
 * @param   rpos    - [I]   index to be rebuilt, -1 if no rebuild required.
 * @param   seed    - [I]   seed value for checksum.
 *
 * @author
 *  06-Nov-98  -SPN-   Redesigned to use checksum toolkit.
 *  12-Nov-99  -Rob Foley-   Modified for rebuild to handle shed data.
 *  04-Apr-00  -MPG-   Ported from Raid component 
 *  03/22/10:  Omer Miranda -- Updated to use the new sector tracing facility.
 *
 *****************************************************************************/
static fbe_status_t fbe_xor_raid5_rebuild_data_sector(fbe_xor_scratch_t *scratch_p,
                                                      fbe_xor_error_t *eboard_p,
                                                      fbe_u16_t bitkey[],
                                                      fbe_u16_t ppos,
                                                      fbe_u16_t rpos,
                                                      fbe_lba_t seed)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_bool_t          b_validate_data;
    fbe_bool_t          b_sector_invalidated = FBE_FALSE;
    fbe_xor_option_t    rebuild_option = 0;
    fbe_u16_t           csum_read = 0;
    fbe_u16_t           csum_calc = 0;

    /* The remaining data is consistent with the parity, so we can rebuild
     * the lost data sector from the scratch buffer.
     */
    status = fbe_xor_rebuild_sector(scratch_p, eboard_p);
    if (status != FBE_STATUS_OK)
    {
        return status; 
    }

    /*! @note Rebuild sector updates the `modified' bitmap.
     */

    /* If this is the `data lost invalidated' data pattern, set the crc to
     * the `known bad' pattern.
     */
    b_validate_data = (scratch_p->fatal_key != bitkey[ppos]) ? FBE_TRUE : FBE_FALSE;
    if (b_validate_data                                         &&
        xorlib_is_sector_data_invalidated(scratch_p->fatal_blk)    )
    {
        /* Bad checksum for this sector's data. Mark this as an uncorrectable 
         * checksum error for now.
         */
        eboard_p->u_crc_bitmap |= scratch_p->fatal_key;
        eboard_p->c_crc_bitmap &= ~scratch_p->fatal_key;

        /* Generate and set the `invalid' crc if the rebuilt data was
         * `data lost' client invalidated.
         */
        b_sector_invalidated = FBE_TRUE;
        csum_read = scratch_p->fatal_blk->crc;
        xorlib_rebuild_invalidated_sector(scratch_p->fatal_blk);
        csum_calc = scratch_p->fatal_blk->crc;

        /* Now determine the reason for the invalidation.
         */
        status = fbe_xor_determine_csum_error(eboard_p,
                                              scratch_p->fatal_blk,
                                              scratch_p->fatal_key,
                                              seed,
                                              csum_read,
                                              csum_calc,
                                              FBE_TRUE);
        if (status != FBE_STATUS_OK)
        {
            return status; 
        }
    }

    /*! @note Data that has been lost (invalidated) should never
     *        have a valid checksum.  Since we just rebuilt this sector
     *        we should always validate the data (as long as it isn't a
     *        parity position)
     */
    if (scratch_p->fatal_key != bitkey[ppos])
    {
        /* Add the validate data flag to the current option flags.
         */
        rebuild_option = scratch_p->option | FBE_XOR_OPTION_VALIDATE_DATA;
    }
    fbe_xor_validate_data_if_enabled(scratch_p->fatal_blk, seed, rebuild_option);

    /* If we have rebuilt a position validate that only the rebuild 
     * position had a crc error.
     */
    if ((rpos != FBE_XOR_INV_DRV_POS)           &&
        (eboard_p->c_crc_bitmap & bitkey[rpos])    )
    {
        /* Since we are degraded, clearing the degraded bitmap,
         * should result in zero correctable errors.
         */
        if (XOR_COND((~bitkey[rpos] & eboard_p->c_crc_bitmap) != 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "((~bitkey[%d] & eboard_p->c_crc_bitmap) != 0),"
                              "bitkey[%d] = 0x%x, eboard_p->c_crc_bitmap = 0x%x\n",
                              rpos,
                              rpos,
                              bitkey[rpos],
                              eboard_p->c_crc_bitmap);      
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* We are rebuilding data. Do not report this as an error,
         * but rather as a "modification".
         */
        eboard_p->c_crc_bitmap &= ~bitkey[rpos];
    }

    /* Return the status 
     */
    return status;
}
/* end of fbe_xor_raid5_rebuild_data_sector()*/

/*****************************************************************************
 *          fbe_xor_raid5_rebuild_parity_from_scratch()
 ***************************************************************************** 
 * 
 * @brief   This method uses parity and the other data position to rebuild
 *          (reconstruct) (1) parity position from the scratch data.
 * 
 * @param   scratch_p - Pointer to parity `scratch' structure used for rebuild
 * @param   sector_p - Array of sector data for these strip
 * @param   eboard_p  - error board for marking different types of errors.
 * @param   bitkey  - [I]   bitkey for stamps/error boards.
 * @param   ppos    - [I]   index containing parity info.
 * @param   rpos    - [I]   index to be rebuilt, -1 if no rebuild required.
 * @param   seed    - [I]   seed value for checksum.
 *
 * @author
 *  06-Nov-98  -SPN-   Redesigned to use checksum toolkit.
 *  12-Nov-99  -Rob Foley-   Modified for rebuild to handle shed data.
 *  04-Apr-00  -MPG-   Ported from Raid component 
 *  03/22/10:  Omer Miranda -- Updated to use the new sector tracing facility.
 *
 *****************************************************************************/
static fbe_status_t fbe_xor_raid5_rebuild_parity_from_scratch(fbe_xor_scratch_t *scratch_p,
                                                        fbe_sector_t * sector_p[],
                                                        fbe_xor_error_t *eboard_p,
                                                        fbe_u16_t bitkey[],
                                                        fbe_u16_t width,
                                                        fbe_u16_t ppos,
                                                        fbe_u16_t rpos,
                                                        fbe_lba_t seed)
{
    fbe_status_t        status = FBE_STATUS_OK;

    /* Rebuild the parity stamps based on the data disks.
     */
    fbe_xor_rebuild_parity_stamps(scratch_p,
                                  eboard_p,
                                  sector_p,
                                  bitkey,
                                  width,
                                  ppos,
                                  seed,
                                  FBE_FALSE,
                                  sector_p[ppos]);

    /* Now rebuild the parity sector from the scratch information.
     */
    status = fbe_xor_rebuild_sector(scratch_p, eboard_p);
    if (status != FBE_STATUS_OK)
    {
        return status; 
    }

    /*! @note Rebuild sector updates the `modified' bitmap.
     */

    /* If we have rebuilt a position validate that only the rebuild 
     * position had a crc error.
     */
    if ((rpos != FBE_XOR_INV_DRV_POS) &&
        (eboard_p->c_crc_bitmap & bitkey[ppos]))
    {
        /* Since we are degraded,
         * clearing the parity's bitmap,
         * should result in zero correctable errors.
         */
        if (XOR_COND((~bitkey[ppos] & eboard_p->c_crc_bitmap) != 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "((~bitkey[%d] & eboard_p->c_crc_bitmap) != 0), "
                                  "bitkey[%d] = 0x%x, eboard_p->c_crc_bitmap = 0x%x\n",
                                  ppos,
                                  bitkey[ppos],
                                  ppos,
                                  eboard_p->c_crc_bitmap);  

            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* We are rebuilding parity.  Do not report this as an error,
         * but rather as a "modification".
         */
        eboard_p->c_crc_bitmap &= ~bitkey[ppos];
    }

    /* Return the status 
     */
    return status;
}
/**************************************************** 
 * end of fbe_xor_raid5_rebuild_parity_from_scratch()
 ****************************************************/

/*!***************************************************************************
 *          fbe_xor_eval_parity_unit()
 *****************************************************************************
 *
 * @brief   This routine is used for Raid5 (or old Raid3) LUs. It checks the 
 *          CRC-ness of the sector from each drive in the LUN, and also checks 
 *          the coherency between Data and Parity.
 *
 * @param   eboard_p  - Error board for marking different types of errors.
 * @param   sector_p  - Vector of ptrs to sector contents.
 * @param   bitkey  - bitkey for stamps/error boards.
 * @param   width   - Number of sectors in array.
 * @param   ppos    - Index containing parity info.
 * @param   rpos    - Index to be rebuilt, -1 if no rebuild required.
 * @param   seed    - Seed value for checksum.
 * @param   b_final_recovery_attempt - FBE_TRUE - This is the final recovery attempt
 *                                     FBE_FALSE - Retries are possible
 * @param   b_preserve_shed_data - FBE_TRUE to preserve shed data, do not
 *                                          rebuild parity. FBE_FALSE to rebuild parity and
 *                                          not preserve shed data.
 * @param   option - xor option flags.
 * @param   invalid_bitmask_p   - bitmask indicating which sectors have been invalidated.
 *
 *      @return fbe_status_t
 *
 *      @notes
 *      There are 3 separate interfaces to this function.
 *
 *      1) No errors encountered - eboard has zero for all counts.
 *
 *      2) A CRC or HARD error encountered, reconstruct the errored disk:
 *         eboard has one bit key set in u_crc_bitmap for the disk taking a
 *         hard or crc error.
 *
 *      3) A rebuild of data is required:
 *             a) rpos is set for the position to be rebuilt,
 *             b) rpos is also used to determine the position
 *                for which the parity drive may have shed data
 *
 * @author
 *  06-Nov-98  -SPN-   Redesigned to use checksum toolkit.
 *  12-Nov-99  -Rob Foley-   Modified for rebuild to handle shed data.
 *  04-Apr-00  -MPG-   Ported from Raid component 
 *  03/22/10:  Omer Miranda -- Updated to use the new sector tracing facility.
 *
 ***************************************************************************/

fbe_status_t fbe_xor_eval_parity_unit(fbe_xor_error_t * eboard_p,
                                      fbe_sector_t * sector_p[],
                                      fbe_u16_t bitkey[],
                                      fbe_u16_t width,
                                      fbe_u16_t ppos,
                                      fbe_u16_t rpos,
                                      fbe_lba_t seed,
                                      fbe_bool_t b_final_recovery_attempt,
                                      fbe_bool_t b_preserve_shed_data,
                                      fbe_xor_option_t option,
                                      fbe_u16_t *invalid_bitmask_p)
{
    fbe_status_t        status;
    fbe_u16_t           invalid_bitmap = 0;
    fbe_u16_t           media_err_bitmap = 0;
    fbe_u16_t           retry_err_bitmap = 0;
    fbe_u16_t           shed_bitmap = 0;
    fbe_u16_t           c_crc_non_parity_bitmap = 0;
    /*! @todo, this eventually should be allocated by the client and passed in 
     *         via the verify request. 
     */
    fbe_xor_scratch_t   Scratch;
    fbe_u16_t           pos;

    /* Determine media error bitmap.
     * If there is a media errd position, then
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
    media_err_bitmap = eboard_p->hard_media_err_bitmap;
    retry_err_bitmap = eboard_p->retry_err_bitmap;

    if ( rpos != FBE_XOR_INV_DRV_POS)
    {
        /* Since we really have a rebuild position,
         * clear its bit from the media error bitmap,
         * so that we will not report this position with errors.
         */
        media_err_bitmap &= ~(bitkey[rpos]);
        retry_err_bitmap &= ~(bitkey[rpos]);
    }


    /* Using the data read from all available positions, determine what if 
     * any data needs to be reconstructed and/or invalidated.
     */
    if (rpos != FBE_XOR_INV_DRV_POS)
    {
        /* We are rebuilding the "rpos" position.
         * Mark this position as invalid, so we will
         * 1) rebuild this array position
         * 2) NOT report a CRC error.
         */
        invalid_bitmap |= bitkey[rpos];
    }

    invalid_bitmap |= eboard_p->hard_media_err_bitmap;
    invalid_bitmap |= eboard_p->retry_err_bitmap;
    invalid_bitmap |= eboard_p->no_data_err_bitmap;

    /* make sure there are zero bits set beyond the array width!
     */
    if (XOR_COND(((0xFFFF << width) & invalid_bitmap) != 0))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "((0xFFFF << width 0x%x) & invalid_bitmap 0x%x) == 0\n",
                              width,
                              invalid_bitmap);  

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Initialize the `scratch' which include the scratch buffer for any
     * reconstructed sectors.
     */
    fbe_xor_init_scratch(&Scratch,
                         sector_p,
                         bitkey,
                         width,
                         seed,
                         option,
                         invalid_bitmap);

    /* If we expect to see shed data and
     * the correct shed bit is set in the parity sector,
     * then check the shed data first.
     */
    if ((rpos != FBE_XOR_INV_DRV_POS) &&
        (rpos != ppos) &&
        ((bitkey[ppos] & invalid_bitmap) == 0) &&
        (sector_p[ppos]->lba_stamp != 0))
    {
        if (XOR_COND(bitkey[rpos] == 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "bitkey[%d] == 0\n",
                                  rpos);  

            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (XOR_COND(bitkey[ppos] == 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "bitkey[%d] == 0\n",
                                  ppos);  

            return FBE_STATUS_GENERIC_FAILURE;
        }


        /* There is shed data on the parity drive.
         */
        if (XOR_COND((invalid_bitmap & bitkey[rpos]) == 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "(invalid_bitmap 0x%x & bitkey[%d] 0x%x) == 0\n",
                                  invalid_bitmap,
                                  rpos,
                                  bitkey[rpos]);  

            return FBE_STATUS_GENERIC_FAILURE;
        }

        FBE_XOR_INC_STATS(&xor_stat_proc0_tbl[XOR_STAT_PROC0_SHED]);

        /*! @note Data that has been lost (invalidated) should never
         *        have a valid checksum.
         */
        fbe_xor_validate_data_if_enabled(sector_p[ppos], seed, option);

        /* We do not support shedding on R5/R3, so we should not look for shed data.
         */
#if 0
        status = fbe_xor_eval_shed_sector(&Scratch,
                                          eboard_p,
                                          sector_p,
                                          bitkey,
                                          ppos,
                                          rpos,
                                          seed,
                                          b_preserve_shed_data,
                                          &invalid_bitmap);
        if (status != FBE_STATUS_OK) {
            return status; 
        }
#endif
        /*! @note Data that has been lost (invalidated) should never
         *        have a valid checksum.
         */
        fbe_xor_validate_data_if_enabled(sector_p[rpos], seed, option);

        /* Assign the shed position, so we
         * do not attempt to eval the data
         * sector again.
         */
        shed_bitmap = bitkey[rpos];

        /* We should always have a fatal count,
         * since parity needs to be rebuilt.
         */
        if (XOR_COND(Scratch.fatal_cnt == 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "Scratch.fatal_cnt == 0\n");  

            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        /* Either this is not a rebuild.
         * Or, this is a rebuild, but
         * no shed data detected on parity drive.
         */
    }

    /* Process each of the sectors containing data, looking
     * for checksum errors or illegal stamp configurations.
     */
    for (pos = width; 0 < pos--;)
    {
        if ((ppos != pos) &&
            (0 == (bitkey[pos] & invalid_bitmap)) &&
            (0 == (bitkey[pos] & shed_bitmap)))
        {
            status = fbe_xor_eval_data_sector(&Scratch,
                                              eboard_p,
                                              sector_p[pos],
                                              bitkey[pos],
                                              seed);
            if (status != FBE_STATUS_OK)
            {
                return status; 
            }

            /*! @note Data that has been lost (invalidated) should never
             *        have a valid checksum.
             */
            fbe_xor_validate_data_if_enabled(sector_p[pos], seed, option);
        }

    } /* for all data positions read */


    /* Process the parity.
     */
    if (0 == (bitkey[ppos] & invalid_bitmap))
    {
        status = fbe_xor_eval_parity_sector(&Scratch,
                                            eboard_p,
                                            sector_p[ppos],
                                            bitkey[ppos],
                                            width,
                                            seed);
        if (status != FBE_STATUS_OK)
        {
            return status; 
        }
    }

    if ( eboard_p->u_coh_bitmap )
    {
        /* Save the vector of sectors to ktrace for debugging purposes.
         * Although the error board states `uncorrectable', we have not
         * yet applied the correction (i.e. correct parity with data).
         * In most cases it will become correctable.  At the same time
         * we want to know about all COH errors so log it as an error.
         * The trace levels are changable via fcli xordrv - stl or
         * the registry via FlareXorTraceLevel.
         */
        fbe_sector_trace_error_flags_t trace_coh_error_flag = FBE_SECTOR_TRACE_ERROR_FLAG_COH;
        if( option & FBE_XOR_OPTION_BVA){
			trace_coh_error_flag = FBE_SECTOR_TRACE_ERROR_FLAG_ECOH;
        }
        status = fbe_xor_sector_history_trace_vector( seed,    /* Lba */
                                                      (const fbe_sector_t * const *)sector_p,    /* Sector vector to save. */
                                                      bitkey,    /* Bitmask of positions in group. */
                                                      width,    /* Width of group. */
                                                      ppos,
                                                      eboard_p->raid_group_object_id,
                                                      eboard_p->raid_group_offset,
                                                      "coherency",    /* Error type. */ 
                                                      FBE_SECTOR_TRACE_ERROR_LEVEL_ERROR,    /* Trace level */
                                                      trace_coh_error_flag    /* Trace error type */); 
        if (status != FBE_STATUS_OK)
        {
            return status; 
        }
    }

    /* If there are no error, rebuild the stamps for the parity strip.
     */
    if (Scratch.fatal_cnt == 0)
    {
        /* No fatalities. Check all the stamps for
         * consistency across the stripe. Fix up
         * anything suspicious.
         */
        fbe_xor_rebuild_parity_stamps(&Scratch,
                                      eboard_p,
                                      sector_p,
                                      bitkey,
                                      width,
                                      ppos,
                                      seed,
                                      FBE_TRUE,
                                      sector_p[ppos]);
    }

    /*! @todo What do we report for this lost data?
     */
    if (Scratch.fatal_cnt > 1)
    {
        /* There are too many fatalities; we can't reliably
         * reconstruct data or rebuild parity. Kiss this
         * customer's data good-bye!
         */
    }

    else if (Scratch.fatal_cnt == 0)
    {
        /* No fatalities. We have already rebuild the parity stamps.
         */
    }

    else if (b_preserve_shed_data &&
             Scratch.b_valid_shed &&
             (bitkey[ppos] == Scratch.fatal_key))
    {
        /* We found shed data and need to preserve it.
         * Do not reconstruct parity. 
         */
        invalid_bitmap &= (~bitkey[ppos]);
        eboard_p->u_crc_bitmap &= (~bitkey[ppos]);

        /* We need to check the rest of the stamps before 
         * we leave since some of this data might get written out. 
         * We set the parity sector to NULL since we have shed 
         * data and do not want this to be mistaken for parity. 
         */
        fbe_xor_rebuild_parity_stamps(&Scratch, eboard_p, sector_p, bitkey, width,
                                      ppos, seed, FBE_FALSE, NULL    /* parity sector. */);
    }
    else if (bitkey[ppos] == Scratch.fatal_key)
    {
        /* The proper content for parity lies in the scratch buffer.  After 
         * we copy those contents to their proper place, we'll need to set 
         * the stamps to match those in the data sectors as closely as 
         * possible.
         */
        status =  fbe_xor_raid5_rebuild_parity_from_scratch(&Scratch,
                                                            sector_p,
                                                            eboard_p,
                                                            bitkey,
                                                            width,
                                                            ppos,
                                                            rpos,
                                                            seed);
        if (status != FBE_STATUS_OK)
        {
            return status; 
        }
    }

    else if (fbe_xor_rebuild_data_stamps(&Scratch,
                                         eboard_p,
                                         sector_p,
                                         bitkey,
                                         width,
                                         ppos))
    {
        /* Else if we successfully rebuilt the stamps attempt to rebuild
         * the data.
         */
        status =  fbe_xor_raid5_rebuild_data_sector(&Scratch,
                                                    eboard_p,
                                                    bitkey,
                                                    ppos,
                                                    rpos,
                                                    seed);
        if (status != FBE_STATUS_OK)
        {
            return status; 
        }
    }

    else
    {
        /* The remaining data was not consistent with
         * the parity, so we cannot rebuild the lost
         * data sector.
         */
    }

    /* We must re-invalidate previously invalidated sectors.
     * For every sector with an error which went uncorrected, we must force 
     * an invalid sector out - we don't want old or inconsistent data
     * suddenly re-appearing.
     */
    invalid_bitmap = (eboard_p->u_crc_bitmap |
                      eboard_p->u_coh_bitmap |
                      eboard_p->u_ss_bitmap  |
                      eboard_p->u_ts_bitmap  |
                      eboard_p->u_ws_bitmap    );

    /* We has lost at least (1) posiiton.  Invoke the method that will invalidate
     * that position and update parity as required.
     */
    if (0 != invalid_bitmap)
    {
        invalid_bitmap = fbe_xor_eval_parity_invalidate( &Scratch,
                                                         invalid_bitmap, 
                                                         media_err_bitmap | retry_err_bitmap,
                                                         eboard_p, 
                                                         sector_p,    /* Vector of sectors. */
                                                         bitkey,    /* Vector of bitkeys. */
                                                         width, 
                                                         ppos, 
                                                         rpos,
                                                         b_final_recovery_attempt /* If this is the final attempt*/);
    }

    /* We always reconstruct parity, but if we are degraded
     * there should be no `correctable' CRC errors.
     */
    if (XOR_COND((rpos != FBE_XOR_INV_DRV_POS)                  &&
                 ((eboard_p->c_crc_bitmap & ~bitkey[ppos])!= 0)    ))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "(rpos 0x%x == FBE_XOR_INV_DRV_POS 0x%x) && "
                              "(eboard_p->c_crc_bitmap 0x%x == 0)\n",
                              rpos,
                              FBE_XOR_INV_DRV_POS,
                              eboard_p->c_crc_bitmap);  
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate that all invalidations are marked as uncorrectable.
     */
    c_crc_non_parity_bitmap = eboard_p->c_crc_bitmap & invalid_bitmap;
    if ((c_crc_non_parity_bitmap & eboard_p->crc_invalid_bitmap)  ||
        (c_crc_non_parity_bitmap & eboard_p->corrupt_crc_bitmap)  ||
        (c_crc_non_parity_bitmap & eboard_p->crc_raid_bitmap)     ||
        (c_crc_non_parity_bitmap & eboard_p->crc_dh_bitmap)       ||
        (c_crc_non_parity_bitmap & eboard_p->crc_copy_bitmap)     ||
        (c_crc_non_parity_bitmap & eboard_p->crc_pvd_metadata_bitmap)    )
    {
        /* Report and error.
         */
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_WARNING,
                              "xor: Unexpected correctable invalidated lba: 0x%llx rebuild pos: %d parity pos: %d fatal_key: 0x%x\n",
                              Scratch.seed, rpos, ppos, Scratch.fatal_key);
        fbe_base_library_trace(FBE_LIBRARY_ID_XOR, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                               "xor: c_crc_bitmap: 0x%x crc_invalid_bitmap: 0x%x crc_raid_bitmap: 0x%x \n",
                               eboard_p->c_crc_bitmap, eboard_p->crc_invalid_bitmap, eboard_p->crc_raid_bitmap);
        fbe_base_library_trace(FBE_LIBRARY_ID_XOR, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "xor: corrupt_crc_bitmap: 0x%x corrupt_data_bitmap: 0x%x crc_dh_bitmap: 0x%x \n",
                               eboard_p->corrupt_crc_bitmap, eboard_p->corrupt_data_bitmap, eboard_p->crc_dh_bitmap);
        fbe_base_library_trace(FBE_LIBRARY_ID_XOR, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "xor: crc_copy_bitmap: 0x%x crc_pvd_metadata: 0x%x\n",
                               eboard_p->crc_copy_bitmap, eboard_p->crc_pvd_metadata_bitmap);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Remove any retryable errors from the modified bitmap. 
     * We do not want to write out these corrections. 
     * We also do not want to report these as correctable errors. 
     */
    eboard_p->m_bitmap &= ~eboard_p->retry_err_bitmap;
    eboard_p->c_crc_bitmap &= ~eboard_p->retry_err_bitmap;

    /* Were any checksum errors caused by media errors?
     *
     * This is information we need to setup in the eboard,
     * and this is the logical place to determine
     * which errors were the result of media errors.
     */
    status = fbe_xor_determine_media_errors( eboard_p, media_err_bitmap );
    if (status != FBE_STATUS_OK)
    {
        return status; 
    }

    *invalid_bitmask_p = invalid_bitmap;
    return FBE_STATUS_OK;
}
/* fbe_xor_eval_parity_unit() */

/*****************************************************************************
 *          fbe_xor_raid5_reconstruct_parity()
 ***************************************************************************** 
 * 
 * @brief   This function reconstructs parity for a raid 5. This function only
 *          initializes the parity timestamps and writestamps to
 *          FBE_SECTOR_INVALID_TSTAMP and 0 respectively.  It does not modify
 *          the data stamps since that is done later.
 *
 * @param   scratch_p - Pointer to RAID-5 scratch information (seed, option etc)
 * @param   raid_invalidate_bitmap - Bitmap of positions lost to raid verify
 * @param   eboard_p - Pointer to error board for the request
 * @param   sector_p - Vector of sectors.
 * @param   bitkey - Each index in this vector has a bitmask which tells
 *                  us which position a corresponding index in
 *                  the sector array has.                 
 *                  In other words, this is the bit position to 
 *                  modify within eboard_p to save the error type.
 * @param   width - Disks in array.
 * @param   parity_pos - Index containing parity info.
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
static fbe_status_t fbe_xor_raid5_reconstruct_parity( const fbe_xor_scratch_t *const scratch_p,
                                                      const fbe_u16_t raid_invalidate_bitmap,
                                                      fbe_xor_error_t *const eboard_p,
                                                      fbe_sector_t *const sector_p[],
                                                      const fbe_u16_t bitkey[],
                                                      const fbe_u16_t width,
                                                      const fbe_u16_t parity_pos)
{
    fbe_status_t    status = FBE_STATUS_OK; /* Status of the reconstruct.       */
    fbe_u32_t       parity_csum = 0;        /* Running checksum of row parity.  */
    fbe_u32_t       data_csum;              /* The current data block checksum. */
    fbe_u16_t       data_csum_cooked_16;    /* The cooked data checksum.        */
    fbe_s32_t       disk_index;             /* Index of a data position.        */
    fbe_sector_t   *row_parity_blk_ptr;     /* Pointer of row parity block.     */
    fbe_sector_t   *data_blk_ptr;           /* Pointer of data block.           */
    fbe_bool_t      b_is_first_disk = FBE_TRUE; /* Is this the first data disk in the strip? */
        

    /* Init the row block pointers.
     */
    row_parity_blk_ptr = sector_p[parity_pos];

    /* Loop over all drives, calculating the row parity.
     */
    for (disk_index = 0; disk_index < width; disk_index++)
    {
        /* If this disk is a parity disk, skip it.
         */
        if (disk_index != parity_pos)
        {
            /* Get a ptr to the data sector.
             */
            data_blk_ptr = sector_p[disk_index];

            /* Calculates a checksum for data within a sector using the
             * "classic" checksum algorithm; this is the first pass over
             * the data so the parity blocks are initialized based on
             * the data.
             */
            if (b_is_first_disk == FBE_TRUE)
            {
                b_is_first_disk = FBE_FALSE;
                data_csum = xorlib_calc_csum_and_cpy(data_blk_ptr->data_word,
                                                     row_parity_blk_ptr->data_word);
            }
            else
            {
                data_csum = xorlib_calc_csum_and_xor(data_blk_ptr->data_word, 
                                                     row_parity_blk_ptr->data_word);
            }

            /* Update the checksum for parity generate the cooked data checksum.
             * Parity will alway have good checksum.
             */
            parity_csum ^= data_csum;
            data_csum_cooked_16 = xorlib_cook_csum(data_csum, XORLIB_SEED_UNREFERENCED_VALUE);

            /* For RAID-5 raid groups we could have client `data lost - invalidated'
             * sectors.  These sectors might not be in the current bitmap of sectors 
             * that were lost due to `raid verify' (raid_invalidate_bitmap).  If that 
             * is the case set the correct flags  in the eboard.
             */
            if (data_blk_ptr->crc != data_csum_cooked_16)
            {
                /* If this position was not invalidated due to raid verify.
                 */
                if ((bitkey[disk_index] & raid_invalidate_bitmap) == 0)
                {
                    fbe_xor_status_t xor_status;
                    
                    /* Handle the data block checksum error on read.
                     */
                    xor_status = fbe_xor_handle_bad_crc_on_read(data_blk_ptr, 
                                                                scratch_p->seed, 
                                                                bitkey[disk_index],
                                                                scratch_p->option,
                                                                eboard_p, 
                                                                NULL,
                                                                width);
                }
                    
                /*! @note Data that has been lost (invalidated) should never
                 *        have a valid checksum.
                 */
                status = fbe_xor_validate_data_if_enabled(data_blk_ptr, 
                                                          scratch_p->seed, 
                                                          (FBE_XOR_OPTION_VALIDATE_DATA | scratch_p->option));
                if (status != FBE_STATUS_OK)
                {
                    return status; 
                }
            }
                    
            /*! @note We don't modify any of the data stamps at this time. A 
             *        separate method will make all the stamps consistent.
             */
        } /* end if not parity */

    } /* end for all positions */

    /* Set up the metadata in the parity sector.  We intentionally use the
     * invalid R5 timestamp so that it will be obvious we have written this 
     * strip, AND because we don't know how many other drives we will be 
     * writing.  We don't modify the shed stamp but it assumed to 0 since 
     * we should have handled shed data earlier. 
     */
    row_parity_blk_ptr->time_stamp = FBE_SECTOR_INVALID_TSTAMP;
    row_parity_blk_ptr->write_stamp = 0;
            
    /* Next, set the row parity checksum
     */
    row_parity_blk_ptr->crc = xorlib_cook_csum(parity_csum, XORLIB_SEED_UNREFERENCED_VALUE);

    /* Depending on which debug options have been set, validate the reconstructed
     * parity sector.  We cannot validate parity if there are any invalidations.
     */
    if ((raid_invalidate_bitmap & ~bitkey[parity_pos]) != 0)
    {
        /* Cannot validate parity since the invalidation data is not constant.
         */
    }
    else
    {
        /* Else validate parity if enabled
         */
        status = fbe_xor_validate_data_if_enabled(row_parity_blk_ptr, 
                                                  scratch_p->seed, 
                                                  scratch_p->option);
    }

    /* Return the status
     */
    return status;
}
/******************************************** 
 * end of fbe_xor_raid5_reconstruct_parity()
 ********************************************/

/*****************************************************************************
 *          fbe_xor_eval_parity_invalidate()
 *****************************************************************************
 *
 * @brief   Now that fbe_xor_eval_parity_unit has fixed as much as it can,
 *          this function determines which blocks to invalidate.
 * 
 * @param   scratch_p - Pointer to RAID-5 scratch information (seed, option etc)
 * @param   invalidate_bitmap - Bitmap of invalid positions that must be
 *                      invalidated or re-invalidated.
 * @param   media_err_bitmap - Bitmap of media err positions.
 * @param   eboard_p - Error board for marking different
 *                      types of errors.
 * @param   sector_p - Vector of sectors.
 * @param   bitkey - Each index in this vector has a bitmask which tells
 *                  us which position a corresponding index in
 *                  the sector array has.                 
 *                  In other words, this is the bit position to 
 *                  modify within eboard_p to save the error type.
 * @param   width - Disks in array.
 * @param   seed - Seed of block, which is usually the lba.
 * @param   parity_pos - Index containing parity info.
 * @param   rebuild_pos - Index to be rebuilt, -1 if no rebuild required.
 * @param   b_final_recovery_attempt - FBE_TRUE - This is final recovery attempt
 *
 * @return  bitmask - Bitmask indicating which sectors have been invalidated.
 *
 * @note    We no longer invalidate parity.  If required we reconstructed it.
 *          
 *
 * @author
 *  08/19/05 - Created. Rob Foley
 *
 *****************************************************************************/
static fbe_u16_t fbe_xor_eval_parity_invalidate( fbe_xor_scratch_t *scratch_p,
                                                 fbe_u16_t invalidate_bitmap, 
                                                 fbe_u16_t media_err_bitmap,
                                                 fbe_xor_error_t * eboard_p,
                                                 fbe_sector_t * sector_p[],
                                                 fbe_u16_t bitkey[],
                                                 fbe_u16_t width,
                                                 fbe_u16_t parity_pos,
                                                 fbe_u16_t rebuild_pos,
                                                 fbe_bool_t b_final_recovery_attempt)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u16_t       fatal_parity_bitkey;    /* Hold a fatal parity bitkey. */
    fbe_u32_t       parity_bitkey;          /* Bitkey of parity positions. */ 
    fbe_u16_t       dead_bitkey = 0;        /* Bitkey of all dead positions. */
    fbe_u16_t       new_invalidation_bitmap = 0;     

    /* Currently we don't use `final recovery attempt'.  This may change in the
     * future.
     */
    FBE_UNREFERENCED_PARAMETER(media_err_bitmap);
    FBE_UNREFERENCED_PARAMETER(b_final_recovery_attempt);

    /* Build the parity bitkey and fatal parity bitkey
     */
    parity_bitkey =  bitkey[parity_pos];
    fatal_parity_bitkey = (parity_bitkey & invalidate_bitmap);
    new_invalidation_bitmap = invalidate_bitmap;
    new_invalidation_bitmap &= ~(eboard_p->crc_invalid_bitmap  | 
                                 eboard_p->crc_raid_bitmap     |
                                 eboard_p->corrupt_crc_bitmap    );
    
    /* Construct a dead_bitkey from the dead position.
     */
    if (rebuild_pos != FBE_XOR_INV_DRV_POS)
    {
        dead_bitkey |= bitkey[rebuild_pos];

        /*! @note It is possible that we need to invalidate the rebuild
         *        position due to too many errors.  Log an informational
         *        message if enabled.
         */
        if (XOR_COND(bitkey[rebuild_pos] & new_invalidation_bitmap))
        {
            /* If enabled, trace the fact that we lost the rebuild data or
             * the rebuilt data was in-fact previously invalidated.
             */
            if (scratch_p->option & FBE_XOR_OPTION_DEBUG)
            {
                /* We lost the data that we were trying to rebuild.
                 */
                fbe_base_library_trace(FBE_LIBRARY_ID_XOR, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "xor: Invalidating rebuild pos: %d seed: 0x%llx fatal key: 0x%x fatat cnt: %d inval bitmap: 0x%x width: %d\n",
                                  rebuild_pos, scratch_p->seed, scratch_p->fatal_key, scratch_p->fatal_cnt, invalidate_bitmap, width);
            }
        }

    } /* end if there is a position to rebuild */
    
    /* Only remove the parity from the invalidate bitmap if it is not shed data.
     */
    if (scratch_p->b_valid_shed == FBE_FALSE)
    {
        /* Remove the row parity from the invalid bitmap. We won't be
         * invalidating parity, but might be reconstructing it.
         */
        invalidate_bitmap &= ~bitkey[parity_pos];
    }

    /* Invalidate all the data sectors in the invalidate_bitmap.
     */
    fbe_xor_invalidate_sectors(sector_p,
                               bitkey,
                               width,
                               scratch_p->seed,
                               invalidate_bitmap,
                               eboard_p);

    /* If we have valid shed data we cannot reconstruct parity.
     */
    if (scratch_p->b_valid_shed == FBE_TRUE)
    {
        /* We need to handle the case where we had valid shed data but it
         * was invalidated.  We need to remove it from the uncorrectable 
         * and invalid bitmask 
         */
        if (eboard_p->u_crc_bitmap & bitkey[parity_pos])
        {
            /* We had shed data and we have an uncorrectable logged against 
             * the parity drive.  We marked an uncorrectable crc error 
             * against parity originally to force it to be reconstructed. 
             *
             * We have now since invalidated parity, because of an uncorrectable,
             * but we do not want to return a crc error since this will get 
             * logged by raid as an unexpected crc error.
             *
             * Raid will automatically log uncorrectables against parity when
             * it detects uncorrectables against data, so it is not
             * necessary to return any error.
             *
             * We do need to mark parity as modified so it will get written
             * out to disk.
            */
            eboard_p->u_crc_bitmap &= (~bitkey[parity_pos]);
            eboard_p->m_bitmap |= bitkey[parity_pos];
            if (XOR_COND(eboard_p->m_bitmap >= (1 << width)))
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "eboard_p->m_bitmap 0x%x >= (1 << width 0x%x)\n",
                                  eboard_p->m_bitmap,
                                  width);  
                return FBE_STATUS_GENERIC_FAILURE;
            }
        
            /*! @todo Although we have invalidated this data position we remove 
             *        it from the invalid bitmap since other code will generate
             *        an error (because they don't check if the parity position
             *        is actually shed data or not).
             */
            invalidate_bitmap &= (~bitkey[parity_pos]);

        } /* end if the shed data has been lost. */

    } /* end if the parity position is shed data */
    else
    {
        /* Else check if we need to reconstruct parity.  If there are any
         * invalidations, we must rebuild parity.  We never invalidate
         * parity.
         */
        if ((invalidate_bitmap != 0)   ||
            (fatal_parity_bitkey != 0)    )
        {
            /* We need to reconstruct parity here since we will not be
             * retrying this operation.
             */
            status = fbe_xor_raid5_reconstruct_parity(scratch_p,
                                                      invalidate_bitmap,
                                                      eboard_p,
                                                      sector_p,
                                                      bitkey,
                                                      width,
                                                      parity_pos);
            if (status != FBE_STATUS_OK) 
            { 
                return status;
            }
       
            /* Now rebuild all the stamps to a consistent value.
             */
            fbe_xor_rebuild_parity_stamps(scratch_p,
                                          eboard_p, 
                                          sector_p, 
                                          bitkey, 
                                          width,
                                          parity_pos, 
                                          scratch_p->seed, 
                                          FBE_FALSE,            /* Don't validate parity */ 
                                          sector_p[parity_pos]  /* Rebuild the parity stamps */);

            /* Make sure we write out the parity blocks.
             */
            eboard_p->m_bitmap |= parity_bitkey;

        } /* end if we need to reconstruct parity */

        /* Clean up the fatal key for parity, since we reconstructed parity.
         * Remove the parity errors from fatal key and fatal count.
         * We also add in the modified bitmap for parity since if we are
         * reconstructing, it needs to be in the modified bitmap.
         */
        fatal_parity_bitkey |= (parity_bitkey & scratch_p->fatal_key);
        if (fatal_parity_bitkey != 0)
        {
            /* Remove the parity position from the fatal information
             */
            if (parity_bitkey & scratch_p->fatal_key)
            {
                scratch_p->fatal_key &= ~fatal_parity_bitkey;
                scratch_p->fatal_cnt--;
            }

            /* Set the modified for the fatal parity
             */
            eboard_p->m_bitmap |= fatal_parity_bitkey;

            /* Correct all uncorrectables for this parity position.
             * We can do this since parity does not have uncorrectables,
             * as parity always gets reconstructed.
             */
            fbe_xor_correct_all_one_pos(eboard_p, fatal_parity_bitkey);
        }

        /* Remove parity from the dead key
         */
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

    } /* end if the parity position is not shed data */

    /* Return the bitmap of positions that have been invalidated */
    return invalidate_bitmap;
}
/*****************************************
 * fbe_xor_eval_parity_invalidate()
 ****************************************/

/***************************************************************************
 *              fbe_xor_eval_parity_sector()
 ***************************************************************************
 *      @brief
 *              This routine is used for Raid5 (or old Raid3) LUs
 *              to determine if the content of the parity sector is
 *              'internally consistent', i.e. the checksum attached to
 *              the data is correct, and that the timstamp, writestamp
 *              and shedstamp values are structured in accordance with
 *              FLARE policy.
 *
 *              scratch_p - [I]   ptr to the scratch database area.
 *              eboard_p  - [I]   error board for marking different
 *                                              types of errors.
 *              pblk_p    - [I]   ptr to parity sector.
 *              pkey      - [I]   bitkey for parity sector.
 *              width     - [I]   disks in array.
 *
 *      @return fbe_status_t
 *
 *      @notes
 *     None.
 *
 *      @author
 *              06-Nov-98       -SPN-   Redesigned to use checksum toolkit.
 *
 ***************************************************************************/

static fbe_status_t fbe_xor_eval_parity_sector(fbe_xor_scratch_t * scratch_p,
                                               fbe_xor_error_t * eboard_p,
                                               fbe_sector_t * pblk_p,
                                               fbe_u16_t pkey,
                                               fbe_u16_t width,
                                               fbe_lba_t seed)
{
    fbe_u32_t       rsum;
    fbe_u16_t       cooked_csum = 0;
    fbe_xor_comparison_result_t cmpstat;
    fbe_status_t    status;
    fbe_u16_t       u_coh_bitmap = 0;

    cmpstat = FBE_XOR_COMPARISON_RESULT_SAME_DATA;

    if (scratch_p->fatal_cnt > 1)
    {
        /* Too many fatalities already to perform reconstruction.
         * Don't bother playing with the scratch buffer anymore.
         */
        rsum = xorlib_calc_csum(pblk_p->data_word);
    }

    else
    {
        fbe_sector_t *sblk_p;

        /* Locate the scratch sector.
         */
        sblk_p = scratch_p->rbld_blk;

        if (scratch_p->fatal_cnt == 0)
        {
            /* So far, there have been no errors. The scratch sector
             * contains the Xor of the data from the data sectors,
             * which should (of course) be the parity. We'll compare
             * the content of the parity with that of the scratch
             * sector as we compute the checksum. (Since we're not
             * Xor'ing, we don't need to worry about backing the
             * data out if we fail one of the internal consistency
             * tests.)
             */
            rsum = xorlib_calc_csum_and_cmp(pblk_p->data_word,
                                             sblk_p->data_word,
                                             (XORLIB_CSUM_CMP *)&cmpstat);
        }

        else
        {
            /* Just a single error. We may be able to reconstruct the
             * lost data. As the checksum is calculated, Xor the parity
             * into the scratch buffer. If the stripe is consistent,
             * this action should yield the missing data.
             */
            rsum = xorlib_calc_csum_and_xor(pblk_p->data_word,
                                             sblk_p->data_word);
        }
    }

    if (pblk_p->crc != (cooked_csum = xorlib_cook_csum(rsum, (fbe_u32_t)scratch_p->seed)))
    {
        /* The checksum calculated for the parity data
         * doesn't match that attached to the sector. Mark
         * this as an uncorrectable checksum error for now.
         */
        eboard_p->u_crc_bitmap |= pkey;

        /* Determine the types of errors.
         */
        status = fbe_xor_determine_csum_error(eboard_p,
                                              pblk_p,
                                              pkey,
                                              seed,
                                              pblk_p->crc,
                                              cooked_csum,
                                              FBE_TRUE);
        if (status != FBE_STATUS_OK)
        {
            return status; 
        }
    }

    else
    {
        fbe_u16_t tstamp,
        wstamp,
        sstamp;

        sstamp = pblk_p->lba_stamp,
        wstamp = pblk_p->write_stamp,
        tstamp = pblk_p->time_stamp;

        /* The checksum was okay, so let's look at
         * the relative consistency of the stamps,
         * starting with the shed stamps.
         */
        if (0 == sstamp)
        {
            /* No shed stamps are set.
             */
        }

        else if ((0 != (sstamp >> width)) ||
                 (0 != (sstamp & pkey)))
        {
            /* The shedstamp has bits set for non-existant
             * drives, or for the parity drive itself. For
             * now, mark this as an uncorrectable shed stamp
             * error.
             */
            eboard_p->u_ss_bitmap |= pkey;
        }

        else
        {
            eboard_p->u_ss_bitmap |= pkey;
        }

        /* Now check the write stamp.
         */
        if (0 == wstamp)
        {
            /* No write stamps are set.
             */
        }

        else if ((0 != (wstamp >> width)) ||
                 (0 != (wstamp & pkey)))
        {
            /* The writestamp has bits set for non-existant
             * drives, or for the parity drive itself. For now,
             * mark this as an uncorrectable write stamp error.
             */
            eboard_p->u_ws_bitmap |= pkey;
        }

        else if (0 != (tstamp & FBE_SECTOR_ALL_TSTAMPS))
        {
            /* The ALL_TIMESTAMPS flag is set, but
             * there are valid bits set in the writestamp
             * as well.
             */
            eboard_p->u_ts_bitmap |= pkey;
        }

        /* Now check the time stamp.
         */
        if ((tstamp == (FBE_SECTOR_ALL_TSTAMPS | FBE_SECTOR_INITIAL_TSTAMP)) ||
            (tstamp == (FBE_SECTOR_ALL_TSTAMPS | FBE_SECTOR_INVALID_TSTAMP)))
        {
            /* If the stripe was written with all
             * timestamps, then neither of these
             * values are permitted.
             */
            eboard_p->u_ts_bitmap |= pkey;
        }

#pragma warning(disable: 4068) /* Suppress warnings about unknown pragmas */
#pragma prefast(disable: 326)    /* Suppress Prefast warning 326 */
        else if ((FBE_SECTOR_INVALID_TSTAMP != FBE_SECTOR_INITIAL_TSTAMP) &&
#pragma prefast(default: 326)    /* Restore defaults */
#pragma warning(default: 4068)
                 (FBE_SECTOR_INVALID_TSTAMP == tstamp))
        {
            /* The parity should never have the invalid
             * timestamp unless it is also the initial
             * binding stamp.
             */
            eboard_p->u_ts_bitmap |= pkey;
        }

        /* If there are no internal consistency errors in
         * this sector, check the coherency of parity with
         * the corresponding data sectors.
         */
        if (cmpstat != FBE_XOR_COMPARISON_RESULT_SAME_DATA)
        {
            if (0 == (pkey & (eboard_p->u_ws_bitmap |
                              eboard_p->u_ss_bitmap |
                              eboard_p->u_ts_bitmap)))
            {
                /* The parity has a good checksum, and the stamps
                 * seem reasonable, but its content doesn't match
                 * what we believe the parity should have. Mark
                 * this as an uncorrectable coherency error for now.
                 */
                eboard_p->u_coh_bitmap |= pkey;
            }
        }
    }

    /*! @todo Remove this option when applicable.
     */
    if (b_xor_raid5_check_for_reconstructable_data == FBE_FALSE)
    {
        u_coh_bitmap = eboard_p->u_coh_bitmap;
    }

    /*! @todo If there are no errors update the running checksum.  We may longer 
     *        include the `u_coh_bitmap' since the data sector is just as likely
     *        to be incorrect as parity.
     */
    if (0 == (pkey & (eboard_p->u_crc_bitmap |
                      u_coh_bitmap           |
                      eboard_p->u_ts_bitmap  |
                      eboard_p->u_ws_bitmap  |
                      eboard_p->u_ss_bitmap    )) )
    {
        /* If there are no internal consistency errors in
         * this sector, then we can use its checksum when
         * regenerating data. Add it to the running checksum
         * information.
         */
        scratch_p->running_csum ^= rsum;
    }

    else if (scratch_p->fatal_cnt++ < 1)
    {
        /* This is the first fatality. Save the
         * position of the drive in the array.
         */
        scratch_p->fatal_key = pkey,
        scratch_p->fatal_blk = pblk_p;

        /* Once again, we don't have to worry about
         * backing the data out if we need to rebuild
         * the parity. We only compared the data...
         * But set the flag so that we copy data from 
         * scratch block to parity block.
         */
        scratch_p->copy_required = FBE_TRUE;

    }

    return FBE_STATUS_OK;
}
/* fbe_xor_eval_parity_sector() */

/*!***************************************************************************
 *          fbe_xor_eval_invalidated_data_sector()
 *****************************************************************************
 *
 * @brief   This methods validates if a data sector was correctly invalidated 
 *          or not. We use the time, write and lba stamps to determine if this 
 *          was a properly invalidated.  If the sector was not properly 
 *          invalidated it is treated like an unexpected CRC error.
 *
 * @param   scratch_p - ptr to the scratch database area.
 * @param   eboard_p - error board for marking different types of errors.
 * @param   dblk_p - ptr to data sector.
 * @param   dkey -bitkey for data sector.
 * @param   b_timestamp_valid - Determines if the time stamp is valid or not
 * @param   b_writestamp_valid - Determines if the write stamp is valid or not
 * @param   b_lbastamp_valid - Determines if the lba stamp is valid or not
 * @param   expected_timestamp - Expected timestamp
 * @param   expected_writestamp - Expected writestamp
 * @param   expected_lbastamp - Expected lba stamp
 *
 * @return  None
 *
 * @author
 *  10/11/2011  Ron Proulx  - Created.
 *   
 *****************************************************************************/

static void fbe_xor_eval_invalidated_data_sector(fbe_xor_scratch_t * scratch_p,
                                                 fbe_xor_error_t * eboard_p,
                                                 fbe_sector_t * dblk_p,
                                                 fbe_u16_t dkey,
                                                 fbe_bool_t b_timestamp_valid,
                                                 fbe_bool_t b_writestamp_valid,
                                                 fbe_bool_t b_lbastamp_valid,
                                                 fbe_u16_t expected_timestamp,
                                                 fbe_u16_t expected_writestamp,
                                                 fbe_u16_t expected_lbastamp)
{
    fbe_bool_t      b_additional_stamp_errors = FBE_FALSE;  /* By default there are no additional stamp errors. */
    fbe_object_id_t rg_object_id = fbe_xor_lib_eboard_get_raid_group_id(eboard_p);

    /*! @todo Currently unreferenced but could be used to validate that this
     *        sector is invalidated.
     */
    FBE_UNREFERENCED_PARAMETER(eboard_p);

    /* Only do anything if this sector has not already been flagged as fatal.
     */
    if ((scratch_p->fatal_key & dkey) == 0)
    {
        /* Check the following fields of the invalidated sector
         *  o write_stamp
         *  o time_stamp
         *  o lba_stamp
        */
        if ((b_writestamp_valid == FBE_TRUE)             &&
            (dblk_p->write_stamp != expected_writestamp)    )
        {
            /* Flag the fact that there are other stamp errors in this sector.
            */
            b_additional_stamp_errors = FBE_TRUE;

            /* Log the write stamp error if enabled.
             */
            if (scratch_p->option & FBE_XOR_OPTION_DEBUG)
            {
                /* Log an informational message since error injection can cause
                 * these errors.
                 */
                fbe_base_library_trace(FBE_LIBRARY_ID_XOR, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                       "xor: Write stamp: 0x%x rg: 0x%x on invalidated lba: 0x%llx not expected: 0x%x \n",
                                       dblk_p->write_stamp, rg_object_id,
				       (unsigned long long)scratch_p->seed,
				       expected_writestamp);
            }
            
            /* Now correct the error.  We don't log the error since the stamps 
             * will be corrected.
             */
            dblk_p->write_stamp = expected_writestamp;
        }

        /* Check the time stamp if valid.
         */
        if ((b_timestamp_valid == FBE_TRUE)            &&
            (dblk_p->time_stamp != expected_timestamp)    ) 
        {
            /* Flag the fact that there are other stamp errors in this sector.
            */
            b_additional_stamp_errors = FBE_TRUE;

            /* Log the timestamp error if enabled
             */
            if (scratch_p->option & FBE_XOR_OPTION_DEBUG)
            {
                /* Log an informational message since error injection can cause
                 * these errors.
                 */
                fbe_base_library_trace(FBE_LIBRARY_ID_XOR, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                       "xor: Time stamp: 0x%x rg: 0x%x on invalidated lba: 0x%llx not expected: 0x%x \n",
                                       dblk_p->time_stamp, rg_object_id,
				       (unsigned long long)scratch_p->seed,
				       expected_timestamp);
            }

            /* Now correct the error.  We don't log the error since the stamps 
             * will be corrected.
             */
            dblk_p->time_stamp = expected_timestamp;
        }

        /* Check the lba stamp if valid.
         */
        if ((b_lbastamp_valid == FBE_TRUE)           &&
            (dblk_p->lba_stamp != expected_lbastamp)    )
        {
            /* Flag the fact that there are other stamp errors in this sector.
            */
            b_additional_stamp_errors = FBE_TRUE;

            /* Log the lba stamp error if enabled.
             */
            if (scratch_p->option & FBE_XOR_OPTION_DEBUG)
            {
                /* Log an informational message since error injection can cause
                 * these errors.
                 */
                fbe_base_library_trace(FBE_LIBRARY_ID_XOR, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                       "xor: LBA stamp rg: 0x%x on %d invalidated lba: 0x%llx not expected: 0x%x \n",
                                       dblk_p->lba_stamp, rg_object_id,
				       (unsigned long long)scratch_p->seed,
				       expected_lbastamp);
            }

            /* Now correct the error.  We don't log the error since the stamps 
             * will be corrected.
             */
            dblk_p->lba_stamp = expected_lbastamp;
        }

        /* If there was a second stamp error, then add this sector to the fatal count.
         */
        if (b_additional_stamp_errors)
        {
            /* Update the fatal information accordingly.
             */
            if (scratch_p->fatal_cnt == 0)
            {
                /* This is the first fatality. Save the
                 * position of the drive in the array.
                 */
                scratch_p->fatal_key = dkey,
                scratch_p->fatal_blk = dblk_p;

                /* If we can perform a reconstruction, we'll have
                 * to back this data out of the scratch sector.
                 */
                scratch_p->xor_required = FBE_TRUE;
            }

            /* Increment fatal count.
             */
            scratch_p->fatal_cnt++;
        }

    } /* end if this data sector is not in the fatal bitmask */

    /* Return
     */
    return;
}
/***********************************************
 * end of fbe_xor_eval_invalidated_data_sector()
 ***********************************************/

/***************************************************************************
 *              fbe_xor_eval_data_sector()
 ***************************************************************************
 *      @brief
 *              This routine is used for Raid5 (or old Raid3) LUs
 *              to determine if the content of the data sector is
 *              'internally consistent', i.e. the checksum attached to
 *              the data is correct, and that the timstamp, writestamp
 *              and shedstamp values are structured in accordance with
 *              FLARE policy.
 *

 *              scratch_p - [I]   ptr to the scratch database area.
 *              eboard_p  - [I]   error board for marking different
 *                                              types of errors.
 *              dblk      - [I]   ptr to data sector.
 *              dkey      - [I]   bitkey for data sector.
 *
 *      @return fbe_status_t
 *
 *      @notes
 *
 *      @author
 *              06-Nov-98       -SPN-   Redesigned to use checksum toolkit.
 ***************************************************************************/

static fbe_status_t fbe_xor_eval_data_sector(fbe_xor_scratch_t * scratch_p,
                                             fbe_xor_error_t * eboard_p,
                                             fbe_sector_t * dblk_p,
                                             fbe_u16_t dkey,
                                             fbe_lba_t seed)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       rsum;
    fbe_u32_t       cooked_csum = 0;
    xorlib_sector_invalid_reason_t invalidation_reason = XORLIB_SECTOR_INVALID_REASON_UNKNOWN;
    fbe_bool_t      b_is_invalidated = FBE_FALSE;
    fbe_u16_t       invalidated_bitmap = 0;

    /* We always reconstruct parity*/
    /* If too many errors don't continue.
     */
    if (scratch_p->fatal_cnt > 1)
    {
        /* Already too many fatalities to allow a reconstruction.
         * Don't be concerned with the scratch sector anymore.
         * Just compute the checksum.
         */
        rsum = xorlib_calc_csum(dblk_p->data_word);
    }

    else
    {
        fbe_sector_t *sblk_p;

        /* Locate the sector within the scratch database area.
         */
        sblk_p = scratch_p->rbld_blk;

        if (scratch_p->initialized)
        {
            /* The scratch sector has been initialized, so
             * Xor the data sector into the  sector.
             */
            rsum = xorlib_calc_csum_and_xor(dblk_p->data_word,
                                             sblk_p->data_word);
        }

        else
        {
            /* The scratch sector has not been initialized, so
             * copy the data sector into the scratch sector.
             */
            scratch_p->initialized = FBE_TRUE;

            rsum = xorlib_calc_csum_and_cpy(dblk_p->data_word,
                                             sblk_p->data_word);
        }
    }

    /* If the checksum read doesn't match the calculated checksum we should 
     * report the error.  Even if the sectors were previously invalidated
     * we must `re-invalidate' them so that the `hit count' etc gets updated.
     */  
    cooked_csum = xorlib_cook_csum(rsum, XORLIB_SEED_UNREFERENCED_VALUE);
    if (dblk_p->crc != cooked_csum)
    {
        /* If the sector was invalidated do not flag it as a fatal error.
         * It is ok to ignore any other stamp errors (ts, ws, etc) since
         * we always re-invalidate previously invalidated sectors.  The stamps
         * will be updated when the re-invalidation occurs.
         */
        b_is_invalidated = fbe_xor_is_sector_invalidated(dblk_p, &invalidation_reason);

        /* `Corrupt Data' is a special case where we treat the sector as an 
         * uncorrectable crc error.  The data will be reconstructed from parity.
         * 'DH_INVALIDATED' is another case where the disk handler invalidated the data 
         * unbeknownst to RAID, so the parity was not updated.  Reconstruct it
         * from parity.
         */
        if ((b_is_invalidated == FBE_TRUE)                                                               &&
            (xorlib_should_invalidated_sector_be_marked_uncorrectable(invalidation_reason) == FBE_FALSE)    )  
        {
            /* This sector was invalidated.  Flag the fact so that we set the
             * the bit in u_crc_bitmap at the end of this routine.
             */
            invalidated_bitmap |= dkey;
        }
        else
        {
            /* Else the checksum is bad. Mark this as an uncorrectable checksum 
             * error for now.
             */
            eboard_p->u_crc_bitmap |= dkey;
        }

        /* Now determine and set the appropriate crc bitmap.
         */
        status = fbe_xor_determine_csum_error(eboard_p,
                                              dblk_p,
                                              dkey,
                                              seed,
                                              dblk_p->crc,
                                              cooked_csum,
                                              FBE_TRUE);
        if (status != FBE_STATUS_OK)
        {
            return status; 
        }
    }

    /* If the crc was either correct or the known invalidation value, continue 
     * to check the lba stamp etc.
     */
    if ((eboard_p->u_crc_bitmap & dkey) == 0)
    {
        /* We treat invalidated blocks just like normal data blocks.  Validate 
         * the lba stamp.
         */
        if (!xorlib_is_valid_lba_stamp(&dblk_p->lba_stamp, seed, eboard_p->raid_group_offset))
        {
            /* The shedstamp does not match the expected
             * lba stamp, and is non-zero. This is treated
             * the same as a checksum error.
             */
            eboard_p->u_crc_bitmap |= dkey;
    
            /* Log the Error. 
             */
            status = fbe_xor_record_invalid_lba_stamp(seed, dkey, eboard_p, dblk_p);
            if (status != FBE_STATUS_OK)
            {
                return status; 
            }
        }
        else
        {
            /* The checksum was okay, so let's look
             * at the rest of the metadata. 
             */
            if (0 != (dblk_p->time_stamp & FBE_SECTOR_ALL_TSTAMPS))
            {
                /* The ALL_TIMESTAMPS bit should never be set
                 * on a data sector. For now, mark this as
                 * an uncorrectable time stamp error.
                 */
                eboard_p->u_ts_bitmap |= dkey;
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
            }
    
            else if (dblk_p->time_stamp != FBE_SECTOR_INVALID_TSTAMP)
            {
                /* The correct bit is set in the writestamp, but
                 * the timestamp has not been invalidated accordingly.
                 * This could go either way. For now, mark this as an
                 * uncorrectable time stamp error.
                 */
                eboard_p->u_ts_bitmap |= dkey;
            }
        }

    } /* end if this was an invalidated sector */

    /* If there are errors with this sector, add it to the running csum.
     */
    if (0 == (dkey & (eboard_p->u_crc_bitmap |
                      eboard_p->u_ts_bitmap  |
                      eboard_p->u_ws_bitmap  |
                      eboard_p->u_ss_bitmap    )) )
    {
        /* There are no internal consistency errors within
         * this sector, so we can use the raw checksum data
         * for reconstructing injured data or rebuilding parity.
         */
        scratch_p->running_csum ^= rsum;
    }

    else
    {
        if (scratch_p->fatal_cnt == 0)
        {
            /* This is the first fatality. Save the
             * position of the drive in the array.
             */
            scratch_p->fatal_key = dkey,
            scratch_p->fatal_blk = dblk_p;

            /* If we can perform a reconstruction, we'll have
             * to back this data out of the scratch sector.
             */
            scratch_p->xor_required = FBE_TRUE;
        }

        /* Increment fatal count.
         */
        scratch_p->fatal_cnt++;
    }

    /* If the sector was invalidated, flag it in the u_crc_bitmap so that
     * it gets re-invalidated.
     */
    if (invalidated_bitmap != 0)
    {
        /* Mark the invalidated sector in the uncorrectable crc bitmap.
         */
        eboard_p->u_crc_bitmap |= invalidated_bitmap;
    }

    /* Return success.
     */
    return FBE_STATUS_OK;
}
/* fbe_xor_eval_data_sector() */

/***************************************************************************
 *              fbe_xor_eval_shed_sector()
 ***************************************************************************
 * @brief
 *   This function handles a shed data sector.
 *   If the shed data is valid then:
 *               1) shed data is copied to the data block,
 *               2) stamps in the data block are cleared.
 *               3) parity is marked invalid, so we will rebuild it.
 *
 * 

 *  scratch_p   -[IO] ptr to scratch info
 *  eboard_p    -[IO] ptr to error board
 *  sector_p[]  -[IO] ptr to sectors in strip
 *  bitkey[]    -[IO] ptr to bitkeys for strip
 *  ppos        -[IO] parity position
 *  spos        -[IO] position for which the parity drive has shed data.
 *  b_preserve_shed_data - [IO] FBE_TRUE to preserve shed data,
 *                              FBE_FALSE to rebuild parity.
 *  u_bitmask_p -[O] bitmask indicaing uncorrectable errors
 *
 * @return fbe_status_t
 *
 * @notes
 *
 * @author
 *  18-Aug-99: Rob Foley Created.
 *
 ***************************************************************************/

static  fbe_status_t fbe_xor_eval_shed_sector(fbe_xor_scratch_t * scratch_p,
                                              fbe_xor_error_t * eboard_p,
                                              fbe_sector_t * sector_p[],
                                              fbe_u16_t bitkey[],
                                              fbe_u16_t ppos,
                                              fbe_u16_t spos,
                                              fbe_lba_t seed,
                                              fbe_bool_t b_preserve_shed_data,
                                              fbe_u16_t *u_bitmask_p)
{
    fbe_status_t status;

    /* Our u_crc bitmap should always be set here.
     */
    if (XOR_COND((eboard_p->u_crc_bitmap & bitkey[spos]) == 0))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "(eboard_p->u_crc_bitmap 0x%x & bitkey[%d] 0x%x) == 0 \n",
                              eboard_p->u_crc_bitmap,
                              spos,
                              bitkey[spos]);  
        return FBE_STATUS_GENERIC_FAILURE;
    }

    {
        /* Handle shed data.
         */

        fbe_u32_t rsum;
        fbe_u32_t cooked_csum = 0;
        fbe_sector_t *pblk_p;
        fbe_sector_t *dblk_p;

        /* Setup data block ptr */
        dblk_p = sector_p[spos];

        /* Setup parity block ptr */
        pblk_p = sector_p[ppos];

        /* Copy the shed data sector -
         * From: parity position (shed data)
         *   To: data position
         */
        rsum = xorlib_calc_csum_and_cpy(pblk_p->data_word,
                                        dblk_p->data_word);

        /* If the checksum read doesn't match the calculated checksum.
         * we should report the error. 
         */  
        cooked_csum = xorlib_cook_csum(rsum, XORLIB_SEED_UNREFERENCED_VALUE);
        if (pblk_p->crc != cooked_csum)
        {
            /* Bad checksum for the shed data sector, mark parity bad.
             * This is an uncorrectable shed stamp error,
             * since the rebuild position is already gone.
             */
            eboard_p->u_crc_bitmap |= bitkey[ppos];
            scratch_p->fatal_cnt++;

            /* Determine the types of errors.
             */
            status= fbe_xor_determine_csum_error( eboard_p, pblk_p, bitkey[ppos], seed,
                                                  pblk_p->crc, cooked_csum, FBE_TRUE);
            if (status != FBE_STATUS_OK)
            {
                return status; 
            }
        }
        else if (sector_p[ppos]->lba_stamp != bitkey[spos])
        {
            /* Checksum looks good, so we can look at the stamps.
             *
             * If the shed bit does not match the bit for the
             * known degraded position, then the wrong shed bit is set, 
             * mark parity bad.
             * This is an uncorrectable shed stamp error.
             */
            eboard_p->u_ss_bitmap |= bitkey[ppos];
            scratch_p->fatal_cnt++;
        }
        else if (sector_p[ppos]->write_stamp != 0)
        {
            /* Write stamp should always be zero for shed data.
             */
            eboard_p->u_ws_bitmap |= bitkey[ppos];
            scratch_p->fatal_cnt++;
        }
        else if (sector_p[ppos]->time_stamp != FBE_SECTOR_INITIAL_TSTAMP)
        {
            /* Time stamp should always be invalid for shed data.
             */
            eboard_p->u_ts_bitmap |= bitkey[ppos];
            scratch_p->fatal_cnt++;
        }
        else
        {
            /************************************************************
             * Valid shed data.
             *
             * Checksum is good.
             * The shed data has already been moved 
             * from parity to data sector.
             *
             * Next, clear this error and set the stamps.
             * Start by clearing the error from the data drive.
             ************************************************************/

            eboard_p->u_crc_bitmap &= ~bitkey[spos];

            /* We explicitly do not mark this as a correctable error,
             * but rather as a modified position.
             */
            eboard_p->m_bitmap |= bitkey[spos];

            /* Fix up stamps on the data, since it is now OK.
             */
            dblk_p->crc = pblk_p->crc;
            dblk_p->lba_stamp = xorlib_generate_lba_stamp(seed, eboard_p->raid_group_offset);
            dblk_p->write_stamp = 0;
            dblk_p->time_stamp = FBE_SECTOR_INVALID_TSTAMP;

            /************************************************************
             * Mark the parity sector as bad.
             * This causes us to rebuild parity.
             *   - We xor the rest of the data sectors into parity
             *     as we are checking these data CRCs.
             *   - The rebuild position's component of the
             *     partial product is already in the parity pos.
             * 
             * The fatal_cnt stays at 1 from the rebuild u_crc,
             * but now the parity is marked as needing a rebuild.
             ************************************************************/

            scratch_p->initialized = FBE_TRUE;
            scratch_p->running_csum = rsum;
            eboard_p->u_crc_bitmap |= bitkey[ppos];
            scratch_p->fatal_key = bitkey[ppos];
            if (b_preserve_shed_data == FBE_TRUE)
            {
                /* Don't rebuild anything.  The shed data was already copied over, 
                 * and the parity data will be left as is. 
                 */
                scratch_p->fatal_blk = NULL;
                scratch_p->rbld_blk = &scratch_p->scratch_sector;
            }
            else
            {
                scratch_p->fatal_blk =
                scratch_p->rbld_blk = sector_p[ppos];
            }
            /* Mark the scratch with info that we have a valid shed
             * sector.  We will use this later to manipulate the
             * eboard to remove the fact that we have an error on
             * parity.
             */
            scratch_p->b_valid_shed = FBE_TRUE;
        }
    }

    *u_bitmask_p = (eboard_p->u_crc_bitmap |
                    eboard_p->u_coh_bitmap |
                    eboard_p->u_ss_bitmap |
                    eboard_p->u_ts_bitmap |
                    eboard_p->u_ws_bitmap);

    return FBE_STATUS_OK;
}  /* fbe_xor_eval_shed_sector() */

/*!***************************************************************************
 *          fbe_xor_rebuild_sector()
 *****************************************************************************
 *
 * @brief   This routine is used for Raid5 (or old Raid3) LUs to reconstruct
 *          the content of a sector (data or parity).
 *
 * @param   scratch_p - [I]   ptr to a Scratch database area.
 * @param   eboard_p - Pointer to eboard to update
 *
 * @return status - Typically FBE_STATUS_OK
 *
 * @note
 *
 * @author
 *  06-Nov-98       -SPN-   Redesigned to use checksum toolkit.
 ***************************************************************************/

static fbe_status_t fbe_xor_rebuild_sector(fbe_xor_scratch_t *scratch_p,
                                           fbe_xor_error_t *eboard_p)
{
    fbe_sector_t *rblk_p;

    /* Validate that exactly (1) bit is set in the fatal_key.
     */
    if (fbe_xor_get_bitcount(scratch_p->fatal_key) != 1)
    {
        /* This is unexpected since we can only reconstruct (1) RAID-5 sector.
         */
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "xor: recontruct seed: 0x%llx unexpected fatal_key: 0x%x",
                              (unsigned long long)scratch_p->seed,
			      scratch_p->fatal_key);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the sector to rebuild
     */
    rblk_p = scratch_p->fatal_blk;
    {
        fbe_sector_t *sblk_p;

        sblk_p = scratch_p->rbld_blk;

        if (sblk_p == rblk_p)
        {
            /* We've known this block was bad from the
             * very start, so we've rebuilt its content
             * in-place.
             */
        }

        /* Determine which action to take
         */
        if ((scratch_p->copy_required)                   &&
            (b_xor_raid5_check_for_reconstructable_data)    )
        {
            /* Fatal block is parity sector. Parity data is accumulated
             * into scratch buffer. So, we need to copy it from scratch to 
             * parity block.  
             */
            (void) xorlib_calc_csum_and_cpy(sblk_p->data_word,
                                            rblk_p->data_word);
        }

        else if (scratch_p->xor_required)
        {
            /* The contents of the target sector were Xor'd
             * into the scratch sector before the checksum
             * was checked; to back it out, we need to Xor
             * it again. The checksum information isn't
             * needed, as we've already calculated it
             * elsewhere.
             */
            (void) xorlib_calc_csum_and_xor(sblk_p->data_word,
                                            rblk_p->data_word);
        }

        else
        {
            /* The contents of the target sector were never
             * Xor'd into the scratch sector, so the contents
             * of the scratch sector should contain the new
             * target data. Copy the data from the scratch sector
             * into the target sector. No checksum information
             * is needed.
             */
            xorlib_copy_data(sblk_p->data_word,
                             rblk_p->data_word);
        }

    }

    /* Calculate the new checksum using the information
     * saved during earlier processing, and set it into
     * the target sector.
     */
    rblk_p->crc = xorlib_cook_csum(scratch_p->running_csum,
                                    (fbe_u32_t)scratch_p->seed);

    /* We have now rebuilt the sector so clear any assocated uncorrectables.
     */
    fbe_xor_correct_all_one_pos(eboard_p, scratch_p->fatal_key);

    /* Update the `modified' bitmap
     */
    eboard_p->m_bitmap |= scratch_p->fatal_key;

    /* Return the status
     */
    return FBE_STATUS_OK;
}
/* fbe_xor_rebuild_sector() */


/***************************************************************************
 *              fbe_xor_rebuild_data_stamps()
 ***************************************************************************
 *      @brief
 *              This routine is used for Raid5 (or old Raid3) LUs to
 *              determine if the coherency information indicates that it
 *              is legal/valid to rebuild the contents of a data sector
 *              from the others. This is what prevents us from 'inventing'
 *              data if, for example, the write stamps are inconsistent.
 *
 *              scratch_p - [I]   ptr to a Scratch database area.
 *              eboard_p  - [I]   error board for marking different
 *                                              types of errors.
 *              sector_p  - [I]   vector of ptrs to sector contents.
 *              bitkey    - [I]   bitkey for stamps/error boards.
 *              width     - [I]   number of sectors.
 *              ppos      - [I]   index containing parity info.
 *
 *      @return VALUE:
 *              This function returns FBE_TRUE if the stamps are
 *              consistent, permitting the target to be rebuilt;
 *              otherwise FBE_FALSE is returned.
 *
 *      @notes
 *              If the target can be rebuilt, then the stamp fields
 *              of the target will be initialized prior to returning.
 *
 *      @author
 *              06-Nov-98       -SPN-   Redesigned to use checksum toolkit.
 ***************************************************************************/

static fbe_bool_t fbe_xor_rebuild_data_stamps(fbe_xor_scratch_t * scratch_p,
                                              fbe_xor_error_t * eboard_p,
                                              fbe_sector_t * sector_p[],
                                              fbe_u16_t bitkey[],
                                              fbe_u16_t width,
                                              fbe_u16_t ppos)
{
    fbe_u16_t   tstamp1;
    fbe_u16_t   tstamp2;
    fbe_u16_t   wstamp;
    fbe_u16_t   pos;
    fbe_bool_t  canRebuild;
    fbe_u16_t   pkey;
    fbe_u16_t   u_coh_bitmap;

    pkey = bitkey[ppos];
    u_coh_bitmap = (eboard_p->u_coh_bitmap & pkey);
    u_coh_bitmap = (b_xor_raid5_check_for_reconstructable_data == FBE_FALSE) ? u_coh_bitmap : 0;

    /* We are here because a DATA drive needs to be rebuilt
     * and all the other data drives and the parity drive
     * are OK. We need to see if there is any reason which
     * would prohibit reconstructing the bad data from the
     * other drives.
     */
    {
        fbe_sector_t *pblk_p;

        pblk_p = sector_p[ppos];

        /* Copy the write stamp and time
         * stamp from the parity drive.
         */
        wstamp = pblk_p->write_stamp,
        tstamp1 = pblk_p->time_stamp;
    }

    /* Check the ALL_TIMESTAMPS bit in the
     * timestamp from the parity sector; if
     * set, it indicates that all data sectors
     * should use timestamps.
     */
    if (0 != (tstamp1 & FBE_SECTOR_ALL_TSTAMPS))
    {
        /* All data drives are required to match the timestamp
         * on the parity drive (remember to mask off the 'full'
         * bit since that only shows up on the Parity drive).
         */
        tstamp1 &= ~FBE_SECTOR_ALL_TSTAMPS,
        tstamp2 = tstamp1;
    }
    else
    {
        /* Each data drive is allowed to have the timestamp
         * on the parity drive (which could be the initial
         * bind value if timestamps have never been used
         * since bind time), or the 'invalid time stamp'
         * value.
         */
        tstamp2 = FBE_SECTOR_INVALID_TSTAMP;
    }

    canRebuild = FBE_TRUE;

    /* Need to have a consistent set of
     * timestamps and write stamps.
     */
    for (pos = width; 0 < pos--;)
    {
        fbe_u16_t   dkey;
        fbe_bool_t  b_is_invalidated = FBE_FALSE;
        xorlib_sector_invalid_reason_t invalidation_reason = XORLIB_SECTOR_INVALID_REASON_UNKNOWN;


        dkey = bitkey[pos];

        /* For raid groups with a single parity we must handle the case where
         * a 2nd position has one or more of the following errors:
         *  o time_stamp
         *  o write_stamp
         *  o lba_stamp
         */
        if ((ppos != pos) &&
            (dkey != scratch_p->fatal_key))
        {
            fbe_sector_t *dblk_p;

            dblk_p = sector_p[pos];

            /* If this data sector has been invalidated, the time stamp
             * or write stamp may be invalid.  We do not need to flag this
             * since the stamps will be updated when we re-invalidate the sector.
             */
             b_is_invalidated = fbe_xor_is_sector_invalidated(dblk_p, &invalidation_reason);

            /* If the timestamp is incorrect on another data sector, take the
             * neccessary action.
             */
            if ((dblk_p->time_stamp != tstamp1) &&
                (dblk_p->time_stamp != tstamp2)    )
            {
                /* The timestamp in this data sector does not match either of
                 * the allowable values.  If the sector was invalidated, mark 
                 * the data sector in error.  Otherwise mark the parity in error. 
                 */
                if ((b_is_invalidated)  &&
                    (u_coh_bitmap == 0)    )
                {
                    eboard_p->u_ts_bitmap |= dkey;
                }
                else
                {
                    eboard_p->u_ts_bitmap |= pkey;
                }
                canRebuild = FBE_FALSE;
            }

            else if (dblk_p->write_stamp != (wstamp & dkey))
            {
                /* The writestamp in this data sector does not match either of
                 * the allowable values.  If the sector was invalidated, mark 
                 * the data sector in error.  Otherwise mark the parity in error. 
                 */
                if ((b_is_invalidated)  &&
                    (u_coh_bitmap == 0)    )
                {
                    eboard_p->u_ws_bitmap |= dkey;
                }
                else
                {
                    eboard_p->u_ws_bitmap |= pkey;
                }
                canRebuild = FBE_FALSE;
            }

            /* Set the lba stamps for all data positions.
             * (This is only for converting from a lba stamp of
             *  0 to the real lba stamp.)
             * The fatal position will be covered below.
             */
            if (dblk_p->lba_stamp == 0)
            {
                dblk_p->lba_stamp = xorlib_generate_lba_stamp(scratch_p->seed, eboard_p->raid_group_offset);
            }
            else
            {
                /* At this point another non 0 lba stamp should be
                 * legal in a non-fatal block.
                 */
                if (XOR_COND(!xorlib_is_valid_lba_stamp(&dblk_p->lba_stamp, scratch_p->seed, eboard_p->raid_group_offset)))
                {
                    fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                          "lba stamp is not legal: lba stamp = 0x%x, seed = 0x%llx\n",
                                          dblk_p->lba_stamp,
                                          (unsigned long long)scratch_p->seed);  

                    return FBE_STATUS_GENERIC_FAILURE;
                }
            }

        }
    }

    if (canRebuild)
    {
        fbe_sector_t *rblk_p;

        rblk_p = scratch_p->fatal_blk;

        /* Before we leave, let's set the stamp fields into place.
         */
        rblk_p->time_stamp = tstamp2;
        rblk_p->write_stamp = (wstamp & scratch_p->fatal_key);
        rblk_p->lba_stamp = xorlib_generate_lba_stamp(scratch_p->seed, eboard_p->raid_group_offset);
    }

    return canRebuild;
}
/* fbe_xor_rebuild_data_stamps() */

/***************************************************************************
 *              fbe_xor_rebuild_parity_stamps()
 ***************************************************************************
 * @brief
 *              This routine is used for Raid5 (or old Raid3) LUs to
 *              check and correct inconsistencies among stamp values
 *              in an otherwise valid stripe. This maximizes the
 *              probability that we will be able to reconstruct the
 *              contents of lost sectors in the future.
 *
 * @param   scratch_p - Pointer to parity `scratch' structure used for rebuild
 * @param   eboard_p  - Pointer to error board for marking different types of errors.
 * @param   sector_p - vector of ptrs to sector contents.
 * @param   bitkey - bitkey for stamps/error boards.
 * @param   width  - width of raid group
 * @param   ppos - index containing parity info.
 * @param   seed - seed for lba stamping.
 * @param   b_check_parity - If FBE_TRUE, check and correct the parity from the set
 *                          of data stamps; otherwise, rebuild the parity stamps
 *                          entirely from from the data stamps.
 * @param   pblk_p - Parity block
 *
 * @return None.
 *
 * @note
 *
 * @author
 *  06-Nov-98       -SPN-   Redesigned to use checksum toolkit.
 ***************************************************************************/

static void fbe_xor_rebuild_parity_stamps(fbe_xor_scratch_t * scratch_p,
                                          fbe_xor_error_t* eboard_p,
                                          fbe_sector_t* sector_p[],
                                          fbe_u16_t bitkey[],
                                          fbe_u16_t width,
                                          fbe_u16_t ppos,
                                          fbe_lba_t seed,
                                          fbe_bool_t b_check_parity,
                                          fbe_sector_t * pblk_p)
{
    fbe_u16_t   pos;
    fbe_u16_t   valid_timestamp_count = 0;
    fbe_u16_t   tstamp = FBE_SECTOR_INITIAL_TSTAMP;
    fbe_u16_t   ptstamp = FBE_SECTOR_INVALID_TSTAMP;
    fbe_u16_t   wstamp = 0;
    fbe_u16_t   pwstamp = 0;
    fbe_bool_t  b_timestamp_valid = FBE_FALSE;
    fbe_bool_t  b_writestamp_valid = FBE_FALSE;
    fbe_bool_t  b_lbastamp_valid = FBE_TRUE;
    fbe_u16_t   expected_tstamp = FBE_SECTOR_INITIAL_TSTAMP;
    fbe_u16_t   expected_wstamp = 0;
    fbe_u16_t   expected_lbastamp = xorlib_generate_lba_stamp(seed, eboard_p->raid_group_offset);
    fbe_sector_t *dblk_p;
    fbe_u16_t   dkey;

    /* When a data position write stamp disagrees with another, use parity to
     * determine if parity or the data position is wrong.
     */
    if (pblk_p != NULL)
    {
        pblk_p = sector_p[ppos];

        ptstamp = pblk_p->time_stamp;
        pwstamp = pblk_p->write_stamp;
    }

    /* No need to check shed stamps, we wouldn't be
     * here if any were set. Instead, perform a pass
     * through the data looking for real timestamps.
     */
    for (pos = width; 0 < pos--;)
    {
        if (pos != ppos)
        {

            dblk_p = sector_p[pos];

            if ((FBE_SECTOR_INVALID_TSTAMP != dblk_p->time_stamp) &&
                (FBE_SECTOR_INITIAL_TSTAMP != dblk_p->time_stamp))
            {
                /* Found a timestamp value.
                 */
                tstamp = dblk_p->time_stamp;
                break;
            }
        }
    }

    /* Now let's do it for real; pass through the stamps again
     * adjusting timestamps and writestamps as necessary.
     */
    for (pos = width; 0 < pos--;)
    {
        if (pos != ppos)
        {
            xorlib_sector_invalid_reason_t invalidated_reason = XORLIB_SECTOR_INVALID_REASON_UNKNOWN;

            /* Get the sector pointer
             */
            dblk_p = sector_p[pos];
            dkey = bitkey[pos];

            /* Generate the expected time stamp.
             */
            b_timestamp_valid = (ptstamp & FBE_SECTOR_ALL_TSTAMPS) ? FBE_TRUE : FBE_FALSE;
            expected_tstamp = (ptstamp & FBE_SECTOR_ALL_TSTAMPS) ? (ptstamp & ~FBE_SECTOR_ALL_TSTAMPS) : FBE_SECTOR_INITIAL_TSTAMP;

            /* Generate the expected write stamp.
             */
            b_writestamp_valid = (ptstamp & FBE_SECTOR_ALL_TSTAMPS) ? FBE_FALSE : FBE_TRUE;
            expected_wstamp = pwstamp & dkey;

            /* First handle purposefully invalidated sectors.
             */
            if ((b_xor_raid5_check_for_reconstructable_data == FBE_TRUE)   &&
                (b_check_parity == FBE_TRUE)                               &&
                (eboard_p->u_crc_bitmap & dkey)                            &&
                fbe_xor_is_sector_invalidated(dblk_p, &invalidated_reason)    )
            {
                /* Determine if the sector was properly invalidted or not.
                 */
                fbe_xor_eval_invalidated_data_sector(scratch_p,
                                                     eboard_p,
                                                     dblk_p,
                                                     dkey,
                                                     b_timestamp_valid,
                                                     b_writestamp_valid,
                                                     b_lbastamp_valid,
                                                     expected_tstamp,
                                                     expected_wstamp,
                                                     expected_lbastamp);
            }

            /* Now check the stamps.
             */
            if (dblk_p->write_stamp != 0)
            {
                /* Validate the write stamp.
                 */
                if ((tstamp != FBE_SECTOR_INVALID_TSTAMP)           &&
                    (ptstamp & FBE_SECTOR_ALL_TSTAMPS)              &&
                    (tstamp == (ptstamp & ~FBE_SECTOR_ALL_TSTAMPS))    )
                {
                    /* Only report a correctable write_stamp error if parity was
                     * read from disk (i.e. b_check_parity is True).
                     */

                    if (b_check_parity == FBE_TRUE)
                    {
                        /* If XOR debug is enabled trace an informational message.
                         * Flag this as a correctable write_stamp error.
                         */
                        if (scratch_p->option & FBE_XOR_OPTION_DEBUG)
                        {
                            fbe_base_library_trace(FBE_LIBRARY_ID_XOR, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "xor: rg: 0x%x pos: %d lba: 0x%llx correctable write_stamp: 0x%x time_stamp: 0x%x lba_stamp: 0x%x\n",
                                fbe_xor_error_get_raid_group_id(eboard_p), pos,
				(unsigned long long)seed,
                                dblk_p->write_stamp, dblk_p->time_stamp, dblk_p->lba_stamp);
                        }
                        eboard_p->c_ws_bitmap |= bitkey[pos];
                    }
                    else
                    {
                        /* Else we need to set the modified bit for this position
                         * so that the updated sector gets written.
                         */
                        eboard_p->m_bitmap |= bitkey[pos];
                    }

                    /* The write stamp is incorrect.  Update it and the time stamp.
                     */
                    dblk_p->write_stamp = 0;

                    /* Set the time and lba stamps, since this position will
                     * now be written.
                     */
                    dblk_p->time_stamp = tstamp;
                    valid_timestamp_count++;
                    dblk_p->lba_stamp = expected_lbastamp;

                }
                else
                {
                    /* Else write stamp is correct. Or it into the cumulative
                     * write stamp value.
                     */
                    wstamp |= dblk_p->write_stamp;
                }
            }

            else if (dblk_p->time_stamp == FBE_SECTOR_INVALID_TSTAMP)
            {
                /* No timestamp. Writestamp just happened to be zero.
                 */
            }

            else if (valid_timestamp_count++, tstamp != dblk_p->time_stamp)
            {
                /* We have mis-matched timestamp values in this
                 * stripe. This can occur when an MR3 is only
                 * partially written, the result of an unexpected
                 * shutdown condition. Replace the timestamp for
                 * this sector with the one selected. Mark this
                 * sector as having had its timestamp corrected.
                 */
                dblk_p->time_stamp = tstamp,
                eboard_p->c_ts_bitmap |= bitkey[pos];

                /* Set the lba stamp, since this position will
                 * now be written.
                 */
                dblk_p->lba_stamp = expected_lbastamp;
            }

        }
    }

    /* Now check to make sure the stamps on the
     * parity sector agree with the stamps on
     * the data sectors.
     */
    if (pblk_p != NULL)
    {
        pblk_p = sector_p[ppos];

        if (tstamp == FBE_SECTOR_INITIAL_TSTAMP)
        {
            if ((valid_timestamp_count < 1) && b_check_parity)
            {
                /* We found no 'real' timestamps and no occurances
                 * of the initial binding value. The timestamp can
                 * be anything. Take the value directly from the
                 * parity, stripping off the ALL_TIMESTAMPS flag.
                 * That way we can keep from triggering needless
                 * 'corrected timestamp' errors.
                 */
                tstamp = pblk_p->time_stamp & ~FBE_SECTOR_ALL_TSTAMPS;
            }
        }

        else if ((valid_timestamp_count + 1) >= width)
        {
            /* We need to set the ALL_TIMESTAMPS bit.
             */
            tstamp |= FBE_SECTOR_ALL_TSTAMPS;
        }

        /* if we were told to check the parity stamps do so.
         */
        if (b_check_parity)
        {
            if (pblk_p->time_stamp != tstamp)
            {
                /* The timestamp on the parity is not
                 * what it should be. Fix it and mark
                 * it as corrected.
                 */
                pblk_p->time_stamp = tstamp,
                eboard_p->c_ts_bitmap |= bitkey[ppos];

            }

            if (pblk_p->write_stamp != wstamp)
            {
                /* The writestamp on the parity does not
                 * match what is on the data drives. Fix
                 * it and mark it as corrected.
                 */
                pblk_p->write_stamp = wstamp,
                eboard_p->c_ws_bitmap |= bitkey[ppos];
            }

        }

        else
        {
            /* Set the stamps in their proper places.
             */
            pblk_p->lba_stamp = 0,
            pblk_p->write_stamp = wstamp,
            pblk_p->time_stamp = tstamp;
        }
    }

    return;
}
/* fbe_xor_rebuild_parity_stamps() */
/*****************************************
 * End of fbe_xor_raid5_eval_parity.c
 *****************************************/
