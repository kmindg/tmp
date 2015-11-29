/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_bvd_interface_event_handle_kernel.c
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
#include "fbe_bvd_interface_kernel.h"
#include "fbe/fbe_types.h"
#include "fbe_bvd_interface_private.h"
#include "fbe/fbe_sep.h"
#include "fbe/fbe_sep_kernel_interface.h"
#include "VolumeAttributes.h"

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

/************************************************************************************************************************************/													

void bvd_interface_send_notification_to_cache(fbe_bvd_interface_t *bvd_object_p, os_device_info_t * bvd_os_info)
{
    if ((bvd_os_info->state_change_notify_ptr != NULL))
    {
        /*and send notification to client*/
        if (!fbe_atomic_exchange(&bvd_os_info->cancelled, 1)) {

            PEMCPAL_IRP pIrp = (PEMCPAL_IRP)bvd_os_info->state_change_notify_ptr;/*this is the EMCPAL_IRP we kept when it was first sent to us*/

			EmcpalIrpCancelRoutineSet (pIrp, NULL);
            EmcpalExtIrpStatusFieldSet(pIrp, EMCPAL_STATUS_SUCCESS);
            EmcpalExtIrpInformationFieldSet(pIrp, 0);

            fbe_base_object_trace((fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "bvd_notify_cache: SEND:LU:%d,cur:0x%x,prv:0x%x,DevRDY=0x%x,IRP=0x%llX\n", 
                                bvd_os_info->lun_number, bvd_os_info->current_attributes, bvd_os_info->previous_attributes, 
                                bvd_os_info->ready_state, (unsigned long long)pIrp);

            /*reset for next time, so if we get event before the client sends a new one, we will not try to complete the same IRP again*/
            bvd_os_info->state_change_notify_ptr = NULL;

            // AR 571143, update previous_attributes just like in fbe_sep_shim_register_for_change_notification_ioctl
            bvd_os_info->previous_attributes = bvd_os_info->current_attributes;

            /* reset the ready state to Invalid state */
            bvd_os_info->ready_state = FBE_TRI_STATE_INVALID;

            EmcpalIrpCompleteRequest(pIrp);
       }else{
            fbe_base_object_trace((fbe_base_object_t *)bvd_object_p,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "bvd_notify_cache: SKIP:LU:%d,cur:0x%x,prv:0x%x,DevRDY=0x%x,PP=0x%llX CANCELLED\n", 
                                  bvd_os_info->lun_number, bvd_os_info->current_attributes,  bvd_os_info->previous_attributes, 
                                  (unsigned int)bvd_os_info->ready_state, (unsigned long long)bvd_os_info->state_change_notify_ptr);
        }
    }else{
        fbe_base_object_trace((fbe_base_object_t *)bvd_object_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "bvd_notify_cache: SKIP:LU:%d,cur:0x%x,prv:0x%x,DevRDY=0x%x,PP=NULL\n", 
                              bvd_os_info->lun_number, bvd_os_info->current_attributes,  bvd_os_info->previous_attributes,
							  bvd_os_info->ready_state);
    }

}
