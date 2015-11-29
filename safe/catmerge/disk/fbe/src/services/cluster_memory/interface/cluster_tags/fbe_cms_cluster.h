#ifndef FBE_CMS_CLUSTER_H
#define FBE_CMS_CLUSTER_H

/***************************************************************************/
/** @file fbe_cms_cluster.h
***************************************************************************
*
* @brief
*  This file contains data structures and functions for managing clustering of
*  buffers across both SPs.
*
* @version
*   2011-10-30 - Created. Vamsi V
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cms_cluster_tag.h"
#include "fbe_cms_buff_tracker.h"

/*****************************
*   COMPLETION FUNCTION TYPES
******************************/
typedef fbe_status_t (* fbe_cms_buff_op_comp_func_t) (fbe_cms_buff_tracker_t * tracker_p);

/*************************
*   DATA STRUCTURES
*************************/
typedef struct fbe_cms_cluster_s
{
    fbe_u64_t dummy;
}fbe_cms_cluster_t;


/*************************
*   FUNCTION PROTOTYPES
*************************/

fbe_status_t fbe_cms_cluster_init();
fbe_status_t fbe_cms_cluster_destroy();
fbe_status_t fbe_cms_cluster_add_tag(fbe_cms_cluster_tag_t * tag_p);
fbe_bool_t fbe_cms_cluster_allocate_buffers(fbe_cms_buff_tracker_t * tracker_p, 
											fbe_cms_buff_op_comp_func_t next_state);

#endif /* FBE_CMS_CLUSTER_H */
