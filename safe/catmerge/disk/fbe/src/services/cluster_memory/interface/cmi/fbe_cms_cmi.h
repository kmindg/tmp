#ifndef FBE_CMS_SERVICE_CMI_H
#define FBE_CMS_SERVICE_CMI_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cms_service_cmi.h
 ***************************************************************************
 *
 * @brief
 *  This file contains CMI related functions and structures for the Cluster memory service.
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe_cmi.h"

typedef enum fbe_cms_cmi_msg_type_e{
    FBE_CMS_CMI_MSG_TYPE_INVALID,
	FBE_CMS_CMI_STATE_MACHINE_MESSAGE,
	FBE_CMS_CMI_MEMORY_MESSAGE,
	FBE_CMS_CMI_CLUSTER_MESSAGE,
	/*******************************/
	/* add new stuff above this line*/
	/*******************************/
    FBE_CMS_CMI_MSG_TYPE_LAST
}fbe_cms_cmi_msg_type_t;

typedef struct fbe_cms_cmi_msg_common_s{
	fbe_cms_cmi_msg_type_t		cms_cmi_msg_type;
    fbe_cmi_event_t 			event_type;/*when we queu it, we need to know what type of a message is it*/
	void * 						callback_context;
	fbe_cpu_id_t				cpu_sent_on;/*we need it until the CMID driver will be core affined, until them we must use it*/
	fbe_cms_run_queue_client_t	run_queue_info;/*we use this to queue to received messages to a seperate thread*/
}fbe_cms_cmi_msg_common_t;

/*functions*/
fbe_status_t fbe_cms_cmi_init(void);
fbe_status_t fbe_cms_cmi_destroy(void);
fbe_status_t fbe_cms_process_lost_peer(void);
void fbe_cms_cmi_send_message(fbe_cms_cmi_msg_type_t msg_type, void * cms_cmi_msg, void *context);
fbe_cms_cmi_msg_common_t * fbe_cms_cmi_get_msg_memory(fbe_cms_cmi_msg_type_t msg_type);
void fbe_cms_cmi_return_msg_memory(fbe_cms_cmi_msg_common_t *cms_cmi_msg);


#endif /*FBE_CMS_SERVICE_CMI_H*/
