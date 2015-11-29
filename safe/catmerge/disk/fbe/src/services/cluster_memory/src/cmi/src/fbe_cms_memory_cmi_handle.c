/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_cms_memory_cmi_handle_msg.c
 ***************************************************************************
 *
 *  Description
 *      Handling of Buffers CMI events
 *    
 ***************************************************************************/

#include "fbe_cms_private.h"
#include "fbe_cmm.h"
#include "fbe_cmi.h"
#include "fbe_cms_cmi.h"
#include "fbe_cms_memory_cmi.h"


fbe_status_t fbe_cms_process_received_memory_cmi_message(fbe_cms_cmi_memory_msg_t *memory_cmi_msg)
{
	switch (memory_cmi_msg->memory_msg_type) {
	case FBE_CMS_MEMORY_CMI_SEND_BUFFER:
		break;
	default:
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s, illegal mdg type:%d\n", __FUNCTION__, memory_cmi_msg->memory_msg_type);
	}

	return FBE_STATUS_OK;
}



