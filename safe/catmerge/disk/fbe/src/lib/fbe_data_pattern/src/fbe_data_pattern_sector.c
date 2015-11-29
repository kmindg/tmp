/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_data_pattern_sector.c
 ***************************************************************************
 *
 * @brief
 *  This file contains data pattern sector related functions.
 *
 * @version
 *   8/30/2010:  Created. Swati Fursule
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_data_pattern.h"
#include "fbe/fbe_xor_api.h"
#include "xorlib_api.h"
#include "fbe/fbe_raid_group.h"
#include "fbe/fbe_random.h"
/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/****************************************************************
 *  fbe_data_pattern_fill_sector
 ****************************************************************
 * @brief
 *  Generate or check seed for a given contiguous range
 *  of sectors.
 *  If we are checking and we find a mismatch, panic.
 *  This functionality is separated out from rdgen sglist . 
 *  implementation. Since now the user of this functionality are 
 *  FBECLI rdt, fbe_test along with rdgen service.
 *  
 * @param sectors - Contiguous sectors to check or generate.
 * @param data_pattern_info_p - data pattern information struct
 * @param memory_ptr - Start of memory range.
 * @param b_append_checksum - If FBE_TRUE, append checksum.
 * @param block_size - Size of the block to check.
 * @param b_is_virtual - indicates if the memory_ptr is virtual memory.
 *
 * RETURNS:
 *  FBE_TRUE on success, FBE_FALSE on failure 
 *
 * HISTORY:
 *  8/29/2010 - Created. Swati Fursule
 *
 ****************************************************************/

fbe_bool_t fbe_data_pattern_fill_sector(fbe_u32_t sectors,
                                        fbe_data_pattern_info_t* data_pattern_info_p,
                                        fbe_u32_t * memory_ptr,
                                        fbe_bool_t b_append_checksum,
                                        fbe_block_size_t block_size,
                                        const fbe_bool_t b_is_virtual)
{
    fbe_bool_t b_status = FBE_TRUE;
    fbe_u32_t sequence_id = data_pattern_info_p->sequence_id;
    fbe_lba_t seed = data_pattern_info_p->seed;
    fbe_u32_t pattern_ls17 = sequence_id<<17;
    fbe_sector_t *sector_p = NULL;
    fbe_u32_t header_index = 0;
    
    /* Use the highest 16 bits as pass count. We need an extra shift
     * here to achieve 48 bit shift (17 + 31 = 48).
     */
    fbe_lba_t seeded_pattern = Int64ShllMod32(pattern_ls17,31) | seed;
    fbe_u32_t meta_count = 1;     /* meta data is one 64 bit 'word' */   
    fbe_lba_t current_pattern = 0;
    fbe_lba_t current_pattern_ls = 0;
    fbe_lba_t current_pattern_rs = 0;
    fbe_lba_t current_pattern_seed_ls = 0;
    void * mapped_memory_ptr = NULL;
    fbe_u32_t mapped_memory_cnt = 0;
    void * reference_ptr = NULL;
    fbe_lba_t bits_to_shift = 0;


    /* MAP IN SECTOR HERE */
    mapped_memory_cnt = (sectors * block_size);
    mapped_memory_ptr = (b_is_virtual)?
                  (void *)memory_ptr:
                  FBE_DATA_PATTERN_MAP_MEMORY(((fbe_u32_t *)memory_ptr), mapped_memory_cnt);

    if(mapped_memory_ptr == NULL)
    {
        fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Mapped memory ptr is NULL. \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    reference_ptr = mapped_memory_ptr;
    sector_p = (fbe_sector_t *)reference_ptr;

    while (sectors--)
    {
        /* Set word count to only touch data portion of sector.
         */
        fbe_u32_t word_count;

        if (block_size == FBE_BYTES_PER_BLOCK)
        {
            word_count = (block_size / sizeof(fbe_lba_t));
        }
        else
        {
            word_count = (block_size / sizeof(fbe_lba_t)) - meta_count;
        }
        /*  Loop through every word in a sector
         *  putting in seed.
         */
        /* reset header index
         */
        header_index = 0;
        while (word_count)
        {            
            if (header_index < data_pattern_info_p->num_header_words) 
            {
                current_pattern = data_pattern_info_p->header_array[header_index];
                header_index++;
            }
            else if (header_index == data_pattern_info_p->num_header_words) {
                /* Use a unique word per sector that combines the seed and pass count. 
                 * This ensures that each sector has a unique checksum. 
                 * It also ensures that the parity is different from one pass to another. 
                 * This means that incomplete writes will be guaranteed to have different parity 
                 * since pass count is built into this pattern. 
                 */
                bits_to_shift = (seed + 1) / FBE_RAID_SECTORS_PER_ELEMENT; /* Get element number. */
                bits_to_shift %= 16;
                
                current_pattern_ls = seeded_pattern << (16 - bits_to_shift);
                current_pattern_rs = seeded_pattern >> (48 - bits_to_shift);
                current_pattern = current_pattern_ls | current_pattern_rs;
                current_pattern_seed_ls = seed << (seed % 7);
                current_pattern ^= current_pattern_seed_ls;
                header_index++;
            }
            else {
                current_pattern = seeded_pattern;
            }
#if 0 // Old logic for determining this fudge pattern.
            {
                /* Below logic picks one of the words beyond header words and mangles it by rotating.
                 * Idea is to pick a different word for different elements such that when row parity 
                 * is caluclated by XORing all the data elelemnts, data pattern does not cancel out
                 * resulting in Zero parity block.
                 * (eg: 0 XOR 80 XOR 100 XOR 180 = 0)
                 * Note: Element size is hard coded; So this solution might not work for large element 
                 * size RG which will still end up with Zeroed parity for some widths.
                 * - Vamsi V.   
                 */
                word_to_mangle = seed / FBE_RAID_SECTORS_PER_ELEMENT; /* Get element number. */
                word_to_mangle %= FBE_XOR_MAX_FRUS; /* Mod it to max possible disk positions. */
                word_to_mangle += FBE_XOR_MAX_FRUS; /* Avoid header words */ 

                if (word_count == word_to_mangle)
                {
                    /* mangle the 64-bit word */
                    current_pattern_ls = seeded_pattern << 13;
                    current_pattern_rs = seeded_pattern >> 51;
                    current_pattern = current_pattern_ls | current_pattern_rs;
                }
                else
                {
                    current_pattern = seeded_pattern;
                }
            }
#endif
            /* Insert generate pattern
             */
            *((fbe_lba_t*)reference_ptr) = current_pattern;

            reference_ptr = (fbe_lba_t *)reference_ptr + 1;
            word_count--;

        } /* end loop over all words in the sector. */

        if (block_size == FBE_BE_BYTES_PER_BLOCK)
        {
            if (b_append_checksum)
            {
                /* If we are appending checksums, then calculate and set
                 * the checksum for this block.
                 */
                *((fbe_u32_t*)reference_ptr) = fbe_xor_lib_calculate_checksum((fbe_u32_t*)sector_p);
                reference_ptr = (fbe_u32_t *)reference_ptr + 1;

                /* Inject a random lba stamp
                 */
                *((fbe_u32_t*)reference_ptr) = fbe_random() & 0xffff0000;
                reference_ptr = (fbe_u32_t *)reference_ptr + 1;
            }
            else
            {
                /* We are not checking checksum,
                 * blast away the meta data on parity units,
                 * Otherwise, insert zero stamps as the cache would.
                 */
                *((fbe_u32_t*)reference_ptr) = 0;
                reference_ptr = (fbe_u32_t *)reference_ptr + 1;

                *((fbe_u32_t*)reference_ptr) = 0;
                reference_ptr = (fbe_u32_t *)reference_ptr + 1;
            }
        }

        /* Increment the seed if there is a valid header word
         */
        if (data_pattern_info_p->num_header_words > 0)
        {
            /* Increment the seed
             */
            seed++;

            /* If we have exceeded the 48-bits allowed for the seed, fail the request.
             */
            if (seed > FBE_DATA_PATTERN_MAX_SEED_VALUE)
            {
                /* Report the error 
                 */
                fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                       "data pattern: seed: 0x%llx is larger that max supported: 0x%llx\n",
                                       (unsigned long long)seed,
				       (unsigned long long)FBE_DATA_PATTERN_MAX_SEED_VALUE);
                return FBE_FALSE;
            }

            /* Populated the 64-bit pattern.
             */
            seeded_pattern = Int64ShllMod32(pattern_ls17,31) | seed;
        }
        /* Goto the next sector.
         */
        sector_p++;
    }

    /* MAP OUT SECTOR HERE */
    if ( !b_is_virtual )
    {
        FBE_DATA_PATTERN_UNMAP_MEMORY(mapped_memory_ptr, mapped_memory_cnt);
    }

    return b_status;
}                               
/*******************************
 * fbe_data_pattern_fill_sector() 
 *******************************/

/*!**************************************************************
 * fbe_data_pattern_trace_sector()
 ****************************************************************
 * @brief
 *  display this sector to the trace buffer.
 *
 * @param sector_p - sector to display.              
 *
 * @return fbe_status_t   
 *
 * @author
 *  8/29/2010 - Created. Swati Fursule
 *
 ****************************************************************/

fbe_status_t fbe_data_pattern_trace_sector(const fbe_sector_t *const sector_p)
{
    fbe_u32_t word_index;
    fbe_u32_t calc_csum;

    /* Calculate the crc so we can display the difference between
     * the expected and actual crc.
     */
    calc_csum = fbe_xor_lib_calculate_checksum(sector_p->data_word);

    /* Display the checksum and calculated checksum.
     */
    if ( calc_csum == sector_p->crc)
    {
        fbe_data_pattern_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                "data pattern: Checksum OK    Actual csum: 0x%04x Calculated csum: 0x%04x\n",
                                sector_p->crc, calc_csum);
    }
    else
    {
        fbe_data_pattern_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                               "data pattern: CHECKSUM ERROR Actual csum: 0x%04x Calculated csum: 0x%04x\n",
                               sector_p->crc, calc_csum);
    }
    
    /* Trace out all the data words in the sector.
     */
    for ( word_index = 0; 
          word_index < (FBE_BYTES_PER_BLOCK / sizeof(fbe_u32_t)); 
          word_index += 4 )
    {
        fbe_data_pattern_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                               "DATA: %08X %08X %08X %08X \n",
                               sector_p->data_word[word_index + 0],
                               sector_p->data_word[word_index + 1],
                               sector_p->data_word[word_index + 2],
                               sector_p->data_word[word_index + 3] );
    }

    /* Trace out the metadata.
     */
    fbe_data_pattern_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "META:  ts:%04X crc:%04X lba/ss:%04X ws:%04X \n",
                            sector_p->time_stamp, sector_p->crc, 
                            sector_p->lba_stamp, sector_p->write_stamp);
    return FBE_STATUS_OK;
} 
/******************************************
 * end fbe_data_pattern_trace_sector()
 ******************************************/
/****************************************************************
 *  fbe_data_pattern_check_sector
 ****************************************************************
 * @brief
 *  Generate or check seed for a given contiguous range
 *  of sectors.
 *  If we are checking and we find a mismatch, panic.
 *  
 * @param sectors - Contiguous sectors to check or generate.
 * @params data_pattern_info_p, - Data pattern information structure.
 * @param memory_ptr - Start of memory range.
 * @param b_is_virtual - indicates if the memory_ptr is virtual memory.
 * @param block_size - Size of the block in bytes.
 * @param object_id - The object identifier for the request being checked
 * @param b_panic - FBE_TRUE to panic on data miscompare,   
 *                  FBE_FALSE to not panic on data miscompare.
 *
 * @return
 *  FBE_STATUS_OK or FBE_STATUS_GENERIC_FAILURE
 *
 * @author
 *   08/29/10 - Created - Swati Fursule
 *
 ****************************************************************/
fbe_status_t fbe_data_pattern_check_sector(fbe_u32_t sectors,
                                           fbe_data_pattern_info_t* data_pattern_info_p,
                                           fbe_u32_t * memory_ptr,
                                           const fbe_bool_t b_is_virtual,
                                           fbe_block_size_t block_size,
                                           fbe_object_id_t object_id,
                                           fbe_bool_t b_panic)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t sequence_id = data_pattern_info_p->sequence_id;
    fbe_lba_t seed = data_pattern_info_p->seed;
    fbe_u32_t pattern_ls17 = sequence_id<<17;
    fbe_sector_t *sector_p = NULL;
    fbe_u32_t header_index = 0;
    
    /* Use the highest 16 bits as pass count. We need an extra shift
     * here to achieve 48 bit shift (17 + 31 = 48).
     */
    fbe_lba_t seeded_pattern = Int64ShllMod32(pattern_ls17,31) | seed;
    fbe_u32_t meta_count = 1;     /* meta data is one 64 bit 'word' */  
    fbe_lba_t expected_pattern = 0;
    fbe_lba_t expected_pattern_ls = 0;
    fbe_lba_t expected_pattern_rs = 0;
    fbe_lba_t expected_pattern_seed_ls = 0;
    fbe_lba_t received_pattern = 0;
    void * mapped_memory_ptr = NULL;
    fbe_u32_t mapped_memory_cnt = 0;
    void * reference_ptr = NULL;
    fbe_u32_t miscompare_sector_count = 0;
    fbe_u64_t data_xor;
    fbe_lba_t bits_to_shift = 0;
    fbe_lba_t current_lba = data_pattern_info_p->start_lba;

    /* MAP IN SECTOR HERE */
    mapped_memory_cnt = (sectors * block_size);
    mapped_memory_ptr = (b_is_virtual)?
                  (void*)memory_ptr:
                  FBE_DATA_PATTERN_MAP_MEMORY(((fbe_u32_t *)memory_ptr), mapped_memory_cnt);

    if(mapped_memory_ptr == NULL)
    {
        fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Mapped memory ptr is NULL. \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    reference_ptr = mapped_memory_ptr;
    sector_p = (fbe_sector_t *)reference_ptr;

    while (sectors--)
    {
        fbe_bool_t b_sector_status = FBE_TRUE;
        /* Set word count to only touch data portion of sector.
         */
        fbe_u32_t word_count;

        if (block_size == FBE_BYTES_PER_BLOCK)
        {
            word_count = (block_size / sizeof(fbe_lba_t));
        }
        else
        {
            word_count = (block_size / sizeof(fbe_lba_t)) - meta_count;
        }
        /*  Loop through every word in a sector
         *  putting in seed.
         */
        /* reset header index
         */
        header_index = 0;
        data_xor = 0;
        while (word_count)
        {            
            if (header_index < data_pattern_info_p->num_header_words) {
                expected_pattern = data_pattern_info_p->header_array[header_index];
                header_index++;
            } else if (header_index == data_pattern_info_p->num_header_words) {
                /* Use a unique word per sector that combines the seed and pass count. 
                 * This ensures that each sector has a unique checksum. 
                 * It also ensures that the parity is different from one pass to another. 
                 * This means that incomplete writes will be guaranteed to have different parity 
                 * since pass count is built into this pattern. 
                 */
                bits_to_shift = (seed + 1) / FBE_RAID_SECTORS_PER_ELEMENT; /* Get element number. */
                bits_to_shift %= 16;
                expected_pattern_ls = seeded_pattern << (16 - bits_to_shift);
                expected_pattern_rs = seeded_pattern >> (48 - bits_to_shift);
                expected_pattern = expected_pattern_ls | expected_pattern_rs;
                expected_pattern_seed_ls = seed << (seed % 7);
                expected_pattern ^= expected_pattern_seed_ls;
                header_index++;
            }
            else {
                expected_pattern = seeded_pattern;
            }
            /* Pull the pattern out of the block.
             */
            received_pattern = *((fbe_lba_t *)reference_ptr);
            data_xor ^= received_pattern;

            /* Check if the pattern we read is as expected.
             */
            if (received_pattern != expected_pattern)
            {
                /* Miscompare found, trace the sector below.
                 */
                status = FBE_STATUS_GENERIC_FAILURE;
                b_sector_status = FBE_FALSE;
                break;
            }
            
            reference_ptr = (fbe_lba_t *)reference_ptr + 1;
            word_count--;

        }    /* end loop over all words in the sector. */

        if (b_sector_status == FBE_FALSE)
        {
            /*! @todo it would make sense to grab an  lock while we are tracing  
             * So if more than one thread hits an issue we trace the entire sector for one thread at a time. 
             */
            /* Miscompare detected.  Display the block.
             */
            fbe_data_pattern_trace(FBE_TRACE_LEVEL_WARNING, 
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                   "%s: miscompare object id: 0x%x lba: 0x%llx\n", 
                                   __FUNCTION__, object_id,
				   (unsigned long long)seed);
            fbe_data_pattern_trace(FBE_TRACE_LEVEL_WARNING, 
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                   "miscompare sector[%d]: expected hdr: 0x%x seeded_pattern: 0x%llx\n", 
                                   miscompare_sector_count, 0x3cc3,
				   (unsigned long long)seeded_pattern);
            fbe_data_pattern_trace(FBE_TRACE_LEVEL_WARNING, 
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                   "miscompare [%d] seed: 0x%llx, sequence_id:0x%x, number_of_headers:0x%x\n", 
                                   miscompare_sector_count,
				   (unsigned long long)seed, sequence_id,
				   data_pattern_info_p->num_header_words);
            fbe_data_pattern_trace_sector(sector_p);
            miscompare_sector_count++;

            if (b_panic)
            {
                fbe_data_pattern_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                       "data pattern library found data miscompare.  Panicking now. \n");
            }
        }
        else
        {
#if 0 /* Not yet ready to enable this in general. */
            if (block_size == FBE_BE_BYTES_PER_BLOCK)
            {
                fbe_u32_t csum;
                fbe_u16_t cooked_csum;
                csum = (data_xor >> 32) ^ (data_xor & 0xFFFFFFFF);
                cooked_csum = xorlib_cook_csum(csum, XORLIB_SEED_UNREFERENCED_VALUE);

                if (cooked_csum != sector_p->crc)
                {
                    fbe_data_pattern_trace(FBE_TRACE_LEVEL_WARNING, 
                                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                           "%s: checksum error object id: 0x%x lba: 0x%llx\n", 
                                           __FUNCTION__, object_id, seed);
                    fbe_data_pattern_trace(FBE_TRACE_LEVEL_WARNING, 
                                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                           "miscompare sector[%d]: expected hdr: 0x%x seeded_pattern: 0x%llx\n", 
                                           miscompare_sector_count, 0x3cc3, seeded_pattern);
                    fbe_data_pattern_trace(FBE_TRACE_LEVEL_WARNING, 
                                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                           "miscompare [%d] seed: 0x%llx, sequence_id:0x%x, number_of_headers:0x%x\n", 
                                           miscompare_sector_count, seed, sequence_id, data_pattern_info_p->num_header_words);
                    fbe_data_pattern_trace_sector(sector_p);
                    if (b_panic)
                    {
                        fbe_data_pattern_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                               "data pattern library found checksum error.  Panicking now. \n");
                    }
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            }
#endif
        }

        if (block_size == FBE_BE_BYTES_PER_BLOCK)
        {
            /* word ptr is now pointing to the meta data.
             * just skip over 2 meta data words.
             */
            reference_ptr = (fbe_lba_t *)reference_ptr + meta_count;
        }

        /* Increment the seed if there is a valid header word
         */
        if (data_pattern_info_p->num_header_words > 0)
        {
            /* Increment the seed
             */
            seed++;

            /* If we have exceeded the 48-bits allowed for the seed, fail the request.
             */
            if (seed > FBE_DATA_PATTERN_MAX_SEED_VALUE)
            {
                /* Report the error 
                 */
                fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                       "data pattern: seed: 0x%llx is larger that max supported: 0x%llx\n",
                                       (unsigned long long)seed,
				       (unsigned long long)FBE_DATA_PATTERN_MAX_SEED_VALUE);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /* Populated the 64-bit pattern.
             */
            seeded_pattern = Int64ShllMod32(pattern_ls17,31) | seed;
        }
        current_lba++;

        /* Goto the next sector.
         */
        sector_p++;
    }

    /* MAP OUT SECTOR HERE */
    if ( !b_is_virtual )
    {
        FBE_DATA_PATTERN_UNMAP_MEMORY(mapped_memory_ptr, mapped_memory_cnt);
    }

    return status;
}                               
/*******************************
 * end fbe_data_pattern_check_sector() 
 *******************************/

/****************************************************************
 *  fbe_data_pattern_check_for_invalidated_sector
 ****************************************************************
 * @brief
 *  check for invalidated sectors for a given contiguous range
 *  of sectors.
 *  If we are checking and we find a mismatch, panic.
 *  
 * @param sectors_to_fill - Contiguous sectors to check or generate.
 * @param data_pattern_info_p, - Data pattern information structure.
 * @param memory_ptr - Start of memory range.
 * @param corrupt_bitmap - bitmap indicating which blocks to invalidate.
 * @param b_is_virtual - indicates if the memory_ptr is virtual memory.
 * @param object_id - Object id thsat is being checked
 * @param b_panic - FBE_TRUE to panic on data miscompare,   
 *                  FBE_FALSE to not panic on data miscompare.
 *
 * @return
 *   FBE_STATUS_OK on success FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @author
 *   08/29/10 - Created - Swati Fursule
 ****************************************************************/
fbe_status_t fbe_data_pattern_check_for_invalidated_sector(fbe_u32_t sectors_to_fill,
                                                           fbe_data_pattern_info_t* data_pattern_info_p,
                                                           fbe_u32_t * memory_ptr,
                                                           fbe_u64_t corrupt_bitmap,
                                                           const fbe_bool_t b_is_virtual,
                                                           fbe_object_id_t object_id,
                                                           fbe_bool_t b_panic)
{
    fbe_status_t       status = FBE_STATUS_OK;
    fbe_sector_t       *sector_p = NULL;
    fbe_lba_t          seed = data_pattern_info_p->seed;
    xorlib_sector_invalid_reason_t expected_reason = XORLIB_SECTOR_INVALID_REASON_UNKNOWN;
    xorlib_sector_invalid_reason_t actual_reason = XORLIB_SECTOR_INVALID_REASON_UNKNOWN;
    void               *mapped_memory_ptr = NULL;
    fbe_u32_t           mapped_memory_cnt = 0;
    void               *reference_ptr = NULL;
    fbe_u32_t           block_index;

    /* MAP IN SECTOR HERE */
    mapped_memory_cnt = (sectors_to_fill * FBE_BE_BYTES_PER_BLOCK);
    mapped_memory_ptr = (b_is_virtual)?
                  (void*)memory_ptr:
                  FBE_DATA_PATTERN_MAP_MEMORY(((fbe_u32_t *)memory_ptr), mapped_memory_cnt);

    if(mapped_memory_ptr == NULL)
    {
        fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Mapped memory ptr is NULL. \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    reference_ptr = mapped_memory_ptr;
    sector_p = (fbe_sector_t *)reference_ptr;

    /* Determine the `reason' for invalidation
     */
    if ((data_pattern_info_p->pattern_flags & FBE_DATA_PATTERN_FLAGS_CORRUPT_CRC) ||
        (data_pattern_info_p->pattern_flags & FBE_DATA_PATTERN_FLAGS_DATA_LOST)      )
    {
        /* If b_corrupt_data is also set it is an error
         */
        if (data_pattern_info_p->pattern_flags & FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA)
        {
            /* We will continue and report the error at the end
             */
            fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                   "%s: Setting both: corrupt crc: 0x%x and  corrupt data: 0x%x not allowed\n", 
                                   __FUNCTION__, FBE_DATA_PATTERN_FLAGS_CORRUPT_CRC, FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA);
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        expected_reason = XORLIB_SECTOR_INVALID_REASON_DATA_LOST;
    }
    else if (data_pattern_info_p->pattern_flags & FBE_DATA_PATTERN_FLAGS_RAID_VERIFY)
    {
        /* If b_corrupt_data is also set it is an error
         */
        if (data_pattern_info_p->pattern_flags & FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA)
        {
            /* We will continue and report the error at the end
             */
            fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                   "%s: Setting both: raid verfify: 0x%x and  corrupt data: 0x%x not allowed\n", 
                                   __FUNCTION__, FBE_DATA_PATTERN_FLAGS_RAID_VERIFY, FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA);
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        expected_reason = XORLIB_SECTOR_INVALID_REASON_VERIFY;
    }
    else if (data_pattern_info_p->pattern_flags & FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA)
    {
        expected_reason = XORLIB_SECTOR_INVALID_REASON_CORRUPT_DATA;
    }

    /* Check all the required blocks.
     */
    for (block_index = 0; block_index < sectors_to_fill; block_index++)
    {
        /* Update the seed with local value
         */
        status = fbe_data_pattern_update_seed(data_pattern_info_p,
                                              seed /*incremented seed*/);
        if (DATA_PATTERN_COND(status != FBE_STATUS_OK))
        {
            fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: data pattern update seed failed 0x%x. \n", 
                                   __FUNCTION__, status);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Only expect an invalidated sector if the sector &'s with the
         * provided corrupt bitmap.
         */
        if ((((fbe_u64_t)1 << (block_index % (sizeof(fbe_u64_t) * BITS_PER_BYTE)))) & corrupt_bitmap)
        {
            fbe_bool_t b_status;
            b_status = xorlib_is_sector_invalidated(sector_p, 
                                                    &actual_reason, 
                                                    FBE_LBA_INVALID,
                                                    FBE_FALSE);
            if (b_status == FBE_FALSE)
            {
                if (data_pattern_info_p->pattern_flags & FBE_DATA_PATTERN_FLAGS_VALID_AND_INVALID_DATA)
                {
                    /* If not invalidated, we expect the seeded pattern.
                     */ 
                    status = fbe_data_pattern_check_sector(1,    /* 1 block */ 
                                                           data_pattern_info_p, 
                                                           (fbe_u32_t *)sector_p, 
                                                           FBE_TRUE,    /* Already mapped, so its 'virtual' */
                                                           FBE_BE_BYTES_PER_BLOCK,
                                                           object_id,
                                                           b_panic);
                }
                else
                {
                    fbe_data_pattern_trace(FBE_TRACE_LEVEL_WARNING, 
                                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                           "%s: non-invalidated sector lba 0x%llx count 0x%x obj: 0x%x\n", __FUNCTION__, seed, sectors_to_fill, object_id);
                    if (b_panic)
                    {
                        fbe_data_pattern_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                                               FBE_TRACE_MESSAGE_ID_INFO,
                                               "data pattern library found non-invalidated sector.  Panicking now. \n");
                    }
                }
            }
            else if ( !(data_pattern_info_p->pattern_flags & FBE_DATA_PATTERN_FLAGS_DO_NOT_CHECK_INVALIDATION_REASON) &&
                      ((actual_reason != expected_reason) &&
                       ((expected_reason != XORLIB_SECTOR_INVALID_REASON_CORRUPT_DATA) &&
                        (actual_reason != XORLIB_SECTOR_INVALID_REASON_INVALIDATED_DATA_LOST)) ) ) 
            {
                /*! @note Previously we didn't validate the reason.  In-fact
                 *        there are cases where the sector was invalidated by
                 *        RAID.
                 */
                fbe_data_pattern_trace(FBE_TRACE_LEVEL_WARNING, 
                                       FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                       "%s: Expected invalidate reason: %d actual: %d\n", 
                                       __FUNCTION__, expected_reason, actual_reason);
                if (b_panic)
                {
                    fbe_data_pattern_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                                           FBE_TRACE_MESSAGE_ID_INFO,
                                           "data pattern library found unexpected invalidation reason.  Panicking now. \n");
                }
            }
        }
        else
        {
            /* If not invalidated, we expect the seeded pattern.
             */ 
            status = fbe_data_pattern_check_sector(1, /* 1 block */ 
                                                   data_pattern_info_p, 
                                                   (fbe_u32_t *)sector_p, 
                                                   FBE_TRUE, /* Already mapped, so its 'virtual' */
                                                   FBE_BE_BYTES_PER_BLOCK,
                                                   object_id,
                                                   b_panic);
        }

        reference_ptr = (fbe_lba_t *)reference_ptr + 1;

        if (seed != (fbe_u32_t) - 1)
        {
            seed++;
        }
        sector_p++;

    } /* end for each block to check */

    /* MAP OUT SECTOR HERE */
    if ( !b_is_virtual )
    {
        FBE_DATA_PATTERN_UNMAP_MEMORY(mapped_memory_ptr, mapped_memory_cnt);
    }

    return status;
}                               
/*******************************
 * fbe_data_pattern_check_for_invalidated_sector() 
 *******************************/


/*!***************************************************************
 *  fbe_data_pattern_fill_invalidated_sector
 ****************************************************************
 * @brief
 *  Generate or check seed for a given contiguous range
 *  of sectors.
 *  If we are checking and we find a mismatch, panic.
 *  
 * @param sectors_to_fill - Contiguous sectors to check or generate.
 * @param data_pattern_info_p - data pattern information struct
 * @param memory_ptr - Start of memory range.
 * @param b_is_virtual - indicates if the memory_ptr is virtual memory.
 * @param corrupt_bitmap - bitmap indicating which blocks to invalidate
 * @param reason - Reason to put in the invalidated sectors.
 *
 * @return status - Typically FBE_STATUS_OK
 *
 * @author
 *   08/29/10 - Created - Swati Fursule
 ****************************************************************/

fbe_status_t fbe_data_pattern_fill_invalidated_sector(fbe_u32_t sectors_to_fill,
                                                      fbe_data_pattern_info_t* data_pattern_info_p,
                                                      fbe_u32_t * memory_ptr,
                                                      const fbe_bool_t b_is_virtual,
                                                      fbe_u64_t corrupt_bitmap,
                                                      xorlib_sector_invalid_reason_t reason)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_sector_t   *sector_p = NULL;
    fbe_lba_t       seed = data_pattern_info_p->seed;
    fbe_u32_t       block_index;
    
    void * mapped_memory_ptr = NULL;
    fbe_u32_t mapped_memory_cnt = 0;
    void * reference_ptr = NULL;

    /* MAP IN SECTOR HERE */
    mapped_memory_cnt = (sectors_to_fill * FBE_BE_BYTES_PER_BLOCK);
    mapped_memory_ptr = (b_is_virtual)?
                  (void *)memory_ptr:
                  FBE_DATA_PATTERN_MAP_MEMORY(((fbe_u32_t *)memory_ptr), mapped_memory_cnt);

    if(mapped_memory_ptr == NULL)
    {
        fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Mapped memory ptr is NULL. \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    reference_ptr = mapped_memory_ptr;
    sector_p = (fbe_sector_t *)reference_ptr;

    /* Fill in all the required blocks.
     */
    for (block_index = 0; block_index < sectors_to_fill; block_index++)
    {
        /* Always update the seed value
         */
        status = fbe_data_pattern_update_seed(data_pattern_info_p,
                                              seed /*incremented seed*/);
        if (DATA_PATTERN_COND(status != FBE_STATUS_OK))
        {
            fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: data pattern update seed failed 0x%x. \n", 
                                   __FUNCTION__, status);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    
        /* Only invalidate the block if the bitmap pattern indicates we should.
         */
        if (((fbe_u64_t)1 << (block_index % (sizeof(fbe_u64_t) * BITS_PER_BYTE))) & corrupt_bitmap)
        {
            fbe_xor_lib_fill_invalid_sector(sector_p,
                                            seed,
                                            reason,
                                            XORLIB_SECTOR_INVALID_WHO_CLIENT);

            /* Clear out the stamps.
             */
            sector_p->lba_stamp = 0;
            sector_p->time_stamp = FBE_SECTOR_INVALID_TSTAMP;
            sector_p->write_stamp = 0;
        }
        else
        {
            /* If the sector is not to be invalidated, 
             * fill the sg with a seeded pattern and crc.
             */
            fbe_data_pattern_fill_sector(1, /* 1 block. */
                                         data_pattern_info_p,
                                         (fbe_u32_t *)sector_p,
                                         FBE_TRUE, /* append CRC */ 
                                         FBE_BE_BYTES_PER_BLOCK,
                                         FBE_TRUE /* Already mapped in, so its 'virtual' */);
        }
    
        if (seed != (fbe_u32_t) - 1)
        {
            seed++;
        }
        sector_p++;
    
    } /* end for all sectors to fill */

    /* MAP OUT SECTOR HERE */
    if ( !b_is_virtual )
    {
        FBE_DATA_PATTERN_UNMAP_MEMORY(mapped_memory_ptr, mapped_memory_cnt);
    }

    return status;
}                               
/*******************************
 * fbe_data_pattern_fill_invalidated_sector() 
 *******************************/
/*************************
 * end file fbe_data_pattern_sector.c
 *************************/

