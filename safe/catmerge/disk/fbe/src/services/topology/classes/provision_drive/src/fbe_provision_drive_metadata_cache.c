/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2014
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_provision_drive_metadata_cache.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the provision drive object's paged metadata cache 
 *  related functionality.
 * 
 * @ingroup provision_drive_class_files
 * 
 * @version
 *   05/03/2014:  Created. Lili Chen
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_provision_drive_private.h"
#include "fbe_transport_memory.h"
#include "fbe_database.h"

static fbe_bool_t fbe_provision_drive_paged_metadata_cache_enabled = FBE_TRUE;

/*************************
 *   FORWARD DECLARATIONS
 *************************/
static __forceinline fbe_status_t fbe_provision_drive_metadata_cache_invalidate_slot_region(fbe_provision_drive_t * provision_drive_p,
	                                                      fbe_provision_drive_metadata_cache_slot_t * slot_ptr,
                                                          fbe_u32_t start_chunk,
                                                          fbe_u32_t chunk_count);
static fbe_status_t fbe_provision_drive_metadata_cache_bgz_callback(fbe_metadata_paged_cache_action_t action,
                                                             fbe_payload_metadata_operation_t * mdo);
static fbe_status_t fbe_provision_drive_metadata_cache_lookup_bgz_cache(fbe_payload_metadata_operation_t * mdo);
static fbe_status_t fbe_provision_drive_metadata_cache_update_bgz_cache(fbe_payload_metadata_operation_t * mdo);
static fbe_status_t fbe_provision_drive_metadata_cache_invalidate_bgz_cache(fbe_metadata_element_t *mde,
                                                                            fbe_lba_t start_lba,
                                                                            fbe_block_count_t block_count);
static fbe_status_t fbe_provision_drive_metadata_cache_invalidate_bgz_cache_by_region(fbe_provision_drive_t * provision_drive_p, 
                                                                                      fbe_u32_t start_chunk, 
                                                                                      fbe_u32_t chunk_count);
static fbe_status_t fbe_provision_drive_metadata_cache_udpate_to_cache_only(fbe_payload_metadata_operation_t * mdo);
static __forceinline void fbe_provision_drive_metadata_cache_clear_flush(fbe_provision_drive_t * provision_drive_p);
static __forceinline fbe_u32_t fbe_provision_drive_metadata_cache_get_flush_start(fbe_provision_drive_t * provision_drive_p);


/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_lock()
 ******************************************************************************
 * @brief
 *  This function is to lock the paged metadata cache area.
 *
 * @param slot_ptr             - pointer to a slot.  
 *
 * @return fbe_status_t        - status of the operation.
 *
 * @author
 *  05/03/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline fbe_status_t
fbe_provision_drive_metadata_cache_lock(fbe_provision_drive_t *  provision_drive_p)
{
    csx_p_spin_pointer_lock(&provision_drive_p->paged_metadata_cache.cache_lock);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_lock()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_unlock()
 ******************************************************************************
 * @brief
 *  This function is to unlock the paged metadata cache area.
 *
 * @param slot_ptr             - pointer to a slot.  
 *
 * @return fbe_status_t        - status of the operation.
 *
 * @author
 *  05/03/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline fbe_status_t
fbe_provision_drive_metadata_cache_unlock(fbe_provision_drive_t *  provision_drive_p)
{
    csx_p_spin_pointer_unlock(&provision_drive_p->paged_metadata_cache.cache_lock);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_unlock()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_bgz_lock()
 ******************************************************************************
 * @brief
 *  This function is to lock the paged metadata cache bgz slot.
 *
 * @param slot_ptr             - pointer to a slot.  
 *
 * @return fbe_status_t        - status of the operation.
 *
 * @author
 *  10/17/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline fbe_status_t
fbe_provision_drive_metadata_cache_bgz_lock(fbe_provision_drive_t *  provision_drive_p)
{
    csx_p_spin_pointer_lock(&provision_drive_p->paged_metadata_cache.bgz_cache_lock);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_bgz_lock()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_bgz_unlock()
 ******************************************************************************
 * @brief
 *  This function is to unlock the paged metadata cache bgz slot.
 *
 * @param slot_ptr             - pointer to a slot.  
 *
 * @return fbe_status_t        - status of the operation.
 *
 * @author
 *  10/17/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline fbe_status_t
fbe_provision_drive_metadata_cache_bgz_unlock(fbe_provision_drive_t *  provision_drive_p)
{
    csx_p_spin_pointer_unlock(&provision_drive_p->paged_metadata_cache.bgz_cache_lock);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_bgz_unlock()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_dump_slot_data()
 ******************************************************************************
 * @brief
 *  This function is to invalidate a paged MD cache slot.
 *
 * @param slot_ptr             - pointer to a slot.  
 *
 * @return fbe_status_t        - status of the operation.
 *
 * @author
 *  05/03/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline fbe_status_t
fbe_provision_drive_metadata_cache_dump_slot_data(fbe_provision_drive_t *  provision_drive_p,
                                                  fbe_provision_drive_metadata_cache_slot_t * slot_ptr,
                                                  fbe_trace_level_t trace_level)
{
    fbe_provision_drive_utils_trace(provision_drive_p,
                                    trace_level,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_PAGED_CACHE,
                                    "slot %p start 0x%x data %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                                    slot_ptr, slot_ptr->start_chunk, 
                                    slot_ptr->cached_data[0], slot_ptr->cached_data[1],  slot_ptr->cached_data[2], slot_ptr->cached_data[3],
                                    slot_ptr->cached_data[4], slot_ptr->cached_data[5],  slot_ptr->cached_data[6], slot_ptr->cached_data[7],
                                    slot_ptr->cached_data[8], slot_ptr->cached_data[9],  slot_ptr->cached_data[10], slot_ptr->cached_data[11],
                                    slot_ptr->cached_data[12], slot_ptr->cached_data[13],  slot_ptr->cached_data[14], slot_ptr->cached_data[15]);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_dump_slot_data()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_invalidate_slot()
 ******************************************************************************
 * @brief
 *  This function is to invalidate a paged MD cache slot.
 *
 * @param slot_ptr             - pointer to a slot.  
 *
 * @return fbe_status_t        - status of the operation.
 *
 * @author
 *  05/03/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline fbe_status_t
fbe_provision_drive_metadata_cache_invalidate_slot(fbe_provision_drive_metadata_cache_slot_t * slot_ptr)
{
    slot_ptr->start_chunk = 0xFFFFFFFF;

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_invalidate_slot()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_is_slot_invalid()
 ******************************************************************************
 * @brief
 *  This function is used to to check whether a paged MD cache slot is invalid.
 *
 * @param slot_ptr             - pointer to a slot.  
 *
 * @return fbe_bool_t          - TRUE if the slot is invalid.
 *
 * @author
 *  05/03/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline fbe_bool_t
fbe_provision_drive_metadata_cache_is_slot_invalid(fbe_provision_drive_metadata_cache_slot_t * slot_ptr)
{
    return (slot_ptr->start_chunk == 0xFFFFFFFF);
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_is_slot_invalid()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_is_region_overlap()
 ******************************************************************************
 * @brief
 *  This function is to check whether the region overlap with a paged MD cache slot.
 *
 * @param slot_ptr             - pointer to a slot.  
 * @param start_chunk          - start chunk.  
 * @param chunk_count          - chunk count.  
 *
 * @return fbe_bool_t          - TRUE if the slot is invalid.
 *
 * @author
 *  05/03/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline fbe_bool_t
fbe_provision_drive_metadata_cache_is_region_overlap(fbe_provision_drive_metadata_cache_slot_t * slot_ptr,
                                                     fbe_u32_t start_chunk,
                                                     fbe_u32_t chunk_count)
{
    fbe_u32_t end_chunk;
    fbe_u32_t slot_start, slot_end;

    if (fbe_provision_drive_metadata_cache_is_slot_invalid(slot_ptr)) {
        return FBE_FALSE;
    }

    end_chunk = (start_chunk + chunk_count - 1);
    slot_start = slot_ptr->start_chunk;
    slot_end = (slot_ptr->start_chunk + FBE_PROVISION_DRIVE_METADATA_CACHE_SLOT_SIZE - 1);

    if ((start_chunk > slot_end) || (end_chunk < slot_start)) {
        return FBE_FALSE;
    }

    return FBE_TRUE;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_is_region_overlap()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_get_free_slot()
 ******************************************************************************
 * @brief
 *  This function is used to get a free slot in paged MD cache.
 *
 * @param provision_drive_p             - Provision drive object.  
 *
 * @return cache_slot
 *
 * @author
 *  05/03/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline fbe_provision_drive_metadata_cache_slot_t *
fbe_provision_drive_metadata_cache_get_free_slot(fbe_provision_drive_t *  provision_drive_p)
{
    fbe_u32_t i;
    fbe_provision_drive_metadata_cache_slot_t *slot_ptr;

    slot_ptr = &provision_drive_p->paged_metadata_cache.slots[0];
    for (i = 0; i < FBE_PROVISION_DRIVE_METADATA_CACHE_MAX_SLOTS; i++)
    {
        if (fbe_provision_drive_metadata_cache_is_slot_invalid(slot_ptr)) {
            return slot_ptr;
        }
        slot_ptr++;
    }
    return NULL;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_get_free_slot()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_get_lru_slot()
 ******************************************************************************
 * @brief
 *  This function is used to get a lru slot in paged MD cache.
 *
 * @param provision_drive_p             - Provision drive object.  
 *
 * @return cache_slot
 *
 * @author
 *  05/03/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline fbe_provision_drive_metadata_cache_slot_t *
fbe_provision_drive_metadata_cache_get_lru_slot(fbe_provision_drive_t *  provision_drive_p)
{
    fbe_u32_t i;
    fbe_provision_drive_metadata_cache_slot_t *slot_ptr, *lru_slot;
    fbe_u64_t last_io = provision_drive_p->paged_metadata_cache.io_count;

    lru_slot = slot_ptr = &provision_drive_p->paged_metadata_cache.slots[0];
    for (i = 1; i < FBE_PROVISION_DRIVE_METADATA_CACHE_MAX_SLOTS; i++)
    {
        slot_ptr++;
        if ((last_io - slot_ptr->last_io) > (last_io - lru_slot->last_io)) {
            lru_slot = slot_ptr;
        }
    }

    /* If the LSU slot hasn't been used for 100 IOs, return it */
    if ((last_io - lru_slot->last_io) > 100) {
        fbe_provision_drive_metadata_cache_invalidate_slot(lru_slot);
        return lru_slot;
    } else {
        return NULL;
    }
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_get_lru_slot()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_invalidate_overlapping_slots()
 ******************************************************************************
 * @brief
 *  This function is used to invalidate any slot that overlaps with input slot.
 *
 * @param provision_drive_p             - Pointer to provision drive.  
 * @param in_slot_ptr                   - Pointer to a slot.  
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  05/03/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_metadata_cache_invalidate_overlapping_slots(fbe_provision_drive_t * provision_drive_p,
                                                                fbe_provision_drive_metadata_cache_slot_t * in_slot_ptr)
{
    fbe_u32_t i;
    fbe_provision_drive_metadata_cache_slot_t *slot_ptr;

    slot_ptr = &provision_drive_p->paged_metadata_cache.slots[0];
    for (i = 0; i < FBE_PROVISION_DRIVE_METADATA_CACHE_MAX_SLOTS; i++)
    {
        if (slot_ptr == in_slot_ptr) {
            slot_ptr++;
            continue;
        }

        if (fbe_provision_drive_metadata_cache_is_region_overlap(slot_ptr, in_slot_ptr->start_chunk, FBE_PROVISION_DRIVE_METADATA_CACHE_SLOT_SIZE)) {
            /* Invalidate overlapped slots */
            fbe_provision_drive_metadata_cache_invalidate_slot(slot_ptr);

            fbe_provision_drive_utils_trace(provision_drive_p,
                                            FBE_TRACE_LEVEL_DEBUG_LOW,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            FBE_PROVISION_DRIVE_DEBUG_FLAG_PAGED_CACHE,
                                            "PAGED CACHE invalidate overlappd slot %d\n", i);
        }
        slot_ptr++;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_invalidate_overlapping_slots()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_evaluate_one_chunk()
 ******************************************************************************
 * @brief
 *  This function is to evaluate a chunk and change the cache if needed.
 *  We set the bit in cached data when:
 *    1) valid bit is set;
 *    2) NZ and UZ are set;
 *    3) consumed bit is set;
 *
 * @param paged_metadata_p              - Pointer to paged metadata.  
 * @param slot_ptr                      - Pointer to the slot.  
 * @param chunk_offset                  - chunk offset.  
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  05/03/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline fbe_status_t
fbe_provision_drive_metadata_cache_evaluate_one_chunk(fbe_provision_drive_paged_metadata_t * paged_metadata_p,
                                                       fbe_provision_drive_metadata_cache_slot_t *slot_ptr,
                                                       fbe_u32_t chunk_offset)
{
    fbe_u32_t byte_offset, bit_offset;
    fbe_u8_t *byte_ptr;

    byte_offset = chunk_offset / 8;
    bit_offset = chunk_offset % 8;
    byte_ptr = &slot_ptr->cached_data[byte_offset];
    if (paged_metadata_p->valid_bit && 
        !(paged_metadata_p->need_zero_bit) && 
        !(paged_metadata_p->user_zero_bit) && 
        paged_metadata_p->consumed_user_data_bit)
    {
        *byte_ptr |= (1 << bit_offset);
    }
    else
    {
        *byte_ptr &= ~(1 << bit_offset);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_evaluate_one_chunk()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_update_slot()
 ******************************************************************************
 * @brief
 *  This function is used to handle the block tranport operation without lock,
 *  calls this routine to process the i/o operation.
 *
 * @param provision_drive_p             - Provision drive object.  
 * @param slot_ptr                      - Pointer to the cache slot.  
 * @param sg_list                       - Pointer to sg list.  
 * @param slot_offset                   - slot offset.  
 * @param start_chunk                   - start chunk.  
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  05/03/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline fbe_status_t
fbe_provision_drive_metadata_cache_update_slot(fbe_provision_drive_t * provision_drive_p,
                                               fbe_provision_drive_metadata_cache_slot_t *slot_ptr,
                                               fbe_sg_element_t * sg_list, 
                                               fbe_u32_t slot_offset,
                                               fbe_u32_t start_chunk)
{
    fbe_u32_t i = 0;
    fbe_sg_element_t * sg_ptr = sg_list;
    fbe_provision_drive_paged_metadata_t * paged_metadata_p;

    fbe_zero_memory(slot_ptr->cached_data, FBE_PROVISION_DRIVE_METADATA_CACHE_SLOT_SIZE_IN_BYTES);

    paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + slot_offset);

    while ((sg_ptr->address != NULL) && (i < FBE_PROVISION_DRIVE_METADATA_CACHE_SLOT_SIZE))
    {
        fbe_provision_drive_metadata_cache_evaluate_one_chunk(paged_metadata_p, slot_ptr, i);
        i++;
        paged_metadata_p++;
        if (paged_metadata_p == (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + FBE_METADATA_BLOCK_DATA_SIZE))
        {
            sg_ptr++;
            paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)sg_ptr->address;
        }
    }

    /* Must be the last one to update */
    slot_ptr->start_chunk = start_chunk;

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_PAGED_CACHE,
                                    "PAGED CACHE update slot\n");
    fbe_provision_drive_metadata_cache_dump_slot_data(provision_drive_p, slot_ptr, FBE_TRACE_LEVEL_DEBUG_LOW);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_update_slot()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_update_slot_region()
 ******************************************************************************
 * @brief
 *  This function is to invalidate a paged MD cache slot.
 *
 * @param provision_drive_p             - Provision drive object.  
 * @param slot_ptr                      - Pointer to the cache slot.  
 * @param sg_list                       - Pointer to sg list.  
 * @param slot_offset                   - slot offset.  
 * @param start_chunk                   - start chunk.  
 *
 * @return fbe_status_t        - status of the operation.
 *
 * @author
 *  05/03/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline fbe_status_t
fbe_provision_drive_metadata_cache_update_slot_region(fbe_provision_drive_t * provision_drive_p,
                                                      fbe_provision_drive_metadata_cache_slot_t * slot_ptr,
                                                      fbe_sg_element_t * sg_ptr,
                                                      fbe_u32_t slot_offset,
                                                      fbe_u32_t start_chunk,
                                                      fbe_u32_t chunk_count)
{
    fbe_u32_t end_chunk;
    fbe_u32_t slot_start, slot_end;
    fbe_u32_t i;
    fbe_provision_drive_paged_metadata_t * paged_metadata_p;

    end_chunk = (start_chunk + chunk_count - 1);
    slot_start = slot_ptr->start_chunk;
    slot_end = (slot_ptr->start_chunk + FBE_PROVISION_DRIVE_METADATA_CACHE_SLOT_SIZE - 1);

    if ((start_chunk > slot_end) || (end_chunk < slot_start)) {
        /* No overlap */
        return FBE_STATUS_OK;
    }

    /* Adjust to the start chunk of this update */
    if (start_chunk < slot_start) {
        fbe_u32_t chunks_to_adjust = slot_start - start_chunk;
        while (chunks_to_adjust >= (FBE_METADATA_BLOCK_DATA_SIZE - slot_offset)/sizeof(fbe_provision_drive_paged_metadata_t)) {
            sg_ptr++;
            if (sg_ptr->address == NULL)
            {
                return FBE_STATUS_OK;
            }
            chunks_to_adjust -= (FBE_METADATA_BLOCK_DATA_SIZE - slot_offset)/sizeof(fbe_provision_drive_paged_metadata_t);
            slot_offset = 0;
        }
        paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + slot_offset + chunks_to_adjust * sizeof(fbe_provision_drive_paged_metadata_t));
        i = slot_start;
    } else {
        paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + slot_offset);
        i = start_chunk;
    }

    while ((sg_ptr->address != NULL) && (i <= end_chunk) && (i <= slot_end))
    {
        fbe_provision_drive_metadata_cache_evaluate_one_chunk(paged_metadata_p, slot_ptr, i - slot_start);

        i++;
        paged_metadata_p++;
        if (paged_metadata_p == (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + FBE_METADATA_BLOCK_DATA_SIZE))
        {
            sg_ptr++;
            paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)sg_ptr->address;
        }
    }

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_PAGED_CACHE,
                                    "PAGED CACHE update region slot %p start 0x%x count 0x%x\n",
                                    slot_ptr, start_chunk, chunk_count);
    fbe_provision_drive_metadata_cache_dump_slot_data(provision_drive_p, slot_ptr, FBE_TRACE_LEVEL_DEBUG_LOW);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_update_slot_region()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_update()
 ******************************************************************************
 * @brief
 *  This function is to update paged MD cache when a read or write to paged MD
 *  finishes.
 *
 * @param mdo                           - Pointer to metadata operation.  
 * @param is_read                       - set if it is a read.  
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  05/03/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_metadata_cache_update(fbe_payload_metadata_operation_t * mdo,
                                          fbe_bool_t is_read)
{
    fbe_status_t status;
    fbe_provision_drive_metadata_cache_slot_t *slot_ptr = NULL;
    fbe_sg_element_t * sg_ptr, *sg_list;
    fbe_lba_t lba_offset;
    fbe_u32_t slot_offset;
    fbe_u32_t start_chunk, chunk_count, end_chunk;
    fbe_bool_t slot_updated = FBE_FALSE;
    fbe_u32_t i;
    fbe_provision_drive_t * provision_drive_p;
    fbe_lba_t paged_metadata_start_lba;
    fbe_u32_t io_start_chunk;
    fbe_atomic_t last_io;

    provision_drive_p = (fbe_provision_drive_t *)fbe_base_config_metadata_element_to_base_config_object(mdo->metadata_element);
    if (!fbe_provision_drive_metadata_cache_is_enabled(provision_drive_p)) {
        return FBE_STATUS_OK;
    }

    fbe_base_config_metadata_get_paged_record_start_lba((fbe_base_config_t *) provision_drive_p,
                                                    &paged_metadata_start_lba);

    start_chunk = (fbe_u32_t)(mdo->u.metadata_callback.offset / mdo->u.metadata_callback.record_data_size);
    chunk_count = (fbe_u32_t) mdo->u.metadata_callback.repeat_count;
    end_chunk = start_chunk + chunk_count - 1;
    io_start_chunk = (fbe_u32_t)((mdo->u.metadata_callback.start_lba) * FBE_METADATA_BLOCK_DATA_SIZE / sizeof(fbe_provision_drive_paged_metadata_t));
    if (io_start_chunk > start_chunk) {
        start_chunk = io_start_chunk;
        chunk_count = end_chunk - start_chunk + 1;
    }
    lba_offset = (start_chunk * sizeof(fbe_provision_drive_paged_metadata_t)) / FBE_METADATA_BLOCK_DATA_SIZE;
    slot_offset = (start_chunk * sizeof(fbe_provision_drive_paged_metadata_t)) % FBE_METADATA_BLOCK_DATA_SIZE;
    sg_list = mdo->u.metadata_callback.sg_list;
    sg_ptr = &sg_list[lba_offset - mdo->u.metadata_callback.start_lba];

    /* First invalidate BGZ cache slot if needed */
	if (!is_read) {
        fbe_provision_drive_metadata_cache_invalidate_bgz_cache_by_region(provision_drive_p, start_chunk, chunk_count);
    }

    fbe_provision_drive_metadata_cache_lock(provision_drive_p);

    /* Increase the total IO count */
    last_io = fbe_atomic_increment(&provision_drive_p->paged_metadata_cache.io_count);

    /* Find slots to update */
    slot_ptr = &provision_drive_p->paged_metadata_cache.slots[0];
    for (i = 0; i < FBE_PROVISION_DRIVE_METADATA_CACHE_MAX_SLOTS; i++)
    {
        if (fbe_provision_drive_metadata_cache_is_region_overlap(slot_ptr, start_chunk, chunk_count)) {
            fbe_provision_drive_metadata_cache_update_slot_region(provision_drive_p, 
                                                                  slot_ptr,
                                                                  sg_ptr,
                                                                  slot_offset,
                                                                  start_chunk,
																  chunk_count);
            slot_ptr->last_io = last_io;
            slot_updated = FBE_TRUE;
        }
        slot_ptr++;
    }

    if (slot_updated) {
        fbe_provision_drive_metadata_cache_unlock(provision_drive_p);
        return FBE_STATUS_OK;
    }

    slot_ptr = fbe_provision_drive_metadata_cache_get_free_slot(provision_drive_p);
    if (slot_ptr == NULL) {
        slot_ptr = fbe_provision_drive_metadata_cache_get_lru_slot(provision_drive_p);
    }

    if (slot_ptr == NULL) {
        fbe_provision_drive_metadata_cache_unlock(provision_drive_p);
        return FBE_STATUS_OK;
    }

    /* Update the slot */
    status = fbe_provision_drive_metadata_cache_update_slot(provision_drive_p,
                                                            slot_ptr,
                                                            sg_ptr,
                                                            slot_offset,
                                                            start_chunk);
    if (status == FBE_STATUS_OK) {
        slot_ptr->last_io = last_io;
        fbe_provision_drive_metadata_cache_invalidate_overlapping_slots(provision_drive_p, 
                                                                        slot_ptr);
    }

    fbe_provision_drive_metadata_cache_unlock(provision_drive_p);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_update()
 ******************************************************************************/


/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_check_slot()
 ******************************************************************************
 * @brief
 *  This function is to check a slot in cached data.
 *
 * @param slot_ptr                      - Pointer to a cache slot.  
 * @param start_chunk                   - start chunk.  
 * @param chunk_count                   - chunk count.  
 *
 * @return fbe_status_t                 - return OK if NO ACTION is needed.
 *
 * @author
 *  05/03/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline fbe_status_t
fbe_provision_drive_metadata_cache_check_slot(fbe_provision_drive_metadata_cache_slot_t *slot_ptr,
                                              fbe_u32_t start_chunk,
                                              fbe_u32_t chunk_count)
{
    fbe_u32_t i;
    fbe_u32_t byte_offset, bit_offset;

    for (i = start_chunk; i < start_chunk + chunk_count; i++) {
        /* We do not have data cached */
        byte_offset = (i - slot_ptr->start_chunk) / 8;
        bit_offset = (i - slot_ptr->start_chunk) % 8;
		if ((slot_ptr->cached_data[byte_offset] & (1 << bit_offset)) == 0) {
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_check_slot()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_lookup_slot()
 ******************************************************************************
 * @brief
 *  This function is used to lookup a fully overlapped slot in paged MD cache.
 *
 * @param provision_drive_p             - Provision drive object.  
 * @param start_chunk                   - start chunk.  
 * @param chunk_count                   - chunk count.  
 * @param slot_ptr                      - slot to copy to.  
 *
 * @return cache_slot
 *
 * @author
 *  05/03/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline fbe_status_t
fbe_provision_drive_metadata_cache_lookup_slot(fbe_provision_drive_t *  provision_drive_p,
                                               fbe_u32_t start_chunk,
                                               fbe_u32_t chunk_count)
{
    fbe_status_t status;
    fbe_u32_t i = 0;
    fbe_provision_drive_metadata_cache_slot_t *slot_ptr;
    fbe_u32_t slot_start, slot_end;
    fbe_u32_t end_chunk = start_chunk + chunk_count - 1;
    fbe_u32_t volatile *start_chunk_ptr;
    fbe_atomic_t last_io;

    /* Increase the total IO count */
    last_io = fbe_atomic_increment(&provision_drive_p->paged_metadata_cache.io_count);

    slot_ptr = &provision_drive_p->paged_metadata_cache.slots[0];
    while (i < FBE_PROVISION_DRIVE_METADATA_CACHE_MAX_SLOTS)
    {
        if (fbe_provision_drive_metadata_cache_is_slot_invalid(slot_ptr)) {
            i++;
            slot_ptr++;
            continue;
        }

        start_chunk_ptr = &slot_ptr->start_chunk;
        slot_start = *start_chunk_ptr;
        slot_end = slot_start + FBE_PROVISION_DRIVE_METADATA_CACHE_SLOT_SIZE - 1;
        if ((start_chunk >= slot_start) && (end_chunk <= slot_end)) {
            status = fbe_provision_drive_metadata_cache_check_slot(slot_ptr, start_chunk, chunk_count);
            /* We do not obtain locks for read lookups.
             * So we check the start_chunk again after checking the cached data. If two values are the same,
             * it means the slot has not been changed by other threads and we could use the result.
             */
            if (slot_start == *start_chunk_ptr) {
                slot_ptr->last_io = last_io;
                if (status == FBE_STATUS_OK) {
                    fbe_provision_drive_utils_trace(provision_drive_p,
                                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                                FBE_TRACE_MESSAGE_ID_INFO,
                                                FBE_PROVISION_DRIVE_DEBUG_FLAG_PAGED_CACHE,
                                                "PAGED CACHE hit slot %p start 0x%x count 0x%x\n",
                                                slot_ptr, start_chunk, chunk_count);
                    fbe_provision_drive_metadata_cache_dump_slot_data(provision_drive_p, slot_ptr, FBE_TRACE_LEVEL_DEBUG_LOW);
                }

                return status;
            } else {
                continue;
            }
        }

        i++;
        slot_ptr++;
    }
    return FBE_STATUS_GENERIC_FAILURE;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_lookup_slot()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_lookup()
 ******************************************************************************
 * @brief
 *  This function is used to search the paged MD cache to check whether the caller
 *  need further action.
 *
 * @param provision_drive_p             - Provision drive object.  
 * @param mdo                           - Pointer to metadata operation.  
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  05/03/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_cache_lookup(fbe_payload_metadata_operation_t * mdo)
{
    fbe_status_t status;
    fbe_u32_t start_chunk, chunk_count;
    fbe_provision_drive_t * provision_drive_p;

    provision_drive_p = (fbe_provision_drive_t *)fbe_base_config_metadata_element_to_base_config_object(mdo->metadata_element);

    if (!fbe_provision_drive_metadata_cache_is_enabled(provision_drive_p)) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get info from mdo */
    start_chunk = (fbe_u32_t)(mdo->u.metadata_callback.offset / mdo->u.metadata_callback.record_data_size);
    chunk_count = (fbe_u32_t) mdo->u.metadata_callback.repeat_count;

    /* Find a slot to check */
    status = fbe_provision_drive_metadata_cache_lookup_slot(provision_drive_p, start_chunk, chunk_count);
    if (status != FBE_STATUS_OK) {
        /* We do not have data cached */
        provision_drive_p->paged_metadata_cache.miss_count++;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    provision_drive_p->paged_metadata_cache.hit_count++;
    return FBE_STATUS_OK;       
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_lookup()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_invalidte_one_chunk()
 ******************************************************************************
 * @brief
 *  This function is to evaluate a chunk and change the cache if needed.
 *
 * @param paged_metadata_p              - Pointer to paged metadata.  
 * @param slot_ptr                      - Pointer to the slot.  
 * @param chunk_offset                  - chunk offset.  
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  05/03/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline fbe_status_t
fbe_provision_drive_metadata_cache_invalidte_one_chunk(fbe_provision_drive_metadata_cache_slot_t *slot_ptr,
                                                       fbe_u32_t chunk_offset)
{
    fbe_u32_t byte_offset, bit_offset;

    byte_offset = chunk_offset / 8;
    bit_offset = chunk_offset % 8;
    slot_ptr->cached_data[byte_offset] &= ~(1 << bit_offset);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_invalidte_one_chunk()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_invalidate_slot_region()
 ******************************************************************************
 * @brief
 *  This function is to invalidate a paged MD cache slot.
 *
 * @param slot_ptr             - pointer to a slot.  
 * @param start_chunk          - start chunk.  
 * @param chunk_count          - chunk count.  
 *
 * @return fbe_status_t        - status of the operation.
 *
 * @author
 *  05/03/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline fbe_status_t
fbe_provision_drive_metadata_cache_invalidate_slot_region(fbe_provision_drive_t * provision_drive_p,
	                                                      fbe_provision_drive_metadata_cache_slot_t * slot_ptr,
                                                          fbe_u32_t start_chunk,
                                                          fbe_u32_t chunk_count)
{
    fbe_u32_t end_chunk;
    fbe_u32_t slot_start, slot_end;
    fbe_u32_t i;
    fbe_u32_t start, end;

    if (fbe_provision_drive_metadata_cache_is_slot_invalid(slot_ptr)) {
        return FBE_STATUS_OK;
    }

    end_chunk = (start_chunk + chunk_count - 1);
    slot_start = slot_ptr->start_chunk;
    slot_end = (slot_ptr->start_chunk + FBE_PROVISION_DRIVE_METADATA_CACHE_SLOT_SIZE - 1);

    if ((start_chunk <= slot_start) && (end_chunk >= slot_end)) {
        /* Full overlap, invalidate whole slot */
        fbe_provision_drive_metadata_cache_invalidate_slot(slot_ptr);
        return FBE_STATUS_OK;
    }

    start = FBE_MAX(start_chunk, slot_start);
    end = FBE_MIN(end_chunk, slot_end);

    if (start > end) {
        /* No overlap */
        return FBE_STATUS_OK;
    }

    if ((end - slot_start) > FBE_PROVISION_DRIVE_METADATA_CACHE_SLOT_SIZE) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: invalidate start: 0x%x end 0x%x\n", __FUNCTION__, start, end);
    }

    for (i = start; i <= end; i++) {
        fbe_provision_drive_metadata_cache_invalidte_one_chunk(slot_ptr, i - slot_start);
    }

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_PAGED_CACHE,
                                    "PAGED CACHE invalidate region slot %p start 0x%x end 0x%x\n",
                                    slot_ptr, start, end);
    fbe_provision_drive_metadata_cache_dump_slot_data(provision_drive_p, slot_ptr, FBE_TRACE_LEVEL_DEBUG_LOW);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_invalidate_slot_region()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_invalidate_region()
 ******************************************************************************
 * @brief
 *  This function is used to initialize pvd md cache structure.
 *
 * @param mde                           - Pointer to metadata element.  
 * @param start_lba                     - start lba.  
 * @param block_count                   - block count.  
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  05/03/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_metadata_cache_invalidate_region(fbe_metadata_element_t *mde,
                                                     fbe_lba_t start_lba,
                                                     fbe_block_count_t block_count)
{
    fbe_u32_t i;
    fbe_provision_drive_metadata_cache_slot_t *slot_ptr;
    fbe_provision_drive_t * provision_drive_p;
    fbe_lba_t paged_metadata_start_lba;
    fbe_u32_t start_chunk, chunk_count;

    provision_drive_p = (fbe_provision_drive_t *)fbe_base_config_metadata_element_to_base_config_object(mde);
    if (!fbe_provision_drive_metadata_cache_is_enabled(provision_drive_p)) {
        return FBE_STATUS_OK;
    }

    if (start_lba == FBE_LBA_INVALID) {
        fbe_provision_drive_metadata_cache_lock(provision_drive_p);
        slot_ptr = &provision_drive_p->paged_metadata_cache.slots[0];
        for (i = 0; i < FBE_PROVISION_DRIVE_METADATA_CACHE_MAX_SLOTS; i++)
        {
            fbe_provision_drive_metadata_cache_invalidate_slot(slot_ptr);
            slot_ptr++;
        }
        fbe_provision_drive_metadata_cache_unlock(provision_drive_p);
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_DEBUG_LOW,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_PAGED_CACHE,
                                        "PAGED CACHE invalidate all slots\n");
        return FBE_STATUS_OK;
    }

    fbe_base_config_metadata_get_paged_record_start_lba((fbe_base_config_t *) provision_drive_p,
                                                        &paged_metadata_start_lba);
    start_chunk = (fbe_u32_t)((start_lba - paged_metadata_start_lba) * FBE_METADATA_BLOCK_DATA_SIZE / sizeof(fbe_provision_drive_paged_metadata_t));
    chunk_count = (fbe_u32_t)(block_count * FBE_METADATA_BLOCK_DATA_SIZE / sizeof(fbe_provision_drive_paged_metadata_t));

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_PAGED_CACHE,
                                    "PAGED CACHE invalidate region start: 0x%x count: 0x%x\n",
                                    start_chunk, chunk_count);

    fbe_provision_drive_metadata_cache_lock(provision_drive_p);
    slot_ptr = &provision_drive_p->paged_metadata_cache.slots[0];
    for (i = 0; i < FBE_PROVISION_DRIVE_METADATA_CACHE_MAX_SLOTS; i++)
    {
        fbe_provision_drive_metadata_cache_invalidate_slot_region(provision_drive_p, slot_ptr, start_chunk, chunk_count);
        slot_ptr++;
    }
    fbe_provision_drive_metadata_cache_unlock(provision_drive_p);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_invalidate_region()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_init()
 ******************************************************************************
 * @brief
 *  This function is used to initialize pvd md cache structure.
 *
 * @param provision_drive_p             - Provision drive object.  
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  05/03/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_metadata_cache_init(fbe_provision_drive_t *  provision_drive_p)
{
    fbe_u32_t i;
    fbe_provision_drive_metadata_cache_slot_t *slot_ptr;

    provision_drive_p->paged_metadata_cache.io_count = 0;
    provision_drive_p->paged_metadata_cache.miss_count = 0;
    provision_drive_p->paged_metadata_cache.hit_count = 0;

    provision_drive_p->paged_metadata_cache.cache_lock = &provision_drive_p->paged_metadata_cache;
    provision_drive_p->paged_metadata_cache.bgz_cache_lock = &provision_drive_p->paged_metadata_cache;

    slot_ptr = &provision_drive_p->paged_metadata_cache.slots[0];
    for (i = 0; i < FBE_PROVISION_DRIVE_METADATA_CACHE_MAX_SLOTS; i++)
    {
        fbe_provision_drive_metadata_cache_invalidate_slot(slot_ptr);
        slot_ptr->last_io = 0;
        slot_ptr++;
    }

    fbe_provision_drive_metadata_cache_invalidate_slot(&provision_drive_p->paged_metadata_cache.bgz_slot);
    fbe_provision_drive_metadata_cache_clear_flush(provision_drive_p);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_init()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_destroy()
 ******************************************************************************
 * @brief
 *  This function is used to destroy pvd md cache structure.
 *
 * @param provision_drive_p             - Provision drive object.  
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  05/03/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_metadata_cache_destroy(fbe_provision_drive_t *  provision_drive_p)
{
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_destroy()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_init()
 ******************************************************************************
 * @brief
 *  This function is used to initialize pvd md cache structure.
 *
 * @param provision_drive_p             - Provision drive object.  
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  05/03/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_bool_t
fbe_provision_drive_metadata_cache_is_enabled(fbe_provision_drive_t *  provision_drive_p)
{
    return (fbe_provision_drive_paged_metadata_cache_enabled);
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_init()
 ******************************************************************************/


/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_callback_entry()
 ******************************************************************************
 * @brief
 *  This function is used to initialize pvd md cache structure.
 *
 * @param mde                           - Pointer to metadata element.  
 * @param start_lba                     - start lba.  
 * @param block_count                   - block count.  
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  05/03/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_metadata_cache_callback_entry(fbe_metadata_paged_cache_action_t action,
                                                  fbe_metadata_element_t * mde,
                                                  fbe_payload_metadata_operation_t * mdo,
                                                  fbe_lba_t start_lba, fbe_block_count_t block_count)
{
    fbe_provision_drive_t * provision_drive_p;
    fbe_status_t status = FBE_STATUS_OK;

    provision_drive_p = (fbe_provision_drive_t *)fbe_base_config_metadata_element_to_base_config_object(mde);
    if (!fbe_provision_drive_metadata_cache_is_enabled(provision_drive_p)) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (mdo && (mdo->u.metadata_callback.operation_flags & FBE_PAYLOAD_METADATA_OPERATION_PAGED_USE_BGZ_CACHE)) {
        return fbe_provision_drive_metadata_cache_bgz_callback(action, mdo);
    }

    switch (action) {
        case FBE_METADATA_PAGED_CACHE_ACTION_PRE_READ:
            status = fbe_provision_drive_metadata_cache_lookup(mdo);
            break;
        case FBE_METADATA_PAGED_CACHE_ACTION_POST_READ:
            status = fbe_provision_drive_metadata_cache_update(mdo, FBE_TRUE);
            break;
        case FBE_METADATA_PAGED_CACHE_ACTION_POST_WRITE:
            status = fbe_provision_drive_metadata_cache_update(mdo, FBE_FALSE);
            break;
        case FBE_METADATA_PAGED_CACHE_ACTION_INVALIDATE:
            status = fbe_provision_drive_metadata_cache_invalidate_region(mde, start_lba, block_count);
            status = fbe_provision_drive_metadata_cache_invalidate_bgz_cache(mde, start_lba, block_count);
            break;
        case FBE_METADATA_PAGED_CACHE_ACTION_NONPAGE_CHANGED:
            break;
        default:
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                  FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: invalid action: 0x%x\n", __FUNCTION__, action);
            break;
    }

    /* Inform unmap bitmap callback handler. */
    fbe_provision_drive_unmap_bitmap_callback_entry(action, mde, mdo, start_lba, block_count);

    return status;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_callback_entry()
 ******************************************************************************/


/************************ Functions for BGZ cache slot *************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_callback_entry()
 ******************************************************************************
 * @brief
 *  This function is used to initialize pvd md cache structure.
 *
 * @param mde                           - Pointer to metadata element.  
 * @param start_lba                     - start lba.  
 * @param block_count                   - block count.  
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  09/25/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_metadata_cache_bgz_callback(fbe_metadata_paged_cache_action_t action,
                                                  fbe_payload_metadata_operation_t * mdo)
{
    fbe_provision_drive_t * provision_drive_p;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    provision_drive_p = (fbe_provision_drive_t *)fbe_base_config_metadata_element_to_base_config_object(mdo->metadata_element);
    switch (action) {
        case FBE_METADATA_PAGED_CACHE_ACTION_PRE_READ:
            status = fbe_provision_drive_metadata_cache_lookup_bgz_cache(mdo);
            break;
        case FBE_METADATA_PAGED_CACHE_ACTION_POST_READ:
            status = fbe_provision_drive_metadata_cache_update_bgz_cache(mdo);
            break;
        case FBE_METADATA_PAGED_CACHE_ACTION_POST_WRITE:
            status = fbe_provision_drive_metadata_cache_update(mdo, FBE_FALSE);
            if (mdo->u.metadata_callback.operation_flags & FBE_PAYLOAD_METADATA_OPERATION_PAGED_USE_BGZ_CACHE) {
                fbe_provision_drive_metadata_cache_bgz_lock(provision_drive_p);
                fbe_provision_drive_metadata_cache_invalidate_slot(&provision_drive_p->paged_metadata_cache.bgz_slot);
                fbe_provision_drive_metadata_cache_clear_flush(provision_drive_p);
                fbe_provision_drive_metadata_cache_bgz_unlock(provision_drive_p);
            }
            break;
        case FBE_METADATA_PAGED_CACHE_ACTION_PRE_UPDATE:
            status = fbe_provision_drive_metadata_cache_udpate_to_cache_only(mdo);
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
 * end fbe_provision_drive_metadata_cache_callback_entry()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_lookup_bgz_cache()
 ******************************************************************************
 * @brief
 *  This function is used to search the paged MD cache to check whether the caller
 *  need further action.
 *
 * @param provision_drive_p             - Provision drive object.  
 * @param mdo                           - Pointer to metadata operation.  
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  09/25/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_metadata_cache_lookup_bgz_cache(fbe_payload_metadata_operation_t * mdo)
{
    fbe_status_t status;
    fbe_u32_t start_chunk, chunk_count, end_chunk;
    fbe_provision_drive_t * provision_drive_p;
    fbe_provision_drive_metadata_cache_slot_t *slot_ptr;
    fbe_u32_t slot_start, slot_end;

    provision_drive_p = (fbe_provision_drive_t *)fbe_base_config_metadata_element_to_base_config_object(mdo->metadata_element);
    slot_ptr = &provision_drive_p->paged_metadata_cache.bgz_slot;

    if (!fbe_provision_drive_metadata_cache_is_enabled(provision_drive_p)) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* The slot is not valid yet */
    if (fbe_provision_drive_metadata_cache_is_slot_invalid(slot_ptr)) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get info from mdo */
    start_chunk = (fbe_u32_t)(mdo->u.metadata_callback.offset / mdo->u.metadata_callback.record_data_size);
    chunk_count = (fbe_u32_t) mdo->u.metadata_callback.repeat_count;
    end_chunk = (start_chunk + chunk_count - 1);

    slot_start = slot_ptr->start_chunk;
    slot_end = slot_start + FBE_PROVISION_DRIVE_METADATA_CACHE_SLOT_SIZE - 1;
    if ((start_chunk >= slot_start) && (end_chunk <= slot_end)) {
        status = fbe_provision_drive_metadata_cache_check_slot(slot_ptr, start_chunk, chunk_count);
        /* If the slot is invalidated, we'd better read the paged again. */
        if (!fbe_provision_drive_metadata_cache_is_slot_invalid(slot_ptr) && (status == FBE_STATUS_OK)) {
            fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_DEBUG_LOW,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_PAGED_CACHE,
                                        "PAGED CACHE BGZ hit start 0x%x count 0x%x\n",
                                        start_chunk, chunk_count);
            return FBE_STATUS_OK;       
        } else {
            return FBE_STATUS_GENERIC_FAILURE;       
        }
    }

    return FBE_STATUS_GENERIC_FAILURE;       
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_lookup_bgz_cache()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_evaluate_one_chunk_for_bgz()
 ******************************************************************************
 * @brief
 *  This function is to evaluate a chunk for background zeroing.
 *  We set the bit in cached data when:
 *    1) valid bit is set;
 *    2) NZ is set, or:
 *    3) UZ is set and consumed bit is set;
 *
 * @param paged_metadata_p              - Pointer to paged metadata.  
 * @param slot_ptr                      - Pointer to the slot.  
 * @param chunk_offset                  - chunk offset.  
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  09/25/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline fbe_status_t
fbe_provision_drive_metadata_cache_evaluate_one_chunk_for_bgz(fbe_provision_drive_paged_metadata_t * paged_metadata_p,
                                                       fbe_provision_drive_metadata_cache_slot_t *slot_ptr,
                                                       fbe_u32_t chunk_offset)
{
    fbe_u32_t byte_offset, bit_offset;
    fbe_u8_t *byte_ptr;

    byte_offset = chunk_offset / 8;
    bit_offset = chunk_offset % 8;
    byte_ptr = &slot_ptr->cached_data[byte_offset];
    if (paged_metadata_p->valid_bit && 
        (paged_metadata_p->need_zero_bit || 
         paged_metadata_p->user_zero_bit && paged_metadata_p->consumed_user_data_bit))
    {
        *byte_ptr |= (1 << bit_offset);
    }
    else
    {
        *byte_ptr &= ~(1 << bit_offset);
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_evaluate_one_chunk_for_bgz()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_update_bgz_slot()
 ******************************************************************************
 * @brief
 *  This function is used to handle the block tranport operation without lock,
 *  calls this routine to process the i/o operation.
 *
 * @param provision_drive_p             - Provision drive object.  
 * @param sg_list                       - Pointer to sg list.  
 * @param slot_offset                   - slot offset.  
 * @param start_chunk                   - start chunk.  
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  09/25/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline fbe_status_t
fbe_provision_drive_metadata_cache_update_bgz_slot(fbe_provision_drive_t * provision_drive_p,
                                               fbe_sg_element_t * sg_list, 
                                               fbe_u32_t slot_offset,
                                               fbe_u32_t start_chunk)
{
    fbe_provision_drive_metadata_cache_slot_t *slot_ptr;
    fbe_u32_t i = 0;
    fbe_sg_element_t * sg_ptr = sg_list;
    fbe_provision_drive_paged_metadata_t * paged_metadata_p;

    slot_ptr = &provision_drive_p->paged_metadata_cache.bgz_slot;
    fbe_zero_memory(slot_ptr->cached_data, FBE_PROVISION_DRIVE_METADATA_CACHE_SLOT_SIZE_IN_BYTES);

    paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + slot_offset);

    while ((sg_ptr->address != NULL) && (i < FBE_PROVISION_DRIVE_METADATA_CACHE_SLOT_SIZE))
    {
        fbe_provision_drive_metadata_cache_evaluate_one_chunk_for_bgz(paged_metadata_p, slot_ptr, i);
        i++;
        paged_metadata_p++;
        if (paged_metadata_p == (fbe_provision_drive_paged_metadata_t *)(sg_ptr->address + FBE_METADATA_BLOCK_DATA_SIZE))
        {
            sg_ptr++;
            paged_metadata_p = (fbe_provision_drive_paged_metadata_t *)sg_ptr->address;
        }
    }

    /* Must be the last one to update */
    slot_ptr->start_chunk = start_chunk;

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_PAGED_CACHE,
                                    "PAGED CACHE update BGZ slot\n");
    fbe_provision_drive_metadata_cache_dump_slot_data(provision_drive_p, slot_ptr, FBE_TRACE_LEVEL_DEBUG_LOW);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_update_bgz_slot()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_update_bgz_cache()
 ******************************************************************************
 * @brief
 *  This function is to update paged MD cache when a read or write to paged MD
 *  finishes.
 *
 * @param mdo                           - Pointer to metadata operation.  
 * @param is_read                       - set if it is a read.  
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  09/25/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_metadata_cache_update_bgz_cache(fbe_payload_metadata_operation_t * mdo)
{
    fbe_status_t status;
    fbe_sg_element_t * sg_ptr, *sg_list;
    fbe_lba_t lba_offset;
    fbe_u32_t slot_offset;
    fbe_u32_t start_chunk, chunk_count;
    fbe_provision_drive_t * provision_drive_p;

    provision_drive_p = (fbe_provision_drive_t *)fbe_base_config_metadata_element_to_base_config_object(mdo->metadata_element);
    if (!fbe_provision_drive_metadata_cache_is_enabled(provision_drive_p)) {
        return FBE_STATUS_OK;
    }

    start_chunk = (fbe_u32_t)(mdo->u.metadata_callback.offset / mdo->u.metadata_callback.record_data_size);
    chunk_count = (fbe_u32_t) mdo->u.metadata_callback.repeat_count;
    lba_offset = (start_chunk * sizeof(fbe_provision_drive_paged_metadata_t)) / FBE_METADATA_BLOCK_DATA_SIZE;
    slot_offset = (start_chunk * sizeof(fbe_provision_drive_paged_metadata_t)) % FBE_METADATA_BLOCK_DATA_SIZE;
    sg_list = mdo->u.metadata_callback.sg_list;
    sg_ptr = &sg_list[lba_offset - mdo->u.metadata_callback.start_lba];

    fbe_provision_drive_metadata_cache_bgz_lock(provision_drive_p);
    /* Update the slot */
    status = fbe_provision_drive_metadata_cache_update_bgz_slot(provision_drive_p,
                                                                sg_ptr,
                                                                slot_offset,
                                                                start_chunk);

    if (fbe_provision_drive_metadata_cache_is_flush_valid(provision_drive_p)) {
        if (start_chunk > fbe_provision_drive_metadata_cache_get_flush_start(provision_drive_p)) {
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                  FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: start_chunk moved forward!\n", __FUNCTION__);
        }
        fbe_provision_drive_metadata_cache_clear_flush(provision_drive_p);
    }
    fbe_provision_drive_metadata_cache_bgz_unlock(provision_drive_p);

    return status;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_update_bgz_cache()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_invalidate_bgz_slot()
 ******************************************************************************
 * @brief
 *  This function is used to invalidate the bgz cache slot and reset zero checkpoint.
 *
 * @param provision_drive_p             - Provision drive object.  
 * @param force_flush                   - Whether we need to flush cached data.  
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  09/25/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_metadata_cache_invalidate_bgz_slot(fbe_provision_drive_t * provision_drive_p,
                                                       fbe_bool_t force_flush)
{
    fbe_provision_drive_metadata_cache_slot_t *slot_ptr;
    fbe_bool_t b_is_system_drive;
    fbe_object_id_t object_id;
    fbe_provision_drive_config_type_t config_type;

    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
    b_is_system_drive = fbe_database_is_object_system_pvd(object_id);
    fbe_provision_drive_get_config_type(provision_drive_p, &config_type);

    slot_ptr = &provision_drive_p->paged_metadata_cache.bgz_slot;

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_PAGED_CACHE,
                                    "PAGED CACHE invalidate BGZ slot force_flush 0x%x\n", force_flush);

    fbe_provision_drive_metadata_cache_bgz_lock(provision_drive_p);
    fbe_provision_drive_metadata_cache_invalidate_slot(slot_ptr);

    if (force_flush || 
        !b_is_system_drive && (config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED)) 
    {
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *)provision_drive_p, FBE_PROVISION_DRIVE_LIFECYCLE_COND_RESET_CHECKPOINT);
        fbe_provision_drive_metadata_cache_clear_flush(provision_drive_p);
    }
    fbe_provision_drive_metadata_cache_bgz_unlock(provision_drive_p);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_invalidate_bgz_slot()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_invalidate_bgz_cache_by_region()
 ******************************************************************************
 * @brief
 *  This function is used to invalidate part of the bgz cache slot.
 *
 * @param provision_drive_p             - Provision drive object.  
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  09/25/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_metadata_cache_invalidate_bgz_cache_by_region(fbe_provision_drive_t * provision_drive_p, 
                                                                  fbe_u32_t start_chunk, 
                                                                  fbe_u32_t chunk_count)
{
    fbe_provision_drive_metadata_cache_slot_t *slot_ptr;
    slot_ptr = &provision_drive_p->paged_metadata_cache.bgz_slot;

    fbe_provision_drive_metadata_cache_bgz_lock(provision_drive_p);

    if (fbe_provision_drive_metadata_cache_is_region_overlap(slot_ptr, start_chunk, chunk_count)) {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_DEBUG_LOW,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_PAGED_CACHE,
                                        "PAGED CACHE invalidate BGZ slot start: 0x%x count: 0x%x, slot_start 0x%x\n",
                                        start_chunk, chunk_count, slot_ptr->start_chunk);

        /* Invalidate part of the slot */
        fbe_provision_drive_metadata_cache_invalidate_slot_region(provision_drive_p, slot_ptr, start_chunk, chunk_count);
    }

    fbe_provision_drive_metadata_cache_bgz_unlock(provision_drive_p);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_invalidate_bgz_cache_by_region()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_invalidate_bgz_cache()
 ******************************************************************************
 * @brief
 *  This function is used to initialize pvd md cache structure.
 *
 * @param mde                           - Pointer to metadata element.  
 * @param start_lba                     - start lba.  
 * @param block_count                   - block count.  
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  09/25/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_metadata_cache_invalidate_bgz_cache(fbe_metadata_element_t *mde,
                                                        fbe_lba_t start_lba,
                                                        fbe_block_count_t block_count)
{
    fbe_provision_drive_t * provision_drive_p;
    fbe_lba_t paged_metadata_start_lba;
    fbe_u32_t start_chunk, chunk_count;

    provision_drive_p = (fbe_provision_drive_t *)fbe_base_config_metadata_element_to_base_config_object(mde);
    if (!fbe_provision_drive_metadata_cache_is_enabled(provision_drive_p)) {
        return FBE_STATUS_OK;
    }

    if (start_lba == FBE_LBA_INVALID) {
        fbe_provision_drive_metadata_cache_invalidate_bgz_slot(provision_drive_p, FBE_FALSE);
        return FBE_STATUS_OK;
    }

    fbe_base_config_metadata_get_paged_record_start_lba((fbe_base_config_t *) provision_drive_p,
                                                        &paged_metadata_start_lba);
    start_chunk = (fbe_u32_t)((start_lba - paged_metadata_start_lba) * FBE_METADATA_BLOCK_DATA_SIZE / sizeof(fbe_provision_drive_paged_metadata_t));
    chunk_count = (fbe_u32_t)(block_count * FBE_METADATA_BLOCK_DATA_SIZE / sizeof(fbe_provision_drive_paged_metadata_t));

    fbe_provision_drive_metadata_cache_invalidate_bgz_cache_by_region(provision_drive_p, start_chunk, chunk_count);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_invalidate_bgz_cache()
 ******************************************************************************/


typedef struct fbe_provision_drive_metadata_cache_flush_s
{
    fbe_u32_t           start_chunk;
    fbe_u32_t           chunk_count;
}fbe_provision_drive_metadata_cache_flush_t;


/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_get_flush_start()
 ******************************************************************************
 * @brief
 *  This function is used to get the start chunk which needs to be flushed to
 *  disk.
 *
 * @param provision_drive_p             - Provision drive object.  
 *
 * @return start_chunk
 *
 * @author
 *  09/25/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline fbe_u32_t
fbe_provision_drive_metadata_cache_get_flush_start(fbe_provision_drive_t * provision_drive_p)
{
    fbe_provision_drive_metadata_cache_flush_t * flush_p;

    flush_p = (fbe_provision_drive_metadata_cache_flush_t *)&provision_drive_p->paged_metadata_cache.bgz_slot.last_io;
    return (flush_p->start_chunk);
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_get_flush_start()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_clear_flush()
 ******************************************************************************
 * @brief
 *  This function clears flush structure.
 *
 * @param provision_drive_p             - Provision drive object.  
 *
 * @return NULL
 *
 * @author
 *  09/25/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline void
fbe_provision_drive_metadata_cache_clear_flush(fbe_provision_drive_t * provision_drive_p)
{
    fbe_provision_drive_metadata_cache_flush_t * flush_p;
    flush_p = (fbe_provision_drive_metadata_cache_flush_t *)&provision_drive_p->paged_metadata_cache.bgz_slot.last_io;
    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_PAGED_CACHE,
                                    "PAGED CACHE clear BGZ flush start: 0x%x count: 0x%x\n",
                                    flush_p->start_chunk, flush_p->chunk_count);

    flush_p->start_chunk = 0xFFFFFFFF;
    flush_p->chunk_count = 0;
    
    return;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_clear_flush()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_is_flush_valid()
 ******************************************************************************
 * @brief
 *  This function checks whether we are using the flush structure now.
 *
 * @param provision_drive_p             - Provision drive object.  
 *
 * @return fbe_bool_t
 *
 * @author
 *  09/25/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_bool_t
fbe_provision_drive_metadata_cache_is_flush_valid(fbe_provision_drive_t * provision_drive_p)
{
    fbe_provision_drive_metadata_cache_flush_t * flush_p;

    flush_p = (fbe_provision_drive_metadata_cache_flush_t *)&provision_drive_p->paged_metadata_cache.bgz_slot.last_io;
    return (flush_p->start_chunk != 0xFFFFFFFF);
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_is_flush_valid()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_add_to_flush()
 ******************************************************************************
 * @brief
 *  This function remembers in flush structure whether the chunks need to be 
 *  flushed to disk later.
 *
 * @param provision_drive_p             - Provision drive object.  
 * @param start_lba                     - start lba.  
 * @param block_count                   - block count.  
 *
 * @return NULL
 *
 * @author
 *  09/25/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline void
fbe_provision_drive_metadata_cache_add_to_flush(fbe_provision_drive_t * provision_drive_p, fbe_u32_t start_chunk, fbe_u32_t chunk_count)
{
    fbe_provision_drive_metadata_cache_slot_t *slot_ptr;
    fbe_provision_drive_metadata_cache_flush_t * flush_p;

    slot_ptr = &provision_drive_p->paged_metadata_cache.bgz_slot;
    flush_p = (fbe_provision_drive_metadata_cache_flush_t *)&(slot_ptr->last_io);

    if (flush_p->start_chunk == 0xFFFFFFFF) {
        flush_p->start_chunk = start_chunk;
    }
    /* Checkpoint may be changed */
    else if (start_chunk <= flush_p->start_chunk) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: flush start changed from 0x%x to 0x%x\n", __FUNCTION__, flush_p->start_chunk, start_chunk);
        flush_p->start_chunk = start_chunk;
        flush_p->chunk_count = 0;
    }
    else if (start_chunk < (flush_p->start_chunk + flush_p->chunk_count)) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: flush count changed from 0x%x to 0x%x\n", __FUNCTION__, flush_p->start_chunk, start_chunk - flush_p->start_chunk);
        flush_p->chunk_count = start_chunk - flush_p->start_chunk;
    }

    flush_p->chunk_count += chunk_count;
    
    return;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_add_to_flush()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_need_to_flush()
 ******************************************************************************
 * @brief
 *  This function checks whether we need to flush the paged MD to disk.
 *
 * @param provision_drive_p             - Provision drive object.  
 * @param start_lba                     - start lba.  
 * @param block_count                   - block count.  
 *
 * @return fbe_bool_t
 *
 * @author
 *  09/25/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static __forceinline fbe_bool_t
fbe_provision_drive_metadata_cache_need_to_flush(fbe_provision_drive_t * provision_drive_p, fbe_u32_t start_chunk, fbe_u32_t chunk_count)
{
    fbe_provision_drive_metadata_cache_slot_t *slot_ptr;
    fbe_provision_drive_metadata_cache_flush_t * flush_p;
    fbe_u32_t end_chunk, slot_start, slot_end;
    fbe_status_t status;

    slot_ptr = &provision_drive_p->paged_metadata_cache.bgz_slot;
    flush_p = (fbe_provision_drive_metadata_cache_flush_t *)&(slot_ptr->last_io);
    
    end_chunk = (start_chunk + chunk_count - 1);
    slot_start = slot_ptr->start_chunk;
    slot_end = slot_start + FBE_PROVISION_DRIVE_METADATA_CACHE_SLOT_SIZE - 1;

    if (start_chunk < slot_start || start_chunk > slot_end)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: start_chunk not fall into the region!\n", __FUNCTION__);
        return FBE_TRUE;
    }

    /* If we don't have enough bits for next request, start flush */
    if (end_chunk + chunk_count > slot_end)
    {
        return FBE_TRUE;
    }

    /* If in next request needs a disk read, flush now */
    status = fbe_provision_drive_metadata_cache_check_slot(slot_ptr, start_chunk + chunk_count, chunk_count);
    if (status != FBE_STATUS_OK)
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_need_to_flush()
 ******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_metadata_cache_udpate_to_cache_only()
 ******************************************************************************
 * @brief
 *  This function is to update BGZ cache only for an update.
 *  finishes.
 *
 * @param mdo                           - Pointer to metadata operation.  
 * @param is_read                       - set if it is a read.  
 *
 * @return fbe_status_t                 - status of the operation.
 *
 * @author
 *  09/25/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_metadata_cache_udpate_to_cache_only(fbe_payload_metadata_operation_t * mdo)
{
    fbe_sg_element_t * sg_ptr, *sg_list;
    fbe_lba_t lba_offset;
    fbe_u32_t slot_offset;
    fbe_u32_t start_chunk, chunk_count, end_chunk;
    fbe_u32_t slot_start, slot_end;
    fbe_provision_drive_t * provision_drive_p;
    fbe_provision_drive_metadata_cache_slot_t *slot_ptr;

    provision_drive_p = (fbe_provision_drive_t *)fbe_base_config_metadata_element_to_base_config_object(mdo->metadata_element);
    slot_ptr = &provision_drive_p->paged_metadata_cache.bgz_slot;

    start_chunk = (fbe_u32_t)(mdo->u.metadata_callback.offset / mdo->u.metadata_callback.record_data_size);
    chunk_count = (fbe_u32_t) mdo->u.metadata_callback.repeat_count;
    end_chunk = (start_chunk + chunk_count - 1);
    lba_offset = (start_chunk * sizeof(fbe_provision_drive_paged_metadata_t)) / FBE_METADATA_BLOCK_DATA_SIZE;
    slot_offset = (start_chunk * sizeof(fbe_provision_drive_paged_metadata_t)) % FBE_METADATA_BLOCK_DATA_SIZE;
    sg_list = mdo->u.metadata_callback.sg_list;
    sg_ptr = &sg_list[lba_offset - mdo->u.metadata_callback.start_lba];

    fbe_provision_drive_metadata_cache_bgz_lock(provision_drive_p);
    /* First check the chunks should be in the cache */
    slot_start = slot_ptr->start_chunk;
    slot_end = slot_start + FBE_PROVISION_DRIVE_METADATA_CACHE_SLOT_SIZE - 1;
    if ((start_chunk < slot_start) || (end_chunk > slot_end)) {
        fbe_provision_drive_metadata_cache_bgz_unlock(provision_drive_p);
        /* The chunks are not fully inside the cache region */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If we reach the end of page, or end of the disk, or end of the cache slot, we need to update to disk */
    if (!(mdo->u.metadata_callback.operation_flags & FBE_PAYLOAD_METADATA_OPERATION_FLAGS_NO_PERSIST) ||
        fbe_provision_drive_metadata_cache_need_to_flush(provision_drive_p, start_chunk, chunk_count))
    {
        fbe_u32_t flush_start = fbe_provision_drive_metadata_cache_get_flush_start(provision_drive_p);
        fbe_provision_drive_metadata_cache_bgz_unlock(provision_drive_p);

        /* We need to update mdo fields */
        if (flush_start != 0xFFFFFFFF) {
            mdo->u.metadata_callback.offset = flush_start * sizeof(fbe_provision_drive_paged_metadata_t);
            mdo->u.metadata_callback.repeat_count = end_chunk - flush_start + 1;
            fbe_provision_drive_utils_trace(provision_drive_p,
                                            FBE_TRACE_LEVEL_DEBUG_LOW,
                                            FBE_TRACE_MESSAGE_ID_INFO,
                                            FBE_PROVISION_DRIVE_DEBUG_FLAG_PAGED_CACHE,
                                            "PAGED CACHE update BGZ mdo changed offset: 0x%llx count: 0x%llx, slot_start 0x%x\n",
                                            mdo->u.metadata_callback.offset, mdo->u.metadata_callback.repeat_count, slot_ptr->start_chunk);

        }
        return FBE_STATUS_CONTINUE;
    }

    /* Update to BGZ cache only */
    //fbe_provision_drive_metadata_cache_invalidate_slot_region(provision_drive_p, slot_ptr, start_chunk, chunk_count);
    fbe_provision_drive_metadata_cache_add_to_flush(provision_drive_p, start_chunk, chunk_count);
    fbe_provision_drive_metadata_cache_bgz_unlock(provision_drive_p);

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_PAGED_CACHE,
                                    "PAGED CACHE update BGZ only slot start: 0x%x count: 0x%x, slot_start 0x%x\n",
                                    start_chunk, chunk_count, slot_ptr->start_chunk);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_cache_udpate_to_cache_only()
 ******************************************************************************/


/***************************************************************************
 * end fbe_provision_drive_metadata_cache.c
 ***************************************************************************/

