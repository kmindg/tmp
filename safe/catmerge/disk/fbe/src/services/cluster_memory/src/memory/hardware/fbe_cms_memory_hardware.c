/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_cms_memory_hardware.c
 ***************************************************************************
 *
 * @brief
 *  This file implements the functions used to access persistent memory.
 *
 * @version
 *   2011-10-17 - Created. Matthew Ferson
 *
 ***************************************************************************/

#include "specl_interface.h"
#include "cmm.h"
#include "fbe/fbe_winddk.h"
#include "common/fbe_cms_private.h"
#include "fbe_cms_memory_private.h"

extern fbe_cms_memory_volatile_data_t fbe_cms_memory_volatile_data;

/* Private Functions */

/*! 
 * @brief Get memory from CMM and map it into SVA space.
 * 
 * @version
 *   2011-10-17 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_memory_get_memory()
{
    fbe_status_t status = FBE_STATUS_OK;
    CMM_ERROR       cmm_status;
    PCMM_SGL        cms_memory_sgl_p = NULL;
    CMM_POOL_ID     cms_pool_id;

    fbe_u64_t       temp_phys_addr;
    fbe_u64_t       temp_length;
    fbe_u8_t *      temp_virt_addr;

    fbe_cms_memory_volatile_data_t  * volatile_data_p = NULL;
    fbe_cms_memory_extended_sgl_t   * sgl_p = NULL;

    fbe_u32_t i = 0;

    volatile_data_p = &fbe_cms_memory_volatile_data;
    sgl_p           = (fbe_cms_memory_extended_sgl_t *)&volatile_data_p->original_sgl;

    /* For now, we're using the same hack that SP Cache is using
     * to get our persistent data memory. We have our own pool
     * which is for our exclusive use, and we just grab the SGL
     * out from under CMM without allocating the memory.
     */
    cms_pool_id         = cmmGetSEPDataPoolID();
    cms_memory_sgl_p    = cmmGetPoolSGL(cms_pool_id);

    /* I'd like to map this all in at once so that it's virtually contiguous,
     * but that may not be possible. For now, I map in each SGL element
     * separately.
     */
    for(i = 0; i < FBE_CMS_MAXIMUM_SYSTEM_SGL_LENGTH; i++)
    {
        if(cms_memory_sgl_p->MemSegLength == 0)
        {
            /* We've hit the end of CMM's SGL, so break out.
             * There's no need to terminate our SGL, since we already
             * zeroed all of the memory it uses.
             */
            break;
        }
        else
        {
            /* Pull an element off of CMM's SGL and advance the pointer */
            temp_phys_addr  = cms_memory_sgl_p->MemSegAddress;
            temp_length     = cms_memory_sgl_p->MemSegLength;
            cms_memory_sgl_p++;


            /* Align the start and end of the block to a 4MB boundary to 
             * improve the performance of page table lookups
             */
            if((temp_phys_addr % FBE_CMS_MEMORY_MAPPING_ALIGNMENT) != 0)
            {
                temp_phys_addr  += (FBE_CMS_MEMORY_MAPPING_ALIGNMENT - (temp_phys_addr % FBE_CMS_MEMORY_MAPPING_ALIGNMENT));
                temp_length     -= (FBE_CMS_MEMORY_MAPPING_ALIGNMENT - (temp_phys_addr % FBE_CMS_MEMORY_MAPPING_ALIGNMENT));
            }

            temp_length -= (temp_length % FBE_CMS_MEMORY_MAPPING_ALIGNMENT);

            /* Map it into SVA space */
            cmm_status = cmmMapMemorySVA(temp_phys_addr, temp_length, (void *)&temp_virt_addr,
                                         &sgl_p->io_map_obj);

            /* Add it to our own SGL and advance the pointer */
            if(cmm_status == CMM_STATUS_SUCCESS)
            {
                sgl_p->physical_address = temp_phys_addr;
                sgl_p->virtual_address = temp_virt_addr;
                sgl_p->length = temp_length;
                sgl_p++;
            }
            else
            {
                /* On a 64-bit system, it's pretty unlikely that there isn't enough contiguous 
                 * virtual address space to complete the mapping.
                 */
                cms_trace(FBE_TRACE_LEVEL_ERROR, 
                          "%s: failed to map memory into SVA space. phys addr: 0x%llX len: 0x%llX\n", 
                          __FUNCTION__, (unsigned long long)temp_phys_addr,
			  (unsigned long long)temp_length);
            }
            
        }
    }

    /* Make a working copy of the SGL for our allocator to use */
    fbe_copy_memory(&volatile_data_p->working_sgl, 
                    &volatile_data_p->original_sgl, 
                    sizeof(volatile_data_p->working_sgl));

    return(status);
}

/*! 
 * @brief Un-map memory acquired from CMM
 * 
 * @version
 *   2011-10-24 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_memory_release_memory()
{
    fbe_status_t    status = FBE_STATUS_OK;
    CMM_ERROR       cmm_status;
    fbe_u32_t       i = 0;

    fbe_cms_memory_volatile_data_t  * volatile_data_p = NULL;
    fbe_cms_memory_extended_sgl_t   * sgl_p = NULL;

    volatile_data_p = &fbe_cms_memory_volatile_data;
    sgl_p           = (fbe_cms_memory_extended_sgl_t *)&volatile_data_p->original_sgl;

    for(i = 0; i < FBE_CMS_MAXIMUM_SYSTEM_SGL_LENGTH; i++)
    {
        if(sgl_p->length == 0)
        {
            break;
        }
        else
        {
            /* Unmap it from SVA space */
          cmm_status = cmmUnMapMemorySVA(&sgl_p->io_map_obj);

            if(cmm_status != CMM_STATUS_SUCCESS)
            {
                status = FBE_STATUS_GENERIC_FAILURE;
                cms_trace(FBE_TRACE_LEVEL_ERROR, 
                          "%s: failed to unmap memory from SVA space. virt addr: 0x%llX len: 0x%llX\n", 
                          __FUNCTION__,
			  (unsigned long long)sgl_p->virtual_address,
			  (unsigned long long)sgl_p->length);
            }

            /* advance the pointer */
            sgl_p++;
        }
    }

    return(status);
}

/*! 
 * @brief Cleans up the contents of the BIOS Memory Persistence structure 
 * to NVRAM. 
 * 
 * @version
 *   2012-08-17 - Created. Sri
 */
fbe_status_t fbe_cms_memory_cleanup_nvram()
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_cms_memory_volatile_data_t * volatile_data_p = NULL;
    NTSTATUS nt_status;

    volatile_data_p = &fbe_cms_memory_volatile_data;
    if (volatile_data_p->nvram_data_p)
    {
        nt_status = NvRamLibCleanupSection(volatile_data_p->nvram_data_p,
                                           volatile_data_p->nvram_section_info.size);

        if(NT_SUCCESS(nt_status))
        {
            status = FBE_STATUS_OK;
        }
    }

    return(status);
}

/*! 
 * @brief Flushes the contents of the BIOS Memory Persistence structure 
 * to NVRAM. 
 * 
 * @version
 *   2011-10-17 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_memory_flush_nvram()
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cms_memory_volatile_data_t * volatile_data_p = NULL;
    NTSTATUS nt_status;

    volatile_data_p = &fbe_cms_memory_volatile_data;
    nt_status = NvRamLibReadWriteBytes(volatile_data_p->nvram_section_info.startOffset,
                                       sizeof (fbe_cms_bios_persistence_request_t),
                                       NvRamLib_ReadWrite_Write,
                                       volatile_data_p->nvram_data_p);

    if(NT_SUCCESS(nt_status))
    {
        status = FBE_STATUS_OK;
    }

    return(status);
}

/*! 
 * @brief Tell BIOS whether or not to persist memory on the next reboot.
 * 
 * @version
 *   2011-10-17 - Created. Matthew Ferson 
 */
fbe_status_t fbe_cms_memory_set_request_persistence() 
{
    fbe_status_t    status  = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t       request = FBE_CMS_MEMORY_BIOS_PERSISTENCE_NOT_REQUESTED;

    fbe_cms_bios_persistence_request_t  * bios_mp_p         = NULL;
    fbe_cms_memory_volatile_data_t      * volatile_data_p   = NULL;
    fbe_cms_memory_persisted_data_t     * persisted_data_p  = NULL;

    volatile_data_p     = &fbe_cms_memory_volatile_data;
    persisted_data_p    = volatile_data_p->persisted_data_p;

    if(persisted_data_p->persistence_requested != 0)
    {
        request = FBE_CMS_MEMORY_BIOS_PERSISTENCE_REQUESTED;
    }

    bios_mp_p = volatile_data_p->bios_mp_p;

    bios_mp_p->persist_request = request;
    bios_mp_p->persist_status  = FBE_CMS_MEMORY_PERSIST_STATUS_SUCCESSFUL;

    status = fbe_cms_memory_flush_nvram();

    return(status);
}

/*! 
 * @brief Tell SPECL whether or not to illuminate the "Unsafe to remove" LED. 
 * 
 * @version
 *   2011-10-17 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_memory_set_unsafe_to_remove() 
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    SPECL_STATUS specl_status;
    fbe_cms_memory_volatile_data_t  * volatile_data_p   = NULL;
    fbe_cms_memory_persisted_data_t * persisted_data_p  = NULL;

    volatile_data_p     = &fbe_cms_memory_volatile_data;
    persisted_data_p    = volatile_data_p->persisted_data_p;

    if(persisted_data_p->unsafe_to_remove != 0)
    {
        specl_status = speclSetUnsafeToRemoveLED(LED_BLINK_ON);
    }
    else
    {
        specl_status = speclSetUnsafeToRemoveLED(LED_BLINK_OFF);
    }

    if(NT_SUCCESS(specl_status))
    {
        status = FBE_STATUS_OK;
    }

    return(status);
}

/*! 
 * @brief Initialize the CMS memory module
 * 
 * @version
 *   2011-10-17 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_memory_init(fbe_u32_t number_of_buffers)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cms_memory_volatile_data_t  *       volatile_data_p     = NULL;
    fbe_cms_bios_persistence_request_t *    bios_mp_p           = NULL;
    fbe_cms_memory_persist_status_t         reason              = FBE_CMS_MEMORY_PERSIST_STATUS_STRUCT_INIT;
    NTSTATUS                                nt_status;

    cms_trace(FBE_TRACE_LEVEL_INFO, 
              "%s: Beginning Initialization\n", 
              __FUNCTION__);

    fbe_cms_memory_init_volatile_data();

    volatile_data_p = &fbe_cms_memory_volatile_data;

    /* Map in NVRAM so that we can check the BIOS persistence status */
    NvRamLibGetSectionInfo(NVRAM_MEMORY_PERSIST_HEADER_SECTION, 
                           &volatile_data_p->nvram_section_info);

    nt_status = NvRamLibInitSection(volatile_data_p->nvram_section_info.startOffset,
                                    volatile_data_p->nvram_section_info.size,
                                    FALSE,
                                    &volatile_data_p->nvram_data_p);
    if(! NT_SUCCESS(nt_status))
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, 
                  "%s: failed to initialize BIOS MP section in NVRAM.\n", 
                  __FUNCTION__);
        return(FBE_STATUS_GENERIC_FAILURE);
    }

    nt_status = NvRamLibReadWriteBytes(volatile_data_p->nvram_section_info.startOffset,
                                       sizeof (fbe_cms_bios_persistence_request_t),
                                       NvRamLib_ReadWrite_Read,
                                       (PUCHAR) volatile_data_p->nvram_data_p);
    if(! NT_SUCCESS(nt_status))
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, 
                  "%s: failed to read BIOS MP section in NVRAM.\n", 
                  __FUNCTION__);

        NvRamLibCleanupSection (volatile_data_p->nvram_data_p, 
                                volatile_data_p->nvram_section_info.size);

        return(FBE_STATUS_GENERIC_FAILURE);
    }

    /* check BIOS memory persistence status */

    volatile_data_p->bios_mp_p = (fbe_cms_bios_persistence_request_t *)volatile_data_p->nvram_data_p;

    bios_mp_p = volatile_data_p->bios_mp_p;

    if(bios_mp_p->object_id != FBE_CMS_MEMORY_BIOS_OBJECT_ID)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, 
                  "%s: BIOS Object ID in NVRAM is incorrect: 0x%X.\n", 
                  __FUNCTION__, bios_mp_p->object_id);

        reason = FBE_CMS_MEMORY_PERSIST_STATUS_STRUCT_CORRUPT;
    }
    else if(bios_mp_p->revision_id != FBE_CMS_MEMORY_BIOS_REVISION_ID)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, 
                  "%s: BIOS Revision ID in NVRAM is incorrect: 0x%X.\n", 
                  __FUNCTION__, bios_mp_p->revision_id);

        reason = FBE_CMS_MEMORY_PERSIST_STATUS_STRUCT_CORRUPT;
    }
    else if(bios_mp_p->persist_request != FBE_CMS_MEMORY_BIOS_PERSISTENCE_REQUESTED)
    {
        cms_trace(FBE_TRACE_LEVEL_INFO, 
                  "%s: Persistence was not requested for this boot.\n", 
                  __FUNCTION__);

        reason = FBE_CMS_MEMORY_PERSIST_STATUS_NORMAL_BOOT;
    }
    else if(bios_mp_p->persist_status != FBE_CMS_MEMORY_PERSIST_STATUS_SUCCESSFUL)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, 
                  "%s: Persistence failed for this boot. BIOS Status Code: 0x%X\n", 
                  __FUNCTION__, bios_mp_p->persist_status);

        reason = bios_mp_p->persist_status;
    }
    else
    {
        reason = FBE_CMS_MEMORY_PERSIST_STATUS_SUCCESSFUL;
    }

    status = fbe_cms_memory_init_common(reason, number_of_buffers);

    if(status != FBE_STATUS_OK)
    {
        return(status);
    }

    cms_trace(FBE_TRACE_LEVEL_INFO, 
              "%s: Completed Initialization\n", 
              __FUNCTION__);

    return(FBE_STATUS_OK);
}

/*! 
 * @brief Called during package tear-down; the main responsibility 
 * is to un-map memory, freeing up the virtual address space. 
 * 
 * @version
 *   2011-10-17 - Created. Matthew Ferson
 */
fbe_status_t fbe_cms_memory_destroy()
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_cms_memory_volatile_data_t * volatile_data_p = NULL;

    cms_trace(FBE_TRACE_LEVEL_INFO, 
              "%s: Beginning Destruction\n", 
              __FUNCTION__);

    status = fbe_cms_memory_destroy_common();

    volatile_data_p = &fbe_cms_memory_volatile_data;

    fbe_spinlock_lock(&volatile_data_p->lock);

    volatile_data_p->init_completed = FBE_FALSE;

    fbe_spinlock_unlock(&volatile_data_p->lock);

    status = fbe_cms_memory_release_memory();

    fbe_cms_memory_cleanup_nvram();
    fbe_zero_memory(&fbe_cms_memory_volatile_data, sizeof(fbe_cms_memory_volatile_data));

    cms_trace(FBE_TRACE_LEVEL_INFO, 
              "%s: Completed Destruction\n", 
              __FUNCTION__);

    return(status);

}

/* end fbe_cms_memory_hardware.c */
