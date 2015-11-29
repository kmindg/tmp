/***************************************************************************
 *  terminator_simulated_disk_main.c
 ***************************************************************************
 *
 *  Description
 *      APIs to emulate the disk drives
 *
 *
 *  History:
*      08/12/09    guov    Created (Move Peter's memory_drive)
 *
 ***************************************************************************/
#include "terminator_simulated_disk_private.h"
#include "fbe/fbe_sector.h"
#include "fbe_terminator_common.h"
/**********************************/
/*        local variables         */
/**********************************/

/******************************/
/*     local function         */
/*****************************/

fbe_status_t terminator_simulated_disk_generate_zero_buffer(fbe_u8_t * data_buffer_pointer, fbe_lba_t lba, fbe_block_count_t block_count, fbe_block_size_t block_size, fbe_bool_t do_valid_metadata)
{
    fbe_u64_t   zero_metadata = 0x7fff5eed;
    fbe_u32_t   current_offset = 512;
    fbe_u32_t   total_bytes;

    if ((block_count * block_size) > FBE_U32_MAX) {
        terminator_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                         "0x%llx * 0x%x > FBE_U32_MAX\n", block_count, block_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    total_bytes = (fbe_u32_t)(block_count * block_size);
    fbe_zero_memory(data_buffer_pointer, total_bytes);

    if (do_valid_metadata)
    {    
        data_buffer_pointer += 512;
        while (current_offset < total_bytes) {
             fbe_copy_memory(data_buffer_pointer, (void *) &zero_metadata, 8);
             data_buffer_pointer += 520;
             current_offset += 520;
        }
    }
    return FBE_STATUS_OK;
}

fbe_status_t terminator_simulated_disk_crypto_process_data(fbe_u8_t *data_buffer,
                                                  fbe_u32_t buffer_size,
                                                  fbe_u8_t *dek, 
                                                  fbe_u32_t dek_size)
{
    fbe_u8_t * data_chunk_ptr = NULL;
    fbe_u8_t * dek_chunk_ptr = NULL;
    fbe_u32_t data_counter = 0;
    fbe_u32_t block_counter = 0;
    fbe_u32_t metadata_counter = 0;

    if (data_buffer == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    //Loop through 1 block data of at a time
    for (block_counter = 0; block_counter < buffer_size; block_counter += FBE_BE_BYTES_PER_BLOCK)
    {
        data_chunk_ptr = data_buffer;
        dek_chunk_ptr = dek;
        for(data_counter = 0; data_counter < dek_size; data_counter++)
        {
            *data_chunk_ptr ^= *dek_chunk_ptr;
            data_chunk_ptr++;
            dek_chunk_ptr++;
        }

        /* Move the pointer to the Metadata area */
        data_chunk_ptr = data_buffer + FBE_BYTES_PER_BLOCK;

        /*Now Encrypt the metadata (last 8 bytes)*/

        for (metadata_counter = 0; metadata_counter < (FBE_BE_BYTES_PER_BLOCK - FBE_BYTES_PER_BLOCK) ; metadata_counter++)
        {
            *data_chunk_ptr = ~(*data_chunk_ptr);
            data_chunk_ptr++;
        }

        data_buffer += FBE_BE_BYTES_PER_BLOCK;

    }

    return FBE_STATUS_OK;
}   

fbe_status_t terminator_simulated_disk_encrypt_data(fbe_u8_t *data_buffer,
                                           fbe_u32_t buffer_size,
                                           fbe_u8_t *dek, 
                                           fbe_u32_t dek_size)
{
    fbe_status_t status;

    status = terminator_simulated_disk_crypto_process_data(data_buffer, buffer_size, dek, dek_size);

    return status;
}

fbe_status_t terminator_simulated_disk_decrypt_data(fbe_u8_t *data_buffer,
                                           fbe_u32_t buffer_size,
                                           fbe_u8_t *dek, 
                                           fbe_u32_t dek_size)
{
    fbe_status_t status;
    status = terminator_simulated_disk_crypto_process_data(data_buffer, buffer_size, dek, dek_size);
    return status;
}
