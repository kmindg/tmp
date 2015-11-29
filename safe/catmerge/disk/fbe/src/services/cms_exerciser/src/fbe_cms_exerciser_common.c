/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_cms_exerciser_common.c
***************************************************************************
*
* @brief
*  This file contains csm exerciser service common functions to be used by all files
*  
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_cms_exerciser_private.h"

fbe_status_t cms_exerciser_complete_packet(fbe_packet_t *packet, fbe_status_t packet_status)
{
    fbe_payload_t *						payload = NULL;
    fbe_payload_control_operation_t *	control_operation = NULL;

    payload = fbe_transport_get_payload(packet);
    control_operation = fbe_payload_get_control_operation(payload);
    if ( control_operation != NULL ){
        fbe_payload_control_set_status(control_operation,
                                       packet_status == FBE_STATUS_OK ?
                                       FBE_PAYLOAD_CONTROL_STATUS_OK :
                                       FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }

    fbe_transport_set_status(packet, packet_status, 0);
    return fbe_transport_complete_packet(packet);
}

fbe_status_t cms_exerciser_get_payload(fbe_packet_t *packet, void **payload_struct, fbe_u32_t expected_payload_len)
{
    fbe_payload_t *						payload = NULL;
    fbe_payload_control_operation_t *	control_operation = NULL;
    fbe_u32_t      						buffer_len = 0;

    payload = fbe_transport_get_payload(packet);
    control_operation = fbe_payload_get_control_operation(payload);
    if(control_operation == NULL){
        cms_exerciser_trace(FBE_TRACE_LEVEL_ERROR, "%s Packet 0x%x returns NULL control_operation.\n", __FUNCTION__, packet);
        cms_exerciser_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the payload - if we specified a buffer */
    if ( payload_struct != NULL ){
        fbe_payload_control_get_buffer(control_operation, payload_struct);
        if(*payload_struct == NULL){
            cms_exerciser_trace(FBE_TRACE_LEVEL_ERROR, "%s: Packet 0x%x has NULL payload buffer.\n", __FUNCTION__, packet);
            cms_exerciser_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_payload_control_get_buffer_length(control_operation, &buffer_len);
        if(buffer_len != expected_payload_len){
            cms_exerciser_trace(FBE_TRACE_LEVEL_ERROR, "%s: Invalid len %d != %d(expected) \n", 
                           __FUNCTION__, buffer_len, expected_payload_len);

            cms_exerciser_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return FBE_STATUS_OK;
}

fbe_status_t cms_exerciser_send_packet_to_service(fbe_payload_control_operation_opcode_t control_code,
													  fbe_payload_control_buffer_t buffer,
													  fbe_payload_control_buffer_length_t buffer_length,
													  fbe_service_id_t service_id,
													  fbe_package_id_t package_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet = NULL;
    fbe_payload_t *                     payload = NULL;
    fbe_sep_payload_t *                 sep_payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;

    /* Allocate packet.*/
    packet = fbe_transport_allocate_packet();
    if (packet == NULL) {
        cms_exerciser_trace (FBE_TRACE_LEVEL_ERROR, "%s: unable to allocate packet\n", __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    if (package_id == FBE_PACKAGE_ID_PHYSICAL) {
        fbe_transport_initialize_packet(packet);
        payload = fbe_transport_get_payload (packet);
        control_operation = fbe_payload_allocate_control_operation(payload);
    }else{
        fbe_transport_initialize_sep_packet(packet);
        sep_payload = fbe_transport_get_sep_payload (packet);
        control_operation = fbe_sep_payload_allocate_control_operation(sep_payload);
    }
    
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
        cms_exerciser_trace(FBE_TRACE_LEVEL_ERROR, "%s: error in sending 0x%x to srv 0x%x, status:0x%x\n",
                       __FUNCTION__, control_code, service_id, status);  
    }

    fbe_transport_wait_completion(packet);

    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);
    if (status != FBE_STATUS_OK ) {
        cms_exerciser_trace(FBE_TRACE_LEVEL_WARNING,
                       "%s: 0x%x to srv 0x%x, return status:0x%x\n",
                       __FUNCTION__, control_code, service_id, status);  
    }

    fbe_payload_control_get_status(control_operation, &control_status);
    if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK ) {
        cms_exerciser_trace(FBE_TRACE_LEVEL_WARNING,
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
        cms_exerciser_trace(FBE_TRACE_LEVEL_INFO, "%s: Set return status to 0x%x\n", __FUNCTION__, status);  
    }

    /* free the memory*/
    if (package_id == FBE_PACKAGE_ID_PHYSICAL) {
        fbe_payload_release_control_operation(payload, control_operation);
    }else{
        fbe_sep_payload_release_control_operation(sep_payload, control_operation);
    }

    fbe_transport_release_packet(packet);
    return status;
}
