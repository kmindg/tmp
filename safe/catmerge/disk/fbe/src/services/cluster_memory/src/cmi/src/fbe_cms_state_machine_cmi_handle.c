/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_cms_state_machine_cmi_handle_msg.c
 ***************************************************************************
 *
 *  Description
 *      Handling of SM CMI events
 *    
 ***************************************************************************/

#include "fbe_cms_private.h"
#include "fbe_cmm.h"
#include "fbe_cmi.h"
#include "fbe_cms_cmi.h"
#include "fbe_cms_sm_cmi.h"


fbe_status_t fbe_cms_process_received_state_machine_cmi_message(fbe_cms_cmi_state_machine_msg_t *sm_cmi_msg)
{
	switch (sm_cmi_msg->cms_sm_msg_type) {
	case FBE_CMS_SM_CMI_UPDATER_PEER:
		break;
	default:
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s, illegal mdg type:%d\n", __FUNCTION__, sm_cmi_msg->cms_sm_msg_type);
	}

	return FBE_STATUS_OK;
}



