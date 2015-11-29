/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_cms_memory.c
 ***************************************************************************
 *
 * @brief
 *  This file implements the functions used to control memory persistence,
 *  obtain the status of memory persistence requests, and carve up memory
 *  into buffers and tags for consumption by CMS.
 * 
 *  Memory obtained from a dedicated CMM pool by grabbing the SGL out from
 *  under CMM in the same way the SP Cache currently gets its data memory.
 *
 *  Code which is specific to the hardware implementation is located in:
 *      fbe_cms_memory_hardware.c
 *
 *  Code which is specific to the simulation implementation is located in:
 *      fbe_cms_memory_simulation.c
 *
 * @version
 *   2011-10-17 - Created. Matthew Ferson
 *
 ***************************************************************************/

#include "fbe_cms_private.h"
#include "fbe_cms_defines.h"
#include "fbe_cms_cluster_tag.h"
#include "fbe_cms_cluster.h"
#include "fbe_cms_main.h"

#include "fbe_cms_memory_private.h"


/* Globals */

/*! 
 * @var fbe_cms_memory_volatile_data 
 *  
 * @brief Holds global data used by the CMS Memory module which is not
 * persisted. This structure will be fully re-initialized each time the 
 * module is loaded.
 */
fbe_cms_memory_volatile_data_t fbe_cms_memory_volatile_data;

/* Private Functions */

/*! 
 * @brief Gets the total amount of unallocated memory, in bytes, by 
 * adding up the size of all of the SGL entries in the local working SGL. 
 *  
 * @version
 *   2011-11-04 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_memory_get_free_memory_size(fbe_u64_t * memory_size)
{
    fbe_cms_memory_volatile_data_t  * volatile_data_p   = NULL;
    fbe_cms_memory_extended_sgl_t   * sgl_p             = NULL;
    fbe_u32_t                         i                 = 0;

    volatile_data_p = &fbe_cms_memory_volatile_data;

    sgl_p = &volatile_data_p->working_sgl[0];

    *memory_size = 0;
    for(i = 0; i < FBE_CMS_MAXIMUM_SYSTEM_SGL_LENGTH; i++)
    {
        *memory_size += sgl_p->length;
        sgl_p++;
    }

    return(FBE_STATUS_OK);
}

/*! 
 * @brief Breaks off a block of persistent memory from the CMS Data Pool. 
 * Memory assigned via this function cannot be freed. Assignment is via 
 * first-fit. Allocations are rounded up for alignment, and the actual
 * amount of memory assigned is filled in. Allocations are guaranteed 
 * to be both virtually and physically contiguous. 
 *  
 * Memory may be allocated from the front or back of the pool; the management 
 * structure and buffers will be allocated from the front, and tags from the 
 * back. Tags and buffers will grow towards the center in order to make it 
 * easier to support growing the memory size. 
 *  
 * @version
 *   2011-10-17 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_memory_allocate_contiguous_block(void     ** virtual_address, 
                                                      fbe_u64_t * physical_address, 
                                                      fbe_u64_t * length,
                                                      fbe_bool_t allocate_from_end,
                                                      fbe_bool_t allocate_remote)
{
    fbe_status_t                      status            = FBE_STATUS_INSUFFICIENT_RESOURCES;
    fbe_cms_memory_volatile_data_t  * volatile_data_p   = NULL;
    fbe_cms_memory_extended_sgl_t   * sgl_p             = NULL;
    fbe_u32_t                         i                 = 0;
    fbe_u32_t                         alignment         = 0;

    volatile_data_p = &fbe_cms_memory_volatile_data;

    if(allocate_from_end == FBE_TRUE)
    {
        alignment = volatile_data_p->backward_allocation_alignment;
    }
    else
    {
        alignment = volatile_data_p->forward_allocation_alignment;
    }

    /* Round the requested length up to the allocation alignment size */
    if((*length % alignment) != 0)
    {
        *length += (alignment - (*length % alignment));
    }

    if(allocate_from_end == FBE_TRUE)
    {
        if(allocate_remote)
        {
            sgl_p = &volatile_data_p->peer_working_sgl[FBE_CMS_MAXIMUM_SYSTEM_SGL_LENGTH-1];
        }
        else
        {
            sgl_p = &volatile_data_p->working_sgl[FBE_CMS_MAXIMUM_SYSTEM_SGL_LENGTH-1];
        }
        for(i = 0; i < FBE_CMS_MAXIMUM_SYSTEM_SGL_LENGTH; i++)
        {
            /* Not enough memory here; move on to the next block */
            if(sgl_p->length < *length)
            {
                sgl_p--;
            }
            /* We can assign from the current block */
            else
            {
                *physical_address = sgl_p->physical_address - *length;
                *virtual_address  = sgl_p->virtual_address - *length;
    
                sgl_p->length    -= *length;
    
                status = FBE_STATUS_OK;
                break;
            }
        }
    }
    else {
        if(allocate_remote)
        {
            sgl_p = &volatile_data_p->peer_working_sgl[0];
        }
        else
        {
            sgl_p = &volatile_data_p->working_sgl[0];
        }

        for(i = 0; i < FBE_CMS_MAXIMUM_SYSTEM_SGL_LENGTH; i++)
        {
            /* Not enough memory here; move on to the next block */
            if(sgl_p->length < *length)
            {
                sgl_p++;
            }
            /* We can assign from the current block */
            else
            {
                *physical_address = sgl_p->physical_address;
                *virtual_address  = sgl_p->virtual_address;
    
                sgl_p->physical_address += *length;
                sgl_p->virtual_address  += *length;
                sgl_p->length           -= *length;
    
                status = FBE_STATUS_OK;
                break;
            }
        }
    }

    return(status);
}

/*! 
 * @brief Carves up local memory and assigns it for buffers and tags.
 *  
 * @version
 *   2011-10-17 - Created. Matthew Ferson 
 */
fbe_status_t fbe_cms_memory_assign_local_memory(fbe_u32_t buffers_needed)
{
    fbe_status_t status         = FBE_STATUS_GENERIC_FAILURE;
    fbe_cms_memory_volatile_data_t * volatile_data_p = NULL;
    fbe_u64_t memory_size       = 0;
    fbe_u32_t max_buffers       = 0;
    fbe_u32_t buffers_to_assign = 0;
    fbe_u32_t buffers_assigned  = 0;
    fbe_u32_t buffer_map_size   = 0;
    fbe_u32_t i                 = 0;
    fbe_u64_t temp_length       = 0;
    fbe_cms_memory_buffer_descriptor_t * buffer_map_ptr = NULL;

    volatile_data_p = &fbe_cms_memory_volatile_data;

    status          = fbe_cms_memory_get_free_memory_size(&memory_size);

    max_buffers     = (fbe_u32_t)(memory_size / (FBE_CMS_BUFFER_SIZE + sizeof(fbe_cms_cluster_tag_t)));

    buffer_map_size = max_buffers * sizeof(fbe_cms_memory_buffer_descriptor_t);

    buffer_map_ptr  = fbe_cms_cmm_memory_allocate(buffer_map_size);

    if(buffer_map_ptr == NULL)
    {
        return(FBE_STATUS_INSUFFICIENT_RESOURCES);
    }

    if(buffers_needed == 0)
    {
        buffers_to_assign = max_buffers;
    }
    else
    {
       buffers_to_assign = buffers_needed;
    }

    volatile_data_p->buffer_map = buffer_map_ptr;

    for(i = 0; i < buffers_to_assign; i++)
    {
        /* Allocate the tag */
        temp_length = sizeof(fbe_cms_cluster_tag_t);
        status = fbe_cms_memory_allocate_contiguous_block(
                    &buffer_map_ptr->local_tag_virtual_address,
                    &buffer_map_ptr->local_tag_physical_address,
                    &temp_length,
                    FBE_TRUE,
                    FBE_FALSE
                    );
        if(status != FBE_STATUS_OK)
        {
            break;
        }

        /* Allocate the buffer */
        temp_length = volatile_data_p->buffer_size;
        status = fbe_cms_memory_allocate_contiguous_block(
                    &buffer_map_ptr->local_buffer_virtual_address,
                    &buffer_map_ptr->local_buffer_physical_address,
                    &temp_length,
                    FBE_FALSE,
                    FBE_FALSE
                    );
        if(status != FBE_STATUS_OK)
        {
            break;
        }

        /* Set the remote addresses to NULL; these will be filled
         * in by fbe_cms_memory_assign_remote_memory() after we have
         * gotten a memory map from the peer
         */
        buffer_map_ptr->remote_tag_physical_address     = NULL;
        buffer_map_ptr->remote_buffer_physical_address  = NULL;

        buffer_map_ptr++;
        buffers_assigned++;
    }

    /* If we're joining a live peer or restoring a vault image and don't have enough
       memory for the required number of buffers, we have to fail initialization
     */
    if(buffers_needed > buffers_assigned)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, 
                  "%s: Insufficient memory to assign %d buffers; only enough memory for %d\n", 
                  __FUNCTION__, buffers_to_assign, buffers_assigned);

        return(FBE_STATUS_INSUFFICIENT_RESOURCES);
    }

    volatile_data_p->number_of_buffers = buffers_assigned;

    cms_trace(FBE_TRACE_LEVEL_INFO, 
              "%s: %d buffers were assigned.\n", 
              __FUNCTION__,
              buffers_assigned);

    return(FBE_STATUS_OK);
}

/*! 
 * @brief Carves up remote memory and assigns it for buffers and tags.
 *  
 * @version
 *   2011-11-10 - Created. Matthew Ferson 
 */
fbe_status_t fbe_cms_memory_assign_remote_memory()
{
    void *                                  temp_virt_addr  = NULL;
    fbe_cms_memory_volatile_data_t *        volatile_data_p = NULL;
    fbe_cms_memory_buffer_descriptor_t *    buffer_map_ptr  = NULL;
    fbe_cms_cluster_tag_t *                 tag_p           = NULL;

    fbe_status_t    status              = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t       buffers_to_assign   = 0;
    fbe_u32_t       buffers_assigned    = 0;
    fbe_u32_t       i                   = 0;
    fbe_u64_t       temp_length         = 0;
    fbe_u64_t       temp_phys_addr      = 0;

    volatile_data_p     = &fbe_cms_memory_volatile_data;

    buffers_to_assign   = volatile_data_p->number_of_buffers;

    buffer_map_ptr      = volatile_data_p->buffer_map;

    if((buffer_map_ptr == NULL) || (buffers_to_assign == 0))
    {
        return(FBE_STATUS_NOT_INITIALIZED);
    }

    /* Assign memory for the peer's persistent data block.
     * We don't need to access it, but need to account for it
     * in order to line up the buffers correctly.
     */
    temp_length = sizeof(fbe_cms_memory_persisted_data_t);
    status = fbe_cms_memory_allocate_contiguous_block(
                &temp_virt_addr,
                &temp_phys_addr,
                &temp_length,
                FBE_FALSE,
                FBE_TRUE
                );
    if(status != FBE_STATUS_OK)
    {
        return(FBE_STATUS_INSUFFICIENT_RESOURCES);
    }

    /* Assign memory for buffers and tags on the peer */
    for(i = 0; i < buffers_to_assign; i++)
    {
        temp_length = sizeof(fbe_cms_cluster_tag_t);
        status = fbe_cms_memory_allocate_contiguous_block(
                    &temp_virt_addr,
                    &buffer_map_ptr->remote_tag_physical_address,
                    &temp_length,
                    FBE_TRUE,
                    FBE_TRUE
                    );
        if(status != FBE_STATUS_OK)
        {
            break;
        }
        temp_length = volatile_data_p->buffer_size;
        status = fbe_cms_memory_allocate_contiguous_block(
                    &temp_virt_addr,
                    &buffer_map_ptr->remote_buffer_physical_address,
                    &temp_length,
                    FBE_FALSE,
                    FBE_TRUE
                    );
        if(status != FBE_STATUS_OK)
        {
            break;
        }

        buffer_map_ptr++;
        buffers_assigned++;
    }

    if(buffers_to_assign > buffers_assigned)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, 
                  "%s: Insufficient memory to assign %d buffers; only enough memory for %d\n", 
                  __FUNCTION__, buffers_to_assign, buffers_assigned);

        return(FBE_STATUS_INSUFFICIENT_RESOURCES);
    }

    return(FBE_STATUS_OK);
}

/*! 
 * @brief Called when we've received a memory map from the peer, so that 
 * we can assign remote addresses to our tags and buffers. 
 *  
 * @version
 *   2011-11-14 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_memory_receive_peer_memory_map(fbe_cms_memory_sgl_t * peer_sgl_p)
{
    fbe_status_t                            status          = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                               i               = 0;
    fbe_cms_memory_extended_sgl_t *         ex_sgl_p        = NULL;
    fbe_cms_memory_volatile_data_t *        volatile_data_p = NULL;

    volatile_data_p = &fbe_cms_memory_volatile_data;

    /* Sanity check arguments */
    if(peer_sgl_p == NULL)
    {
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    ex_sgl_p = &volatile_data_p->peer_original_sgl[0];

    for(i = 0; i < FBE_CMS_MAXIMUM_SYSTEM_SGL_LENGTH; i++)
    {
        ex_sgl_p->length            = peer_sgl_p->length;
        ex_sgl_p->physical_address  = peer_sgl_p->physical_address;

        /* We don't actually care about the peer's virtual address,
         * but faking this out means we can use the same allocator
         * for local and remote memory
         */
        ex_sgl_p->virtual_address   = NULL;

        if(peer_sgl_p->length == 0)
        {
            break;
        }
    }

    /* Make a copy of the original SGL for our allocator to use */
    fbe_copy_memory(&volatile_data_p->peer_working_sgl[0], 
                    &volatile_data_p->peer_original_sgl[0],
                    sizeof(volatile_data_p->peer_original_sgl));

    /* Carve up buffers and tags from remote memory */
    status = fbe_cms_memory_assign_remote_memory();
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    return(FBE_STATUS_OK);
}

/*! 
 * @brief Called when we've lost connectivity with the peer, to 
 * clear out all of the remote tag and buffer addresses 
 *  
 * @version
 *   2011-11-14 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_memory_lost_contact_with_peer()
{
    fbe_cms_memory_volatile_data_t *        volatile_data_p = NULL;
    fbe_cms_memory_buffer_descriptor_t *    buffer_map_ptr  = NULL;

    fbe_status_t    status              = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t       i                   = 0;

    volatile_data_p     = &fbe_cms_memory_volatile_data;

    buffer_map_ptr      = volatile_data_p->buffer_map;

    if(buffer_map_ptr == NULL)
    {
        return(FBE_STATUS_NOT_INITIALIZED);
    }

    for(i = 0; i < volatile_data_p->number_of_buffers; i++)
    {
        buffer_map_ptr->remote_tag_physical_address     = NULL;
        buffer_map_ptr->remote_buffer_physical_address  = NULL;
        buffer_map_ptr++;
    }

    return(FBE_STATUS_OK);
}

/*! 
 * @brief Clear out our volatile data and initialize it. 
 * It's a global, so it should already be zeroed anyway. 
 * Belt and suspenders! 
 *  
 * @version
 *   2011-10-17 - Created. Matthew Ferson
 */
void fbe_cms_memory_init_volatile_data()
{
    fbe_cms_memory_volatile_data_t  *       volatile_data_p     = NULL;
    /* Clear out our volatile data and initialize it. 
     * It's a global, so it should already be zeroed anyway.
     * Belt and suspenders!
     */
    fbe_zero_memory(&fbe_cms_memory_volatile_data, sizeof(fbe_cms_memory_volatile_data));

    volatile_data_p = &fbe_cms_memory_volatile_data;

    fbe_spinlock_init(&volatile_data_p->lock);
}

/*! 
 * @brief 
 * 
 * @version
 *   2011-11-14 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_memory_give_tags_to_cluster()
{
    fbe_status_t                        status          = FBE_STATUS_GENERIC_FAILURE;
    fbe_cms_memory_volatile_data_t *    volatile_data_p = NULL;
    fbe_bool_t                          init_tags       = FBE_FALSE;
    fbe_cms_memory_persist_status_t     persist_status  = FBE_CMS_MEMORY_PERSIST_STATUS_STRUCT_INIT;
    fbe_u32_t                           i               = 0;
    fbe_cms_cluster_tag_t *             tag_p           = NULL;

    volatile_data_p = &fbe_cms_memory_volatile_data;

    if(! fbe_cms_memory_is_initialized())
    {
        return(FBE_STATUS_NOT_INITIALIZED);
    }

    persist_status = volatile_data_p->status;

    if(persist_status != FBE_CMS_MEMORY_PERSIST_STATUS_SUCCESSFUL)
    {
        init_tags = FBE_TRUE;
    }

    /* Hand over the tags */
    for(i = 0; i < volatile_data_p->number_of_buffers; i++)
    {
        tag_p = (fbe_cms_cluster_tag_t * )fbe_cms_memory_get_va_of_tag(i);

        if(init_tags == FBE_TRUE)
        {
            fbe_cms_cluster_tag_init(tag_p);
            fbe_cms_tag_set_tag_index(tag_p, i);
        }
        status = fbe_cms_cluster_add_tag(tag_p);
        if(status != FBE_STATUS_OK)
        {
            cms_trace(FBE_TRACE_LEVEL_ERROR, 
                      "%s: Failed to add tag to cluster: 0x%X\n", 
                      __FUNCTION__, status);
        }
    }
    return(FBE_STATUS_OK);
}

/*! 
 * @brief 
 * 
 * @version
 *   2011-10-17 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_memory_init_common(fbe_cms_memory_persist_status_t reason, fbe_u32_t number_of_buffers)
{
    fbe_status_t                            status              = FBE_STATUS_GENERIC_FAILURE;
    fbe_cms_memory_volatile_data_t  *       volatile_data_p     = NULL;
    fbe_cms_memory_persisted_data_t  *      persisted_data_p    = NULL;
    fbe_u64_t                               temp_length         = 0;
    fbe_u64_t                               temp_phys_addr      = 0;
    fbe_u32_t                               i                   = 0;
    fbe_cms_cluster_tag_t *                 tag_p               = NULL;
    fbe_bool_t                              persisted           = FBE_FALSE;

    volatile_data_p = &fbe_cms_memory_volatile_data;

    /* We will eventually need to get these parameters from the CDR */
    volatile_data_p->forward_allocation_alignment   = FBE_CMS_MEMORY_FORWARD_ALLOCATION_ALIGNMENT;
    volatile_data_p->backward_allocation_alignment  = FBE_CMS_MEMORY_BACKWARD_ALLOCATION_ALIGNMENT;
    volatile_data_p->buffer_size                    = FBE_CMS_BUFFER_SIZE;


    /* Get memory */
    status = fbe_cms_memory_get_memory();
    if(status != FBE_STATUS_OK)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, 
                  "%s: failed to get memory.\n", 
                  __FUNCTION__);

        return(status);
    }

    /* Allocate our persisted data block */
    temp_length = sizeof(fbe_cms_memory_persisted_data_t);
    status      = fbe_cms_memory_allocate_contiguous_block(&persisted_data_p, 
                                                           &temp_phys_addr, 
                                                           &temp_length, 
                                                           FBE_FALSE,
                                                           FBE_FALSE);
    if(status != FBE_STATUS_OK)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, 
                  "%s: failed to allocate data memory for internal usage.\n", 
                  __FUNCTION__);
        return(status);
    }
    volatile_data_p->persisted_data_p = persisted_data_p;

    /* If memory was persisted from BIOS' perspective, sanity check our own data */
    if(reason == FBE_CMS_MEMORY_PERSIST_STATUS_SUCCESSFUL)
    {
        if(fbe_compare_string(persisted_data_p->signature, 
                                  FBE_CMS_MEMORY_GLOBAL_DATA_SIGNATURE_SIZE, 
                                  FBE_CMS_MEMORY_GLOBAL_DATA_SIGNATURE, 
                                  FBE_CMS_MEMORY_GLOBAL_DATA_SIGNATURE_SIZE,
                                  FBE_TRUE) != 0)
        {
            cms_trace(FBE_TRACE_LEVEL_ERROR, 
                      "%s: Failed to find signature in DRAM.\n", 
                      __FUNCTION__);
    
            reason = FBE_CMS_MEMORY_PERSIST_STATUS_STRUCT_CORRUPT;
        }
        else if(persisted_data_p->persistence_requested == 0)
        {
            cms_trace(FBE_TRACE_LEVEL_INFO, 
                      "%s: Memory was unexpectedly persisted, and will be ignored.\n", 
                      __FUNCTION__);
    
            reason = FBE_CMS_MEMORY_PERSIST_STATUS_NORMAL_BOOT;
        }
        else
        {
            persisted               = FBE_TRUE;
            volatile_data_p->status = FBE_CMS_MEMORY_PERSIST_STATUS_SUCCESSFUL;
    
            cms_trace(FBE_TRACE_LEVEL_INFO, 
                "%s: Memory was persisted for this boot\n", 
                __FUNCTION__);
        }
    }
    /* If memory wasn't persisted for any reason, reinitialize CMS's persisted data */
    if(persisted != FBE_TRUE)
    {
        cms_trace(FBE_TRACE_LEVEL_INFO, 
            "%s: Memory was not persisted for this boot\n", 
            __FUNCTION__);

        fbe_zero_memory(persisted_data_p, sizeof(fbe_cms_memory_persisted_data_t));

        strcpy(persisted_data_p->signature, FBE_CMS_MEMORY_GLOBAL_DATA_SIGNATURE);

        volatile_data_p->status = reason;
    }

    /* At this point we've either recovered the memory or reinitialized,
     * so set the BIOS memory persistence request and unsafe to remove LED
     * to match our internal state.
     */
    status = fbe_cms_memory_set_request_persistence();
    if(status != FBE_STATUS_OK)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, 
                  "%s: Failed to set initial BIOS persistence request\n", 
                  __FUNCTION__);

        return(status);
    }

    status = fbe_cms_memory_set_unsafe_to_remove();
    if(status != FBE_STATUS_OK)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, 
                  "%s: Failed to set initial Unsafe to Remove LED state\n", 
                  __FUNCTION__);

        return(status);
    }

    status = fbe_cms_memory_assign_local_memory(number_of_buffers);
    if(status != FBE_STATUS_OK)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, 
                  "%s: Failed to assign local memory\n", 
                  __FUNCTION__);

        return(status);
    }

    /* Set a flag so that our interface functions know that we're ready
     * for business.
     */
    volatile_data_p->init_completed = FBE_TRUE;

    return(FBE_STATUS_OK);
}

fbe_status_t fbe_cms_memory_destroy_common()
{
    fbe_cms_memory_volatile_data_t * volatile_data_p     = NULL;

    volatile_data_p = &fbe_cms_memory_volatile_data;

    if(volatile_data_p->buffer_map != NULL)
    {
        fbe_cms_cmm_memory_release(volatile_data_p->buffer_map);
    }

    return(FBE_STATUS_OK);
}

/* Public Functions */

/*! 
 * @brief Interface function for clients (eg CMS or SP Cache) to request memory 
 * be persisted, or cancel a previous memory persistence request. Coordinates 
 * persistence for all consumers of persistent memory in the system. 
 * 
 * @version
 *   2011-10-17 - Created. Matthew Ferson 
 */
fbe_status_t fbe_cms_memory_request_persistence(fbe_cms_client_id_t client_id, fbe_bool_t request)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cms_memory_volatile_data_t  * volatile_data_p = NULL;
    fbe_cms_memory_persisted_data_t * persisted_data_p = NULL;

    volatile_data_p     = &fbe_cms_memory_volatile_data;

    fbe_spinlock_lock(&volatile_data_p->lock);

    if(! fbe_cms_memory_is_initialized())
    {
        fbe_spinlock_unlock(&volatile_data_p->lock);
        return(FBE_STATUS_NOT_INITIALIZED);
    }

    persisted_data_p    = volatile_data_p->persisted_data_p;

    if(request)
    {
        persisted_data_p->persistence_requested |= (1 << client_id);
    }
    else
    {
        persisted_data_p->persistence_requested &= (~ (1 << client_id));
    }

    status = fbe_cms_memory_set_request_persistence();

    fbe_spinlock_unlock(&volatile_data_p->lock);

    return(status);
}

/*! 
 * @brief Interface function for clients (eg CMS or SP Cache) to request the
 * the unsafe to remove LED to be lit, or cancel a previous request. Coordinates
 * the unsafe to remove LED for all consumers of persistent memory in the system. 
 *  
 * The LED will be illuminated any time this SP's memory contains the only valid 
 * copy of user data. 
 * 
 * @version
 *   2011-10-17 - Created. Matthew Ferson 
 */
fbe_status_t fbe_cms_memory_request_unsafe_to_remove(fbe_cms_client_id_t client_id, fbe_bool_t request)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cms_memory_volatile_data_t  * volatile_data_p = NULL;
    fbe_cms_memory_persisted_data_t * persisted_data_p = NULL;

    volatile_data_p     = &fbe_cms_memory_volatile_data;

    fbe_spinlock_lock(&volatile_data_p->lock);

    if(! fbe_cms_memory_is_initialized())
    {
        fbe_spinlock_unlock(&volatile_data_p->lock);
        return(FBE_STATUS_NOT_INITIALIZED);
    }

    persisted_data_p    = volatile_data_p->persisted_data_p;

    if(request)
    {
        persisted_data_p->unsafe_to_remove |= (1 << client_id);
    }
    else
    {
        persisted_data_p->unsafe_to_remove &= (~ (1 << client_id));
    }

    status = fbe_cms_memory_set_unsafe_to_remove();

    fbe_spinlock_unlock(&volatile_data_p->lock);

    return(status);
}

/*! 
 * @brief Returns the memory persistence status from the perspective of CMS
 * 
 * @version
 *   2011-10-17 - Created. Matthew Ferson 
 */
fbe_status_t fbe_cms_memory_get_memory_persistence_status(fbe_cms_memory_persist_status_t * status)
{
    fbe_cms_memory_volatile_data_t  * volatile_data_p = NULL;

    volatile_data_p     = &fbe_cms_memory_volatile_data;

    fbe_spinlock_lock(&volatile_data_p->lock);

    if(! fbe_cms_memory_is_initialized())
    {
        fbe_spinlock_unlock(&volatile_data_p->lock);
        return(FBE_STATUS_NOT_INITIALIZED);
    }

    *status = volatile_data_p->status;

    fbe_spinlock_unlock(&volatile_data_p->lock);

    return(FBE_STATUS_OK);
}

/*! 
 * @brief Get a pointer to a buffer descriptor, given its index. 
 * The buffer descriptor contains the local and remote physical 
 * addresses and local virtual address for the buffer and tag. 
 * 
 * @version
 *   2011-10-17 - Created. Matthew Ferson 
 */
fbe_status_t fbe_cms_memory_get_buffer_descriptor_by_index(fbe_u32_t tag_index, fbe_cms_memory_buffer_descriptor_t ** buffer_desc_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cms_memory_volatile_data_t * volatile_data_p = NULL;

    volatile_data_p = &fbe_cms_memory_volatile_data;

    if(! fbe_cms_memory_is_initialized())
    {
        return(FBE_STATUS_NOT_INITIALIZED);
    }

    if(tag_index > volatile_data_p->number_of_buffers)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, 
                  "%s: Index %d is out of bounds; only %d buffers/tags are assigned\n", 
                  __FUNCTION__, tag_index, volatile_data_p->number_of_buffers);

        return(FBE_STATUS_GENERIC_FAILURE);
    }

    *buffer_desc_ptr = volatile_data_p->buffer_map + tag_index;

    return(FBE_STATUS_OK);

}

/*! 
 * @brief Get a pointer to a tag, given its index
 * 
 * @version
 *   2011-10-17 - Created. Matthew Ferson 
 */
void * fbe_cms_memory_get_va_of_tag(fbe_u32_t tag_index)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cms_memory_buffer_descriptor_t *buffer_desc_ptr = NULL;

    status = fbe_cms_memory_get_buffer_descriptor_by_index(tag_index, &buffer_desc_ptr);
    if(status != FBE_STATUS_OK)
    {
        return(NULL);
    }

    return(buffer_desc_ptr->local_tag_virtual_address);
}

/*! 
 * @brief Get the local physical address of a tag, given its index
 * 
 * @version
 *   2011-10-17 - Created. Matthew Ferson 
 */
fbe_u64_t fbe_cms_memory_get_local_pa_of_tag(fbe_u32_t tag_index)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cms_memory_buffer_descriptor_t *buffer_desc_ptr = NULL;

    status = fbe_cms_memory_get_buffer_descriptor_by_index(tag_index, &buffer_desc_ptr);
    if(status != FBE_STATUS_OK)
    {
        return(NULL);
    }

    return(buffer_desc_ptr->local_tag_physical_address);
}

/*! 
 * @brief Get the physical address of a tag on the peer, given its index
 * 
 * @version
 *   2011-10-17 - Created. Matthew Ferson 
 */
fbe_u64_t fbe_cms_memory_get_remote_pa_of_tag(fbe_u32_t tag_index)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cms_memory_buffer_descriptor_t *buffer_desc_ptr = NULL;

    status = fbe_cms_memory_get_buffer_descriptor_by_index(tag_index, &buffer_desc_ptr);
    if(status != FBE_STATUS_OK)
    {
        return(NULL);
    }

    return(buffer_desc_ptr->remote_tag_physical_address);
}

/*! 
 * @brief Get a pointer to a buffer, given its index
 * 
 * @version
 *   2011-10-17 - Created. Matthew Ferson 
 */
void * fbe_cms_memory_get_va_of_buffer(fbe_u32_t tag_index)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cms_memory_buffer_descriptor_t *buffer_desc_ptr = NULL;

    status = fbe_cms_memory_get_buffer_descriptor_by_index(tag_index, &buffer_desc_ptr);
    if(status != FBE_STATUS_OK)
    {
        return(NULL);
    }

    return buffer_desc_ptr->local_buffer_virtual_address;
}

/*! 
 * @brief Get the local physical address of a buffer, given its index
 * 
 * @version
 *   2011-10-17 - Created. Matthew Ferson 
 */
fbe_u64_t fbe_cms_memory_get_local_pa_of_buffer(fbe_u32_t tag_index)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cms_memory_buffer_descriptor_t *buffer_desc_ptr = NULL;

    status = fbe_cms_memory_get_buffer_descriptor_by_index(tag_index, &buffer_desc_ptr);
    if(status != FBE_STATUS_OK)
    {
        return(NULL);
    }

    return buffer_desc_ptr->local_buffer_physical_address;
}

/*! 
 * @brief Get the physical address of a buffer on the peer, given its index
 * 
 * @version
 *   2011-10-17 - Created. Matthew Ferson 
 */
fbe_u64_t fbe_cms_memory_get_remote_pa_of_buffer(fbe_u32_t tag_index)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cms_memory_buffer_descriptor_t *buffer_desc_ptr = NULL;

    status = fbe_cms_memory_get_buffer_descriptor_by_index(tag_index, &buffer_desc_ptr);
    if(status != FBE_STATUS_OK)
    {
        return(NULL);
    }

    return buffer_desc_ptr->remote_buffer_physical_address;
}

/*! 
 * @brief Get the number of buffers being managed
 * 
 * @version
 *   2011-10-17 - Created. Matthew Ferson 
 */
fbe_u32_t fbe_cms_memory_get_number_of_buffers()
{
    fbe_cms_memory_volatile_data_t * volatile_data_p = NULL;

    volatile_data_p = &fbe_cms_memory_volatile_data;

    return(volatile_data_p->number_of_buffers);
}

/*! 
 * @brief Return true if the memory component is initialized
 * 
 * @version
 *   2011-10-17 - Created. Matthew Ferson 
 */
fbe_bool_t fbe_cms_memory_is_initialized()
{
    fbe_cms_memory_volatile_data_t * volatile_data_p = NULL;

    volatile_data_p = &fbe_cms_memory_volatile_data;

    return(volatile_data_p->init_completed);
}

/* end fbe_cms_memory.c */
