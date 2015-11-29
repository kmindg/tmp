/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************/
/** @file fbe_cms_cluster_tag.c
***************************************************************************
*
* @brief
*  This file describes Cluster Tag structure. Each Cluster Tag describes a   
*  persistent buffer. There is fixed one-to-one mapping between Tag and 
*  a persistent buffer.
*
* @version
*   2011-10-30 - Created. Vamsi V
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cms_cluster_tag.h"
#include "fbe_cms_memory.h"
#include "xorlib_api.h"

void fbe_cms_cluster_tag_init(fbe_cms_cluster_tag_t * p_tag)
{
    fbe_u32_t i;

    fbe_queue_init(&p_tag->m_free_q_element);
    fbe_sll_element_init(&p_tag->m_hash_element);
    p_tag->m_allocation_id.owner_id = 0;
    p_tag->m_allocation_id.buff_id = 0;
    p_tag->m_buffer_checksum = 0;
    p_tag->m_tag_index = 0;
    p_tag->m_owning_sp = 0;
    p_tag->m_sp_lock = FBE_CMS_CLUSTER_TAG_NO_LOCK;
    p_tag->m_client_lock = FBE_CMS_CLUSTER_TAG_NO_LOCK;

    for(i = 0; i < (FBE_CMS_BUFFER_SIZE_IN_SECTORS/8); i++)
    {
        p_tag->m_valid_bits[i] = 0;
        p_tag->m_dirty_bits[i] = 0;
    }
}

fbe_queue_element_t * fbe_cms_tag_get_free_q_element(fbe_cms_cluster_tag_t * p_tag)
{
    return  &p_tag->m_free_q_element; 
}

fbe_sll_element_t * fbe_cms_tag_get_hash_element(fbe_cms_cluster_tag_t * p_tag)
{
    return  &p_tag->m_hash_element; 
}

fbe_cms_alloc_id_t fbe_cms_tag_get_allocation_id(fbe_cms_cluster_tag_t * p_tag)
{
    return  p_tag->m_allocation_id; 
}

fbe_u64_t fbe_cms_tag_get_owner_id(fbe_cms_cluster_tag_t * p_tag)
{
    return  p_tag->m_allocation_id.owner_id; 
}

fbe_u64_t fbe_cms_tag_get_buffer_id(fbe_cms_cluster_tag_t * p_tag)
{
    return  p_tag->m_allocation_id.buff_id; 
}

void fbe_cms_tag_set_allocation_id(fbe_cms_cluster_tag_t * p_tag, fbe_cms_alloc_id_t alloc_id)
{
    p_tag->m_allocation_id = alloc_id; 
}

void fbe_cms_tag_set_buffer_checksum(fbe_cms_cluster_tag_t * p_tag, fbe_u32_t buffer_checksum)
{
    p_tag->m_buffer_checksum = buffer_checksum; 
}

fbe_u32_t fbe_cms_tag_get_buffer_checksum(fbe_cms_cluster_tag_t * p_tag)
{
    return  p_tag->m_buffer_checksum; 
}

fbe_u32_t fbe_cms_tag_get_tag_index(fbe_cms_cluster_tag_t * p_tag)
{
    return  p_tag->m_tag_index; 
}

void fbe_cms_tag_set_tag_index(fbe_cms_cluster_tag_t * p_tag, fbe_u32_t tag_index)
{
    p_tag->m_tag_index = tag_index; 
}

fbe_bool_t fbe_cms_tag_get_owning_sp(fbe_cms_cluster_tag_t * p_tag)
{
    return p_tag->m_owning_sp; 
}

void fbe_cms_tag_set_owning_sp(fbe_cms_cluster_tag_t * p_tag, fbe_bool_t owning_sp)
{
    p_tag->m_owning_sp = owning_sp; 
}

fbe_bool_t fbe_cms_tag_is_buffer_valid(fbe_cms_cluster_tag_t * p_tag)
{
    fbe_u32_t i;

    for(i = 0; i < (FBE_CMS_BUFFER_SIZE_IN_SECTORS/8); i++)
    {
        if(p_tag->m_valid_bits[i] != 0)
        {
            return FBE_TRUE; 
        }
    }
    return FBE_FALSE; 
}

void fbe_cms_tag_set_buffer_valid(fbe_cms_cluster_tag_t * p_tag)
{
    fbe_u32_t i;

    for(i = 0; i < (FBE_CMS_BUFFER_SIZE_IN_SECTORS/8); i++)
    {
        p_tag->m_valid_bits[i] = 1;
        p_tag->m_dirty_bits[i] = 0;
    }
}

fbe_u8_t fbe_cms_tag_get_sp_lock(fbe_cms_cluster_tag_t * p_tag)
{
    return  p_tag->m_sp_lock; 
}

void fbe_cms_tag_set_sp_lock(fbe_cms_cluster_tag_t * p_tag, fbe_u8_t sp_lock)
{
    p_tag->m_sp_lock = sp_lock; 
}

fbe_u8_t fbe_cms_tag_get_client_lock(fbe_cms_cluster_tag_t * p_tag)
{
    return  p_tag->m_client_lock; 
}

void fbe_cms_tag_set_client_lock(fbe_cms_cluster_tag_t * p_tag, fbe_u8_t client_lock)
{
    p_tag->m_client_lock = client_lock; 
}

fbe_cms_cluster_tag_t * fbe_cms_tag_get_tag_from_free_q_element(fbe_queue_element_t * p_element)
{
    /* Its a direct map as sll element is first field in cluster_tag struct  */
    return (fbe_cms_cluster_tag_t *)p_element;
}

fbe_cms_cluster_tag_t * fbe_cms_tag_get_tag_from_hash_element(fbe_sll_element_t * p_sll_element)
{
    return (fbe_cms_cluster_tag_t *)((fbe_u8_t *)p_sll_element - (fbe_u8_t *)(&((fbe_cms_cluster_tag_t *)0)->m_hash_element));
}

void fbe_cms_tag_alloc_prepare(fbe_cms_cluster_tag_t * tag_p, fbe_cms_alloc_id_t alloc_id)
{
    fbe_u32_t i;

	fbe_sll_element_init(&tag_p->m_hash_element);
    tag_p->m_allocation_id.owner_id = alloc_id.owner_id;
    tag_p->m_allocation_id.buff_id = alloc_id.buff_id;
    tag_p->m_sp_lock = FBE_CMS_CLUSTER_TAG_EXCL_LOCK;
    tag_p->m_client_lock = FBE_CMS_CLUSTER_TAG_EXCL_LOCK;

    for(i = 0; i < (FBE_CMS_BUFFER_SIZE_IN_SECTORS/8); i++)
    {
		/* Set buffer to Prepared State */
		tag_p->m_valid_bits[i] = 0x00;
        tag_p->m_dirty_bits[i] = 0xff;
    }

	/* Zero the buffer */
	fbe_zero_memory(fbe_cms_memory_get_va_of_buffer(tag_p->m_tag_index), FBE_CMS_BUFFER_SIZE);

	/* Update checksum */
	tag_p->m_buffer_checksum = xorlib_cook_csum( 0 /*Csum*/, 0 /*Seed is Ignored*/);
}
