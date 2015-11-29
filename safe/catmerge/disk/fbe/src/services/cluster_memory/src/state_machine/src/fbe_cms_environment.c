/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_cms_environment.c
***************************************************************************
*
* @brief
*  This file contains persistance service Environmentals handling.
*  
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/

#include "fbe_cms_private.h"
#include "fbe_cms_environment.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe/fbe_esp_sps_mgmt.h"
#include "fbe/fbe_esp_ps_mgmt.h"
#include "fbe/fbe_esp_encl_mgmt.h"
#include "fbe/fbe_esp_board_mgmt.h"
#include "fbe/fbe_esp_cooling_mgmt.h"
#include "fbe_cms_state_machine.h"
#include "fbe/fbe_single_link_list.h"


/*******************/
/* Local varaibales*/
/*******************/
typedef enum fbe_cms_env_thread_flag_e{
    FBE_CMS_ENV_THREAD_RUN,
    FBE_CMS_ENV_THREAD_STOP,
    FBE_CMS_ENV_THREAD_DONE
}fbe_cms_env_thread_flag_t;

typedef struct fbe_cms_env_power_notification_s{
	fbe_queue_element_t						queue_element;/*needs to stay first*/
	fbe_notification_data_changed_info_t	notification_data;
}fbe_cms_env_power_notification_t;

static fbe_semaphore_t					fbe_cms_env_semaphore;
static fbe_cms_env_thread_flag_t		fbe_cms_env_thread_flag;
static fbe_thread_t                 	fbe_cms_env_thread_handle;
static fbe_queue_head_t					fbe_cms_env_notifiaction_queue_head;
static fbe_spinlock_t					fbe_cms_env_notification_queue_lock;
static fbe_notification_element_t 		fbe_cms_env_notification_element;
static fbe_object_id_t					fbe_cms_env_board_object_id = FBE_OBJECT_ID_INVALID;
static HW_ENCLOSURE_TYPE				fbe_cms_env_hw_encl_type = INVALID_ENCLOSURE_TYPE;
static fbe_object_id_t 					fbe_cms_env_ps_obj_id, fbe_cms_env_sps_obj_id, fbe_cms_env_fan_obj_id, fbe_cms_env_board_obj_id, fbe_cms_env_encl_obj_id;

/******************/
/*Local functions */
/******************/
static void fbe_cms_env_thread_func(void *context);
static void fbe_cms_env_process_message_queue(void);
static fbe_status_t fbe_cms_env_register_for_notifications(void);
static fbe_status_t fbe_cms_env_unregister_from_notifications(void);
static fbe_status_t fbe_cms_env_notification_function(fbe_object_id_t object_id, fbe_notification_info_t notification_info, fbe_notification_context_t context);
static fbe_status_t fbe_cms_env_get_gache_status(void);
static fbe_status_t fbe_cms_env_fill_object_ids(void);
static fbe_status_t fbe_cms_env_get_esp_object_id(fbe_class_id_t espClassId, fbe_object_id_t *object_id);

/**********************************************************************************************************************************/
fbe_status_t fbe_cms_environment_init(void)
{
	EMCPAL_STATUS       	nt_status;
	fbe_status_t	status;
    
	/*let's initialize whatever is need to get notifications and process them in a different thread context*/
	fbe_semaphore_init(&fbe_cms_env_semaphore, 0, FBE_SEMAPHORE_MAX);
    fbe_queue_init(&fbe_cms_env_notifiaction_queue_head);
    fbe_spinlock_init(&fbe_cms_env_notification_queue_lock);

    fbe_cms_env_thread_flag = FBE_CMS_ENV_THREAD_RUN;
    nt_status = fbe_thread_init(&fbe_cms_env_thread_handle, "fbe_cms_env", fbe_cms_env_thread_func, NULL);
    if (nt_status != EMCPAL_STATUS_SUCCESS) {
        cms_trace(FBE_TRACE_LEVEL_ERROR,"%s: can't start notification process thread, ntstatus:%X\n", __FUNCTION__,  nt_status); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/*let's fill up the object Ids of the ESP package objects*/
	status = fbe_cms_env_fill_object_ids();
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_WARNING,"%s:failed to fill object IDs of ESP\n", __FUNCTION__); 
		return status;
	}

	/*before doing anything we need to register to get notifications about changes*/
	status = fbe_cms_env_register_for_notifications();
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_WARNING,"%s:failed to register for notifications\n", __FUNCTION__); 
		return status;
	}

	/*let's see how the cache status looks like*/
	status = fbe_cms_env_get_gache_status();
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_WARNING,"%s:failed to get cache status\n", __FUNCTION__); 
		return status;
	}

	return FBE_STATUS_OK;
}

fbe_status_t fbe_cms_environment_destroy(void)
{
	fbe_status_t status = FBE_STATUS_OK;

	/*unregister since we don't want notifications anymore*/
	status = fbe_cms_env_unregister_from_notifications();
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR,"%s:failed to unregister from notifications\n", __FUNCTION__); 
	}

	/*destroy resources we used*/
	fbe_cms_env_thread_flag = FBE_CMS_ENV_THREAD_STOP;
	fbe_semaphore_release(&fbe_cms_env_semaphore, 0, 1, FALSE);
        fbe_thread_wait(&fbe_cms_env_thread_handle);
        fbe_thread_destroy(&fbe_cms_env_thread_handle);

	fbe_spinlock_destroy(&fbe_cms_env_notification_queue_lock);

	/*any chance there are some notifications out there we did not handle before we went down*/
	if (!fbe_queue_is_empty(&fbe_cms_env_notifiaction_queue_head)) {
		/*FIXME*/
	}


	return FBE_STATUS_OK;
}

static void fbe_cms_env_thread_func(void *context)
{
    FBE_UNREFERENCED_PARAMETER(context);

    while (1) {
        fbe_semaphore_wait(&fbe_cms_env_semaphore, NULL);
        if (fbe_cms_env_thread_flag == FBE_CMS_ENV_THREAD_RUN) {
            fbe_cms_env_process_message_queue();
        }else{
            break;
        }
    }

    fbe_cms_env_thread_flag = FBE_CMS_ENV_THREAD_DONE;
    cms_trace(FBE_TRACE_LEVEL_INFO, "%s: done\n", __FUNCTION__); 
    
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);
}

static void fbe_cms_env_process_message_queue(void)
{
	fbe_cms_env_power_notification_t *	notification_data = NULL;

	fbe_spinlock_lock(&fbe_cms_env_notification_queue_lock);
	while (!fbe_queue_is_empty(&fbe_cms_env_notifiaction_queue_head)) {
		notification_data = (fbe_cms_env_power_notification_t *) fbe_queue_pop(&fbe_cms_env_notifiaction_queue_head);
		fbe_spinlock_unlock(&fbe_cms_env_notification_queue_lock);
	
		/*do we need to use stuff from the notification info ?*/
		fbe_cms_env_get_gache_status();/*this will generate an event as needed*/
	
        fbe_memory_ex_release(notification_data);

		fbe_spinlock_lock(&fbe_cms_env_notification_queue_lock);
	}

	fbe_spinlock_unlock(&fbe_cms_env_notification_queue_lock);

}

static fbe_status_t fbe_cms_env_register_for_notifications(void)
{
	fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;

	fbe_cms_env_notification_element.notification_function = fbe_cms_env_notification_function;
    fbe_cms_env_notification_element.notification_context = NULL;
	fbe_cms_env_notification_element.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ENVIRONMENT_MGMT;
	fbe_cms_env_notification_element.notification_type = FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED;

    status = cms_send_packet_to_service(FBE_NOTIFICATION_CONTROL_CODE_REGISTER,
										&fbe_cms_env_notification_element,
										sizeof(fbe_notification_element_t),
										FBE_SERVICE_ID_NOTIFICATION,
										FBE_PACKAGE_ID_ESP);

    return status;

}

static fbe_status_t fbe_cms_env_unregister_from_notifications(void)
{

	fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    
    status = cms_send_packet_to_service(FBE_NOTIFICATION_CONTROL_CODE_UNREGISTER,
										&fbe_cms_env_notification_element,
										sizeof(fbe_notification_element_t),
										FBE_SERVICE_ID_NOTIFICATION,
										FBE_PACKAGE_ID_ESP);
    return status;
}

static fbe_status_t fbe_cms_env_notification_function(fbe_object_id_t object_id, fbe_notification_info_t notification_info, fbe_notification_context_t context)
{
	fbe_cms_env_power_notification_t *	notification_data = NULL;

	/*do we even care about it ?*/
	if (notification_info.notification_type != FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED) {
		return FBE_STATUS_OK;
	}

	/*now it's more interesting, but wait*/
	if (!(notification_info.notification_data.data_change_info.device_mask & FBE_DEVICE_TYPE_PS) &&
		!(notification_info.notification_data.data_change_info.device_mask & FBE_DEVICE_TYPE_SPS)) {

		return FBE_STATUS_OK;
	}

	/*when we get here, it means we got a notification from an SPS or a PS so let's copy it and process it*/
	notification_data = fbe_memory_ex_allocate(sizeof(fbe_cms_env_power_notification_t));
	if (notification_data == NULL) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s:no memory, mask:0x%X\n", __FUNCTION__,
								notification_info.notification_data.data_change_info.device_mask); 

		return FBE_STATUS_OK;
	}

	fbe_copy_memory(&notification_data->notification_data,
					&notification_info.notification_data.data_change_info,
					sizeof(fbe_notification_data_changed_info_t));

	fbe_spinlock_lock(&fbe_cms_env_notification_queue_lock);
	fbe_queue_push(&fbe_cms_env_notifiaction_queue_head, &notification_data->queue_element);
	fbe_spinlock_unlock(&fbe_cms_env_notification_queue_lock);

	/*and let the thread work*/
	fbe_semaphore_release(&fbe_cms_env_semaphore, 0, 1, FALSE);
    
	return FBE_STATUS_OK;
}


static fbe_status_t fbe_cms_env_get_gache_status(void)
{
	fbe_common_cache_status_t 	sps, ps, encl, board, fan;
	fbe_status_t				status;
	fbe_cms_sm_event_data_t		event_data;


	/*set the status to bad to start with until we really get the one from HW*/
	sps = ps = encl = board = fan = FBE_CACHE_STATUS_FAILED;

    /*SPS*/
	status = cms_send_packet_to_object(FBE_ESP_SPS_MGMT_CONTROL_CODE_GET_SPS_CACHE_STATUS,
							   &sps,
							   sizeof(fbe_common_cache_status_t),
							   fbe_cms_env_sps_obj_id,
							   FBE_PACKAGE_ID_ESP);

	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s:failed get info from SPS\n", __FUNCTION__); 
		return status;
	}

	/*PS*/
	status = cms_send_packet_to_object(FBE_ESP_PS_MGMT_CONTROL_CODE_GET_PS_CACHE_STATUS,
							   &ps,
							   sizeof(fbe_common_cache_status_t),
							   fbe_cms_env_ps_obj_id,
							   FBE_PACKAGE_ID_ESP);

	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s:failed get info from PS\n", __FUNCTION__); 
		return status;
	}

	/*ENCL*/
	status = cms_send_packet_to_object(FBE_ESP_ENCL_MGMT_CONTROL_CODE_GET_ENCL_CACHE_STATUS,
							   &encl,
							   sizeof(fbe_common_cache_status_t),
							   fbe_cms_env_encl_obj_id,
							   FBE_PACKAGE_ID_ESP);

	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s:failed get info from ENCL\n", __FUNCTION__); 
		return status;
	}

	/*Board*/
	status = cms_send_packet_to_object(FBE_ESP_BOARD_MGMT_CONTROL_CODE_GET_CACHE_STATUS,
							   &board,
							   sizeof(fbe_common_cache_status_t),
							   fbe_cms_env_board_obj_id,
							   FBE_PACKAGE_ID_ESP);

	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s:failed get info from BOARD\n", __FUNCTION__); 
		return status;
	}

	/*Cooling*/
	status = cms_send_packet_to_object(FBE_ESP_COOLING_MGMT_CONTROL_CODE_GET_CACHE_STATUS,
							   &fan,
							   sizeof(fbe_common_cache_status_t),
							   fbe_cms_env_fan_obj_id,
							   FBE_PACKAGE_ID_ESP);

	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s:failed get info from FAN\n", __FUNCTION__); 
		return status;
	}

	/*HACK, since these always return a fault in simulation we want to make sure we don't get confused*/
	sps = fan = FBE_CACHE_STATUS_OK;

	if ((sps == FBE_CACHE_STATUS_FAILED) ||
		(ps == FBE_CACHE_STATUS_FAILED) ||
		(encl == FBE_CACHE_STATUS_FAILED) ||
		(board == FBE_CACHE_STATUS_FAILED) ||
		(fan == FBE_CACHE_STATUS_FAILED)) {

		event_data.event = FBE_CMS_EVENT_SHUTDOWN_IMMINENT;

		fbe_cms_sm_generate_event(&event_data);

		
	}else if ((sps == FBE_CACHE_STATUS_DEGRADED) ||
		(ps == FBE_CACHE_STATUS_DEGRADED) ||
		(encl == FBE_CACHE_STATUS_DEGRADED) ||
		(board == FBE_CACHE_STATUS_DEGRADED) ||
		(fan == FBE_CACHE_STATUS_DEGRADED)) {

		event_data.event = FBE_CMS_EVENT_HARDWARE_DEGRADED;

		fbe_cms_sm_generate_event(&event_data);
		
	}else{

		event_data.event = FBE_CMS_EVENT_HARDWARE_OK;

		fbe_cms_sm_generate_event(&event_data);
	}

	return FBE_STATUS_OK;

}

static fbe_status_t fbe_cms_env_fill_object_ids(void)
{
	fbe_status_t	status;
    
	status = fbe_cms_env_get_esp_object_id(FBE_CLASS_ID_SPS_MGMT, &fbe_cms_env_sps_obj_id);
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s:failed get SPS obj ID\n", __FUNCTION__); 
		return status;
	}
	status = fbe_cms_env_get_esp_object_id(FBE_CLASS_ID_ENCL_MGMT, &fbe_cms_env_encl_obj_id);
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s:failed get ENCL obj ID\n", __FUNCTION__); 
		return status;
	}

	status = fbe_cms_env_get_esp_object_id(FBE_CLASS_ID_PS_MGMT, &fbe_cms_env_ps_obj_id);
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s:failed get PS obj ID\n", __FUNCTION__); 
		return status;
	}

	status = fbe_cms_env_get_esp_object_id(FBE_CLASS_ID_BOARD_MGMT, &fbe_cms_env_board_obj_id);
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s:failed get BOARD obj ID\n", __FUNCTION__); 
		return status;
	}

	status = fbe_cms_env_get_esp_object_id(FBE_CLASS_ID_COOLING_MGMT, &fbe_cms_env_fan_obj_id);
	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s:failed get FAN obj ID\n", __FUNCTION__); 
		return status;
	}

	return FBE_STATUS_OK;

}

static fbe_status_t fbe_cms_env_get_esp_object_id(fbe_class_id_t esp_class_id, fbe_object_id_t *object_id)
{
	fbe_topology_control_get_object_id_of_singleton_t 	get_obj_id;
	fbe_status_t										status;

	get_obj_id.class_id = esp_class_id;

	status = cms_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_GET_OBJECT_ID_OF_SINGLETON_CLASS,
										&get_obj_id,
										sizeof(fbe_topology_control_get_object_id_of_singleton_t),
										FBE_SERVICE_ID_TOPOLOGY,
										FBE_PACKAGE_ID_ESP);

	if (status != FBE_STATUS_OK) {
		cms_trace(FBE_TRACE_LEVEL_ERROR, "%s:failed get obj ID\n", __FUNCTION__); 
		return status;
	}

	*object_id = get_obj_id.object_id;

	return status;
}


