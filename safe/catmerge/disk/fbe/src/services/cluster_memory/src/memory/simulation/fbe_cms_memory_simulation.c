/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_cms_memory_simulation.c
 ***************************************************************************
 *
 * @brief
 *  This file implements the functions used to access persistent memory.
 *
 * @version
 *   2011-10-31 - Created. Matthew Ferson
 *
 ***************************************************************************/

#include "common/fbe_cms_private.h"
#include "fbe_cms_memory_private.h"
#include "flare_sgl.h"

extern fbe_cms_memory_volatile_data_t fbe_cms_memory_volatile_data;
fbe_cms_memory_persistence_pointers_t fbe_cms_memory_persistence_pointers;

/*! 
 * @brief Called by the SEP DLL initialization to give us pointers back to 
 * functions in the Terminator. 
 * 
 * @version
 *   2011-11-01 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_memory_set_memory_persistence_pointers(fbe_cms_memory_persistence_pointers_t * pointers)
{
    fbe_cms_memory_persistence_pointers.get_persistence_request = pointers->get_persistence_request;
    fbe_cms_memory_persistence_pointers.set_persistence_request = pointers->set_persistence_request;
    fbe_cms_memory_persistence_pointers.get_persistence_status  = pointers->get_persistence_status;
    fbe_cms_memory_persistence_pointers.get_sgl                 = pointers->get_sgl;

    return(FBE_STATUS_OK);
}

/*! 
 * @brief Get persistent memory from the Terminator
 * 
 * @version
 *   2011-10-31 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_memory_get_memory()
{
    fbe_status_t    status             = FBE_STATUS_OK;
    SGL             sgl[FBE_CMS_MAXIMUM_SYSTEM_SGL_LENGTH];
    SGL *           cms_memory_sgl_p   = NULL;
    fbe_cms_memory_volatile_data_t  * volatile_data_p = NULL;
    fbe_cms_memory_extended_sgl_t   * sgl_p = NULL;

    fbe_u32_t i = 0;

    fbe_status_t (* get_sgl_function)(SGL *) = NULL;

    volatile_data_p = &fbe_cms_memory_volatile_data;
    sgl_p           = (fbe_cms_memory_extended_sgl_t *)&volatile_data_p->original_sgl;

    cms_memory_sgl_p = &sgl[0];

    get_sgl_function = fbe_cms_memory_persistence_pointers.get_sgl;

    status = get_sgl_function(cms_memory_sgl_p);
    if(status != FBE_STATUS_OK)
    {
        return(status);
    }
    /* I'd like to map this all in at once so that it's virtually contiguous,
     * but that may not be possible. For now, I map in each SGL element
     * separately.
     */
    for(i = 0; i < FBE_CMS_MAXIMUM_SYSTEM_SGL_LENGTH; i++)
    {
        if(cms_memory_sgl_p->PhysLength == 0)
        {
            /* We've hit the end of the Terminator's SGL, so break out.
             * There's no need to terminate our SGL, since we already
             * zeroed all of the memory it uses.
             */
            break;
        }
        else
        {
            /* Pull an element off of the Terminator's SGL and advance the pointers */
            sgl_p->physical_address = cms_memory_sgl_p->PhysAddress;
            sgl_p->virtual_address  = (fbe_u8_t *)(fbe_ptrhld_t)cms_memory_sgl_p->PhysAddress;
            sgl_p->length           = cms_memory_sgl_p->PhysLength;

            cms_memory_sgl_p++;
            sgl_p++;
        }
    }

    /* Make a working copy of the SGL for our allocator to use */
    fbe_copy_memory(&volatile_data_p->working_sgl, 
                    &volatile_data_p->original_sgl, 
                    sizeof(volatile_data_p->working_sgl));

    return(status);
}

/*! 
 * @brief Tell SPECL whether or not to illuminate the "Unsafe to remove" LED. 
 * 
 * @version
 *   2011-10-31 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_memory_set_unsafe_to_remove()
{
    return(FBE_STATUS_OK);
}

/*! 
 * @brief Tell the Terminator whether or not to persist memory on the next reboot.
 * 
 * @version
 *   2011-10-31 - Created. Matthew Ferson 
 */
fbe_status_t fbe_cms_memory_set_request_persistence() 
{
    fbe_status_t    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t       request = FBE_FALSE;
    fbe_status_t (* set_persistence_request_function)(fbe_bool_t) = NULL;
    fbe_cms_memory_volatile_data_t      * volatile_data_p   = NULL;
    fbe_cms_memory_persisted_data_t     * persisted_data_p  = NULL;

    volatile_data_p     = &fbe_cms_memory_volatile_data;
    persisted_data_p    = volatile_data_p->persisted_data_p;

    set_persistence_request_function = fbe_cms_memory_persistence_pointers.set_persistence_request;

    if(persisted_data_p->persistence_requested != 0)
    {
        request = FBE_TRUE;
    }

    status = set_persistence_request_function(request);

    return(status);
}

/*! 
 * @brief Initialize the CMS memory module
 * 
 * @version
 *   2011-10-31 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_memory_init(fbe_u32_t number_of_buffers)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cms_memory_volatile_data_t * volatile_data_p = NULL;
    fbe_status_t (* get_persistence_request_function)(fbe_bool_t *) = NULL;
    fbe_status_t (* get_persistence_status_function)(fbe_cms_memory_persist_status_t *) = NULL;
    fbe_bool_t persistence_requested = FBE_FALSE;
    fbe_cms_memory_persist_status_t persistence_status;

    cms_trace(FBE_TRACE_LEVEL_INFO, 
              "%s: Beginning Initialization\n", 
              __FUNCTION__);

    fbe_cms_memory_init_volatile_data();

    volatile_data_p = &fbe_cms_memory_volatile_data;

    get_persistence_request_function = fbe_cms_memory_persistence_pointers.get_persistence_request;

    status = get_persistence_request_function(&persistence_requested);

    if(status != FBE_STATUS_OK)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, 
                  "%s: Failed to get Terminator persistence request: %d\n", 
                  __FUNCTION__, status);
        return(status);
    }

    get_persistence_status_function = fbe_cms_memory_persistence_pointers.get_persistence_status;

    status = get_persistence_status_function(&persistence_status);

    if(status != FBE_STATUS_OK)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, 
                  "%s: Failed to get Terminator persistence status: %d\n", 
                  __FUNCTION__, status);
        return(status);
    }

    if(! persistence_requested)
    {
        persistence_status = FBE_CMS_MEMORY_PERSIST_STATUS_NORMAL_BOOT;
    }

    status = fbe_cms_memory_init_common(persistence_status, number_of_buffers);

    if(status != FBE_STATUS_OK)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, 
                  "%s: Common Initialization Failed: %d\n", 
                  __FUNCTION__, status);
        return(status);
    }

    volatile_data_p->init_completed = FBE_TRUE;

    return(FBE_STATUS_OK);
}

/*! 
 * @brief Called during package tear-down; the main responsibility 
 * is to un-map memory, freeing up the virtual address space. 
 * 
 * @version
 *   2011-10-31 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_memory_destroy()
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cms_memory_volatile_data_t * volatile_data_p = NULL;

    status = fbe_cms_memory_destroy_common();

    volatile_data_p = &fbe_cms_memory_volatile_data;

    fbe_spinlock_lock(&volatile_data_p->lock);

    volatile_data_p->init_completed = FBE_FALSE;

    fbe_spinlock_unlock(&volatile_data_p->lock);

    fbe_zero_memory(&fbe_cms_memory_volatile_data, sizeof(fbe_cms_memory_volatile_data));

    return(FBE_STATUS_OK);
}

/* end fbe_cms_memory_simulation.c */
