/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/***************************************************************************/
/** @file fbe_cms_cluster_ht.c
***************************************************************************
*
* @brief
*  This file contains data structures and functions that implement hashtable 
*  for cluster tags.
*
* @version
*   2011-10-30 - Created. Vamsi V 
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cms_cluster_ht.h"
#include "fbe_cms_defines.h"
#include "fbe_cms_private.h"
#include "fbe_cms_main.h"

static fbe_cms_cluster_ht_t g_ht;

fbe_sll_element_t * fbe_cms_cluster_ht_entry_get_head(fbe_cms_cluster_ht_entry_t * p_ht_entry)
{
    return &p_ht_entry->m_sll_head; 
}

fbe_status_t fbe_cms_cluster_ht_init() 
{
    if(FBE_CMS_MAX_HASH_SIZE > FBE_CMS_CMM_MEMORY_BLOCKING_SIZE)
    {   
        /* TODO: Need to implement Catalog */
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: Cluster Hashtable size 0x%X bigger than CMM blocking size 0x%X\n", 
                  __FUNCTION__,
                  FBE_CMS_MAX_HASH_SIZE,
                  FBE_CMS_CMM_MEMORY_BLOCKING_SIZE
                  );
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    g_ht.mp_ht_mem = fbe_cms_cmm_memory_allocate(FBE_CMS_MAX_HASH_SIZE);

    if(!g_ht.mp_ht_mem)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_cms_cluster_ht_setup(&g_ht);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_cluster_ht_destroy() 
{
    fbe_cms_cmm_memory_release(g_ht.mp_ht_mem);
    
    return FBE_STATUS_OK;
}

fbe_cms_cluster_ht_entry_t * fbe_cms_get_hash_entry(fbe_cms_alloc_id_t alloc_id) 
{
    // Hash the Key
    fbe_u64_t hash_idx = fbe_cms_cluster_hash(alloc_id);   

    // Index into hashtable and return pointer to Hash entry
    fbe_cms_cluster_ht_entry_t * p = ((fbe_cms_cluster_ht_entry_t*)g_ht.mp_ht_mem) + hash_idx;

    return p;
}

fbe_u64_t fbe_cms_cluster_hash(fbe_cms_alloc_id_t alloc_id)
{
    /*
     * Below computation is only based on LUN/LBA. We also need to take into account objectID, etc.
     */
    fbe_u64_t hash;
	fbe_u64_t key = (alloc_id.buff_id ^ alloc_id.owner_id);
    
    hash = key & (0xffffffffffff); // initialize the hash with the LBA (low 48 bits)
    hash += (key >> 33) & 0xffff8000; // offset by LUN * 2GB zones
    hash %= (FBE_CMS_MAX_HASH_ENTRIES);

    return hash;
}

void fbe_cms_cluster_ht_setup(fbe_cms_cluster_ht_t * ht)
{
    ULONG l_hash_idx;
    fbe_cms_cluster_ht_entry_t * lp_hash_entry = ht->mp_ht_mem;  

    // Loop through all hash entries within a cmm block
    for (l_hash_idx = 0; l_hash_idx < FBE_CMS_MAX_HASH_ENTRIES; l_hash_idx++)
    {
        // Initialize Hash chain to NUll
        fbe_sll_element_init(fbe_cms_cluster_ht_entry_get_head(lp_hash_entry));
        lp_hash_entry++;
    }
}

void fbe_cms_cluster_ht_insert(fbe_cms_cluster_tag_t * p_new_tag)
{
    fbe_cms_cluster_ht_entry_t  * p_hash_entry;
    fbe_cms_alloc_id_t          alloc_id;

    alloc_id = fbe_cms_tag_get_allocation_id(p_new_tag);
    if(alloc_id.buff_id == FBE_CMS_INVALID_ALLOC_ID)
    {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: Invalid allocID. Tag: 0x%x \n", __FUNCTION__, p_new_tag);
    }

    /* Index into hashtable and get pointer to Hash entry */
    p_hash_entry = fbe_cms_get_hash_entry(alloc_id);

    /* Add passed in Tag to the head of Hash Chain */
    cms_trace(FBE_TRACE_LEVEL_DEBUG_LOW, "%s: allocID: 0x%x \n", __FUNCTION__, alloc_id);
    fbe_sll_push_front(fbe_cms_cluster_ht_entry_get_head(p_hash_entry), fbe_cms_tag_get_hash_element(p_new_tag));
}

fbe_bool_t fbe_cms_cluster_ht_delete(fbe_cms_alloc_id_t alloc_id)
{
    fbe_cms_cluster_ht_entry_t * p_hash_entry;
    fbe_cms_cluster_tag_t * p_tag;

    cms_trace(FBE_TRACE_LEVEL_DEBUG_LOW, "%s: allocID: 0x%x \n", __FUNCTION__, alloc_id);

    /* Index into hashtable and get pointer to Hash entry */
    p_hash_entry = fbe_cms_get_hash_entry(alloc_id);

    /* Search the hash chain of cluster tags for match on allocID */
    for (p_tag = fbe_cms_tag_get_tag_from_hash_element(fbe_sll_front(fbe_cms_cluster_ht_entry_get_head(p_hash_entry))); 
          p_tag; 
          p_tag = fbe_cms_tag_get_tag_from_hash_element(fbe_sll_next(fbe_cms_cluster_ht_entry_get_head(p_hash_entry), fbe_cms_tag_get_hash_element(p_tag))))
    {
        if ((fbe_cms_tag_get_owner_id(p_tag) == alloc_id.owner_id) && 
			(fbe_cms_tag_get_buffer_id(p_tag) == alloc_id.buff_id))
        { 
            cms_trace(FBE_TRACE_LEVEL_DEBUG_LOW, "%s: Found allocID: 0x%x cluster_tag 0x%x \n", __FUNCTION__, alloc_id, p_tag);
            
            /* Remove it from the hash chain */
            fbe_sll_remove(fbe_cms_cluster_ht_entry_get_head(p_hash_entry), fbe_cms_tag_get_hash_element(p_tag));
            return true;
        }
    }

    /* We did not find this element on our hash chain */
    return false;
}

