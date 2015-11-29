/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_cms_common.c
***************************************************************************
*
* @brief
*  This file contains persistance service common functions to be used by all files
*  
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cms_private.h"
#include "fbe_transport_memory.h"
#include "fbe/fbe_service_manager_interface.h"
#include "fbe_cmi.h"


fbe_status_t cms_complete_packet(fbe_packet_t *packet, fbe_status_t packet_status)
{
    fbe_payload_ex_t *						payload = NULL;
    fbe_payload_control_operation_t *	control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    if ( control_operation != NULL ){
        fbe_payload_control_set_status(control_operation,
                                       packet_status == FBE_STATUS_OK ?
                                       FBE_PAYLOAD_CONTROL_STATUS_OK :
                                       FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }

    fbe_transport_set_status(packet, packet_status, 0);
    return fbe_transport_complete_packet(packet);
}

fbe_status_t cms_get_payload(fbe_packet_t *packet, void **payload_struct, fbe_u32_t expected_payload_len)
{
    fbe_payload_ex_t *						payload = NULL;
    fbe_payload_control_operation_t *	control_operation = NULL;
    fbe_u32_t      						buffer_len = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL){
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s Packet 0x%x returns NULL control_operation.\n", __FUNCTION__, packet);
        cms_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the payload - if we specified a buffer */
    if ( payload_struct != NULL ){
        fbe_payload_control_get_buffer(control_operation, payload_struct);
        if(*payload_struct == NULL){
            cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: Packet 0x%x has NULL payload buffer.\n", __FUNCTION__, packet);
            cms_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_payload_control_get_buffer_length(control_operation, &buffer_len);
        if(buffer_len != expected_payload_len){
            cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: Invalid len %d != %d(expected) \n", 
                           __FUNCTION__, buffer_len, expected_payload_len);

            cms_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return FBE_STATUS_OK;
}

fbe_status_t cms_send_packet_to_service(fbe_payload_control_operation_opcode_t control_code,
													  fbe_payload_control_buffer_t buffer,
													  fbe_payload_control_buffer_length_t buffer_length,
													  fbe_service_id_t service_id,
													  fbe_package_id_t package_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;

    /* Allocate packet.*/
    packet = fbe_transport_allocate_packet();
    if (packet == NULL) {
        cms_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    if (package_id == FBE_PACKAGE_ID_PHYSICAL) {
        fbe_transport_initialize_packet(packet);
    }else{
        fbe_transport_initialize_sep_packet(packet);
    }
    
    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_payload_control_build_operation (control_operation,
                                         control_code,
                                         buffer,
                                         buffer_length);
    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              package_id,
                              service_id,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID); 
    
    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_NO_ATTRIB);

    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    status = fbe_service_manager_send_control_packet(packet);
    if (status != FBE_STATUS_OK) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: error in sending 0x%x to srv 0x%x, status:0x%x\n",
                       __FUNCTION__, control_code, service_id, status);  
    }

    fbe_transport_wait_completion(packet);

    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);
    if (status != FBE_STATUS_OK ) {
        cms_trace(FBE_TRACE_LEVEL_WARNING,
                       "%s: 0x%x to srv 0x%x, return status:0x%x\n",
                       __FUNCTION__, control_code, service_id, status);  
    }

    fbe_payload_control_get_status(control_operation, &control_status);
    if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK ) {
        cms_trace(FBE_TRACE_LEVEL_WARNING,
                       "%s: 0x%x to srv 0x%x, return payload status:0x%x\n",
                       __FUNCTION__, control_code, service_id, control_status);

        switch (control_status){
        case FBE_PAYLOAD_CONTROL_STATUS_BUSY:
            status = FBE_STATUS_BUSY;
            break;
        case FBE_PAYLOAD_CONTROL_STATUS_INSUFFICIENT_RESOURCES:
            status = FBE_STATUS_INSUFFICIENT_RESOURCES;
            break;
        case FBE_PAYLOAD_CONTROL_STATUS_FAILURE:
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        }
        cms_trace(FBE_TRACE_LEVEL_INFO, "%s: Set return status to 0x%x\n", __FUNCTION__, status);  
    }

    /* free the memory*/
    fbe_payload_ex_release_control_operation(payload, control_operation);

    fbe_transport_release_packet(packet);
    return status;
}

fbe_status_t cms_send_packet_to_object(fbe_payload_control_operation_opcode_t control_code,
													  fbe_payload_control_buffer_t buffer,
													  fbe_payload_control_buffer_length_t buffer_length,
													  fbe_object_id_t object_id,
													  fbe_package_id_t package_id)
{
	fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;

    /* Allocate packet.*/
    packet = fbe_transport_allocate_packet();
    if (packet == NULL) {
        cms_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    if (package_id == FBE_PACKAGE_ID_PHYSICAL) {
        fbe_transport_initialize_packet(packet);
    }else{
        fbe_transport_initialize_sep_packet(packet);
    }
    
    payload = fbe_transport_get_payload_ex (packet);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);

    fbe_payload_control_build_operation (control_operation,
                                         control_code,
                                         buffer,
                                         buffer_length);
    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 
    
    /* if the object is in specialized state we'll get the right status and not a messy one */
    fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_EXTERNAL);

    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    status = fbe_service_manager_send_control_packet(packet);
    if (status != FBE_STATUS_OK) {
        cms_trace(FBE_TRACE_LEVEL_ERROR, "%s: error in sending 0x%x to srv 0x%x, status:0x%x\n",
                       __FUNCTION__, control_code, FBE_SERVICE_ID_TOPOLOGY, status);  
    }

    fbe_transport_wait_completion(packet);

    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);
    if (status != FBE_STATUS_OK ) {
        cms_trace(FBE_TRACE_LEVEL_WARNING,
                       "%s: 0x%x to srv 0x%x, return status:0x%x\n",
                       __FUNCTION__, control_code, FBE_SERVICE_ID_TOPOLOGY, status);  
    }

    fbe_payload_control_get_status(control_operation, &control_status);
    if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK ) {
        cms_trace(FBE_TRACE_LEVEL_WARNING,
                       "%s: 0x%x to srv 0x%x, return payload status:0x%x\n",
                       __FUNCTION__, control_code, FBE_SERVICE_ID_TOPOLOGY, control_status);

        switch (control_status){
        case FBE_PAYLOAD_CONTROL_STATUS_BUSY:
            status = FBE_STATUS_BUSY;
            break;
        case FBE_PAYLOAD_CONTROL_STATUS_INSUFFICIENT_RESOURCES:
            status = FBE_STATUS_INSUFFICIENT_RESOURCES;
            break;
        case FBE_PAYLOAD_CONTROL_STATUS_FAILURE:
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        }
        cms_trace(FBE_TRACE_LEVEL_INFO, "%s: Set return status to 0x%x\n", __FUNCTION__, status);  
    }

    /* free the memory*/
    fbe_payload_ex_release_control_operation(payload, control_operation);

    fbe_transport_release_packet(packet);
    return status;

}

fbe_status_t fbe_cms_wait_for_multicore_queue_empty(fbe_multicore_queue_t *queue_ptr, fbe_u32_t sec_to_wait, fbe_u8_t *queue_name)
{
	fbe_u32_t 		delay_count = 0;
	fbe_cpu_id_t	current_cpu = 0;
	fbe_bool_t		timeout = FBE_FALSE;
	fbe_cpu_id_t	cpu_count = fbe_get_cpu_count();

	for (current_cpu = 0; current_cpu < cpu_count; current_cpu++) {
		
		fbe_multicore_queue_lock(queue_ptr, current_cpu);
		while (!fbe_multicore_queue_is_empty(queue_ptr, current_cpu) && delay_count < (sec_to_wait * 10)) {
			fbe_multicore_queue_unlock(queue_ptr, current_cpu);
			fbe_thread_delay(100);
			if (delay_count == 0) {
				cms_trace (FBE_TRACE_LEVEL_WARNING, "%s %s queue not empty, waiting...\n",__FUNCTION__, queue_name);
			}
			delay_count ++;
			fbe_multicore_queue_lock(queue_ptr, current_cpu);
		}

        if (!fbe_multicore_queue_is_empty(queue_ptr, current_cpu) ){
			fbe_multicore_queue_unlock(queue_ptr, current_cpu);
			cms_trace (FBE_TRACE_LEVEL_ERROR, "%s %s queue still not empty after %d sec !\n",__FUNCTION__, queue_name, sec_to_wait);
			timeout = FBE_TRUE;
		}else{
			fbe_multicore_queue_unlock(queue_ptr, current_cpu);
		}
	}

	if (FBE_IS_TRUE(timeout)) {
		return FBE_STATUS_TIMEOUT;
	}else{
		return FBE_STATUS_OK;
	}

}

/*are we the active or passive SP*/
fbe_bool_t fbe_cms_common_is_active_sp(void)
{
	fbe_cmi_sp_state_t get_state;

	do{
		/*a bit tight but hopefully we should never be in aloop here, but just in case*/
		fbe_cmi_get_current_sp_state(&get_state);
	}while (get_state != FBE_CMI_STATE_ACTIVE && get_state != FBE_CMI_STATE_PASSIVE);

	if (get_state == FBE_CMI_STATE_ACTIVE) {
		return FBE_TRUE;
	}else{
		return FBE_FALSE;
	}
}

