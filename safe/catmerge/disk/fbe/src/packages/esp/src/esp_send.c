/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved. Licensed material -- property of EMC
 * Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file esp_init.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the ESP code for sending usurper commands from within
 *  the esp package.
 * 
 *  
 * @ingroup esp_init_files
 * 
 * @version
 *   07-July-2010:  Created. D. McFarland
 *
 ***************************************************************************/
                  
/*************************
 *   INCLUDE FILES
 *************************/
#include "csx_ext.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_memory.h"
#include "fbe_transport_memory.h"
#include "fbe_topology.h"
#include "fbe_notification.h"
#include "fbe_scheduler.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_package.h"
#include "esp_class_table.h"
#include "esp_service_table.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe/fbe_api_common.h"

/*!***************************************************************
 * esp_send_log_error()
 **************************************************************** 
 * @brief
 *  ESP trace function to log error messages to the traces with fbe_esp_send_usurper_cmd.
 *
 * @param fmt - Pointer to the format characters
 *
 * @return None
 *
 * @author
 *   07-July-2010:  Created. D. McFarland
 *
 ****************************************************************/
static void esp_send_log_error(const char * fmt, ...)
                               __attribute__((format(__printf_func__,1,2)));
static void esp_send_log_error(const char * fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_PACKAGE,
                     FBE_PACKAGE_ID_ESP,
                     FBE_TRACE_LEVEL_ERROR,
                     0, /* message_id */
                     fmt, 
                     args);
    va_end(args);
}

fbe_status_t fbe_esp_send_control_packet_with_sg (
        fbe_payload_control_operation_opcode_t control_code,
        fbe_payload_control_buffer_t buffer,
        fbe_payload_control_buffer_length_t buffer_length,
        fbe_sg_element_t *sg_ptr,
        fbe_service_id_t service_id,
        fbe_class_id_t class_id,
        fbe_object_id_t object_id,
        fbe_packet_attr_t attr)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                  packet_p = NULL;
    fbe_payload_ex_t *             sep_payload = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;

    packet_p = fbe_transport_allocate_packet();

    if (packet_p == NULL) 
    {
        esp_send_log_error( "%s Null packet_p\n",
                __FUNCTION__ );

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(packet_p);
    sep_payload = fbe_transport_get_payload_ex (packet_p);
    control_operation = fbe_payload_ex_allocate_control_operation(sep_payload);

    fbe_payload_control_build_operation (control_operation,
                                         control_code,
                                         buffer,
                                         buffer_length);

    fbe_payload_ex_set_sg_list(sep_payload, sg_ptr, 2);

    /* Set packet address.*/
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_ESP,
                              service_id,
                              class_id,
                              object_id); 

    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet_p, attr);

    fbe_transport_set_sync_completion_type(packet_p, 
            FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* Control packets should be sent via service manager. */
    status = fbe_service_manager_send_control_packet(packet_p);

    fbe_transport_wait_completion(packet_p);

    fbe_payload_ex_release_control_operation(sep_payload, control_operation);
    fbe_transport_release_packet(packet_p);
    return status;
}


static fbe_status_t fbe_esp_get_object_id_from_class_id(fbe_object_id_t *object_id_p, fbe_class_id_t class_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_topology_control_enumerate_class_t enumerate_class;
    fbe_sg_element_t sg[FBE_TOPLOGY_CONTROL_ENUMERATE_CLASS_SG_COUNT];
    fbe_object_id_t object_id;

    /* We only expect to get back a single sg element for the single object.
     */
    fbe_sg_element_init(&sg[0], sizeof(fbe_object_id_t), &object_id);
    fbe_sg_element_terminate(&sg[1]);

    enumerate_class.class_id = class_id;
    enumerate_class.number_of_objects = 1; /* only expect one object */

    status = fbe_esp_send_control_packet_with_sg (FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_CLASS,
                                                        &enumerate_class,
                                                        sizeof(fbe_topology_control_enumerate_class_t),
                                                        &sg[0],
                                                        FBE_SERVICE_ID_TOPOLOGY,
                                                        FBE_CLASS_ID_INVALID,
                                                        FBE_OBJECT_ID_INVALID,
                                                        FBE_PACKET_FLAG_NO_ATTRIB);
    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        esp_send_log_error( "%s entry; status %d\n",
                __FUNCTION__, status );

        return status;
    }

    /* There is only one object per class in ESP */
    *object_id_p = object_id;

    return status;
}


/*!***************************************************************
 * fbe_esp_send_usurper_cmd()
 **************************************************************** 
 * @brief
 *  ESP function used to send usurper commands from within esp.
 *
 * @param fmt - Pointer to the format characters
 *
 * @return status
 *
 * @author
 *   07-July-2010:  Created. D. McFarland
 *
 ****************************************************************/

fbe_status_t 
fbe_esp_send_usurper_cmd(fbe_payload_control_operation_opcode_t	 control_code,    
                         fbe_class_id_t                          class_id,
                         fbe_payload_control_buffer_t            *buffer,
                         fbe_payload_control_buffer_length_t     buffer_length)  
{
    fbe_packet_t * packet;
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_object_id_t object_id_p;
 
    packet = (fbe_packet_t *) fbe_transport_allocate_packet();

    if (packet == NULL) {
        esp_send_log_error("%s: fbe_transport_allocate_packet returned a null packet pointer\n",
                __FUNCTION__ );
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_transport_initialize_sep_packet(packet);
    sep_payload_p = fbe_transport_get_payload_ex (packet);
    control_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);

    fbe_payload_control_build_operation (control_p,
                                         control_code,
                                         buffer,
                                         buffer_length);

    status = fbe_esp_get_object_id_from_class_id( &object_id_p, class_id );

    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_ESP,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id_p );
                                  
    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    status = fbe_service_manager_send_control_packet(packet);

    fbe_transport_wait_completion(packet);
    fbe_payload_ex_release_control_operation(sep_payload_p, control_p);
    fbe_transport_release_packet(packet);
    return status;
}

