/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_bvd_interface_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry points for the basic volume driver object.
 *  This includes the create and destroy methods for the basic volume driver, as
 *  well as the event entry point, load and unload methods.
 * 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_bvd_interface.h"
#include "fbe_base_config_private.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_notification.h"
#include "fbe_block_transport.h"
#include "fbe_bvd_interface_private.h"
#include "fbe/fbe_winddk.h"
#include "VolumeAttributes.h"
#include "fbe_private_space_layout.h"
#include "fbe_service_manager.h"
#include "fbe_transport_memory.h"
#include "base_object_private.h"

#define BVD_MAX_CACHE_EVENT_SEMAPHORE	0x0FFFFFFF
/* Class methods forward declaration.
 */


/* Export class methods  */
fbe_class_methods_t fbe_bvd_interface_class_methods = {FBE_CLASS_ID_BVD_INTERFACE,
                                                       fbe_bvd_interface_load,
                                                       fbe_bvd_interface_unload,
                                                       fbe_bvd_interface_create_object,
                                                       fbe_bvd_interface_destroy_object,
                                                       fbe_bvd_interface_control_entry,
                                                       fbe_bvd_interface_event_entry,
                                                       fbe_bvd_interface_io_entry,
                                                       fbe_bvd_interface_monitor_entry};

fbe_block_transport_const_t fbe_bvd_interface_block_transport_const = {fbe_bvd_interface_block_transport_entry, NULL, NULL, NULL, NULL};

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

static fbe_status_t fbe_bvd_interface_process_attribute_change(fbe_bvd_interface_t * const bvd_interface_p, fbe_event_context_t event_context);
static fbe_bool_t bvd_interface_map_fbe_state_to_bvd_attribute(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *bvd_os_info, fbe_bool_t *immediate);
static void bvd_interface_report_path_state_change(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *bvd_os_info, fbe_path_state_t path_state);
static void bvd_interface_cache_notification_thread(void * context);
static void bvd_interface_cache_notification_dispatch_queue(fbe_bvd_interface_t * bvd_interface_p);
static __forceinline os_device_info_t *bvd_queue_element_to_os_dev_ino(fbe_queue_element_t * queue_element);
static fbe_status_t fbe_bvd_interface_send_detach_edge(fbe_bvd_interface_t *bvd_object_p, fbe_block_transport_control_detach_edge_t *detach_edge_p);
static fbe_status_t bvd_interface_send_lun_degraded_notification(fbe_bvd_interface_t *bvd_interface_p, os_device_info_t *bvd_os_info, fbe_bool_t degraded_flag);

/************************************************************************************************************************************/                                                  

fbe_status_t fbe_bvd_interface_create_object(fbe_packet_t * packet_p, fbe_object_handle_t * object_handle)
{
    fbe_bvd_interface_t *   bvd_obj_p = NULL;
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;

    /* Call the base class create function.*/
    status = fbe_base_config_create_object(packet_p, object_handle);
    if(status != FBE_STATUS_OK){
        return status;
    }

    /* Set class id.*/
    bvd_obj_p = (fbe_bvd_interface_t *)fbe_base_handle_to_pointer(*object_handle);

    fbe_base_object_set_class_id((fbe_base_object_t *) bvd_obj_p, FBE_CLASS_ID_BVD_INTERFACE);

   /* Init the parent class first.*/
    status = fbe_base_config_set_block_transport_const((fbe_base_config_t *)bvd_obj_p, &fbe_bvd_interface_block_transport_const);

    fbe_queue_init(&bvd_obj_p->server_queue_head);
    fbe_queue_init(&bvd_obj_p->cache_notification_queue_head);

    /*start the thread that will process events for cache*/
    fbe_semaphore_init(&bvd_obj_p->cache_notification_semaphore, 0, BVD_MAX_CACHE_EVENT_SEMAPHORE);
    fbe_spinlock_init(&bvd_obj_p->cache_notification_queue_lock);
    bvd_obj_p->cache_notification_therad_run = FBE_TRUE;
    fbe_thread_init(&bvd_obj_p->cache_notification_thread, "fbe_bvd_ca_not", bvd_interface_cache_notification_thread,(void *)bvd_obj_p);

    /*this will call user/krnel specific implementations*/
    fbe_bvd_interface_init(bvd_obj_p);

    /* Force verify to make sure the object is properly initialized before scheduling it.
     * This avoids race condition between scheduler thread and usurper command trying to 
     * initialize the object rotary preset at the same time.
     */
    fbe_lifecycle_verify((fbe_lifecycle_const_t *)&fbe_bvd_interface_lifecycle_const, (fbe_base_object_t *) bvd_obj_p);

    /* Enable lifecycle tracing if requested */
    status = fbe_base_object_enable_lifecycle_debug_trace((fbe_base_object_t *)bvd_obj_p);

    return status;
}

fbe_status_t fbe_bvd_interface_destroy_object(fbe_object_handle_t object_handle)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bvd_interface_t *   bvd_obj_p = NULL;
    
    bvd_obj_p = (fbe_bvd_interface_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)bvd_obj_p, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_DESTROY_OBJECT,
                          "%s entry\n", __FUNCTION__);


    fbe_base_object_lock((fbe_base_object_t*)bvd_obj_p);
    if (!fbe_queue_is_empty(&bvd_obj_p->server_queue_head)){
        
         fbe_base_object_trace((fbe_base_object_t*)bvd_obj_p, 
                               FBE_TRACE_LEVEL_WARNING, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s, server queue is not empty !!!\n", __FUNCTION__);
    }
    fbe_base_object_unlock((fbe_base_object_t*)bvd_obj_p);

    fbe_queue_destroy(&bvd_obj_p->server_queue_head);
    
    /*kill the thread*/
    fbe_base_object_trace((fbe_base_object_t*)bvd_obj_p, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s,stopping cache_notification_thread\n", __FUNCTION__);

    bvd_obj_p->cache_notification_therad_run = FBE_FALSE;
    fbe_semaphore_release(&bvd_obj_p->cache_notification_semaphore, 0, 1, FALSE);
    fbe_thread_wait(&bvd_obj_p->cache_notification_thread);
    fbe_thread_destroy(&bvd_obj_p->cache_notification_thread);

    fbe_spinlock_lock(&bvd_obj_p->cache_notification_queue_lock);
    /*at this point the events queue worth nothing since we destroyed all od_dev_info on detached edges and deleted them from memory*/
    if (!fbe_queue_is_empty(&bvd_obj_p->cache_notification_queue_head)){
        
         fbe_base_object_trace((fbe_base_object_t*)bvd_obj_p, 
                               FBE_TRACE_LEVEL_WARNING, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s, cache_notification_queue is not empty !!!\n", __FUNCTION__);
    }
    fbe_queue_destroy(&bvd_obj_p->cache_notification_queue_head);
    fbe_spinlock_unlock(&bvd_obj_p->cache_notification_queue_lock);

    fbe_semaphore_destroy(&bvd_obj_p->cache_notification_semaphore);
    fbe_spinlock_destroy(&bvd_obj_p->cache_notification_queue_lock);

    fbe_bvd_interface_destroy(bvd_obj_p);

    /* Call parent destructor.*/
    status = fbe_base_config_destroy_object(object_handle);

    return status;
}




fbe_status_t 
fbe_bvd_interface_state_change_event_entry(fbe_bvd_interface_t * const bvd_interface_p, fbe_event_context_t event_context)
{
    fbe_path_state_t        path_state = FBE_PATH_STATE_INVALID;
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_block_edge_t *      edge_p = (fbe_block_edge_t *)event_context;/*the context is the calling edge*/
    os_device_info_t *		bvd_os_info = NULL;

    status = fbe_block_transport_get_path_state(edge_p, &path_state);

    if (status != FBE_STATUS_OK){
        /* Not able to get the path state.*/
        fbe_base_object_trace((fbe_base_object_t*)bvd_interface_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO, 
                              "%s cannot get block edge path state 0x%x!!\n", __FUNCTION__, status);
        return status;
    }

    /*we use the block edge pointer to get the os device info structure we have per LU
    and use it to notify upper layers that there was a state change*/
    bvd_os_info = bvd_interface_block_edge_to_os_device(edge_p);

    /* Set default ready state as Invalid State */
    bvd_os_info->ready_state = FBE_TRI_STATE_INVALID;

    switch (path_state){
        case FBE_PATH_STATE_INVALID:
            break;
        case FBE_PATH_STATE_ENABLED:
        case FBE_PATH_STATE_DISABLED:
        case FBE_PATH_STATE_BROKEN:
        case FBE_PATH_STATE_GONE:
        case FBE_PATH_STATE_SLUMBER:
            bvd_interface_report_path_state_change(bvd_interface_p, bvd_os_info, path_state);
            break;
        default:
            /* This is a path state we do not expect.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_base_object_trace((fbe_base_object_t*)bvd_interface_p, 
                                  FBE_TRACE_LEVEL_WARNING, 
                                  FBE_TRACE_MESSAGE_ID_INFO, 
                                  "%s path state unexpected %d\n", __FUNCTION__, path_state);
            break;

    }
    
    return status;
}

fbe_status_t 
fbe_bvd_interface_event_entry(fbe_object_handle_t object_handle, 
                              fbe_event_type_t event_type,
                              fbe_event_context_t event_context)
{
    fbe_bvd_interface_t *   bvd_interface_p = NULL;
    fbe_status_t            status = FBE_STATUS_OK;
    
    bvd_interface_p = (fbe_bvd_interface_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)bvd_interface_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry event_type %d context 0x%p\n", 
                          __FUNCTION__, event_type, event_context);

    /* First handle the event we have received.*/
    switch (event_type){
        case FBE_EVENT_TYPE_EDGE_STATE_CHANGE:
            status = fbe_bvd_interface_state_change_event_entry(bvd_interface_p, event_context);
            break;
        case FBE_EVENT_TYPE_EDGE_TRAFFIC_LOAD_CHANGE:
            /*we don't really care about it at this level(for now :))*/
            break;
        case FBE_EVENT_TYPE_ATTRIBUTE_CHANGED:
            status  = fbe_bvd_interface_process_attribute_change(bvd_interface_p, event_context);
            break;
        default:
            status = fbe_base_object_event_entry(object_handle, event_type, event_context);
            break;

    }

    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)bvd_interface_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s error processing event %d, status 0x%X\n", 
                               __FUNCTION__, event_type, status);
    }

    return status;
}

fbe_status_t fbe_bvd_interface_remove_os_device(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *os_device_info)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;

    /* Don't need to report the attributes change here.
       Since the remove of the device is triggered by lun edge removal,
       cache has been notified about it.  */
    status = fbe_bvd_interface_unexport_os_device(bvd_object_p, os_device_info);

    return status;

}

fbe_status_t fbe_bvd_interface_add_os_device(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *os_device_info)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_private_space_layout_lun_info_t     lun_info;
    fbe_u8_t					            device_name[64];
    fbe_u8_t					            link_name[64];


    os_device_info->state_change_notify_ptr = NULL;

    /*! @todo We need to communicate with spcache for LUNs that need to be 
     * exported with specific device names, but are also supposed to be cached
     */
    status = fbe_private_space_layout_get_lun_by_lun_number(os_device_info->lun_number, &lun_info);
    if(status == FBE_STATUS_OK)
    {
        if(lun_info.export_device_b != FBE_TRUE)
        {
            return(FBE_STATUS_OK);
        }
        else if(strcmp(lun_info.exported_device_name, "") != 0)
        {
            fbe_zero_memory(device_name, sizeof(device_name));
            fbe_sprintf(device_name, sizeof(device_name), "\\Device\\%s", lun_info.exported_device_name);

            fbe_zero_memory(link_name, sizeof(link_name));
            fbe_sprintf(link_name, sizeof(link_name), "\\DosDevices\\%s", lun_info.exported_device_name);

            status = fbe_bvd_interface_export_os_special_device(
                bvd_object_p, os_device_info, device_name, link_name);

            return(status);
        }
    }

    status = fbe_bvd_interface_export_os_device(bvd_object_p, os_device_info);
    return(status);
}

static fbe_status_t fbe_bvd_interface_process_attribute_change(fbe_bvd_interface_t * const bvd_interface_p, fbe_event_context_t event_context)
{
    fbe_block_edge_t *      edge_p = (fbe_block_edge_t *)event_context;/*the context is the calling edge*/
    os_device_info_t *		bvd_os_info = bvd_interface_block_edge_to_os_device(edge_p);
    fbe_path_attr_t			attr, full_attr;
    fbe_status_t			status = FBE_STATUS_GENERIC_FAILURE;
    fbe_volume_attributes_flags current_attributes;
    fbe_bool_t              immediate = FBE_FALSE;
    
    status = fbe_block_transport_get_path_attributes(edge_p, &attr);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)bvd_interface_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                                  "bvd_proc_attr_change: Failed to get Attr.  Sts:0x%x\n", status);
        return status;
    }

	/*keep it so we can preserve it for next time since the other one wil get XORed out so we know which one is the changed one*/
	full_attr = attr;

    fbe_base_object_trace((fbe_base_object_t*)bvd_interface_p,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                          "LU:%d, bvd_proc_attr:fbe_attr:0x%x,prev_fbe_attr:0x%x curr_attr=0x%x, prev_attr:0x%x\n",
                          bvd_os_info->lun_number, attr, bvd_os_info->previous_path_attributes, bvd_os_info->current_attributes, bvd_os_info->previous_attributes);

	/*let's xor out the one that changed from before*/
	attr ^= bvd_os_info->previous_path_attributes;

	if (attr == 0) {
		/*there was not really a change we care about*/
		return FBE_STATUS_OK;
	}

    current_attributes = bvd_os_info->current_attributes;
    /*in general we check for all bits and the upper layer of bvd will decide if it already knows about it or not
    and if it wants to do anything about it*/
    if (attr & FBE_BLOCK_PATH_ATTR_FLAGS_HAS_BEEN_WRITTEN && !(bvd_os_info->current_attributes & VOL_ATTR_HAS_BEEN_WRITTEN)) {
        bvd_os_info->current_attributes |= VOL_ATTR_HAS_BEEN_WRITTEN;
        fbe_base_object_trace((fbe_base_object_t*)bvd_interface_p,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                              "LU: %d, bvd_proc_attr_change: Set _WRITTEN: attr:0x%x, curr_attr=0x%x, prev_attr:0x%x\n",
                              bvd_os_info->lun_number, attr, bvd_os_info->current_attributes, bvd_os_info->previous_attributes);

    }else if (attr & FBE_BLOCK_PATH_ATTR_FLAGS_HAS_BEEN_WRITTEN && (bvd_os_info->previous_path_attributes & FBE_BLOCK_PATH_ATTR_FLAGS_HAS_BEEN_WRITTEN)) {
        bvd_os_info->current_attributes &= ~VOL_ATTR_HAS_BEEN_WRITTEN;/*techincally not possible but just in case*/
        fbe_base_object_trace((fbe_base_object_t*)bvd_interface_p,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                              "LU:%d, bvd_proc_attr_change: Clear _WRITTEN: attr:0x%x, curr_attr=0x%x, prev_attr:0x%x\n",
                              bvd_os_info->lun_number, attr, bvd_os_info->current_attributes, bvd_os_info->previous_attributes);

    }

    /*LUN will hibernate shortly*/
    if (attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_READY_TO_HIBERNATE && !(bvd_os_info->current_attributes & VOL_ATTR_IDLE_TIME_MET)) {
        bvd_os_info->current_attributes |= VOL_ATTR_IDLE_TIME_MET;
		fbe_base_object_trace((fbe_base_object_t*)bvd_interface_p,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                              "LU:%d, Set VOL_ATTR_IDLE_TIME_MET: fbe_attr:0x%x, curr_attr=0x%x, prev_attr:0x%x\n",
                              bvd_os_info->lun_number, attr, bvd_os_info->current_attributes, bvd_os_info->previous_attributes);

    }else if (attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_READY_TO_HIBERNATE && (bvd_os_info->previous_path_attributes & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_READY_TO_HIBERNATE)) {
        bvd_os_info->current_attributes &= ~VOL_ATTR_IDLE_TIME_MET;
		fbe_base_object_trace((fbe_base_object_t*)bvd_interface_p,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "LU:%d Clear VOL_ATTR_IDLE_TIME_MET: fbe_attr:0x%x, curr_attr=0x%x, prev_attr:0x%x\n",
                               bvd_os_info->lun_number, attr, bvd_os_info->current_attributes, bvd_os_info->previous_attributes);

    }

	/*LUN will hibernate now, after waiting, this comes after VOL_ATTR_IDLE_TIME_MET + N seconds delay*/
    if (attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_HIBERNATING && !(bvd_os_info->current_attributes & VOL_ATTR_STANDBY_TIME_MET)) {
        bvd_os_info->current_attributes |= VOL_ATTR_STANDBY_TIME_MET;
		fbe_base_object_trace((fbe_base_object_t*)bvd_interface_p,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                              "LU:%d, Set VOL_ATTR_STANDBY_TIME_MET: fbe_attr:0x%x, curr_attr=0x%x, prev_attr:0x%x\n",
                              bvd_os_info->lun_number, attr, bvd_os_info->current_attributes, bvd_os_info->previous_attributes);

    }else if (attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_HIBERNATING && (bvd_os_info->previous_path_attributes & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_HIBERNATING)) {
        bvd_os_info->current_attributes &= ~VOL_ATTR_STANDBY_TIME_MET;
		immediate = FBE_TRUE;
		fbe_base_object_trace((fbe_base_object_t*)bvd_interface_p,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "LU:%d Clear VOL_ATTR_STANDBY_TIME_MET: fbe_attr:0x%x, curr_attr=0x%x, prev_attr:0x%x\n",
                               bvd_os_info->lun_number, attr, bvd_os_info->current_attributes, bvd_os_info->previous_attributes);

    }

    /*RG is degraded*/
    if (attr & FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED && !(bvd_os_info->current_attributes & VOL_ATTR_RAID_PROTECTION_DEGRADED)) {
        bvd_os_info->current_attributes |= VOL_ATTR_RAID_PROTECTION_DEGRADED;
        bvd_interface_send_lun_degraded_notification(bvd_interface_p, bvd_os_info, FBE_TRUE);

		fbe_base_object_trace((fbe_base_object_t*)bvd_interface_p,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "LU:%d, Set VOL_ATTR_RAID_PROTECTION_DEGRADED: fbe_attr:0x%x, curr_attr=0x%x, prev_attr:0x%x\n",
                               bvd_os_info->lun_number, attr, bvd_os_info->current_attributes, bvd_os_info->previous_attributes);

    }else if (attr & FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED && (bvd_os_info->previous_path_attributes & FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED)) {
        bvd_os_info->current_attributes &= ~VOL_ATTR_RAID_PROTECTION_DEGRADED;
        bvd_interface_send_lun_degraded_notification(bvd_interface_p, bvd_os_info, FBE_FALSE);

		fbe_base_object_trace((fbe_base_object_t*)bvd_interface_p,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "LU:%d, Clear VOL_ATTR_RAID_PROTECTION_DEGRADED: fbe_attr:0x%x, curr_attr=0x%x, prev_attr:0x%x\n",
                               bvd_os_info->lun_number, attr, bvd_os_info->current_attributes, bvd_os_info->previous_attributes);

    }
    
    if (attr & FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED && !(bvd_os_info->current_attributes & VOL_ATTR_PATH_NOT_PREFERRED)) {
        bvd_os_info->current_attributes |= VOL_ATTR_PATH_NOT_PREFERRED;
		fbe_base_object_trace((fbe_base_object_t*)bvd_interface_p,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "LU:%dSet VOL_ATTR_PATH_NOT_PREFERRED: fbe_attr:0x%x, curr_attr=0x%x, prev_attr:0x%x\n",
                               bvd_os_info->lun_number, attr, bvd_os_info->current_attributes, bvd_os_info->previous_attributes);

    }else if (attr & FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED && (bvd_os_info->previous_path_attributes & FBE_BLOCK_PATH_ATTR_FLAGS_PATH_NOT_PREFERRED)) {
        bvd_os_info->current_attributes &= ~VOL_ATTR_PATH_NOT_PREFERRED;

		fbe_base_object_trace((fbe_base_object_t*)bvd_interface_p,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "LU:%d, Clear VOL_ATTR_PATH_NOT_PREFERRED: fbe_attr:0x%x, curr_attr=0x%x, prev_attr:0x%x\n",
                               bvd_os_info->lun_number, attr, bvd_os_info->current_attributes, bvd_os_info->previous_attributes);

    }

    if (attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_CAN_SAVE_POWER && !(bvd_os_info->current_attributes & VOL_ATTR_STANDBY_POSSIBLE)) {
        bvd_os_info->current_attributes |= VOL_ATTR_STANDBY_POSSIBLE;

		fbe_base_object_trace((fbe_base_object_t*)bvd_interface_p,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "LU:%d, Set VOL_ATTR_STANDBY_POSSIBLE: fbe_attr:0x%x, curr_attr=0x%x, prev_attr:0x%x\n",
                               bvd_os_info->lun_number, attr, bvd_os_info->current_attributes, bvd_os_info->previous_attributes);

    }else if (attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_CAN_SAVE_POWER && (bvd_os_info->previous_path_attributes & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_CAN_SAVE_POWER)) {
        bvd_os_info->current_attributes &= ~VOL_ATTR_STANDBY_POSSIBLE;

		fbe_base_object_trace((fbe_base_object_t*)bvd_interface_p,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "LU %d, Clear VOL_ATTR_STANDBY_POSSIBLE: fbe_attr:0x%x, curr_attr=0x%x, prev_attr:0x%x\n",
                               bvd_os_info->lun_number, attr, bvd_os_info->current_attributes, bvd_os_info->previous_attributes);

    }

    if (attr & FBE_BLOCK_PATH_ATTR_FLAGS_WEAR_LEVEL_REQUIRED) {
        immediate = FBE_TRUE;
		fbe_base_object_trace((fbe_base_object_t*)bvd_interface_p,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "LU:%d, PROCESS WEAR LEVEL: fbe_attr:0x%x, curr_attr=0x%x, prev_attr:0x%x\n",
                               bvd_os_info->lun_number, attr, bvd_os_info->current_attributes, bvd_os_info->previous_attributes);
        status = fbe_block_transport_clear_path_attributes(edge_p, FBE_BLOCK_PATH_ATTR_FLAGS_WEAR_LEVEL_REQUIRED);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)bvd_interface_p,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                      "bvd_proc_attr_change: Failed to get Attr.  Sts:0x%x\n", status);
            return status;
        }

    }

	if (bvd_os_info->current_attributes != current_attributes)
	{
		bvd_os_info->previous_path_attributes = full_attr;/*keep for next time*/
		bvd_interface_report_attribute_change(bvd_interface_p, bvd_os_info, immediate);
    }
    else if (attr & FBE_BLOCK_PATH_ATTR_FLAGS_WEAR_LEVEL_REQUIRED)
    {
        /* If there was no attribute change make sure we report something up anyway
         * so that cache gets the wear level
         */
        bvd_interface_report_attribute_change(bvd_interface_p, bvd_os_info, immediate);
    }
	/*nothing to do with this attribute*/
	return FBE_STATUS_OK;
}

static void bvd_interface_report_path_state_change(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *bvd_os_info, fbe_path_state_t path_state)
{
    fbe_bool_t			send_notification;
    fbe_object_id_t		lu_id;
	fbe_bool_t			is_tripple_mirror = FBE_FALSE;
    fbe_bool_t			immediate_notification = FBE_FALSE;

    fbe_block_transport_get_server_id(&bvd_os_info->block_edge, &lu_id);
    
    /*for private space layout LUNs, we want to be very quiet. They never power save and cache
    expects them to always be in a good shape so we just skip the notification(unless we remove them, but cache should be fine with that)*/
	fbe_private_space_layout_is_lun_on_tripple_mirror(lu_id, &is_tripple_mirror);
    if (FBE_IS_TRUE(is_tripple_mirror) && (path_state != FBE_PATH_STATE_GONE)){

            fbe_base_object_trace((fbe_base_object_t *)bvd_object_p,
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: path state 0x%x,lun num 0x%x skipping cache notification\n", 
                                    __FUNCTION__, bvd_os_info->path_state, bvd_os_info->lun_number);
        return;
    }
    

    /*we are using the same piece of memory here for this LUN. If it gets two more consecuite events before the thread takes 
    them off the queue, we will corrupt the queue by using fbe_queue_push on cache_event_queue_element.
    To prevent that we do it only if there are no pending events for this LUN. We should not worry about missing 
    the latest and greated bits, they are upadted anyway and when the event will be sent to cache, it will get the latest*/
    fbe_spinlock_lock(&bvd_object_p->cache_notification_queue_lock);

    switch (path_state) 
    {
        case FBE_PATH_STATE_ENABLED:
            bvd_os_info->ready_state = FBE_TRI_STATE_TRUE;
            break;

        case FBE_PATH_STATE_DISABLED:
        case FBE_PATH_STATE_BROKEN:
            bvd_os_info->ready_state = FBE_TRI_STATE_FALSE;
            break;

        default:
            bvd_os_info->ready_state = FBE_TRI_STATE_INVALID;
            break; 
    }

    bvd_os_info->path_state = path_state;

    send_notification = bvd_interface_map_fbe_state_to_bvd_attribute(bvd_object_p, bvd_os_info, &immediate_notification );

    if (FBE_IS_FALSE(send_notification)) {
        fbe_spinlock_unlock(&bvd_object_p->cache_notification_queue_lock);
        return;
    }

	/*dont skip the thread becuse it's the one removing the edge to BVD*/
    if (FBE_IS_TRUE(immediate_notification) && (path_state != FBE_PATH_STATE_GONE)) {
		fbe_spinlock_unlock(&bvd_object_p->cache_notification_queue_lock);
		bvd_interface_send_notification_to_cache(bvd_object_p, bvd_os_info);
        return;
	}

    if (!bvd_os_info->cache_event_signaled) {
        /*queue the os dev info the queue, it has new bits in the attributes and we just have to propagate them*/
        bvd_os_info->cache_event_signaled = FBE_TRUE;
        fbe_queue_push(&bvd_object_p->cache_notification_queue_head, &bvd_os_info->cache_event_queue_element);
        fbe_spinlock_unlock(&bvd_object_p->cache_notification_queue_lock);
        fbe_semaphore_release(&bvd_object_p->cache_notification_semaphore, 0, 1, FALSE);
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: path state 0x%x causes lun num 0x%x in Q\n", 
                                __FUNCTION__, bvd_os_info->path_state, bvd_os_info->lun_number);

    }else{
        fbe_spinlock_unlock(&bvd_object_p->cache_notification_queue_lock);
    }
}

static fbe_bool_t bvd_interface_map_fbe_state_to_bvd_attribute(fbe_bvd_interface_t *bvd_object_p, os_device_info_t *bvd_os_info, fbe_bool_t *immediate)
{
    fbe_bool_t return_status = FBE_TRUE;
    fbe_object_id_t		lu_id;
	fbe_bool_t			is_tripple_mirror = FBE_FALSE;

	*immediate = FBE_FALSE;

    fbe_block_transport_get_server_id(&bvd_os_info->block_edge, &lu_id);

     switch (bvd_os_info->path_state){
        case FBE_PATH_STATE_INVALID:
            break;
        case FBE_PATH_STATE_ENABLED:
            /*do not clear the not_preferred bit*/
            /*bvd_os_info->current_attributes &= ~VOL_ATTR_PATH_NOT_PREFERRED;*//*if path is ok we can be preffered back again*/
            // Commented out references of VOL_ATTR_UNATTACHED. AR 582312
//            bvd_os_info->current_attributes &= ~VOL_ATTR_UNATTACHED;
            bvd_os_info->current_attributes &= ~ VOL_ATTR_BECOMING_ACTIVE;
            break;
       case FBE_PATH_STATE_DISABLED:
             /*edge might be disabled if we are in activate and going out of hibernation
            the other case if when RAID goes to activate which is fatal*/
            if ((bvd_os_info->current_attributes & VOL_ATTR_STANDBY) || (bvd_os_info->current_attributes & VOL_ATTR_BECOMING_ACTIVE)) {
                bvd_os_info->current_attributes |= VOL_ATTR_BECOMING_ACTIVE;/*if it was hibernating before let's assume it's now getting out*/
				bvd_os_info->current_attributes &= ~ VOL_ATTR_STANDBY;
				bvd_os_info->current_attributes &= ~VOL_ATTR_STANDBY_TIME_MET;
				bvd_os_info->current_attributes &= ~VOL_ATTR_EXPLICIT_STANDBY;
				*immediate = FBE_TRUE;

                            fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "LU:%d, getting out our Hibernation, path attrib:0x%x\n",bvd_os_info->lun_number, bvd_os_info->current_attributes);
                
            }else{
//                bvd_os_info->current_attributes |= VOL_ATTR_UNATTACHED;

                /*for PSL luns (except the vault) we don't want to scare cache, who is sure that 3 way mirror will
                never break and expects MCR to keep it in a healthy state*/
				fbe_private_space_layout_is_lun_on_tripple_mirror(lu_id, &is_tripple_mirror);
                if (FBE_IS_TRUE(is_tripple_mirror)) {
                    return_status = FBE_FALSE;
                }
            }
            break;
        case FBE_PATH_STATE_BROKEN:
//            bvd_os_info->current_attributes |= VOL_ATTR_UNATTACHED;
            /* When RG is broken, current_attributes is set to VOL_ATTR_UNATTACHED, we need to send LUN_degraded_notification to Admin.*/
            if (!(bvd_os_info->current_attributes & VOL_ATTR_RAID_PROTECTION_DEGRADED)) {
                bvd_interface_send_lun_degraded_notification(bvd_object_p, bvd_os_info, FBE_TRUE);
            }
            break;
		case FBE_PATH_STATE_GONE:
//            bvd_os_info->current_attributes |= VOL_ATTR_UNATTACHED;
            break;
        case FBE_PATH_STATE_SLUMBER:
            /* in base_config we make sure we will not hibernate if non of the servers can support hibernation for any reason,
            ipso facto, if we hibernate, at lease one of them is good which means we can set VOL_ATTR_STANDBY.
			these attributes are also supposed to be set in fbe_bvd_interface_process_attribute_change function*/
            bvd_os_info->current_attributes |= (VOL_ATTR_STANDBY_TIME_MET | VOL_ATTR_STANDBY);
            break;
        default:
            
            break;

    }

     return return_status;
}



void bvd_interface_report_attribute_change(fbe_bvd_interface_t *bvd_interface_p, os_device_info_t *bvd_os_info, fbe_bool_t immediate)
{
    fbe_object_id_t lu_id;
	fbe_bool_t 		is_tripple_mirror = FBE_FALSE;

    fbe_block_transport_get_server_id(&bvd_os_info->block_edge, &lu_id);

    /*for private space layout LUNs, we want to be very quiet. They never power save and cache
    expects them to always be in a good shape so we just skip the notification*/
	fbe_private_space_layout_is_lun_on_tripple_mirror(lu_id, &is_tripple_mirror);
    if(FBE_IS_TRUE(is_tripple_mirror)){
        fbe_base_object_trace((fbe_base_object_t *)bvd_interface_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: path attr 0x%x,lun num 0x%x skipping cache notification\n", 
                                __FUNCTION__, bvd_os_info->current_attributes, bvd_os_info->lun_number);
        return;
    }

    if (immediate) {
        fbe_base_object_trace((fbe_base_object_t *)bvd_interface_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s:Immediate notification:current_attr 0x%x,lun num 0x%x\n", 
                              __FUNCTION__, bvd_os_info->current_attributes, bvd_os_info->lun_number);

        bvd_interface_send_notification_to_cache(bvd_interface_p, bvd_os_info);
        return;
    }

    /*we are using the same piece of memory here for this LUN. If it gets two more consecuite events before the thread takes 
    them off the queue, we will corrupt the queue by using fbe_queue_push on cache_event_queue_element.
    To prevent that we do it only if there are no pending events for this LUN. We should not worry about missing 
    the latest and greated bits, they are upadted anyway and when the event will be sent to cache, it will get the latest*/

    fbe_spinlock_lock(&bvd_interface_p->cache_notification_queue_lock);
    if (!bvd_os_info->cache_event_signaled) {
        bvd_os_info->cache_event_signaled = FBE_TRUE;/*prevent from using it two time in a row since we will corrupt the queue*/
        fbe_queue_push(&bvd_interface_p->cache_notification_queue_head, &bvd_os_info->cache_event_queue_element);
        fbe_spinlock_unlock(&bvd_interface_p->cache_notification_queue_lock);

        fbe_base_object_trace(  (fbe_base_object_t *)bvd_interface_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: path attr 0x%x causes lun num 0x%x in Q\n", 
                                __FUNCTION__, bvd_os_info->current_attributes, bvd_os_info->lun_number);

        fbe_semaphore_release(&bvd_interface_p->cache_notification_semaphore, 0, 1, FALSE);
    }else{
        fbe_spinlock_unlock(&bvd_interface_p->cache_notification_queue_lock);
    }

}

static void bvd_interface_cache_notification_thread(void * context)
{
    fbe_bvd_interface_t * bvd_interface_p = (fbe_bvd_interface_t *)context;
    
    fbe_base_object_trace((fbe_base_object_t*)bvd_interface_p, 
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s entry\n",__FUNCTION__);

    while(1)    
    {
        /*block until someone takes down the semaphore*/
        fbe_semaphore_wait(&bvd_interface_p->cache_notification_semaphore, NULL);
        if(bvd_interface_p->cache_notification_therad_run) {
            /*check if there are any notifictions waiting on the queue*/
            bvd_interface_cache_notification_dispatch_queue(bvd_interface_p);
        } else {
            break;
        }
    }

    fbe_base_object_trace((fbe_base_object_t*)bvd_interface_p, 
                                    FBE_TRACE_LEVEL_DEBUG_LOW,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s exited\n",__FUNCTION__);
    
    fbe_thread_exit(EMCPAL_STATUS_SUCCESS);

}

static void bvd_interface_cache_notification_dispatch_queue(fbe_bvd_interface_t * bvd_interface_p)
{
    fbe_queue_element_t * 	queue_element = NULL;
    os_device_info_t *		os_dev_info = NULL;

    fbe_spinlock_lock(&bvd_interface_p->cache_notification_queue_lock);
    while (!fbe_queue_is_empty(&bvd_interface_p->cache_notification_queue_head)) {
        queue_element = fbe_queue_pop(&bvd_interface_p->cache_notification_queue_head);

        os_dev_info = bvd_queue_element_to_os_dev_ino(queue_element);
        os_dev_info->cache_event_signaled = FBE_FALSE;/*we can use it now to queue another event*/
        fbe_queue_element_init(&os_dev_info->cache_event_queue_element);
        fbe_spinlock_unlock(&bvd_interface_p->cache_notification_queue_lock);

        fbe_base_object_trace(  (fbe_base_object_t *)bvd_interface_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s:LU:%d state:0x%X next is 0x%x\n", 
                                __FUNCTION__, os_dev_info->lun_number, os_dev_info->path_state, (unsigned int)bvd_interface_p->cache_notification_queue_head.next);

        /*send notification*/
        bvd_interface_send_notification_to_cache(bvd_interface_p, os_dev_info);
        if (os_dev_info->path_state == FBE_PATH_STATE_GONE) {
            /* if we get to here, a LUN is being destroyed.
               Let's remove the device, so upper driver can not send any more IO.
               Detach the edge to LUN, and free the os_dev_info memory for this lun*/

            fbe_block_transport_control_detach_edge_t detach_edge;
            fbe_bvd_interface_remove_os_device(bvd_interface_p, os_dev_info);
            detach_edge.block_edge = &os_dev_info->block_edge;
            fbe_bvd_interface_send_detach_edge(bvd_interface_p, &detach_edge);
            fbe_bvd_interface_destroy_edge_ptr(bvd_interface_p, detach_edge.block_edge);
        }

        fbe_spinlock_lock(&bvd_interface_p->cache_notification_queue_lock);/*let's go no to the next one*/
    }

    fbe_spinlock_unlock(&bvd_interface_p->cache_notification_queue_lock);

}

/*!**************************************************************
 * fbe_bvd_interface_send_detach_edge()
 ****************************************************************
 * @brief
 * This function is used to send a detach edge request to the
 * Topology Service. This removes the edge from the client list.
 *
 * @param bvd_object_p  - pointer to BVD object (IN)
 * @param packet_p      - pointer to incoming "destroy edge" packet (IN)
 * @param detach_edge_p - pointer to edge to destroy (IN)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  03/2010 - Created. rundbs
 ****************************************************************/
static fbe_status_t fbe_bvd_interface_send_detach_edge(fbe_bvd_interface_t *bvd_object_p, fbe_block_transport_control_detach_edge_t *detach_edge_p)
{

    fbe_status_t                        status      = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t*                       packet_p = NULL;
    fbe_payload_ex_t*                  payload_p   = NULL;
    fbe_payload_control_operation_t*    control_p   = NULL;


    fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /* Allocate a packet */
    packet_p = fbe_transport_allocate_packet();
    fbe_transport_initialize_sep_packet(packet_p);

    /* Allocate control operation. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_allocate_control_operation(payload_p);

    fbe_payload_control_build_operation(control_p,
                                        FBE_BLOCK_TRANSPORT_CONTROL_CODE_DETACH_EDGE,
                                        detach_edge_p,
                                        sizeof(fbe_block_transport_control_detach_edge_t));

    /* Set completion function */
     fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* We are sending control packet, so we have to form address manually. */
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              detach_edge_p->block_edge->base_edge.server_id);

    /* Control packets should be sent via service manager. */
    status = fbe_service_manager_send_control_packet(packet_p);

    fbe_transport_wait_completion(packet_p);

    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(  (fbe_base_object_t *)bvd_object_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed status %X\n", 
                                __FUNCTION__, status);

        fbe_payload_ex_release_control_operation(payload_p, control_p);
        fbe_transport_release_packet(packet_p);
        return status;
    }

    /* Free the resources we allocated previously.*/
    fbe_payload_ex_release_control_operation(payload_p, control_p);
    fbe_transport_release_packet(packet_p);

    return FBE_STATUS_OK;
}
/**********************************************************
 * end fbe_bvd_interface_send_detach_edge()
 **********************************************************/

/*!**************************************************************
 * bvd_interface_send_lun_degraded_notification()
 ****************************************************************
 * @brief
 * This function is used to send a detach edge request to the
 * Topology Service. This removes the edge from the client list.
 * 
 * @param bvd_interface_p - pointer to bvd interface for tracing.
 * @param bvd_os_info  - pointer to BVD os info (IN)
 * @param degraded_flag - the flag to know if to generate degraded or
 *                        not_degraded notification(IN)
 *
 * @return None.
 *
 * @author
 *  05/23/2012 - Created. Vera Wang
 ****************************************************************/
static fbe_status_t bvd_interface_send_lun_degraded_notification(fbe_bvd_interface_t *bvd_interface_p, os_device_info_t *bvd_os_info, fbe_bool_t degraded_flag)
{
    fbe_status_t                status;
    fbe_object_id_t             lu_id;
    fbe_notification_info_t     notification_info;

    fbe_block_transport_get_server_id(&bvd_os_info->block_edge, &lu_id);

    notification_info.notification_type = FBE_NOTIFICATION_TYPE_LU_DEGRADED_STATE_CHANGED;
    notification_info.class_id = FBE_CLASS_ID_LUN;
    notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_LUN;
    notification_info.notification_data.lun_degraded_flag = degraded_flag;  

    /* send the notification to registered callers. */
    status = fbe_notification_send(lu_id, notification_info);

    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)bvd_interface_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s failed to send lun degraded notification for lun object 0x%x.\n", 
                              __FUNCTION__, lu_id);
        return status;
    }
    return FBE_STATUS_OK;
}

/**********************************************************
 * end bvd_interface_send_lun_degraded_notification()
 **********************************************************/

static __forceinline os_device_info_t *
bvd_queue_element_to_os_dev_ino(fbe_queue_element_t * queue_element)
{
    os_device_info_t * os_dev_info;
    os_dev_info = (os_device_info_t  *)((fbe_u8_t *)queue_element - (fbe_u8_t *)(&((os_device_info_t  *)0)->cache_event_queue_element));
    return os_dev_info;
}
