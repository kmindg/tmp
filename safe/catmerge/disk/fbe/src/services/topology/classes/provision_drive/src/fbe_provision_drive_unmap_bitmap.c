/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2015
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_provision_drive_unmap_bitmap.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the provision drive object's unmap bitmap 
 *  related functionality.
 * 
 * @ingroup provision_drive_class_files
 * 
 * @version
 *   06/03/2015:  Created. Lili Chen
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_provision_drive_private.h"
#include "fbe_transport_memory.h"
#include "fbe_database.h"

static fbe_bool_t fbe_provision_drive_unmap_bitmap_user_enabled = FBE_TRUE;

/*!****************************************************************************
 * @enum fbe_provision_drive_unmap_bitmap_constants_e
 *
 * @brief
 *    Enum for paged metadata cache constants.
 ******************************************************************************/
typedef enum fbe_provision_drive_unmap_bitmap_constants_e
{
#if defined(UMODE_ENV) || defined(SIMMODE_ENV)
    FBE_PROVISION_DRIVE_UNMAP_BITMAP_MB_PER_BIT = 8,
#else
    /* For now it is fit to a 512 block (256 * 2)*/
    FBE_PROVISION_DRIVE_UNMAP_BITMAP_MB_PER_BIT = 256,
#endif
    FBE_PROVISION_DRIVE_UNMAP_BITMAP_BLOCKS_PER_BIT = (FBE_PROVISION_DRIVE_UNMAP_BITMAP_MB_PER_BIT * 1024 * 2),
    FBE_PROVISION_DRIVE_UNMAP_BITMAP_CHUNKS_PER_BIT = (FBE_PROVISION_DRIVE_UNMAP_BITMAP_BLOCKS_PER_BIT / FBE_PROVISION_DRIVE_CHUNK_SIZE),
}fbe_provision_drive_unmap_bitmap_constants_t;

/*************************
 *   FORWARD DECLARATIONS
 *************************/
static fbe_status_t fbe_provision_drive_unmap_bitmap_update(fbe_provision_drive_t * provision_drive_p, 
                                                            fbe_payload_metadata_operation_t * mdo);
static fbe_status_t fbe_provision_drive_unmap_bitmap_check_nonpaged(fbe_provision_drive_t * provision_drive_p);



/*!****************************************************************************
 * @fn fbe_provision_drive_unmap_bitmap_is_enabled()
 ******************************************************************************
 * @brief
 *  This function checks whether the unmap bitmap is enabled for this PVD.
 *
 * @param provision_drive_p    - pointer to the provision drive.  
 *
 * @return fbe_bool_t          - return true if all the blocks are mapped.
 *
 * @author
 *  06/03/2015 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline fbe_bool_t
fbe_provision_drive_unmap_bitmap_is_enabled(fbe_provision_drive_t * provision_drive_p)
{
    return (fbe_provision_drive_unmap_bitmap_user_enabled && 
            provision_drive_p->unmap_bitmap.is_enabled);
}
/******************************************************************************
 * end fbe_provision_drive_unmap_bitmap_is_enabled()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_unmap_bitmap_set_bit()
 ******************************************************************************
 * @brief
 *  This function sets one bit the unmap bitmap.
 *
 * @param provision_drive_p    - pointer to the provision drive.  
 * @param bit_offset           - bit offset to the bitmap.  
 *
 * @return NULL
 *
 * @author
 *  06/03/2015 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline void
fbe_provision_drive_unmap_bitmap_set_bit(fbe_provision_drive_t * provision_drive_p, 
                                         fbe_u64_t bit_offset)
{
    fbe_atomic_t * word_ptr;
    fbe_atomic_t mask;

    word_ptr = (fbe_atomic_t *)provision_drive_p->unmap_bitmap.bitmap;
    word_ptr += (bit_offset/64);
    mask = (1ull << (bit_offset%64));
    fbe_atomic_or(word_ptr, mask);
}
/******************************************************************************
 * end fbe_provision_drive_unmap_bitmap_set_bit()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_unmap_bitmap_clear_bit()
 ******************************************************************************
 * @brief
 *  This function clears one bit the unmap bitmap.
 *
 * @param provision_drive_p    - pointer to the provision drive.  
 * @param bit_offset           - bit offset to the bitmap.  
 *
 * @return NULL
 *
 * @author
 *  06/03/2015 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline void
fbe_provision_drive_unmap_bitmap_clear_bit(fbe_provision_drive_t * provision_drive_p, 
                                           fbe_u64_t bit_offset)
{
    fbe_atomic_t * word_ptr;
    fbe_atomic_t mask;

    word_ptr = (fbe_atomic_t *)provision_drive_p->unmap_bitmap.bitmap;
    word_ptr += (bit_offset/64);
    mask = ~(1ull << (bit_offset%64));
    fbe_atomic_and(word_ptr, mask);
}
/******************************************************************************
 * end fbe_provision_drive_unmap_bitmap_clear_bit()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_unmap_bitmap_evaluate_metadata()
 ******************************************************************************
 * @brief
 *  This function evaluates 1 chunk (1MBytes) of metadata.
 *  When the need_zero bit is cleared, this chunk is mapped.
 *
 * @param paged_metadata_p     - pointer to the paged metadata.  
 *
 * @return fbe_bool_t          - return true if all the blocks are mapped.
 *
 * @author
 *  06/03/2015 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline fbe_bool_t
fbe_provision_drive_unmap_bitmap_evaluate_metadata(fbe_provision_drive_paged_metadata_t * paged_metadata_p)
{
    if (paged_metadata_p->valid_bit && 
        !(paged_metadata_p->need_zero_bit))
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}
/******************************************************************************
 * end fbe_provision_drive_unmap_bitmap_evaluate_metadata()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_unmap_bitmap_get_size()
 ******************************************************************************
 * @brief
 *  This function get the bitmap size
 *
 * @param provision_drive_p    - pointer to the provision drive.  
 * @param bit_map_size_p       - pointer to the unmap bit map size 
 * @return fbe_status_t        - status of the operation.
 *
 * @author
 *  11/10/2015 - Created. Geng.Han
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_unmap_bitmap_get_size(fbe_provision_drive_t *  provision_drive_p, fbe_u32_t* bit_map_size_p)
{
    fbe_u32_t         number_of_bytes;
    fbe_lba_t         exported_capacity;
    fbe_chunk_count_t total_chunks; 

    /* get the exported capacity of provision drive object. */
    fbe_base_config_get_capacity((fbe_base_config_t*)provision_drive_p, &exported_capacity);
    total_chunks = (fbe_chunk_count_t)(exported_capacity / FBE_PROVISION_DRIVE_CHUNK_SIZE);

    number_of_bytes = (total_chunks / FBE_PROVISION_DRIVE_UNMAP_BITMAP_CHUNKS_PER_BIT + 7) / 8;

    // align the size to sizeof(fbe_u64_t)
    number_of_bytes = ((number_of_bytes + sizeof(fbe_u64_t) - 1) / sizeof(fbe_u64_t)) * sizeof(fbe_u64_t);
    
    *bit_map_size_p = number_of_bytes;

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_unmap_bitmap_get_size()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_unmap_bitmap_init()
 ******************************************************************************
 * @brief
 *  This function initializes the unmap bitmap.
 *
 * @param provision_drive_p    - pointer to the provision drive.  
 *
 * @return fbe_status_t        - status of the operation.
 *
 * @author
 *  06/03/2015 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_unmap_bitmap_init(fbe_provision_drive_t *  provision_drive_p)
{
    fbe_lba_t zero_checkpoint = 0;
    fbe_u32_t bitmap_size;


    if (!fbe_provision_drive_unmap_bitmap_user_enabled) {
        return FBE_STATUS_OK;
    }

    fbe_provision_drive_unmap_bitmap_get_size(provision_drive_p, &bitmap_size);

    fbe_provision_drive_lock(provision_drive_p);
    if (provision_drive_p->unmap_bitmap.is_initialized) {
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_STATUS_OK;
    }

    provision_drive_p->unmap_bitmap.bitmap = (fbe_u8_t *)fbe_memory_allocate_required(bitmap_size);
    if (provision_drive_p->unmap_bitmap.bitmap == NULL) {
        fbe_provision_drive_unlock(provision_drive_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }


    fbe_zero_memory(provision_drive_p->unmap_bitmap.bitmap, bitmap_size);
    provision_drive_p->unmap_bitmap.is_initialized = FBE_TRUE;
    /* Get the curent zero checkpoint. */
    fbe_provision_drive_metadata_get_background_zero_checkpoint(provision_drive_p, &zero_checkpoint);
    provision_drive_p->unmap_bitmap.is_enabled = (zero_checkpoint == FBE_LBA_INVALID) ? FBE_TRUE : FBE_FALSE;

    fbe_provision_drive_unlock(provision_drive_p);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_unmap_bitmap_init()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_unmap_bitmap_destroy()
 ******************************************************************************
 * @brief
 *  This function deallocates unmap bitmap.
 *
 * @param provision_drive_p    - pointer to the provision drive.  
 *
 * @return fbe_status_t        - status of the operation.
 *
 * @author
 *  06/03/2015 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_unmap_bitmap_destroy(fbe_provision_drive_t * provision_drive_p)
{
    if (provision_drive_p->unmap_bitmap.bitmap) {
        fbe_memory_release_required(provision_drive_p->unmap_bitmap.bitmap);
        provision_drive_p->unmap_bitmap.bitmap = NULL;
    }

    provision_drive_p->unmap_bitmap.is_initialized = FBE_FALSE;
    provision_drive_p->unmap_bitmap.is_enabled = FBE_FALSE;

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_unmap_bitmap_destroy()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_unmap_bitmap_is_lba_range_mapped()
 ******************************************************************************
 * @brief
 *  This function checks the bitmap if all the blocks are mapped.
 *
 * @param provision_drive_p    - pointer to the provision drive.  
 * @param lba                  - start lba.  
 * @param block_count          - block count.  
 *
 * @return fbe_bool_t          - return true if all the blocks are mapped.
 *
 * @author
 *  06/03/2015 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_bool_t
fbe_provision_drive_unmap_bitmap_is_lba_range_mapped(fbe_provision_drive_t * provision_drive_p, 
                                                     fbe_lba_t lba, 
                                                     fbe_block_count_t block_count)
{
    fbe_u32_t start_bit_offset, end_bit_offset, i;
    fbe_u32_t byte_offset, bit_offset;
    fbe_u8_t *byte_ptr;
    fbe_lba_t exported_capacity;

    if (!fbe_provision_drive_is_unmap_supported(provision_drive_p)) {
        return FBE_TRUE;
    }

    if (!fbe_provision_drive_unmap_bitmap_is_enabled(provision_drive_p)) {
        return FBE_FALSE;
    }

    if (block_count == 0) {
        return FBE_TRUE;
    }

    /* Assume paged metadata itself is always mapped */
    fbe_base_config_get_capacity((fbe_base_config_t *)provision_drive_p, &exported_capacity);
    if (lba >= exported_capacity) {
        return FBE_TRUE;
    }

    start_bit_offset = (fbe_u32_t)lba/FBE_PROVISION_DRIVE_UNMAP_BITMAP_BLOCKS_PER_BIT;
    end_bit_offset = (fbe_u32_t)(lba + block_count - 1)/FBE_PROVISION_DRIVE_UNMAP_BITMAP_BLOCKS_PER_BIT;

    for (i = start_bit_offset; i <= end_bit_offset; i++)
    {
        byte_offset = i / 8;
        bit_offset = i % 8;
        byte_ptr = &provision_drive_p->unmap_bitmap.bitmap[byte_offset];

        if ((*byte_ptr & (1 << bit_offset)) == 0) {
            return FBE_FALSE;
        }
    }

    return FBE_TRUE;
}
/******************************************************************************
 * end fbe_provision_drive_unmap_bitmap_is_lba_range_mapped()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_unmap_bitmap_callback_entry()
 ******************************************************************************
 * @brief
 *  This is the callback entry for updating unmap bitmap.
 *
 * @param mde                  - Pointer to metadata element.  
 * @param mdo                  - pointer to the metadata operation.  
 * @param start_lba            - start lba.  
 * @param block_count          - block count.  
 *
 * @return fbe_status_t        - status of the operation.
 *
 * @author
 *  06/03/2015 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_unmap_bitmap_callback_entry(fbe_metadata_paged_cache_action_t action,
                                                  fbe_metadata_element_t * mde,
                                                  fbe_payload_metadata_operation_t * mdo,
                                                  fbe_lba_t start_lba, 
                                                  fbe_block_count_t block_count)
{
    fbe_provision_drive_t * provision_drive_p;
    fbe_status_t status = FBE_STATUS_OK;

    provision_drive_p = (fbe_provision_drive_t *)fbe_base_config_metadata_element_to_base_config_object(mde);
    if (!fbe_provision_drive_unmap_bitmap_user_enabled) {
        return FBE_STATUS_OK;
    }

    if (!fbe_provision_drive_is_unmap_supported(provision_drive_p)) {
        return FBE_STATUS_OK;
    }

    switch (action) {
        case FBE_METADATA_PAGED_CACHE_ACTION_PRE_READ:
            break;
        case FBE_METADATA_PAGED_CACHE_ACTION_POST_READ:
        case FBE_METADATA_PAGED_CACHE_ACTION_POST_WRITE:
            status = fbe_provision_drive_unmap_bitmap_update(provision_drive_p, mdo);
            break;
        case FBE_METADATA_PAGED_CACHE_ACTION_INVALIDATE:
            break;
        case FBE_METADATA_PAGED_CACHE_ACTION_NONPAGE_CHANGED:
            status = fbe_provision_drive_unmap_bitmap_check_nonpaged(provision_drive_p);
            break;
        default:
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                  FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: invalid action: 0x%x\n", __FUNCTION__, action);
            break;
    }

    
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_unmap_bitmap_callback_entry()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_unmap_bitmap_is_mapped_in_disk()
 ******************************************************************************
 * @brief
 *  This function checks the paged metadata if all the blocks are mapped.
 *
 * @param provision_drive_p    - pointer to the provision drive.  
 * @param sg_ptr               - SG pointer.  
 * @param slot_offset          - offset to the page.  
 *
 * @return fbe_bool_t          - return true if all the blocks are mapped.
 *
 * @author
 *  06/03/2015 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_bool_t
fbe_provision_drive_unmap_bitmap_is_mapped_in_disk(fbe_provision_drive_t * provision_drive_p, 
                                                   fbe_sg_element_t * sg_ptr,
                                                   fbe_u32_t slot_offset)
{
    fbe_provision_drive_paged_metadata_t * paged_metadata_p;
    fbe_u32_t i;

    paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + slot_offset);
    for (i = 0; i < FBE_PROVISION_DRIVE_UNMAP_BITMAP_CHUNKS_PER_BIT; i++)
    {
        if (!fbe_provision_drive_unmap_bitmap_evaluate_metadata(paged_metadata_p))
        {
            return FBE_FALSE;
        }

        paged_metadata_p++;
        if (paged_metadata_p == (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + FBE_METADATA_BLOCK_DATA_SIZE))
        {
            sg_ptr++;
            paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)sg_ptr->address;
        }
    }

    return FBE_TRUE;
}
/******************************************************************************
 * end fbe_provision_drive_unmap_bitmap_is_mapped_in_disk()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_unmap_bitmap_update()
 ******************************************************************************
 * @brief
 *  This function updates the unmap bitmap when the metadata read/write finishes.
 *  We only update necessary area, although we could read/write more pages.
 *
 * @param provision_drive_p    - pointer to the provision drive.  
 * @param mdo                  - pointer to the metadata operation.  
 *
 * @return fbe_status_t        - status of the operation.
 *
 * @author
 *  06/03/2015 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_unmap_bitmap_update(fbe_provision_drive_t * provision_drive_p, 
                                          fbe_payload_metadata_operation_t * mdo)
{
    fbe_sg_element_t * sg_ptr, *sg_list;
    fbe_lba_t lba_offset;
    fbe_u32_t slot_offset;
    fbe_u32_t start_chunk, chunk_count, end_chunk;
    fbe_u32_t i;
    fbe_u32_t io_start_chunk, bitmap_start_chunk;
    fbe_u32_t start_bit_offset, end_bit_offset;
    fbe_bool_t is_mapped_in_disk, is_mapped_in_bitmap;

    if (!provision_drive_p->unmap_bitmap.is_enabled) {
        return FBE_STATUS_OK;
    }

    start_chunk = (fbe_u32_t)(mdo->u.metadata_callback.offset / mdo->u.metadata_callback.record_data_size);
    chunk_count = (fbe_u32_t) mdo->u.metadata_callback.repeat_count;
    end_chunk = start_chunk + chunk_count - 1;
    io_start_chunk = (fbe_u32_t)((mdo->u.metadata_callback.start_lba) * FBE_METADATA_BLOCK_DATA_SIZE / sizeof(fbe_provision_drive_paged_metadata_t));
    if (io_start_chunk > start_chunk) {
        start_chunk = io_start_chunk;
        chunk_count = end_chunk - start_chunk + 1;
    }

    start_bit_offset = (start_chunk / FBE_PROVISION_DRIVE_UNMAP_BITMAP_CHUNKS_PER_BIT);
    end_bit_offset = (end_chunk / FBE_PROVISION_DRIVE_UNMAP_BITMAP_CHUNKS_PER_BIT);
    bitmap_start_chunk = (start_chunk / FBE_PROVISION_DRIVE_UNMAP_BITMAP_CHUNKS_PER_BIT) * FBE_PROVISION_DRIVE_UNMAP_BITMAP_CHUNKS_PER_BIT;
    for (i = start_bit_offset; i <= end_bit_offset; i++) {
        lba_offset = (bitmap_start_chunk * sizeof(fbe_provision_drive_paged_metadata_t)) / FBE_METADATA_BLOCK_DATA_SIZE;
        if (lba_offset < mdo->u.metadata_callback.start_lba) {
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                  FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: No enough data start: 0x%x end 0x%x\n", __FUNCTION__, start_bit_offset, end_bit_offset);
        }
        slot_offset = (bitmap_start_chunk * sizeof(fbe_provision_drive_paged_metadata_t)) % FBE_METADATA_BLOCK_DATA_SIZE;
        sg_list = mdo->u.metadata_callback.sg_list;
        sg_ptr = &sg_list[lba_offset - mdo->u.metadata_callback.start_lba];

        if (sg_ptr->count == 0)
        {
            return FBE_STATUS_OK;
        }

        /* Check the metadata pages to see whether it is mapped already */
        is_mapped_in_disk = fbe_provision_drive_unmap_bitmap_is_mapped_in_disk(provision_drive_p, sg_ptr, slot_offset);
        is_mapped_in_bitmap = fbe_provision_drive_unmap_bitmap_is_lba_range_mapped(provision_drive_p, start_chunk * FBE_PROVISION_DRIVE_CHUNK_SIZE, 1);
        if (is_mapped_in_disk && !is_mapped_in_bitmap) 
        {
            fbe_provision_drive_utils_trace(provision_drive_p,
                                            FBE_TRACE_LEVEL_INFO,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            FBE_PROVISION_DRIVE_DEBUG_FLAG_PAGED_CACHE,
                                            "%s SET bit %d\n", __FUNCTION__, i);
            fbe_provision_drive_unmap_bitmap_set_bit(provision_drive_p, i);
        } 
        else if (!is_mapped_in_disk && is_mapped_in_bitmap) 
        {
            fbe_provision_drive_utils_trace(provision_drive_p,
                                            FBE_TRACE_LEVEL_INFO,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            FBE_PROVISION_DRIVE_DEBUG_FLAG_PAGED_CACHE,
                                            "%s CLEAR bit %d\n", __FUNCTION__, i);
            fbe_provision_drive_unmap_bitmap_clear_bit(provision_drive_p, i);
        }

        bitmap_start_chunk += FBE_PROVISION_DRIVE_UNMAP_BITMAP_CHUNKS_PER_BIT;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_unmap_bitmap_update()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_unmap_bitmap_invalidate()
 ******************************************************************************
 * @brief
 *  This function checks the bitmap if all the blocks are mapped.
 *
 * @param provision_drive_p    - pointer to the provision drive.  
 * @param lba                  - start lba.  
 * @param block_count          - block count.  
 *
 * @return fbe_bool_t          - return true if all the blocks are mapped.
 *
 * @author
 *  06/03/2015 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_unmap_bitmap_invalidate(fbe_provision_drive_t * provision_drive_p, 
                                            fbe_lba_t lba, 
                                            fbe_block_count_t block_count)
{
    fbe_u32_t start_bit_offset, end_bit_offset, i;
    fbe_u32_t bitmap_size;

    if (!fbe_provision_drive_is_unmap_supported(provision_drive_p)) {
        return FBE_STATUS_OK;
    }

    if (!fbe_provision_drive_unmap_bitmap_is_enabled(provision_drive_p)) {
        return FBE_STATUS_OK;
    }

    fbe_provision_drive_unmap_bitmap_get_size(provision_drive_p, &bitmap_size);

    if (lba == FBE_LBA_INVALID) {
        fbe_zero_memory(provision_drive_p->unmap_bitmap.bitmap, bitmap_size);
        return FBE_STATUS_OK;
    }

    start_bit_offset = (fbe_u32_t)lba/FBE_PROVISION_DRIVE_UNMAP_BITMAP_BLOCKS_PER_BIT;
    end_bit_offset = (fbe_u32_t)(lba + block_count - 1)/FBE_PROVISION_DRIVE_UNMAP_BITMAP_BLOCKS_PER_BIT;

    for (i = start_bit_offset; i <= end_bit_offset; i++)
    {
        fbe_provision_drive_unmap_bitmap_clear_bit(provision_drive_p, i);
    }

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_PAGED_CACHE,
                                    "%s invalidated bits from %d to %d\n", __FUNCTION__, start_bit_offset, end_bit_offset);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_unmap_bitmap_invalidate()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_unmap_bitmap_check_nonpaged()
 ******************************************************************************
 * @brief
 *  This function checks nonpaged metadata (checkpoint) to see whether the bitmap.
 *  need to be enabled.
 *
 * @param provision_drive_p    - pointer to the provision drive.  
 *
 * @return fbe_status_t        - status of the operation.
 *
 * @author
 *  06/03/2015 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_unmap_bitmap_check_nonpaged(fbe_provision_drive_t * provision_drive_p)
{
    fbe_status_t status;
    fbe_lba_t zero_checkpoint;
    fbe_u32_t bitmap_size;

    fbe_provision_drive_unmap_bitmap_get_size(provision_drive_p, &bitmap_size);

    /* Get the curent zero checkpoint. */
    status = fbe_provision_drive_metadata_get_background_zero_checkpoint(provision_drive_p, &zero_checkpoint);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Failed to get checkpoint. Status: 0x%X\n", __FUNCTION__, status);

        return FBE_STATUS_OK;
    }

    /* We enable the unmap bitmap only when the zeroing is finished
     * This is to avoid the invalidation of bitmap in dualsp case.
     * An optimization could be: the bitmap befor the zero checkpoint can be valid. */
    fbe_provision_drive_lock(provision_drive_p);
    if (provision_drive_p->unmap_bitmap.is_enabled && (zero_checkpoint != FBE_LBA_INVALID)) 
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: disabled. checkpoint: 0x%llx\n", __FUNCTION__, zero_checkpoint);
        provision_drive_p->unmap_bitmap.is_enabled = FBE_FALSE;
    }
    else if (!provision_drive_p->unmap_bitmap.is_enabled && (zero_checkpoint == FBE_LBA_INVALID))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: enabled. checkpoint: 0x%llx\n", __FUNCTION__, zero_checkpoint);
        fbe_zero_memory(provision_drive_p->unmap_bitmap.bitmap, bitmap_size);
        provision_drive_p->unmap_bitmap.is_enabled = FBE_TRUE;
    }

    fbe_provision_drive_unlock(provision_drive_p);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_unmap_bitmap_check_nonpaged()
 ******************************************************************************/


/***************************************************************************
 * end fbe_provision_drive_unmap_bitmap.c
 ***************************************************************************/

