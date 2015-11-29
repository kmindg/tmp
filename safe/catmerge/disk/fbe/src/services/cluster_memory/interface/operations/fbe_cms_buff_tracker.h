#ifndef FBE_CMS_BUFF_TRACKER_H
#define FBE_CMS_BUFF_TRACKER_H

/***************************************************************************/
/** @file fbe_cms_ops_tracker.h
***************************************************************************
*
* @brief
*  This file contains tracking structure for buffer operations and supporting 
*  function definitions. 
*
* @version
*   2011-11-22 - Created. Vamsi V 
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe/fbe_types.h"
#include "fbe_cms.h"
#include "fbe_cms_cluster_tag.h"

/*************************
*   DATA STRUCTURES
*************************/
typedef struct fbe_cms_buff_tracker_s{
    fbe_queue_element_t     m_next;
    fbe_cms_buff_req_t      * m_req_p;
    fbe_status_t            m_status;
    fbe_u32_t               m_free_buff_cnt;
    fbe_bool_t              m_contigous_alloc;
    fbe_queue_element_t     m_buff_queue_head;

}fbe_cms_buff_tracker_t;

fbe_status_t fbe_cms_buff_tracker_init(fbe_cms_buff_tracker_t * tracker_p, fbe_cms_buff_req_t * req_p);
void fbe_cms_buff_tracker_push_buffer(fbe_cms_buff_tracker_t * tracker_p, fbe_cms_cluster_tag_t * buff_p);
void fbe_cms_buff_tracker_insert_buffer(fbe_cms_buff_tracker_t * tracker_p, fbe_cms_cluster_tag_t * buff_p);
fbe_cms_cluster_tag_t * fbe_cms_buff_tracker_pop_buffer(fbe_cms_buff_tracker_t * tracker_p);
fbe_cms_buff_tracker_t * fbe_cms_buff_tracker_from_tracker_q_element(fbe_queue_element_t * p_element);
fbe_cms_buff_tracker_t * fbe_cms_buff_tracker_from_buff_q_element(fbe_queue_element_t * p_element);
fbe_cms_alloc_id_t fbe_cms_buff_tracker_get_alloc_id(fbe_cms_buff_tracker_t * tracker_p);

#endif /* FBE_CMS_BUFF_TRACKER_H */
