#ifndef FBE_CMS_CLUSTER_HT_H
#define FBE_CMS_CLUSTER_HT_H

/***************************************************************************/
/** @file fbe_cms_cluster_ht.h
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
#include "fbe_cms_cluster_tag.h"

/*************************
*   DATA STRUCTURES
*************************/
typedef struct fbe_cms_cluster_ht_entry_s
{
    fbe_sll_element_t      m_sll_head;
}fbe_cms_cluster_ht_entry_t;

typedef struct fbe_cms_cluster_ht_s
{
    void * mp_ht_mem;
}fbe_cms_cluster_ht_t;

/*************************
*   FUNCTIONS
*************************/
fbe_sll_element_t * fbe_cms_cluster_ht_entry_get_head(fbe_cms_cluster_ht_entry_t * p_ht_entry);
fbe_status_t fbe_cms_cluster_ht_init();
fbe_status_t fbe_cms_cluster_ht_destroy();
void fbe_cms_cluster_ht_setup(fbe_cms_cluster_ht_t * ht);
fbe_cms_cluster_ht_entry_t * fbe_cms_get_hash_entry(fbe_cms_alloc_id_t alloc_id);
fbe_u64_t fbe_cms_cluster_hash(fbe_cms_alloc_id_t alloc_id);
void fbe_cms_cluster_ht_insert(fbe_cms_cluster_tag_t * p_new_tag);
fbe_bool_t fbe_cms_cluster_ht_delete(fbe_cms_alloc_id_t alloc_id);

#endif /* FBE_CMS_CLUSTER_HT_H */
