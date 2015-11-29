#ifndef FBE_CMS_CLUSTER_FREE_Q_H
#define FBE_CMS_CLUSTER_FREE_Q_H

/***************************************************************************/
/** @file fbe_cms_cluster_free_q.h
***************************************************************************
*
* @brief
*  This file contains data structures and functions to implement Free Q for 
*  cluster tags.
*
* @version
*   2011-10-30 - Created. Vamsi V
* 
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cms_cluster_tag.h"

/*************************
*   DATA STRUCTURES
*************************/
/* TODO: Make it per-core in future */
typedef struct fbe_cms_cluster_free_q_s
{
    fbe_queue_element_t m_head;
}fbe_cms_cluster_free_q_t;


/*************************
*   FUNCTIONS
*************************/
fbe_status_t fbe_cms_cluster_free_q_init();
fbe_status_t fbe_cms_cluster_free_q_destroy();
void fbe_cms_cluster_free_q_push(fbe_cms_cluster_tag_t * tag);
fbe_cms_cluster_tag_t * fbe_cms_cluster_free_q_pop();

#endif /* FBE_CMS_CLUSTER_FREE_Q_H */
