/***************************************************************************
 * Copyright (C) EMC Corporation 1999-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
/***************************************************************************
 * fbe_raid_check_sector.c
 ***************************************************************************
 *
 * @brief
 *  This file contains functions that check various conditions of a given
 *  sector.
 *
 * @author
 *  10/29/2009 - Created. Rob Foley
 *
 *
 ***************************************************************************/

/*************************
 **   INCLUDE FILES
 *************************/
#include "fbe_raid_library.h"
#include "fbe_raid_library_proto.h"
#include "fbe_raid_library_private.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe/fbe_data_pattern.h"
#include "fbe/fbe_xor_api.h"

/****************************
 *  GLOBAL VARIABLES
 ****************************/
fbe_status_t fbe_raid_check_fruts_sectors(fbe_raid_siots_t* const siots_p,
                                          fbe_raid_fruts_t* const fruts_p);

static fbe_status_t fbe_raid_check_data(fbe_sg_element_t *sg_p,
                                        fbe_lba_t seed,
                                        fbe_u32_t blkcnt,
                                        fbe_raid_option_flags_t raid_option_flags,
                                        fbe_payload_block_operation_opcode_t opcode);

 /****************************************************************
 * fbe_raid_fruts_check_sectors()
 ****************************************************************
 * @brief:
 *  Determine if any of the blocks we are writing out are invalid.
 *  This function returns if we don't need to check anything.  The only side effect is to
 *   panic if we find we are writing an invalid sector.
 *
 * @param fruts_p - head of a list of fruts.
 *
 * @return fbe_status_t
 *
 ****************************************************************/

fbe_status_t fbe_raid_fruts_check_sectors( fbe_raid_fruts_t *fruts_p )
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_siots_t *siots_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_sg_element_t *sg_p = NULL;
    fbe_raid_fruts_get_sg_ptr(fruts_p, &sg_p);
    
    siots_p = fbe_raid_fruts_get_siots(fruts_p);
    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    
    /* Determine if we should stop due to invalid sectors.
     */
    if (fbe_raid_is_option_flag_set(raid_geometry_p, FBE_RAID_OPTION_FLAG_DATA_CHECKING) &&
        (fruts_p->opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE))
    {
        //fbe_raid_fruts_check_data_pattern(fruts_p);
    }

    if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_WRITE_SECTOR_CHECKING))
    {
        /* Make sure the stamps are consistent.
         */
        status = fbe_raid_check_fruts_sectors(fbe_raid_fruts_get_siots(fruts_p), fruts_p);
        if (RAID_FRUTS_COND(status != FBE_STATUS_OK) )
        {
            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                 "raid: %s errored: stamps are not consistent:  siots = 0x%p, "
                                 "fruts = 0x%p, status = 0x%x\n",
                                 __FUNCTION__,
                                 siots_p, 
                                 fruts_p,
                                 status);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    return status;
}
/****************************************
 * fbe_raid_fruts_check_sectors()
 ****************************************/

/*!***************************************************************************
 *          fbe_raid_is_data_pattern_present()
 *****************************************************************************
 *
 * @brief   This method attempts to determine if there is any repeating pattern
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
static fbe_bool_t fbe_raid_is_data_pattern_present(const fbe_sector_t *const sector_p)
{
    fbe_bool_t  b_data_pattern_present = FBE_FALSE;
    fbe_u32_t   num_of_pieces_in_sector = 6;
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
 * end of fbe_raid_is_data_pattern_present()
 ******************************************/
/*!**************************************************************
 * fbe_raid_check_sector_for_repeating_pattern()
 ****************************************************************
 * @brief
 *  Check that the first 32 bytes of the sector match
 *  the other chunks of 32 bytes in the sector.
 *
 * @param sector_p - Ptr to sector to check.
 * @param seed - unused.
 *
 * @return fbe_status_t   
 *
 * @author
 *  1/20/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_check_sector_for_repeating_pattern(fbe_sector_t * sector_p,
                                                         fbe_lba_t seed)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       chunk_index;
    fbe_u32_t       bytes_per_chunk = 32;
    fbe_u32_t       words_per_chunk = bytes_per_chunk / sizeof(fbe_u32_t);
    fbe_u32_t       chunks_per_block = FBE_BYTES_PER_BLOCK / bytes_per_chunk;
    fbe_u32_t       start_chunk_index = 0; 

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
                fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                       "raid: mismatch at word: %d/%d 0x%08x != 0x%08x \n",
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
 * end fbe_raid_check_sector_for_repeating_pattern()
 ***************************************************/

/************************************************************
 *  fbe_raid_check_fruts_sectors()
 ************************************************************
 *
 * @brief
 *  Check the write stamps. Panic if a inconsistency is found.
 *  If enabled check the data also.
 *
 *  siots_p: [I] - pointer to a siots struct to check.
 *  fruts_p: [I] - pointer to a fruts struct to check.
 *
 * @return
 *  void
 *
 * @author
 *  08/10/06:  SL -- Created.
 *
 ************************************************************/

fbe_status_t fbe_raid_check_fruts_sectors(fbe_raid_siots_t* const siots_p,
                                         fbe_raid_fruts_t* const fruts_p)
{
    fbe_status_t            status = FBE_STATUS_OK;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    fbe_u32_t width = fbe_raid_siots_get_width(siots_p);
    fbe_sg_element_t* first_sg_p;
    fbe_sg_element_t* current_sg_p;
    fbe_u16_t bitmap_parity;
    fbe_block_count_t total_blocks = fruts_p->blocks;
    fbe_lba_t current_lba = fruts_p->lba;
    fbe_lba_t               journal_log_start_lba;
    fbe_u32_t       journal_log_hdr_blocks;

    /* Do not check if error injection is enabled.
     */
    if (fbe_raid_siots_is_validation_enabled(siots_p)) {
        return FBE_STATUS_OK;
    }

    /* Retrive the journal log start pba
     */
    fbe_raid_geometry_get_journal_log_start_lba(raid_geometry_p, &journal_log_start_lba); 
    fbe_raid_geometry_get_journal_log_hdr_blocks(raid_geometry_p, &journal_log_hdr_blocks); 

    /* Get the pointer to the SG elements. 
     * Save the first SG element pointer for debugging purposes. 
     */
    fbe_raid_fruts_get_sg_ptr(fruts_p, &first_sg_p);
    current_sg_p = first_sg_p;

    /* Only check the write stamps if the fruts is a write operation 
     * on normal data drives. 
     */
    if ((current_sg_p == NULL) || 
        ((fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE) && 
         (fruts_p->opcode != FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY)    )    )
    {
        return FBE_STATUS_OK;
    }

    /* Verify:
     * 1) The SG element pointer is not NULL.
     * 2) The SG element address is not NULL. 
     * 3) The SG element address is not a tokenized address.
     * 4) The SG size is a multiple of BE block size.
     */
    if(RAID_FRUTS_COND( (current_sg_p == NULL) ||
                  (current_sg_p->address == NULL) ||
                  ((current_sg_p->count % FBE_BE_BYTES_PER_BLOCK) != 0) ) )
    {
        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                             "raid: %s errored: current_sg_p is NULL or current_sg_p->address is NULL "
                             "or current_sg_p->count is not correct\n",
                             __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set up the physical parity bitmap. 
     */
    bitmap_parity = 1 << fbe_raid_siots_get_row_parity_pos(siots_p);
    if (fbe_raid_geometry_is_raid6(raid_geometry_p))
    {
        bitmap_parity |= 1 << fbe_raid_siots_dp_pos(siots_p);
    }

    /* Loop through all SG elements in this list. 
     */
    while ((total_blocks > 0) && (current_sg_p->count != 0))
    {
         /* Get the physical or virtual address of the SG elements.
         */ 
        fbe_u32_t blocks = current_sg_p->count / FBE_BE_BYTES_PER_BLOCK;
        fbe_sector_t* sector_p = (fbe_sector_t*)fbe_sg_element_address(current_sg_p);

        /* Loop over the number of blocks of this SG element.
         */
        while ((blocks > 0) && (total_blocks > 0) && (current_sg_p->count != 0))
        {
            /* Map in the sectors here iff the sectors are currently physical 
             * addresses.
             */
            const fbe_u16_t crc = fbe_xor_lib_calculate_checksum(sector_p->data_word);

            /* Check RAID-0 raid groups.
             */
            if (fbe_raid_geometry_is_raid0(raid_geometry_p))
            {
                status = fbe_xor_lib_validate_raid0_stamps(sector_p, current_lba, fbe_raid_siots_get_eboard_offset(siots_p));

                /* We will not panic. We will print critical error.
                 */
                if (status != FBE_STATUS_OK)
                {
                    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_CRITICAL,
                                                "RAID: status 0x%x of fbe_xor_validate_raid0_stamps() is not valid\n", 
                                                status);
                }
            }
            else if (fbe_raid_geometry_is_raw_mirror(raid_geometry_p))
            {
                /* We will not panic. We will print critical error.
                 */
                status = fbe_xor_lib_validate_raw_mirror_stamps(sector_p, current_lba, fbe_raid_siots_get_eboard_offset(siots_p));
                if (status != FBE_STATUS_OK)
                {
                    fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_CRITICAL,
                                                "RAID: status 0x%x of fbe_xor_lib_validate_raw_mirror_stamps() is not valid\n", 
                                                status);
                }
            }
            else if (fbe_raid_geometry_is_mirror_type(raid_geometry_p))
            {
                /* If the mirror is in `sparing' mode it does not `own' the data.
                 * this means we cannot validate the stamps.
                 */
                if (fbe_raid_geometry_is_sparing_type(raid_geometry_p) == FBE_FALSE)
                {
                    /* We will not panic. We will print critical error.
                     */
                    status = fbe_xor_lib_validate_mirror_stamps(sector_p, current_lba, fbe_raid_siots_get_eboard_offset(siots_p));
                    if (status != FBE_STATUS_OK)
                    {
                        fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_CRITICAL,
                                                "RAID: status 0x%x of fbe_xor_lib_validate_mirror_stamps() is not valid\n", 
                                                status);
                    }
                }
            }
            else if ((sector_p->write_stamp &  bitmap_parity) != 0 || 
                     (sector_p->write_stamp >> width)         != 0)
            {
                /* The time stamp is invalid if either the parity bit(s) 
                 * or the non-existent FRU position(s) are set. 
                 */
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "%s: write stamp is invalid!\n", __FUNCTION__);

                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "%s: ws = 0x%x, parity = 0x%x, width = %d\n",
                                            __FUNCTION__, sector_p->write_stamp, bitmap_parity, width);

            }
            /*! We do not allow parity to be invalidated on r6.
             */

            else if (fbe_raid_geometry_is_raid6(raid_geometry_p) && 
                     fbe_raid_siots_is_parity_pos(siots_p, fruts_p->position) &&
                     sector_p->crc != crc)
            {
                /* Check the checksums on RAID 6 parity blocks only 
                 * as they should always be consistent.
                 */
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR,
                                            "RAID: %s R6 parity checksum is invalid!\n", __FUNCTION__);
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_CRITICAL,
                                            "RAID: %s crc(drive) = 0x%x, crc(computed) = 0x%x\n",
                                            __FUNCTION__, sector_p->crc, crc);
            }

            /*! @note if we get rid of the raid 6 poc, we can get rid of this raid 6  
             * check.  
             */
            else if (!fbe_raid_geometry_is_raid6(raid_geometry_p) &&
                     fbe_raid_geometry_is_parity_type(raid_geometry_p) &&
                     fbe_raid_siots_is_parity_pos(siots_p, fruts_p->position) &&
                     (sector_p->lba_stamp != 0) &&
                     (current_lba < journal_log_start_lba))
            {
                /* Check the checksums on RAID 6 parity blocks only 
                 * as they should always be consistent. 
                 * Journal writes will have lba stamps on the headers, so do not perform this check on journal writes. 
                 */
                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_CRITICAL,
                                            "RAID: R6 parity shed stamp is set to 0x%x!\n", sector_p->lba_stamp);
            }
            else if ( fbe_raid_geometry_is_parity_type(raid_geometry_p) &&
                      !fbe_raid_geometry_is_raid6(raid_geometry_p) && 
                      (current_lba < journal_log_start_lba))
            {
                fbe_u16_t current_bitmask = (1 << fruts_p->position);

                /* Raid 5/3 do not use a 0 time stamp.
                 */
                if(RAID_FRUTS_COND( (sector_p->time_stamp == 0) && 
                                    !fbe_raid_geometry_position_needs_alignment(raid_geometry_p, fruts_p->position) ) )
                {
                    fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                         "raid: %s errored: sector_p->time_stamp 0x%x is zero\n",
                                        __FUNCTION__, sector_p->time_stamp);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                /* On parity units, fbe_panic if we try to write out
                 * all timestamps on a data position.
                 */
                if (fruts_p->position == siots_p->parity_pos)
                {
                    /* Write stamp should not be set for parity position.
                     */
                    if(RAID_FRUTS_COND( (sector_p->write_stamp & current_bitmask) != 0) )
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                             "raid: %s errored: sector_p->write_stamp 0x%x & current_bitmask 0x%x is nonzero\n",
                                             __FUNCTION__, sector_p->write_stamp, current_bitmask);
                        return FBE_STATUS_GENERIC_FAILURE;
                    }
                }
                else
                {
                    /* If any writestamp set, should be for this position.
                     */
                    if(RAID_FRUTS_COND( sector_p->write_stamp != current_bitmask &&
                                sector_p->write_stamp != 0 ))
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                             "raid: %s errored: sector_p->write_stamp 0x%x & current_bitmask 0x%x is nonzero\n",
                                             __FUNCTION__, sector_p->write_stamp, current_bitmask);
                        return FBE_STATUS_GENERIC_FAILURE;
                    }

                    if(RAID_FRUTS_COND( sector_p->write_stamp != 0 && 
                                sector_p->time_stamp != FBE_SECTOR_INVALID_TSTAMP) )
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                            "raid: %s errored: sector_p->write_stamp 0x%x nonzero and sector_p->time_stamp 0x%x is valid\n",
                                             __FUNCTION__, sector_p->write_stamp, sector_p->time_stamp);
                        return FBE_STATUS_GENERIC_FAILURE;
                    }

                    if(RAID_FRUTS_COND( (sector_p->time_stamp & FBE_SECTOR_ALL_TSTAMPS) != 0) )
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                             "raid: %s errored: sector_p->time_stamp 0x%x & FBE_SECTOR_ALL_TSTAMPS is nonzero\n",
                                             __FUNCTION__, sector_p->time_stamp);
                        return FBE_STATUS_GENERIC_FAILURE;
                    }

                    if (sector_p->time_stamp != FBE_SECTOR_INVALID_TSTAMP)
                    {
                        if(RAID_FRUTS_COND(sector_p->write_stamp != 0) )
                        {
                            fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                                "raid: %s errored: sector_p->write_stamp 0x%x is nonzero\n",
                                                __FUNCTION__, sector_p->write_stamp);
                            return FBE_STATUS_GENERIC_FAILURE;
                        }
                    }
                }
            }/* end if r5 or r3 */
            if (fbe_raid_geometry_is_parity_type(raid_geometry_p) && 
                !(fbe_raid_siots_is_parity_pos(siots_p, fruts_p->position)))
            {
                if (sector_p->time_stamp != FBE_SECTOR_INVALID_TSTAMP)
                {
                    if(RAID_FRUTS_COND(sector_p->write_stamp != 0) )
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_ERROR, 
                                            "raid: %s errored: sector_p->write_stamp 0x%x nonzero\n",
                                            __FUNCTION__, sector_p->write_stamp);
                        return FBE_STATUS_GENERIC_FAILURE;
                    }
                }

                /*! @note Currently the lba stamps for sectors in the journal
                 *        do not have the correct lba stamp.  Therefore if this
                 *        request is to the journal area or the crc is incorrect
                 *        do not validate the lba stamp.
                 */
                if ((sector_p->crc == crc)                &&
                    ((current_lba < journal_log_start_lba) || (current_lba < (journal_log_start_lba + journal_log_hdr_blocks)) ))
                {
                    if (RAID_FRUTS_COND(!fbe_xor_lib_is_lba_stamp_valid(sector_p, current_lba, fbe_raid_siots_get_eboard_offset(siots_p))))
                    {
                        fbe_raid_siots_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_CRITICAL, 
                                             "raid: %s errored: sector_p->lba_stamp 0x%x bad\n",
                                             __FUNCTION__, sector_p->lba_stamp);
                        return FBE_STATUS_GENERIC_FAILURE;
                    }
                }
            } 

            /* If data pattern checking is enabled and this isn't a metadata I/O 
             * validate the data being written.
             */
            if (fbe_raid_geometry_is_debug_flag_set(raid_geometry_p, FBE_RAID_LIBRARY_DEBUG_FLAG_PATTERN_DATA_CHECKING))
            {
                /* The crc must be correct and not a metadata request.
                 */
                if ( (sector_p->crc == crc)                        &&
                    !fbe_raid_siots_is_metadata_operation(siots_p)    )
                {
                    /* Validate that there is a data pattern.  We cannot use
                     * fbe_raid_find_lba_pattern_in_data() since the lba in the
                     * fruts is physical not logical and therefore isn't written
                     * in the data.
                     */
                    if (fbe_raid_is_data_pattern_present(sector_p))
                    {
                        /* We want to check parity also but only if there are no 
                         * invalidated blocks (since that pattern isn't repeating).
                         */
                        if ( fbe_raid_geometry_is_parity_type(raid_geometry_p)        &&
                             fbe_raid_siots_is_parity_pos(siots_p, fruts_p->position)    )
                        {
                            /* Cannot check this parity position
                             */
                        }
                        else
                        {
                            /* Else either a parity without invalids or a data position.
                             */
                            status = fbe_raid_check_sector_for_repeating_pattern(sector_p, current_lba);
                            if (status != FBE_STATUS_OK) 
                            { 
                                fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_CRITICAL,
                                                            "raid: Failed checking for data pattern on lba: 0x%llx\n", 
                                                            (unsigned long long)current_lba);
                                return status; 
                            }
                        }
                    } /* end if there is a data pattern present*/
                } /* end if this is a non-metadata request */

            } /* end if data pattern trace is enabled*/

            /* Advance to the next block. 
             */
            current_lba++;
            --blocks;
            total_blocks--;
            sector_p++;
        } /* while (block > 0) */

        /* Advance to the next SG element. 
         */
        ++current_sg_p;
    } /* end of while (current_sg_p->count != 0) */

    return status;
}
/* end fbe_raid_check_fruts_sectors() */


static fbe_u32_t fbe_word_check_mask = 0xFFFFFFFF;
static fbe_u32_t fbe_check_word_increment = 1;
/*!**************************************************************
 * fbe_raid_find_lba_pattern_in_data()
 ****************************************************************
 * @brief
 *  Look for a specific pattern in the sector.
 *
 * @param  seed - lba to search for.
 * @param ref_sect_p - Sector ptr to search.
 *
 * @return fbe_bool_t - FBE_TRUE on success, FBE_FALSE on failure.   
 *
 * @author
 *  1/20/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_bool_t fbe_raid_find_lba_pattern_in_data(fbe_lba_t seed, fbe_sector_t * const sector_p)
{
    fbe_u32_t word_index;
    fbe_u32_t check_word;
#define BYTE_SWAP_32(x)                                  \
	(((x) >> 24)                |                        \
	 (((x) & 0x00ff0000) >>  8) |                        \
	 (((x) & 0x0000ff00) <<  8) |                        \
	 ((x) << 24))

    if (seed > FBE_U32_MAX)
    {
        return FALSE;
    }
    check_word = (fbe_u32_t)seed;

    /* Find first word with lba match.
     */
    for (word_index = 5;
         word_index < 8;//FBE_BYTES_PER_BLOCK/sizeof(fbe_u32_t);
         word_index += fbe_check_word_increment)
    {
        /* First check for non-swapped.
         */
        if ((sector_p->data_word[word_index] & fbe_word_check_mask) == check_word)
        {
            return FBE_TRUE;
        }
        /* Next check for byte swapped.
         */
        if ((sector_p->data_word[word_index] & fbe_word_check_mask) == 
            BYTE_SWAP_32(check_word))
        {
            /* Mark the fact that we found a byte swapped pattern. 
             */
            //fbe_check_byte_swap = TRUE;
            return FBE_TRUE;
        }
    }/* end for all words in block to check */

    return FBE_FALSE;
}
/******************************************
 * end fbe_raid_find_lba_pattern_in_data()
 ******************************************/
/*!**************************************************************
 * fbe_raid_check_data()
 ****************************************************************
 * @brief
 *  Check the input sg list sectors for a pattern.
 *
 * @param sg_p - Scatter gather list to iterate over to check for a pattern.
 * @param lba - lba seeded in the pattern.
 * @param blkcnt - Number of blocks to check.
 * @param raid_option_flags - The raid option flags
 * @param opcode - The block operation
 * 
 * @return fbe_status_t
 *
 * @author
 *  1/20/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_check_data(fbe_sg_element_t *sg_p,
                                 fbe_lba_t lba,
                                 fbe_u32_t blocks,
                                 fbe_raid_option_flags_t raid_option_flags,
                                 fbe_payload_block_operation_opcode_t opcode)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_bool_t  b_sector_invalidated = FBE_FALSE;
    xorlib_sector_invalid_reason_t invalidate_reason = XORLIB_SECTOR_INVALID_REASON_UNKNOWN;

    /* Loop through all the blocks in our transfer.
     */
    while (blocks > 0)
    {
        fbe_u8_t *byte_p = NULL;
        fbe_u16_t current_sg_blocks;

        current_sg_blocks = FBE_MIN(blocks, sg_p->count / FBE_BE_BYTES_PER_BLOCK);

        blocks -= current_sg_blocks,
        byte_p = sg_p->address;

        /* Loop through all the blocks 
         * in this SG element.
         */
        do
        {
            fbe_sector_t *blkptr = (fbe_sector_t *) byte_p;

            /* Validate that we are not attempting to transfer a sector that
             * has been invalidated (i.e. data is the invalidated pattern)
             * but the crc is good.
             */
            status = fbe_xor_lib_validate_data(blkptr, lba, FBE_XOR_OPTION_VALIDATE_DATA /* Just validate*/);
            if (status != FBE_STATUS_OK)
            {
                /*! @note This is not expected.  Log the error and return.
                 */
                fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_CRITICAL,
                                       "raid: Check data found lba: 0x%llx was invalidated with good crc!\n",
                                       (unsigned long long)lba);
                return status; 
            }

            /* If this is a read and the `fail data lost - invalidated' flag is
             * set in the raid option flags and this sector is the `data lost -
             * invalidated' pattern, log an error and fail the siots.
             */
            if ((raid_option_flags & FBE_RAID_OPTION_FLAG_FAIL_DATA_LOST_READ) &&
                (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)               )
            {
                /* If the sector was invalidated and the reason is `data lost'
                 * generate the proper trace and change the status to `fail'.
                */
                b_sector_invalidated = fbe_xor_lib_is_sector_invalidated(blkptr, &invalidate_reason);
                if ((b_sector_invalidated == FBE_TRUE)                            &&
                    (invalidate_reason == XORLIB_SECTOR_INVALID_REASON_DATA_LOST)    )
                {
                    /* Trace an error and report an error
                     */
                    fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                           "raid: Check data found lba: 0x%llx `data lost' in data read.\n",
                                           (unsigned long long)lba);
                    return FBE_STATUS_GENERIC_FAILURE; 
                }
            }
  
            /* If this is a write and the `fail data lost - invalidated' flag is
             * set in the raid option flags and this sector is the `data lost -
             * invalidated' pattern, log an error and fail the siots.
             */
            if ((raid_option_flags & FBE_RAID_OPTION_FLAG_FAIL_DATA_LOST_WRITE) &&
                (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)               )
            {
                /* If the sector was invalidated and the reason is `data lost'
                 * generate the proper trace and change the status to `fail'.
                */
                b_sector_invalidated = fbe_xor_lib_is_sector_invalidated(blkptr, &invalidate_reason);
                if ((b_sector_invalidated == FBE_TRUE)                            &&
                    (invalidate_reason == XORLIB_SECTOR_INVALID_REASON_DATA_LOST)    )
                {
                    /* Trace an error and report an error
                             */
                    fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_ERROR,
                                           "raid: Check data found lba: 0x%llx `data lost' in write data.\n",
                                           (unsigned long long)lba);
                    return FBE_STATUS_GENERIC_FAILURE; 
                }
            }    
                  
            /* If the sector doesn't have a repeating pattern return success.
             */
            if (!fbe_raid_find_lba_pattern_in_data(lba, blkptr))
            {
                return FBE_STATUS_OK;
            }

            /* Make sure this sector matches our seeded pattern.
             */
            status = fbe_raid_check_sector_for_repeating_pattern(blkptr, lba);
            if (status != FBE_STATUS_OK) 
            { 
                fbe_raid_library_trace(FBE_RAID_LIBRARY_TRACE_PARAMS_WARNING,
                                       "raid: Failed checking for data pattern on lba: %llx\n", (unsigned long long)lba);
                return status; 
            }
            byte_p += FBE_BE_BYTES_PER_BLOCK,
            lba++;
        }
        while (0 < --current_sg_blocks);

        /* Increment to the next SG element in this SG list.
         */
        sg_p++;
    }
    return status;
}
/******************************************
 * end fbe_raid_check_data()
 ******************************************/

/*!*******************************************************************
 * @var fbe_raid_data_checking_min_lba
 *********************************************************************
 * @brief This is the LBA where we will begin checking sectors.
 *        We offset a bit to avoid the OS header for each LUN.
 *        Experience has told that this is a reasonable offset.
 *
 *********************************************************************/
static fbe_u32_t fbe_raid_data_checking_min_lba = 1024;

 /****************************************************************
 * fbe_raid_siots_check_data_pattern()
 ****************************************************************
 * @brief:
 *  Check the blocks for a pattern and trace if the pattern does
 *  not match.
 *
 * @param siots_p - Current I/O.
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t fbe_raid_siots_check_data_pattern( fbe_raid_siots_t *siots_p )
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_raid_iots_t    *iots_p = NULL;
    fbe_payload_block_operation_opcode_t opcode;
    fbe_sg_element_t *sg_p = NULL;
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_raid_option_flags_t raid_option_flags = FBE_RAID_OPTION_FLAG_INVALID;

    fbe_raid_siots_get_lba(siots_p, &lba);
    fbe_raid_siots_get_blocks(siots_p, &blocks);
    fbe_raid_siots_get_sg_ptr(siots_p, &sg_p);
    raid_geometry_p = fbe_raid_siots_get_raid_geometry(siots_p);
    iots_p = fbe_raid_siots_get_iots(siots_p);
    fbe_raid_iots_get_opcode(iots_p, &opcode);

    /* Get the raid library `option' flags
     */
    raid_option_flags = fbe_raid_get_option_flags(raid_geometry_p);

    /* Determine if we should check the contents of the sector.
     * Only check if this is not a metadata lba since 
     * metadata I/Os do not have a pattern.
     */
    if ( (sg_p != NULL) &&
         !fbe_raid_siots_is_metadata_operation(siots_p) &&
         (lba > fbe_raid_data_checking_min_lba))
    {
        fbe_status_t check_data_status;
        fbe_payload_block_operation_opcode_t opcode;
        fbe_raid_siots_get_opcode(siots_p, &opcode);

        if(blocks > FBE_U32_MAX)
        {
            /* blocks here should not cross the 32bit limit
             */
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_CRITICAL,
                                        "raid: siots: 0x%p: blocks crossing 32bit limit : 0x%llx\n", 
                                        siots_p, (unsigned long long)blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        check_data_status = fbe_raid_check_data(sg_p, lba, (fbe_u32_t)blocks,
                                                raid_option_flags,
                                                opcode);



        if (check_data_status != FBE_STATUS_OK)
        {
            fbe_raid_siots_object_trace(siots_p, FBE_RAID_SIOTS_TRACE_PARAMS_CRITICAL,
                                        "raid: siots: 0x%p op: 0x%x lba: 0x%llx bl: 0x%llx data check failed\n",
                                        siots_p, opcode,
					(unsigned long long)lba,
					(unsigned long long)blocks);
        }
    }
    return status;
}
/****************************************
 * fbe_raid_siots_check_data_pattern()
 ****************************************/
/*************************
 * end file fbe_raid_check_sector.c
 *************************/
