#ifndef	FBE_CMS_MEMORY_H
#define	FBE_CMS_MEMORY_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_cms_memory.h
 ***************************************************************************
 *
 * @brief
 *  This file defines the data types and function prototypes used to access
 *  persistent memory.
 *
 * @version
 *   2011-10-17 - Created. Matthew Ferson
 *
 ***************************************************************************/

/*
 * INCLUDE FILES
 */
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"

typedef struct fbe_cms_memory_persistence_pointers_s
{
    void * set_persistence_request;
    void * get_persistence_request;
    void * get_persistence_status;
    void * get_sgl;
} fbe_cms_memory_persistence_pointers_t;

/*! @struct fbe_cms_memory_buffer_descriptor_t
 *  @brief 
 */
typedef struct fbe_cms_memory_buffer_descriptor_s
{
    void *     local_tag_virtual_address;
    fbe_u64_t  local_tag_physical_address;
    fbe_u64_t  remote_tag_physical_address;
    void *     local_buffer_virtual_address;
    fbe_u64_t  local_buffer_physical_address;
    fbe_u64_t  remote_buffer_physical_address;
} fbe_cms_memory_buffer_descriptor_t;


/* CMS Internal Use Function prototypes */
fbe_status_t    fbe_cms_memory_init(fbe_u32_t number_of_buffers);

fbe_status_t    fbe_cms_memory_destroy(void);
fbe_status_t    fbe_cms_memory_give_tags_to_cluster(void);

fbe_status_t    fbe_cms_memory_set_memory_persistence_pointers(fbe_cms_memory_persistence_pointers_t * pointers);

fbe_status_t    fbe_cms_memory_get_buffer_descriptor_by_index(fbe_u32_t tag_index, 
                                                              fbe_cms_memory_buffer_descriptor_t ** buffer_desc_ptr);

void *          fbe_cms_memory_get_va_of_tag(fbe_u32_t tag_index);
fbe_u64_t       fbe_cms_memory_get_local_pa_of_tag(fbe_u32_t tag_index);
fbe_u64_t       fbe_cms_memory_get_remote_pa_of_tag(fbe_u32_t tag_index);

void *          fbe_cms_memory_get_va_of_buffer(fbe_u32_t tag_index);
fbe_u64_t       fbe_cms_memory_get_local_pa_of_buffer(fbe_u32_t tag_index);
fbe_u64_t       fbe_cms_memory_get_remote_pa_of_buffer(fbe_u32_t tag_index);

fbe_u32_t       fbe_cms_memory_get_number_of_buffers(void);
fbe_bool_t      fbe_cms_memory_is_initialized(void);

#endif /* #ifndef FBE_CMS_MEMORY_H */
