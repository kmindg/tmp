#ifndef FBE_CMS_CLUSTER_TAG_H
#define FBE_CMS_CLUSTER_TAG_H

/***************************************************************************/
/** @file fbe_cluster_tag.h
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
#include "fbe/fbe_single_link_list.h"
#include "fbe_cms_defines.h"
#include "fbe_cms.h"

/*************************
*   DATA STRUCTURES
*************************/

typedef enum fbe_cms_cluster_tag_lock_e {
    FBE_CMS_CLUSTER_TAG_NO_LOCK = 0,
    FBE_CMS_CLUSTER_TAG_SHARED_LOCK = 1,
    FBE_CMS_CLUSTER_TAG_EXCL_LOCK = 2,
} fbe_cms_cluster_tag_lock_t;

#pragma pack(1)/*this structue goes to the disk, better be packed*/
typedef struct fbe_cms_cluster_tag_s{
    fbe_queue_element_t     m_free_q_element;
    fbe_sll_element_t       m_hash_element;
    fbe_cms_alloc_id_t      m_allocation_id;
    fbe_u8_t                m_valid_bits[FBE_CMS_BUFFER_SIZE_IN_SECTORS/8];
    fbe_u8_t                m_dirty_bits[FBE_CMS_BUFFER_SIZE_IN_SECTORS/8];
    fbe_u32_t               m_buffer_checksum;
    fbe_u32_t               m_tag_index;
    fbe_bool_t              m_owning_sp;    
    fbe_u8_t                m_sp_lock;
    fbe_u8_t                m_client_lock;        
}fbe_cms_cluster_tag_t;
#pragma pack()

/*************************
*   FUNCTIONS
*************************/
void fbe_cms_cluster_tag_init(fbe_cms_cluster_tag_t * p_tag);
fbe_queue_element_t * fbe_cms_tag_get_free_q_element(fbe_cms_cluster_tag_t * p_tag);
fbe_sll_element_t * fbe_cms_tag_get_hash_element(fbe_cms_cluster_tag_t * p_tag);
fbe_cms_alloc_id_t fbe_cms_tag_get_allocation_id(fbe_cms_cluster_tag_t * p_tag);
fbe_u64_t fbe_cms_tag_get_owner_id(fbe_cms_cluster_tag_t * p_tag);
fbe_u64_t fbe_cms_tag_get_buffer_id(fbe_cms_cluster_tag_t * p_tag);
void fbe_cms_tag_set_allocation_id(fbe_cms_cluster_tag_t * p_tag, fbe_cms_alloc_id_t alloc_id);
void fbe_cms_tag_set_buffer_checksum(fbe_cms_cluster_tag_t * p_tag, fbe_u32_t buffer_checksum);
fbe_u32_t fbe_cms_tag_get_buffer_checksum(fbe_cms_cluster_tag_t * p_tag);
fbe_u32_t fbe_cms_tag_get_tag_index(fbe_cms_cluster_tag_t * p_tag);
void fbe_cms_tag_set_tag_index(fbe_cms_cluster_tag_t * p_tag, fbe_u32_t tag_index);
fbe_bool_t fbe_cms_tag_get_owning_sp(fbe_cms_cluster_tag_t * p_tag);
void fbe_cms_tag_set_owning_sp(fbe_cms_cluster_tag_t * p_tag, fbe_bool_t owning_sp);
fbe_bool_t fbe_cms_tag_is_buffer_valid(fbe_cms_cluster_tag_t * p_tag);
void fbe_cms_tag_set_buffer_valid(fbe_cms_cluster_tag_t * p_tag);
fbe_u8_t fbe_cms_tag_get_sp_lock(fbe_cms_cluster_tag_t * p_tag);
void fbe_cms_tag_set_sp_lock(fbe_cms_cluster_tag_t * p_tag, fbe_u8_t sp_lock);
fbe_u8_t fbe_cms_tag_get_client_lock(fbe_cms_cluster_tag_t * p_tag);
void fbe_cms_tag_set_client_lock(fbe_cms_cluster_tag_t * p_tag, fbe_u8_t client_lock);
fbe_cms_cluster_tag_t * fbe_cms_tag_get_tag_from_free_q_element(fbe_queue_element_t * p_element);
fbe_cms_cluster_tag_t * fbe_cms_tag_get_tag_from_hash_element(fbe_sll_element_t * p_sll_element);
void fbe_cms_tag_alloc_prepare(fbe_cms_cluster_tag_t * tag_p, fbe_cms_alloc_id_t alloc_id);

#endif /* FBE_CMS_CLUSTER_TAG_H */
