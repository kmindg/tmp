/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_raid_debug_sg.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the scatter gather related debug functions for the raid library.
 *
 * @author
 *  10/15/2010 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe/fbe_transport.h"
#include "fbe_block_transport.h"
#include "fbe/fbe_raid_ts.h"
#include "fbe_transport_debug.h"
#include "fbe_raid_algorithm.h"
#include "fbe_raid_geometry.h"
#include "fbe_raid_iots_accessor.h"
#include "fbe_raid_siots_accessor.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static fbe_status_t fbe_raid_library_debug_display_sg_sectors(const fbe_u8_t * module_name,
                                                              fbe_u32_t count,
                                                              fbe_u64_t address,
                                                              fbe_trace_func_t trace_func,
                                                              fbe_trace_context_t trace_context,
                                                              fbe_u32_t spaces_to_indent,
                                                              fbe_u32_t words_to_display,
                                                              fbe_lba_t lba);

/*!**************************************************************
 * fbe_raid_library_debug_print_sg_list()
 ****************************************************************
 * @brief
 *  Display an sg list.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param sg_p - ptr to sg list.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * @param num_sector_words_to_display - Number of words to display from each sector.
 * @param lba_base - base lba to use in display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/13/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_library_debug_print_sg_list(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr sg_p,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_u32_t spaces_to_indent,
                                                  fbe_u32_t num_sector_words_to_display,
                                                  fbe_lba_t lba_base)
{
    fbe_u64_t current_sg_p;
    fbe_u32_t count;
    fbe_u64_t address;
    fbe_u32_t blocks;
    fbe_u32_t sg_index = 0;
    fbe_u32_t total_blocks = 0;
    fbe_u32_t ptr_size;
    fbe_u32_t count_offset;
    fbe_u32_t addr_offset;
    fbe_u64_t sg_element_size;
    fbe_lba_t lba_offset = lba_base;
    
    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);
    FBE_GET_TYPE_SIZE(module_name, fbe_sg_element_t, &sg_element_size);
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    FBE_GET_FIELD_OFFSET(module_name, fbe_sg_element_t, "count", &count_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_sg_element_t, "address", &addr_offset);
    
    /* Read in the entire sg list.
     * It will never be bigger than BM_MAX_SG_ELEMENTS.
     */ 
    trace_func(trace_context, "fbe_sg_element_t * 0x%llx\n",
	       (unsigned long long)sg_p);

    /* Print out each sg and sector in the list
     */
    current_sg_p = sg_p;

    FBE_READ_MEMORY(current_sg_p + count_offset, &count, sizeof(fbe_u32_t));
    FBE_READ_MEMORY(current_sg_p + addr_offset, &address, ptr_size);

    /* If we are tracing zero sectors, just have one header.
     */
    if (num_sector_words_to_display == 0)
    {
        trace_func(trace_context, "|Index| sg memory address  | count   address           | blocks    | total blocks | lba   |\n");
        trace_func(trace_context, "|-----|--------------------|---------------------------|-----------|--------------|------\n");
    }
    while (count != 0)
    {
        /* If we are tracing multiple sectors, trace the header for each sg.
         */
        if (num_sector_words_to_display > 0)
        {
            trace_func(trace_context, "|Index| sg memory address  | count   address           | blocks    | total blocks | lba \n");
            trace_func(trace_context, "|-----|--------------------|---------------------------|-----------|--------------|-----\n");
        }
        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_CANCELED;
        }

        if ((count % FBE_BE_BYTES_PER_BLOCK) == 0)
        {
            blocks = count / FBE_BE_BYTES_PER_BLOCK;
        }
        else
        {
            blocks = (count / FBE_BE_BYTES_PER_BLOCK) + 1;
        }
        total_blocks += blocks;
        trace_func(trace_context, "| %3d | 0x%016llx | 0x%04x 0x%016llx | %3d       | %4d 0x%04x  | 0x%llx\n", 
                   sg_index,
	           (unsigned long long)current_sg_p, count,
	           (unsigned long long)address, blocks,
		   total_blocks, total_blocks,
	           (unsigned long long)lba_offset );

        /* When verbose mode is on we call print_sectors_64 to 
         * display a brief snapshot of a few words in the block and the meta-data. 
         */
        if (num_sector_words_to_display > 0)
        {
            fbe_raid_library_debug_display_sg_sectors(module_name, count, address, trace_func, trace_context, spaces_to_indent, 
                                                      num_sector_words_to_display, lba_offset);
        }

        current_sg_p = current_sg_p + sg_element_size;
        sg_index++;
        lba_offset += blocks;
        FBE_READ_MEMORY(current_sg_p + count_offset, &count, sizeof(fbe_u32_t));
        FBE_READ_MEMORY(current_sg_p + addr_offset, &address, ptr_size);
    }

    return FBE_STATUS_OK;
}
/******************************* 
 * fbe_raid_library_debug_print_sg_list()
 *******************************/
/*!**************************************************************
 * fbe_raid_library_debug_calc_checksum()
 ****************************************************************
 * @brief
 *  calculate a checksum for an input sector.
 *  We redefine this here since we do not want to pull in
 *  the entire xor library just for this (simple) algorithm.
 *
 * @param sector_p - Pointer to remote sector on SP.
 * @param checksum_p - Returned checksum value.               
 *
 * @return FBE_STATUS_OK OR FBE_STATUS_CANCELED   
 *
 * @author
 *  10/14/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_library_debug_calc_checksum(fbe_sector_t *sector_p,
                                                  fbe_u32_t *checksum_p)
{
    fbe_u32_t  word_index = 0;
    fbe_u32_t checksum = 0;

    /* Calculate Checksum
     */
    word_index = 0;
    checksum = 0;

    while (word_index < FBE_WORDS_PER_BLOCK)
    {    
        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_CANCELED;
        }
        checksum ^= sector_p->data_word[word_index];
        word_index++;
    }

    /* Start with a seed.
     */
    checksum ^= FBE_SECTOR_CHECKSUM_SEED_CONST;

    /* Rotate and fold.
     */
    if (checksum & 0x80000000)
    {
        checksum = checksum << 0x1;
        checksum = checksum | 0x1;
    }
    else
    {
        checksum = checksum << 0x01;
    }
        
    checksum = 0xFFFF & ((checksum >> 16) ^ checksum);

    /* Set value to return.
     */
    *checksum_p = checksum;
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_library_debug_calc_checksum()
 ******************************************/

/*!**************************************************************
 * fbe_raid_library_debug_display_sg_sectors()
 ****************************************************************
 * @brief
 *  Display an sg list.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param count - Count from sg element.
 * @param address - Address of sg element.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * @param num_display_words - Number of words in the block to display.
 * @param lba - start lba to display at.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/13/2010 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_raid_library_debug_display_sg_sectors(const fbe_u8_t * module_name,
                                                              fbe_u32_t count,
                                                              fbe_u64_t address,
                                                              fbe_trace_func_t trace_func,
                                                              fbe_trace_context_t trace_context,
                                                              fbe_u32_t spaces_to_indent,
                                                              fbe_u32_t num_display_words,
                                                              fbe_lba_t lba)
{
    fbe_u32_t blocks;
    fbe_u64_t sector_p = 0;
    fbe_sector_t sector;
    fbe_status_t status;
    fbe_u32_t sector_index = 0;

    /* 4 data words + 2 metadata words is the minimum we will display.
     */
    #define FBE_RAID_LIBRARY_DEBUG_MIN_DISPLAY_WORDS 6
    blocks = 0;

    if ((count % BE_BYTES_PER_BLOCK) == 0)
    {
        blocks = count / BE_BYTES_PER_BLOCK;
    }
    else
    {
        /* If this does happen, display something.
         */
        trace_func(trace_context, "ERROR: sg Count not multiple of %d count: %d address: 0x%llx.\n",
                   BE_BYTES_PER_BLOCK, count, (unsigned long long)address);
        blocks = (count / BE_BYTES_PER_BLOCK) + 1;
    }
    sector_p = address;

    if (num_display_words <= FBE_RAID_LIBRARY_DEBUG_MIN_DISPLAY_WORDS)
    {
        trace_func(trace_context, "\n");
        trace_func(trace_context, "Idx | sector ptr         | words 0..4                                  |  metadata                   | [Calculated crc] \n");
        trace_func(trace_context, "----|--------------------|---------------------------------------------|-----------------------------|---------------------------\n");
    }

    while (blocks != 0)
    {
        fbe_u32_t crc; 

        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_CANCELED;
        }

        FBE_READ_MEMORY(sector_p, &sector, sizeof(fbe_sector_t));

        status = fbe_raid_library_debug_calc_checksum(&sector, &crc);
        if (status != FBE_STATUS_OK)
        {
            trace_func(trace_context, "%s status of %d from calculate checksum\n",
                       __FUNCTION__, status);
            return status; 
        }
        
        if (num_display_words <= FBE_RAID_LIBRARY_DEBUG_MIN_DISPLAY_WORDS)
        {
            trace_func(trace_context, "%03d | 0x%016llx | 0x%08x 0x%08x 0x%08x 0x%08x | 0x%04x 0x%04x 0x%04x 0x%04x | [0x%x] %s | 0x%llx\n", 
                       sector_index,
                       (unsigned long long)sector_p,
                       sector.data_word[0], 
                       sector.data_word[1], 
                       sector.data_word[2], 
                       sector.data_word[3],
                       sector.time_stamp,
                       sector.crc,
                       sector.lba_stamp,    /* aka shed_stamp */
                       sector.write_stamp,    /* aka write_stamp */
                       crc,
                       crc == sector.crc ? "crc ok" : "CHECKSUM ERROR",
                       (unsigned long long)lba);
        }
        else
        {
            fbe_u32_t word;
            /* We always display the metadata words to subtract these from the total.
             */
            fbe_u32_t words_to_display = num_display_words - 2;

            /* Make sure words to display is not too large.
             */
            if (words_to_display > FBE_WORDS_PER_BLOCK)
            {
                words_to_display = FBE_WORDS_PER_BLOCK;
            }
            fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
            trace_func(trace_context, "sector ptr: 0x%016llx sector_index: %d lba: 0x%llx\n", (unsigned long long)sector_p, sector_index, (unsigned long long)lba);
            /* Display 4 words at a time.
             */
            for (word = 0; word < words_to_display; word += 4)
            {
                fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
                trace_func(trace_context, "0x%08x 0x%08x 0x%08x 0x%08x\n",
                           sector.data_word[word + 0], 
                           sector.data_word[word + 1], 
                           sector.data_word[word + 2], 
                           sector.data_word[word + 3]); 
                if (FBE_CHECK_CONTROL_C())
                {
                    return FBE_STATUS_CANCELED;
                }
            }
            /* Display metadata.
             */
            fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
            trace_func(trace_context, "0x%04x 0x%04x 0x%04x 0x%04x   calculated crc: [0x%x] %s\n",
                       sector.time_stamp,
                       sector.crc,
                       sector.lba_stamp,
                       sector.write_stamp, 
                       crc, crc == sector.crc ? "crc ok" : "CHECKSUM ERROR");
            trace_func(trace_context, "\n");
        }

        sector_p = sector_p + FBE_BE_BYTES_PER_BLOCK;
        blocks--;
        sector_index++;
        lba++;
    }
    trace_func(trace_context, "\n");
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_library_debug_display_sg_sectors
 **************************************/
/*************************
 * end file fbe_raid_debug_sg.c
 *************************/
