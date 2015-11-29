/***************************************************************************
 * Copyright (C) EMC Corporation 2005-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file xor_check_sector.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions to deal with reporting of specific
 *  checksum errors.  This is what we refer to as "checking" the contents
 *  of a sector to determine if it matches a particular pattern.
 *
 * FUNCTIONS:
 *  fbe_xor_determine_csum_error
 *  fbe_xor_sector_invalidated_by_flare
 *  fbe_xor_determine_media_errors
 *  xor_sector_invalidated_by_klondike
 *  xor_count_bits
 *
 * @notes
 *  none
 *
 * @author
 *  02/02/2005 Created. Rob Foley.
 *
 ***************************************************************************/

/************************
 * INCLUDE FILES
 ************************/

#include "fbe_xor_private.h"
#include "fbe/fbe_library_interface.h"
#include "fbe/fbe_data_pattern.h"


/****************************************
 * static FUNCTIONS
 ****************************************/

static fbe_bool_t fbe_xor_sector_invalidated_by_klondike( const fbe_sector_t * const blk_p );

static fbe_u32_t fbe_xor_count_bits( fbe_u16_t xor_sum );


/****************************************
 * FUNCTIONS
 ****************************************/

/*****************************************************************************
 *          fbe_xor_determine_csum_error()
 ***************************************************************************** 
 * 
 * @brief   This function determines the kind of error that we are receiving.
 *
 *  We can choose from several different types of errors and
 *  will fill out the appropriate bit in the eboard for this block.
 *  If we have an unknown CRC error, the calculated checksum is 
 *  compared to one read from disk to determine if they differ 
 *  by more than 1 bit.
 *
 *  We are here because this block has been deemed to have
 *  a checksum error.
 *
 * @note
 *  We assume that if any of the "known" invalid sector formats
 *  changes, then this function and logic in the caller
 *  that interprets the eboard will need to change.
 *
 * @param   eboard_p - Current error board.
 * @param   blk_p - Block to classify.
 * @param   key - Current array bit position of this sector.
 *                  In other words, this is the bit position to 
 *                  modify within eboard_p to save the error type.
 * @param   seed - Seed of block, which is usually the lba.
 * @param   csum_read - The checksum read from disk.  
 * @param   csum_calc - The calculated checksum.
 * @param   b_check_invalidated_lba - FBE_TRUE - The lba in the invalidated
 *              block must match the seed.
 *                                    FBE_FALSE - The seed passed is logical
 *              and therefore we cannot check the lba in the invalidated
 *              block.
 *
 * @return fbe_status_t.
 *
 *
 * @author
 *  01/27/05 - Created. Rob Foley
 *  06/10/09 - HEW  Detemine if we have a single or multi-bit CRC error
 *
 *****************************************************************************/
fbe_status_t fbe_xor_determine_csum_error( fbe_xor_error_t * const eboard_p,
                               const fbe_sector_t * const blk_p,
                               const fbe_u16_t key,
                               const fbe_lba_t seed,
                               const fbe_u16_t csum_read,
                               const fbe_u16_t csum_calc,
                               const fbe_bool_t b_check_invalidated_lba)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u16_t lba_stamp;

    /* For now we are not sure what kind of checksum error
     * this block has.  Assume it's unknown.
     *
     * Below we will fill out the InvalidSectorType.
     * If we did not detect any of the known
     * formats, then it will remain as the "unknown"
     * sector format when we exit this function.
     *
     * Unknown means that there is a correctable or uncorrectable
     * error, but we were not able to pin down where it came from.
     *
     * The caller has the responsibility of interpreting this
     * as an "unknown" error since none of the bits are set
     * for "known" errors.
     */
    xorlib_sector_invalid_reason_t InvalidSectorType = XORLIB_SECTOR_INVALID_REASON_UNKNOWN;
    
    /* First check for the Known Flare Invalid checksum format.
     */
    if ( xorlib_is_sector_invalidated( blk_p,
                                       &InvalidSectorType,
                                       seed,
                                       b_check_invalidated_lba ) )
    {
        if ( InvalidSectorType == XORLIB_SECTOR_INVALID_REASON_DH_INVALIDATED )
        {
            /* The DH Invalidated this sector.
             */
            eboard_p->crc_dh_bitmap |= key;
        }
        else if ( InvalidSectorType == XORLIB_SECTOR_INVALID_REASON_VERIFY )
        {            
            /* The Raid Driver invalidated this sector.
             */
            eboard_p->crc_raid_bitmap |= key;
        }
        else if ( InvalidSectorType == XORLIB_SECTOR_INVALID_REASON_DATA_LOST )
        {            
            /* This sector has been invalidated due to `data lost'
             */
            eboard_p->crc_invalid_bitmap |= key;
        }
        else if ( InvalidSectorType == XORLIB_SECTOR_INVALID_REASON_PVD_METADATA_INVALID )
        {
             /* This sector has been invalidated due to PVD Metadata invalid
             */
            eboard_p->crc_pvd_metadata_bitmap |= key;
        }
        else if ( InvalidSectorType == XORLIB_SECTOR_INVALID_REASON_CORRUPT_DATA )
        {
            /* The Raid Driver invalidated this sector because
             * of a corrupt data command.
             */
            eboard_p->corrupt_data_bitmap |= key;
        }

        else if ( InvalidSectorType == XORLIB_SECTOR_INVALID_REASON_RAID_CORRUPT_CRC )
        {
            /* All other codes defaults to the corrupt crc invalid sector format.
             * This format is written by raid when it receives a
             * corrupt crc command.  So raid was told to invalidate the block.
             *
             * Note that in debug builds we fbe_panic because
             * we do not expect any other codes.
             */

            /* The Raid Driver invalidated this sector because
             * of a corrupt crc command.
             */
            eboard_p->corrupt_crc_bitmap |= key;
        }
        else if ( InvalidSectorType == XORLIB_SECTOR_INVALID_REASON_COPY_INVALIDATED )
        {
            /* The COPY operation Invalidated this sector.
             */
            eboard_p->crc_copy_bitmap |= key;
        }
    }
    else if ( fbe_xor_sector_invalidated_by_klondike( blk_p ) )
    {
        /* Sector invalidated by klondike.  
         * Set the klondike error bit.
         */
        eboard_p->crc_klondike_bitmap |= key;
    }
    else 
    {
        /* Else if the sector was invalidated with the incorrect `seed' report
         * the error.
         */
        fbe_u32_t num_different_bits = fbe_xor_count_bits(csum_read ^ csum_calc);

        /* Check for the Known Flare Invalid checksum format without validating
         * the seed.
         */
        if ( xorlib_is_sector_invalidated( blk_p,
                                           &InvalidSectorType,
                                           seed,
                                           FBE_FALSE ) )
        {
            fbe_lba_t   invalidated_lba = 0;

            /* Get the lba from the invalidated block
             */
            xorlib_get_seed_from_invalidated_sector(blk_p, &invalidated_lba);

            /*! Trace the error and return a failure.
             */
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "xor: rg obj: 0x%x pos bm: 0x%x seed: 0x%llx didn't match invalidation lba: 0x%llx reason: %d",
                                  eboard_p->raid_group_object_id, key, seed, invalidated_lba, InvalidSectorType);
        }

        /* Checksum is bad but we didn't recognize
         * the format of the sector.
         * Set the CRC single or multi-bit error bitmap,
         * as appropiate.
         */

        /* Count the bits that differ in the exclusive OR of 
         * the CRC we read and the CRC we calculated.
         */
        if( num_different_bits > 1 )
        {
            /* Mult-bit CRC error.  Set the appropriate
             * bitmap so the caller will know.
             */
            eboard_p->crc_multi_bitmap |= key;
            lba_stamp = blk_p->lba_stamp;
            /* In some cases we also need to know if the lba stamp was also bad. 
             * When encrypted we end up not shooting drives that have bad checksums and bad lba stamps, 
             * since the drive shooting code was originally for drives that exhibited crc errors but matching lba 
             * stamps. 
             */
            if (!xorlib_is_valid_lba_stamp(&lba_stamp, seed, eboard_p->raid_group_offset)) {
                eboard_p->crc_multi_and_lba_bitmap |= key;
            }
        }
        else
        {
            /* Single bit error */
            eboard_p->crc_single_bitmap |= key;
        }
        
        /* Verify that the user passed in a valid raid group object identifier
         */
        if (XOR_COND(eboard_p->raid_group_object_id == FBE_OBJECT_ID_INVALID))
        {
            fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                                  "Invalid raid group object id: %d\n",
                                  eboard_p->raid_group_object_id);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Save the sector for debugging purposes.
         */
        status = fbe_xor_report_error_trace_sector( seed,  /* Lba */
                                              key, /* Bitmask of position in group. */
                                              num_different_bits,
                                              eboard_p->raid_group_object_id,
                                              eboard_p->raid_group_offset, 
                                              blk_p, /* Sector to save. */
                                              ( num_different_bits > 1 ) ?
                                              "checksum multi" : "checksum single", /* Error type. */ 
                                              FBE_SECTOR_TRACE_ERROR_LEVEL_ERROR, /* Tracing level */
                                              FBE_SECTOR_TRACE_ERROR_FLAG_UCRC  /* Error Type */);

        if (status != FBE_STATUS_OK) 
        { 
            return status; 
        }
    }

        
    /* Here we expect one of two cases to be true.
     * 
     * 1) We set one of the bits in the eboard related to
     *    specific type of checksum errors.
     *
     * or
     *
     * 2) The type of checksum error is unknown, so
     *    we set the key in the unknown_crc_bitmap.
     *
     */
    if (InvalidSectorType == XORLIB_SECTOR_INVALID_REASON_UNKNOWN)
    {
        eboard_p->crc_unknown_bitmap |= key;
    }
    return FBE_STATUS_OK;
}
/****************************************
 * fbe_xor_determine_csum_error()
 ****************************************/

/****************************************************************
 * fbe_xor_sector_invalidated_by_klondike()
 ****************************************************************
 * @brief
 *  This function determines if the sector was invalidated by Klondike.
 *
 *  We intentionally do not check all the possible fields in the
 *  invalid sector.  This is an optimization.
 *
 *  The Klondike invalid sector format has the first and last 
 *  4 bytes of 0xFF, and the rest of the sector zero.
 *
 *  We are here because this block has been deemed to have
 *  a checksum error.
 *

 * @param blk_p - The block to classify.
 *
 * @return
 *  fbe_bool_t - FBE_TRUE means this was invalidated by Klondike.
 *         FBE_FALSE means this was not invalidated by Klondike.
 *
 * @author
 *  01/27/05 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_xor_sector_invalidated_by_klondike( const fbe_sector_t * const blk_p )
{
    fbe_u32_t* word_ptr;
    
    /* Take an optimistic view.  This is the value we return.
     * We set it to FBE_TRUE here. 
     * Below if we see something that indicates 
     * the sector is not valid, then we set this to FBE_FALSE.
     */
    fbe_bool_t bReturnValue = FBE_TRUE;
    
    /* The below traits of this sector are checked to determine
     * if this is the Klondike invalid pattern.
     *
     * A Klondike invalidated sector has the first and last 
     * 4 bytes of 0xFF, and the rest of the sector zero.
     *
     * We made an optimization to only check the first
     * and last 8 bytes in the sector.
     */

    /* Check the first 4 bytes are 0xFF.
     */
    word_ptr = (fbe_u32_t*) blk_p->data_word;
    
    if ( *word_ptr != 0xFFFFFFFF )
    {
        bReturnValue = FBE_FALSE;
    }

    /* Check the next 4 bytes in the sector for zeros.
     */
    word_ptr++;

    if ( *word_ptr != 0 )
    {
        bReturnValue = FBE_FALSE;
    }

    /* Check the final 8 bytes in the sector.
     * This corresponds to TS, CRC, SS, WS.
     */
    if ( blk_p->time_stamp != 0 ||
         blk_p->crc != 0 ||
         blk_p->lba_stamp != 0xFFFF ||
         blk_p->write_stamp != 0xFFFF )
    {
        bReturnValue = FBE_FALSE;
    }
    
    /* Return the value computed above.
     */
    return bReturnValue;
}
/****************************************
 * fbe_xor_sector_invalidated_by_klondike()
 ****************************************/


/**************************************************************************
 * fbe_xor_determine_media_errors()
 **************************************************************************
 *
 * @brief
 *   Check to see if any checksum errors resulted from media errors.
 *

 * @param eboard_p - Eboard to check for errors.
 *
 * @param media_err_bitmap - Bitmap of positions with media errors.
 *                           Each set bit represents one 
 *                           drive with a media error.
 *
 * @todo    This method and all uses of it should be removed.  We not longer
 *          should see and support:
 *              o crc_dh_bitmap
 *              o crc_klondike_bitmap
 *
 * @return fbe_status_t
 *
 **************************************************************************/

fbe_status_t fbe_xor_determine_media_errors( fbe_xor_error_t *eboard_p,
                                 fbe_u16_t media_err_bitmap )
{
    /* If we did take a media error then our bitmap
     * will indicate which positions.
     *
     * If we did take a media error, then
     * any crc errors on that position must
     * be due to that media error.
     *
     * Recall that the xor driver converts all media errors
     * into uncorrectable crc errors for convenience.
     */
    if ( media_err_bitmap != 0 )
    {
        fbe_u16_t combined_crc_bitmap;

        /* Calculate how many combined correctable, 
         * uncorrectable crc errors we received.
         */
        combined_crc_bitmap = eboard_p->u_crc_bitmap | eboard_p->c_crc_bitmap;

        /* Next, determine how many of those CRC errors were
         * directly the result of a media error.
         *
         * Put these directly into the media_err_bitmap,
         * which will be bubbled up to the caller.
         */
        eboard_p->media_err_bitmap = (media_err_bitmap & combined_crc_bitmap);
    }
    
    /* If we took a media error on a position, then
     * it is not possible to have any of these other crc style errors
     * since that would imply that we actually read some data.
     * Also, if we had done a rebuild and seen a pattern, these
     * would have caused us to invalidate.
     */
     if (XOR_COND((eboard_p->media_err_bitmap & eboard_p->crc_klondike_bitmap) != 0))
     {
         fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_ERROR,
                               "both media_err_bitmap 0x%x and crc_klondike_bitmap 0x%x should not exist\n",
                               eboard_p->media_err_bitmap,
                               eboard_p->crc_klondike_bitmap);

         return FBE_STATUS_GENERIC_FAILURE;
     }

    return FBE_STATUS_OK;
}
/****************************************
 * fbe_xor_determine_media_errors()
 ****************************************/

/*!************************************************************************
 *  @fn fbe_xor_count_bits()
 **************************************************************************
 *
 * @brief This function is used to determine the number of bits that are 
 * set in the input parameter. 
 *
 * @param xor_sum - A 16 bit unsigned number. 
 *
 * @return - The number of bits that are set in xor_sum
 *
 * @revision
 *      06/10/09 HEW -- Created
 *
 *************************************************************************/

fbe_u32_t fbe_xor_count_bits( fbe_u16_t xor_sum )
{
    fbe_u32_t num_bits_on = 0;
    fbe_u16_t position;

    for( position = 0 ; position < 16 ; position++ )
    {
        if( (1 << position) & xor_sum )
        {
            num_bits_on++;
        }
    }

    return num_bits_on;
}
/****************************************
 * fbe_xor_count_bits()
 ****************************************/

/*!***************************************************************************
 *          fbe_xor_is_data_pattern_present()
 *****************************************************************************
 *
 * @brief   This method attempt to determine if there is any repeating pattern
 *          throughout the sector.  The token size is assumed to be 64-bit and
 *          every other token should match.  The sector is divided into (4)
 *          pieces and there must be a token A and B found in each piece.  If
 *          both A and B are zero it is not considered a match.
 *
 * @param   sector_p - Ptr to sector to check.
 *
 * @return  bool - FBE_TRUE - There is a data pattern in this sector 
 *
 * @author
 *  09/16/2011  Ron Proulx
 *
 ****************************************************************/
static fbe_bool_t fbe_xor_is_data_pattern_present(const fbe_sector_t *const sector_p)
{
    fbe_bool_t  b_data_pattern_present = FBE_FALSE;
    fbe_u32_t   num_of_pieces_in_sector = 4;
    fbe_u32_t   num_of_tokens_in_piece;
    fbe_u64_t   token_a = 0;
    fbe_u64_t   token_b = 0;
    fbe_u32_t   token_index;
    fbe_u32_t   piece_index;
    fbe_u64_t  *token_p = (fbe_u64_t *)sector_p;

    /* Get the number of tokens in a piece
     */
    num_of_tokens_in_piece = (FBE_WORDS_PER_BLOCK / 2) / num_of_pieces_in_sector;

    /* Walk thru first piece and determine if there is a valid pattern
     */
    for (token_index = 0; token_index < num_of_tokens_in_piece / 2; token_index++)
    {
        /* Compare the current and next tokens.
         */
        if ((token_p[token_index]     == token_p[token_index + 2]) &&
            (token_p[token_index + 1] == token_p[token_index + 3])    )
        {
            /* If the tokens are non-zero then it is a match.
             * Flag the fact that we found at least (1) token in this piece.
             */
            if ((token_p[token_index]     != 0) &&
                (token_p[token_index + 1] != 0)    )
            {
                b_data_pattern_present = FBE_TRUE;
                token_a = token_p[token_index];
                token_b = token_p[token_index + 1];
                break;
            }
        }
    }

    /* If we have found tokens in the first piece, validate that the same
     * tokens are found at least (1) time in each of the remaining pieces.
     */
    if (b_data_pattern_present == FBE_TRUE)
    {
        /* Walk thru all the pieces
         */
        for (piece_index = 1; piece_index < num_of_pieces_in_sector; piece_index++)
        {
            /* Set the token pointer and then check each piece.  We reset
             * b_data_pattern_present to False until we match both tokens in the
             * same row.
             */
            token_p = &(((fbe_u64_t *)sector_p)[piece_index * num_of_tokens_in_piece]);
            b_data_pattern_present = FBE_FALSE;
            for (token_index = 0; token_index < num_of_tokens_in_piece / 2; token_index++)
            {
                /* Compare the current against the known tokens.
                 */
                if ((token_p[token_index]     == token_a) &&
                    (token_p[token_index + 1] == token_b)    )
                {
                    /* We found at least one match in this piece.  Flag it
                     * and break out.
                     */
                    b_data_pattern_present = FBE_TRUE;
                    break;
                }
            }

            /* If both tokens were not found, exit
             */
            if (b_data_pattern_present != FBE_TRUE)
            {
                break;
            }

        } /* for all pieces */

    } /* if tokens were found in the first piece*/

    /* Return if we found a data pattern or not
     */
    return b_data_pattern_present;

}
/******************************************
 * end of fbe_xor_is_data_pattern_present()
 ******************************************/

/*!**************************************************************
 * fbe_xor_check_sector_for_repeating_pattern()
 ****************************************************************
 * @brief
 *  Check that the first 32 bytes of the sector match
 *  the other chunks of 32 bytes in the sector.
 *
 * @param sector_p - Ptr to sector to check.
 * @param seed - For reporting errors
 *
 * @return fbe_status_t   
 *
 * @author
 *  1/20/2011 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_xor_check_sector_for_repeating_pattern(const fbe_sector_t *const sector_p,
                                                               const fbe_lba_t seed)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       chunk_index;
    fbe_u32_t       bytes_per_chunk = 32;
    fbe_u32_t       words_per_chunk = bytes_per_chunk / sizeof(fbe_u32_t);
    fbe_u32_t       chunks_per_block = FBE_BYTES_PER_BLOCK / bytes_per_chunk;
    fbe_u32_t       start_chunk_index = 0; 
    fbe_bool_t      b_data_pattern_found;

    /*! @note We need to determine that this is valid data before checking.
     */
    b_data_pattern_found = fbe_xor_is_data_pattern_present(sector_p);
    if (b_data_pattern_found != FBE_TRUE)
    {
        return status;
    }

    /* Skip over the header (round up to next chunk)
     */
    start_chunk_index = ((sizeof(fbe_u64_t) * FBE_DATA_PATTERN_MAX_HEADER_PATTERN) + (bytes_per_chunk - 1)) / bytes_per_chunk;

    /* Compare the first 32 bytes to the rest of the 32 byte chunks in the sector 
     */
    for  (chunk_index = start_chunk_index + 1; chunk_index < chunks_per_block; chunk_index++)
    {
        fbe_u32_t base_word_index;
        fbe_u32_t check_word_index;
        fbe_u32_t word_index;

        check_word_index = chunk_index * words_per_chunk;
        base_word_index  = (chunk_index - 1) * words_per_chunk;

        /* We validate that all the words in each chunk contain the same data
         */
        for (word_index = 0; word_index < words_per_chunk; word_index++)
        {
            if (sector_p->data_word[check_word_index] != sector_p->data_word[base_word_index])
            {
                fbe_base_library_trace(FBE_LIBRARY_ID_XOR, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                       "xor: mismatch lba: 0x%llx at word: %d/%d 0x%08x != 0x%08x \n",
                                       (unsigned long long)seed,
				       check_word_index, base_word_index,
                                       sector_p->data_word[check_word_index],
                                       sector_p->data_word[base_word_index]);

                /* Next display the sector.
                 */
                status = fbe_xor_lib_sector_trace_raid_sector(sector_p);
                if (status != FBE_STATUS_OK)
                {
                    return status;
                }
                
                /* Return an error
                 */
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Goto next word in chunk
             */
            check_word_index++;
            base_word_index++;
        }
    }
    return FBE_STATUS_OK;
}
/***************************************************
 * end fbe_xor_check_sector_for_repeating_pattern()
 ***************************************************/

/*!**************************************************************
 *          fbe_xor_validate_data()
 ****************************************************************
 * @brief
 *  Interface function that validates that if the sector has been
 *  invalidated the checksum better be invalid.
 *
 * @param   sector_p - sector to validate
 * @param   seed - lba of sector to validate
 * @param   option - Xor options
 *
 * @return  status - Typically FBE_STATUS_OK
 *                   Otherwise the failure status 
 *  
 * @author
 *  08/03/2011  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_status_t fbe_xor_validate_data(const fbe_sector_t *const sector_p,
                                   const fbe_lba_t seed,
                                   const fbe_xor_option_t option)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /* Invoke xorlib method to validate the sector.
     */
    if (xorlib_is_data_invalid_with_good_crc((void *)sector_p) != FBE_FALSE)
    {
        /* Save the sector the xor trace buffer
         */
        fbe_xor_sector_trace_sector(FBE_SECTOR_TRACE_ERROR_LEVEL_WARNING,
                                    FBE_SECTOR_TRACE_ERROR_FLAG_RINV,
                                    sector_p);
        fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_CRITICAL,
                              "xor: seed: 0x%llx sector buffer: 0x%p was invalidated but has correct crc.\n",
                              (unsigned long long)seed, sector_p);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    /* If enabled check the data pattern
     */
    if ((status == FBE_STATUS_OK)                    &&
        (option & FBE_XOR_OPTION_CHECK_DATA_PATTERN)    )
    {
        fbe_u16_t   chksum;

        /* If the sector has a good checksum check for the repeating data
         * pattern.
         */
        chksum = fbe_xor_lib_calculate_checksum((const fbe_u32_t *)sector_p);
        if (sector_p->crc == chksum)
        {
            /* Check the data pattern.
             */
            status = fbe_xor_check_sector_for_repeating_pattern(sector_p, seed);
            if (status != FBE_STATUS_OK)
            {
                fbe_xor_library_trace(FBE_XOR_LIBRARY_TRACE_PARAMS_CRITICAL,
                                      "xor: seed: 0x%llx sector buffer: 0x%p data pattern check failed.\n",
                                      (unsigned long long)seed, sector_p);
            }
        }
    }

    return status;
}
/******************************************
 * end fbe_xor_validate_data()
 ******************************************/

/*!**************************************************************
 *          fbe_xor_is_sector_invalidated()
 ****************************************************************
 * @brief
 *  Interface function checks if the sector is invalidated or not
 *
 * @param   sector_p - sector to validate
 * @param   reason_p - If the sector was invalidated, this is the
 *                     invalidated reason.
 *
 * @return  bool 
 *  
 * @author
 *  08/017/2011  Ron Proulx  - Created
 *
 ****************************************************************/
fbe_bool_t fbe_xor_is_sector_invalidated(const fbe_sector_t *const sector_p,
                                         xorlib_sector_invalid_reason_t *const reason_p)
{
    /* Initialize hte reaosn to `invalid'
     */
    *reason_p = XORLIB_SECTOR_INVALID_REASON_UNKNOWN;
        
    /* We can't check the lba seed at this level for an invalidate.
     */
     if (xorlib_is_sector_invalidated(sector_p, 
                                      reason_p,
                                      FBE_LBA_INVALID,
                                      FBE_FALSE) == FBE_TRUE)
     {
         /* Sector was invalidated, return true
          */
         return FBE_TRUE;
     }

     /* Else it wasn't invalidated
      */
     return FBE_FALSE;
}
/******************************************
 * end of fbe_xor_is_sector_invalidated()
 ******************************************/


/****************************************
 * end xor_check_sector.c
 ****************************************/

