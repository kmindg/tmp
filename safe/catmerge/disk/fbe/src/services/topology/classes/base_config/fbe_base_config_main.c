/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_base_config_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry points for the base config object.
 *  This includes the create and destroy methods for the base config, as
 *  well as the event entry point, load and unload methods.
 * 
 * @ingroup base_config_class_files
 * 
 * @version
 *   05/20/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "base_object_private.h"
#include "fbe_base_config_private.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_notification.h"
#include "fbe/fbe_winddk.h"
#include "fbe_event_service.h"
#include "fbe_medic.h"
#include "fbe_database.h"
#include "fbe_cmi.h"
#include "fbe_scheduler.h"
#include "fbe_database.h"

#define FBE_BASE_CONFIG_DENY_COUNT_MAX 3

static fbe_status_t fbe_base_config_verify_permission_data(fbe_base_config_operation_permission_t *operation_permission_p);
static fbe_status_t fbe_base_config_encryption_remove_keys_downstream(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_base_config_remove_keys_downstream_allocation_completion(fbe_memory_request_t * memory_request_p,
                                                                                 fbe_memory_completion_context_t context);
static fbe_status_t fbe_base_config_remove_keys_downstream_subpacket_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

fbe_status_t fbe_base_config_load(void);
fbe_status_t fbe_base_config_unload(void);

fbe_class_methods_t fbe_base_config_class_methods = {FBE_CLASS_ID_BASE_CONFIG,
                                                     fbe_base_config_load,
                                                     fbe_base_config_unload,
                                                     NULL,
                                                     NULL,
                                                     fbe_base_config_control_entry,
                                                     NULL,
                                                     NULL,
                                                     NULL};

/* Note that this class does not export a fbe_class_methods_t,
 * since we never expect to instantiate one of these objects on its own. 
 */

/* This is our global transport constant, which we use to setup a portion of our block 
 * transport server. This is global since it is the same for all raid groups.
 */
static fbe_block_transport_const_t fbe_base_config_block_transport_const = {NULL, NULL, NULL, NULL, NULL};
static fbe_u32_t max_bg_deny_count = FBE_BASE_CONFIG_DENY_COUNT_MAX;
static fbe_system_power_saving_info_t fbe_system_power_saving_info;
static fbe_system_encryption_info_t   fbe_system_encryption_info;
static fbe_spinlock_t base_config_class_lock;
static fbe_base_config_control_system_bg_service_flags_t base_config_control_system_bg_service_flag 
                                                        = FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_NONE;

static fbe_bool_t base_config_enable_load_balance = FBE_TRUE;
static fbe_bool_t base_config_all_bgs_enabled_externally = FBE_FALSE;/*keep track if SEP was told to start all background services one the system finished loading*/
static int base_config_all_bgs_disabled_count = 0;
static fbe_bool_t base_config_deny_credits = FBE_FALSE;

/*!***************************************************************
 * fbe_base_config_load()
 ****************************************************************
 * @brief
 *  This function is called to construct any
 *  global data for the class.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_base_config_load(void)
{
        fbe_status_t status;

        FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_base_config_t) < FBE_MEMORY_CHUNK_SIZE);

        /*set some default power saving related information*/
        fbe_spinlock_init(&base_config_class_lock);
        fbe_system_power_saving_info.enabled = FBE_FALSE;
        fbe_system_power_saving_info.hibernation_wake_up_time_in_minutes = FBE_BASE_CONFIG_HIBERNATION_WAKE_UP_DEFAULT;
        fbe_system_power_saving_info.stats_enabled = FBE_FALSE;

        status = fbe_base_config_monitor_load_verify();
    return status;
}
/* end fbe_base_config_load() */

/*!***************************************************************
 * fbe_base_config_unload()
 ****************************************************************
 * @brief
 *  This function is called to tear down any
 *  global data for the class.
 *
 * @param  - None.
 *
 * @return fbe_status_t
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_base_config_unload(void)
{
    /* Destroy any global data.
     */
    fbe_spinlock_destroy(&base_config_class_lock);
    return FBE_STATUS_OK;
}
/* end fbe_base_config_unload() */

fbe_status_t 
fbe_base_config_create_object(fbe_packet_t * packet_p, fbe_object_handle_t * object_handle)
{
    fbe_base_config_t * base_config = NULL;
    fbe_status_t status;

    /* Call the base class create function. */
    status = fbe_base_object_create_object(packet_p, object_handle);    
    if(status != FBE_STATUS_OK){
        return status;
    }

    base_config = (fbe_base_config_t *)fbe_base_handle_to_pointer(*object_handle);

    /* Set class id.*/
    fbe_base_object_set_class_id((fbe_base_object_t *) base_config, FBE_CLASS_ID_BASE_CONFIG);

    /* It initialze the base config object. */
    fbe_base_config_init((fbe_base_config_t *) base_config);

    /* Force verify to make sure the object is properly initialized before scheduling it.
     * This avoids race condition between scheduler thread and usurper command trying to 
     * initialize the object rotary preset at the same time.
     */
    fbe_lifecycle_verify((fbe_lifecycle_const_t *)&fbe_base_config_lifecycle_const, (fbe_base_object_t *) base_config);

    return FBE_STATUS_OK;
}


/*!***************************************************************
 * fbe_base_config_init()
 ****************************************************************
 * @brief
 *  This function initializes the base config object.
 *
 *  Note that everything that is created here must be
 *  destroyed within fbe_base_config_destroy_object().
 *
 * @param base_config_p - The base config object.
 *
 * @return
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  07/14/2009 - Created. Peter Puhov
 *
 ****************************************************************/
fbe_status_t 
fbe_base_config_init(fbe_base_config_t * const base_config_p)
{
    /* Initialize our local members. */
    fbe_base_config_set_width(base_config_p, FBE_BASE_CONFIG_WIDTH_INVALID);
    fbe_base_config_set_block_edge_ptr(base_config_p, NULL);

    /* Init block transport server. */
    fbe_block_transport_server_init(&base_config_p->block_transport_server);
    base_config_p->global_path_attr = 0;
    base_config_p->background_operation_deny_count = 0;

    /*make sure we are neither active or passive */
    fbe_metadata_element_set_state(&base_config_p->metadata_element, FBE_METADATA_ELEMENT_STATE_INVALID);
    fbe_queue_element_init(&base_config_p->metadata_element.queue_element);

    /* Initialize the event queue head */
    fbe_queue_init(&base_config_p->event_queue_head);
    fbe_spinlock_init(&base_config_p->event_queue_lock);

    /*initialize power saving related parameter*/
    base_config_p->power_saving_idle_time_in_seconds = FBE_BASE_CONFIG_DEFAULT_IDLE_TIME_IN_SECONDS;
    base_config_p->power_save_capable.spindown_qualified = FBE_TRUE;
        
    fbe_base_config_clear_flag(base_config_p, FBE_BASE_CONFIG_FLAG_POWER_SAVING_ENABLED);
    base_config_p->hibernation_time = 0;

	base_config_p->resource_priority = 0;

    base_config_p->encryption_state = FBE_BASE_CONFIG_ENCRYPTION_STATE_INVALID;

    base_config_p->key_handle = NULL;

    return FBE_STATUS_OK;
}
/* end fbe_base_config_init() */

/*!***************************************************************
 * fbe_base_config_destroy_object()
 ****************************************************************
 * @brief
 *  This function is called to destroy the base config object.
 *
 *  This function tears down everything that was created in
 *  fbe_base_config_init().
 *
 * @param object_handle - The object being destroyed.
 *
 * @return fbe_status_t
 *  STATUS of the destroy operation.
 *
 * @author
 *  05/20/2009 - Created. RPF
 *
 ****************************************************************/
fbe_status_t 
fbe_base_config_destroy_object(fbe_object_handle_t object_handle)
{
    fbe_status_t        status = FBE_STATUS_OK;
    fbe_base_config_t  *base_config_p = NULL;
    fbe_event_t        *data_event_p = NULL;

    fbe_base_object_trace((fbe_base_object_t*)fbe_base_handle_to_pointer(object_handle), 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_DESTROY_OBJECT, 
                          "%s entry\n", __FUNCTION__);
    
    base_config_p = (fbe_base_config_t *)fbe_base_handle_to_pointer(object_handle);

    /* Destroy block transport server. */
    status = fbe_block_transport_server_destroy(&base_config_p->block_transport_server);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, 
                          FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s fbe_block_transport_server_destroy failed!\n", __FUNCTION__);

        return status;
    }

    fbe_metadata_element_destroy(&base_config_p->metadata_element);

    /* Check if the event_queue in base_config is empty. If not, 
     * drain the queue before destroying the base config/object..
     */
    if(!fbe_base_config_event_queue_is_empty(base_config_p))
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s, Event Q is NOT empty !!!\n", __FUNCTION__);

        /* Drain the event queue */
        while (!fbe_base_config_event_queue_is_empty(base_config_p))
        {
            /* Pop the data event */
            fbe_base_config_event_queue_pop_event(base_config_p, &data_event_p);
    
            /* Deny the event for further processing */
            fbe_event_set_flags(data_event_p, FBE_EVENT_FLAG_DENY);

            /* Complete the event with appropriate status */
            fbe_event_complete(data_event_p);
        } 
    }

    /* Destroy the event queue and its lock.. */
    fbe_queue_destroy(&base_config_p->event_queue_head);
    fbe_spinlock_destroy(&base_config_p->event_queue_lock);

    fbe_base_object_trace((fbe_base_object_t*)base_config_p, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, 
                          FBE_TRACE_MESSAGE_ID_DESTROY_OBJECT, 
                          "%s Calling destroy object.. Event Q is empty.\n", __FUNCTION__);

    /* Call parent destructor. */
    status = fbe_base_object_destroy_object(object_handle);

    return status;
}
/* end fbe_base_config_destroy_object */

/*!***************************************************************
 * @fn fbe_base_config_detach_edges(fbe_base_config_t * base_config,
 *                                  fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *  This function will remove all the edges with its downstream
 *  objects, it will go through all the edges connected to it and
 *  it will detach it.
 *
 * @param object_handle - The base_config handle.
 * @param packet_p - The io packet that is arriving.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK: If edge detach passes successfully
                   and on error it returns the error which it receive
                   while detaching edge with downstream object.
 *
 * @author
 *  05/20/2009 - Created. DP.
 ****************************************************************/
fbe_status_t 
fbe_base_config_detach_edges(fbe_base_config_t * base_config,
                             fbe_packet_t * packet)
{
    fbe_block_edge_t *edge_p = NULL;
    fbe_status_t status;
    fbe_u32_t i;

    fbe_base_object_trace((fbe_base_object_t *)base_config,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /* Go through the all the edges connected to this base 
     * config object.
     */
    for(i = 0; i < base_config->width; i++)
    {
        fbe_base_config_get_block_edge(base_config, &edge_p, i);

        if (edge_p == NULL) {
            /* No edges were attached in the first place, just return success.
             */
            fbe_base_object_trace((fbe_base_object_t *)base_config,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Detach block edge found no edges attached.\n",__FUNCTION__);
            return FBE_STATUS_OK;
        }

        /* Detach the specific edge with downstream object.
         */
        status = fbe_base_config_detach_edge (base_config, packet, edge_p);

        if (status == FBE_STATUS_NOT_INITIALIZED)
        {
            continue;
        }
        else if(status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)base_config,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Detach block edge failed\n",__FUNCTION__);
            return status;
        }
    }

    return FBE_STATUS_OK;
}


/*!***************************************************************
 * @fn fbe_base_config_detach_edge(fbe_base_config_t * base_config,
 *                                 fbe_packet_t * packet,
 *                                 fbe_block_edge_t *edge_p)
 ****************************************************************
 * @brief
 *  This function will remove the specific edge with the downstream
 *  object.
 *
 * @param object_handle - The base_config handle.
 * @param packet_p - The io packet that is arriving.
 * @param edge_p - Edge which caller wants to detach with.
 * @return
 *  fbe_status_t - FBE_STATUS_OK: If edge detach passes successfully
                   and on error it returns the error which it receive
                   while detaching edge with downstream object.
 *
 * @author
 *  05/20/2009 - Created. DP.
 ****************************************************************/
fbe_status_t 
fbe_base_config_detach_edge(fbe_base_config_t * base_config_p, fbe_packet_t * packet, fbe_block_edge_t *edge_p)
{

    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_block_transport_control_detach_edge_t detach_edge;
    fbe_path_state_t path_state;
    fbe_payload_ex_t * payload = NULL;

    fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /* Get the payload pointer from the packet. */
    payload = fbe_transport_get_payload_ex(packet);

    if (payload == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Failed to get the payload from packet\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    detach_edge.block_edge = edge_p;

    /* Get the path state of edge and if it is INVALID return error. */
    fbe_block_transport_get_path_state(edge_p, &path_state);

    if(path_state == FBE_PATH_STATE_INVALID) {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, 
                        FBE_TRACE_LEVEL_DEBUG_LOW,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s Path state is invalid\n",__FUNCTION__);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    control_operation = fbe_payload_ex_allocate_control_operation(payload); 

    if(control_operation == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Failed to allocate control operation\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Create the payload control operation to detach edge the edge. */
    fbe_payload_control_build_operation(control_operation,
                                        FBE_BLOCK_TRANSPORT_CONTROL_CODE_DETACH_EDGE,
                                        &detach_edge,
                                        sizeof(fbe_block_transport_control_detach_edge_t));

    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* Send control packet on edges, it will be trasported to server object-id of edge. */
    fbe_block_transport_send_control_packet(edge_p, packet);

    fbe_transport_wait_completion(packet);

    fbe_payload_ex_release_control_operation(payload, control_operation);

    /* Reinitialize the edge (we may want to create reinit for the block edges as well */
    fbe_base_edge_reinit((fbe_base_edge_t *)edge_p);

    return FBE_STATUS_OK;

}
/********************************************
 * end fbe_base_config_detach_edge()
 ********************************************/

fbe_status_t 
fbe_base_config_bouncer_entry(fbe_base_config_t * base_config, fbe_packet_t * packet)
{
    fbe_status_t status;

    /* The packet may be enqueued */
    fbe_transport_set_cancel_function(packet, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)base_config);
    status = fbe_block_transport_server_bouncer_entry(&base_config->block_transport_server,
                                                        packet,
                                                        base_config);
    if(status == FBE_STATUS_GENERIC_FAILURE){
        fbe_base_object_trace((fbe_base_object_t*)base_config, 
                          FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s fbe_block_transport_server_bouncer_entry failed!\n", __FUNCTION__);

    }

    return status;
}

fbe_status_t 
fbe_base_config_set_block_transport_const(fbe_base_config_t * base_config, fbe_block_transport_const_t * block_transport_const)
{
    fbe_status_t status;

    status = fbe_block_transport_server_set_block_transport_const(&base_config->block_transport_server, block_transport_const, base_config);

    return status;
}

fbe_status_t 
fbe_base_config_set_outstanding_io_max(fbe_base_config_t * base_config, fbe_u32_t outstanding_io_max)
{
    fbe_status_t status;

    status = fbe_block_transport_server_set_outstanding_io_max(&base_config->block_transport_server, outstanding_io_max);

    return status;
}

fbe_status_t 
fbe_base_config_get_path_state_counters(fbe_base_config_t * base_config, 
                                        fbe_base_config_path_state_counters_t * path_state_counters)
{
    fbe_block_edge_t *edge = NULL;
    fbe_u32_t edge_index;
    fbe_path_state_t path_state;
    fbe_path_attr_t path_attrib;

    path_state_counters->enabled_counter = 0;
    path_state_counters->disabled_counter = 0;
    path_state_counters->broken_counter = 0;
    path_state_counters->invalid_counter = 0;
    
    fbe_base_object_trace((fbe_base_object_t*)base_config,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    for(edge_index = 0; edge_index < base_config->width; edge_index ++)
    {
        fbe_base_config_get_block_edge(base_config, &edge, edge_index);
        fbe_block_transport_get_path_state(edge, &path_state);
        fbe_block_transport_get_path_attributes(edge, &path_attrib);

        if ((base_config->base_object.class_id != FBE_CLASS_ID_PROVISION_DRIVE) &&
            (path_attrib & FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS))
        {
            /* If we have this attribute, consider the edge broken.
             */
            path_state_counters->broken_counter++;
            continue;
        }

        switch(path_state){
            case FBE_PATH_STATE_INVALID:
                path_state_counters->invalid_counter++;
                break;
            case FBE_PATH_STATE_ENABLED:
            case FBE_PATH_STATE_SLUMBER:
                path_state_counters->enabled_counter++;
                break;
            case FBE_PATH_STATE_DISABLED:
                path_state_counters->disabled_counter++;
                break;
            case FBE_PATH_STATE_BROKEN:
            case FBE_PATH_STATE_GONE:
                path_state_counters->broken_counter++;
                break;
        }
    }

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_config_set_capacity(fbe_base_config_t *base_config, fbe_lba_t capacity)
{
    fbe_status_t status;

    if(capacity != 0){
        fbe_base_object_trace((fbe_base_object_t *)base_config,
                FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                "%s capacity = 0x%llx\n", __FUNCTION__, (unsigned long long)capacity);
    }
    status = fbe_block_transport_server_set_capacity(&base_config->block_transport_server, capacity);
    return status;
}

fbe_status_t 
fbe_base_config_get_capacity(fbe_base_config_t *base_config_p, fbe_lba_t *capacity_p)
{
    fbe_status_t status;

    status = fbe_block_transport_server_get_capacity(&base_config_p->block_transport_server, capacity_p);
    return status;
}

fbe_status_t 
fbe_base_config_send_event(fbe_base_config_t *base_config_p, fbe_event_type_t event_type, fbe_event_t * event)
{
    fbe_status_t status;

    fbe_event_set_type(event, event_type);
    fbe_event_set_block_transport_server(event, &base_config_p->block_transport_server);

    if(!(base_config_p->block_transport_server.attributes & FBE_BLOCK_TRANSPORT_FLAGS_COMPLETE_EVENTS_ON_DESTROY))
    {
        status = fbe_event_service_send_event(event);
    }
    else
    {
        /* The object is in process of destroy so stop sending events. Complete the event with status busy and sender needs to handle
          * this event completion accordingly 
          */
            
        fbe_base_transport_trace(FBE_TRACE_LEVEL_INFO,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                             "%s complete event forcefully, block transport server 0x%p, event 0x%p, event type %d, attr 0x%llx\n",
                             __FUNCTION__,&base_config_p->block_transport_server, 
                             event, event_type,
                             (unsigned long long)base_config_p->block_transport_server.attributes);

        /* return busy status. */
        fbe_event_set_status(event, FBE_EVENT_STATUS_BUSY);

        /* complete the event.without being process */
        status = fbe_event_complete(event);
    }

    return status;
}

/* Accessors for the width. */
fbe_status_t 
fbe_base_config_set_width(fbe_base_config_t *base_config_p, fbe_u32_t width)
{
    base_config_p->width = width;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_get_width(fbe_base_config_t *base_config_p, fbe_u32_t *width_p)
{
    *width_p = base_config_p->width;
    return FBE_STATUS_OK;
}

/* Accessors for the generation number */
fbe_status_t fbe_base_config_set_generation_num(fbe_base_config_t * base_config_p, fbe_config_generation_t generation_number)
{
    fbe_cmi_sp_state_t  sp_state = FBE_CMI_STATE_INVALID;
    fbe_bool_t          b_is_active;
    
    /*based on CMI service, we decide if this element is passive or active*/
    fbe_cmi_get_current_sp_state(&sp_state);
    b_is_active = (sp_state == FBE_CMI_STATE_ACTIVE) ? FBE_TRUE : FBE_FALSE;
    fbe_base_object_trace((fbe_base_object_t * )base_config_p, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: active: %d generation number changed from 0x%llX to 0x%llX\n",
                          __FUNCTION__, b_is_active, base_config_p->generation_number, generation_number);
    base_config_p->generation_number = generation_number;

    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_config_get_generation_num(fbe_base_config_t * base_config_p, fbe_config_generation_t *generation_number)
{
    *generation_number = base_config_p->generation_number;
    return FBE_STATUS_OK;
}


fbe_status_t 
fbe_base_config_set_block_edge_ptr(fbe_base_config_t *base_config_p, fbe_block_edge_t *block_edge_p)
{
    base_config_p->block_edge_p = block_edge_p;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_get_block_edge_ptr(fbe_base_config_t *base_config_p, fbe_block_edge_t **block_edge_p_p)
{   
    *block_edge_p_p = base_config_p->block_edge_p;
    return FBE_STATUS_OK;
}

fbe_bool_t fbe_base_config_is_active(fbe_base_config_t * base_config)
{
    return (fbe_bool_t)(base_config->metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_ACTIVE);
}

fbe_status_t
fbe_base_config_set_configuration_committed(fbe_base_config_t * base_config_p)
{
    /* set the configuration committed state. */    
    fbe_base_config_set_flag(base_config_p, FBE_BASE_CONFIG_FLAG_CONFIGURATION_COMMITED);

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_unset_configuration_committed(fbe_base_config_t * base_config_p)
{
    /* unset the configuration commited state. */
    fbe_base_config_clear_flag(base_config_p, FBE_BASE_CONFIG_FLAG_CONFIGURATION_COMMITED);

    return FBE_STATUS_OK;
}

fbe_bool_t
fbe_base_config_is_configuration_commited(fbe_base_config_t * base_config_p)
{
    fbe_bool_t is_configuration_commited;

    is_configuration_commited = fbe_base_config_is_flag_set(base_config_p, FBE_BASE_CONFIG_FLAG_CONFIGURATION_COMMITED);

    return is_configuration_commited;
}

fbe_bool_t
fbe_base_config_is_initial_configuration(fbe_base_config_t * base_config_p)
{
    fbe_bool_t is_initial_configuration;

    is_initial_configuration = fbe_base_config_is_flag_set(base_config_p, FBE_BASE_CONFIG_FLAG_INITIAL_CONFIGURATION);

    return is_initial_configuration;
}

/*go over all edges and check which one has the highest priority, and this one counts for all*/
fbe_medic_action_priority_t fbe_base_config_get_downstream_edge_priority(fbe_base_config_t * base_config)
{
    fbe_u32_t                   i = 0;
    fbe_block_edge_t *          edge_p = NULL;
    fbe_medic_action_priority_t highest_edge_priority = FBE_MEDIC_ACTION_IDLE;
    fbe_medic_action_priority_t temp_edge_priority = FBE_MEDIC_ACTION_IDLE;
    fbe_medic_action_priority_t current_highest = FBE_MEDIC_ACTION_IDLE;
    fbe_path_state_t            path_state = FBE_PATH_STATE_INVALID;

    for(i = 0; i < base_config->width; i++)
    {
        fbe_base_config_get_block_edge(base_config, &edge_p, i);
        fbe_block_transport_get_path_state(edge_p, &path_state);
        
        if (path_state == FBE_PATH_STATE_INVALID)
        {
            continue;/*no point on talking to this edge, it's either non exisitng or already woken up*/
        }

        temp_edge_priority = fbe_block_transport_edge_get_priority(edge_p);
        highest_edge_priority = fbe_medic_get_highest_priority(temp_edge_priority, current_highest);

        /*we look for the highest priority and if we did not record it yet, we record it for next edge*/
        if (current_highest != highest_edge_priority) 
        {
            current_highest = highest_edge_priority;
        }
    }

    return current_highest;
}

fbe_status_t fbe_base_config_set_upstream_edge_priority(fbe_base_config_t * base_config, fbe_medic_action_priority_t priority)
{
    return fbe_block_transport_server_set_medic_priority(&base_config->block_transport_server, priority);
}


fbe_status_t fbe_base_config_get_downstream_geometry(fbe_base_config_t * base_config, fbe_block_edge_geometry_t * block_edge_geometry)
{
    fbe_u32_t                   i = 0;
    fbe_block_edge_t *          edge_p = NULL;
    fbe_block_edge_geometry_t   geometry;

    *block_edge_geometry = FBE_BLOCK_EDGE_GEOMETRY_INVALID; /* Default value */
    for(i = 0; i < base_config->width; i++) {
        fbe_base_config_get_block_edge(base_config, &edge_p, i);
        fbe_block_transport_edge_get_geometry(edge_p, &geometry);
        if (edge_p->base_edge.path_state == FBE_PATH_STATE_INVALID) {
            continue;
        }
        /*! @note The order of the geometry check is important. */
        if(geometry == FBE_BLOCK_EDGE_GEOMETRY_4160_4160){
            *block_edge_geometry = FBE_BLOCK_EDGE_GEOMETRY_4160_4160;
            break;
        }
        if(geometry == FBE_BLOCK_EDGE_GEOMETRY_520_520){
            *block_edge_geometry = FBE_BLOCK_EDGE_GEOMETRY_520_520;
            /* Goto the next edge */
        }
    }

    if(*block_edge_geometry == FBE_BLOCK_EDGE_GEOMETRY_INVALID){
        fbe_base_object_trace((fbe_base_object_t *) base_config, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fail\n", __FUNCTION__);

    }

    return FBE_STATUS_OK;
}

/*get eh avergag load from all downstream edges*/
fbe_traffic_priority_t fbe_base_config_get_downstream_edge_traffic_priority(fbe_base_config_t * base_config_p)
{
    fbe_block_edge_t *          edge = NULL;
    fbe_u32_t                   edge_index;
    fbe_traffic_priority_t      total_load = 0;
    fbe_traffic_priority_t      average_load = 0;
    fbe_path_state_t            path_state = FBE_PATH_STATE_INVALID;

    if(base_config_p->width == 0){
        return 0;
    }

    /*first let's find the highest one*/
    for(edge_index = 0; edge_index < base_config_p->width; edge_index ++){

        fbe_base_config_get_block_edge(base_config_p, &edge, edge_index);
        fbe_block_transport_get_path_state(edge, &path_state);

        if (path_state == FBE_PATH_STATE_INVALID) {
            continue;/*no point on talking to this edge, it's either non exisitng or already woken up*/
        }

        total_load += fbe_block_transport_edge_get_traffic_priority(edge);
    }

    average_load = (total_load / base_config_p->width);
    return average_load;

}


static fbe_bool_t 
base_config_is_operation_permission_disabled(fbe_base_config_t *base_config_p)
{
    fbe_bool_t  b_is_operation_permission_disabled;

    b_is_operation_permission_disabled = fbe_base_config_is_flag_set(base_config_p, FBE_BASE_CONFIG_FLAG_DENY_OPERATION_PERMISSION);

    return b_is_operation_permission_disabled;
}

fbe_status_t fbe_base_config_set_deny_operation_permission(fbe_base_config_t *base_config_p)
{
    fbe_base_config_set_flag(base_config_p, FBE_BASE_CONFIG_FLAG_DENY_OPERATION_PERMISSION);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_config_clear_deny_operation_permission(fbe_base_config_t *base_config_p)
{
    fbe_base_config_clear_flag(base_config_p, FBE_BASE_CONFIG_FLAG_DENY_OPERATION_PERMISSION);

    return FBE_STATUS_OK;
}


fbe_bool_t 
fbe_base_config_is_permanent_destroy(fbe_base_config_t *base_config_p)
{
    fbe_bool_t  b_is_permanent_destroy;

    b_is_permanent_destroy = fbe_base_config_is_flag_set(base_config_p, FBE_BASE_CONFIG_FLAG_PERM_DESTROY);

    return b_is_permanent_destroy;
}

fbe_status_t 
fbe_base_config_set_permanent_destroy(fbe_base_config_t *base_config_p)
{
    fbe_base_config_set_flag(base_config_p, FBE_BASE_CONFIG_FLAG_PERM_DESTROY);

    return FBE_STATUS_OK;
}


/*get a permission to do a background operation based on all the inputs we pass in*/
fbe_status_t fbe_base_config_get_operation_permission(fbe_base_config_t * base_config_p,
                                                      fbe_base_config_operation_permission_t *operation_permission_p,
                                                      fbe_bool_t count_as_io,/*is this operation an IO or not (for power savign purposes)*/
                                                      fbe_bool_t *ok_to_proceed)
{
    //fbe_traffic_priority_t   downstream_priority = FBE_TRAFFIC_PRIORITY_INVALID;
    //fbe_traffic_priority_t   highest_priority = FBE_TRAFFIC_PRIORITY_INVALID;
    fbe_status_t             status;
	fbe_status_t             memory_status;
    //fbe_bool_t               io_priority_granted = FBE_FALSE;
    //fbe_bool_t               scheduler_credits_granted = FBE_FALSE;
    fbe_cpu_id_t cpu_id;
    fbe_bool_t               io_credits_granted = FBE_TRUE;
    fbe_u32_t                available_io_credits;
	fbe_bool_t				 check_data_memory;

    
    /*is the big switch for everyone switched on ?*/
    if(base_config_control_system_bg_service_flag == 0){
        *ok_to_proceed = FBE_FALSE;
        return FBE_STATUS_OK;
    }

    /* Check if background operation permission is disabled or not*/
    if (base_config_is_operation_permission_disabled(base_config_p) == FBE_TRUE) {
        *ok_to_proceed = FBE_FALSE;
        return FBE_STATUS_OK;
    }

    /*since this is the monitor context and the block transport server has no incomming IO we have to make sure the object 
      knows there is active IO on it (unless the user don't want to count it as IO)*/
    if (base_config_is_object_power_saving_enabled(base_config_p) && count_as_io) {
        block_transport_server_set_attributes(&base_config_p->block_transport_server,
                                              FBE_BLOCK_TRANSPORT_FLAGS_IO_IN_PROGRESS);
        fbe_block_transport_server_set_last_io_time(&base_config_p->block_transport_server, fbe_get_time());
    }


    /*sanity check to make sure the user filled everything*/
    status = fbe_base_config_verify_permission_data(operation_permission_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s User passed in illegal data !\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

#if 0
    /*let's see how busy are the downstream edges*/
    downstream_priority = fbe_base_config_get_downstream_edge_traffic_priority(base_config_p);

    /*is our priority bigger or at least as important?*/
    highest_priority = fbe_block_transport_edge_get_highest_traffic_priority(downstream_priority,
            operation_permission_p->operation_priority);
    /*do we have enough priority to do the operation, if so we are good to go*/
    if (operation_permission_p->operation_priority == highest_priority) {
        io_priority_granted = FBE_TRUE;
    } else {
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                               FBE_TRACE_LEVEL_DEBUG_LOW,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s Our priority %d; Downstream priority %d \n", __FUNCTION__,
                               operation_permission_p->operation_priority,
                               downstream_priority);
    }

    /*now let's see if we have enough scheduler credits for out operation*/
    /*FIXME, we need to get the correct core number here, either from the monitor of from the OS*/
    status = fbe_scheduler_request_credits(&operation_permission_p->credit_requests, cpu_id, &scheduler_credits_granted);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Failed to get credits from scheduler\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(!scheduler_credits_granted){
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                FBE_TRACE_LEVEL_DEBUG_HIGH,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s Credit not granted \n", __FUNCTION__);
    }
#endif
    if (operation_permission_p->operation_priority == FBE_TRAFFIC_PRIORITY_HIGH)
    {
        io_credits_granted = FBE_TRUE;
        memory_status = FBE_STATUS_OK;
    } 
    else
    { 
        /* If the bts has available io credits then the object is not congested due
         * to host I/O and can proceed as normal. 
         */
        fbe_block_transport_server_get_available_io_credits(&base_config_p->block_transport_server, &available_io_credits);
        if (operation_permission_p->io_credits > available_io_credits || base_config_deny_credits){
            io_credits_granted = FBE_FALSE;
            fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                                   FBE_TRACE_LEVEL_INFO,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: io credits not granted asking: 0x%x avail: 0x%x deny_creds: %d\n", __FUNCTION__, operation_permission_p->io_credits, available_io_credits, base_config_deny_credits);
    	} else {
    		io_credits_granted = FBE_TRUE;
    	}

    	/* Check if we low on memory */
    	if(operation_permission_p->credit_requests.mega_bytes_consumption > 0){ /* Raid group */
    		check_data_memory = FBE_TRUE;
    	} else {
    		check_data_memory = FBE_FALSE;
    	}

    	cpu_id = fbe_get_cpu_id();

    	memory_status = fbe_memory_check_state(cpu_id, check_data_memory);
    }


    /*let's see if we got all the permissions and grants we want*/
    if (io_credits_granted && (memory_status == FBE_STATUS_OK)) {
        base_config_p->background_operation_deny_count = 0;/*reset it for next time*/
        *ok_to_proceed = FBE_TRUE;
        return FBE_STATUS_OK;
    }

    /*If we got here, we did not get permission.
      let's see if we are starving here after multiple times we did not get the permission*/
    if (base_config_p->background_operation_deny_count >= max_bg_deny_count) {
        base_config_p->background_operation_deny_count = 0;
        *ok_to_proceed = FBE_TRUE;/*let's give the object a chance to get in some IO*/
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                FBE_TRACE_LEVEL_DEBUG_HIGH,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: activating starvation prevention\n", __FUNCTION__);

        return FBE_STATUS_OK;
    }

    /*since we end up not doing anything let's return the credits we were granted*/
    //fbe_scheduler_return_credits(&operation_permission_p->credit_requests, cpu_id);
    base_config_p->background_operation_deny_count++;
    *ok_to_proceed = FBE_FALSE;
    return FBE_STATUS_OK;   
}

static fbe_status_t fbe_base_config_verify_permission_data(fbe_base_config_operation_permission_t *operation_permission_p)
{

    if (operation_permission_p == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((operation_permission_p->operation_priority <= FBE_TRAFFIC_PRIORITY_INVALID) || 
            (operation_permission_p->operation_priority >= FBE_TRAFFIC_PRIORITY_LAST)) {

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((operation_permission_p->credit_requests.cpu_operations_per_second <= 0) || 
            (operation_permission_p->credit_requests.mega_bytes_consumption < 0)) {

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

void fbe_base_config_set_max_deny_count(fbe_u32_t max_count)
{
    max_bg_deny_count = max_count;

}

void fbe_base_config_class_trace(fbe_trace_level_t trace_level,
                         fbe_u32_t message_id,
                         const fbe_char_t * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    va_list argList;

    default_trace_level = fbe_trace_get_default_trace_level();
    if (trace_level > default_trace_level) {
        return;
    }

    va_start(argList, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_CLASS,
                     FBE_CLASS_ID_BASE_CONFIG,
                     trace_level,
                     message_id,
                     fmt, 
                     argList);
    va_end(argList);
}

fbe_status_t fbe_base_config_usurper_set_system_encryption_info(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_system_encryption_info_t * 		    encryption_info = NULL;    /* INPUT */
    fbe_status_t 					        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t * 					    payload = NULL;
    fbe_payload_control_operation_t * 	    control_operation = NULL;
    fbe_payload_control_buffer_length_t     len = 0;
    
    fbe_base_config_class_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s entry\n", __FUNCTION__);

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &encryption_info);
    if (encryption_info == NULL){
        fbe_base_config_class_trace( FBE_TRACE_LEVEL_WARNING,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s fbe_payload_control_get_buffer failed\n",
                                     __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);    
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_system_encryption_info_t)){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Invalid length:%d\n",
                                    __FUNCTION__, len);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    status = fbe_base_config_set_system_encryption_info(encryption_info);
    if(status != FBE_STATUS_OK){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s failed to set encryption info\n",
                                    __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}

fbe_status_t fbe_base_config_usurper_get_system_encryption_info(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_system_encryption_info_t * 		    encryption_info = NULL;    /* INPUT */
    fbe_status_t 						    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t * 					    payload = NULL;
    fbe_payload_control_operation_t * 	    control_operation = NULL;
    fbe_payload_control_buffer_length_t 	len = 0;
    
    fbe_base_config_class_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s entry\n", __FUNCTION__);

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &encryption_info);
    if (encryption_info == NULL){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s fbe_payload_control_get_buffer failed\n",
                                    __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);    
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_system_encryption_info_t)){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Invalid length:%d\n",
                                    __FUNCTION__, len);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    status = fbe_base_config_get_system_encryption_info(encryption_info);
    if(status != FBE_STATUS_OK){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s failed to get encryption info\n",
                                    __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}

fbe_status_t fbe_base_config_set_system_encryption_info(fbe_system_encryption_info_t *set_info)
{
    if (set_info == NULL) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s NULL pointer\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*make sure we trace important information which was changed*/
    if (fbe_system_encryption_info.encryption_mode != set_info->encryption_mode) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_INFO,
                "== System wide encryption mode 0x%x ==\n",set_info->encryption_mode);
    }

    fbe_spinlock_lock(&base_config_class_lock);
    fbe_copy_memory(&fbe_system_encryption_info, set_info, sizeof(fbe_system_encryption_info_t));
    fbe_spinlock_unlock(&base_config_class_lock);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_config_get_system_encryption_info(fbe_system_encryption_info_t *get_info)
{
    if (get_info == NULL) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s NULL pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // do we really need this lock?
    fbe_spinlock_lock(&base_config_class_lock);
    fbe_copy_memory(get_info, &fbe_system_encryption_info, sizeof(fbe_system_encryption_info_t));
    fbe_spinlock_unlock(&base_config_class_lock);

    return FBE_STATUS_OK;

}


fbe_status_t fbe_base_config_set_system_power_saving_info(fbe_system_power_saving_info_t *set_info)
{
    if (set_info == NULL) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s NULL pointer\n", __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*make sure we trace important information which was changed*/
    if (fbe_system_power_saving_info.enabled != set_info->enabled) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_INFO,
                "== System wide power save %s ==\n",((set_info->enabled) ? "Enabled" : "Disabled"));
    }

    if (fbe_system_power_saving_info.hibernation_wake_up_time_in_minutes != set_info->hibernation_wake_up_time_in_minutes) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_INFO,
                "== System wide power save periodic wakeup %d minutes ==\n",(int)set_info->hibernation_wake_up_time_in_minutes);
    }

    if (fbe_system_power_saving_info.stats_enabled != set_info->stats_enabled) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO,
                FBE_TRACE_MESSAGE_ID_INFO,
                "== System wide power save statistics %s ==\n",((set_info->stats_enabled) ? "Enabled" : "Disabled"));
    }

    fbe_spinlock_lock(&base_config_class_lock);
    fbe_copy_memory(&fbe_system_power_saving_info, set_info, sizeof(fbe_system_power_saving_info_t));
    fbe_spinlock_unlock(&base_config_class_lock);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_config_get_system_power_saving_info(fbe_system_power_saving_info_t *get_info)
{
    if (get_info == NULL) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s NULL pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock(&base_config_class_lock);
    fbe_copy_memory(get_info, &fbe_system_power_saving_info, sizeof(fbe_system_power_saving_info_t));
    fbe_spinlock_unlock(&base_config_class_lock);

    return FBE_STATUS_OK;

}

/*!***************************************************************
 * fbe_base_config_set_power_saving_idle_time()
 ****************************************************************
 * @brief
 *  This function is called to set the power saving idle time.
 *
 * @param 
 *     base_config_p - base config structure.
 *     idle_time_in_seconds - idle time to set 
 *
 * @return fbe_status_t
 *  STATUS of the operation
 *
 * @author
 *
 ****************************************************************/
fbe_status_t 
fbe_base_config_set_power_saving_idle_time(fbe_base_config_t * base_config_p, fbe_u64_t idle_time_in_seconds)
{
    /* set idle time */
    base_config_p->power_saving_idle_time_in_seconds = idle_time_in_seconds;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Set Power Saving Idle Time (secs): %d\n", (int)idle_time_in_seconds);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_config_get_power_saving_idle_time(fbe_base_config_t * base_config_p, fbe_u64_t *idle_time_in_seconds)
{
    *idle_time_in_seconds = base_config_p->power_saving_idle_time_in_seconds;
    return FBE_STATUS_OK;

}

    fbe_status_t
fbe_base_config_get_nonpaged_metadata_ptr(fbe_base_config_t * base_config, void ** nonpaged_metadata_ptr)
{
    * nonpaged_metadata_ptr = base_config->metadata_element.nonpaged_record.data_ptr;
    return FBE_STATUS_OK;
}

    fbe_status_t 
fbe_base_config_get_nonpaged_metadata_size(fbe_base_config_t * base_config, fbe_u32_t * nonpaged_metadata_size)
{
    * nonpaged_metadata_size = base_config->metadata_element.nonpaged_record.data_size;
    return FBE_STATUS_OK;
}

    fbe_status_t
fbe_base_config_set_nonpaged_metadata_ptr(fbe_base_config_t * base_config, void * nonpaged_metadata_ptr)
{
    base_config->metadata_element.nonpaged_record.data_ptr = nonpaged_metadata_ptr;
    return FBE_STATUS_OK;
}

fbe_bool_t 
fbe_base_config_stripe_lock_is_started(fbe_base_config_t * base_config)
{ 
    return fbe_metadata_stripe_lock_is_started(&base_config->metadata_element);
}

/* Accessors for flags. */
fbe_bool_t 
fbe_base_config_is_flag_set(fbe_base_config_t * base_config, fbe_base_config_flags_t flags)
{
    fbe_bool_t is_flag_set = FBE_FALSE;
    if(((base_config->flags & flags) == flags)) {
        is_flag_set = FBE_TRUE;
    }
    return is_flag_set;
}

 void 
fbe_base_config_init_flags(fbe_base_config_t * base_config)
{
    base_config->flags = 0;
}

void 
fbe_base_config_set_flag(fbe_base_config_t * base_config, fbe_base_config_flags_t flags)
{
    base_config->flags |= flags;
}

void 
fbe_base_config_clear_flag(fbe_base_config_t * base_config, fbe_base_config_flags_t flags)
{
    base_config->flags &= ~flags;
}

fbe_status_t 
fbe_base_config_get_flags(fbe_base_config_t * base_config, fbe_base_config_flags_t * flags)
{
    * flags = base_config->flags;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_config_set_encryption_state_with_notification(fbe_base_config_t * base_config_p, 
                                                       fbe_base_config_encryption_state_t encryption_state)
{
	if (base_config_p->encryption_state == encryption_state) { /* Nothing to be changed */
		return FBE_STATUS_OK;
	}
    base_config_p->encryption_state = encryption_state;
    fbe_base_config_encryption_state_notification(base_config_p);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_config_encryption_state_notification(fbe_base_config_t * base_config_p)
{
	fbe_notification_info_t notification_info;
    fbe_object_id_t object_id;
    fbe_class_id_t class_id;
    fbe_handle_t object_handle;
	fbe_status_t status;

    if(!fbe_base_config_is_object_encryption_state_locked(base_config_p->encryption_state)){/* It is transitional state - no notification */
		return FBE_STATUS_OK;
	}

	object_handle = fbe_base_pointer_to_handle((fbe_base_t *)base_config_p);
	class_id = fbe_base_object_get_class_id(object_handle);
	fbe_base_object_get_object_id((fbe_base_object_t *)base_config_p, &object_id);

    fbe_zero_memory(&notification_info, sizeof(fbe_notification_info_t));
	notification_info.notification_type = FBE_NOTIFICATION_TYPE_ENCRYPTION_STATE_CHANGED;    
	notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
	notification_info.class_id = class_id;

	notification_info.notification_data.encryption_info.encryption_state = base_config_p->encryption_state;
	notification_info.notification_data.encryption_info.object_id = object_id;
	fbe_base_config_get_generation_num((fbe_base_config_t *)base_config_p, &notification_info.notification_data.encryption_info.generation_number);
	if (class_id > FBE_CLASS_ID_RAID_FIRST && class_id < FBE_CLASS_ID_RAID_LAST) {
		fbe_database_lookup_raid_by_object_id(object_id, &notification_info.notification_data.encryption_info.control_number);
		if (notification_info.notification_data.encryption_info.control_number == FBE_RAID_ID_INVALID) { /* Mirrors under striper */
			fbe_base_transport_server_t * base_transport_server_p = (fbe_base_transport_server_t *)&(base_config_p->block_transport_server);
			fbe_base_edge_t * base_edge_p = base_transport_server_p->client_list;
			fbe_object_id_t striper_id;

			if (base_edge_p) {
				striper_id = base_edge_p->client_id;
			} else {
				fbe_database_config_table_get_striper(object_id, &striper_id);
			}
			fbe_database_lookup_raid_by_object_id(striper_id, &notification_info.notification_data.encryption_info.control_number);
		}
	} /* if (class_id > FBE_CLASS_ID_RAID_FIRST && class_id < FBE_CLASS_ID_RAID_LAST) */
	fbe_base_config_get_width((fbe_base_config_t*)base_config_p, &notification_info.notification_data.encryption_info.width);

	status = fbe_notification_send(object_id, notification_info);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)base_config_p, 
						  FBE_TRACE_LEVEL_ERROR, 
						  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
						  "%s Notification send Failed \n", __FUNCTION__);

	}


    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_config_set_encryption_state(fbe_base_config_t * base_config_p, fbe_base_config_encryption_state_t encryption_state)
{
    base_config_p->encryption_state = encryption_state;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_config_get_encryption_state(fbe_base_config_t * base_config, fbe_base_config_encryption_state_t * encryption_state)
{
    * encryption_state = base_config->encryption_state;
    return FBE_STATUS_OK;
}

fbe_bool_t fbe_base_config_is_encryption_state_push_keys_downstream(fbe_base_config_t * base_config)
{
    fbe_base_config_encryption_state_t encryption_state;

    fbe_base_config_get_encryption_state(base_config, &encryption_state);

    if ((encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_PUSH_KEYS_DOWNSTREAM) ||
        (encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_KEYS))
    {
        return FBE_TRUE;
    }
    else
    {
        return FBE_FALSE;
    }
}

fbe_bool_t fbe_base_config_is_object_encryption_state_locked(fbe_base_config_encryption_state_t encryption_state)
{
	fbe_bool_t is_locked;

	switch(encryption_state){
	case FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_NEW_KEYS_REQUIRED:
	case FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_CURRENT_KEYS_REQUIRED:
	case FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE:
	case FBE_BASE_CONFIG_ENCRYPTION_STATE_ALL_KEYS_NEED_REMOVED:
	case FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_REMOVE_OLD_KEY:
		is_locked = FBE_TRUE;
		break;
	default:
		is_locked = FBE_FALSE;
		break;
	}

	return is_locked;
}


fbe_status_t 
fbe_base_config_set_encryption_mode(fbe_base_config_t * base_config, fbe_base_config_encryption_mode_t encryption_mode)
{
    base_config->encryption_mode = encryption_mode;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_config_get_encryption_mode(fbe_base_config_t * base_config, fbe_base_config_encryption_mode_t * encryption_mode)
{
    * encryption_mode = base_config->encryption_mode;
    return FBE_STATUS_OK;
}

fbe_bool_t fbe_base_config_is_encrypted_mode(fbe_base_config_t * base_config)
{
    fbe_base_config_encryption_mode_t encryption_mode;
	fbe_system_encryption_mode_t system_encryption_mode;

	fbe_database_get_system_encryption_mode(&system_encryption_mode);
	if(system_encryption_mode != FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED){ /* The system is NOT in encrypted mode */
		return FBE_FALSE;
	}

    fbe_base_config_get_encryption_mode(base_config, &encryption_mode);

    if(encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED)
    {
        return FBE_TRUE;
    }
    else
    {
        return FBE_FALSE;
    }
}

fbe_bool_t fbe_base_config_is_encrypted_state(fbe_base_config_t * base_config)
{
    fbe_base_config_encryption_state_t encryption_state;
	fbe_system_encryption_mode_t system_encryption_mode;

	fbe_database_get_system_encryption_mode(&system_encryption_mode);
	if(system_encryption_mode != FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED){ /* The system is NOT in encrypted mode */
		return FBE_FALSE;
	}

    fbe_base_config_get_encryption_state(base_config, &encryption_state);

    if(encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_ENCRYPTED)
    {
        return FBE_TRUE;
    }
    else
    {
        return FBE_FALSE;
    }
}

fbe_bool_t fbe_base_config_is_rekey_mode(fbe_base_config_t * base_config)
{
    fbe_base_config_encryption_mode_t encryption_mode;

    fbe_base_config_get_encryption_mode(base_config, &encryption_mode);

    if ((encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) ||
        (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS))
    {
        return FBE_TRUE;
    }
    else
    {
        return FBE_FALSE;
    }
}

fbe_status_t 
fbe_base_config_set_key_handle(fbe_base_config_t * base_config, fbe_encryption_setup_encryption_keys_t *key_handle)
{
    base_config->key_handle = key_handle;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_config_get_key_handle(fbe_base_config_t * base_config, fbe_encryption_setup_encryption_keys_t ** key_handle)
{
    *key_handle = base_config->key_handle;
    return FBE_STATUS_OK;
}

/* Accessors for clustered flags. */
fbe_bool_t 
fbe_base_config_is_clustered_flag_set(fbe_base_config_t * base_config, fbe_base_config_clustered_flags_t flags)
{
    fbe_bool_t is_flag_set = FBE_FALSE;
    fbe_base_config_metadata_memory_t * base_config_metadata_memory = NULL;

    if(base_config->metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_FALSE;
    }

    base_config_metadata_memory = (fbe_base_config_metadata_memory_t *)base_config->metadata_element.metadata_memory.memory_ptr;

    if (base_config_metadata_memory == NULL) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s NULL pointer\n", __FUNCTION__);
        return FBE_FALSE;
    }

    if(((base_config_metadata_memory->flags & flags) == flags)) {
        is_flag_set = FBE_TRUE;
    }
    return is_flag_set;
}


/*!****************************************************************************
 * fbe_base_config_is_any_clustered_flag_set()
 ******************************************************************************
 * @brief
 *  This function checks to see if any of the input flags
 *  are set locally.
 * 
 * @param raid_group_p - a pointer to the base config
 * @param flags - flag to check in the clustered flags.
 *
 * @return fbe_status_t
 *
 * @author
 *  7/25/2011 - Created. Rob Foley
 ******************************************************************************/
fbe_bool_t 
fbe_base_config_is_any_clustered_flag_set(fbe_base_config_t * base_config, fbe_base_config_clustered_flags_t flags)
{
    fbe_bool_t is_flag_set = FBE_FALSE;
    fbe_base_config_metadata_memory_t * base_config_metadata_memory = NULL;

    if(base_config->metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_FALSE;
    }

    base_config_metadata_memory = (fbe_base_config_metadata_memory_t *)base_config->metadata_element.metadata_memory.memory_ptr;

    if (base_config_metadata_memory == NULL) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s NULL pointer\n", __FUNCTION__);
        return FBE_FALSE;
    }

    if(((base_config_metadata_memory->flags & flags) != 0)) {
        is_flag_set = FBE_TRUE;
    }
    return is_flag_set;
}
/**************************************
 * end fbe_base_config_is_any_clustered_flag_set()
 **************************************/

fbe_bool_t 
fbe_base_config_is_peer_clustered_flag_set(fbe_base_config_t * base_config, fbe_base_config_clustered_flags_t flags)
{
    fbe_bool_t is_flag_set = FBE_FALSE;
    fbe_base_config_metadata_memory_t * base_config_metadata_memory = NULL;

    if(base_config->metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_FALSE;
    }

    base_config_metadata_memory = (fbe_base_config_metadata_memory_t *)base_config->metadata_element.metadata_memory_peer.memory_ptr;

    if (base_config_metadata_memory == NULL) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s NULL pointer\n", __FUNCTION__);
        return FBE_FALSE;
    }

    if(((base_config_metadata_memory->flags & flags) == flags)) {
        is_flag_set = FBE_TRUE;
    }
    return is_flag_set;
}


fbe_bool_t 
fbe_base_config_is_any_peer_clustered_flag_set(fbe_base_config_t * base_config, fbe_base_config_clustered_flags_t flags)
{
    fbe_bool_t is_flag_set = FBE_FALSE;
    fbe_base_config_metadata_memory_t * base_config_metadata_memory = NULL;

    if(base_config->metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_FALSE;
    }

    base_config_metadata_memory = (fbe_base_config_metadata_memory_t *)base_config->metadata_element.metadata_memory_peer.memory_ptr;

    if (base_config_metadata_memory == NULL) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s NULL pointer\n", __FUNCTION__);
        return FBE_FALSE;
    }

    if(((base_config_metadata_memory->flags & flags) != 0)) {
        is_flag_set = FBE_TRUE;
    }
    return is_flag_set;
}

fbe_status_t 
fbe_base_config_init_clustered_flags(fbe_base_config_t * base_config)
{
    fbe_base_config_metadata_memory_t * base_config_metadata_memory = NULL;

    if(base_config->metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    base_config_metadata_memory = (fbe_base_config_metadata_memory_t *)base_config->metadata_element.metadata_memory.memory_ptr;

    if (base_config_metadata_memory == NULL) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s NULL pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    base_config_metadata_memory->flags = 0;
    fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, (fbe_base_object_t*)base_config, FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_config_set_clustered_flag(fbe_base_config_t * base_config, fbe_base_config_clustered_flags_t flags)
{
    fbe_base_config_metadata_memory_t * base_config_metadata_memory = NULL;

    if(base_config->metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    base_config_metadata_memory = (fbe_base_config_metadata_memory_t *)base_config->metadata_element.metadata_memory.memory_ptr;

    if (base_config_metadata_memory == NULL) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s NULL pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (!((base_config_metadata_memory->flags & flags) == flags))
    {
        base_config_metadata_memory->flags |= flags;
        fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, (fbe_base_object_t*)base_config, FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);
    }

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_config_clear_clustered_flag(fbe_base_config_t * base_config, fbe_base_config_clustered_flags_t flags)
{
    fbe_base_config_metadata_memory_t * base_config_metadata_memory = NULL;

    if(base_config->metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    base_config_metadata_memory = (fbe_base_config_metadata_memory_t *)base_config->metadata_element.metadata_memory.memory_ptr;

    if (base_config_metadata_memory == NULL) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s NULL pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (flags & FBE_BASE_CONFIG_CLUSTERED_FLAG_JOINED)
    {
        fbe_base_config_metadata_memory_t * peer_metadata_memory = NULL;
        peer_metadata_memory = (fbe_base_config_metadata_memory_t *)base_config->metadata_element.metadata_memory_peer.memory_ptr;

        fbe_base_object_trace((fbe_base_object_t*)base_config, FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                "clearing joined 0x%x lifecycle: 0x%x local: 0x%x peer: 0x%x\n", 
                (unsigned int)flags,
                base_config->base_object.lifecycle.state,
                (unsigned int)base_config_metadata_memory->flags, (unsigned int)peer_metadata_memory->flags);
    }

    if ((base_config_metadata_memory->flags & flags) != 0)
    {
        base_config_metadata_memory->flags &= ~flags;
        fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, (fbe_base_object_t*)base_config, FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);
    }

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_config_get_clustered_flags(fbe_base_config_t * base_config, fbe_base_config_clustered_flags_t * flags)
{
    fbe_base_config_metadata_memory_t * base_config_metadata_memory = NULL;

    if(base_config->metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    base_config_metadata_memory = (fbe_base_config_metadata_memory_t *)base_config->metadata_element.metadata_memory.memory_ptr;

    if (base_config_metadata_memory == NULL) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s NULL pointer\n", __FUNCTION__);
        * flags = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    * flags = base_config_metadata_memory->flags;

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_config_get_peer_clustered_flags(fbe_base_config_t * base_config, fbe_base_config_clustered_flags_t * flags)
{
    fbe_base_config_metadata_memory_t * base_config_metadata_memory = NULL;

    if(base_config->metadata_element.metadata_element_state == FBE_METADATA_ELEMENT_STATE_INVALID){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s FBE_METADATA_ELEMENT_STATE_INVALID \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    base_config_metadata_memory = (fbe_base_config_metadata_memory_t *)base_config->metadata_element.metadata_memory_peer.memory_ptr;

    if (base_config_metadata_memory == NULL) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s NULL pointer\n", __FUNCTION__);
        * flags = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    * flags = base_config_metadata_memory->flags;

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_event_queue_push_event(fbe_base_config_t * base_config_p, fbe_event_t * event_p)
{
    /*!@note we are deliberately using base config lock here. */
    fbe_base_config_lock(base_config_p);
    fbe_queue_push(&base_config_p->event_queue_head, &event_p->queue_element);
    fbe_base_config_unlock(base_config_p);
    return FBE_STATUS_OK;
}


/*!**************************************************************************************
 * @fn fbe_base_event_queue_coalesce_and_push_event(fbe_base_config_t *base_config,
 *                                                  fbe_event_t       *event_p,
 *                                                  fbe_u64_t         chunk_range,
 *                                                  fbe_bool_t        *queued,
 *                                                  fbe_lba_t         coalesce_boundary,
 ****************************************************************************************
 * @brief
 *  Will coalesce a new event into any matching candidates found on the event q. and add 
 *  them to the q. If the new event is fully overlapped by an existing event, then there is
 *  no need to add a new one.
 *  Will not coalesce requests across teh coalesce_boundary.
 *
 * @param object_handle - The base_config handle.
 * @param event_p - The new event to add to the event q.
 * @param chunk_range - Size of gap allowed between requests to coalesce
 * @param queued - returns TRUE if the request was queued.
 * @param coalesce_boundary - lba of region to not coalesce across.
 * @return
 *  fbe_status_t - FBE_STATUS_OK: If event was succesfully added to the event q
 ****************************************************************/
fbe_status_t
fbe_base_config_event_queue_coalesce_and_push_event(fbe_base_config_t *base_config_p, 
                                                    fbe_event_t       *event_p, 
                                                    fbe_u64_t         chunk_range,
                                                    fbe_bool_t        *queued,
                                                    fbe_lba_t         coalesce_boundary)
{
    fbe_queue_element_t*         queue_element_p = NULL;
    fbe_event_t*                 queued_event_p = NULL;
    fbe_event_data_request_t     data_request;
    fbe_data_event_type_t        event_type;
    fbe_event_stack_t*           queued_event_stack_p = NULL;
    fbe_event_stack_t*           new_event_stack_p = NULL;
    fbe_lba_t                    new_event_start_lba;
    fbe_lba_t                    new_event_end_lba;           /* Actually End lba + 1 */
    fbe_block_count_t            new_event_block_count;
    fbe_lba_t                    current_event_lba;
    fbe_lba_t                    current_event_end_lba;       /* Actually End lba + 1 */
    fbe_block_count_t            current_event_block_count;
    fbe_block_count_t            block_count;
    
    /* Get the information for the new event to coalesce.
     */
    new_event_stack_p = fbe_event_get_current_stack(event_p);
    fbe_event_stack_get_extent(new_event_stack_p, &new_event_start_lba, &new_event_block_count);
    new_event_end_lba = new_event_start_lba + new_event_block_count;


    /* Search the event queue for coalesce candidates.
     */
    fbe_base_config_lock(base_config_p);
    queue_element_p = fbe_queue_front(&base_config_p->event_queue_head);
    while(queue_element_p != NULL) 
    {
        /* Get the information for the current queue element.
         */
        queued_event_p = fbe_event_queue_element_to_event(queue_element_p);
        fbe_event_get_data_request_data(queued_event_p, &data_request);
        event_type = data_request.data_event_type;

        /* If the event is in use, or is not a mark verify, go to the next event 
         */
        if((queued_event_p->event_in_use) || 
           ((event_type != FBE_DATA_EVENT_TYPE_MARK_VERIFY) &&
            (event_type != FBE_DATA_EVENT_TYPE_MARK_IW_VERIFY)) )
        {
            queue_element_p = fbe_queue_next(&base_config_p->event_queue_head, queue_element_p);
            continue;
        }

        /* This is a mark verify, get the information for the current event candidate.
         */
        queued_event_stack_p = fbe_event_get_current_stack( queued_event_p );
        fbe_event_stack_get_extent(queued_event_stack_p, &current_event_lba, &current_event_block_count);
        current_event_end_lba = current_event_lba + current_event_block_count;

        /* If the completion function and context are not the same, don't coalesce!
         */
        if( (queued_event_stack_p->completion_function != new_event_stack_p->completion_function) ||
            (queued_event_stack_p->completion_context != new_event_stack_p->completion_context)) 
        {
            queue_element_p = fbe_queue_next(&base_config_p->event_queue_head, queue_element_p);
            continue;
        }

        /* If one request is the protected area and the other is not, do not coalesce.
         */
        else if( (coalesce_boundary != FBE_LBA_INVALID) &&
                 (((new_event_start_lba >= coalesce_boundary) && (current_event_lba < coalesce_boundary)) ||
                  ((current_event_lba >= coalesce_boundary) && (new_event_start_lba < coalesce_boundary))) )
        {
            queue_element_p = fbe_queue_next(&base_config_p->event_queue_head, queue_element_p);
            continue;
        }

        /* If the request exactly matches with the current event or if the current event subsumes the 
           new event, no need to queue it 
           |----------|   (or)        |------------------|
             |----|                         |------------|
         */
        else if( (current_event_lba <= new_event_start_lba ) && (current_event_end_lba >= new_event_end_lba ) ) 
        {
            *queued = FBE_FALSE;
            fbe_base_config_unlock(base_config_p);
            return FBE_STATUS_OK;
        }
        /* If the new request subsumes the current event, replace with the new one
             |----|       (or)              |------------|
           |----------|               |------------------|
         */
        else if( ( new_event_start_lba <= current_event_lba) && (new_event_end_lba >= current_event_end_lba  ) ) 
        {
            fbe_event_stack_set_extent(queued_event_stack_p, new_event_start_lba, new_event_block_count);

            *queued = FBE_FALSE;
            fbe_base_config_unlock(base_config_p);
            return FBE_STATUS_OK;
        }

        

        /*  if the requests are not overlapping at all and they are in this order
            (current_event)| ------ |       (new_Event) |----------| 
         */
        else if( (current_event_lba < new_event_start_lba) && (current_event_end_lba < new_event_start_lba) )
        {
            /* If they are close enough together, then coalesce the two requests.
             */
            if( (new_event_start_lba - current_event_end_lba) <= chunk_range ) 
            {
                block_count = current_event_block_count + (new_event_start_lba - current_event_end_lba) + new_event_block_count;
                fbe_event_stack_set_extent(queued_event_stack_p, current_event_lba, block_count);
               
                *queued = FBE_FALSE;
                fbe_base_config_unlock(base_config_p);
                return FBE_STATUS_OK;
            }
        }

        /*  if the requests are not overlapping at all and they are in this order
            (new_event)| ------ |       (current_Event) |----------| 
         */
        else if((new_event_start_lba < current_event_lba) && (new_event_end_lba < current_event_lba))
        {
            /* If they are close enough together, then coalesce the two requests.
             */
            if( (current_event_lba - new_event_end_lba) <= chunk_range ) 
            {
                block_count = new_event_block_count + (current_event_lba - new_event_end_lba) + current_event_block_count;
                fbe_event_stack_set_extent(queued_event_stack_p, new_event_start_lba, block_count);

                *queued = FBE_FALSE;
                fbe_base_config_unlock(base_config_p);
                return FBE_STATUS_OK;
            }
        }

       /*  if the requests are overlapping and if it looked like 
           (current_event)| ------ | 
                  (new_Event) |----------|    
       */ 
       else if( (current_event_lba < new_event_start_lba) && (current_event_end_lba > new_event_start_lba))
       {
           block_count = current_event_block_count + (new_event_end_lba - current_event_end_lba);
           fbe_event_stack_set_extent(queued_event_stack_p, current_event_lba, block_count);
           
           *queued = FBE_FALSE;
           fbe_base_config_unlock(base_config_p);
           return FBE_STATUS_OK;
           
       }

       /*  if the requests are overlapping and if it looked like 
              (current_Event) |----------|        (OR) (current_Event) |----------|
               (new_event)|--------|                    (new_event)|--------------|       
       */
       else if( (new_event_start_lba < current_event_lba) && (new_event_end_lba > current_event_lba))
       {
           block_count = current_event_block_count + (current_event_lba - new_event_start_lba);
           current_event_lba = new_event_start_lba;
           fbe_event_stack_set_extent(queued_event_stack_p, current_event_lba, block_count);
           
           *queued = FBE_FALSE;
           fbe_base_config_unlock(base_config_p);
           return FBE_STATUS_OK;
       }

       /* Move on to the next element in the queue */
       queue_element_p = fbe_queue_next(&base_config_p->event_queue_head, queue_element_p);

    }

    /* If all of the above are not met, then push the event into the queue */
    fbe_queue_push(&base_config_p->event_queue_head, &event_p->queue_element);
    *queued = FBE_TRUE;
    fbe_base_config_unlock(base_config_p);
    return FBE_STATUS_OK;
}


fbe_status_t
fbe_base_config_event_queue_pop_event(fbe_base_config_t * base_config_p, fbe_event_t ** event_p)
{
    fbe_queue_element_t * queue_element_p = NULL; 

    /*!@note we are deliberately using base config lock here. */
    fbe_base_config_lock(base_config_p);
    queue_element_p = fbe_queue_pop(&base_config_p->event_queue_head);
    fbe_base_config_unlock(base_config_p);
    if(queue_element_p != NULL)
    {
        *event_p = fbe_event_queue_element_to_event(queue_element_p);
    }
    return FBE_STATUS_OK;
}

fbe_bool_t
fbe_base_config_event_queue_is_empty(fbe_base_config_t * base_config_p)
{
    fbe_bool_t is_empty;

    /*!@note we are deliberately using base config lock here. */
    fbe_base_config_lock(base_config_p);
    is_empty = fbe_queue_is_empty(&base_config_p->event_queue_head);
    fbe_base_config_unlock(base_config_p);
    return is_empty;
}

fbe_bool_t
fbe_base_config_event_queue_is_empty_no_lock(fbe_base_config_t * base_config_p)
{
    fbe_bool_t is_empty;

    is_empty = fbe_queue_is_empty(&base_config_p->event_queue_head);
    return is_empty;
}

fbe_status_t
fbe_base_config_event_queue_front_event_in_use(fbe_base_config_t * base_config_p, fbe_event_t ** event_p)
{
    fbe_queue_element_t * queue_element_p = NULL; 
    

    /* initialize the event pointer as null. */
    *event_p = NULL;

    /* get the front node of the event queue. */
    fbe_base_config_lock(base_config_p);
    queue_element_p = fbe_queue_front(&base_config_p->event_queue_head);
    if(queue_element_p != NULL)
    {
        *event_p = fbe_event_queue_element_to_event(queue_element_p);
        (*event_p)->event_in_use = FBE_TRUE;
    }
    
    fbe_base_config_unlock(base_config_p);
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_event_queue_front_event(fbe_base_config_t * base_config_p, fbe_event_t ** event_p)
{
    fbe_queue_element_t * queue_element_p = NULL; 

    /* initialize the event pointer as null. */
    *event_p = NULL;

    /* get the front node of the event queue. */
    fbe_base_config_lock(base_config_p);
    queue_element_p = fbe_queue_front(&base_config_p->event_queue_head);
    fbe_base_config_unlock(base_config_p);
    if(queue_element_p != NULL)
    {
        *event_p = fbe_event_queue_element_to_event(queue_element_p);
    }
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_nonpaged_metadata_set_object_id(fbe_base_config_nonpaged_metadata_t * base_config_nonpaged_metadata_p, fbe_object_id_t object_id)
{
    base_config_nonpaged_metadata_p->object_id = object_id;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_nonpaged_metadata_set_generation_number(fbe_base_config_nonpaged_metadata_t * base_config_nonpaged_metadata_p, fbe_generation_code_t generation_number)
{
    base_config_nonpaged_metadata_p->generation_number = generation_number;
    return FBE_STATUS_OK;
}
fbe_status_t
fbe_base_config_nonpaged_metadata_set_np_state(fbe_base_config_nonpaged_metadata_t * base_config_nonpaged_metadata_p, fbe_base_config_nonpaged_metadata_state_t np_state)
{
    base_config_nonpaged_metadata_p->nonpaged_metadata_state = np_state;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_nonpaged_metadata_set_background_op_bitmask(fbe_base_config_nonpaged_metadata_t * base_config_nonpaged_metadata_p, 
                                                                fbe_base_config_background_operation_t background_op_bitmask)
{
    base_config_nonpaged_metadata_p->operation_bitmask = background_op_bitmask;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_nonpaged_metadata_get_background_op_bitmask(fbe_base_config_t* base_config_p, 
                                                                fbe_base_config_background_operation_t* background_op_bitmask)
{
    fbe_base_config_nonpaged_metadata_t* base_config_nonpaged_metadata_p;
    fbe_base_config_get_nonpaged_metadata_ptr(base_config_p, (void **)&base_config_nonpaged_metadata_p);
    *background_op_bitmask = base_config_nonpaged_metadata_p->operation_bitmask ;
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_base_config_class_set_nonpaged_metadata(fbe_generation_code_t generation_number,
                                            fbe_object_id_t       object_id, 
                                            fbe_u8_t *            nonpaged_data_p)
{
    fbe_base_config_nonpaged_metadata_t * base_config_nonpaged_metadata_p = NULL;

    if (nonpaged_data_p == NULL) {
        /* log error and return fail */
        return FBE_STATUS_GENERIC_FAILURE;
    }
    base_config_nonpaged_metadata_p = (fbe_base_config_nonpaged_metadata_t *) nonpaged_data_p;
    fbe_base_config_nonpaged_metadata_set_object_id(base_config_nonpaged_metadata_p, object_id);
    fbe_base_config_nonpaged_metadata_set_generation_number(base_config_nonpaged_metadata_p, generation_number);
    return FBE_STATUS_OK;
}


fbe_status_t 
fbe_base_config_set_stack_limit(fbe_base_config_t * base_config)
{
    fbe_status_t status;

    status = fbe_block_transport_server_set_stack_limit(&base_config->block_transport_server);

    return status;
}

fbe_status_t fbe_base_config_get_default_offset(fbe_base_config_t * base_config_p, fbe_lba_t *default_offset)
{
    *default_offset = fbe_block_transport_server_get_default_offset(&base_config_p->block_transport_server);
    return FBE_STATUS_OK;
}

/*******************************
 * end fbe_base_config_main.c
 *******************************/
/*!**************************************************************
 * fbe_base_config_set_default_offset()
 ****************************************************************
 * @brief
 *  get the default offset of this object and sets it on the block transport
 *
 * @param base_config_p - The Base config object.
 * @return status - The status of the operation.
 *
 ****************************************************************/
fbe_status_t fbe_base_config_set_default_offset(fbe_base_config_t *base_config_p)
{
    fbe_object_id_t     my_object_id = FBE_OBJECT_ID_INVALID;
    fbe_lba_t           lba = 0x11112222;
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_class_id_t      obj_class_id;

    fbe_base_object_get_object_id((fbe_base_object_t *)base_config_p, &my_object_id);

    obj_class_id = fbe_base_object_get_class_id(fbe_base_pointer_to_handle((fbe_base_t *)base_config_p));   

    /*let's get it from the database*/
    status = fbe_database_get_default_offset(obj_class_id, my_object_id, &lba);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s failed to get lba offset from DB. Obj: %d\n",
                __FUNCTION__, my_object_id);

        return status;
    }

    fbe_block_transport_server_set_default_offset(&base_config_p->block_transport_server, lba);

    fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s: lba offset 0x%x, for object: %d\n",
                            __FUNCTION__, (unsigned int)lba, my_object_id);

    return FBE_STATUS_OK;

}
/**************************************
 * end base_config_set_default_offset()
 **************************************/


/*!**************************************************************
 * fbe_base_config_is_system_bg_service_enabled()
 ****************************************************************
 * @brief
 *  whether a specific system background service(identified by the bgs_flag) 
 *  is enabled system wide.
 *
 * @param fbe_base_config_control_system_bg_service_t bgs_flag
 * @return bool.
 *
 ****************************************************************/
fbe_bool_t fbe_base_config_is_system_bg_service_enabled(fbe_base_config_control_system_bg_service_flags_t bgs_flag)
{
    if (base_config_control_system_bg_service_flag & bgs_flag) {
        return FBE_TRUE;
    }
    else {
        return FBE_FALSE;
    }
}
/****************************************************************
 * end fbe_base_config_is_system_bg_service_enabled()
 ****************************************************************/

/*!**************************************************************
 * fbe_base_config_control_system_bg_service()
 ****************************************************************
 * @brief
 *  This function controls system background service flags.
 *  
 *
 * @param fbe_base_config_control_system_bg_service_t
 * @return status - FBE_STATUS_OK.
 *
 ****************************************************************/
fbe_status_t fbe_base_config_control_system_bg_service(fbe_base_config_control_system_bg_service_t * system_bg_service)
{
    if(system_bg_service->enable) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "_system_bg_service: Enable flags 0x%x count 0x%x\n",
                                    (unsigned int)system_bg_service->bgs_flag,
                                    base_config_all_bgs_disabled_count);
    }
    else {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "_system_bg_service: Disable flags 0x%x count 0x%x\n", 
                                    (unsigned int)system_bg_service->bgs_flag,
                                    base_config_all_bgs_disabled_count);
    }

    /*to keep track if someone enabled already all the background services, we'll just flip the bits here once it happened at least once.
	This is used by the scheduller to make sure that if Navicimom did not re-enabled all the bgs, we'll do it on our own.
	All flags will be set as well when the DB will initialize them*/
    if(FBE_IS_TRUE(system_bg_service->issued_by_scheduler))
    {
        base_config_all_bgs_disabled_count = 0;
    }
    else if(FBE_IS_TRUE(system_bg_service->enable) && FBE_IS_TRUE(system_bg_service->issued_by_ndu))
    {
        if(base_config_all_bgs_disabled_count == 0) {
            fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING, 
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "_system_bg_service: Enable All would have caused count to go negative\n");
            return FBE_STATUS_OK;
        }

        base_config_all_bgs_disabled_count--;

        if(base_config_all_bgs_disabled_count > 0) {
            fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO, 
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "_system_bg_service: Enable All; decrement count\n");

            return FBE_STATUS_OK;
        }
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "_system_bg_service: Enable All; enable BGS\n");

        base_config_all_bgs_enabled_externally = FBE_TRUE;
    }
    else if(FBE_IS_FALSE(system_bg_service->enable) && FBE_IS_TRUE(system_bg_service->issued_by_ndu))
    {
        base_config_all_bgs_disabled_count++;

        /* now we need to restart the scheduler to keep an eye on us because we were disabled by NDU */
        fbe_scheduler_start_to_monitor_background_service_enabled();

        if(base_config_all_bgs_disabled_count > 1) {
            fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO, 
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "_system_bg_service: Disable All; increment count\n");
            return FBE_STATUS_OK;
        }

        fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "_system_bg_service: Disable All; disable BGS\n");

        base_config_all_bgs_enabled_externally = FBE_FALSE;
    }

    if (system_bg_service->enable) {
        base_config_control_system_bg_service_flag |= system_bg_service->bgs_flag;
    }
    else{
        base_config_control_system_bg_service_flag &= ~system_bg_service->bgs_flag; 
    }



    
    return FBE_STATUS_OK;

}
/****************************************************************
 * end fbe_base_config_control_system_bg_service()
 ****************************************************************/

/*!**************************************************************
 * fbe_base_config_get_system_bg_service_status()
 ****************************************************************
 * @brief
 *  This function gets system background service status. This function passed
 *  in system_bg_service_p which has a flag to identify which service we will get
 *  enable/disable status for.
 *
 * @param system_bg_service_p 
 * @return status - FBE_STATUS_OK.
 *
 ****************************************************************/
fbe_status_t fbe_base_config_get_system_bg_service_status(fbe_base_config_control_system_bg_service_t* system_bg_service_p)
{
    if (system_bg_service_p == NULL) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s NULL pointer\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    system_bg_service_p->base_config_control_system_bg_service_flag = base_config_control_system_bg_service_flag;

    if((base_config_control_system_bg_service_flag & system_bg_service_p->bgs_flag)== system_bg_service_p->bgs_flag){
        system_bg_service_p->enable = FBE_TRUE;
    } else {
        system_bg_service_p->enable = FBE_FALSE;
    }

    return FBE_STATUS_OK;

}

fbe_status_t
fbe_base_config_passive_request(fbe_base_config_t * base_config)
{
    if(!fbe_cmi_is_peer_alive()){/* There is no peer, nothing needs to be done */
        return FBE_STATUS_OK;
    }

    if(!fbe_base_config_is_active(base_config)){ /* We already passive, nothing needs to be done */
        return FBE_STATUS_OK;
    }

    if (!fbe_base_config_is_peer_clustered_flag_set(base_config, FBE_BASE_CONFIG_CLUSTERED_FLAG_NONPAGED_INITIALIZED)){ /* Peer nonpaged metadata not updated */
        fbe_base_object_trace((fbe_base_object_t *)base_config,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s FAILED peer nonpaged not updated\n", __FUNCTION__);
        return FBE_STATUS_BUSY;
    }

    if (fbe_base_config_is_any_clustered_flag_set(base_config, FBE_BASE_CONFIG_CLUSTERED_FLAG_PASSIVE_MASK) ||
        fbe_base_config_is_any_peer_clustered_flag_set(base_config, FBE_BASE_CONFIG_CLUSTERED_FLAG_PASSIVE_MASK)){ /* Passive request already started */
        fbe_base_object_trace((fbe_base_object_t *)base_config,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s passive request already in progress\n", __FUNCTION__);
        return FBE_STATUS_BUSY;
    }

    fbe_base_object_trace(  (fbe_base_object_t *)base_config,
            FBE_TRACE_LEVEL_DEBUG_LOW,
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s Set PASSIVE_REQ flag\n", __FUNCTION__);

    fbe_base_config_set_clustered_flag(base_config, FBE_BASE_CONFIG_CLUSTERED_FLAG_PASSIVE_REQ);

    return FBE_STATUS_OK;
}

fbe_bool_t fbe_base_config_is_load_balance_enabled(void)
{
    return base_config_enable_load_balance;
}

void fbe_base_config_set_load_balance(fbe_bool_t is_enabled)
{
    base_config_enable_load_balance = is_enabled;
}

/* Accessors for global_path_attr. */
fbe_bool_t 
fbe_base_config_is_global_path_attr_set(fbe_base_config_t * base_config, fbe_path_attr_t global_path_attr)
{
    fbe_bool_t is_attr_set = FBE_FALSE;
    if(((base_config->global_path_attr & global_path_attr) == global_path_attr)) {
        is_attr_set = FBE_TRUE;
    }
    return is_attr_set;
}

void 
fbe_base_config_set_global_path_attr(fbe_base_config_t * base_config, fbe_path_attr_t global_path_attr)
{
    base_config->global_path_attr |= global_path_attr;
}

void 
fbe_base_config_clear_global_path_attr(fbe_base_config_t * base_config, fbe_path_attr_t global_path_attr)
{
    base_config->global_path_attr &= ~global_path_attr;
}
/****************************************************************
 * end fbe_base_config_get_system_bg_service_status()
 ****************************************************************/

fbe_bool_t fbe_base_config_all_bgs_started_externally(void)
{
	return base_config_all_bgs_enabled_externally;
}

fbe_status_t fbe_base_config_send_specialize_notification(fbe_base_config_t * base_config)
{
	fbe_status_t status;
	fbe_object_id_t object_id;
	fbe_notification_info_t notification_info;
	fbe_class_id_t class_id;
	fbe_handle_t object_handle;

    fbe_base_object_get_object_id((fbe_base_object_t *)base_config, &object_id);
	notification_info.notification_type = FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_SPECIALIZE; /* Need to check if it is current state */
    
	object_handle = fbe_base_pointer_to_handle((fbe_base_t *) base_config);
	class_id = fbe_base_object_get_class_id(object_handle);
	notification_info.class_id = class_id;

	status = fbe_topology_class_id_to_object_type(class_id, &notification_info.object_type);

	if(status != FBE_STATUS_OK){
		notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
	}

    status = fbe_notification_send(object_id, notification_info);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_config, 
                          FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                          "%s Notification send Failed \n", __FUNCTION__);

		return status;
	}

	return FBE_STATUS_OK;
}


/*!***************************************************************
 * fbe_base_config_encryption_remove_keys_downstream()
 ****************************************************************
 * @brief
 *  This function removes encryption keys from downstream.
 *
 * @param base_config_p - The base config object.
 *
 * @return
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  12/18/2013 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t 
fbe_base_config_encryption_remove_keys_downstream(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_u32_t width;
    fbe_base_config_memory_allocation_chunks_t mem_alloc_chunks;

    /* Get the width of the raid group object. */
    fbe_base_config_get_width(base_config_p, &width);

    /* We need to send the request down to all the edges and so get the width and allocate
     * packet for every edge */
    /* Set the various counts and buffer size */
    mem_alloc_chunks.buffer_count = width;
    mem_alloc_chunks.number_of_packets = width;
    mem_alloc_chunks.sg_element_count = 0;
    mem_alloc_chunks.buffer_size = sizeof(fbe_provision_drive_control_register_keys_t);
    mem_alloc_chunks.alloc_pre_read_desc = FALSE;
    mem_alloc_chunks.pre_read_desc_count = 0;

    /* allocate the subpackets to collect the zero checkpoint for all the edges. */
    status = fbe_base_config_memory_allocate_chunks(base_config_p,
                                                    packet_p,
                                                    mem_alloc_chunks,
                                                    (fbe_memory_completion_function_t)fbe_base_config_remove_keys_downstream_allocation_completion); 
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s memory allocation failed, status:0x%x\n", __FUNCTION__, status);
        return status; 
    }

    return FBE_STATUS_PENDING;
}
/********************************************
 * end fbe_base_config_encryption_remove_keys_downstream()
 ********************************************/

/*!****************************************************************************
 * fbe_base_config_remove_keys_downstream_allocation_completion()
 ******************************************************************************
 * @brief
 *  This function gets called after memory has been allocated and sends the 
 *  unregister request down to the PVD
 *
 * @param memory_request_p  - memory request.
 * @param context           - context passed to acquire the memory.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  12/18/2013 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_base_config_remove_keys_downstream_allocation_completion(fbe_memory_request_t * memory_request_p,
                                                             fbe_memory_completion_context_t context)
{
    fbe_packet_t *                                  packet_p = NULL;
    fbe_packet_t **                                 new_packet_p = NULL;
    fbe_u8_t **                                     buffer_p = NULL;
    fbe_payload_ex_t *                              new_sep_payload_p = NULL;
    fbe_payload_control_operation_t *               new_control_operation_p = NULL;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_base_config_t *                             base_config_p = NULL;
    fbe_u32_t                                       width = 0;
    fbe_u32_t                                       index = 0;
    fbe_block_edge_t *                              block_edge_p = NULL;
    fbe_base_config_memory_alloc_chunks_address_t   mem_alloc_chunks_addr = {0};
    fbe_sg_element_t                               *pre_read_sg_list_p = NULL;
    fbe_payload_pre_read_descriptor_t              *pre_read_desc_p = NULL;
    fbe_packet_attr_t                               packet_attr;
    //fbe_provision_drive_control_register_keys_t     *key_info_handle_p;
    fbe_encryption_setup_encryption_keys_t *key_handle = NULL;
    
    /* get the pointer to original packet. */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);

    /* get the pointer to raid group object. */
    base_config_p = (fbe_base_config_t *) context;

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: memory request: 0x%p failed. request state: %d \n",
                              __FUNCTION__, memory_request_p, memory_request_p->request_state);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_FAILED;
    }

    /* get the width of the base config object. */
    fbe_base_config_get_width(base_config_p, &width);

    /* subpacket control buffer size. */
    mem_alloc_chunks_addr.buffer_size = sizeof(fbe_provision_drive_control_register_keys_t);
    mem_alloc_chunks_addr.number_of_packets = width;
    mem_alloc_chunks_addr.buffer_count = width;
    mem_alloc_chunks_addr.sg_element_count = 0;
    mem_alloc_chunks_addr.sg_list_p = NULL;
    mem_alloc_chunks_addr.assign_pre_read_desc = FALSE;
    mem_alloc_chunks_addr.pre_read_sg_list_p = &pre_read_sg_list_p;
    mem_alloc_chunks_addr.pre_read_desc_p = &pre_read_desc_p;
    
    /* get subpacket pointer from the allocated memory. */
    status = fbe_base_config_memory_assign_address_from_allocated_chunks(base_config_p,
                                                                         memory_request_p,
                                                                         &mem_alloc_chunks_addr);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s memory distribution failed, status:0x%x, width:0x%x, buffer_size:0x%x\n",
                              __FUNCTION__, status, width, mem_alloc_chunks_addr.buffer_size);
        fbe_memory_free_request_entry(memory_request_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    new_packet_p = mem_alloc_chunks_addr.packet_array_p;
    buffer_p = mem_alloc_chunks_addr.buffer_array_p;
    status = fbe_base_config_get_key_handle(base_config_p, &key_handle);

    while(index < width)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Remove Key down for index:%d\n", 
                              __FUNCTION__, index);

        /* Initialize the sub packets. */
        fbe_transport_initialize_sep_packet(new_packet_p[index]);
        new_sep_payload_p = fbe_transport_get_payload_ex(new_packet_p[index]);

        /* initialize the control buffer. */
        //key_info_handle_p = (fbe_provision_drive_control_register_keys_t *) buffer_p[index];
        //key_info_handle_p->key_handle = fbe_encryption_get_key_info_handle(key_handle, index);

        /* allocate and initialize the control operation. */
        new_control_operation_p = fbe_payload_ex_allocate_control_operation(new_sep_payload_p);
        fbe_payload_control_build_operation(new_control_operation_p,
                                            FBE_PROVISION_DRIVE_CONTROL_CODE_UNREGISTER_KEYS,
                                            NULL,
                                            0);

        /* Initialize the new packet with FBE_PACKET_FLAG_EXTERNAL same as the master packet */
        fbe_transport_get_packet_attr(packet_p, &packet_attr);
        if (packet_attr & FBE_PACKET_FLAG_EXTERNAL) 
        {
            fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_EXTERNAL);            
        }

        /* set completion functon. */
        fbe_transport_set_completion_function(new_packet_p[index],
                                              fbe_base_config_remove_keys_downstream_subpacket_completion,
                                              base_config_p);

        /* add the subpacket to master packet queue. */
        fbe_transport_add_subpacket(packet_p, new_packet_p[index]);
        index++;
    }

    /* send all the subpackets together. */
    for(index = 0; index < width; index++)
    {
        fbe_base_config_get_block_edge(base_config_p, &block_edge_p, index);
        fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_TRAVERSE);
        fbe_block_transport_send_control_packet(block_edge_p, new_packet_p[index]);
    }

    /* send all the subpackets. */
     return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_base_config_remove_keys_downstream_allocation_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_base_config_remove_keys_downstream_subpacket_completion()
 ******************************************************************************
 * @brief
 *  This function is used to handle the subpacket completion for the registration
 *  function for all the downstream objects.
 *
 * @param packet_p          - packet.
 * @param context           - context passed to request.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  12/18/2013 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t 
fbe_base_config_remove_keys_downstream_subpacket_completion(fbe_packet_t * packet_p,
                                                            fbe_packet_completion_context_t context)
{
    fbe_base_config_t *                                         base_config_p = NULL;
    fbe_packet_t *                                              master_packet_p = NULL;
    fbe_payload_ex_t *                                          master_sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           master_control_operation_p = NULL;
    fbe_payload_ex_t *                                          sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           control_operation_p = NULL;
    fbe_payload_control_status_t                                control_status;
    fbe_payload_control_status_qualifier_t                      control_qualifier;
    fbe_bool_t                                                  is_empty;
    fbe_status_t                                                status;

    /* Cast the context to base config object pointer. */
    base_config_p = (fbe_base_config_t *) context;

    /* Get the master packet, its payload and associated control operation. */
    master_packet_p = fbe_transport_get_master_packet(packet_p);
    master_sep_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    master_control_operation_p = fbe_payload_ex_get_control_operation(master_sep_payload_p);

    /* Get the status of the subpacket operation. */
    status = fbe_transport_get_status_code(packet_p);

    /* Get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation_p, &control_qualifier);

    /* If any of the subpacket failed with bad status then update the master status. */
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(master_packet_p, status, 0);
    }

    /* If any of the subpacket failed with failure status then update the master status. */
    if(control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_payload_control_set_status(master_control_operation_p, control_status);
        fbe_payload_control_set_status_qualifier(master_control_operation_p, control_qualifier);
    }

    /* Remove the sub packet from master queue. */ 
    fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);

    /* When the queue is empty, all subpackets have completed. */
    if(is_empty) {
        fbe_transport_destroy_subpackets(master_packet_p);
        //fbe_memory_request_get_entry_mark_free(&master_packet_p->memory_request, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(&master_packet_p->memory_request);

        fbe_payload_control_get_status(master_control_operation_p, &control_status);

        if(control_status == FBE_PAYLOAD_CONTROL_STATUS_OK)
        {
            fbe_base_config_encryption_state_t encryption_state;
            fbe_base_config_get_encryption_state(base_config_p, &encryption_state);

            fbe_base_config_set_encryption_state(base_config_p, FBE_BASE_CONFIG_ENCRYPTION_STATE_ALL_KEYS_REMOVED);
            fbe_base_config_set_key_handle(base_config_p, NULL);
            fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "Encryption: keys removed\n");
        }

        fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(master_packet_p);
    }
    else
    {
        /* Not done with processing sub packets.
         */
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}
/******************************************************************************
 * end fbe_base_config_remove_keys_downstream_subpacket_completion()
 ******************************************************************************/

/*!***************************************************************
 * fbe_base_config_encryption_check_key_state()
 ****************************************************************
 * @brief
 *  This function checks key state before detaching edges.
 *
 * @param base_config_p - The base config object.
 *
 * @return
 *  The status of the operation typically FBE_STATUS_OK.
 *
 * @author
 *  12/18/2013 - Created. Lili Chen
 *
 ****************************************************************/
fbe_status_t
fbe_base_config_encryption_check_key_state(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_encryption_setup_encryption_keys_t *key_handle = NULL;
    fbe_base_config_encryption_state_t state;
    fbe_system_encryption_mode_t system_encryption_mode;

    fbe_database_get_system_encryption_mode(&system_encryption_mode);
    if (system_encryption_mode != FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED)
    {
        return FBE_STATUS_CONTINUE;
    }

    if (!fbe_base_config_is_permanent_destroy(base_config_p))
    {
        return FBE_STATUS_CONTINUE;
    }

    if ((base_config_p->base_object.class_id < FBE_CLASS_ID_RAID_FIRST) || 
        (base_config_p->base_object.class_id > FBE_CLASS_ID_RAID_LAST))
    {
        return FBE_STATUS_CONTINUE;
    }

#if 0
    if (fbe_raid_geometry_is_raid10(fbe_raid_group_get_raid_geometry((fbe_raid_group_t *)base_config_p)))
    {
        return FBE_STATUS_CONTINUE;
    }
#endif

    fbe_base_config_get_encryption_state(base_config_p, &state);
    fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "Encryption: keys needs to be removed state %d\n", state);

    if (state == FBE_BASE_CONFIG_ENCRYPTION_STATE_ALL_KEYS_REMOVED)
    {
        return FBE_STATUS_CONTINUE;
    }

    if (fbe_base_config_is_flag_set(base_config_p, FBE_BASE_CONFIG_FLAG_REMOVE_ENCRYPTION_KEYS))
    {
        fbe_base_config_get_key_handle(base_config_p, &key_handle);
        if (key_handle == NULL)
        {
            fbe_base_config_set_encryption_state(base_config_p, 
                                                 FBE_BASE_CONFIG_ENCRYPTION_STATE_ALL_KEYS_REMOVED);
            fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "Encryption: keys not exist, orig state %d\n", state);
            return FBE_STATUS_CONTINUE;
        }

        fbe_base_config_set_encryption_state(base_config_p, 
                                             FBE_BASE_CONFIG_ENCRYPTION_STATE_PUSH_ALL_KEYS_REMOVE_DOWNSTREAM);
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption: KEYS_DELETE_DOWNSTREAM, orig state %d\n", state);
        status = fbe_base_config_encryption_remove_keys_downstream(base_config_p, packet_p);
        return status;
    }

    /* We should inform KMS to remove the key now */
    fbe_base_config_set_encryption_state_with_notification(base_config_p, 
                                                           FBE_BASE_CONFIG_ENCRYPTION_STATE_ALL_KEYS_NEED_REMOVED);


    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_base_config_encryption_check_key_state()
 ********************************************/

fbe_status_t 
fbe_base_config_get_edges_with_attribute(fbe_base_config_t * base_config, 
                                         fbe_path_attr_t path_attrib,
                                         fbe_u32_t * path_attr_count_p)
{
    fbe_block_edge_t *edge = NULL;
    fbe_u32_t edge_index;
    fbe_u32_t edge_count = 0;
    
    for (edge_index = 0; edge_index < base_config->width; edge_index ++) {
        fbe_base_config_get_block_edge(base_config, &edge, edge_index);
        fbe_block_transport_get_path_attributes(edge, &path_attrib);

        if (path_attrib & path_attrib) {
            /* If we have this attribute, consider the edge broken.
             */
            edge_count++;
        }
    }

    *path_attr_count_p = edge_count;
    return FBE_STATUS_OK;
}
