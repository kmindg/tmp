#ifndef FBE_CMS_SERVICE_SM_CMI_H
#define FBE_CMS_SERVICE_SM_CMI_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_cms_service_sm_cmi.h
 ***************************************************************************
 *
 * @brief
 *  This file contains SM CMI related functions and structures for the Cluster memory service.
 *
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_cms_cmi.h"

typedef enum cms_sm_message_type_e{
	FBE_CMS_SM_CMI_UPDATER_PEER,
}cms_sm_message_type_t;

/*dummy structure for state machine*/
typedef struct fbe_cms_sm_cmi_update_peer_s{
	fbe_u64_t	dummy;
}fbe_cms_sm_cmi_update_peer_t;

struct fbe_cms_cmi_state_machine_msg_s;
typedef fbe_status_t (*fbe_cms_sm_cmi_callback_t)(struct fbe_cms_cmi_state_machine_msg_s *returned_msg, fbe_cmi_event_t completion_status, void *context);

/*this is the generic data structure used by state machine to send data to the peer SP*/
typedef struct fbe_cms_cmi_state_machine_msg_s{
	fbe_cms_cmi_msg_common_t	common_msg_data;/*must be first in all CMS CMI mesages*/
    fbe_cms_sm_cmi_callback_t 	completion_callback;
	cms_sm_message_type_t		cms_sm_msg_type;
    
     union{
         fbe_cms_sm_cmi_update_peer_t	peer_update;/*Just a fake struct*/
    } payload;

}fbe_cms_cmi_state_machine_msg_t;


/*functions*/
fbe_status_t fbe_cms_process_received_state_machine_cmi_message(fbe_cms_cmi_state_machine_msg_t *sm_cmi_msg);
void fbe_cms_cmi_send_sm_message(fbe_cms_cmi_state_machine_msg_t * cms_sm_msg, void *context);
fbe_cms_cmi_state_machine_msg_t * fbe_cms_cmi_get_sm_msg();
void fbe_cms_cmi_return_sm_msg(fbe_cms_cmi_state_machine_msg_t *cms_sm_cmi_msg);


#endif /*FBE_CMS_SERVICE_SM_CMI_H*/
