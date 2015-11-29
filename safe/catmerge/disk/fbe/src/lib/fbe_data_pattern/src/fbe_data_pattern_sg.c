/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_data_pattern_sg.c
 ***************************************************************************
 *
 * @brief
 *  This file contains data pattern sg list functions.
 *  This functionality is separated out from rdgen sglist . 
 *  implementation. Since now the user of this functionality are 
 *  FBECLI rdt, fbe_test along with rdgen service.
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
#include "fbe/fbe_random.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!***************************************************************
 *  fbe_data_pattern_sg_fill_with_memory
 ****************************************************************
 * @brief
 *  Generate a sg list with random counts.
 *  Fill the sg list passed in.
 *  Fill in the memory portion of this sg list from the
 *  contiguous range passed in.
 *  
 * @param sg_p - Sg element list.
 * @param memory_ptr - passed in memroy pointer
 * @param blocks - block count
 * @param block_size - block size
 * @param max_sg_data_elements - max SG elements
 * @param max_blocks_per_sg - Maximum block in a SG
 *
 * @return FBE_STATUS_OK on success, 
           FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @author
 *  8/29/2010 - Created. Swati Fursule
 *
 ****************************************************************/

fbe_status_t fbe_data_pattern_sg_fill_with_memory(fbe_sg_element_t *sg_p,
                                                  fbe_u8_t *memory_ptr,
                                                  fbe_block_count_t blocks,
                                                  fbe_block_size_t block_size,
                                                  fbe_u32_t max_sg_data_elements,
                                                  fbe_u32_t max_blocks_per_sg)
{
    fbe_u32_t sg_count = 0;
    fbe_block_count_t current_blocks;
    
    while (blocks > 0)
    {
        /* Determine the current blocks count is.
         */
        current_blocks = max_blocks_per_sg;

        /* We limit the amount in each SG so that we will
         * not exceed sg limits.
         */
        current_blocks = FBE_MIN(blocks, current_blocks);

        /* Validate our current blocks and our memory_ptr.
         */
        if (DATA_PATTERN_COND(current_blocks == 0))
        {
            fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: current blocks  0x%llx are zero. \n", 
                                    __FUNCTION__,
				    (unsigned long long)current_blocks );
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (DATA_PATTERN_COND(memory_ptr == NULL))
        {
            fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: memory pointer is null. \n", 
                                    __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
        /* Fill out the address and count
         * for this SG.
         */
        fbe_sg_element_init(sg_p, (fbe_u32_t)(current_blocks * block_size), memory_ptr);

        if (DATA_PATTERN_COND(sg_p->count % block_size != 0))
        {
            fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: sg_p->count 0x%x is not aligned to block size 0x%x. \n", 
                                    __FUNCTION__, sg_p->count, block_size );
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Increment sg and memory ptr.
         */
        sg_p++;
        memory_ptr += (current_blocks * block_size); 

        sg_count++;
        if (DATA_PATTERN_COND(sg_count > max_sg_data_elements))
        {
            fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: sg count 0x%x > max_sg_data_elements 0x%x. \n", 
                                    __FUNCTION__, sg_count, max_sg_data_elements );
            return FBE_STATUS_GENERIC_FAILURE;
        }
        blocks -= current_blocks;
    }
    /* Terminate sg list.
     */
    fbe_sg_element_terminate(sg_p);

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_data_pattern_sg_fill_with_memory
 **************************************/

/*!**************************************************************
 * fbe_data_pattern_fill_sg_list()
 ****************************************************************
 * @brief
 *  Fill an sg list with a given pattern.
 *
 * @param sg_ptr - Sg element list.
 * @param data_pattern_info_p - data pattern information struct
 * @param block_size - block size information
 * @param corrupt_bitmap - corrupted bitmap
 * @param max_sg_data_elements - MAX SG data elements
 *
 * @return FBE_STATUS_OK on success, 
           FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @author
 *  8/29/2010 - Created. Swati Fursule
 *
 ****************************************************************/
fbe_status_t fbe_data_pattern_fill_sg_list(fbe_sg_element_t * sg_ptr,
                                           fbe_data_pattern_info_t* data_pattern_info_p,
                                           fbe_block_size_t block_size,
                                           fbe_u64_t corrupt_bitmap,
                                           fbe_u32_t max_sg_data_elements)
{
    fbe_u32_t sg_count = 0;
    fbe_u32_t current_blocks;
    fbe_u32_t inc_blocks = 0;
    fbe_u32_t *memory_ptr;
    fbe_bool_t b_status = FBE_TRUE;   /* Status of pattern checking. */
    fbe_bool_t b_is_virtual = FBE_FALSE;
    fbe_lba_t seed = data_pattern_info_p->seed;
    fbe_data_pattern_t pattern = data_pattern_info_p->pattern;
    fbe_status_t status;
    fbe_sg_element_t * input_sg_ptr = sg_ptr;
    fbe_block_count_t total_blocks = 0;
    fbe_block_count_t num_sg_blocks = data_pattern_info_p->num_sg_blocks;

    if (DATA_PATTERN_COND(sg_ptr[max_sg_data_elements].address != NULL))
    {
        fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: last sg ptr's address 0x%p is not null. \n", 
                                __FUNCTION__, sg_ptr[max_sg_data_elements].address);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if (DATA_PATTERN_COND(sg_ptr[max_sg_data_elements].count != 0))
    {
        fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: last sg ptr's count 0x%x is not zero. \n", 
                                __FUNCTION__, sg_ptr[max_sg_data_elements].count);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Populate all sectors with the correct pattern.
     * We loop until either sg terminator or until we fill the number of sgs they asked for. 
     */
    while (((sg_ptr)->count != 0) && ((data_pattern_info_p->num_sg_blocks == 0) ||
                                      (data_pattern_info_p->num_sg_blocks > total_blocks)))
    {
        fbe_bool_t b_current_status = FBE_TRUE;
        
        if (DATA_PATTERN_COND(sg_ptr->count % block_size != 0))
        {

            fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: sg_ptr->count 0x%x is not aligned to block_size 0x%x. \n", 
                                    __FUNCTION__, sg_ptr->count, block_size);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Get current memory ptr and current blk count.
         */
        if (block_size == FBE_BYTES_PER_BLOCK)
        {
            inc_blocks = (sg_ptr)->count / FBE_BYTES_PER_BLOCK;
            current_blocks = (sg_ptr)->count / FBE_BYTES_PER_BLOCK;
        }
        else
        {
            current_blocks = (sg_ptr)->count / FBE_BE_BYTES_PER_BLOCK;
            inc_blocks = current_blocks;
        }
        
        memory_ptr = (fbe_u32_t *) (sg_ptr)->address;
        b_is_virtual = FBE_DATA_PATTERN_VIRTUAL_ADDRESS_AVAIL(sg_ptr);

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

        /* If we find the token, just skip over this sg.
         */
        if (memory_ptr != (fbe_u32_t *) -1)
        {
            /* If it is one of the `Data Lost - Invalidated' patterns, we expect
             * the invalidated read to be `data lost'.
             */
            if ((data_pattern_info_p->pattern_flags & FBE_DATA_PATTERN_FLAGS_CORRUPT_CRC) ||
                (data_pattern_info_p->pattern_flags & FBE_DATA_PATTERN_FLAGS_DATA_LOST)      )
            {
                /* This is a corrupt crc, invalidate the blocks.
                 */
                fbe_data_pattern_fill_invalidated_sector(current_blocks,
                                                         data_pattern_info_p,
                                                         memory_ptr,
                                                         b_is_virtual,
                                                         corrupt_bitmap,
                                                         XORLIB_SECTOR_INVALID_REASON_DATA_LOST);
            }
            else if (data_pattern_info_p->pattern_flags & FBE_DATA_PATTERN_FLAGS_RAID_VERIFY)
            {
                /* Else if some of the blocks were invalidated by RAID we expect the
                 * invalidation reason to be `RAID VERIFY"
                 */
                fbe_data_pattern_fill_invalidated_sector(current_blocks,
                                                         data_pattern_info_p,
                                                         memory_ptr,
                                                         b_is_virtual,
                                                         corrupt_bitmap,
                                                         XORLIB_SECTOR_INVALID_REASON_VERIFY);
            }
            else if (data_pattern_info_p->pattern_flags & FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA)
            {
                /* This is a corrupt data, invalidate the blocks.
                 */
                fbe_data_pattern_fill_invalidated_sector(current_blocks,
                                                         data_pattern_info_p,
                                                         memory_ptr,
                                                         b_is_virtual,
                                                         corrupt_bitmap,
                                                         XORLIB_SECTOR_INVALID_REASON_CORRUPT_DATA);
            }
            else if (pattern == FBE_DATA_PATTERN_CLEAR)
            {
                /* If we are not checking, we are clearing out these sgs.
                 * Fill with pattern.
                 */
                status = fbe_data_pattern_build_info(data_pattern_info_p,
                                            pattern,
                                            0, /* No special data pattern flags */
                                            0,/*seed*/
                                            0, /*pass count*/
                                            0, /*number of header words*/
                                            NULL);
                if (DATA_PATTERN_COND(status != FBE_STATUS_OK))
                {
                    fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, 
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "%s: data pattern build info failed 0x%x. \n", 
                                            __FUNCTION__, status);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                fbe_data_pattern_set_sg_blocks(data_pattern_info_p, num_sg_blocks);
                fbe_data_pattern_fill_sector(current_blocks,
                                             data_pattern_info_p,
                                             memory_ptr,
                                             FBE_FALSE /* do not append crc */, 
                                             block_size,
                                             b_is_virtual);
            }
            else if (pattern == FBE_DATA_PATTERN_ZEROS)
            {
                /* Fill the sg with a zeroed pattern and crc.
                 */
                /* Since we are checking for zeros, set pass and
                 * lba to zero, and set num_header_words to 0
                 */
                status = fbe_data_pattern_build_info(data_pattern_info_p,
                                            pattern,
                                            0, /* No special data pattern flags */
                                            0, /*seed*/
                                            0, /*pass count*/
                                            0, /*number of header words*/
                                            NULL);
                if (DATA_PATTERN_COND(status != FBE_STATUS_OK))
                {
                    fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, 
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "%s: data pattern build info failed 0x%x. \n", 
                                            __FUNCTION__, status);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                fbe_data_pattern_set_sg_blocks(data_pattern_info_p, num_sg_blocks);
                fbe_data_pattern_fill_sector(current_blocks, 
                                      /* Since we are checking for zeros, set pass and
                                       * lba to zero, and set b_check_header to 
                                       * FBE_FALSE. 
                                       */
                                             data_pattern_info_p,
                                             memory_ptr,
                                             FBE_TRUE /* append CRC */, 
                                             block_size,
                                             b_is_virtual);
            }
            else
            {
                if (DATA_PATTERN_COND(pattern != FBE_DATA_PATTERN_LBA_PASS))
                {
                    fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, 
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "%s: pattern 0x%x != FBE_DATA_PATTERN_LBA_PASS 0x%x. \n", 
                                            __FUNCTION__, pattern, FBE_DATA_PATTERN_LBA_PASS );
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                /* Seed was updated above
                 */

                /* Fill the sg with a seeded pattern and crc.
                 */
                fbe_data_pattern_fill_sector(current_blocks, 
                                             data_pattern_info_p,
                                             memory_ptr,
                                             FBE_TRUE /* append CRC */, 
                                             block_size,
                                             b_is_virtual);
            }

            if ( b_status )
            {
                /* If our status is still good, then save the latest status.
                 */
                b_status = b_current_status;
            }
        }

        /* Increment sg ptr.
         */
        sg_ptr++;

        sg_count++;
        if (DATA_PATTERN_COND(sg_count > max_sg_data_elements))
        {
            fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: sg count 0x%x > max_sg_data_elements 0x%x. \n", 
                                    __FUNCTION__, sg_count, max_sg_data_elements );
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Increment seed. */
        seed += inc_blocks;
        total_blocks += inc_blocks;
    }

    if (DATA_PATTERN_COND(input_sg_ptr[max_sg_data_elements].address != NULL))     
    {     
        fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR,      
                               FBE_TRACE_MESSAGE_ID_INFO,     
                                "%s: last sg ptr's address 0x%p is not null. \n",      
                                __FUNCTION__, input_sg_ptr[max_sg_data_elements].address );     
        return FBE_STATUS_GENERIC_FAILURE;     
    }     
    
    if (DATA_PATTERN_COND(input_sg_ptr[max_sg_data_elements].count != 0))     
    {     
        fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR,      
                                FBE_TRACE_MESSAGE_ID_INFO,     
                                "%s: last sg ptr's count 0x%x is not zero. \n",      
                                __FUNCTION__, input_sg_ptr[max_sg_data_elements].count);     
        return FBE_STATUS_GENERIC_FAILURE;     
    }     
    if (b_status == FBE_TRUE)
    {
        return FBE_STATUS_OK;
    }
    else
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
}                               /* fbe_data_pattern_fill_sg_list */


static fbe_bool_t rdgen_display_sector = FBE_FALSE;
static fbe_lba_t rdgen_lba_stamp_offset = 0x10000;
/****************************************************************
 *  fbe_data_pattern_check_sector_stamp
 ****************************************************************
 * @brief
 *  Check the checksum or lba stamp for a given range.
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
 *  12/10/2012 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_data_pattern_check_sector_stamp(fbe_u32_t sectors,
                                                 fbe_data_pattern_info_t* data_pattern_info_p,
                                                 fbe_u32_t * memory_ptr,
                                                 const fbe_bool_t b_is_virtual,
                                                 fbe_block_size_t block_size,
                                                 fbe_object_id_t object_id,
                                                 fbe_bool_t b_panic)
{
    fbe_u32_t current_block;
    fbe_sector_t *sector_p = (fbe_sector_t*)memory_ptr;
    fbe_lba_t current_seed = data_pattern_info_p->seed;
    fbe_u16_t crc;
    fbe_bool_t b_miscompare = FBE_FALSE;
    fbe_u16_t lba_stamp;

    for (current_block = 0; current_block < sectors; current_block++)
    {
        if (data_pattern_info_p->pattern_flags & FBE_DATA_PATTERN_FLAGS_CHECK_CRC_ONLY)
        {
            crc = fbe_xor_lib_calculate_checksum((fbe_u32_t*)sector_p);
            if (sector_p->crc != crc)
            {
                fbe_data_pattern_trace(FBE_TRACE_LEVEL_WARNING, 
                                       FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                       "checksum error  0x%04x != 0x%04x object id: 0x%x lba: 0x%llx\n", 
                                       sector_p->crc, crc, object_id, (unsigned long long)current_seed);
                b_miscompare = FBE_TRUE;
            }
        }
        if (data_pattern_info_p->pattern_flags & FBE_DATA_PATTERN_FLAGS_CHECK_LBA_STAMP_ONLY)
        {
            lba_stamp = xorlib_generate_lba_stamp(current_seed - rdgen_lba_stamp_offset, 0);
            if ((sector_p->lba_stamp != 0) && 
                (sector_p->lba_stamp != lba_stamp))
            {
                fbe_data_pattern_trace(FBE_TRACE_LEVEL_WARNING, 
                                       FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                       "lba stamp error  0x%04x != 0x%04x object id: 0x%x lba: 0x%llx\n", 
                                       sector_p->lba_stamp, lba_stamp, object_id, (unsigned long long)current_seed);
                b_miscompare = FBE_TRUE;
            }
        }
        if (b_miscompare && rdgen_display_sector)
        {
            fbe_data_pattern_trace_sector(sector_p);
        }
        sector_p++;
        current_seed++;
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_data_pattern_check_sector_stamp()
 **************************************/
/*!**************************************************************
 * fbe_data_pattern_check_sg_list()
 ****************************************************************
 * @brief
 *  Check an sg list for a given pattern.
 *
 * @param sg_ptr - Sg element list.
 * @param data_pattern_info_p - data pattern information struct
 * @param block_size - block size information
 * @param corrupt_bitmap - corrupted bitmap
 * @param object_id - object id
 * @param max_sg_data_elements - MAX SG data elements   
 * @param b_panic - FBE_TRUE to panic on data miscompare,   
 *                  FBE_FALSE to not panic on data miscompare.   
 *
 * @return FBE_STATUS_OK on success, 
           FBE_STATUS_GENERIC_FAILURE on failure.
 *
 * @author
 *  8/29/2010 - Created. Swati Fursule
 *
 ****************************************************************/
fbe_status_t fbe_data_pattern_check_sg_list(fbe_sg_element_t * sg_ptr,
                                            fbe_data_pattern_info_t* data_pattern_info_p,
                                            fbe_block_size_t block_size,
                                            fbe_u64_t corrupt_bitmap,
                                            fbe_object_id_t object_id,
                                            fbe_u32_t max_sg_data_elements,
                                            fbe_bool_t b_panic)
{
    fbe_u32_t sg_count = 0;
    fbe_u32_t current_blocks;
    fbe_block_count_t total_blocks = 0;
    fbe_u32_t inc_blocks = 0;
    fbe_u32_t *memory_ptr;
    fbe_bool_t b_is_virtual = FBE_FALSE;
    fbe_lba_t seed = data_pattern_info_p->seed;
    fbe_status_t return_status = FBE_STATUS_OK;
    fbe_status_t build_info_status;
    fbe_data_pattern_t pattern = data_pattern_info_p->pattern;
    fbe_sg_element_t * input_sg_ptr = sg_ptr;
    fbe_block_count_t num_sg_blocks = data_pattern_info_p->num_sg_blocks;

    if (DATA_PATTERN_COND(sg_ptr[max_sg_data_elements].address != NULL))
    {
        fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: last sg ptr's address %p is not null. \n", 
                                __FUNCTION__, sg_ptr[max_sg_data_elements].address);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if (DATA_PATTERN_COND(sg_ptr[max_sg_data_elements].count != 0))
    {
        fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: last sg ptr's count 0x%x is not zero. \n", 
                                __FUNCTION__, sg_ptr[max_sg_data_elements].count);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Populate all sectors with the correct pattern.
     * We loop until either sg terminator or until we fill the number of sgs they asked for. 
     */
    while (((sg_ptr)->count != 0) && ((data_pattern_info_p->num_sg_blocks == 0) ||
                                      (data_pattern_info_p->num_sg_blocks > total_blocks)))
    {
        fbe_status_t current_status = FBE_TRUE;
        
        if (DATA_PATTERN_COND(sg_ptr->count % block_size != 0))
        {
            fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: sg_ptr->count 0x%x is not aligned to block_size 0x%x. \n", 
                                    __FUNCTION__, sg_ptr->count, block_size);
            return FBE_STATUS_GENERIC_FAILURE;
        }


        /* Get current memory ptr and current blk count.
         */
        if (block_size == FBE_BYTES_PER_BLOCK)
        {
            inc_blocks = (sg_ptr)->count / FBE_BYTES_PER_BLOCK;
            current_blocks = (sg_ptr)->count / FBE_BYTES_PER_BLOCK;
        }
        else
        {
            current_blocks = (sg_ptr)->count / FBE_BE_BYTES_PER_BLOCK;
            inc_blocks = current_blocks;
        }
        
        memory_ptr = (fbe_u32_t *) (sg_ptr)->address;
        b_is_virtual = FBE_DATA_PATTERN_VIRTUAL_ADDRESS_AVAIL (sg_ptr);

        /* Update the seed in data pattern info.
         */
        build_info_status = fbe_data_pattern_update_seed(data_pattern_info_p,
                                                         seed /*incremented seed*/);
        if (DATA_PATTERN_COND(build_info_status != FBE_STATUS_OK))
        {
            fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, 
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: data pattern update seed failed 0x%x. \n", 
                                   __FUNCTION__, build_info_status);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* If we find the token, just skip over this sg.
         */
        if (memory_ptr != (fbe_u32_t *) -1)
        {
            
            /* If any of the blocks read were purposefully invalidated, use the
             * `corrupt_bitmap' to determine and check the invalidated blocks.
             */
            if ((data_pattern_info_p->pattern_flags & FBE_DATA_PATTERN_FLAGS_CORRUPT_CRC)  ||
                (data_pattern_info_p->pattern_flags & FBE_DATA_PATTERN_FLAGS_DATA_LOST)    ||
                (data_pattern_info_p->pattern_flags & FBE_DATA_PATTERN_FLAGS_RAID_VERIFY)  ||
                (data_pattern_info_p->pattern_flags & FBE_DATA_PATTERN_FLAGS_CORRUPT_DATA) ||
                (data_pattern_info_p->pattern == FBE_DATA_PATTERN_INVALIDATED) )
            {
                current_status = fbe_data_pattern_check_for_invalidated_sector(current_blocks,
                                                                               data_pattern_info_p,
                                                                               memory_ptr,
                                                                               corrupt_bitmap,
                                                                               b_is_virtual,
                                                                               object_id,
                                                                               b_panic);
            }
            else if ((data_pattern_info_p->pattern_flags & FBE_DATA_PATTERN_FLAGS_CHECK_CRC_ONLY) ||
                     (data_pattern_info_p->pattern_flags & FBE_DATA_PATTERN_FLAGS_CHECK_LBA_STAMP_ONLY))
            {
                current_status = fbe_data_pattern_check_sector_stamp(current_blocks,
                                                                     data_pattern_info_p,
                                                                     memory_ptr,
                                                                     b_is_virtual,
                                                                     block_size,
                                                                     object_id,
                                                                     b_panic);
            }
            else if (pattern == FBE_DATA_PATTERN_LBA_PASS)
            {
                /* Seed was updated above
                 */

                /* Check current block of memory for correct seed
                 */
                current_status = fbe_data_pattern_check_sector(current_blocks,
                                                               data_pattern_info_p,
                                                               memory_ptr,
                                                               b_is_virtual,
                                                               block_size,
                                                               object_id,
                                                               b_panic);
            }
            else if (pattern == FBE_DATA_PATTERN_CLEAR)
            {
                /* Since we are checking for all zeros, set pass and
                 * lba to zero, and set num_header_words to 0
                 */
                build_info_status = fbe_data_pattern_build_info(data_pattern_info_p,
                                                                pattern,
                                                                0, /* No special data pattern flags */
                                                                0, /*seed*/
                                                                0,    /*pass count*/
                                                                0,    /*number of header words*/
                                                                NULL);
                if (DATA_PATTERN_COND(build_info_status != FBE_STATUS_OK))
                {
                    fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, 
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "%s: data pattern build info failed 0x%x. \n", 
                                            __FUNCTION__, build_info_status);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                fbe_data_pattern_set_sg_blocks(data_pattern_info_p, num_sg_blocks);
                fbe_data_pattern_set_current_lba(data_pattern_info_p, seed);
                current_status = fbe_data_pattern_check_sector(current_blocks,
                                                               data_pattern_info_p,
                                                               memory_ptr, 
                                                               b_is_virtual,
                                                               block_size,
                                                               object_id,
                                                               b_panic);
            }
            else
            {
                if (DATA_PATTERN_COND(pattern != FBE_DATA_PATTERN_ZEROS))
                {
                    fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, 
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "%s: pattern 0x%x != FBE_DATA_PATTERN_ZEROS 0x%x. \n", 
                                            __FUNCTION__, pattern, FBE_DATA_PATTERN_ZEROS );
                    return FBE_STATUS_GENERIC_FAILURE;
                }

                /* Check current block of memory for zeros
                 */
                /* Since we are checking for zeros, set pass and
                 * lba to zero, and set num_header_words to 0
                 */
                build_info_status = fbe_data_pattern_build_info(data_pattern_info_p,
                                            pattern,
                                            0, /* No special data pattern flags */
                                            0, /*seed*/
                                            0, /*pass count*/
                                            0, /*number of header words*/
                                            NULL);
                if (DATA_PATTERN_COND(build_info_status != FBE_STATUS_OK))
                {
                    fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, 
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            "%s: data pattern build info failed 0x%x. \n", 
                                            __FUNCTION__, build_info_status);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                fbe_data_pattern_set_sg_blocks(data_pattern_info_p, num_sg_blocks);
                fbe_data_pattern_set_current_lba(data_pattern_info_p, seed);
                current_status = fbe_data_pattern_check_sector(current_blocks,
                                                               data_pattern_info_p,
                                                               memory_ptr, 
                                                               b_is_virtual,
                                                               block_size,
                                                               object_id,
                                                               b_panic);
            }

            if ( current_status != FBE_STATUS_OK )
            {
                /* If our status is still good, then save the latest status.
                 */
                return_status = current_status;
            }
        }

        /* Increment sg ptr.
         */
        sg_ptr++;

        sg_count++;
        if (DATA_PATTERN_COND(sg_count > max_sg_data_elements))
        {
            fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: sg count 0x%x > max_sg_data_elements 0x%x. \n", 
                                    __FUNCTION__, sg_count, max_sg_data_elements );
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Increment seed. */
        seed += inc_blocks;
        total_blocks += inc_blocks;
    }

    if (DATA_PATTERN_COND(input_sg_ptr[max_sg_data_elements].address != NULL))     
    {     
        fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR,      
                               FBE_TRACE_MESSAGE_ID_INFO,     
                                "%s: last sg ptr's address %p is not null. \n",      
                                __FUNCTION__, input_sg_ptr[max_sg_data_elements].address );     
        return FBE_STATUS_GENERIC_FAILURE;     
    }     
    
    if (DATA_PATTERN_COND(input_sg_ptr[max_sg_data_elements].count != 0))     
    {     
        fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR,      
                                FBE_TRACE_MESSAGE_ID_INFO,     
                                "%s: last sg ptr's count 0x%x is not zero. \n",      
                                __FUNCTION__, input_sg_ptr[max_sg_data_elements].count);     
        return FBE_STATUS_GENERIC_FAILURE;     
    }  

    return return_status;
}
/******************************************
 * end fbe_data_pattern_check_sg_list()
 ******************************************/

/*!**************************************************************
 * fbe_data_pattern_region_list_validate()
 ****************************************************************
 * @brief
 *  Validate the data pattern region.
 *
 * @param list_p - Ptr to source list to validate.
 * @param list_length - Number of active entries in list.
 *
 * @return fbe_status_t
 *
 * @author
 *  9/12/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_data_pattern_region_list_validate(fbe_data_pattern_region_list_t *list_p,
                                                   fbe_u32_t list_length)
{
    fbe_u32_t index = 0;
       
    for ( index = 0; index < list_length; index++)
    {
        /* We validate the block count, pattern and sp_id
         */
        if (list_p[index].blocks == 0) 
        {
            fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: data pattern region idx %d blocks == 0 \n", 
                                   __FUNCTION__, index);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if ((list_p[index].pattern == FBE_DATA_PATTERN_INVALID)  ||
            (list_p[index].pattern >= FBE_DATA_PATTERN_LAST))
        {
            fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: data pattern region idx %d pattern %d invalid lba: 0x%llx bl: 0x%llx\n", 
                                   __FUNCTION__, index, list_p[index].pattern,
                                   (unsigned long long)list_p[index].lba,
				   (unsigned long long)list_p[index].blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if ((list_p[index].sp_id != FBE_DATA_PATTERN_SP_ID_A) &&
            (list_p[index].sp_id != FBE_DATA_PATTERN_SP_ID_B))
        {
            fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: data pattern region idx %d sp_id %d invalid lba: 0x%llx bl: 0x%llx\n", 
                                   __FUNCTION__, index, list_p[index].sp_id,
                                   (unsigned long long)list_p[index].lba,
				   (unsigned long long)list_p[index].blocks);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_data_pattern_region_list_validate()
 ******************************************/
/*!**************************************************************
 * fbe_data_pattern_region_list_copy_pattern()
 ****************************************************************
 * @brief
 *  Copy matching pattern/pass count entries in the source
 *  to the destination.
 *
 * @param src_list_p - Ptr to source list to copy from.
 * @param src_list_length - Number of active entries in src list.
 *                          This is the number we copy to destination.
 * @param dst_list_p - Ptr to destination list to copy to.
 * @param dst_list_length_p - Output number of active entries in dst list.
 *                          This is the number we copied to the destination.
 * @param pattern - Each source entry with this pattern is a candidate for
 *                  being copied to destination assuming input pass_count matches or
 *                  is FBE_U32_MAX.
 * @param pass_count - If the pass count is FBE_U32_MAX then we do not use it
 *                     as a criteria for selecting source entries to copy.
 *                     Otherwise the pass count in the source must match
 *                     to be copied.
 *
 * @return fbe_status_t
 *
 * @author
 *  9/7/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_data_pattern_region_list_copy_pattern(fbe_data_pattern_region_list_t *src_list_p,
                                                       fbe_u32_t src_list_length,
                                                       fbe_data_pattern_region_list_t *dst_list_p,
                                                       fbe_u32_t *dst_list_length_p,
                                                       fbe_data_pattern_t pattern,
                                                       fbe_bool_t b_pass_count_exclude,
                                                       fbe_u32_t pass_count)
{
    fbe_u32_t src_index = 0;
    fbe_u32_t dst_index = 0;
       
    for ( src_index = 0; src_index < src_list_length; src_index++)
    {
        /* Only copy entries from source that match the given pattern and pass count. 
         * If pass count is invalid we consider it a match. 
         * If we are excluding certain pass counts, then check for a mismatch on the pass count. 
         */
        if ( (src_list_p[src_index].pattern == pattern) && 
             ((pass_count == FBE_U32_MAX) || 
              (!b_pass_count_exclude && (src_list_p[src_index].pass_count == pass_count)) ||
              (b_pass_count_exclude && (src_list_p[src_index].pass_count != pass_count)) ) ) 
        {
            dst_list_p[dst_index] = src_list_p[src_index];
            dst_index++;
        }
    }
    *dst_list_length_p = dst_index;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_data_pattern_region_list_copy_pattern()
 ******************************************/
/*!**************************************************************
 * fbe_data_pattern_region_list_validate_pair()
 ****************************************************************
 * @brief
 *  Copy the sp id of matching lbas in the source
 *  to the destination.
 *  
 * @param src_list_p - Ptr to source list to copy from.
 * @param src_list_length - Number of active entries in src list.
 *                          This is the number we copy to destination.
 * @param dst_list_p - Ptr to destination list to copy to.
 * @param dst_list_length - Output number of active entries in dst list.
 *                          This is the number we copied to the destination.
 * @param pattern - Each source entry with this pattern is a candidate for
 *                  being copied to destination assuming input pass_count matches or
 *                  is FBE_U32_MAX.
 * @param pass_count - If the pass count is FBE_U32_MAX then we do not use it
 *                     as a criteria for selecting source entries to copy.
 *                     Otherwise the pass count in the source must match
 *                     to be copied.
 *
 * @return fbe_status_t
 *
 * @author
 *  9/7/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_data_pattern_region_list_validate_pair(fbe_data_pattern_region_list_t *src_list_p,
                                                        fbe_u32_t src_list_length,
                                                        fbe_data_pattern_region_list_t *dst_list_p,
                                                        fbe_u32_t dst_list_length)
{
    fbe_u32_t src_index = 0;
    fbe_u32_t dst_index = 0;

    /* We scan all source entries. 
     * For each source entry we scan all destination entries for a match. 
     */
    for ( src_index = 0; src_index < src_list_length; src_index++)
    {
        for ( dst_index = 0; dst_index < dst_list_length; dst_index++)
        {
            /* Copy one entry in the destination if the lbas match.
             */
            if (src_list_p[src_index].lba == dst_list_p[dst_index].lba)
            {
                if (src_list_p[src_index].lba != dst_list_p[dst_index].lba)
                {
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                if (src_list_p[src_index].pass_count != dst_list_p[dst_index].pass_count)
                {
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                if (src_list_p[src_index].blocks != dst_list_p[dst_index].blocks)
                {
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                if (src_list_p[src_index].sp_id != dst_list_p[dst_index].sp_id)
                {
                    fbe_data_pattern_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: data pattern region idx %d sp_id %x != %x lba: 0x%llx bl: 0x%llx\n", 
                                   __FUNCTION__, src_index, src_list_p[src_index].sp_id, dst_list_p[dst_index].sp_id,
                                   (unsigned long long)src_list_p[src_index].lba,
				   (unsigned long long)src_list_p[src_index].blocks);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                break;
            }
        }
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_data_pattern_region_list_validate_pair()
 ******************************************/
/*!**************************************************************
 * fbe_data_pattern_region_list_copy_sp_id()
 ****************************************************************
 * @brief
 *  Copy the sp id of matching lbas in the source
 *  to the destination.
 *  
 * @param src_list_p - Ptr to source list to copy from.
 * @param src_list_length - Number of active entries in src list.
 *                          This is the number we copy to destination.
 * @param dst_list_p - Ptr to destination list to copy to.
 * @param dst_list_length - Output number of active entries in dst list.
 *                          This is the number we copied to the destination.
 * @param pattern - Each source entry with this pattern is a candidate for
 *                  being copied to destination assuming input pass_count matches or
 *                  is FBE_U32_MAX.
 * @param pass_count - If the pass count is FBE_U32_MAX then we do not use it
 *                     as a criteria for selecting source entries to copy.
 *                     Otherwise the pass count in the source must match
 *                     to be copied.
 *
 * @return fbe_status_t
 *
 * @author
 *  9/7/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_data_pattern_region_list_copy_sp_id(fbe_data_pattern_region_list_t *src_list_p,
                                                     fbe_u32_t src_list_length,
                                                     fbe_data_pattern_region_list_t *dst_list_p,
                                                     fbe_u32_t dst_list_length)
{
    fbe_u32_t src_index = 0;
    fbe_u32_t dst_index = 0;

    /* We scan all source entries. 
     * For each source entry we scan all destination entries for a match. 
     */
    for ( src_index = 0; src_index < src_list_length; src_index++)
    {
        for ( dst_index = 0; dst_index < dst_list_length; dst_index++)
        {
            /* Copy one entry in the destination if the lbas match.
             */
            if (src_list_p[src_index].lba == dst_list_p[dst_index].lba)
            {
                dst_list_p[dst_index].sp_id = src_list_p[src_index].sp_id;
                break;
            }
        }
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_data_pattern_region_list_copy_sp_id()
 ******************************************/
/*!**************************************************************
 * fbe_data_pattern_region_list_set_random_pattern()
 ****************************************************************
 * @brief
 *  
 *
 * @param list_p - Pointer to list.  This list already was setup
 *                 via a call to fbe_data_pattern_create_region_list().
 * @param max_list_length - Max possible length of the list.
 *                          This is the number of valid entries in it.
 * @param pattern - This is where we should start the regions.
 * @param pattern - Pattern to put in each entry.               
 * @param background_pattern - This is the background pattern we already
 *                             put down.
 * @param background_pass_count - Pass count to use for background pattern.
 *                                    if this is FBE_U32_MAX,
 *                                    then we choose a random background
 *                                    pass count.          
 *
 * @return fbe_status_t  
 *
 * @author
 *  9/7/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_data_pattern_region_list_set_random_pattern(fbe_data_pattern_region_list_t *list_p,
                                                             fbe_u32_t max_list_length,
                                                             fbe_data_pattern_t pattern,
                                                             fbe_data_pattern_t background_pattern,
                                                             fbe_u32_t background_pass_count,
                                                             fbe_data_pattern_sp_id_t background_sp_id)
{
    fbe_u32_t index = 0;
    /* Randomly select between background and normal pattern for the first region.
     */
    fbe_bool_t b_background = fbe_random() % 2;
       
    for ( index = 0; index < max_list_length; index++)
    {
        list_p[index].sp_id = background_sp_id;
        if (b_background)
        {
            list_p[index].pattern = background_pattern;
            if (background_pass_count == FBE_U32_MAX)
            {
                list_p[index].pass_count = fbe_random() % 0xFF;
            }
            else
            {
                list_p[index].pass_count = background_pass_count;
            }
        }
        else
        {
            list_p[index].pattern = pattern;
            list_p[index].pass_count = fbe_random() % 0xFF;
        }
        /* We want to alternate between background and normal pattern.
         */
        b_background = !b_background;
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_data_pattern_region_list_set_random_pattern()
 ******************************************/
/*!**************************************************************
 * fbe_data_pattern_create_region_list()
 ****************************************************************
 * @brief
 *  Create the list of random sized regions.
 *
 * @param list_p - Pointer to list.
 * @param list_length_p - Output Length of the list.
 * @param max_list_length - Max possible length of the list.
 *                          This is the size we allocated for it.
 * @param start_lba - This is where we should start the regions.
 * @param max_blocks - Max size of each region.
 * @param capacity - Total capacity to cover.
 * @param pattern - Pattern to put in each entry.            
 *
 * @return fbe_status_t
 *
 * @author
 *  9/7/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_data_pattern_create_region_list(fbe_data_pattern_region_list_t *list_p,
                                                 fbe_u32_t *list_length_p,
                                                 fbe_u32_t max_list_length,
                                                 fbe_lba_t start_lba,
                                                 fbe_block_count_t max_blocks,
                                                 fbe_lba_t capacity,
                                                 fbe_data_pattern_t pattern)
{
    fbe_u32_t index = 0;
    fbe_lba_t current_lba = start_lba;
    fbe_block_count_t current_blocks;

    while ((index < max_list_length- 1) && (current_lba < capacity))
    {
        list_p[index].lba = current_lba;
        /* We use a random number to generate a block count. 
         * We use max_blocks + 1 so we can generate a number up to max_blocks. 
         * We use + 1 for the overall block count so that we do not generate 
         * a current_blocks that is zero. 
         */
        current_blocks = (fbe_random() % (max_blocks + 1)) + 1;
        list_p[index].blocks = FBE_MIN(current_blocks, (capacity - current_lba));
        list_p[index].pattern = pattern;
        current_lba += list_p[index].blocks;
        index++;
    }
    /* If there is anything left, give it the remainder of capacity so 
     * we will represent the entire range. 
     */
    if (current_lba < capacity)
    {
        list_p[index].lba = current_lba;
        list_p[index].blocks = capacity - current_lba;
        list_p[index].pattern = pattern;
        index++;
    }
    *list_length_p = index;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_data_pattern_create_region_list()
 ******************************************/

/*************************
 * end file fbe_data_pattern_sg.c
 *************************/

