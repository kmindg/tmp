#ifndef FBE_CMS_SERVICE_CLUSTER_CMI_H
#define FBE_CMS_SERVICE_CLUSTER_CMI_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cms_memory_cmi.h
 ***************************************************************************
 *
 * @brief
 *  This file contains Cluster CMI related functions and structures for the Cluster memory service.
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_cms_cmi.h"

typedef enum cms_cluster_message_type_e{
	FBE_CMS_CLUSTER_CMI_SEND_TAG,
}cms_cluster_message_type_t;

/*dummy structure for state machine*/
typedef struct fbe_cms_cluster_cmi_send_tag_s{
	fbe_u64_t	dummy[4096];/*just create a buffer big enough to exercize allocation logic*/
}fbe_cms_cluster_cmi_send_tag_t;

struct fbe_cms_cmi_cluster_msg_s;
typedef fbe_status_t (*fbe_cms_cluster_cmi_callback_t)(struct fbe_cms_cmi_cluster_msg_s *returned_msg, fbe_cmi_event_t completion_status, void *context);

/*this is the generic data structure used by tags to send data to the peer SP*/
typedef struct fbe_cms_cmi_cluster_msg_s{
	fbe_cms_cmi_msg_common_t		common_msg_data;/*must be first in all CMS CMI mesages*/
    fbe_cms_cluster_cmi_callback_t 	completion_callback;
	cms_cluster_message_type_t		cluster_msg_type;
    
     union{
         fbe_cms_cluster_cmi_send_tag_t	send_tag;/*Just a fake struct*/
    } payload;

}fbe_cms_cmi_cluster_msg_t;


/*functions*/
fbe_status_t fbe_cms_process_received_cluster_cmi_message(fbe_cms_cmi_cluster_msg_t *cluster_cmi_msg);
void fbe_cms_cmi_send_cluster_message(fbe_cms_cmi_cluster_msg_t * cms_cluster_msg, void *context);
fbe_cms_cmi_cluster_msg_t * fbe_cms_cmi_get_cluster_msg();
void fbe_cms_cmi_return_cluster_msg(fbe_cms_cmi_cluster_msg_t *cms_clsuter_cmi_msg);

#endif /*FBE_CMS_SERVICE_CLUSTER_CMI_H*/


