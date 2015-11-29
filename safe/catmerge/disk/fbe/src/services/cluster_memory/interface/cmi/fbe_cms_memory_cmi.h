#ifndef FBE_CMS_SERVICE_MEMORY_CMI_H
#define FBE_CMS_SERVICE_MEMORY_CMI_H
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
 *  This file contains Memory CMI related functions and structures for the Cluster memory service.
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_cms_cmi.h"

typedef enum cms_memory_message_type_e{
	FBE_CMS_MEMORY_CMI_SEND_BUFFER,
}cms_memory_message_type_t;

/*dummy structure for state machine*/
typedef struct fbe_cms_memory_cmi_send_buffer_s{
	fbe_u64_t	dummy;
}fbe_cms_memory_cmi_send_buffer_t;

struct fbe_cms_cmi_memory_msg_s;
typedef fbe_status_t (*fbe_cms_memory_cmi_callback_t)(struct fbe_cms_cmi_memory_msg_s *returned_msg, fbe_cmi_event_t completion_status, void *context);

/*this is the generic data structure used by buffers to send data to the peer SP*/
typedef struct fbe_cms_cmi_memory_msg_s{
	fbe_cms_cmi_msg_common_t		common_msg_data;/*must be first in all CMS CMI mesages*/
    fbe_cms_memory_cmi_callback_t 	completion_callback;
	cms_memory_message_type_t		memory_msg_type;
    
     union{
         fbe_cms_memory_cmi_send_buffer_t	send_buffer;/*Just a fake struct*/
    } payload;

}fbe_cms_cmi_memory_msg_t;


/*functions*/
fbe_status_t fbe_cms_process_received_memory_cmi_message(fbe_cms_cmi_memory_msg_t *memory_cmi_msg);
void fbe_cms_cmi_send_memory_message(fbe_cms_cmi_memory_msg_t * cms_memory_msg, void *context);
fbe_cms_cmi_memory_msg_t * fbe_cms_cmi_get_memory_msg();
void fbe_cms_cmi_return_memory_msg(fbe_cms_cmi_memory_msg_t *cms_memory_cmi_msg);


#endif /*FBE_CMS_SERVICE_MEMORY_CMI_H*/

