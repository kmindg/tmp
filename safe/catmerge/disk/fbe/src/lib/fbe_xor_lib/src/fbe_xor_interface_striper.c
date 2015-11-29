/***************************************************************************
 * Copyright (C) EMC Corporation 1989-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_xor_interface_striper.c
 ***************************************************************************
 *
 * @brief
 *       This file contains functions used by proc0 to execute XOR commands.
 *   Proc0 will execute XOR xommands based on Flare's xor library
 *   functions.  The commands are executed in-line. The operation and any
 *   necessary error recovery are all completed before the function returns
 *   to the calling driver. This component provides synchronous behavior
 *   and is assumed to provide the best performace in a single processor
 *   architecture.
 *
 * @notes
 *
 * @author
 *   02/25/2000 Created.  Bruss.
 *
 ***************************************************************************/

/************************
 * INCLUDE FILES
 ************************/
#include "fbe_xor_private.h"
#include "fbe_xor_epu_tracing.h"

/*****************************************************
 * static FUNCTIONS DECLARED GLOBALLY FOR VISIBILITY
 *****************************************************/

static fbe_status_t fbe_xor_eval_raw_unit(fbe_xor_error_t * eboard_p,
                                          fbe_sector_t * sector_p[],
                                          fbe_u16_t bitkey[],
                                          fbe_u16_t width,
                                          fbe_lba_t seed,
                                          fbe_u16_t *u_bitmap_p);

/*************************************************************** 
 * fbe_xor_raid0_verify 
 ****************************************************************
 *
 * @brief
 *  This function executes a raid 0 verify operation.
 *
 * @param xor_verify_p - Structure with all info we need.
 *
 * @return fbe_status_t
 *         FBE_STATUS_OK on success
 *         FBE_STATUS_GENERIC_FAILURE on error.  
 *
 * @author
 *  5/15/2000:  MPG -- Created.
 *
 ****************************************************************/
fbe_status_t fbe_xor_raid0_verify(fbe_xor_striper_verify_t *xor_verify_p)
{
    fbe_block_count_t count = xor_verify_p->count;
    fbe_lba_t seed = xor_verify_p->seed;
    fbe_sector_t *sector_v[FBE_XOR_MAX_FRUS];    /* Sector ptr for current position */
    fbe_xor_sgl_descriptor_t sgd_v[FBE_XOR_MAX_FRUS];

    fbe_u32_t width = xor_verify_p->array_width;
    fbe_u16_t array_pos;
    fbe_block_count_t block;
    fbe_u32_t i;
    fbe_status_t status;
    fbe_u16_t u_bitmap;
    fbe_u32_t sg_count;

    for (i = 0; i < width; i++)
    {
        fbe_u32_t bytcnt;

        bytcnt = FBE_XOR_SGLD_INIT_BE(sgd_v[i], xor_verify_p->fru[i].sg_p);

        if (XOR_COND(bytcnt == 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "bytcnt == 0\n");

            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (XOR_COND(FBE_XOR_SGLD_ADDR(sgd_v[i]) == NULL))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "FBE_XOR_SGLD_ADDR(sgd_v[%d]) == NULL\n",
                                  i);

            return FBE_STATUS_GENERIC_FAILURE;
        }


        sector_v[i] = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(sgd_v[i]);
    }

    fbe_xor_setup_eboard(xor_verify_p->eboard_p, 0);

    /* Verify that the Calling driver has work for the XOR driver, otherwise
       this may be an unnecessary call into the XOR driver and will effect
       system performance.
     */

    if (XOR_COND(count == 0))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "count == 0\n");

        return FBE_STATUS_GENERIC_FAILURE;
    }


    for (block = 0;
        block < count;
        block++)
    {
        fbe_xor_error_t EBoard_local;

        /* Initialize the error board.
         * Set the u_crc bit for any hard errored drives.
         */
        fbe_xor_init_eboard((fbe_xor_error_t *)&EBoard_local,
                            xor_verify_p->eboard_p->raid_group_object_id,
                            xor_verify_p->eboard_p->raid_group_offset);
        fbe_xor_setup_eboard(((fbe_xor_error_t *) & EBoard_local),
                            xor_verify_p->eboard_p->hard_media_err_bitmap); 

        /* Do the stripwise verify.
         */
        status = fbe_xor_eval_raw_unit(&EBoard_local,
                                       sector_v,
                                       xor_verify_p->position_mask,
                                       width,
                                       seed,
                                       &u_bitmap);
        if (status != FBE_STATUS_OK)
        {
            return status; 
        }
        EBoard_local.u_bitmap = u_bitmap;


        /* If the user passed in an error region struct, then fill it out.
         */
        status = fbe_xor_save_error_region( &EBoard_local, 
                                            xor_verify_p->eregions_p,
                                            FBE_TRUE, /* Invalidate on crc error. */ 
                                            seed,
                                            0, /* parity drives*/
                                            width,
                                            FBE_XOR_INV_DRV_POS);
        if (status != FBE_STATUS_OK)
        {
            return status; 
        }

        /* Setup the sector vector for the next strip.
         */
        for (array_pos = 0; array_pos < width; array_pos++)
        {
            if (XOR_COND(FBE_XOR_SGLD_ADDR(sgd_v[array_pos]) == NULL))
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "FBE_XOR_SGLD_ADDR(sgd_v[%d]) == NULL\n",
                                      array_pos);

                return FBE_STATUS_GENERIC_FAILURE;
            }

            status = FBE_XOR_SGLD_INC_BE(sgd_v[array_pos], sg_count);
            if (status != FBE_STATUS_OK)
            {
                return status; 
            }

            sector_v[array_pos] = (fbe_sector_t *) FBE_XOR_SGLD_ADDR(sgd_v[array_pos]);
        }

        /* Copy the error information,
         * so we know which frus to write when we finish.
         */
        FBE_XOR_INIT_EBOARD_WMASK(((fbe_xor_error_t *) & EBoard_local));

        /* Update the global eboard structure */
        fbe_xor_update_global_eboard(xor_verify_p->eboard_p, ((fbe_xor_error_t *) & EBoard_local));

        if (xor_verify_p->error_counts_p != NULL)
        {
            fbe_xor_vr_increment_errcnt(xor_verify_p->error_counts_p, &EBoard_local, NULL, 0);
        }

        /* Increment the seed for the next strip.
         */
        seed++;
    }

    return FBE_STATUS_OK;

}  /* fbe_xor_raid0_verify() */
/*!**************************************************************
 * fbe_xor_validate_raid0_stamps()
 ****************************************************************
 * @brief
 *  Determine if the stamps are ok for a raid 0.
 *
 * @param sector_p - the block we will be validating.
 * @param lba - the disk lba of this block. 
 * @param offset - The raid group offset to use to check lba stamp              
 *
 * @return fbe_status_t
 *         FBE_STATUS_OK on success
 *         FBE_STATUS_GENERIC_FAILURE on error.  
 *
 * @author
 *  5/11/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_xor_validate_raid0_stamps(fbe_sector_t *sector_p, fbe_lba_t lba, fbe_lba_t offset)
{
    const fbe_u16_t crc = xorlib_cook_csum(xorlib_calc_csum(sector_p->data_word), 0);

    /* Only do the checks if we are not an invalid block.
     */
    if (crc == sector_p->crc)
    {
        /* Only check the lba stamp since this is the only stamp that should always be 
         * valid. 
         */
        if (XOR_COND(!fbe_xor_lib_is_lba_stamp_valid(sector_p, lba, offset)))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "raid 0 lba stamp is invalid!\n");
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "lba_stamp = 0x%x, lba: 0x%llx\n", 
                                  sector_p->lba_stamp,
                                  (unsigned long long)lba);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_xor_validate_raid0_stamps()
 ******************************************/

/****************************************************************************
 *              fbe_xor_eval_raw_unit()
 ****************************************************************************
 *      @brief
 *              This routine authenticates checksums of corresponding
 *              sectors in a Raid0 or Single Disk LUs.  It will also
 *              invalidate sectors found to be incorrectable.
 *
 *              eboard_p  -          [I]   error board for marking different
 *                                         types of errors.
 *              sector_p  -          [I]   vector of ptrs to sector contents.
 *              bitkey    -          [I]   bitkey for stamps/error boards.
 *              width     -          [I]   number of sectors.
 *              invalid_bitmask_p  - [O]   bitmask indicating which sectors have been invalidated.
 *                                         Always zero for these unit types. (Since there is no
 *                                         redundancy, there's no sense invalidating...)
 *
 *      @return fbe_status_t
 *
 *      @notes
 *
 *      @author
 *              06-Nov-98       -SPN-   Redesigned to use checksum toolkit.
 *              17-Feb-00       -kls-   Invalidate the invalid bitmap.
 ***************************************************************************/

fbe_status_t fbe_xor_eval_raw_unit(fbe_xor_error_t * eboard_p,
                                   fbe_sector_t * sector_p[],
                                   fbe_u16_t bitkey[],
                                   fbe_u16_t width,
                                   fbe_lba_t seed,
                                   fbe_u16_t *invalid_bitmask_p)
{
    fbe_u16_t media_err_bitmap = 0;
    fbe_u16_t invalid_bitmap = 0;
    fbe_u16_t cooked_csum = 0;
    fbe_status_t status;

    /* Determine media error bitmap.
     * If there is a media errd position, then
     * we clear its bit from the mask below in
     * fbe_xor_determine_media_errors(),
     * and that function requires this value.
     *
     * Note that all uncorrectables at this point
     * are due to media errors.
     */
    media_err_bitmap = eboard_p->u_crc_bitmap;

    {
        fbe_u16_t pos;

        for (pos = width; 0 < pos--;)
        {
            fbe_u16_t dkey = bitkey[pos];

            /* If we have read the sector, then proceed to check its checksum.
             */
            if (0 == (eboard_p->u_crc_bitmap & dkey))
            {
                fbe_sector_t *dblk_p;
                fbe_u32_t rsum;

                dblk_p = sector_p[pos];
                rsum = xorlib_calc_csum(dblk_p->data_word);

                /* We always report an error for requests that encounter crc errors.
                 */
                cooked_csum = xorlib_cook_csum(rsum, XORLIB_SEED_UNREFERENCED_VALUE);
                if (dblk_p->crc != cooked_csum)
                {
                    /* The checksum we've calculated doesn't
                     * match the checksum attached to the data.
                     * Mark this as an uncorrectable crc error.
                     */
                    eboard_p->u_crc_bitmap |= dkey;

                    /* Determine the types of errors.
                     */
                    status = fbe_xor_determine_csum_error( eboard_p, dblk_p, dkey, seed,
                                                           dblk_p->crc, cooked_csum, FBE_TRUE);
                    if (status != FBE_STATUS_OK) 
                    {
                        return status; 
                    }
                }
                else if (!xorlib_is_valid_lba_stamp(&dblk_p->lba_stamp, seed, eboard_p->raid_group_offset))
                {
                    /* The lba stamp is incorrect.
                     * Handle the same as a checksum error.
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
            }
        }

        /* The invalid bitmap is the uncorrectable crc bitmap
         * since this is the only error in the board we can get.
         */
        invalid_bitmap = eboard_p->u_crc_bitmap;
    }

    /* Invalidate the sector
     */
    if (0 != invalid_bitmap)
    {
        fbe_xor_invalidate_sectors(sector_p,
                                   bitkey,
                                   width,
                                   seed,
                                   invalid_bitmap,
                                   eboard_p);
    }

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
/* end fbe_xor_eval_raw_unit() */

/*****************************************
 * End of fbe_xor_interface_striper.c
 *****************************************/
