/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_cms_cluster_cmi_handle_msg.c
 ***************************************************************************
 *
 *  Description
 *      Handling of Clsuter tag CMI events
 *    
 ***************************************************************************/

#include "fbe_cms_private.h"
#include "fbe_cmm.h"
#include "fbe_cmi.h"
#include "fbe_cms_cmi.h"
#include "fbe_cms_cluster_cmi.h"


fbe_status_t fbe_cms_process_received_cluster_cmi_message(fbe_cms_cmi_cluster_msg_t *cluster_cmi_msg)
{
	switch (cluster_cmi_msg->cluster_msg_type) {
	case FBE_CMS_CLUSTER_CMI_SEND_TAG:
		break;
	default:
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s, illegal mdg type:%d\n", __FUNCTION__, cluster_cmi_msg->cluster_msg_type);
	}

	return FBE_STATUS_OK;
}



