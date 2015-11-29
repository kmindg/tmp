/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raw_mirror_service_sg.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the code for the raw mirror service scatter-gather list support
 *  for I/O requests.
 * 
 * @version
 *   10/2011:  Created. Susan Rundbaken 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe_raw_mirror_service_private.h"
#include "fbe/fbe_api_common_transport.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static fbe_status_t fbe_raw_mirror_service_fill_sg_list(fbe_sg_element_t * sg_list_p,
                                                        fbe_data_pattern_t  pattern,
                                                        fbe_lba_t seed,
                                                        fbe_u32_t sequence_id,
                                                        fbe_u64_t sequence_num,
                                                        fbe_block_size_t block_size,
                                                        fbe_u32_t num_sg_list_elements);
static fbe_status_t fbe_raw_mirror_service_fill_sector(fbe_u32_t sectors,
                                                       fbe_u32_t * memory_ptr,
                                                       fbe_bool_t b_append_checksum,
                                                       fbe_lba_t seed,
                                                       fbe_u32_t sequence_id,
                                                       fbe_u64_t sequence_num,
                                                       fbe_block_size_t block_size);
static fbe_status_t fbe_raw_mirror_service_check_sector(fbe_u32_t sectors,
                                                        fbe_u32_t * memory_ptr,
                                                        fbe_bool_t b_append_checksum,
                                                        fbe_lba_t seed,
                                                        fbe_u32_t sequence_id,
                                                        fbe_u64_t sequence_num,
                                                        fbe_block_size_t block_size,
                                                        fbe_bool_t dmc_expected_b,
                                                        fbe_bool_t b_panic);


/*!**************************************************************
 *  fbe_raw_mirror_service_sg_list_create()
 ****************************************************************
 * @brief
 *  Create a SG list for raw mirror testing and set it in the specified payload.
 *
 * @param sep_payload_p - payload for the start I/O operation.
 * @param start_io_p - start I/O specification.
 *
 * @return fbe_status_t.
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/
fbe_status_t fbe_raw_mirror_service_sg_list_create(fbe_payload_ex_t * sep_payload_p,
                                                   fbe_raw_mirror_service_io_specification_t * start_io_spec_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_sg_element_t *                  sg_list_p = NULL;
    fbe_sg_element_t *                  sg_element_p = NULL;
    void *                              memory_p = NULL;
    fbe_block_size_t                    block_size;
    fbe_u32_t                           block_count;
    fbe_u32_t                           sg_element_count;


    /* Get the block size and block count.
     */
    block_size = start_io_spec_p->block_size;
    block_count = start_io_spec_p->block_count;

    /* Allocate memory for a 2-element SG list for testing purposes.
     */
    sg_list_p = (fbe_sg_element_t *)fbe_memory_native_allocate(2 * sizeof(fbe_sg_element_t));
    if (sg_list_p == NULL)
    {
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: cannot allocate memory for SG list. \n", 
                                     __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    memory_p = fbe_memory_native_allocate(block_size * block_count);
    if (memory_p == NULL)
    {
        fbe_memory_native_release(sg_list_p);
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: cannot allocate memory for SG list. \n", 
                                     __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    sg_element_p = sg_list_p;
    fbe_sg_element_init(sg_element_p, block_size * block_count, memory_p);
    fbe_sg_element_increment(&sg_element_p);
    fbe_sg_element_terminate(sg_element_p);

    sg_element_count = fbe_sg_element_count_entries(sg_list_p);
    if ((start_io_spec_p->operation == FBE_RAW_MIRROR_SERVICE_OPERATION_WRITE_ONLY) ||
        (start_io_spec_p->operation == FBE_RAW_MIRROR_SERVICE_OPERATION_WRITE_VERIFY))
    {
        fbe_lba_t seed = start_io_spec_p->start_lba;
        fbe_u32_t sequence_id = start_io_spec_p->sequence_id;

        /* Fill SG list with specification pattern.
         */
        status = fbe_raw_mirror_service_fill_sg_list(sg_list_p,
                                                     start_io_spec_p->pattern,
                                                     seed,
                                                     sequence_id,
                                                     start_io_spec_p->sequence_num,
                                                     block_size,
                                                     sg_element_count);
        if (status != FBE_STATUS_OK )
        {
            fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                         FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: fill data pattern sg list failed. \n", 
                                         __FUNCTION__);

            /* Free SG list.
             */
            fbe_raw_mirror_service_sg_list_free(sg_list_p);

            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Set sg list in the payload. 
     */
    fbe_payload_ex_set_sg_list(sep_payload_p, sg_list_p, sg_element_count);

    return status;

}
/******************************************
 * end fbe_raw_mirror_service_sg_list_create()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_fill_sg_list()
 ****************************************************************
 * @brief
 *  Fill the sectors in a SG list with a specific pattern.
 *
 *  Note that much of this code comes from fbe_data_pattern_fill_sg_list().
 *
 * @param sg_list_p - pointer to SG list.
 * @param pattern - data pattern used for filling up the sectors.
 * @param seed - LBA value.
 * @param sequence_id - fixed portion of seed; used for debugging.
 * @param sequence_num - raw mirror I/O sequence number.
 * @param block_size - number of bytes in a block.
 * @param num_sg_list_elements - number of elements in SG list.
 *
 * @return fbe_status_t.
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/
static fbe_status_t fbe_raw_mirror_service_fill_sg_list(fbe_sg_element_t * sg_list_p,
                                                        fbe_data_pattern_t  pattern,
                                                        fbe_lba_t seed,
                                                        fbe_u32_t sequence_id,
                                                        fbe_u64_t sequence_num,
                                                        fbe_block_size_t block_size,
                                                        fbe_u32_t num_sg_list_elements)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       sg_block_count;
    fbe_u32_t       *memory_ptr;


    /* Make sure the SG list is terminated properly.
     */
    if (sg_list_p[num_sg_list_elements].address != NULL)
    {
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: last sg ptr's address 0x%p is not null. \n", 
                                     __FUNCTION__, sg_list_p[num_sg_list_elements].address);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if (sg_list_p[num_sg_list_elements].count != 0)
    {
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: last sg ptr's count 0x%x is not zero. \n", 
                                     __FUNCTION__, sg_list_p[num_sg_list_elements].count);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Populate all sectors with the correct pattern
     */
    while (sg_list_p->count != 0)
    {

        /* Make sure count is aligned to block size.
         */
        if (sg_list_p->count % block_size != 0)
        {

            fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                         FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: sg_list_p->count 0x%x is not aligned to block_size 0x%x. \n", 
                                         __FUNCTION__, sg_list_p->count, block_size);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Set the memory ptr.
         */
        memory_ptr = (fbe_u32_t *)sg_list_p->address;

        /* Get the block count.
         */
        if (block_size == FBE_BYTES_PER_BLOCK)
        {
            sg_block_count = sg_list_p->count / FBE_BYTES_PER_BLOCK;
        }
        else
        {
            sg_block_count = sg_list_p->count / FBE_BE_BYTES_PER_BLOCK;
        }
        
        /* For now, only supporting FBE_DATA_PATTERN_LBA_PASS.  Additional patterns
         * will be supported as testing progresses.
         */
        if (pattern != FBE_DATA_PATTERN_LBA_PASS)
        {
            fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                         FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: pattern 0x%x != FBE_DATA_PATTERN_LBA_PASS 0x%x. \n", 
                                         __FUNCTION__, pattern, FBE_DATA_PATTERN_LBA_PASS );
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Fill the sg with a seeded pattern, sequence number for raw mirror I/O, and crc.
         */
        fbe_raw_mirror_service_fill_sector(sg_block_count, 
                                           memory_ptr,
                                           FBE_TRUE /* append CRC */,
                                           seed,
                                           sequence_id, 
                                           sequence_num,
                                           block_size);

        /* Increment SG ptr.
         */
        sg_list_p++;

        /* Increment seed. */
        seed += sg_block_count;
    }

    return status;
}
/******************************************
 * end fbe_raw_mirror_service_fill_sg_list()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_fill_sector()
 ****************************************************************
 * @brief
 *  Fill the words in the sector of a SG list with a specific pattern.
 *
 *  Note that much of this code comes from fbe_data_pattern_fill_sector().
 *
 * @param sectors - contiguous sectors to fill.
 * @param memory_ptr - start of memory range.
 * @param b_append_checksum - if FBE_TRUE, append checksum.
 * @param seed - LBA value.
 * @param sequence_id - fixed portion of seed; used for debugging.
 * @param sequence_num - raw mirror I/O sequence number.
 * @param block_size - number of bytes in a block.
 *
 * @return fbe_status_t.
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/
static fbe_status_t fbe_raw_mirror_service_fill_sector(fbe_u32_t sectors,
                                                       fbe_u32_t * memory_ptr,
                                                       fbe_bool_t b_append_checksum,
                                                       fbe_lba_t seed,
                                                       fbe_u32_t sequence_id,
                                                       fbe_u64_t sequence_num,
                                                       fbe_block_size_t block_size)
{
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_u32_t pattern_ls17 = sequence_id<<17;
    
    /* Use the highest 16 bits as pass count. We need an extra shift
     * here to achieve 48 bit shift (17 + 31 = 48).
     */
    fbe_lba_t seeded_pattern = Int64ShllMod32(pattern_ls17,31) | seed;
    fbe_u32_t raw_mirror_io_meta_count = 2;     /* raw mirror I/O meta data is two 64 bit 'words' */   
    void * reference_ptr = memory_ptr;
    fbe_sector_t *sector_p = (fbe_sector_t *)reference_ptr;
    fbe_u32_t meta_count = 1;                   /* crc data is one 64 bit 'word' */ 

    while (sectors--)
    {
        /* Set word count to only touch data portion of sector.
         */
        fbe_u32_t word_count;

        if (sequence_num != FBE_LBA_INVALID)
        {
            /* Sequence number will be set for this test; will set it after the data field is set.
             */
            word_count = (FBE_RAW_MIRROR_DATA_SIZE / sizeof(fbe_lba_t)); 
        }
        else
        {
            /* Sequence number will not be set so not treating this I/O as raw mirror I/O for this test.
             * Will set all of the data field with the seeded pattern accordingly.
             */
            if (block_size == FBE_BYTES_PER_BLOCK)
            {
                word_count = (block_size / sizeof(fbe_lba_t));
            }
            else
            {
                word_count = (block_size / sizeof(fbe_lba_t)) - meta_count;
            }
        }

        /*  Loop through every word in the data portion of a sector
         *  putting in seed.
         */
        while (word_count)
        {            
            *((fbe_lba_t*)reference_ptr) = seeded_pattern;

            reference_ptr = (fbe_lba_t *)reference_ptr + 1;
            word_count--;
        } 

        /* For raw mirror I/O, if the sequence number is set, append the sequence number and magic number to 
         * end of data portion of sector.
         */
        if (sequence_num != FBE_LBA_INVALID)
        {
            fbe_xor_lib_raw_mirror_sector_set_seq_num(sector_p, sequence_num);

            /* Reference ptr needs to skip over this metadata. */
            reference_ptr = (fbe_u64_t*)reference_ptr + raw_mirror_io_meta_count;
        }

        if (block_size == FBE_BE_BYTES_PER_BLOCK)
        {
            if (b_append_checksum)
            {
                /* If we are appending checksums, then calculate and set
                 * the checksum for this block.
                 */
                *((fbe_u32_t*)reference_ptr) = fbe_xor_lib_calculate_checksum((fbe_u32_t*)sector_p);
                reference_ptr = (fbe_u32_t *)reference_ptr + 1;
    
                *((fbe_u32_t*)reference_ptr) = 0;
                reference_ptr = (fbe_u32_t *)reference_ptr + 1;
            }
            else
            {
                /* Not appending checksum, so just set it to 0 and increment the reference pointer.
                 */
                *((fbe_u32_t*)reference_ptr) = 0;
                reference_ptr = (fbe_u32_t *)reference_ptr + 1;
    
                *((fbe_u32_t*)reference_ptr) = 0;
                reference_ptr = (fbe_u32_t *)reference_ptr + 1;
            }
        }

        sector_p++;
    }

    return status;
} 
/******************************************
 * end fbe_raw_mirror_service_fill_sector()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_sg_list_check()
 ****************************************************************
 * @brief
 *  Check a SG list for a specific pattern.
 *
 *  Note that much of this code comes from fbe_data_pattern_check_sg_list().
 *
 * @param start_io_p - start I/O specification. 
 * @param sg_list_p - pointer to SG list.
 * @param num_sg_list_elements - number of elements in SG list.
 *
 * @return fbe_status_t.
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/
fbe_status_t fbe_raw_mirror_service_sg_list_check(fbe_raw_mirror_service_io_specification_t * start_io_spec_p,
                                                  fbe_sg_element_t * sg_list_p,
                                                  fbe_u32_t num_sg_list_elements)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       sg_block_count;
    fbe_u64_t       seed = start_io_spec_p->start_lba;
    fbe_u32_t       *memory_ptr;


    /* Make sure the SG list is terminated properly.
     */
    if (sg_list_p[num_sg_list_elements].address != NULL)
    {
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: last sg ptr's address 0x%p is not null. \n", 
                                     __FUNCTION__, sg_list_p[num_sg_list_elements].address);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if (sg_list_p[num_sg_list_elements].count != 0)
    {
        fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                     FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s: last sg ptr's count 0x%x is not zero. \n", 
                                     __FUNCTION__, sg_list_p[num_sg_list_elements].count);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Check all sectors for the specified pattern
     */
    while (sg_list_p->count != 0)
    {

        /* Make sure count is aligned to block size.
         */
        if (sg_list_p->count % start_io_spec_p->block_size != 0)
        {

            fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                         FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: sg_list_p->count 0x%x is not aligned to block_size 0x%x. \n", 
                                         __FUNCTION__, sg_list_p->count, start_io_spec_p->block_size);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Set the memory ptr.
         */
        memory_ptr = (fbe_u32_t *)sg_list_p->address;

        /* Get the block count.
         */
        if (start_io_spec_p->block_size == FBE_BYTES_PER_BLOCK)
        {
            sg_block_count = sg_list_p->count / FBE_BYTES_PER_BLOCK;
        }
        else
        {
            sg_block_count = sg_list_p->count / FBE_BE_BYTES_PER_BLOCK;
        }

        /* For now, only supporting FBE_DATA_PATTERN_LBA_PASS.  Additional patterns
         * will be supported as testing progresses.
         */
        if (start_io_spec_p->pattern != FBE_DATA_PATTERN_LBA_PASS)
        {
            fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                         FBE_TRACE_MESSAGE_ID_INFO,
                                         "%s: pattern 0x%x != FBE_DATA_PATTERN_LBA_PASS 0x%x. \n", 
                                         __FUNCTION__, start_io_spec_p->pattern, FBE_DATA_PATTERN_LBA_PASS );
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Check the sector for the seeded pattern, sequence number for raw mirror I/O, and crc.
         */
        fbe_raw_mirror_service_check_sector(sg_block_count, 
                                            memory_ptr,
                                            FBE_TRUE, /* append CRC */
                                            seed,
                                            start_io_spec_p->sequence_id,
                                            start_io_spec_p->sequence_num,
                                            start_io_spec_p->block_size,
                                            start_io_spec_p->dmc_expected_b,
                                            FBE_TRUE /* do not panic on miscompare */);

        /* Increment SG ptr.
         */
        sg_list_p++;

        /* Increment seed. 
         */
        seed += sg_block_count;
    }

    return status;
}
/******************************************
 * end fbe_raw_mirror_service_sg_list_check()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_check_sector()
 ****************************************************************
 * @brief
 *  Check the words in the sectors of a SG list for a specific pattern.
 *
 *  Note that much of this code comes from fbe_data_pattern_check_sector().
 *
 * @param sectors - contiguous sectors to fill.
 * @param memory_ptr - start of memory range.
 * @param b_append_checksum - if FBE_TRUE, append checksum.
 * @param seed - LBA value.
 * @param sequence_id - fixed portion of seed; used for debugging.
 * @param sequence_num - raw mirror I/O sequence number.
 * @param block_size - number of bytes in a block.
 * @param dmc_expected_b - boolean indicating if DMC expected or not for this test.
 * @param b_panic - FBE_TRUE to panic on data miscompare,   
 *                  FBE_FALSE to not panic on data miscompare.
 *
 * @return fbe_status_t.
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/
static fbe_status_t fbe_raw_mirror_service_check_sector(fbe_u32_t sectors,
                                                        fbe_u32_t * memory_ptr,
                                                        fbe_bool_t b_append_checksum,
                                                        fbe_lba_t seed,
                                                        fbe_u32_t sequence_id,
                                                        fbe_u64_t sequence_num,
                                                        fbe_block_size_t block_size,
                                                        fbe_bool_t dmc_expected_b,
                                                        fbe_bool_t b_panic)
{
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_u32_t pattern_ls17 = sequence_id<<17;
    
    /* Use the highest 16 bits as pass count. We need an extra shift
     * here to achieve 48 bit shift (17 + 31 = 48).
     */
    fbe_lba_t seeded_pattern = Int64ShllMod32(pattern_ls17,31) | seed;
    fbe_lba_t expected_pattern = 0;
    fbe_lba_t received_pattern = 0;
    fbe_u32_t raw_mirror_io_meta_count = 2;     /* raw mirror I/O meta data is two 64 bit 'word' */ 
    fbe_u32_t meta_count = 1;                   /* crc data is one 64 bit 'word' */   
    fbe_u32_t miscompare_sector_count = 0;
    fbe_bool_t b_sector_ok = FBE_TRUE;
    fbe_u64_t received_sequence_num; 
    void * reference_ptr = memory_ptr;
    fbe_sector_t *sector_p = (fbe_sector_t *)reference_ptr;

    while (sectors--)
    {
        /* Set word count to only touch data portion of sector.
         */
        fbe_u32_t word_count;  

        if (sequence_num != FBE_LBA_INVALID)
        {
            /* Sequence number is set for this test; will check for it after the data check.
             */
            word_count = (FBE_RAW_MIRROR_DATA_SIZE / sizeof(fbe_lba_t)); 
        }
        else
        {
            /* Sequence number is not set so not treating this I/O as raw mirror I/O for this test.
             * Will check all of the data field for the expected pattern accordingly.
             */
            if (block_size == FBE_BYTES_PER_BLOCK)
            {
                word_count = (block_size / sizeof(fbe_lba_t));
            }
            else
            {
                word_count = (block_size / sizeof(fbe_lba_t)) - meta_count;
            }
        }

        /*  Loop through every word in the data portion of a sector
         *  checking the pattern.
         */
        while (word_count)
        {            
            expected_pattern = seeded_pattern;

            /* Pull the pattern out of the block.
             */
            received_pattern = *((fbe_lba_t *)reference_ptr);

            /* Check if the pattern we read is as expected.
             */
            if (received_pattern != expected_pattern)
            {
                /* Miscompare found, trace the sector below.
                 */
                status = FBE_STATUS_GENERIC_FAILURE;
                b_sector_ok = FBE_FALSE;
                break;
            }
            reference_ptr = (fbe_lba_t *)reference_ptr + 1;
            word_count--;
        } 

        if ((b_sector_ok == FBE_FALSE) && !dmc_expected_b)
        {
            /* Miscompare detected and not expected.  Display the block.
             */
            fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_WARNING, 
                                         FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                         "%s: miscompare lba: 0x%llx\n", 
                                         __FUNCTION__, (unsigned long long)seed);
            fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_WARNING, 
                                         FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                         "miscompare sector[%d]: expected hdr: 0x%x seeded_pattern: 0x%llx\n", 
                                         miscompare_sector_count, 0x3cc3,
					 (unsigned long long)seeded_pattern);
            fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_WARNING, 
                                         FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                         "miscompare [%d] seed: 0x%llx, sequence_id:0x%x\n", 
                                         miscompare_sector_count,
					 (unsigned long long)seed, sequence_id);
            fbe_data_pattern_trace_sector(sector_p);
            miscompare_sector_count++;

            if (b_panic)
            {
                fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                             "Raw mirror service found data miscompare.  Panicking now. \n");
            }
        }

        /* Check the sequence number if set.
         */
        if (sequence_num != FBE_LBA_INVALID)
        {
            received_sequence_num = fbe_xor_lib_raw_mirror_sector_get_seq_num(sector_p);
            
            if (received_sequence_num != sequence_num)
            {
                fbe_raw_mirror_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                             "%s: sequence number mismatch: received seq num:0x%llx; expected seq num:0x%llx\n",
                                             __FUNCTION__,
                                             (unsigned long long)received_sequence_num, 
                                             (unsigned long long)sequence_num);
            }
    
            /* Reference ptr needs to skip over the raw I/O metadata. */
            reference_ptr = (fbe_u64_t*)reference_ptr + raw_mirror_io_meta_count;
        }

        if (block_size == FBE_BE_BYTES_PER_BLOCK)
        {
            /* Reference ptr is now pointing to the sector meta data.
             * Just skip over 2 meta data words.
             */
            reference_ptr = (fbe_lba_t *)reference_ptr + meta_count;
        }

        sector_p++;
    }

    return status;
} 
/******************************************
 * end fbe_raw_mirror_service_check_sector()
 ******************************************/

/*!**************************************************************
 *  fbe_raw_mirror_service_sg_list_free()
 ****************************************************************
 * @brief
 *  Free the memory allocated for a SG list.
 *
 * @param sg_list_p - pointer to SG list.
 * 
 * @return None.
 *
 * @author
 *  10/2011 - Created. Susan Rundbaken 
 *
 ****************************************************************/
void fbe_raw_mirror_service_sg_list_free(fbe_sg_element_t * sg_list_p)
{
    /* Free the SG list
     */
    fbe_memory_native_release(sg_list_p->address);
    fbe_memory_native_release(sg_list_p);

    return;
}
/******************************************
 * end fbe_raw_mirror_service_sg_list_free()
 ******************************************/


/*************************
 * end file fbe_raw_mirror_main.c
 *************************/


