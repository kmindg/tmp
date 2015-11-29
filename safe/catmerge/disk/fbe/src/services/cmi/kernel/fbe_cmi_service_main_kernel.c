/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_cmi_service_main_kernel.c
 ***************************************************************************
 *
 *  Description
 *      This file contains the Kerenl implementation for the FBE CMI service code
 *      
 *    
 ***************************************************************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_cmi_interface.h"
#include "fbe_cmi_private.h"
#include "fbe/fbe_memory.h"
#include "fbe_cmi.h"
#include "fbe/fbe_trace_interface.h"
#include "CmiUpperInterface.h"
#include "spid_types.h" 
#include "spid_kernel_interface.h" 
#include "fbe/fbe_queue.h"
#include "fbe_cmi_kernel_private.h"
#ifndef ALAMOSA_WINDOWS_ENV
#include "safe_fix_null.h"
#endif /* ALAMOSA_WINDOWS_ENV - STDPORT */

/* Local members*/
fbe_cmi_driver_t 				        fbe_cmi_driver_info;
static SPID			 					fbe_cmi_kernel_my_sp_id;
SPID			 					    fbe_cmi_kernel_other_sp_id;
static fbe_cmi_sp_id_t					fbe_cmi_kernel_sp_id = FBE_CMI_SP_ID_INVALID;
static volatile fbe_bool_t				fbe_cmi_kernel_ok_to_exit = FBE_FALSE;

/* Forward declerations */
static EMCPAL_STATUS fbe_cmi_kernel_conduit_event_wrapper(CMI_CONDUIT_EVENT event, PVOID data_ptr);
fbe_status_t fbe_cmi_kernel_get_cmid_conduit_id(fbe_cmi_conduit_id_t conduit_id, CMI_CONDUIT_ID * cmid_conduit_id);
static fbe_status_t fbe_cmi_kernel_open_cmid_conduit(CMI_CONDUIT_ID conduit_id);
static fbe_status_t fbe_cmi_kernel_close_cmid_conduit(fbe_cmi_conduit_id_t conduit_id);

static fbe_status_t fbe_cmi_kernel_fill_cmid_ioctl(fbe_cmi_kernel_message_info_t *fbe_cmi_kernel_message,
                                                   fbe_cmi_conduit_id_t	conduit,
                                                   fbe_u32_t message_length,
                                                   fbe_cmi_message_t message);

static fbe_status_t fbe_cmi_get_my_sp_id(SPID *cmi_sp_id);
static fbe_status_t fbe_cmi_kernel_process_msg_transmitted_event(PCMI_MESSAGE_TRANSMISSION_EVENT_DATA evnet_ptr);
static fbe_status_t fbe_cmi_kernel_process_msg_received_event(PCMI_MESSAGE_RECEPTION_EVENT_DATA evnet_ptr);
static fbe_status_t fbe_cmi_kernel_process_msg_contact_lost_event(PCMI_SP_CONTACT_LOST_EVENT_DATA evnet_ptr);
static fbe_status_t fbe_cmi_kernel_get_fbe_conduit_id(CMI_CONDUIT_ID cmid_conduit_id, fbe_cmi_conduit_id_t * conduit_id);
static EMCPAL_STATUS fbe_cmi_kernel_send_completion(PEMCPAL_DEVICE_OBJECT DeviceObject,PEMCPAL_IRP Irp,PVOID  Context);
static fbe_status_t fbe_cmi_kernel_process_msg_dma_addresses_needed(PCMI_FIXED_DATA_DMA_NEEDED_EVENT_DATA pDMAAddrEventData);

/*********************************************************************************************************/
static void check_extra_data_length(void)
{
    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_cmi_client_id_t) < CMI_MESSAGE_NUM_BYTES_EXTRA_DATA);
}

fbe_status_t fbe_cmi_init_connections(void)
{
    EMCPAL_STATUS 		nt_status;
    fbe_cmi_conduit_id_t	conduit = FBE_CMI_CONDUIT_ID_INVALID;
    fbe_status_t		status  = FBE_STATUS_GENERIC_FAILURE;
    CMI_CONDUIT_ID 		cmid_conduit_id = Cmi_Invalid_Conduit;
    fbe_package_id_t		package_id = FBE_PACKAGE_ID_INVALID;

    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_LOW, "%s entry\n", __FUNCTION__);

    fbe_cmi_driver_info.cmid_file_object = NULL;

    /*Attach to the CMI device*/
    nt_status = EmcpalDeviceOpen(EmcpalDriverGetCurrentClientObject(),
				 CMI_NT_DEVICE_NAME_CHAR,
                                 &fbe_cmi_driver_info.cmid_file_object);

    if (!EMCPAL_IS_SUCCESS(nt_status)) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,
		      "%s DeviceOpen failed with status = %X\n",
		       __FUNCTION__, nt_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*cache our sp ID for future use, it will never change*/
    status = fbe_cmi_init_sp_id();
    if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,"%s unable to get my SP ID\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_cmi_kernel_allocate_message_pool();
    if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,"%s failed to allocate message pool\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*we need to open only the conduits that this package will use*/
    status = fbe_get_package_id(&package_id);
    if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,"%s Failed to get package ID\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (conduit ++; conduit < FBE_CMI_CONDUIT_ID_LAST; conduit ++) {
        fbe_cmi_kernel_get_cmid_conduit_id(conduit, &cmid_conduit_id);
        if (fbe_cmi_need_to_open_conduit(conduit, package_id)) {
            status = fbe_cmi_kernel_open_cmid_conduit(cmid_conduit_id);
            if (status != FBE_STATUS_OK) {
                fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,"%s can't open conduit %d, status %d\n", __FUNCTION__, conduit, status);
                continue;
            }
        }
        
    }

    return FBE_STATUS_OK;
}

fbe_status_t fbe_cmi_close_connections(void)
{
    fbe_cmi_conduit_id_t	conduit = FBE_CMI_CONDUIT_ID_INVALID;
    CMI_CONDUIT_ID 			cmid_conduit_id = Cmi_Invalid_Conduit;
    fbe_package_id_t		package_id = FBE_PACKAGE_ID_INVALID;
    fbe_status_t			status;
    
    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_LOW, "%s entry\n", __FUNCTION__);

    fbe_cmi_kernel_destroy_message_pool();

    /*we need to close only the conduits that this package will use*/
    status = fbe_get_package_id(&package_id);
    if (status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,"%s Failed to get package ID\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    for (conduit ++; conduit < FBE_CMI_CONDUIT_ID_LAST; conduit ++) {
        if (fbe_cmi_need_to_open_conduit(conduit, package_id)) {
            fbe_cmi_kernel_get_cmid_conduit_id(conduit, &cmid_conduit_id);
            fbe_cmi_kernel_close_cmid_conduit(cmid_conduit_id);
        }
    }

    if (fbe_cmi_driver_info.cmid_file_object != NULL){
        EmcpalDeviceClose(fbe_cmi_driver_info.cmid_file_object);
        fbe_cmi_driver_info.cmid_file_object = NULL;
    }
    return FBE_STATUS_OK;

}

fbe_status_t fbe_cmi_kernel_get_cmid_conduit_id(fbe_cmi_conduit_id_t conduit_id, CMI_CONDUIT_ID * cmid_conduit_id)
{
    *cmid_conduit_id = Cmi_Invalid_Conduit;

    switch(conduit_id){
        case FBE_CMI_CONDUIT_ID_TOPLOGY:
            *cmid_conduit_id = Cmi_SEP_Toplogy_Conduit;
            break;
        case FBE_CMI_CONDUIT_ID_ESP:
            *cmid_conduit_id = Cmi_ESP_Conduit;
            break;
        case FBE_CMI_CONDUIT_ID_NEIT:
            *cmid_conduit_id = Cmi_NEIT_Conduit;
            break;
#if 0/*FIXME, return to these conduits when enabling CMS*/
		case FBE_CMI_CONDUIT_ID_CMS_STATE_MACHINE:
           *cmid_conduit_id = Cmi_SEP_CMS_State_Machine_Conduit;
            break;
		case FBE_CMI_CONDUIT_ID_CMS_TAGS:
            *cmid_conduit_id = Cmi_SEP_CMS_Tag_Conduit;
            break;
#endif
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE0:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core0;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE1:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core1;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE2:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core2;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE3:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core3;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE4:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core4;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE5:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core5;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE6:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core6;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE7:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core7;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE8:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core8;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE9:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core9;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE10:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core10;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE11:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core11;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE12:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core12;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE13:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core13;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE14:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core14;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE15:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core15;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE16:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core16;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE17:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core17;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE18:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core18;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE19:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core19;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE20:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core20;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE21:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core21;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE22:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core22;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE23:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core23;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE24:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core24;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE25:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core25;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE26:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core26;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE27:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core27;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE28:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core28;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE29:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core29;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE30:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core30;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE31:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core31;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE32:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core32;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE33:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core33;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE34:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core34;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE35:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core35;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE36:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core36;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE37:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core37;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE38:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core38;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE39:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core39;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE40:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core40;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE41:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core41;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE42:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core42;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE43:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core43;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE44:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core44;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE45:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core45;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE46:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core46;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE47:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core47;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE48:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core48;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE49:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core49;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE50:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core50;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE51:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core51;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE52:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core52;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE53:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core53;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE54:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core54;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE55:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core55;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE56:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core56;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE57:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core57;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE58:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core58;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE59:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core59;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE60:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core60;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE61:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core61;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE62:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core62;
            break;
        case FBE_CMI_CONDUIT_ID_SEP_IO_CORE63:
            *cmid_conduit_id = Cmi_SEP_IO_Conduit_Core63;
            break;
        default:
            fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s Uknown fbe_cmi_conduit_id_t %X\n", __FUNCTION__, conduit_id);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

STATIC_ASSERT(FBE_CONDUIT_TRANSLATION_MAX_CORE_COUNT_NE_64, MAX_CORES <= 64);

static fbe_status_t
fbe_cmi_kernel_open_cmid_conduit(CMI_CONDUIT_ID conduit_id)
{
    EMCPAL_STATUS 			status = EMCPAL_STATUS_SUCCESS;
    CMI_OPEN_CONDUIT_IOCTL_INFO 	ioctl_info;
    
    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s conduit:%d\n", __FUNCTION__, conduit_id);

    if (fbe_cmi_driver_info.cmid_file_object == NULL) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,
		      "%s cmid_file_object not initialize\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We must issue the ioctl to open the conduit.
     Don't worry about another thread trying to do the same thing
     at the same time; only one caller can successfully issue the
     open-conduit ioctl, and all latecomers will get an "already open"
     error nt_status from CMI.
     Start off by getting a pointer to the CMI pseudo-device object

     Fill in the ioctl packet with the parameters needed by CM's
     conduit.  The CM conduit requires message receptions to be
     confirmed, but it needs no retries.*/

    ioctl_info.conduit_id = conduit_id;
    ioctl_info.event_callback_routine = fbe_cmi_kernel_conduit_event_wrapper;
    ioctl_info.confirm_reception = TRUE;
    ioctl_info.notify_of_remote_array_failures = FALSE;
 
    /* Call the CMI driver to perform the ioctl */
    /* NOTE: could be EmcpalDeviceIoctl without a timeout */
    status = EmcpalExtIrpDoIoctlWithTimeout(
			EmcpalDriverGetCurrentClientObject(),
			IOCTL_CMI_OPEN_CONDUIT,
			EmcpalDeviceGetRelatedDeviceObject(fbe_cmi_driver_info.cmid_file_object),
			&ioctl_info, sizeof(ioctl_info), NULL, 0, 10000);

    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,
		      "%s Failed to open conduit, status %X\n",
		      __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO,
		  "%s Conduit (CMI_CONDUIT_ID):%d opened\n",
		  __FUNCTION__, conduit_id);

    return FBE_STATUS_OK;
}

static EMCPAL_STATUS 
fbe_cmi_kernel_conduit_event_wrapper(CMI_CONDUIT_EVENT event, PVOID data_ptr)
{
    CMI_CONDUIT_ID 						conduit_id = Cmi_Invalid_Conduit;
    PCMI_CLOSE_COMPLETION_EVENT_DATA 	close_completion;
    fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;

    fbe_cmi_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, "%s entry\n", __FUNCTION__);
            
    switch(event)
    {
        case Cmi_Conduit_Event_Close_Completed:
            close_completion = (PCMI_CLOSE_COMPLETION_EVENT_DATA)data_ptr;
            conduit_id = close_completion->ioctl_info_ptr->conduit_id;
            fbe_memory_native_release(close_completion->ioctl_info_ptr->close_handle);
            fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s conduit (CMI_CONDUIT_ID)%d closed successfully\n", __FUNCTION__, conduit_id);
			status = FBE_STATUS_OK;
			fbe_cmi_kernel_ok_to_exit = FBE_TRUE;
            break;

        case Cmi_Conduit_Event_Sp_Contact_Lost:
            status = fbe_cmi_kernel_process_msg_contact_lost_event((PCMI_SP_CONTACT_LOST_EVENT_DATA)data_ptr);
            break;
            
    case Cmi_Conduit_Event_Message_Transmitted:
            status = fbe_cmi_kernel_process_msg_transmitted_event((PCMI_MESSAGE_TRANSMISSION_EVENT_DATA)data_ptr);
            break;
            
        case Cmi_Conduit_Event_Message_Received:
            status = fbe_cmi_kernel_process_msg_received_event((PCMI_MESSAGE_RECEPTION_EVENT_DATA)data_ptr);
            break;

        case Cmi_Conduit_Event_DMA_Addresses_Needed:
            status = fbe_cmi_kernel_process_msg_dma_addresses_needed((PCMI_FIXED_DATA_DMA_NEEDED_EVENT_DATA)data_ptr);
            break;

        default:
            fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s Uknown event %X\n", __FUNCTION__, event);
            status  = FBE_STATUS_GENERIC_FAILURE;
    }

    if (status == FBE_STATUS_OK) {
        return EMCPAL_STATUS_SUCCESS;
    }else if (status == FBE_STATUS_BUSY){
        return EMCPAL_STATUS_DEVICE_BUSY;
    } else{
        return EMCPAL_STATUS_UNSUCCESSFUL;/*the caller should get that and understand something went wrong, even though we got the message*/
    }
    
}


static fbe_status_t fbe_cmi_kernel_close_cmid_conduit(fbe_cmi_conduit_id_t conduit_id)
{
    EMCPAL_STATUS               	status = EMCPAL_STATUS_SUCCESS;
    PCMI_CLOSE_CONDUIT_IOCTL_INFO	ioctl_info_p = NULL;
    fbe_u32_t				wait_exit_count = 0;

    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s entry\n", __FUNCTION__);

    if (fbe_cmi_driver_info.cmid_file_object == NULL) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,
		      "%s cmid_file_object not initialize\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Fill in the ioctl packet with the parameters needed by the
     conduit. We fbe_memory_native_allocate it because closes don't happen
     very often, and we only have 1 outstanding per conduit at a time. It will
     be freed on the ack.*/

    ioctl_info_p = fbe_memory_native_allocate(sizeof(CMI_CLOSE_CONDUIT_IOCTL_INFO));
    if(ioctl_info_p == NULL){
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s Can't allocate memory\n",
		      __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    ioctl_info_p->conduit_id = conduit_id;
    ioctl_info_p->close_handle = ioctl_info_p;

    /* Call the CMI driver to perform the ioctl */
    /* NOTE: could be EmcpalDeviceIoctl without a timeout */
    status = EmcpalExtIrpDoIoctlWithTimeout(
			EmcpalDriverGetCurrentClientObject(),
			IOCTL_CMI_CLOSE_CONDUIT,
			EmcpalDeviceGetRelatedDeviceObject(fbe_cmi_driver_info.cmid_file_object),
			ioctl_info_p, sizeof (*ioctl_info_p), NULL, 0, 10000);

    /* Check to see how the ioctl completed. Any sort of failure is fatal. */
    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,
		      "%s Failed to send close conduit IOCTL, status %X\n",
		      __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*wait here till we got the ack*/
    while (!fbe_cmi_kernel_ok_to_exit && wait_exit_count < 100) {
	fbe_thread_delay(100);
	wait_exit_count ++;
    }

    if (wait_exit_count == 100) {
	fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s Failed to receive ack\n",
		      __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_cmi_send_mailbomb_to_other_sp(fbe_cmi_conduit_id_t conduit_id)
{
    EMCPAL_STATUS		status = EMCPAL_STATUS_SUCCESS;
    PCMI_PANIC_SP_IOCTL_INFO	ioctl_info_p = NULL;

    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s entry\n", __FUNCTION__);

    if (fbe_cmi_driver_info.cmid_file_object == NULL) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,
		      "%s cmid_file_object not initialize\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Fill in the ioctl packet with the parameters needed by the
     conduit. We fbe_memory_native_allocate it because closes don't happen
     very often, and we only have 1 outstanding per conduit at a time. It will
     be freed on the ack.*/

    ioctl_info_p = fbe_memory_native_allocate(sizeof(CMI_PANIC_SP_IOCTL_INFO));
    if(ioctl_info_p == NULL){
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s Can't allocate memory\n",
		      __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_copy_memory(&ioctl_info_p->sp_id, &fbe_cmi_kernel_other_sp_id,
		    sizeof (SPID));
    ioctl_info_p->conduit_id = conduit_id;

    /* Call the CMI driver to perform the ioctl */
    /* NOTE: could be EmcpalDeviceIoctl without a timeout */
    status = EmcpalExtIrpDoIoctlWithTimeout(
			EmcpalDriverGetCurrentClientObject(),
			IOCTL_CMI_PANIC_SP,
			EmcpalDeviceGetRelatedDeviceObject(fbe_cmi_driver_info.cmid_file_object),
			ioctl_info_p, sizeof (*ioctl_info_p), NULL, 0, 10000);

    /* Check to see how the ioctl completed. Any sort of failure is fatal. */
    if (!EMCPAL_IS_SUCCESS(status)) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR,
		      "%s Failed to send PANIC_SP IOCTL, status %X\n",
		      __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO,
		  "%s Conduit %d send PANIC SP to peer.\n",
		  __FUNCTION__, conduit_id);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_cmi_send_message_to_other_sp(fbe_cmi_client_id_t client_id,
                                 fbe_cmi_conduit_id_t	conduit,
                                 fbe_u32_t message_length,
                                 fbe_cmi_message_t message,
                                 fbe_cmi_event_callback_context_t context)
{
    fbe_cmi_kernel_message_info_t *	fbe_cmi_kernel_message = NULL;
    EMCPAL_STATUS 			status = EMCPAL_STATUS_UNSUCCESSFUL;
    fbe_status_t            fbe_status;
    
    /*any chance the other SP is not even there?*/
    if (!fbe_cmi_is_peer_alive()){
        return FBE_STATUS_NO_DEVICE;
    }
    
    /*get a free structure from the pool*/
    fbe_cmi_kernel_message = fbe_cmi_kernel_get_message_info_memory();
    if (fbe_cmi_kernel_message == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*fill up the CMID related information*/
    fbe_status = fbe_cmi_kernel_fill_cmid_ioctl(fbe_cmi_kernel_message, conduit, message_length, message);
    if (fbe_status != FBE_STATUS_OK) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s, failed to fill CMI_TRANSMIT_MESSAGE_IOCTL_INFO\n", __FUNCTION__);
        return fbe_status;
    }

    /*and fill our context information which we will use upon completion*/
    fbe_cmi_kernel_message->client_id = client_id;
    fbe_cmi_kernel_message->context = context;
    fbe_cmi_kernel_message->user_msg = message;

    /*we overload the extra_data to pass the client id to the other side,
    we are protected by assertion at start of code*/
    fbe_copy_memory(fbe_cmi_kernel_message->cmid_ioctl.extra_data, &client_id, sizeof(fbe_cmi_client_id_t));
  

    /*and send.*/
    status = EmcpalExtIrpSendAsync(EmcpalDeviceGetRelatedDeviceObject(fbe_cmi_driver_info.cmid_file_object), fbe_cmi_kernel_message->pirp);

    /*pay attention to status, the rest is done in completion*/
    if (status == EMCPAL_STATUS_PORT_DISCONNECTED) {
        cmi_service_handshake_set_peer_alive(FBE_FALSE);
        return FBE_STATUS_NO_DEVICE;
    } else if (status != EMCPAL_STATUS_SUCCESS &&
	       status != EMCPAL_STATUS_PENDING) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;

}

fbe_status_t fbe_cmi_get_sp_id(fbe_cmi_sp_id_t *cmi_sp_id)
{
    if (cmi_sp_id != NULL) {
        *cmi_sp_id = fbe_cmi_kernel_sp_id;
        return FBE_STATUS_OK;
    }else{
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
}

static fbe_status_t fbe_cmi_kernel_fill_cmid_ioctl(fbe_cmi_kernel_message_info_t *fbe_cmi_kernel_message,
                                                   fbe_cmi_conduit_id_t	conduit,
                                                   fbe_u32_t message_length,
                                                   fbe_cmi_message_t message)
{
    CMI_CONDUIT_ID			cmid_conduit_id = Cmi_Invalid_Conduit;
    fbe_status_t			status = FBE_STATUS_GENERIC_FAILURE;
    PEMCPAL_IO_STACK_LOCATION		newIrpStack = NULL;
    
    fbe_cmi_kernel_message->cmid_ioctl.transmission_handle = fbe_cmi_kernel_message;/*this is how we will find ourselvs back on completion*/

    status = fbe_cmi_kernel_get_cmid_conduit_id(conduit, &cmid_conduit_id);
    if (status != FBE_STATUS_OK) {
        return status;
    }else{
        fbe_cmi_kernel_message->cmid_ioctl.conduit_id = cmid_conduit_id;
    }
    
    /*copy our sp id to the structure*/
    fbe_copy_memory(&fbe_cmi_kernel_message->cmid_ioctl.destination_sp, &fbe_cmi_kernel_other_sp_id, sizeof(SPID));

    /*set up our sgl correctly*/
    fbe_cmi_kernel_message->cmi_sgl[0].PhysAddress = fbe_get_contigmem_physical_address(message);
    fbe_cmi_kernel_message->cmi_sgl[0].length = message_length;
    fbe_cmi_kernel_message->cmi_sgl[1].PhysAddress = NULL;/*termination*/
    fbe_cmi_kernel_message->cmi_sgl[1].length = 0;

    if(fbe_cmi_kernel_message->cmi_sgl[0].PhysAddress == NULL) {
        fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "%s Can't get physical address of payload\n",
                      __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*set the sgl to point to the pre-allocated memory in our data structure*/
    fbe_cmi_kernel_message->cmid_ioctl.physical_floating_data_sgl = fbe_cmi_kernel_message->cmi_sgl;
    fbe_cmi_kernel_message->cmid_ioctl.physical_data_offset = 0;
    fbe_cmi_kernel_message->cmid_ioctl.virtual_floating_data_sgl = NULL;      
    fbe_cmi_kernel_message->cmid_ioctl.fixedDataDescriptionBlob = NULL;     
    fbe_cmi_kernel_message->cmid_ioctl.fixedDataDescriptionBlobLength = 0;
    fbe_cmi_kernel_message->cmid_ioctl.fixed_data_sgl = NULL;
    fbe_cmi_kernel_message->cmid_ioctl.cancelled = FALSE;

    EmcpalIrpBuildLocalIoctl(fbe_cmi_kernel_message->pirp,
			     IOCTL_CMI_TRANSMIT_MESSAGE,
			     &fbe_cmi_kernel_message->cmid_ioctl,
    			     sizeof (fbe_cmi_kernel_message->cmid_ioctl),
			     sizeof (fbe_cmi_kernel_message->cmid_ioctl));

    newIrpStack = EmcpalIrpGetNextStackLocation(fbe_cmi_kernel_message->pirp); 
    EmcpalExtIrpStackMinorFunctionSet(newIrpStack, 0);
    EmcpalIrpStackSetFlags(newIrpStack, 0);
    EmcpalExtIrpCompletionRoutineSet(fbe_cmi_kernel_message->pirp, fbe_cmi_kernel_send_completion, fbe_cmi_kernel_message, TRUE, TRUE, TRUE);

    return FBE_STATUS_OK;

}

static fbe_status_t fbe_cmi_get_my_sp_id(SPID *cmi_sp_id)
{
    EMCPAL_STATUS 			nt_status = EMCPAL_STATUS_UNSUCCESSFUL;
    
    nt_status = spidGetSpid(cmi_sp_id);

    if(nt_status != EMCPAL_STATUS_SUCCESS){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_cmi_kernel_sp_id = ((cmi_sp_id->engine == SP_A) ? FBE_CMI_SP_ID_A : FBE_CMI_SP_ID_B);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_cmi_kernel_process_msg_transmitted_event(PCMI_MESSAGE_TRANSMISSION_EVENT_DATA evnet_ptr)
{
    fbe_cmi_kernel_message_info_t *		fbe_cmi_msg_ptr = NULL;
    fbe_cmi_client_id_t					client_id = FBE_CMI_CLIENT_ID_LAST;
    fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;
    
    /*use the address of the returned data to find our structure*/
    fbe_cmi_msg_ptr = evnet_ptr->ioctl_info_ptr->transmission_handle;

    /*get the client id from the extra info*/
    fbe_copy_memory(&client_id, evnet_ptr->ioctl_info_ptr->extra_data, sizeof(fbe_cmi_client_id_t));

    switch (evnet_ptr->ioctl_info_ptr->transmission_status) {
    case EMCPAL_STATUS_SUCCESS:
        /*and use it to let the client know we received a transmition event for his message*/
        status = fbe_cmi_call_registered_client(FBE_CMI_EVENT_MESSAGE_TRANSMITTED, client_id, 0, fbe_cmi_msg_ptr->user_msg, fbe_cmi_msg_ptr->context);
        break;
    case EMCPAL_STATUS_DEVICE_BUSY:
        /*and use it to let the client know we received a transmition event for his message*/
        status = fbe_cmi_call_registered_client(FBE_CMI_EVENT_PEER_BUSY, client_id, 0, fbe_cmi_msg_ptr->user_msg, fbe_cmi_msg_ptr->context);
        break;
    case EMCPAL_STATUS_PORT_DISCONNECTED:
        /*and use it to let the client know we lost his message*/
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s Mesage lost on CMI_CONDUIT_ID %X, peer might be down\n",
                       __FUNCTION__,  evnet_ptr->ioctl_info_ptr->conduit_id);

        status = fbe_cmi_call_registered_client(FBE_CMI_EVENT_PEER_NOT_PRESENT, client_id, 0, fbe_cmi_msg_ptr->user_msg, fbe_cmi_msg_ptr->context);
        break;
    default:
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s got error 0x%X CMI_CONDUIT_ID %d\n",
                       __FUNCTION__, evnet_ptr->ioctl_info_ptr->transmission_status, evnet_ptr->ioctl_info_ptr->conduit_id);

        status = fbe_cmi_call_registered_client(FBE_CMI_EVENT_FATAL_ERROR, client_id, 0, fbe_cmi_msg_ptr->user_msg, fbe_cmi_msg_ptr->context);
    }

	/*the IO path allocates it's own IOCTLS (and technically should have it's own callback but it does not)*/
	if (client_id != FBE_CMI_CLIENT_ID_SEP_IO ) {
		/*put back on the queue. If the actual IRP we used to send this did not complete, this would not be on the queue itself but rather be
		marked in a way that only the IRP completion would put it back on the queue*/
		fbe_cmi_kernel_return_message_info_memory(fbe_cmi_msg_ptr);
	}

    return status;

}

static fbe_status_t fbe_cmi_kernel_process_msg_received_event(PCMI_MESSAGE_RECEPTION_EVENT_DATA evnet_ptr)
{

    fbe_cmi_client_id_t					client_id = FBE_CMI_CLIENT_ID_LAST;

    /*get the client id from the extra info*/
    fbe_copy_memory(&client_id, evnet_ptr->extra_data, sizeof(fbe_cmi_client_id_t));

    /*and use it to let the client know we received a transmition event for his message*/
    return fbe_cmi_call_registered_client(FBE_CMI_EVENT_MESSAGE_RECEIVED, client_id, (fbe_u32_t)evnet_ptr->num_bytes_floating_data, (fbe_cmi_message_t)evnet_ptr->floating_data_ptr, NULL);
    
}

static fbe_status_t fbe_cmi_kernel_process_msg_contact_lost_event(PCMI_SP_CONTACT_LOST_EVENT_DATA evnet_ptr)
{
    fbe_cmi_conduit_id_t	conduit_id = FBE_CMI_CONDUIT_ID_INVALID;
    fbe_status_t			status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_cmi_kernel_get_fbe_conduit_id(evnet_ptr->conduit_id, &conduit_id);
    if (status != FBE_STATUS_OK) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s peer is dead on CMI_CONDUIT_ID:%d\n", __FUNCTION__, evnet_ptr->conduit_id);
    
    /*let cmi service tell all clients the other SP is dead*/
    fbe_cmi_mark_other_sp_dead(conduit_id);

    return FBE_STATUS_OK;

}

static fbe_status_t fbe_cmi_kernel_get_fbe_conduit_id(CMI_CONDUIT_ID cmid_conduit_id, fbe_cmi_conduit_id_t * fbe_conduit_id)
{
    *fbe_conduit_id = FBE_CMI_CONDUIT_ID_INVALID;

    switch(cmid_conduit_id){
        case Cmi_SEP_Toplogy_Conduit:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_TOPLOGY;
            break;
        case Cmi_ESP_Conduit:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_ESP;
            break;
        case Cmi_NEIT_Conduit:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_NEIT;
            break;
#if 0
		case Cmi_SEP_CMS_State_Machine_Conduit:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_CMS_STATE_MACHINE;
            break;
		case Cmi_SEP_CMS_Tag_Conduit:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_CMS_TAGS;
            break;
#endif
        case Cmi_SEP_IO_Conduit_Core0:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE0;
            break;
        case Cmi_SEP_IO_Conduit_Core1:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE1;
            break;
        case Cmi_SEP_IO_Conduit_Core2:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE2;
            break;
        case Cmi_SEP_IO_Conduit_Core3:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE3;
            break;
        case Cmi_SEP_IO_Conduit_Core4:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE4;
            break;
        case Cmi_SEP_IO_Conduit_Core5:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE5;
            break;
        case Cmi_SEP_IO_Conduit_Core6:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE6;
            break;
        case Cmi_SEP_IO_Conduit_Core7:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE7;
            break;
        case Cmi_SEP_IO_Conduit_Core8:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE8;
            break;
        case Cmi_SEP_IO_Conduit_Core9:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE9;
            break;
        case Cmi_SEP_IO_Conduit_Core10:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE10;
            break;
        case Cmi_SEP_IO_Conduit_Core11:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE11;
            break;
        case Cmi_SEP_IO_Conduit_Core12:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE12;
            break;
        case Cmi_SEP_IO_Conduit_Core13:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE13;
            break;
        case Cmi_SEP_IO_Conduit_Core14:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE14;
            break;
        case Cmi_SEP_IO_Conduit_Core15:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE15;
            break;
        case Cmi_SEP_IO_Conduit_Core16:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE16;
            break;
        case Cmi_SEP_IO_Conduit_Core17:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE17;
            break;
        case Cmi_SEP_IO_Conduit_Core18:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE18;
            break;
        case Cmi_SEP_IO_Conduit_Core19:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE19;
            break;
        case Cmi_SEP_IO_Conduit_Core20:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE20;
            break;
        case Cmi_SEP_IO_Conduit_Core21:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE21;
            break;
        case Cmi_SEP_IO_Conduit_Core22:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE22;
            break;
        case Cmi_SEP_IO_Conduit_Core23:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE23;
            break;
        case Cmi_SEP_IO_Conduit_Core24:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE24;
            break;
        case Cmi_SEP_IO_Conduit_Core25:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE25;
            break;
        case Cmi_SEP_IO_Conduit_Core26:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE26;
            break;
        case Cmi_SEP_IO_Conduit_Core27:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE27;
            break;
        case Cmi_SEP_IO_Conduit_Core28:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE28;
            break;
        case Cmi_SEP_IO_Conduit_Core29:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE29;
            break;
        case Cmi_SEP_IO_Conduit_Core30:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE30;
            break;
        case Cmi_SEP_IO_Conduit_Core31:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE31;
            break;
        case Cmi_SEP_IO_Conduit_Core32:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE32;
            break;
        case Cmi_SEP_IO_Conduit_Core33:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE33;
            break;
        case Cmi_SEP_IO_Conduit_Core34:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE34;
            break;
        case Cmi_SEP_IO_Conduit_Core35:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE35;
            break;
        case Cmi_SEP_IO_Conduit_Core36:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE36;
            break;
        case Cmi_SEP_IO_Conduit_Core37:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE37;
            break;
        case Cmi_SEP_IO_Conduit_Core38:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE38;
            break;
        case Cmi_SEP_IO_Conduit_Core39:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE39;
            break;
        case Cmi_SEP_IO_Conduit_Core40:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE40;
            break;
        case Cmi_SEP_IO_Conduit_Core41:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE41;
            break;
        case Cmi_SEP_IO_Conduit_Core42:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE42;
            break;
        case Cmi_SEP_IO_Conduit_Core43:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE43;
            break;
        case Cmi_SEP_IO_Conduit_Core44:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE44;
            break;
        case Cmi_SEP_IO_Conduit_Core45:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE45;
            break;
        case Cmi_SEP_IO_Conduit_Core46:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE46;
            break;
        case Cmi_SEP_IO_Conduit_Core47:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE47;
            break;
        case Cmi_SEP_IO_Conduit_Core48:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE48;
            break;
        case Cmi_SEP_IO_Conduit_Core49:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE49;
            break;
        case Cmi_SEP_IO_Conduit_Core50:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE50;
            break;
        case Cmi_SEP_IO_Conduit_Core51:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE51;
            break;
        case Cmi_SEP_IO_Conduit_Core52:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE52;
            break;
        case Cmi_SEP_IO_Conduit_Core53:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE53;
            break;
        case Cmi_SEP_IO_Conduit_Core54:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE54;
            break;
        case Cmi_SEP_IO_Conduit_Core55:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE55;
            break;
        case Cmi_SEP_IO_Conduit_Core56:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE56;
            break;
        case Cmi_SEP_IO_Conduit_Core57:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE57;
            break;
        case Cmi_SEP_IO_Conduit_Core58:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE58;
            break;
        case Cmi_SEP_IO_Conduit_Core59:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE59;
            break;
        case Cmi_SEP_IO_Conduit_Core60:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE60;
            break;
        case Cmi_SEP_IO_Conduit_Core61:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE61;
            break;
        case Cmi_SEP_IO_Conduit_Core62:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE62;
            break;
        case Cmi_SEP_IO_Conduit_Core63:
            *fbe_conduit_id = FBE_CMI_CONDUIT_ID_SEP_IO_CORE63;
            break;
        default:
            fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s Uknown CMI_CONDUIT_ID %X\n", __FUNCTION__, cmid_conduit_id);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

STATIC_ASSERT(FBE_CMI_CONDUIT_TRANSLATION_MAX_CORE_COUNT_NE_64, MAX_CORES <= 64);

static EMCPAL_STATUS fbe_cmi_kernel_send_completion(PEMCPAL_DEVICE_OBJECT DeviceObject,PEMCPAL_IRP Irp,PVOID  Context)
{
    fbe_cmi_kernel_message_info_t *	fbe_cmi_kernel_message = (fbe_cmi_kernel_message_info_t *)Context;

	FBE_UNREFERENCED_PARAMETER(DeviceObject);
    
    /*mark it as completed, if it was also acked, it would be returned to the pool*/
	fbe_cmi_kernel_mark_irp_completed(fbe_cmi_kernel_message);

    return EMCPAL_STATUS_MORE_PROCESSING_REQUIRED;
}

static fbe_status_t fbe_cmi_kernel_process_msg_dma_addresses_needed(PCMI_FIXED_DATA_DMA_NEEDED_EVENT_DATA pDMAAddrEventData)
{
    fbe_cmi_conduit_id_t conduit_id;
    fbe_u32_t i, totalLength = 0;
    CMI_PHYSICAL_SG_ELEMENT * sglIn = (CMI_PHYSICAL_SG_ELEMENT *)pDMAAddrEventData->fixedDataDescriptionBlob;

    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s conduit %d\n", __FUNCTION__, pDMAAddrEventData->conduit_id);

    fbe_cmi_kernel_get_fbe_conduit_id(pDMAAddrEventData->conduit_id, &conduit_id);
    if ((conduit_id < FBE_CMI_CONDUIT_ID_SEP_IO_FIRST) || (conduit_id > FBE_CMI_CONDUIT_ID_SEP_IO_LAST))
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_ERROR, "%s event not supported in conduit %d\n", __FUNCTION__, pDMAAddrEventData->conduit_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Copy the sgl from blob to sglMemory */
    for (i = 0; sglIn[i].length && (i < pDMAAddrEventData->maxSglElements); i++)
    {
        pDMAAddrEventData->sglMemory[i].length = sglIn[i].length;
        pDMAAddrEventData->sglMemory[i].PhysAddress = sglIn[i].PhysAddress;
        totalLength += sglIn[i].length;
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "DMA Addr Needed: entry 0x%x phy_addr %llx, length 0x%x\n", i, sglIn[i].PhysAddress, sglIn[i].length);
    }
    pDMAAddrEventData->numSglElementsUsed = i;
    pDMAAddrEventData->numBytesToTransfer = totalLength;
    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "DMA Addr Needed: total_len 0x%x entries 0x%x\n", totalLength, i);

    if (totalLength != pDMAAddrEventData->totalBytes)
    {
        fbe_cmi_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, "DMA Addr Needed: total_len 0x%x not equal 0x%x\n", totalLength, pDMAAddrEventData->totalBytes);
    }

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_cmi_kernel_process_dma_addresses_needed_event(PCMI_FIXED_DATA_DMA_NEEDED_EVENT_DATA evnet_ptr)
{
    
    fbe_cmi_conduit_id_t conduit_id;
    fbe_cmi_client_id_t	client_id = FBE_CMI_CLIENT_ID_LAST;
	
    fbe_cmi_kernel_get_fbe_conduit_id(evnet_ptr->conduit_id, &conduit_id);
    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s conduit %d\n", __FUNCTION__, conduit_id);

    /* Get the client id */
    if ((conduit_id >= FBE_CMI_CONDUIT_ID_SEP_IO_FIRST) && (conduit_id <= FBE_CMI_CONDUIT_ID_SEP_IO_LAST))
    {
        client_id = FBE_CMI_CLIENT_ID_SEP_IO;
    }
    else
    {
        /* Curently only SLF support fixed data transfer */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Let the client know we received the event for his message */
    fbe_cmi_call_registered_client(FBE_CMI_EVENT_DMA_ADDRESSES_NEEDED, client_id, sizeof(CMI_FIXED_DATA_DMA_NEEDED_EVENT_DATA), evnet_ptr, NULL);
    fbe_cmi_trace(FBE_TRACE_LEVEL_INFO, "%s num_sgl %d num_bytes 0x%x current 0x%x\n", __FUNCTION__, 
		evnet_ptr->numSglElementsUsed, evnet_ptr->numBytesToTransfer, evnet_ptr->currentBytes);
    
    return FBE_STATUS_OK;
}

fbe_status_t fbe_cmi_init_sp_id(void)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /*cache our sp ID for future use, it will never change*/
    status = fbe_cmi_get_my_sp_id(&fbe_cmi_kernel_my_sp_id);

    if (status != FBE_STATUS_OK) {
        return FBE_STATUS_GENERIC_FAILURE;
    }else{
        /*flip the bits for other sp*/
        fbe_copy_memory(&fbe_cmi_kernel_other_sp_id, &fbe_cmi_kernel_my_sp_id, sizeof(SPID));
        fbe_cmi_kernel_other_sp_id.engine = ((fbe_cmi_kernel_my_sp_id.engine == SP_A) ? SP_B : SP_A);
        fbe_cmi_trace(FBE_TRACE_LEVEL_INFO,"%s We are %s\n", __FUNCTION__, ((fbe_cmi_kernel_my_sp_id.engine == SP_A) ? "SPA" : "SPB"));
    }

    return status;
}

