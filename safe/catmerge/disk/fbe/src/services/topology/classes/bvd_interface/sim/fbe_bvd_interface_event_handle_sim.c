/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_bvd_interface_event_handle_sim.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the kernel implementation for edge events
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_bvd_interface.h"
#include "fbe/fbe_types.h"
#include "fbe_bvd_interface_private.h"
#include "fbe/fbe_sep.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_notification.h"

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/



/************************************************************************************************************************************/													

#if 0
void bvd_interface_report_path_state_change(fbe_path_state_t path_state, os_device_info_t *bvd_os_info)
{
	bvd_os_info->previously_reported_state = path_state;
}

void bvd_interface_report_attribute_change(os_device_info_t *bvd_os_info)
{

}
#endif

// fake a notification from the bvd
void bvd_interface_send_notification_to_cache(fbe_bvd_interface_t *bvd_object_p, os_device_info_t * bvd_os_info)
{
    fbe_status_t                status;
    fbe_object_id_t             lu_id;
    fbe_notification_info_t     notification_info;

    fbe_block_transport_get_server_id(&bvd_os_info->block_edge, &lu_id);

    notification_info.notification_type = FBE_NOTIFICATION_TYPE_CONFIGURATION_CHANGED;
    notification_info.class_id = FBE_CLASS_ID_LUN;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_LUN;

    /* send the notification to registered callers. */
    status = fbe_notification_send(lu_id, notification_info);

    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)bvd_object_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s failed to send lun notification for lun object 0x%x.\n", 
                              __FUNCTION__, lu_id);
    }

}
