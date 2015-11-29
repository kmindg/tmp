/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_base_config_power_save.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the power saving handling of base config
 * 
 * @ingroup base_config_class_files
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "base_object_private.h"
#include "fbe_base_config_private.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe/fbe_winddk.h"
#include "fbe_event_service.h"
#include "fbe_cmi.h"

fbe_status_t fbe_base_config_check_if_need_to_power_save(fbe_base_config_t *base_config_p, fbe_bool_t *need_to_power_save)
{
    fbe_bool_t                  	active = FBE_FALSE;
    fbe_u32_t						elapsed_seconds = 0;
    fbe_time_t						last_io_time = 0;
    fbe_system_power_saving_info_t	get_info;
	fbe_medic_action_priority_t		medic_priority;

    /*is the global power saving enabled?*/
    fbe_base_config_get_system_power_saving_info(&get_info);
    if (!get_info.enabled) {
        *need_to_power_save = FBE_FALSE;
        return FBE_STATUS_OK;
    }

    /*let's get the time now and the time of the last IO*/
    fbe_block_transport_server_get_last_io_time(&base_config_p->block_transport_server, &last_io_time);

    /*did enough time pass for us to go to power save ? (we might get here 0xffffffff if the drive was up for 136 years :)*/
    elapsed_seconds = fbe_get_elapsed_seconds(last_io_time);
    if ((elapsed_seconds < base_config_p->power_saving_idle_time_in_seconds) ||
        (base_config_p->block_transport_server.attributes & FBE_BLOCK_TRANSPORT_FLAGS_IO_IN_PROGRESS)) {

        *need_to_power_save = FBE_FALSE;
        return FBE_STATUS_OK;
    }

    /*are we even the active side, if not, we have nothing to care for*/
    active = fbe_base_config_is_active(base_config_p);
    if (!active) {
        /*if we are on the passive side, we will check what state the peer is and if he is in hibrenation, we will start power saving
        simply because it is waiting for us*/
        if (fbe_cmi_is_peer_alive() && 
            fbe_base_config_is_peer_clustered_flag_set(base_config_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_HIBERNATE_READY)){
            *need_to_power_save = FBE_TRUE;
        }else{
            *need_to_power_save = FBE_FALSE;
        }
        
        return FBE_STATUS_OK;
    }

    /*even if the global is enabled, we may disable power save on a specific object*/
    if (!base_config_is_object_power_saving_enabled(base_config_p)) {
        *need_to_power_save = FBE_FALSE;
        return FBE_STATUS_OK;
    }

    /*we also have to check if the peer's last IO time is good enough. Any caluse that is not 0 means the peer passed
    this time*/
    fbe_base_config_metadata_get_peer_last_io_time(base_config_p, &last_io_time);
    if (last_io_time == 0 && fbe_cmi_is_peer_alive()) {
        *need_to_power_save = FBE_FALSE;
        return FBE_STATUS_OK;
    }

	/*let check the medic priority under us, if some stuff is going on, we don't want to save power*/
	medic_priority = fbe_base_config_get_downstream_edge_priority(base_config_p);
	if (FBE_MEDIC_ACTION_IDLE != medic_priority && FBE_MEDIC_ACTION_SNIFF != medic_priority) {
		/*sorry, something is going on*/
		*need_to_power_save = FBE_FALSE;
        return FBE_STATUS_OK;
	}

    /*at this point, we are ready to hibernate because we are the active side and enough time passed w/o IO*/
    *need_to_power_save = FBE_TRUE;

    return FBE_STATUS_OK;

}

fbe_status_t base_config_send_hibernation_usurper_to_servers(fbe_base_config_t *base_config_p, fbe_packet_t * packet)
{
    fbe_block_edge_t *								edge = NULL;
    fbe_u32_t 										edge_index;
    fbe_payload_control_operation_t *           	control_operation = NULL;
    fbe_payload_ex_t *                             	payload = NULL;
    fbe_block_client_hibernating_info_t				client_hibernating_info;
    fbe_payload_control_status_t					control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    fbe_status_t									status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u64_t										lowest_latency_time_in_seconds = 0;
    fbe_u32_t										total_unable_to_hibernate = 0;
    fbe_path_state_t								path_state = FBE_PATH_STATE_INVALID;
    fbe_bool_t										can_clear_condition = FBE_TRUE;
    fbe_path_attr_t									path_attr = 0;

    /*first of all we need to check what is the lowest latency time on the edges above us, the lowest one wins*/
    status  = fbe_block_transport_server_get_lowest_ready_latency_time(&base_config_p->block_transport_server,
                                                                       &lowest_latency_time_in_seconds);

    if(status != FBE_STATUS_OK) {    
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                "%s can't figure lowest latency time from edges\n",__FUNCTION__);
        
        return FBE_STATUS_GENERIC_FAILURE;    
    }

    if (lowest_latency_time_in_seconds == 0) {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_WARNING,FBE_TRACE_MESSAGE_ID_INFO,
                                "%s client request latency time is 0, which prevents power saving. Change it to default value.\n",__FUNCTION__);
        lowest_latency_time_in_seconds = FBE_LUN_MAX_LATENCY_TIME_DEFAULT;
    }

    for(edge_index = 0; edge_index < base_config_p->width; edge_index ++){

        fbe_base_config_get_block_edge(base_config_p, &edge, edge_index);
        fbe_block_transport_get_path_state(edge, &path_state);
        fbe_block_transport_get_path_attributes(edge, &path_attr);

        /*now there is some logic to do:
        1) If there is an invalid edge here, just continue to the next one. this is possible and OK with many objects
        2) If the edge is there but it's broken or busy, we jus return a status that indicate not to clear the condition
        3) if the object is already sleeping or the edge to it was already marked in a previous monitor cycle, just go on
        */
        if ((path_state == FBE_PATH_STATE_INVALID) || 
            (path_state == FBE_PATH_STATE_SLUMBER) || 
            (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_HIBERNATING)) {
            continue;/*no point on talking to this edge, it's either non exisitng or already sleeping*/
        }

        if ((path_state != FBE_PATH_STATE_ENABLED) && (path_state != FBE_PATH_STATE_SLUMBER) && (path_state != FBE_PATH_STATE_BROKEN)) {
            /*let's stop here and continue in the next monitor cycle*/
            can_clear_condition = FBE_FALSE;
            continue;/*no point on talking to this edge*/
        }
        

        payload = fbe_transport_get_payload_ex(packet);
        control_operation = fbe_payload_ex_allocate_control_operation(payload); 
        if(control_operation == NULL) {    
            fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s Failed to allocate control operation\n",__FUNCTION__);
            
            return FBE_STATUS_GENERIC_FAILURE;    
        }

        /*set the usurper correctly*/
        client_hibernating_info.max_latency_time_is_sec = lowest_latency_time_in_seconds;
        client_hibernating_info.active_side = fbe_base_config_is_active(base_config_p);/*will be used actually only by PDO when it gets it*/

        fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s send client hibernating cmd\n",__FUNCTION__);
        
        fbe_payload_control_build_operation(control_operation,  
                                            FBE_BLOCK_TRANSPORT_CONTROL_CODE_CLIENT_HIBERNATING,  
                                            &client_hibernating_info, 
                                            sizeof(fbe_block_client_hibernating_info_t)); 

        fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
        fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_TRAVERSE | FBE_PACKET_FLAG_EXTERNAL);/*in case this will be sent to logical drive, it will forward it to PDO*/

        fbe_block_transport_send_control_packet(edge, packet);  
        
        fbe_transport_wait_completion(packet);

        status = fbe_transport_get_status_code(packet);
        fbe_payload_control_get_status(control_operation, &control_status);
        fbe_payload_ex_release_control_operation(payload, control_operation);

        /*any chance this drive just can't satisy the time we give it to spin up, or simply does not suppot power save*/
        if (control_status == FBE_PAYLOAD_CONTROL_STATUS_INSUFFICIENT_RESOURCES) {
            total_unable_to_hibernate++;
            /*did all of them returned the same code?*/
            if (total_unable_to_hibernate == base_config_p->width) {
                fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s none of the clients support hibernation\n",__FUNCTION__);

                return FBE_STATUS_INSUFFICIENT_RESOURCES;
            }

        }else if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||(status != FBE_STATUS_OK)) {
             fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s Error is setting hibernation on edge\n",__FUNCTION__);
            
            return FBE_STATUS_GENERIC_FAILURE; 
        }
    
    }

    
    return (can_clear_condition ? FBE_STATUS_OK : FBE_STATUS_BUSY);

}

fbe_bool_t 
base_config_is_object_power_saving_enabled(fbe_base_config_t *base_config_p)
{
    fbe_bool_t is_object_power_saving_enabled;
    is_object_power_saving_enabled = fbe_base_config_is_flag_set(base_config_p, FBE_BASE_CONFIG_FLAG_POWER_SAVING_ENABLED);

    return is_object_power_saving_enabled;	
}

fbe_status_t fbe_base_config_enable_object_power_save(fbe_base_config_t* base_config_p)
{
    fbe_status_t 							status = FBE_STATUS_GENERIC_FAILURE;
    fbe_class_id_t							my_class_id = FBE_CLASS_ID_INVALID;
    
    /*first, enable power saving for us*/
    base_config_enable_object_power_save(base_config_p);

    /*if we are the PVD, we are all set, we are the last one in the chain*/
    my_class_id = fbe_base_object_get_class_id(fbe_base_pointer_to_handle((fbe_base_t *) base_config_p));   

    if (my_class_id != FBE_CLASS_ID_PROVISION_DRIVE) {
        /*and enable all of our severs as well*/
        status = fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, 
                                        (fbe_base_object_t *)base_config_p, 
                                        FBE_BASE_CONFIG_LIFECYCLE_COND_ENABLE_POWER_SAVING_ON_SERVERS);
    
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Failed to enable servers power saving\n", __FUNCTION__);
    
            return status;
        }
    
    }

    return FBE_STATUS_OK;

}

fbe_status_t fbe_base_config_disable_object_power_save(fbe_base_config_t* base_config_p)
{
    /*we might have nmore things here in the future so we leave it as a seperate function*/
    base_config_disable_object_power_save(base_config_p);
    
    return FBE_STATUS_OK;

}

fbe_status_t fbe_base_config_usurper_set_system_power_saving_info(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_system_power_saving_info_t * 		power_save_info = NULL;    /* INPUT */
    fbe_status_t 							status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t * 						payload = NULL;
    fbe_payload_control_operation_t * 		control_operation = NULL;
    fbe_payload_control_buffer_length_t 	len = 0;
    
    fbe_base_config_class_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s entry\n", __FUNCTION__);

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &power_save_info);
    if (power_save_info == NULL){
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
    if(len != sizeof(fbe_system_power_saving_info_t)){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Invalid length:%d\n",
                                    __FUNCTION__, len);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    status = fbe_base_config_set_system_power_saving_info(power_save_info);
    if(status != FBE_STATUS_OK){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s failed to set power saving info\n",
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

fbe_status_t fbe_base_config_usurper_get_system_power_saving_info(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_system_power_saving_info_t * 		power_save_info = NULL;    /* INPUT */
    fbe_status_t 							status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t * 						payload = NULL;
    fbe_payload_control_operation_t * 		control_operation = NULL;
    fbe_payload_control_buffer_length_t 	len = 0;
    
    fbe_base_config_class_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s entry\n", __FUNCTION__);

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &power_save_info);
    if (power_save_info == NULL){
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
    if(len != sizeof(fbe_system_power_saving_info_t)){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Invalid length:%d\n",
                                    __FUNCTION__, len);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    status = fbe_base_config_get_system_power_saving_info(power_save_info);
    if(status != FBE_STATUS_OK){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s failed to set power saving info\n",
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

fbe_status_t fbe_base_config_get_object_power_save_info(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_base_config_object_power_save_info_t * 		object_power_save_info = NULL;    /* INPUT */
    fbe_payload_ex_t * 								payload = NULL;
    fbe_payload_control_operation_t * 				control_operation = NULL;
    fbe_payload_control_buffer_length_t 			len = 0;
    fbe_u32_t										elapsed_seconds = 0;
    fbe_time_t										last_io_time = 0;
    
    fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                                FBE_TRACE_LEVEL_DEBUG_HIGH,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s entry\n", __FUNCTION__);

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &object_power_save_info);
    if (object_power_save_info == NULL){
        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                                    FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s fbe_payload_control_get_buffer failed\n",
                                    __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);    
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_base_config_object_power_save_info_t)){
        fbe_base_object_trace((fbe_base_object_t*)base_config_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Invalid length:%d\n",
                              __FUNCTION__, len);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    /*let's get the time now and the time of the last IO*/
    fbe_block_transport_server_get_last_io_time(&base_config_p->block_transport_server, &last_io_time);
    elapsed_seconds = fbe_get_elapsed_seconds(last_io_time);
    object_power_save_info->seconds_since_last_io = elapsed_seconds;
    object_power_save_info->power_saving_enabled = base_config_is_object_power_saving_enabled(base_config_p);
    fbe_base_config_get_power_saving_idle_time(base_config_p, &object_power_save_info->configured_idle_time_in_seconds);
    
    /* Lock base config before getting the power saving state */
    fbe_base_config_lock(base_config_p);    
 
    fbe_base_config_metadata_get_power_save_state(base_config_p, &object_power_save_info->power_save_state);

    /* Unlock base config after getting the power saving state*/
    fbe_base_config_unlock(base_config_p);    

    object_power_save_info->hibernation_time = base_config_p->hibernation_time;
    object_power_save_info->flags = base_config_p->flags;

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}

/*send a ususrper command to all the sleeping edges, if all of them are woken up, we will be able to clear the condition*/
fbe_status_t fbe_base_config_send_wake_up_usurper_to_servers(fbe_base_config_t *base_config_p, fbe_packet_t * packet)
{
    fbe_block_edge_t *								edge = NULL;
    fbe_u32_t 										edge_index;
    fbe_payload_control_operation_t *           	control_operation = NULL;
    fbe_payload_ex_t *                             	payload = NULL;
    fbe_payload_control_status_t					control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    fbe_status_t									status = FBE_STATUS_GENERIC_FAILURE;
    fbe_path_state_t								path_state = FBE_PATH_STATE_INVALID;
    
    for(edge_index = 0; edge_index < base_config_p->width; edge_index ++){

        fbe_base_config_get_block_edge(base_config_p, &edge, edge_index);
        fbe_block_transport_get_path_state(edge, &path_state);
        
        if (path_state == FBE_PATH_STATE_INVALID) {
            continue;/*no point on talking to this edge, it's non exisitng*/
        }

        payload = fbe_transport_get_payload_ex(packet);
        control_operation = fbe_payload_ex_allocate_control_operation(payload); 
        if(control_operation == NULL) {    
            fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s Failed to allocate control operation\n",__FUNCTION__);
            
            return FBE_STATUS_GENERIC_FAILURE;    
        }

        fbe_payload_control_build_operation(control_operation,  
                                            FBE_BLOCK_TRANSPORT_CONTROL_CODE_EXIT_HIBERNATION,  
                                            NULL, 
                                            0); 

        fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s send wakeup to servers \n",__FUNCTION__);


        fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
        fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_TRAVERSE);/*in case this will be sent to logical drive, it will forward it to PDO*/

        status  = fbe_block_transport_send_control_packet(edge, packet);  
        if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING)) {    
            fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s Failed to send hibernate packet\n",__FUNCTION__);

            fbe_payload_ex_release_control_operation(payload, control_operation);

            return FBE_STATUS_GENERIC_FAILURE;    
        }

        fbe_transport_wait_completion(packet);

        status = fbe_transport_get_status_code(packet);
        fbe_payload_control_get_status(control_operation, &control_status);
        fbe_payload_ex_release_control_operation(payload, control_operation);

        if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || status != FBE_STATUS_OK) {
             fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s Error is setting hibernation on edge\n",__FUNCTION__);
            
            return FBE_STATUS_GENERIC_FAILURE; 
        }
    
    }

    
    return FBE_STATUS_OK ;

}
fbe_status_t base_config_enable_object_power_save(fbe_base_config_t *base_config_p)
{
    
    fbe_base_config_set_flag(base_config_p, FBE_BASE_CONFIG_FLAG_POWER_SAVING_ENABLED);

    fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Power saving enabled for object\n");

    return FBE_STATUS_OK;
}

fbe_status_t base_config_disable_object_power_save(fbe_base_config_t *base_config_p)
{
    fbe_status_t    status;
    fbe_lifecycle_state_t      lifecycle_state;
	
	fbe_base_config_clear_flag(base_config_p, FBE_BASE_CONFIG_FLAG_POWER_SAVING_ENABLED);
	
	fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Power saving disabled for object\n");

	status = fbe_base_object_get_lifecycle_state((fbe_base_object_t *)base_config_p, &lifecycle_state);
	
    if (lifecycle_state == FBE_LIFECYCLE_STATE_HIBERNATE) {
        /*and get back to ready*/
		fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, 
							  (fbe_base_object_t *)base_config_p, 
                               FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);
    }
	
	return FBE_STATUS_OK;
}

fbe_status_t fbe_base_config_send_power_save_enable_usurper_to_servers(fbe_base_config_t *base_config_p, fbe_packet_t * packet)
{
    fbe_block_edge_t *								edge = NULL;
    fbe_u32_t 										edge_index;
    fbe_payload_control_operation_t *           	control_operation = NULL;
    fbe_payload_ex_t *                             	payload = NULL;
    fbe_payload_control_status_t					control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    fbe_status_t									status = FBE_STATUS_GENERIC_FAILURE;
    fbe_path_state_t								path_state = FBE_PATH_STATE_INVALID;
    
    for(edge_index = 0; edge_index < base_config_p->width; edge_index ++){

        fbe_base_config_get_block_edge(base_config_p, &edge, edge_index);
        fbe_block_transport_get_path_state(edge, &path_state);

        if ((path_state == FBE_PATH_STATE_INVALID)) {
            continue;/*no point on talking to this edge, it's non exisitng*/
        }

        payload = fbe_transport_get_payload_ex(packet);
        control_operation = fbe_payload_ex_allocate_control_operation(payload); 
        if(control_operation == NULL) {    
            fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s Failed to allocate control operation\n",__FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;    
        }

        fbe_payload_control_build_operation(control_operation,  
                                            FBE_BASE_CONFIG_CONTROL_CODE_ENABLE_OBJECT_POWER_SAVE,  
                                            NULL, 
                                            0); 

        fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
        fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_TRAVERSE);/*in case this will be sent to logical drive, it will forward it to PDO*/

        status  = fbe_block_transport_send_control_packet(edge, packet);  
        if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING)) {    
            fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s Failed to send enable power saving packet\n",__FUNCTION__);

            fbe_payload_ex_release_control_operation(payload, control_operation);
            return FBE_STATUS_GENERIC_FAILURE;    
        }

        fbe_transport_wait_completion(packet);

        status = fbe_transport_get_status_code(packet);
        fbe_payload_control_get_status(control_operation, &control_status);
        fbe_payload_ex_release_control_operation(payload, control_operation);

        if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) || status != FBE_STATUS_OK) {
             fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s Error is setting hibernation on edge\n",__FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE; 
        }

    }


    return FBE_STATUS_OK;


}

fbe_status_t fbe_base_config_set_object_power_save_idle_time(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_base_config_set_object_idle_time_t *		set_idle_time = NULL;    /* INPUT */
    fbe_payload_ex_t * 								payload = NULL;
    fbe_payload_control_operation_t * 				control_operation = NULL;
    fbe_payload_control_buffer_length_t 			len = 0;
    fbe_class_id_t									my_class_id = FBE_CLASS_ID_INVALID;
    
    fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_DEBUG_HIGH,	
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s entry\n", __FUNCTION__);

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &set_idle_time);

    if (set_idle_time == NULL){
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_payload_control_get_buffer failed\n",
                              __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);    
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_base_config_set_object_idle_time_t)){
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Invalid length:%d\n",
                              __FUNCTION__, len);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    /*if this is a LUN this usuper command is TEMPORARY invalid (until layered drivers will start using the
    FLARE_POWER_SAVE_POLICY IOCTL, we allow this at RAID level and below,*/
    my_class_id = fbe_base_object_get_class_id(fbe_base_pointer_to_handle((fbe_base_t *) base_config_p)); 

    if (my_class_id == FBE_CLASS_ID_LUN) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Illegal command for LU, send it to RAID only\n",
                                __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;

    }

    fbe_base_config_set_power_saving_idle_time(base_config_p, set_idle_time->power_save_idle_time_in_seconds); 

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}

fbe_status_t fbe_base_config_power_save_verify_servers_not_slumber(fbe_base_config_t *base_config_p)
{
    fbe_block_edge_t *								edge = NULL;
    fbe_u32_t 										edge_index;
    fbe_path_state_t								path_state = FBE_PATH_STATE_INVALID;
    
    for(edge_index = 0; edge_index < base_config_p->width; edge_index ++){

        fbe_base_config_get_block_edge(base_config_p, &edge, edge_index);
        fbe_block_transport_get_path_state(edge, &path_state);
        
        if (path_state == FBE_PATH_STATE_INVALID) {
            continue;/*no point on talking to this edge, it's non exisitng*/
        }

        if ((path_state == FBE_PATH_STATE_SLUMBER) || (path_state == FBE_PATH_STATE_DISABLED)) {
            return FBE_STATUS_BUSY;/*at least one is still sleeping or activate*/
        }
    }

    return FBE_STATUS_OK;/*all are not sleeping*/

}

fbe_status_t base_config_send_spindown_qualified_usurper_to_servers(fbe_base_config_t *base_config_p, fbe_packet_t * packet)
{
    fbe_block_edge_t *								edge = NULL;
    fbe_u32_t 										edge_index;
    fbe_payload_control_operation_t *           	control_operation = NULL;
    fbe_payload_ex_t *                             	payload = NULL;
    fbe_base_config_power_save_capable_info_t		power_save_capable_info;
    fbe_payload_control_status_t					control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    fbe_status_t									status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t										spindown_qualified = FBE_TRUE;
    fbe_path_state_t								path_state = FBE_PATH_STATE_INVALID;

    for(edge_index = 0; edge_index < base_config_p->width; edge_index ++){

        fbe_base_config_get_block_edge(base_config_p, &edge, edge_index);
        fbe_block_transport_get_path_state(edge, &path_state);

        /*now there is some logic to do:
        1) If there is an invalid edge here, just continue to the next one. this is possible and OK with many objects
        2) If the edge is there but it's broken or busy, we jus return a status that indicate not to clear the condition
        */
        if ((path_state == FBE_PATH_STATE_INVALID)) {
            continue;/*no point on talking to this edge, it's either non exisitng or already sleeping*/
        }

        if ((path_state == FBE_PATH_STATE_DISABLED) || (path_state == FBE_PATH_STATE_BROKEN) || (path_state == FBE_PATH_STATE_GONE)) {
            /*let's stop here if the edge is currently busy or broken.*/
            return FBE_STATUS_BUSY;
        }
        
        payload = fbe_transport_get_payload_ex(packet);
        control_operation = fbe_payload_ex_allocate_control_operation(payload); 
        if(control_operation == NULL) {    
            fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s Failed to allocate control operation\n",__FUNCTION__);
            
            return FBE_STATUS_GENERIC_FAILURE;    
        }

        /*set the usurper correctly*/
        power_save_capable_info.spindown_qualified = FBE_TRUE;
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s send get server spindown qualified cmd\n",__FUNCTION__);
        
        fbe_payload_control_build_operation(control_operation,  
                                            FBE_BASE_CONFIG_CONTROL_CODE_GET_SPINDOWN_QUALIFIED,  
                                            &power_save_capable_info, 
                                            sizeof(fbe_base_config_power_save_capable_info_t)); 

        fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

        fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_TRAVERSE);
        fbe_block_transport_send_control_packet(edge, packet);  
        
        fbe_transport_wait_completion(packet);

        status = fbe_transport_get_status_code(packet);
        fbe_payload_control_get_status(control_operation, &control_status);
        fbe_payload_ex_release_control_operation(payload, control_operation);
            
        if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||(status != FBE_STATUS_OK)) {
            fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s Failed to get spindown qualified flag on edge\n",__FUNCTION__);
            
            return FBE_STATUS_GENERIC_FAILURE; 
        }
        else{
            if (power_save_capable_info.spindown_qualified == FBE_FALSE) {
                spindown_qualified = FBE_FALSE;
                break;
            }
        }  
    }
    if (spindown_qualified == FBE_TRUE) {
        return FBE_STATUS_OK;
    }
    else{
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

}

fbe_status_t fbe_base_config_set_power_saving_idle_time_usurper_to_servers(fbe_base_config_t *base_config_p, fbe_packet_t * packet)
{
    fbe_block_edge_t *								edge = NULL;
    fbe_u32_t 										edge_index;
    fbe_payload_control_operation_t *           	control_operation = NULL;
    fbe_payload_ex_t *                             	payload = NULL;
    fbe_payload_control_status_t					control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    fbe_status_t									status = FBE_STATUS_GENERIC_FAILURE;
    fbe_path_state_t								path_state = FBE_PATH_STATE_INVALID;
    fbe_bool_t										can_clear_condition = FBE_TRUE;
    fbe_path_attr_t									path_attr = 0;
    fbe_raid_group_configuration_t		    		rg_config_info;


    for(edge_index = 0; edge_index < base_config_p->width; edge_index ++)
    {
        fbe_base_config_get_block_edge(base_config_p, &edge, edge_index);
        fbe_block_transport_get_path_state(edge, &path_state);
        fbe_block_transport_get_path_attributes(edge, &path_attr);


        /*now there is some logic to do:
        1) If there is an invalid edge here, just continue to the next one. this is possible and OK with many objects
        2) If the edge is there but it's broken or busy, we jus return a status that indicate not to clear the condition
        3) if the object is already sleeping or the edge to it was already marked in a previous monitor cycle, just go on
        */
        if ((path_state == FBE_PATH_STATE_INVALID) || 
            (path_state == FBE_PATH_STATE_SLUMBER) || 
            (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_HIBERNATING)) 
        {
            continue;/*no point on talking to this edge, it's either non exisitng or already sleeping*/
        }

        if ((path_state != FBE_PATH_STATE_ENABLED) && (path_state != FBE_PATH_STATE_SLUMBER) && (path_state != FBE_PATH_STATE_BROKEN)) 
        {
            /*let's stop here and continue in the next monitor cycle*/
            can_clear_condition = FBE_FALSE;
            continue;/*no point on talking to this edge*/
        }

        payload = fbe_transport_get_payload_ex(packet);
        control_operation = fbe_payload_ex_allocate_control_operation(payload); 

        if(control_operation == NULL) 
        {    
            fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                    "_idle_time_usurper_to_servers- Failed to allocate control operation\n");
            return FBE_STATUS_GENERIC_FAILURE;    
        }

        /*set the usurper correctly*/
        rg_config_info.power_saving_idle_time_in_seconds = base_config_p->power_saving_idle_time_in_seconds;
        rg_config_info.update_type = FBE_UPDATE_RAID_TYPE_CHANGE_IDLE_TIME;
        
        fbe_payload_control_build_operation(control_operation,  
                                            FBE_RAID_GROUP_CONTROL_CODE_UPDATE_CONFIGURATION,  
                                            &rg_config_info, 
                                            sizeof(fbe_raid_group_configuration_t)); 

        fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
        /*in case this will be sent to logical drive, it will forward it to PDO*/
        fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_TRAVERSE | FBE_PACKET_FLAG_EXTERNAL);

        status  = fbe_block_transport_send_control_packet(edge, packet);  

        if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING)) 
        {    
            fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                    "_idle_time_usurper_to_servers- Failed to send idle time packet\n");

            fbe_payload_ex_release_control_operation(payload, control_operation);

            return FBE_STATUS_GENERIC_FAILURE;    
        }
        
        fbe_transport_wait_completion(packet);

        status = fbe_transport_get_status_code(packet);
        fbe_payload_control_get_status(control_operation, &control_status);
        fbe_payload_ex_release_control_operation(payload, control_operation);

        if ((control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ||(status != FBE_STATUS_OK)) 
        {
             fbe_base_object_trace((fbe_base_object_t*)base_config_p, FBE_TRACE_LEVEL_ERROR,FBE_TRACE_MESSAGE_ID_INFO,
                                    "_idle_time_usurper_to_servers- Error is setting power saving idle time on edge\n");

            return FBE_STATUS_GENERIC_FAILURE; 
        }

    }

    return (can_clear_condition ? FBE_STATUS_OK : FBE_STATUS_BUSY);
}


