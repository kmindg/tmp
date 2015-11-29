/***************************************************************************
 * Copyright (C) EMC Corporation 1989-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_xor_mirror.c
 ***************************************************************************
 *
 * @brief
 *  This file contains mirror functions of the xor library.
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
 * static FUNCTIONS
 *****************************************************/

static fbe_status_t fbe_xor_equate_mirror(fbe_xor_error_t * eboard_p,
                                          fbe_sector_t * pblk,
                                          fbe_u16_t pkey,
                                          fbe_sector_t * sblk,
                                          fbe_u16_t skey,
                                          fbe_lba_t seed,
                                          fbe_xor_option_t option);
static fbe_status_t fbe_xor_rebuild_mirror(fbe_xor_error_t * eboard_p,
                                           fbe_sector_t * blk,
                                           fbe_lba_t seed,
                                           fbe_u16_t key,
                                           fbe_u16_t rb_key,
                                           fbe_xor_option_t option);

static fbe_status_t fbe_xor_equate_mirror_primary_valid( fbe_xor_error_t * eboard_p,
                                                         fbe_sector_t * pblk,
                                                         fbe_u16_t pkey,
                                                         fbe_sector_t * sblk,
                                                         fbe_u16_t skey,
                                                         fbe_lba_t seed,
                                                         fbe_xor_option_t option );

static void fbe_xor_raw_mirror_sector_validate(fbe_xor_error_t * eboard_p,
                                               fbe_sector_t * sector_p[],
                                               fbe_u16_t bitkey[],
                                               fbe_u16_t width,
                                               fbe_u16_t rm_positions_to_check_bitmap,
                                               fbe_u32_t *primary);

/*!***************************************************************************
 *          fbe_xor_verify_r1_unit()
 *****************************************************************************
 *
 * @brief   This routine authenticates checksums and coherency for all positions
 *          of a mirror raid group.  The (4) steps taken are:
 *          1. Walk thru valid positions (excluding those positions in error) and
 *             validate the checksum.  First position that has a valid checksum
 *             and LBA stamp is selected as the primary.
 *          2. If no primary position is located, invalidate all uncorrectable
 *             and `need rebuild' positions.
 *          3. If a primary position is located compare the data against other
 *             valid positions.  Any positions with different data will be flagged
 *             with a coherency error.
 *          4. Use the primary position to `rebuild' any positions in error (this
 *             includes known degraded positions, hard error positions and 
 *             positions with coherency errors).
 *
 *
 * @param   eboard_p - [I]   error board for marking different types of errors.
 * @param   sector_p - [I]   vector of ptrs to sector contents.
 * @param   bitkey   - [I]   bitkey for stamps/error boards.
 * @param   width    - [I]   width of the raid group
 * @param   valid_bitmap - [I] Bitmap of positions read (also includes those
 *                             that detected `hard error')
 * @param   needs_rebuild_bitmap - [I] Bitmap of positions that need to be rebuilt
 *                           (this does not include any read positions that detected
 *                            a hard error)
 * @param   seed               - [I]   seed value for checksum.
 * @param   option             - [I]   only supports FBE_XOR_OPTION_CHK_LBA_STAMPS
 * @param   invalid_bitmask_p  - [O]   bitmask indicating which sectors have been invalidated.
 * @param   raw_mirror_io_b - used to differentiate between object I/O and raw mirror I/O.
 *
 * @return  fbe_status_t
 *
 * @notes   Currently there can only be (1) position to be rebuilt.
 *
 * @todo    Since the XOR library supports 3-way (and possibly 4-way in the future)
 *          mirrors, the rpos should actually be a mask of positions known
 *          to be invalid that need to be rebuilt.
 *
 * @author
 *  06-Nov-98       -SPN-   Redesigned to use checksum toolkit.
 *  16-Aug-00       -RG-    Created from rg_eval_mirror_unit().
 *  19-Sep-00       -Rob Foley-   Added rebuild support.
 *  25-Jun-01       -BJP-   Renamed to fbe_xor_verify_r1_unit and moved all 
 *                          rebuild functionality to fbe_xor_rebuild_r1_unit
 *
 *  04/02/2010  Ron Proulx - Fixed support for rpos != FBE_XOR_INV_DRV_POS
 *
 *****************************************************************************/
 fbe_status_t fbe_xor_verify_r1_unit(fbe_xor_error_t * eboard_p,
                                     fbe_sector_t * sector_p[],
                                     fbe_u16_t bitkey[],
                                     fbe_u16_t width,
                                     fbe_u16_t valid_bitmap,
                                     fbe_u16_t needs_rebuild_bitmap,
                                     fbe_lba_t seed,
                                     fbe_xor_option_t option,
                                     fbe_u16_t *invalid_bitmask_p,
                                     fbe_bool_t raw_mirror_io_b)
{
    fbe_u16_t   invalid_bitmap = 0;          /* Bitmap of positions that have been invalidated */
    fbe_u16_t   media_err_bitmap;            /* Bitmap of errors induced by media errors. */
    fbe_u16_t   positions_to_correct_bitmap; /* Bitmap of positions that need to be corrected. */
    fbe_u16_t   positions_to_check_bitmap;   /* Bitmap of positions that need to be checked */
    fbe_u32_t   primary, secondary;
    fbe_u16_t   pos;
    fbe_status_t status;
    
    /* Only `check lba stamps' and `debug' are supported
     */
    if (XOR_COND((option & ~(FBE_XOR_OPTION_CHK_LBA_STAMPS | FBE_XOR_OPTION_DEBUG | FBE_XOR_OPTION_DEBUG_ERROR_CHECKSUM)) != 0))
    {
        /* Unsupported options.
         */
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "Unsupported option: 0x%x \n",
                              option);
        return FBE_STATUS_GENERIC_FAILURE;
    }

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
    
    /* Initialize the `positions_to_correct' bitmap to the value of the 
     * `needs rebuild' and set the primary to invalid since the primary isn't 
     * known.
     */
    positions_to_correct_bitmap = needs_rebuild_bitmap;
    primary = FBE_XOR_INV_DRV_POS;
    
    /* Initialize the positons to check to the valid bitmap.
     */
    positions_to_check_bitmap = valid_bitmap;

    /* Step 1: Walk thru valid positions (excluding those positions in error)
     *         and validate the checksum.  First position that has a valid 
     *         checksum and LBA stamp is selected as the primary.
     */
    positions_to_check_bitmap &= ~media_err_bitmap;
    for (pos = 0; pos < width; pos++)
    {
        fbe_u16_t dkey = bitkey[pos];
        
       /* If we have successfully read the sector proceed to check 
        * the checksum.  The first position with a valid checksum is
        * marked as the `primary' position.
        */
        if (dkey & positions_to_check_bitmap) 
        {
            fbe_sector_t *dblk_p;
            fbe_u32_t rsum;
            fbe_u16_t cooked_csum = 0;
            
            dblk_p = sector_p[pos];
            rsum = xorlib_calc_csum(dblk_p->data_word);
           
            /* If the checksum read doesn't match the calculated checksum
             * we should report the error. 
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
                status = fbe_xor_determine_csum_error(eboard_p, dblk_p, dkey, seed,
                                             dblk_p->crc, cooked_csum, FBE_TRUE);
                if (status != FBE_STATUS_OK) 
                { 
                    return status; 
                }
            }
            else if ((option & FBE_XOR_OPTION_CHK_LBA_STAMPS) && 
                     !xorlib_is_valid_lba_stamp(&dblk_p->lba_stamp, seed, eboard_p->raid_group_offset))
            {
                /* The lba stamps doesn't mach, treat this
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
            else if (primary == FBE_XOR_INV_DRV_POS)
            {
                primary = pos;
            }
        
        } /* end if this is a valid position */
    
    } /* end for all mirror positions */
    
    /* Step 2: For raw mirror I/O, walk through valid positions (excluding those positions in error)
     *         and validate the raw mirror data metadata (magic number and sequence number).
     */
    if (raw_mirror_io_b && (primary != FBE_XOR_INV_DRV_POS) )
    {
        fbe_u16_t rm_positions_to_check_bitmap = positions_to_check_bitmap & ~eboard_p->u_crc_bitmap;

        fbe_xor_raw_mirror_sector_validate(eboard_p,
                                           sector_p,
                                           bitkey,
                                           width,
                                           rm_positions_to_check_bitmap,
                                           &primary);
    }

    /* Step 3: If no primary position is located, invalidate all uncorrectable
     *         and `needs rebuild' positions.
     */
    if (primary == FBE_XOR_INV_DRV_POS)
    {
        /* The invalid bitmap consist of those positions that have not been
         * corrected and those positions that are uncorrectable.  In theory
         * this should be all array positions.
         */
        invalid_bitmap = (positions_to_correct_bitmap | eboard_p->u_crc_bitmap | eboard_p->u_rm_magic_bitmap);

        /* Were any checksum errors caused by media errors?
         *
         * This is information we need to setup in the eboard,
         * and this is the logical place to determine
         * which errors were the result of media errors.
         */
        status = fbe_xor_determine_media_errors(eboard_p, media_err_bitmap);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
        
       /* No valid primary was found, invalidate data
        */
        if (XOR_COND(invalid_bitmap == 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "invalid_bitmap == 0\n");  

            return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_xor_invalidate_sectors(sector_p,
                                   bitkey,
                                   width,
                                   seed,
                                   invalid_bitmap,
                                   eboard_p);
        *invalid_bitmask_p = invalid_bitmap;
        return FBE_STATUS_OK;
    }
    
    /* By this time we have a valid primary from which to rebuild data
     * so there will be no uncorrectable errors.  Thus we also clear 
     * the uncorrectable bitmap since that is the return which will be 
     * used to to mark uncorrectable positions in the eboard.  Since we 
     * have a valid primary position there are no uncorrectable positions.
     */
    positions_to_correct_bitmap |= eboard_p->u_crc_bitmap;
    positions_to_check_bitmap &= ~eboard_p->u_crc_bitmap;
    eboard_p->c_crc_bitmap |= eboard_p->u_crc_bitmap;
    eboard_p->u_crc_bitmap = 0;
    
    /* Step 4: If a primary position is located compare the data against other
     *         valid positions.  Any positions with different data will be 
     *         flagged with a coherency error.
     */
    {
        fbe_u16_t       loop_count = 0;
        fbe_u16_t       pkey, skey;
        fbe_sector_t   *pblk, *sblk;
        
        /* Get the primary information.
         */
        pkey = bitkey[primary];
        pblk = sector_p[primary];
        
        /* Generate a bitmask of all read positions excluding the primary and
         * any postions with bad checksums.
         */
        positions_to_check_bitmap &= ~pkey;

        /* We clear a position bit for each pass.  Therefore
         * loop until there aren't any bits remaining.
         */
        while ((positions_to_check_bitmap != 0)    && 
               (loop_count++ < width) /* Sanity */    )
        {
            /* Set the secondary to the first non-zero bit position.
             */
            secondary = fbe_xor_get_first_position_from_bitmask(positions_to_check_bitmap,
                                                                width);
            if (XOR_COND(secondary >= width))
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "secondary 0x%x >= width 0x%x\n",
                                      secondary,
                                      width);  

                return FBE_STATUS_GENERIC_FAILURE;
            }

            if (XOR_COND(primary == secondary))
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "primary 0x%x == secondary 0x%x\n",
                                      primary,
                                      secondary);  

                return FBE_STATUS_GENERIC_FAILURE;
            }

            skey = bitkey[secondary];
            sblk = sector_p[secondary];
 
            /* Don't check sectors that have already been marked
             * as having a bad checksum, they will be corrected below.
             */
            if (!(bitkey[secondary] & eboard_p->c_crc_bitmap))
            {
                /* Look for coherency errors
                 */
                status = fbe_xor_equate_mirror(eboard_p,
                                      pblk,
                                      pkey,
                                      sblk,
                                      skey,
                                      seed,
                                      option);
                if (status != FBE_STATUS_OK) 
                { 
                    return status; 
                }
            }                
                
            /* Clear the `positions_to_check_bitmap' for this position.
             */
            positions_to_check_bitmap &= ~skey;
            
        } /* end while there are more positions to check */
    
    } /* end check for coherency errors */

    /* Step 5: Use the primary position to `rebuild' any positions in error 
     *         (this includes known degraded positions, hard error positions 
     *          and positions with coherency errors and raw mirror errors if applicable).
     */
    positions_to_correct_bitmap |= (eboard_p->c_crc_bitmap | eboard_p->c_coh_bitmap); 
    positions_to_correct_bitmap |= (eboard_p->c_rm_magic_bitmap | eboard_p->c_rm_seq_bitmap); 
    
    /*! If one or more positions read had a crc or coherency error or
     *  they was a known position that needs to be rebuilt, walk thru
     *  and use the primary position to rebuild them.
     *
     *  @note   Even if the primary was found to be bad it was at least
     *          marked uncorrectable above.  Thus the assumption is that
     *          all positions will be marked uncorrectable.
     *  @todo   Need to validate previous statement.
     */
    if (positions_to_correct_bitmap != 0)
    {
        fbe_u16_t   loop_count = 0;
        fbe_u16_t   positions_to_rebuild;
        fbe_u32_t   position_to_rebuild;
        fbe_u16_t   rkey;
        fbe_u16_t   rb_invalid_bitmap;

        /* We clear a position bit for each pass.  Therefore
         * loop until there aren't any bits remaining.
         */
        positions_to_rebuild = positions_to_correct_bitmap;
        while ((positions_to_rebuild != 0)         && 
               (loop_count++ < width) /* Sanity */    )
        {
            /* Set the secondary to the first non-zero bit position.
             */
            position_to_rebuild = fbe_xor_get_first_position_from_bitmask(positions_to_rebuild,
                                                                          width);

            if (XOR_COND(position_to_rebuild >= width))
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "position_to_rebuild 0x%x >= width 0x%x\n",
                                      position_to_rebuild,
                                      width);  

                return FBE_STATUS_GENERIC_FAILURE;
            }

            if (XOR_COND(position_to_rebuild == primary))
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "position_to_rebuild 0x%x == primary 0x%x\n",
                                      position_to_rebuild,
                                      primary);  

                return FBE_STATUS_GENERIC_FAILURE;
            }

            rkey = bitkey[position_to_rebuild];

            /* Rebuild any correctable errors. 
             */
            status = fbe_xor_rebuild_r1_unit(eboard_p,
                                      sector_p,
                                      bitkey,
                                      width,
                                      FBE_TRUE,
                                      primary,
                                      position_to_rebuild,
                                      seed,
                                      option,
                                      &rb_invalid_bitmap,
                                      raw_mirror_io_b);
            invalid_bitmap |= rb_invalid_bitmap;
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }
                        
            /* Clear the `positions_to_rebuild' for this position.
             */
            positions_to_rebuild &= ~rkey;
        
        } /* end while there are more positions to rebuild */
    
    } /* end if there are positions to rebuild */

    /* Were any checksum errors caused by media errors?
     *
     * This is information we need to setup in the eboard,
     * and this is the logical place to determine
     * which errors were the result of media errors.
     */
    status = fbe_xor_determine_media_errors(eboard_p, media_err_bitmap);
    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }
    
    /* Return the bitmap of those positions that were invalidated by
     * rebuild.
     */
    *invalid_bitmask_p = invalid_bitmap;
    return FBE_STATUS_OK;
}
/* end of fbe_xor_verify_r1_unit() */ 

/*!***************************************************************************
 *          fbe_xor_validate_mirror_verify()
 *****************************************************************************
 *
 * @brief   Validate that the mirror verify request is properly formed.
 *
 * @param   verify_p - Pointer to verify request to validate
 *
 * @return  status - If verify request is valid - FBE_STATUS_OK
 *                   Otherwise - Failure status
 *
 * @author
 *  01/05/2009  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_xor_validate_mirror_verify(fbe_xor_mirror_reconstruct_t *verify_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       valid_positions = 0;
    fbe_u32_t       needs_rebuild_positions = 0;
    fbe_u16_t       position;

    /* Only `check lba stamps' and `debug' are supported
     */
    if (XOR_COND((verify_p->options & ~(FBE_XOR_OPTION_CHK_LBA_STAMPS | FBE_XOR_OPTION_DEBUG | FBE_XOR_OPTION_DEBUG_ERROR_CHECKSUM)) != 0))
    {
        /* Unsupported options.
         */
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "Unsupported options: 0x%x \n",
                              verify_p->options);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Verify that the Calling driver has work for the XOR driver, otherwise
     * this may be an unnecessary call into the XOR driver and will effect
     * system performance.
     */
    if (XOR_COND((verify_p->blocks <= 0)))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "Request blocks: 0x%llx not expected \n",
                              (unsigned long long)verify_p->blocks);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Validate the width.
     */
    if (XOR_COND(((verify_p->width == 0) ||
                  (verify_p->width > FBE_XOR_MAX_FRUS))))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "Width: 0x%x not expected \n",
                               verify_p->width);
        return(FBE_STATUS_GENERIC_FAILURE);
    }
    
    /*! @todo Currently we validate that there is at least one source
     *        position, but it may be allowed to not have a valid source
     *        so that we invalidate the region. (Not sure why corrupt CRC/DATA
     *        wouldn't be used to accomplish this?)
     */
    valid_positions = fbe_xor_get_bitcount(verify_p->valid_bitmask);
    if (XOR_COND(valid_positions < 1))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "Must specify at least (1) source position. valid_bitmask: 0x%x \n",
                              verify_p->valid_bitmask); 
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* The sum of the valid and needs rebuild must equal the width.
     */
    needs_rebuild_positions = fbe_xor_get_bitcount(verify_p->needs_rebuild_bitmask);

    /* All sector buffers must be valid.
     */
    for (position = 0; position < verify_p->width; position++)
    {
        /* If no buffer was provided something is wrong.
         */
        if (XOR_COND(verify_p->sg_p[position] == NULL) &&
            (1 << position & verify_p->valid_bitmask))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "Buffer pointer for position: 0x%x is NULL.  All positions must have a valid buffer\n",
                                  position); 
            return(FBE_STATUS_GENERIC_FAILURE);
        }
    }

    /* Return the status.
     */
    return(status);
}
/* end of fbe_xor_validate_mirror_verify() */

/*!***************************************************************************
 *          fbe_xor_validate_mirror_rebuild()
 *****************************************************************************
 *
 * @brief   Validate that the mirror rebuild request is properly formed.
 *
 * @param   rebuild_p - Pointer to rebuild request to validate
 *
 * @return  status - If rebuild request is valid - FBE_STATUS_OK
 *                   Otherwise - Failure status
 *
 * @author
 *  01/05/2010  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_xor_validate_mirror_rebuild(fbe_xor_mirror_reconstruct_t *rebuild_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       valid_positions = 0;
    fbe_u32_t       needs_rebuild_positions = 0;
    fbe_u16_t       position;

    /* Only `check lba stamps' and `don't update lba stamp' and `debug' are supported
     */
    if (XOR_COND((rebuild_p->options & ~(FBE_XOR_OPTION_CHK_LBA_STAMPS       | 
                                         FBE_XOR_OPTION_DEBUG                | 
                                         FBE_XOR_OPTION_DEBUG_ERROR_CHECKSUM | 
                                         FBE_XOR_OPTION_LOGICAL_REQUEST      |
                                         FBE_XOR_OPTION_IGNORE_INVALIDS        )) != 0))
    {
        /* Unsupported options.
         */
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "Unsupported options: 0x%x \n",
                              rebuild_p->options);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Verify that the Calling driver has work for the XOR driver, otherwise
     * this may be an unnecessary call into the XOR driver and will effect
     * system performance.
     */
    if (XOR_COND(rebuild_p->blocks <= 0))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "Request blocks: 0x%llx not expected \n",
                              (unsigned long long)rebuild_p->blocks);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Validate the width.
     */
    if (XOR_COND(((rebuild_p->width == 0) ||
                  (rebuild_p->width > FBE_XOR_MAX_FRUS))))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "Width: 0x%x not expected \n",
                               rebuild_p->width);
        return(FBE_STATUS_GENERIC_FAILURE);
    }
    
    /*! @todo Currently we validate that there is at least one source
     *        position, but it may be allowed to not have a valid source
     *        so that we invalidate the region. (Not sure why corrupt CRC/DATA
     *        wouldn't be used to accomplish this?)
     */
    valid_positions = fbe_xor_get_bitcount(rebuild_p->valid_bitmask);
    if (XOR_COND(valid_positions < 1))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "Must specify at least (1) source position. valid_bitmask: 0x%x\n",
                              rebuild_p->valid_bitmask); 
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* Must specify at least (1) position to be rebuilt.
     */
    needs_rebuild_positions = fbe_xor_get_bitcount(rebuild_p->needs_rebuild_bitmask);
    if (XOR_COND(needs_rebuild_positions < 1))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "Must specify at least (1) rebuild position. needs_rebuild_bitmask: 0x%x\n",
                              rebuild_p->needs_rebuild_bitmask); 
        return(FBE_STATUS_GENERIC_FAILURE);
    }
    
    /* Total of valid and positions that need rebuild must be the width.
     */
    if (XOR_COND(((valid_positions + needs_rebuild_positions) != rebuild_p->width)))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "Sum of valid and needs rebuild positions: 0x%x doesn't match width: 0x%x\n",
                              (valid_positions + needs_rebuild_positions), rebuild_p->width); 
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* All sector buffers must be valid.
     */
    for (position = 0; position < rebuild_p->width; position++)
    {
        /* If no buffer was provided something is wrong.
         */
        if (XOR_COND(rebuild_p->sg_p[position] == NULL))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "Buffer pointer for position: 0x%x is NULL.  All positions must have a valid buffer\n",
                                  position); 
            return(FBE_STATUS_GENERIC_FAILURE);
        }
    }

    /* Return the status.
     */
    return(status);
}
/* end fbe_xor_validate_mirror_rebuild() */

/*!***************************************************************************
*           fbe_xor_rebuild_r1_unit()
******************************************************************************
*
* @brief    This method optionaly validate the specified `primary' position and
*           rebuilds exactly the (1) position to be rebuilt (`rpos').
*
* @param    eboard_p - [I]   error board for marking different types of errors.
* @param    sector_p - [I]   vector of ptrs to sector contents.
* @param    bitkey   - [I]   position bitkey array for stamps/error boards.
* @param    width    - [I]   the total width of the array to be rebuilt
*                            (i.e. the size of sector[] and bitkey[])
* @param    primary_validated - [I] Indicates that we have already validated the
*                                   primary data.  For normal rebuild requests
*                                   this routine will validate the primary data.                
* @param    primary  - [I]   Primary (i.e. source position for rebuild)
* @param    needs_rebuild - [I]   the position to be rebuilt
* @param    seed     - [I]   seed value for checksum.
* @param    options   - [I]   only supports FBE_XOR_OPTION_CHK_LBA_STAMPS
* @param    invalid_bitmask_p  - [o]   bitmask indicating which sectors have been invalidated.
* @param    raw_mirror_io_b     -   indicates whether this I/O request is to a raw mirror or not.
*
* @return fbe_status_t
*
* @notes    This method acts on exactly (2) positions:
*           1. The primary position is optionally validated (if the validation
*              fails both the primary and needs_rebuild positions are invalidated)
*           2. The needs_rebuild position is a position that is know to need a
*              rebuild.
*           This function intentionally does not call 
*           determine_media_errors.  It is the job of the caller
*           to keep track of which errors are caused by media errors.
*
* @author
*   25-Jun-01               -BJP- Created
*   01/06/2010  Ron Proulx  - Updated to allow the the primary to be located
*                             at any position (instead of just 0) in either
*                             the sector[] or bitkey[] array.
****************************************************************************/
fbe_status_t fbe_xor_rebuild_r1_unit(fbe_xor_error_t * eboard_p,
                                     fbe_sector_t * sector_p[],
                                     fbe_u16_t bitkey[],
                                     fbe_u16_t width,
                                     fbe_bool_t primary_validated,
                                     fbe_u32_t primary,
                                     fbe_u32_t needs_rebuild,
                                     fbe_lba_t seed,
                                     fbe_xor_option_t options,
                                     fbe_u16_t *invalid_bitmask_p,
                                     fbe_bool_t raw_mirror_io_b)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u16_t       invalid_bitmap = 0;
    fbe_u32_t       invalid_count = 0;
    fbe_bool_t      primary_validation_failed = FBE_FALSE;
    fbe_bool_t      b_check_invalidated_lba = FBE_TRUE;     /* Default is to check seed */
    
    /* Only `check lba stamps' and `don't update lba stamp' and `debug' are supported
     */
    if (XOR_COND((options & ~(FBE_XOR_OPTION_CHK_LBA_STAMPS       | 
                              FBE_XOR_OPTION_DEBUG                | 
                              FBE_XOR_OPTION_DEBUG_ERROR_CHECKSUM | 
                              FBE_XOR_OPTION_LOGICAL_REQUEST      |
                              FBE_XOR_OPTION_IGNORE_INVALIDS        )) != 0))
    {
        /* Unsupported options.
         */
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "Unsupported options: 0x%x \n",
                              options);
        return FBE_STATUS_GENERIC_FAILURE;
    }
       
    /* The primary position must be specified.
     */
    if (XOR_COND(primary == FBE_XOR_INV_DRV_POS))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "primary 0x%x == FBE_XOR_INV_DRV_POS 0x%x\n",
                              primary,
                              FBE_XOR_INV_DRV_POS);  

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* The position to be rebuilt must be specified.
     */
    if (XOR_COND(needs_rebuild == FBE_XOR_INV_DRV_POS))
    {
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "needs_rebuild 0x%x == FBE_XOR_INV_DRV_POS 0x%x\n",
                              needs_rebuild,
                              FBE_XOR_INV_DRV_POS);  

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Determine if we need to validate the lba written to invalidated blocks
     * or not.
     */
    if (options & FBE_XOR_OPTION_LOGICAL_REQUEST)
    {
        /* This is a logical request, we cannot confirm the lba written in any
         * invalidated blocks to the seed passed.
        */
        b_check_invalidated_lba = FBE_FALSE;
    }

    /* Until the position is rebuilt, add it to the uncorrectable bitmap.
     * This does not apply to raw mirror I/O.
     */
    if (!raw_mirror_io_b)
    {
        eboard_p->u_crc_bitmap |= bitkey[needs_rebuild];
    }
    
    /* Since this method is also invoked from the mirror verify function
     * which validates the read before invoking this method, we only need
     * to validate the primary of it is a normal rebuild request.
     */
    if (primary_validated == FBE_FALSE)
    {
        fbe_u16_t pos = primary;
        fbe_u16_t dkey = bitkey[pos];
        
        invalid_count = 0;
        
       /* We only come in here if no verify needs to be performed
        * prior to rebuilding the unit, width for this case is 2
        */
        if (XOR_COND(width != 2))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "width 0x%x != 2\n",
                                  width);  

            return FBE_STATUS_GENERIC_FAILURE;
        }

       /* If we have read the sector, then
        * proceed to check its checksum.
        */
        if (0 == (eboard_p->u_crc_bitmap & dkey))
        {
            fbe_sector_t *dblk_p;
            fbe_u32_t rsum;
            fbe_u16_t cooked_csum = 0;
            
            /* The method `xorlib_is_valid_lba_stamp' will actually change the
             * lba stamp from 0x0000 to a valid lba_stamp.  But we should only
             * do this if `generate lba stamp' is set.  Therefore save the
             * original lba stamp so that we can restore it after the check.
             */
            dblk_p = sector_p[pos];
            rsum = xorlib_calc_csum(dblk_p->data_word);
            
            /* If the checksum read doesn't match the calculated checksum 
             * we should report the error. 
             */  
            cooked_csum = xorlib_cook_csum(rsum, XORLIB_SEED_UNREFERENCED_VALUE);
            if (dblk_p->crc != cooked_csum)
            {                         
               /* The checksum we've calculated doesn't
                * match the checksum attached to the data.
                * Mark this as an uncorrectable crc error.
                */
                primary_validation_failed = FBE_TRUE;
                eboard_p->u_crc_bitmap |= dkey;

                /* Determine the types of errors.
                 */
                status = fbe_xor_determine_csum_error(eboard_p, dblk_p, dkey, seed,
                                                      dblk_p->crc, cooked_csum, b_check_invalidated_lba);
                if (status != FBE_STATUS_OK) 
                { 
                    return status; 
                }

                /* For copy operations we simply need to validate that the 
                 * pattern is recognized.  If it is a properly invalidated
                 * sector we should simply copy it.
                 */
                if (options & FBE_XOR_OPTION_IGNORE_INVALIDS)
                {
                    primary_validation_failed = FBE_FALSE;
                    eboard_p->u_crc_bitmap &= ~dkey;
                }
            }
            else if ((options & FBE_XOR_OPTION_CHK_LBA_STAMPS) &&
                     !xorlib_is_valid_lba_stamp(&dblk_p->lba_stamp, seed, eboard_p->raid_group_offset))
            {
                /* The lba stamps doesn't mach, treat this
                 * the same as a checksum error.
                 */
                primary_validation_failed = FBE_TRUE;
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
        else
        {
            /* Else we were purposfully (to force an invalidation) given a
             * primary with an invalid crc.  Mark it as such.
             */
            primary_validation_failed = FBE_TRUE;
            eboard_p->u_crc_bitmap |= dkey;
        }
        
       /* The invalid bitmap is the uncorrectable crc bitmap
        * since this is the only error in the board we can get.
        */
        invalid_bitmap = eboard_p->u_crc_bitmap;
        invalid_count = fbe_xor_get_bitcount(invalid_bitmap);
    }
    else
    {
        /* Else verify has validated the primary so mark the position to
         * be rebuilt as invalid until it is actually rebuilt.
         */
        invalid_bitmap |= bitkey[needs_rebuild];
    }
    
    /* If we failed to validate the primary mark the sector appropriately.
     */
    if (primary_validation_failed == FBE_TRUE)
    {
       /* We don't have any good data to rebuild 
        * from, invalidate sectors
        */
        if (XOR_COND(invalid_bitmap == 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "invalid_bitmap == 0\n");  

            return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_xor_invalidate_sectors(sector_p,
                               bitkey,
                               width,
                               seed,
                               invalid_bitmap,
                               eboard_p);
        *invalid_bitmask_p = invalid_bitmap;
        return FBE_STATUS_OK;
    }
    
    /* Validate that the primary is ok.
     */
    if (XOR_COND((invalid_count >= width) ||
                  (primary >= width) ||
                  (primary == needs_rebuild) ||
                  ((bitkey[primary] & invalid_bitmap) != 0) ))
    {
          fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                "(invalid_count 0x%x >= width 0x%x ) ||"
                                " (primary 0x%x  >= width 0x%x ) || (primary 0x%x  == needs_rebuild 0x%x ) || "
                                " ((bitkey[%d] 0x%x & invalid_bitmap 0x%x ) != 0)\n",
                                invalid_count,
                                width,
                                primary,
                                width,
                                primary,
                                needs_rebuild,
                                primary,
                                bitkey[primary],
                                invalid_bitmap);

          return FBE_STATUS_GENERIC_FAILURE;
    }

    
    /* First correct any non-primary positions with the data from the
     * primary they update the rebuild position.
     */
    {
        fbe_u16_t pkey, skey;
        fbe_sector_t *pblk = NULL;
        fbe_sector_t *sblk = NULL;
        fbe_u32_t secondary = needs_rebuild;
        
        /* Get primary block and key.
         */
        pkey = bitkey[primary]; 
        pblk = sector_p[primary];
        
        /* Get secondary block and key.
         */
        skey = bitkey[secondary];
        sblk = sector_p[secondary];
                
        /* At this point we should have marked the secondary as invalid.
         */
        if (XOR_COND((skey & invalid_bitmap) == 0))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "(skey 0x%x & invalid_bitmap 0x%x) == 0\n",
                                  skey,
                                  invalid_bitmap);  

            return FBE_STATUS_GENERIC_FAILURE;
        }


        /* Copy the data and metadata from the primary to the secondary.
         */
        xorlib_copy_data(pblk->data_word,
                          sblk->data_word);   
                    
        sblk->crc = pblk->crc;
        sblk->time_stamp = pblk->time_stamp;
        sblk->write_stamp = pblk->write_stamp;
        sblk->lba_stamp = pblk->lba_stamp;
                    
        /* Update the `modified' and the `uncorrectable' bits.
         */
        eboard_p->m_bitmap |= skey;
        eboard_p->u_crc_bitmap &= ~skey;
        invalid_bitmap &= ~skey;
    }
    
    /* Return the bitmap of positions that are invalid.
     */
    *invalid_bitmask_p = invalid_bitmap;
    return FBE_STATUS_OK;
}
/* end fbe_xor_rebuild_r1_unit() */


/***************************************************************************
 * fbe_xor_equate_mirror()
 ***************************************************************************
 * @brief
 *  This function checks the CRC-ness and coherency of
 *  corresponding sectors from each drive in one mirror-
 *  pair of a Raid 1 or Raid 1/0 LU. If corrections can
 *  be completed, they are performed and noted.
 *  Uncorrectable errors are noted as well.
 *

 * @param eboard_p -  error board for marking different
 *                   types of errors.
 * @param pblk -  ptr to sector from primary.
 * @param pkey -  bitkey for primary.
 * @param sblk -  ptr to sector from secondary.
 * @param skey -  bitkey for secondary.
 * @param seed -  seed value for checksum.
 * @param option   - [I]   only supports FBE_XOR_OPTION_CHK_LBA_STAMPS
 *
 * @return fbe_status_t
 *
 * @notes
 *
 * @author
 *  06-Nov-98       -SPN-   Redesigned to use checksum toolkit.
 *  16-Aug-00       -RG-    Created from rg_equate_mirror().
 ***************************************************************************/

static fbe_status_t fbe_xor_equate_mirror(fbe_xor_error_t * eboard_p,
                             fbe_sector_t * pblk,
                             fbe_u16_t pkey,
                             fbe_sector_t * sblk,
                             fbe_u16_t skey,
                             fbe_lba_t seed,
                             fbe_xor_option_t option)
{
    fbe_xor_comparison_result_t cmpstat;
    fbe_u32_t rsum;
    fbe_u16_t pcsum,
        scsum;
    fbe_bool_t check_lba_stamps = (option & FBE_XOR_OPTION_CHK_LBA_STAMPS);
    fbe_status_t status;
    fbe_lba_t   offset = eboard_p->raid_group_offset;
        
    /* Only `check lba stamps' and `debug' are supported
     */
    if (XOR_COND((option & ~(FBE_XOR_OPTION_CHK_LBA_STAMPS | FBE_XOR_OPTION_DEBUG | FBE_XOR_OPTION_DEBUG_ERROR_CHECKSUM)) != 0))
    {
        /* Unsupported options.
         */
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "Unsupported option: 0x%x \n",
                              option);
        return FBE_STATUS_GENERIC_FAILURE;
    }            
    
    /* Both primary and secondary have live data
     * for this seed. Calculate the checksum for
     * the primary sector while comparing its
     * content with that of the secondary sector.
     */
    rsum = xorlib_calc_csum_and_cmp(pblk->data_word,
                                    sblk->data_word,
                                    (XORLIB_CSUM_CMP *)&cmpstat);
    pcsum = xorlib_cook_csum(rsum, (fbe_u32_t)seed);

    /* If the data is `data lost' simply update the checksum to the
     * `invalid' value.
     */
    if ((pcsum != pblk->crc)                                           &&
        (xorlib_is_sector_invalidated_for_data_lost(pblk) == FBE_TRUE)    )
    {
        /* Since the data is `data lost invalidated' use the `invalid' checksum
         */
        pcsum = pblk->crc;
    }

    /* Now compare the primary to the secondary
     */
    if (cmpstat == FBE_XOR_COMPARISON_RESULT_SAME_DATA)
    {
        /* The same data appears in both blocks.
         */
        if ((pcsum == pblk->crc) &&
            (pcsum == sblk->crc) &&
            ((check_lba_stamps == FBE_FALSE) ||
             xorlib_is_valid_lba_stamp(&pblk->lba_stamp, seed, offset) &&
             xorlib_is_valid_lba_stamp(&sblk->lba_stamp, seed, offset)))
        {
            /* Both sectors have correct checksums and lba stamps.
             * Nothing more to do.
             */
        }
        else if ((pcsum == pblk->crc) &&
                 ((check_lba_stamps == FBE_FALSE) ||
                  xorlib_is_valid_lba_stamp(&pblk->lba_stamp, seed, offset)))
        {
            if (pcsum != sblk->crc)
            {
                /* A checksum error.
                 * Determine the types of errors.
                 */
                status = fbe_xor_determine_csum_error( eboard_p, sblk, skey, seed,
                                                       sblk->crc, pcsum, FBE_TRUE);
                if (status != FBE_STATUS_OK) 
                { 
                    return status; 
                }
            }
            else
            {
                /* Must be a lba stamp error.
                 */
                if (XOR_COND(check_lba_stamps != FBE_TRUE))
                {
                    fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                          "check_lba_stamps != FBE_TRUE\n");  

                    return FBE_STATUS_GENERIC_FAILURE;
                }

                
                if (XOR_COND(xorlib_is_valid_lba_stamp(&sblk->lba_stamp, seed, offset)))
                {
                    fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                          "lba stamp is not legal: lba stamp = 0x%x, seed = 0x%llx\n",
                                          sblk->lba_stamp,
                                          (unsigned long long)seed);  

                    return FBE_STATUS_GENERIC_FAILURE;
                }

                /* Log the Error. 
                 */
                status = fbe_xor_record_invalid_lba_stamp(seed, skey, eboard_p, sblk);
                if (status != FBE_STATUS_OK) 
                { 
                    return status; 
                }
            }


            /* The primary has the correct checksum.
             * Correct the secondary.
             */
            sblk->crc = pcsum;

            /* Also don't set lba stamps if we were told not to check.
             * (its a parity position)
             */
            if (check_lba_stamps != FBE_FALSE)
            {
                sblk->lba_stamp = xorlib_generate_lba_stamp(seed, eboard_p->raid_group_offset);
            }

            eboard_p->c_crc_bitmap |= skey;
        }
        else if ((pcsum == sblk->crc) ||
                 ((check_lba_stamps == FBE_TRUE) &&
                   (xorlib_is_valid_lba_stamp(&sblk->lba_stamp, seed, offset))))
        {
            if (pcsum != pblk->crc)
            {
                /* A checksum error.
                 * Determine the types of errors.
                 */
                status = fbe_xor_determine_csum_error( eboard_p, pblk, pkey, seed,
                                                       pblk->crc, pcsum, FBE_TRUE);
                if (status != FBE_STATUS_OK) 
                { 
                    return status; 
                }
            }
            else
            {
                /* Must be a lba stamp error.
                 */
                
                if (XOR_COND(check_lba_stamps != FBE_TRUE))
                {
                    fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                          "check_lba_stamps != FBE_TRUE\n");  

                    return FBE_STATUS_GENERIC_FAILURE;
                }
                
                if (XOR_COND(!xorlib_is_valid_lba_stamp(&pblk->lba_stamp, seed, offset)))
                {
                    fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                          "lba stamp is not legal: lba stamp = 0x%x, seed = 0x%llx\n",
                                          pblk->lba_stamp,
                                          (unsigned long long)seed);  

                    return FBE_STATUS_GENERIC_FAILURE;
                }

                /* We already know the 'type' of checksum error, 
                 * just set the type and log the error.
                 */
                eboard_p->crc_lba_stamp_bitmap |= pkey;

                /* Log the Error. 
                 */
                status = fbe_xor_record_invalid_lba_stamp(seed, pkey, eboard_p, pblk);
                if (status != FBE_STATUS_OK) 
                { 
                    return status; 
                }
            }

            /* The secondary has the correct checksum.
             * Correct the primary.
             */
            pblk->crc = pcsum;

            /* Also don't set lba stamps if we were told not to check.
             * (its a parity position)
             */
            if (check_lba_stamps != FBE_FALSE)
            {
                pblk->lba_stamp = xorlib_generate_lba_stamp(seed, eboard_p->raid_group_offset);
            }

            eboard_p->c_crc_bitmap |= pkey;
        }
        else
        {
            /* Determine the types of errors on both blocks
             * since both blocks are bad.
             */
            if (pcsum != pblk->crc)
            {
                /* A checksum error.
                 * Determine the types of errors.
                 */
                status = fbe_xor_determine_csum_error( eboard_p, pblk, pkey, seed,
                                                       pblk->crc, pcsum, FBE_TRUE);
                if (status == FBE_STATUS_OK) { return status; }
            }
            else
            {
                /* Must be a lba stamp error.
                 */
                
                if (XOR_COND(check_lba_stamps != FBE_TRUE))
                {
                    fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                          "check_lba_stamps != FBE_TRUE\n");  

                    return FBE_STATUS_GENERIC_FAILURE;
                }
                
                if (XOR_COND(!xorlib_is_valid_lba_stamp(&sblk->lba_stamp, seed, offset)))
                {
                    fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                          "lba stamp is not legal: lba stamp = 0x%x, seed = 0x%llx\n",
                                          sblk->lba_stamp,
                                          (unsigned long long)seed);  

                    return FBE_STATUS_GENERIC_FAILURE;
                }

                /* Log the Error. 
                 */
                status = fbe_xor_record_invalid_lba_stamp(seed, pkey, eboard_p, pblk);
                if (status != FBE_STATUS_OK) 
                {
                    return status; 
                }
            }

            if (pcsum != sblk->crc)
            {
                /* A checksum error.
                 * Determine the types of errors.
                 */
                status = fbe_xor_determine_csum_error( eboard_p, sblk, skey, seed,
                                                       sblk->crc, pcsum, FBE_TRUE);
                if (status != FBE_STATUS_OK) 
                { 
                    return status; 
                }
            }
            else
            {
                /* Must be a lba stamp error.
                 */
                
                if (XOR_COND(check_lba_stamps != FBE_TRUE))
                {
                    fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                          "check_lba_stamps != FBE_TRUE\n");  

                    return FBE_STATUS_GENERIC_FAILURE;
                }
                
                if (XOR_COND(!xorlib_is_valid_lba_stamp(&pblk->lba_stamp, seed, offset)))
                {
                    fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                          "lba stamp is not legal: lba stamp = 0x%x, seed = 0x%llx\n",
                                          pblk->lba_stamp,
                                          (unsigned long long)seed);  

                    return FBE_STATUS_GENERIC_FAILURE;
                }

                /* Log the Error. 
                 */
                status = fbe_xor_record_invalid_lba_stamp(seed, skey, eboard_p, sblk);
                if (status != FBE_STATUS_OK) 
                { 
                    return status; 
                }
            }
            

            /* Both sectors have incorrect checksums. This
             * could be a sector we've forcibly invalidated
             * on both disks, or an ugly twist of fate.
             * We must mark both as uncorrectable.
             */
            eboard_p->u_crc_bitmap |= (pkey | skey);
        }
    }
    else if ((pcsum != pblk->crc)                                               ||
             ((check_lba_stamps == FBE_TRUE)                                &&
              (!xorlib_is_valid_lba_stamp(&pblk->lba_stamp, seed, offset))    )   )
    {
        if (pcsum != pblk->crc)
        {
            /* Determine the types of error on primary.
             */
            status = fbe_xor_determine_csum_error( eboard_p, pblk, pkey, seed,
                                                   pblk->crc, pcsum, FBE_TRUE);
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }
        }
        else
        {
            /* Must be a lba stamp error.
            */
            
             if (XOR_COND(check_lba_stamps != FBE_TRUE))
             {
                 fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                       "check_lba_stamps != FBE_TRUE\n");  

                 return FBE_STATUS_GENERIC_FAILURE;
             }
            
             if (XOR_COND(!xorlib_is_valid_lba_stamp(&pblk->lba_stamp, seed, offset)))
             {
                 fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                       "lba stamp is not legal: lba stamp = 0x%x, seed = 0x%llx\n",
                                       pblk->lba_stamp,
                                       (unsigned long long)seed);  

                 return FBE_STATUS_GENERIC_FAILURE;
             }

            /* Log the Error. 
             */
            status = fbe_xor_record_invalid_lba_stamp(seed, pkey, eboard_p, pblk);
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }
        }

        /* The primary sector has an incorrect checksum or lba stamp.
         * We'll need to compute the checksum for the
         * secondary, and we'll copy its data onto the
         * primary as we go.
         */
        rsum = xorlib_calc_csum_and_cpy(sblk->data_word,
                                        pblk->data_word);
        scsum = xorlib_cook_csum(rsum, (fbe_u32_t)seed);

        /* If the data is `data lost invalidated' simply update the checksum to the
         * `invalid' value.
         */
        if ((scsum != sblk->crc)                                           &&
            (xorlib_is_sector_invalidated_for_data_lost(sblk) == FBE_TRUE)    )
        {
            /* Since the data is `data lost invalidated' use the `invalid' checksum
             */
            scsum = sblk->crc;
        }
        pblk->crc = scsum;

        /* Also don't set lba stamps if we were told not to check.
         * (its a parity position)
         */
        if (check_lba_stamps != FBE_FALSE)
        {
            pblk->lba_stamp = xorlib_generate_lba_stamp(seed, eboard_p->raid_group_offset);
        }

        if ((scsum == sblk->crc) &&
            ((check_lba_stamps == FBE_FALSE) ||
             (xorlib_is_valid_lba_stamp(&sblk->lba_stamp, seed, offset))))
        {
            /* The secondary has a valid checksum and lba stamp,
             * so the primary has been corrected.
             */
            eboard_p->c_crc_bitmap |= pkey;
        }

        else
        {
            /* The secondary has an invalid checksum or lba stamp
             * as well. Mark both as uncorrectable.
             */
            eboard_p->u_crc_bitmap |= (pkey | skey);

            if (scsum != sblk->crc)
            {
                /* Since Secondary is bad as well, determine error.
                 */
                status = fbe_xor_determine_csum_error( eboard_p, sblk, skey, seed,
                                                       sblk->crc, scsum, FBE_TRUE);
                if (status != FBE_STATUS_OK) 
                {
                    return status; 
                }
            }
            else
            {
                /* Must be a lba stamp error.
                */
                
                if (XOR_COND(check_lba_stamps != FBE_TRUE))
                {
                    fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                          "check_lba_stamps != FBE_TRUE\n");

                    return FBE_STATUS_GENERIC_FAILURE;
                }
                
                if (XOR_COND(!xorlib_is_valid_lba_stamp(&sblk->lba_stamp, seed, offset)))
                {
                    fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                          "lba stamp is not legal: lba stamp = 0x%x, seed = 0x%llx\n",
                                          sblk->lba_stamp,
                                          (unsigned long long)seed);  

                    return FBE_STATUS_GENERIC_FAILURE;
                }

                /* Log the Error. 
                 */
                status = fbe_xor_record_invalid_lba_stamp(seed, skey, eboard_p, sblk);
                if (status != FBE_STATUS_OK) 
                { 
                    return status; 
                }
            }
        }
    }

    else
    {
        /* The primary has a valid checksum,
         * but the primary and secondary data is not the same.
         */
        status = fbe_xor_equate_mirror_primary_valid(eboard_p, pblk, pkey, sblk, skey, seed, option);
        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
    }

    return FBE_STATUS_OK;
}
/*****************************************
 * fbe_xor_equate_mirror() 
 *****************************************/

/***************************************************************************
 * fbe_xor_equate_mirror_primary_valid()
 ***************************************************************************
 * @brief
 *  This function checks the CRC-ness and coherency of
 *  corresponding sectors from each drive in one mirror-
 *  pair of a Raid 1 or Raid 1/0 LU. If corrections can
 *  be completed, they are performed and noted.
 *  Uncorrectable errors are noted as well.
 *

 * @param eboard_p -  error board for marking different
 *                   types of errors.
 * @param pblk - ptr to sector from primary.
 * @param pkey - bitkey for primary.
 * @param sblk - ptr to sector from secondary.
 * @param skey - bitkey for secondary.
 * @param seed - seed value for checksum.
 *  option    [I]   only supports FBE_XOR_OPTION_CHK_LBA_STAMPS
 *
 * @return fbe_status_t
 *
 * @notes
 *
 * @author
 *  08/22/05:  Created. Rob Foley
 *  03/22/10:  Omer Miranda -- Updated to use the new sector tracing facility.
 ***************************************************************************/

static fbe_status_t fbe_xor_equate_mirror_primary_valid( fbe_xor_error_t * eboard_p,
                                            fbe_sector_t * pblk,
                                            fbe_u16_t pkey,
                                            fbe_sector_t * sblk,
                                            fbe_u16_t skey,
                                            fbe_lba_t seed,
                                            fbe_xor_option_t option )
{
    fbe_u32_t rsum;
    fbe_u16_t scsum;
    fbe_bool_t check_lba_stamps = (option & FBE_XOR_OPTION_CHK_LBA_STAMPS);
    fbe_status_t status;
    fbe_lba_t offset = eboard_p->raid_group_offset;
    
    /* Only `check lba stamps' and `debug' are supported
     */
    if (XOR_COND((option & ~(FBE_XOR_OPTION_CHK_LBA_STAMPS | FBE_XOR_OPTION_DEBUG | FBE_XOR_OPTION_DEBUG_ERROR_CHECKSUM)) != 0))
    {
        /* Unsupported options.
         */
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "Unsupported option: 0x%x \n",
                              option);
        return FBE_STATUS_GENERIC_FAILURE;
    }  
    
    /* The primary has a valid checksum. For the purpose of
     * determining if this is a consistency error or a checksum
     * error, we need to calculate the checksum of the secondary.
     * We also need to copy the data from the primary onto
     * the secondary; its a pity we can't do both at once, but
     * there's currently no interface to support that. Maybe
     * something to be added later...
     */
    rsum = xorlib_calc_csum(sblk->data_word);
    scsum = xorlib_cook_csum(rsum, (fbe_u32_t)seed);

    /* If the data is `data lost invalidated' simply update the checksum to the
     * `invalid' value.
     */
    if ((scsum != sblk->crc)                                           &&
        (xorlib_is_sector_invalidated_for_data_lost(sblk) == FBE_TRUE)    )
    {
        /* Since the data is `data lost invalidated' use the `invalid' checksum
         */
        scsum = sblk->crc;
    }

    if ((scsum == sblk->crc) &&
        ((check_lba_stamps == FBE_FALSE) ||
         (xorlib_is_valid_lba_stamp(&sblk->lba_stamp, seed, offset))))
    {
        /* The secondary has a valid checksum and lba stamp, so we
         * mark it as a correctable coherency error.
         */
        eboard_p->c_coh_bitmap |= skey;

        /* Save the sector for debugging purposes (status ignored).
         */
        fbe_xor_report_error_trace_sector(seed,  /* Lba */
                                          skey, /* Bitmask of position in group. */
                                          0,
                                          eboard_p->raid_group_object_id,
                                          eboard_p->raid_group_offset, 
                                          sblk, /* Sector to save. */
                                          "coherency on secondary",    /* Error type. */ 
                                          FBE_SECTOR_TRACE_ERROR_LEVEL_ERROR,    /* Trace level */
                                          FBE_SECTOR_TRACE_ERROR_FLAG_COH    /* Trace error type */); 

        /* Trace the primary which will be used to correct the secondary.
         */
        fbe_xor_trace_sector(seed,  /* Lba */
                             pkey, /* Bitmask of position in group. */
                             0,
                             eboard_p->raid_group_object_id,
                             eboard_p->raid_group_offset, 
                             pblk, /* Sector to save. */
                             "coherency primary data used",    /* Error type. */ 
                             FBE_SECTOR_TRACE_ERROR_LEVEL_ERROR,    /* Trace level */
                             FBE_SECTOR_TRACE_ERROR_FLAG_COH    /* Trace error type */); 
    }
    else
    {
        /* The secondary has an invalid checksum or lba stamp, so
         * we mark it as a correctable checksum error.
         */
        eboard_p->c_crc_bitmap |= skey;

        if (scsum != sblk->crc)
        {
            /* Since Secondary is bad, determine error type.
             */
            status = fbe_xor_determine_csum_error( eboard_p, sblk, skey, seed,
                                                   sblk->crc, scsum, FBE_TRUE);
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }
        }
        else
        {
            /* Must be a lba stamp error.
             */
            if (XOR_COND(check_lba_stamps != FBE_TRUE))
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "check_lba_stamps != FBE_TRUE\n");  

                return FBE_STATUS_GENERIC_FAILURE;
            }
            
            if (XOR_COND(xorlib_is_valid_lba_stamp(&sblk->lba_stamp, seed, offset)))
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                      "lba stamp 0x%x error is not there, seed = 0x%llx\n",
                                      sblk->lba_stamp,
                                      (unsigned long long)seed);  

                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Log the Error. 
             */
            status = fbe_xor_record_invalid_lba_stamp(seed, skey, eboard_p, sblk);
            if (status != FBE_STATUS_OK) 
            { 
                return status; 
            }
        }

    }

    /* Okay, copy the data from the primary to the
     * secondary, and set the checksum value.
     */
    xorlib_copy_data(pblk->data_word,
                     sblk->data_word);

    sblk->crc = pblk->crc;
    sblk->lba_stamp = pblk->lba_stamp;
    return FBE_STATUS_OK;
}
/****************************************
 * fbe_xor_equate_mirror_primary_valid()
 ****************************************/
/***************************************************************************
 *                        fbe_xor_rebuild_mirror()
 ***************************************************************************
 * @brief
 *  This function checks the CRC-ness of the single
 *  readable sector from one member of a mirror-pair
 *  of a Raid 1 or Raid 1/0 LU. The lost sector is
 *  corrected if the sector read has a good CRC;
 *  otherwise, both sectors are uncorrectable.
 *

 * @param eboard_p -  error board for marking different
 *                     types of errors.
 * @param blk - ptr to good sector.
 * @param gkey - bitkey for good, i.e. available, sector.
 * @param bblk - ptr to bad sector.
 * @param bkey - bitkey for bad, i.e. lost, sector.
 *  option   - [I]   only supports FBE_XOR_OPTION_CHK_LBA_STAMPS
 *
 * @return fbe_status_t:
 *  
 *
 * @notes
 *
 * @author
 *  09/19/00: Rob Foley - Created.
 *
 ***************************************************************************/

static fbe_status_t fbe_xor_rebuild_mirror(fbe_xor_error_t * eboard_p,
                              fbe_sector_t * blk_p,
                              fbe_lba_t seed,
                              fbe_u16_t key,
                              fbe_u16_t rb_key,
                              fbe_xor_option_t option)
{
    fbe_u32_t rsum;
    fbe_u16_t csum;
   
    /* Only `check lba stamps' and `debug' are supported
     */
    if (XOR_COND((option & ~(FBE_XOR_OPTION_CHK_LBA_STAMPS | FBE_XOR_OPTION_DEBUG | FBE_XOR_OPTION_DEBUG_ERROR_CHECKSUM)) != 0))
    {
        /* Unsupported options.
         */
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                              "Unsupported option: 0x%x \n",
                              option);
        return FBE_STATUS_GENERIC_FAILURE;
    }          
    
    /* Re-validate the `good' (a.k.a. primary) checksum
     * and if valid update the modify and uncorrectable mask
     * for the position that was rebuilt.  Otherwise if the
     * primary position is bad also mark both positions as
     * uncorrectable
     */
    rsum = xorlib_calc_csum(blk_p->data_word);
    csum = xorlib_cook_csum(rsum, 0);

    /* If the data is `data lost invalidated' simply update the checksum to the
     * `invalid' value.
     */
    if ((csum != blk_p->crc)                                            &&
        (xorlib_is_sector_invalidated_for_data_lost(blk_p) == FBE_TRUE)    )
    {
        /* Since the data is `data lost invalidated' use the `invalid' checksum
         */
        csum = blk_p->crc;
    }

    /* If the checksum and lba stamps of the `good' sector are `good', then
     * mark the rebuild position as `good' (we will write the data from the
     * `good' sector to the `needs rebuild' sector.
     */
    if ((blk_p->crc == csum) &&
        (!(option & FBE_XOR_OPTION_CHK_LBA_STAMPS) ||
         (xorlib_is_valid_lba_stamp(&blk_p->lba_stamp, seed, eboard_p->raid_group_offset))))
    {
        /* Checksum is valid.
         * Don't touch the other stamps.
         */
        eboard_p->m_bitmap |= rb_key,
            eboard_p->u_crc_bitmap &= ~rb_key;
    }

    else
    {
        /* Checksum is bad.
         * This is now an uncorrectable crc error for both sectors.
         */
        eboard_p->u_crc_bitmap |= (rb_key | key);
    }

    return FBE_STATUS_OK;
}
/* fbe_xor_rebuild_mirror() */


/*!***************************************************************************
 *          fbe_xor_raw_mirror_sector_validate()
 *****************************************************************************
 *
 * @brief   This routine is called for raw mirror I/O validation.  It checks the
 *          raw mirror data's metadata to determine the correct position to use
 *          for reads.
 *
 *
 * @param   eboard_p - [I]   error board for marking different types of errors.
 * @param   sector_p - [I]   vector of ptrs to sector contents.
 * @param   bitkey   - [I]   bitkey for stamps/error boards.
 * @param   width    - [I]   width of the raid group
 * @param   rm_positions_to_check_bitmap  - [I]   bitmask indicating which positions to check for validation.
 * @param   primary  - [IO]  primary mirror to use for read; may be changed by this routine
 *
 * @return  none
 *
 * @author
 *  11/2011 - Created. Susan Rundbaken
 *
 *****************************************************************************/
static void fbe_xor_raw_mirror_sector_validate(fbe_xor_error_t * eboard_p,
                                               fbe_sector_t * sector_p[],
                                               fbe_u16_t bitkey[],
                                               fbe_u16_t width,
                                               fbe_u16_t rm_positions_to_check_bitmap,
                                               fbe_u32_t *primary)
{
    fbe_raw_mirror_sector_data_t  *reference_p;         /* pointer to the raw mirror sector data and metadata */
    fbe_u16_t   pos;                                    /* used to traverse mirrors in the raw mirror */
    fbe_u64_t   cur_highest_seq_num = FBE_LBA_INVALID;  /* used to keep track of greatest sequence number across mirrors */
    fbe_u16_t   cur_highest_seq_num_pos;                /* used to keep track of position with greatest sequence number */


    for (pos = 0; pos < width; pos++)        
    {
        fbe_u16_t dkey = bitkey[pos];

        if (dkey & rm_positions_to_check_bitmap)
        {
            reference_p = (fbe_raw_mirror_sector_data_t*)sector_p[pos];

            /* Validate magic number. 
             */
            if (reference_p->magic_number != FBE_MAGIC_NUMBER_RAW_MIRROR_IO)
            {
                /* Invalid magic number; update eboard for pos.
                 */
                eboard_p->c_rm_magic_bitmap |= dkey;

                if (pos == *primary)
                {
                    /* Primary no longer valid.
                     */
                    *primary = FBE_XOR_INV_DRV_POS;
                }
                continue;
            }

            /* Magic number okay for this position; compare sequence number to 
             * tracking sequence number. 
             */
            if (cur_highest_seq_num == FBE_LBA_INVALID)
            {
                /* Initialize the local sequence number and position tracking variables.
                 */
                cur_highest_seq_num = reference_p->sequence_number;
                cur_highest_seq_num_pos = pos;

                /* Set the primary if it was invalidated.
                 * It could have been invalidated by the magic number check.
                 */
                if (*primary == FBE_XOR_INV_DRV_POS)
                {
                    *primary = pos;
                }
                continue;
            }

            if (reference_p->sequence_number == cur_highest_seq_num)
            {
                cur_highest_seq_num_pos = pos;
            }

            if (reference_p->sequence_number != cur_highest_seq_num)
            {
                fbe_u16_t skey;
                fbe_s32_t pos_to_add;

                if (reference_p->sequence_number > cur_highest_seq_num)
                {
                    /* Tracking highest position is less than current position, so tracking
                     * highest position needs to change.
                     */

                    /* Add tracking highest position and any valid positions before it to
                     * the eboard as correctable errors.
                     */
                    for (pos_to_add = cur_highest_seq_num_pos; pos_to_add >= 0; pos_to_add--)
                    {
                        /* Get bitkey for eboard update. 
                         */
                        skey = bitkey[pos_to_add];
    
                        if (skey & rm_positions_to_check_bitmap)
                        {
                            /* Update eboard for failed pos. 
                             */
                            eboard_p->c_rm_seq_bitmap |= skey;
                        }
                    }
                    /* Update tracking seq num and position
                     */
                    cur_highest_seq_num = reference_p->sequence_number;
                    cur_highest_seq_num_pos = pos;
                    if (*primary != pos)
                    {
                        /* Got a new primary.
                         */
                        *primary = pos;
                    }
                }
                else
                {
                    /* Position sequence number is less than current highest, so this position
                     * has an invalid sequence number. 
                     * Set the bitkey for eboard update.
                     */
                    skey = bitkey[pos];
    
                    /* Update eboard for failed pos. 
                     */
                    eboard_p->c_rm_seq_bitmap |= skey;
                }
            }
        }
    }
    if (*primary == FBE_XOR_INV_DRV_POS)
    {
        /* If primary is no longer valid, could not find a drive with a valid magic number; update eboard.
         */
        eboard_p->u_rm_magic_bitmap |= eboard_p->c_rm_magic_bitmap;
        eboard_p->c_rm_magic_bitmap &= ~eboard_p->c_rm_magic_bitmap;
    }

    return;
}
/* end of fbe_xor_raw_mirror_sector_validate() */ 


/*!**************************************************************
 * fbe_xor_lib_raw_mirror_sector_set_seq_num()
 ****************************************************************
 * @brief
 *  This function appends the given sequence number to the given sector
 *  for raw mirror I/O.
 * 
 *  It also appends the raw mirror I/O magic number used internally by RAID.
 *
 * @param sector_p - pointer to the sector to update.
 * @param sequence_num - sequence number to append to sector.
 *
 * @return fbe_status_t.
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken
 *
 ****************************************************************/
fbe_status_t fbe_xor_lib_raw_mirror_sector_set_seq_num(fbe_sector_t *sector_p, 
                                                       fbe_u64_t sequence_num)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_raw_mirror_sector_data_t    *reference_p;

    reference_p = (fbe_raw_mirror_sector_data_t*)sector_p;
    reference_p->sequence_number = sequence_num;
    reference_p->magic_number = FBE_MAGIC_NUMBER_RAW_MIRROR_IO;

    return status;
}
/**************************************
 * end fbe_xor_lib_raw_mirror_sector_set_seq_num
 **************************************/

/*!**************************************************************
 * fbe_xor_lib_raw_mirror_sector_get_seq_num()
 ****************************************************************
 * @brief
 *  This function returns the sequence number from a given sector
 *  for raw mirror I/O.
 *
 * @param sector_p - pointer to the sector.
 *
 * @return fbe_u64_t - the sequence number associated with that sector.
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken
 *
 ****************************************************************/
fbe_u64_t fbe_xor_lib_raw_mirror_sector_get_seq_num(fbe_sector_t *sector_p)
{
    fbe_raw_mirror_sector_data_t    *reference_p;
    fbe_u64_t                       sequence_num;

    reference_p = (fbe_raw_mirror_sector_data_t*)sector_p;
    sequence_num = reference_p->sequence_number;
          
    return sequence_num;
}
/**************************************
 * end fbe_xor_lib_raw_mirror_sector_get_seq_num
 **************************************/


/*****************************************
 * End of fbe_xor_mirror.c
 *****************************************/
