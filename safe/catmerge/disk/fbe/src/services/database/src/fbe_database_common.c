
#include "fbe_database_common.h"
#include "fbe_database_private.h"
#include "fbe_database_config_tables.h"
#include "fbe/fbe_payload_ex.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_bvd_interface.h"
#include "fbe/fbe_cmi_interface.h"
#include "fbe_cmi.h"
#include "fbe_database_drive_connection.h"
#include "fbe_metadata.h"

#include "fbe_hash_table.h"

#include "fbe_notification.h"

#include "fbe/fbe_api_raid_group_interface.h"
#include "fbe/fbe_api_lun_interface.h"
#include "fbe/fbe_api_database_interface.h"
#include "fbe/fbe_api_base_config_interface.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_discovery_transport.h"
#include "fbe/fbe_api_provision_drive_interface.h"
#include "fbe/fbe_api_physical_drive_interface.h"
#include "fbe/fbe_api_virtual_drive_interface.h"

extern fbe_database_service_t fbe_database_service;

static fbe_status_t fbe_database_metadata_nonpaged_preset_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_database_enable_object_lifecycle_trace(fbe_object_id_t object_id);
static fbe_status_t database_export_lun(fbe_object_id_t lun_id);
static fbe_status_t database_forward_packate_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t database_get_source_destination_drive_in_vd(fbe_object_id_t vd_object, fbe_object_id_t* src_pvd, fbe_object_id_t* dest_pvd, fbe_packet_t* packet);

/*!***************************************************************
 * @fn fbe_database_send_control_packet_to_class
 *****************************************************************
 * @brief
 *   send a control packet to a service
 *
 * @param control_code - control code
 * @param buffer - buffer
 * @param buffer_length - buffer length
 * @param service_id - service ID
 * @param sg_list - sg list
 * @param sg_list_count - sg list count
 * @param attr - packet attribute
 * @param package_id - packet ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    5/11/2011: Arun S - created
 *
 ****************************************************************/
fbe_status_t fbe_database_send_control_packet_to_class(fbe_payload_control_buffer_t buffer, 
                                                       fbe_payload_control_buffer_length_t buffer_length, 
                                                       fbe_payload_control_operation_opcode_t opcode, 
                                                       fbe_package_id_t package_id, 
                                                       fbe_service_id_t service_id, 
                                                       fbe_class_id_t class_id, 
                                                       fbe_sg_element_t *sg_list, 
                                                       fbe_u32_t sg_count, 
                                                       fbe_bool_t traverse)
{
    fbe_packet_t * packet;
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;

    /* Allocate packet */
    packet = fbe_transport_allocate_packet();
    if(packet == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                       "%s: fbe_transport_allocate_packet failed\n", 
                       __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(packet);
    sep_payload_p = fbe_transport_get_payload_ex(packet);

    control_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);
    fbe_payload_control_build_operation(control_p, opcode, buffer, buffer_length);

    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* set sg list if one was passed */
    if ( sg_list != NULL )
    {
        fbe_payload_ex_set_sg_list(sep_payload_p, sg_list, sg_count);
    }
    
    /* Set packet address */
    fbe_transport_set_address(packet,
                              package_id,
                              service_id,
                              class_id,
                              FBE_OBJECT_ID_INVALID);

    /* Set traversal flag if set */
    if ( traverse )
    {
        fbe_transport_set_packet_attr(packet, FBE_PACKET_FLAG_TRAVERSE);
    }

    status = fbe_service_manager_send_control_packet(packet);

    fbe_transport_wait_completion(packet);

    /* Get the status of the control operation */
    if ( status == FBE_STATUS_OK )
    {
        fbe_payload_control_status_t control_status;
        
        fbe_payload_control_get_status(control_p, &control_status);

        if ( control_status != FBE_PAYLOAD_CONTROL_STATUS_OK )
        {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }

    fbe_payload_ex_release_control_operation(sep_payload_p, control_p);

    /* retrieve the status of the packet */
    if ( status == FBE_STATUS_OK || status == FBE_STATUS_PENDING )
    {
        status = fbe_transport_get_status_code(packet);
    }

    fbe_transport_release_packet(packet);

    return status;
}

/*!***************************************************************
 * @fn fbe_database_common_send_control_packet_to_service
 *****************************************************************
 * @brief
 *   send a control packet to a service
 *
 * @param control_code - control code
 * @param buffer - buffer
 * @param buffer_length - buffer length
 * @param service_id - service ID
 * @param sg_list - sg list
 * @param sg_list_count - sg list count
 * @param attr - packet attribute
 * @param package_id - packet ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    2/3/2011:   created
 *
 ****************************************************************/
fbe_status_t fbe_database_send_packet_to_service(fbe_payload_control_operation_opcode_t control_code,
                                                 fbe_payload_control_buffer_t buffer,
                                                 fbe_payload_control_buffer_length_t buffer_length,
                                                 fbe_service_id_t service_id,
                                                 fbe_sg_element_t *sg_list,
                                                 fbe_u32_t sg_list_count,
                                                 fbe_packet_attr_t attr,
                                                 fbe_package_id_t package_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                      packet = NULL;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_payload_control_status_qualifier_t status_qualifier;
    fbe_cpu_id_t cpu_id;

    /* Allocate packet.*/
    packet = fbe_transport_allocate_packet();
    if (packet == NULL) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: unable to allocate packet\n", 
                        __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    fbe_transport_initialize_packet(packet);
    payload = fbe_transport_get_payload_ex (packet);
    fbe_payload_ex_allocate_memory_operation(payload);

    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    if (sg_list != NULL) {    
        fbe_payload_ex_set_sg_list (payload, sg_list, sg_list_count);
    }

    cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(packet, cpu_id);

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
    fbe_transport_set_packet_attr(packet, attr);

    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    status = fbe_service_manager_send_control_packet(packet);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: error in sending 0x%x to srv 0x%x, status:0x%x\n",
                       __FUNCTION__, control_code, service_id, status);  
    }

    fbe_transport_wait_completion(packet);

    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);
    if (status != FBE_STATUS_OK ) {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: 0x%x to srv 0x%x, return status:0x%x\n",
                       __FUNCTION__, control_code, service_id, status);  
    }
    else
    {
        fbe_payload_control_get_status(control_operation, &control_status);
        if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK ) {
            database_trace(FBE_TRACE_LEVEL_WARNING,
                           FBE_TRACE_MESSAGE_ID_INFO,
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
                fbe_payload_control_get_status_qualifier(control_operation, &status_qualifier);
                status = (status_qualifier == FBE_OBJECT_ID_INVALID) ? FBE_STATUS_NO_OBJECT: FBE_STATUS_GENERIC_FAILURE;
                break;
            default:
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
            }
            database_trace(FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: Set return status to 0x%x\n",
                           __FUNCTION__, status);  
        }
    }

    /* free the memory*/
    fbe_payload_ex_release_control_operation(payload, control_operation);
    fbe_payload_ex_release_memory_operation(payload, payload->payload_memory_operation);
    fbe_transport_release_packet(packet);
    return status;
}

/*!***************************************************************
 * @fn fbe_database_send_packet_to_service_asynch
 *****************************************************************
 * @brief
 *   send a control packet to a service asynchronisely.
 *   The caller needs to allocate the packet and provide
 *   the completion function.  The completion_function needs
 *   to free the packet.
 *
 * @param packet_p - pointer to the packet structure,
 *                   which allocated by the caller
 *                   and freed by the completion function
 * @param control_code - control code
 * @param buffer - buffer
 * @param buffer_length - buffer length
 * @param service_id - service ID
 * @param sg_list - sg list
 * @param sg_list_count - sg list count
 * @param attr - packet attribute
 * @param completion_function - completion function
 * @param completion_context - completion context
 * @param package_id - packet ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    2/3/2011:   created
 *
 ****************************************************************/
fbe_status_t fbe_database_send_packet_to_service_asynch(fbe_packet_t *packet,
                                                        fbe_payload_control_operation_opcode_t control_code,
                                                        fbe_payload_control_buffer_t buffer,
                                                        fbe_payload_control_buffer_length_t buffer_length,
                                                        fbe_service_id_t service_id,
                                                        fbe_sg_element_t *sg_list,
                                                        fbe_u32_t sg_list_count,
                                                        fbe_packet_attr_t attr,
                                                        fbe_packet_completion_function_t completion_function,
                                                        fbe_packet_completion_context_t completion_context,
                                                        fbe_package_id_t package_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_cpu_id_t cpu_id;
 
    fbe_transport_initialize_packet(packet);
    payload = fbe_transport_get_payload_ex (packet);
    fbe_payload_ex_allocate_memory_operation(payload);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    if (sg_list != NULL) {
        fbe_payload_ex_set_sg_list (payload, sg_list, sg_list_count);
    }

    cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(packet, cpu_id);

    fbe_transport_set_completion_function(packet, completion_function,  completion_context);

    fbe_payload_control_build_operation(control_operation,
                                        control_code,
                                        buffer,
                                        buffer_length);
    
    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              package_id,
                              service_id,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID); 
    
    /* Mark packet with the attribute, either traverse or not.*/
    fbe_transport_set_packet_attr(packet, attr);
    database_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "send pkt_p 0x%p to service 0x%x in pkg0x%x\n",
                   packet, service_id, package_id);  
    status = fbe_service_manager_send_control_packet(packet);
    if (status != FBE_STATUS_PENDING) {
        database_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: send_control_packet to srv 0x%x, status:0x%x\n",
                       __FUNCTION__, service_id, status);  
    }
    return status;
}


/*!***************************************************************
 * @fn fbe_database_send_packet_to_object
 *****************************************************************
 * @brief
 *   send a control packet to a service
 *
 * @param control_code - control code
 * @param buffer - buffer
 * @param buffer_length - buffer length
 * @param class_id - class id of the object
 * @param object_id - object id
 * @param sg_list - sg list
 * @param sg_list_count - sg list count
 * @param attr - packet attribute
 * @param package_id - packet ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    2/18/2011:   created
 *
 ****************************************************************/
fbe_status_t fbe_database_send_packet_to_object(fbe_payload_control_operation_opcode_t control_code,
                                                fbe_payload_control_buffer_t buffer,
                                                fbe_payload_control_buffer_length_t buffer_length,
                                                fbe_object_id_t object_id,
                                                fbe_sg_element_t *sg_list,
                                                fbe_u32_t sg_list_count,
                                                fbe_packet_attr_t attr,
                                                fbe_package_id_t package_id)
{
        fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE, send_status;
    fbe_packet_t *                      packet = NULL;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_ex_t *                 sep_payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_cpu_id_t  cpu_id;		

    /* Allocate packet.*/
    packet = fbe_transport_allocate_packet();
    if (packet == NULL) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: unable to allocate packet\n", 
                        __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
 
    if (package_id == FBE_PACKAGE_ID_PHYSICAL) {
        fbe_transport_initialize_packet(packet);
        payload = fbe_transport_get_payload_ex (packet);
        control_operation = fbe_payload_ex_allocate_control_operation(payload);
        if (sg_list != NULL)
        {
            /*set sg list*/
            fbe_payload_ex_set_sg_list (payload, sg_list, sg_list_count);
        }
    }else{
        fbe_transport_initialize_sep_packet(packet);
        sep_payload = fbe_transport_get_payload_ex (packet);
        control_operation = fbe_payload_ex_allocate_control_operation(sep_payload);
        if (sg_list != NULL)
        {
            /*set sg list*/
            fbe_payload_ex_set_sg_list (sep_payload, sg_list, sg_list_count);
        }
    }
    
    cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(packet, cpu_id);
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
    
    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet, attr);

    fbe_transport_set_sync_completion_type(packet, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    send_status = fbe_service_manager_send_control_packet(packet);
    if ((send_status != FBE_STATUS_OK)&&(send_status != FBE_STATUS_NO_OBJECT) && (send_status != FBE_STATUS_PENDING) && (send_status != FBE_STATUS_MORE_PROCESSING_REQUIRED)) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: error in sending 0x%x to obj 0x%x, status:0x%x\n",
                       __FUNCTION__, control_code, object_id, send_status);  
    }

    fbe_transport_wait_completion(packet);

    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);
    database_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: 0x%x to obj 0x%x, packet status:0x%x\n",
                   __FUNCTION__, control_code, object_id, status);  

    fbe_payload_control_get_status(control_operation, &control_status);
    if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK ) {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: 0x%x to obj 0x%x, return payload status:0x%x\n",
                       __FUNCTION__, control_code, object_id, control_status);

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
        database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Set return status to 0x%x\n",
                       __FUNCTION__, status);  
    }

    /* free the memory*/
    if (package_id == FBE_PACKAGE_ID_PHYSICAL) {
        fbe_payload_ex_release_control_operation(payload, control_operation);
    }else{
        fbe_payload_ex_release_control_operation(sep_payload, control_operation);
    }
    fbe_transport_release_packet(packet);
    return status;
}

/*!***************************************************************
 * @fn fbe_database_send_packet_to_object_async
 *****************************************************************
 * @brief
 *   send a control packet to an object asynchronisely.
 *   The caller needs to allocate the packet and provide
 *   the completion function.  The completion_function needs
 *   to free the packet.
 *
 * @param packet_p - pointer to the packet structure,
 *                   which allocated by the caller
 *                   and freed by the completion function
 * @param control_code - control code
 * @param buffer - buffer
 * @param buffer_length - buffer length
 * @param object_id - Object ID
 * @param sg_list - sg list
 * @param sg_list_count - sg list count
 * @param attr - packet attribute
 * @param completion_function - completion function
 * @param completion_context - completion context
 * @param package_id - packet ID
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    10/3/2013:   Ashok Tamilarasan - created
 *
 ****************************************************************/
fbe_status_t fbe_database_send_packet_to_object_async(fbe_packet_t *packet,
                                                        fbe_payload_control_operation_opcode_t control_code,
                                                        fbe_payload_control_buffer_t buffer,
                                                        fbe_payload_control_buffer_length_t buffer_length,
                                                        fbe_object_id_t object_id,
                                                        fbe_sg_element_t *sg_list,
                                                        fbe_u32_t sg_list_count,
                                                        fbe_packet_attr_t attr,
                                                        fbe_packet_completion_function_t completion_function,
                                                        fbe_packet_completion_context_t completion_context,
                                                        fbe_package_id_t package_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *                     payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_cpu_id_t cpu_id;
 
    fbe_transport_initialize_packet(packet);
    payload = fbe_transport_get_payload_ex (packet);
    fbe_payload_ex_allocate_memory_operation(payload);
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    if (sg_list != NULL) {
        fbe_payload_ex_set_sg_list (payload, sg_list, sg_list_count);
    }

    cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(packet, cpu_id);

    fbe_transport_set_completion_function(packet, completion_function,  completion_context);

    fbe_payload_control_build_operation(control_operation,
                                        control_code,
                                        buffer,
                                        buffer_length);
    
    /* Set packet address.*/
    fbe_transport_set_address(packet,
                              package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id); 

    /* Mark packet with the attribute, either traverse or not.*/
    fbe_transport_set_packet_attr(packet, attr);
    database_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "send pkt_p 0x%p to Object 0x%x in pkg0x%x\n",
                   packet, object_id, package_id);  
    status = fbe_service_manager_send_control_packet(packet);
    if (status != FBE_STATUS_PENDING) {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: send_control_packet to Object 0x%x, status:0x%x\n",
                       __FUNCTION__, object_id, status);  
    }
    return status;
}


/*!***************************************************************
 * @fn fbe_database_get_payload
 *****************************************************************
 * @brief
 *   Helper function to get the payload buffer from the given packet
 *   and validate the payload length.
 *
 * @param packet - packet
 * @param payload_struct - pointer to the payload
 * @param expected_payload_len - the size of the payload buffer
 *                  that the caller expects to get
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    2/18/2011:   created
 *
 ****************************************************************/
fbe_status_t fbe_database_get_payload_impl(fbe_packet_t *packet, 
                                      void **payload_struct, 
                                      fbe_u32_t expected_payload_len)
{
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_u32_t      buffer_len;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    if(control_operation == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s Packet 0x%p returns NULL control_operation.\n", __FUNCTION__, packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the payload - if we specified a buffer */
    if ( payload_struct != NULL )
    {
        fbe_payload_control_get_buffer(control_operation, payload_struct);
        if(*payload_struct == NULL)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: Packet 0x%p has NULL payload buffer.\n", __FUNCTION__, packet);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        fbe_payload_control_get_buffer_length(control_operation, &buffer_len);
        if(buffer_len != expected_payload_len)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: Invalid len %d != %d(expected) \n", 
                           __FUNCTION__, buffer_len, expected_payload_len);

            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_complete_packet
 *****************************************************************
 * @brief
 *   Helper function to set the packet status and complete the packet
 *
 * @param packet - packet
 * @param packet_status - status to set for the packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    2/18/2011:   created
 *
 ****************************************************************/
fbe_status_t fbe_database_complete_packet(fbe_packet_t *packet, fbe_status_t packet_status)
{
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    if ( control_operation != NULL )
    {
        fbe_payload_control_set_status(control_operation,
                                       packet_status == FBE_STATUS_OK ?
                                       FBE_PAYLOAD_CONTROL_STATUS_OK :
                                       FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }

    fbe_transport_set_status(packet, packet_status, 0);
    return fbe_transport_complete_packet(packet);
}

/*!***************************************************************
 * @fn fbe_database_create_object
 *****************************************************************
 * @brief
 *   Create an object for the given class.
 *   If object_id is FBE_OBJECT_ID_INVALID, a valid id will return.
 *
 * @param class_id - class type in which object will be created
 * @param object_id -
 * if !=FBE_OBJECT_ID_INVALID, an object with given id will be created(IN)
 * if ==FBE_OBJECT_ID_INVALID, a valid object id will be return(OUT)
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    2/18/2011:   created
 *
 ****************************************************************/
fbe_status_t fbe_database_create_object(fbe_class_id_t class_id, fbe_object_id_t *object_id)
{
    fbe_base_object_mgmt_create_object_t base_object_mgmt_create_object;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    fbe_const_class_info_t * p_class_info;
    const char * p_class_name;

    /* populate the struture first */
    fbe_base_object_mgmt_create_object_init(&base_object_mgmt_create_object,
                                            class_id,
                                            *object_id);
    /* Ask topology to create the object*/
    status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_CREATE_OBJECT,
                                                 &base_object_mgmt_create_object,
                                                 sizeof(fbe_base_object_mgmt_create_object_t),
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 NULL,  /* no sg list*/
                                                 0,  /* no sg list*/
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 FBE_PACKAGE_ID_SEP_0);
    /* We need to deal with the failure here */
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: create object failed. class id :0x%X, obj_id: 0x%x, status: 0x%x\n",
                       __FUNCTION__, class_id, *object_id, status);
        return status;
    }
    /* copy the object_id for returning to caller*/
    *object_id = base_object_mgmt_create_object.object_id;

    status = fbe_get_class_by_id(class_id, &p_class_info);
    p_class_name = (status == FBE_STATUS_OK) ? p_class_info->class_name : "?";
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: class_id: 0x%X:%s, Obj_id 0x%x, Status 0x%x\n", 
                   __FUNCTION__, class_id, p_class_name, *object_id, status);

    return status;
}

/*!***************************************************************
 * @fn fbe_database_destroy_object
 *****************************************************************
 * @brief
 *   Destroy the object with given object id.
 *
 * @param object_id -
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    2/18/2011:   created
 *
 ****************************************************************/
fbe_status_t fbe_database_destroy_object(fbe_object_id_t object_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: Object id = 0x%x\n", 
                   __FUNCTION__, object_id);

    /* tell the object to destory itself */
    status = fbe_database_send_packet_to_object(FBE_BASE_OBJECT_CONTROL_CODE_SET_DESTROY_COND,
                                                NULL, /* No payload for this control code*/
                                                0,  /* No payload for this control code*/
                                                object_id, 
                                                NULL,  /* no sg list*/
                                                0,  /* no sg list*/
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);

    database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: Obj_id 0x%x, Return Status 0x%x\n", 
                   __FUNCTION__, object_id, status);

    return status;
}
/*!*************************************************************************************
 * @fn fbe_database_commit_object
 ***************************************************************************************
 * @brief
 *   Commit the object with given object id.
 *
 * @param object_id     - the object id that we want to commit
 * @param db_class_id   - the database class id for the object
 * @param is_initial_commit - the object is the very first time to commit or not
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    2/18/2011:   created
 *
 **************************************************************************************/
fbe_status_t fbe_database_commit_object(fbe_object_id_t object_id, database_class_id_t db_class_id, fbe_bool_t is_initial_commit)
{
    fbe_status_t                           	status;
    fbe_base_config_commit_configuration_t 	commit;
    fbe_class_id_t fbe_class_id;
#if 0
    /* debug object lifecycle */
    fbe_database_enable_object_lifecycle_trace(object_id);
#endif
    commit.is_configuration_commited = FBE_TRUE;
    commit.is_initial_commit = is_initial_commit;
    status = fbe_database_send_packet_to_object(FBE_BASE_CONFIG_CONTROL_CODE_COMMIT_CONFIGURATION,
                                                &commit, 
                                                sizeof(commit),  
                                                object_id, 
                                                NULL,  /* no sg list*/
                                                0,  /* no sg list*/
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: Obj_id 0x%x, Status 0x%x\n", 
                   __FUNCTION__, object_id, status);

    /*for LUNs, we also export them(if needed) at this stage.
    This way, once SEP driver is finisehd loading, All luns are ready to at least get opened by upper layers.
    They may not be in READY state which is fine, but we MUST have the device there so upper layers that persisted the fact the lun is there
    will be able to talk to it*/
    fbe_class_id = database_common_map_class_id_database_to_fbe(db_class_id);
    if ((fbe_class_id == FBE_CLASS_ID_LUN) || (fbe_class_id == FBE_CLASS_ID_EXTENT_POOL_LUN)) {
        database_export_lun(object_id);
    }

    return status;
}

static fbe_status_t fbe_database_enable_object_lifecycle_trace(fbe_object_id_t object_id)
{
    fbe_status_t              status;
    fbe_trace_level_control_t flag_control;

    flag_control.trace_type = FBE_TRACE_TYPE_MGMT_ATTRIBUTE;
    flag_control.fbe_id = object_id;
    flag_control.trace_level = FBE_TRACE_LEVEL_INFO;
    flag_control.trace_flag = FBE_BASE_OBJECT_FLAG_ENABLE_LIFECYCLE_TRACE;

    status = fbe_database_send_packet_to_service(FBE_TRACE_CONTROL_CODE_SET_TRACE_FLAG,
                                                 &flag_control,
                                                 sizeof(fbe_trace_level_control_t),
                                                 FBE_SERVICE_ID_TRACE,
                                                 NULL, /* no sg list*/
                                                 0,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: Obj_id 0x%x, Status 0x%x\n", 
                   __FUNCTION__, object_id, status);
    }
    return status;
}

/*!***************************************************************************
 *          fbe_database_validate_object()
 *****************************************************************************
 *
 * @brief   Simply validate that the object exist or not.
 *
 * @param   src_object_id - Object id to validate
 *
 * @return fbe_status_t -   FBE_STATUS_OK  - Object exist
 *                              or
 *                          FBE_STATUS_NO_OBJECT - Object doesn't exist
 *                              or
 *                          FBE_STATUS_GENERIC_FAILURE
 *
 * @version
 *  12/02/2011  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_database_validate_object(fbe_object_id_t src_object_id)
{
    fbe_status_t                                status;
    /*Variables for different table entries associate with src & des object*/
    database_object_entry_t                     *src_object_entry_ptr = NULL;
    fbe_base_object_mgmt_get_lifecycle_state_t  get_lifecycle;

    /* Validate destination object does not exist.
     * Get the des object lifecycle state based on des object ID.
     */
    get_lifecycle.lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    status = fbe_database_send_packet_to_object(FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
                                                &get_lifecycle,
                                                sizeof(fbe_base_object_mgmt_get_lifecycle_state_t),
                                                src_object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK && status != FBE_STATUS_NO_OBJECT)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: unable to get object: 0x%x lifecycle state\n", 
                        __FUNCTION__, src_object_id); 
        return FBE_STATUS_GENERIC_FAILURE;
    } 
 
    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: object: 0x%x lifecycle: %d status: %d\n", 
                   __FUNCTION__, src_object_id, get_lifecycle.lifecycle_state, status);  
             
    /*Confirm NO valid entries associate with destination object.*/
    if (status == FBE_STATUS_NO_OBJECT)
    {
        /* Only report a warning.  It is up to the caller to take the neccessary action.
         */
        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                  src_object_id,
                                                                  &src_object_entry_ptr);
        if (status == FBE_STATUS_OK || src_object_entry_ptr != NULL) 
        {
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: get object: 0x%x entry success not expected.\n", 
                           __FUNCTION__, src_object_id);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    /* Return the object status.
     */
    return status;
}
/****************************************
 * end of fbe_database_validate_object()
 ****************************************/

/*!***************************************************************
 * @fn fbe_database_clone_object
 *****************************************************************
 * @brief
 *   Clone the source object database entries to the destination 
 * object with given src & des object id. Creates the destination
 * object using all the config from the source object
 *
 * @param src_object_id
 * @param des_object_id
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    9/22/2011: Jian.G   created
 *
 ****************************************************************/
fbe_status_t fbe_database_clone_object(fbe_object_id_t src_object_id,
                                       fbe_object_id_t *des_object_id)
{
    fbe_status_t                                status;

    /*Variables for different table entries associate with src & des object*/
    database_object_entry_t                     *src_object_entry_ptr = NULL;
    database_object_entry_t                     *des_object_entry_ptr = NULL;
    database_object_entry_t                     *obj_table_ptr = NULL;
    database_user_entry_t                       *user_table_ptr = NULL;
    database_object_entry_t                     des_object_entry;
    database_edge_entry_t                       *src_object_edge_entry_ptr = NULL;
    database_edge_entry_t                       *des_object_edge_entry_ptr = NULL;
    database_edge_entry_t                       des_object_edge_entry[DATABASE_TRANSACTION_MAX_EDGES];
    database_user_entry_t                       *src_object_user_entry_ptr = NULL;
    database_user_entry_t                       *des_object_user_entry_ptr = NULL;
    database_user_entry_t                       des_object_user_entry;
    
    /*Loop index for scaning edge entries associate with src object*/
    fbe_u32_t                                   edge_i = 0;

    fbe_class_id_t                              class_id;
    fbe_base_object_mgmt_get_lifecycle_state_t  get_lifecycle;

    fbe_config_generation_t     des_object_generation_number;

    if(NULL == des_object_id)
        return FBE_STATUS_GENERIC_FAILURE;

    /*if the dest object id is INVALID, it means database service should assign a valid object id*/
    if(FBE_OBJECT_ID_INVALID == *des_object_id)
    {
        fbe_spinlock_lock(&fbe_database_service.object_table.table_lock);
        status = fbe_database_config_table_get_non_reserved_free_object_id(
                        fbe_database_service.object_table.table_content.object_entry_ptr, 
                        fbe_database_service.object_table.table_size, des_object_id);
        fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);
        if(FBE_STATUS_OK != status)
        {
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s: unable to allocate a valid dest object id\n", 
                            __FUNCTION__); 
            return status;
        }
    }

    /* Validate destination object does not exist.
     * Get the des object lifecycle state based on des object ID.
     */
    status = fbe_database_send_packet_to_object(FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
                                                &get_lifecycle,
                                                sizeof(fbe_base_object_mgmt_get_lifecycle_state_t),
                                                *des_object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK && status != FBE_STATUS_NO_OBJECT)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: unable to get object lifecycle state\n", 
                        __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    } 
        
    /*Check whether des object is existing or not*/
    if (status == FBE_STATUS_OK && get_lifecycle.lifecycle_state != FBE_LIFECYCLE_STATE_NOT_EXIST)
    {
        /*destination object exists*/
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: the destination object exists\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*Confirm NO valid entries associate with destination object.*/
    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              *des_object_id,
                                                              &des_object_entry_ptr);
    if (status != FBE_STATUS_OK || des_object_entry_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: get destination object entry failed!\n", 
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* !DATABASE_CONFIG_ENTRY_INVALID shows there is valid object entries for
     * the destination object...
     */
    if (des_object_entry_ptr->header.state != DATABASE_CONFIG_ENTRY_INVALID) {
        /*There is valid entries associated with destination object*/
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: destination object valid entries exist!\n", 
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Copies all valid table entries that associate with the source object 
     * to the destination object's entries. Make sure the destination object
     * entries has the object id of the destination object and its original 
     * entry id.
     *
     * Total 3 steps to copy all the valid entries associate with the object
     *      1. Copy object table entry.
     *      2. Copy user table entry.
     *      3. Copy edge table entry.
     */

    /* Before copy object, user, edge entries, we point each pointers to
     * the 'orphan' entries which declared at the beginning of the function.
     * Then commit these 'orphan' entries into in-memory database via corresponding
     * database transaction add entry interface...
     */
    des_object_entry_ptr        = &des_object_entry;
    des_object_user_entry_ptr   = &des_object_user_entry;
    des_object_edge_entry_ptr   = des_object_edge_entry;


    /*1.Copy object table entry.
        Get src_object_entry. */
    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              src_object_id,
                                                              &src_object_entry_ptr);
    if (status != FBE_STATUS_OK || src_object_entry_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: get source object entry failed!\n", 
                       __FUNCTION__);
        return FBE_STATUS_NO_OBJECT;
    }
    /* Check whether src object has valid entries... */
    if (src_object_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_INVALID) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: source object valid entries do NOT exist!\n", 
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set header.object_id as des_object_id here...
     * header.state and entry_id will be taken care by db service commit
     * transaction function..
     * Assign a new generation number to des object.
     */
    
    /* THIS IS MISUNDERSTANDING OF ENTRY HEADER PROCESSING BEFORE!!
     * Store the orginal des_object_entry header, before clone src object entry
     * to des object entry. And then restore the des object entry header with 
     * original value.
     * des_object_entry_state would be set as DATABASE_CONFIG_ENTRY_VALID manually
     * des_object_id is aready there(des_object_id)...
     */
    des_object_generation_number = fbe_database_transaction_get_next_generation_id(&fbe_database_service);
    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: Active side: Assigned generation number: 0x%llx for Object, 0x%x.\n",
                   __FUNCTION__, 
                   (unsigned long long)des_object_generation_number,
           *des_object_id);

    fbe_copy_memory(des_object_entry_ptr, src_object_entry_ptr, sizeof(database_object_entry_t));

    /* Get object class id */
    class_id = database_common_map_class_id_database_to_fbe(des_object_entry_ptr->db_class_id);
    /* Restore the des object entry header.object_id. */
    des_object_entry_ptr->header.object_id   = *des_object_id;
    des_object_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_VALID;
   
    /* Restore the des object entry generation number. */
    switch (class_id) {
        case FBE_CLASS_ID_LUN:
            des_object_entry_ptr->set_config_union.lu_config.generation_number = des_object_generation_number;
            break;
        case FBE_CLASS_ID_VIRTUAL_DRIVE:
            des_object_entry_ptr->set_config_union.vd_config.generation_number = des_object_generation_number;
            break;
        case FBE_CLASS_ID_PROVISION_DRIVE:
            des_object_entry_ptr->set_config_union.pvd_config.generation_number = des_object_generation_number;
            break;
        case FBE_CLASS_ID_STRIPER:
        case FBE_CLASS_ID_MIRROR:
        case FBE_CLASS_ID_PARITY:
            des_object_entry_ptr->set_config_union.rg_config.generation_number = des_object_generation_number;
            break;
    }

     /*
     we need update the database table here. It is a hack for system drive clone, because
     some code in the swap job path will use the entry in the database table, rather than
     the transaction table. It is not safe and we will omit the hack when working out a good
     approach to synchronize the access between database table and transaction table
     */
    if (fbe_private_space_layout_object_id_is_system_spare_pvd(*des_object_id)
        || fbe_private_space_layout_object_id_is_system_spare_pvd(src_object_id))
    {
        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                  *des_object_id,
                                                                  &obj_table_ptr);
        if (status != FBE_STATUS_OK || obj_table_ptr == NULL) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: get destination object entry failed!\n", 
                           __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        if (obj_table_ptr != NULL) {
            fbe_copy_memory(obj_table_ptr, des_object_entry_ptr, sizeof (database_object_entry_t));
        }
    }

    /*each entry added to the transaction table should have header state set to 
      DATABASE_CONFIG_ENTRY_CREATE, DATABASE_CONFIG_ENTRY_DELETE or DATABASE_CONFIG_ENTRY_MODIFY*/
    des_object_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_CREATE;


    /*2.Copy user table entry.*/
    /* Get src_object_user_entry. */
    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                                   src_object_id,
                                                                   &src_object_user_entry_ptr);
    if (status != FBE_STATUS_OK || src_object_user_entry_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: get source object user entry failed!\n", 
                       __FUNCTION__);
        status = FBE_STATUS_NO_OBJECT;
        return status;
    }

    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                                   *des_object_id,
                                                                   &user_table_ptr);
    if (status != FBE_STATUS_OK || user_table_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: get source object user entry failed!\n", 
                       __FUNCTION__);
        status = FBE_STATUS_NO_OBJECT;
        return status;
    }

    fbe_copy_memory(des_object_user_entry_ptr, src_object_user_entry_ptr, sizeof(database_user_entry_t));

    /* Restore the des object entry header.object_id. */
    des_object_user_entry_ptr->header.object_id   = *des_object_id;
    des_object_user_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_VALID;

    if (fbe_private_space_layout_object_id_is_system_spare_pvd(*des_object_id)
        || fbe_private_space_layout_object_id_is_system_spare_pvd(src_object_id)) 
    {
        /*same hack for database drive clone as the object entry*/
        if (user_table_ptr != NULL) {
            fbe_copy_memory(user_table_ptr, des_object_user_entry_ptr, sizeof (database_user_entry_t));
        }
    }

    /*each entry added to the transaction table should have header state set to 
      DATABASE_CONFIG_ENTRY_CREATE, DATABASE_CONFIG_ENTRY_DELETE or DATABASE_CONFIG_ENTRY_MODIFY*/
    des_object_user_entry_ptr->header.state = DATABASE_CONFIG_ENTRY_CREATE;

    /* 3.Copy edge table entry*/
    /* Scaning all edge entries associate with source object and clone entries to
     * destination object edge table.
     */
    for (edge_i = 0; edge_i < DATABASE_MAX_EDGE_PER_OBJECT; edge_i++)
    {
       status = fbe_database_config_table_get_edge_entry(&fbe_database_service.edge_table,
                                                         src_object_id,
                                                         edge_i,
                                                         &src_object_edge_entry_ptr);
       if (status != FBE_STATUS_OK) {
           database_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: get source object edge entry failed!\n", 
                          __FUNCTION__);
       }
       else
       {
           /* Copy edge_i entry to des_object_edge_entry. */
           des_object_edge_entry_ptr[edge_i] = *src_object_edge_entry_ptr;
           /* Restore the des_object_edge_entry[edge_i]'s header.object_id
            * the entry_id and state will be taken care by db trasaction add
            * edge entries function
            */
            /* Return from get edge loop when src_object_edge_entry_ptr->header.state == 
             * DATABASE_CONFIG_ENTRY_INVALID. */
           if (src_object_edge_entry_ptr->header.state != DATABASE_CONFIG_ENTRY_INVALID) {
               des_object_edge_entry_ptr[edge_i].header.object_id   = *des_object_id;
               des_object_edge_entry_ptr[edge_i].header.state = DATABASE_CONFIG_ENTRY_CREATE;
           }
       }
    }

    /* Clone object nonpaged metadata.*/
    status = database_common_clone_object_nonpaged_metadata_data(src_object_id,
                                                                 *des_object_id,
                                                                 des_object_generation_number);
    if (status != FBE_STATUS_OK) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: Failed to clone object nonpaged metadata\n", 
                        __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Create the destination object using all the config from the source object. */
    status = fbe_database_create_object(class_id, des_object_id);
    if (status != FBE_STATUS_OK) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: Failed to create object with object_id %d\n", 
                        __FUNCTION__, *des_object_id); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*Set the config to the object via database common interface*/
    status = database_common_set_config_from_table_entry(des_object_entry_ptr);


    database_trace (FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: create object of fbe_class 0x%x, obj_id 0x%x!\n",
                   __FUNCTION__, class_id, *des_object_id);


    /* Commit the object entries into transaction entry list,
     * then when the transaction is commited in job service, all those
     * entries will be commited to in-memory database and will be
     * persisted to disk.
     */
    status = fbe_database_transaction_add_object_entry(fbe_database_service.transaction_ptr, des_object_entry_ptr);
    if (status != FBE_STATUS_OK) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: Failed to add object entry into transaction object entry table\n", 
                        __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_database_transaction_add_user_entry(fbe_database_service.transaction_ptr, des_object_user_entry_ptr);
    if (status != FBE_STATUS_OK) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: Failed to add object user entry into transaction object user entry table\n", 
                        __FUNCTION__); 
        return FBE_STATUS_GENERIC_FAILURE;
    }


    for (edge_i = 0; edge_i < DATABASE_MAX_EDGE_PER_OBJECT; edge_i++)
    {
        if (des_object_edge_entry_ptr[edge_i].header.state != DATABASE_CONFIG_ENTRY_INVALID) {
            status = fbe_database_transaction_add_edge_entry(fbe_database_service.transaction_ptr,
                                                            des_object_edge_entry_ptr + edge_i);
            if (status != FBE_STATUS_OK) {
                database_trace (FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s: Failed to add object edge entry into transaction object edge entry table\n", 
                                __FUNCTION__); 
                return FBE_STATUS_GENERIC_FAILURE;
            }
        }
    }
    
    return status;
}

/*!***************************************************************
 * @fn fbe_database_create_edge
 *****************************************************************
 * @brief
 *   Create an edge.
 *
 * @param object_id - client
 * @param create_edge - create edge struct, which decribes the edge to be created
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    2/18/2011:   created
 *
 ****************************************************************/
fbe_status_t fbe_database_create_edge(fbe_object_id_t object_id, 
                                      fbe_object_id_t   server_id,       
                                      fbe_edge_index_t client_index,    
                                      fbe_lba_t capacity,
                                      fbe_lba_t offset,
                                      fbe_edge_flags_t edge_flags)
{
    fbe_block_transport_control_create_edge_t create_edge;
    fbe_status_t status;

    create_edge.server_id = server_id;
    create_edge.client_index = client_index;
    create_edge.capacity = capacity;
    create_edge.offset = offset;
    create_edge.edge_flags = edge_flags;

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                   "create_edge: client:0x%x server:0x%x(ind:0x%x) offset:0x%llx capacity:0x%llx flags: 0x%x\n",
                   object_id, create_edge.server_id, create_edge.client_index,
                   (unsigned long long)create_edge.offset, (unsigned long long)create_edge.capacity, create_edge.edge_flags);

    status = fbe_database_send_packet_to_object(FBE_BLOCK_TRANSPORT_CONTROL_CODE_CREATE_EDGE,
                                                &create_edge, 
                                                sizeof(fbe_block_transport_control_create_edge_t),
                                                object_id, 
                                                NULL,  /* no sg list*/
                                                0,  /* no sg list*/
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to create edge between client:0x%x server:0x%x(ind:0x%x)\n", 
                       __FUNCTION__, object_id, create_edge.server_id, create_edge.client_index);
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_database_destroy_edge
 *****************************************************************
 * @brief
 *   destroy an edge.
 *
 * @param client_id - client
 * @param destroy_edge - destroy edge struct, which decribes the edge
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    2/18/2011:   created
 *
 ****************************************************************/
fbe_status_t fbe_database_destroy_edge(fbe_object_id_t client_id, fbe_edge_index_t  client_index)
{
    fbe_status_t                               status;
    fbe_block_transport_control_destroy_edge_t destroy_edge;

    database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: client:0x%x client_index:0x%x.\n", __FUNCTION__,
                   client_id, client_index);

    destroy_edge.client_index = client_index;
    status = fbe_database_send_packet_to_object(FBE_BLOCK_TRANSPORT_CONTROL_CODE_DESTROY_EDGE,
                                                &destroy_edge, 
                                                sizeof(fbe_block_transport_control_destroy_edge_t),
                                                client_id, 
                                                NULL,  /* no sg list*/
                                                0,  /* no sg list*/
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if ( status == FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Disconnected client:0x%x client_index:0x%x.\n", 
                       __FUNCTION__, client_id, client_index);
    }
    else
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: Failed to disconnect client:0x%x client_index:0x%x.\n", 
                       __FUNCTION__, client_id, client_index);
    }
    
    return status;
}

static void 
fbe_database_set_pvd_config_adjust_cap_limit(fbe_object_id_t  object_id, fbe_provision_drive_configuration_t *config)
{
    fbe_u32_t cap_limit = 0;
    fbe_lba_t new_capacity;

    /* Capacity limit for user drives only */
    if (object_id < FBE_RESERVED_OBJECT_IDS)
    {
        return;
    }

    fbe_database_get_user_capacity_limit(&cap_limit);
    if (cap_limit == 0)
    {
        return;
    }

    new_capacity = (((fbe_u64_t)cap_limit) * 1024 * 1024 * 1024)/512;
    if (new_capacity && (new_capacity < config->configured_capacity))
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s obj 0x%x setting cap from 0x%llx to 0x%llx\n",
                   __FUNCTION__, object_id, config->configured_capacity, new_capacity);       
        config->configured_capacity = new_capacity;
    }
    
    return;
}

/*!***************************************************************
 * @fn fbe_database_set_pvd_config
 *****************************************************************
 * @brief
 *   set pvd config .
 *
 * @param object_id - the pvd to set the config
 * @param config - the config
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    4/12/2011:   created
 *
 ****************************************************************/
fbe_status_t fbe_database_set_pvd_config(fbe_object_id_t  object_id, fbe_provision_drive_configuration_t *config)
{
    fbe_status_t status;
    fbe_provision_drive_set_configuration_t set_config;

    fbe_copy_memory(&set_config.db_config, config, sizeof(fbe_provision_drive_configuration_t));
    set_config.user_capacity = config->configured_capacity;
    fbe_database_set_pvd_config_adjust_cap_limit(object_id, &set_config.db_config);

    /* Configure the raid group portion of the virtual drive. */
    status = fbe_database_send_packet_to_object(FBE_PROVISION_DRIVE_CONTROL_CODE_SET_CONFIGURATION,
                                                &set_config,
                                                sizeof(fbe_provision_drive_set_configuration_t),
                                                object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if(status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s Failed to send config to pvd object 0x%x\n",
                   __FUNCTION__, object_id);       
    }
    return status;
}

fbe_status_t fbe_database_update_pvd_config(fbe_object_id_t object_id, database_object_entry_t *config)
{
    fbe_status_t status;

    status = fbe_database_send_packet_to_object(FBE_PROVISION_DRIVE_CONTROL_CODE_UPDATE_CONFIGURATION,
                                                config,
                                                sizeof(*config),
                                                object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                    "%s Failed to send pvd config to pvd object\n",
                   __FUNCTION__);          
    }

    return status;
}

fbe_status_t fbe_database_update_pvd_block_size(fbe_object_id_t object_id, database_object_entry_t *config)
{
    fbe_status_t status;

    status = fbe_database_send_packet_to_object(FBE_PROVISION_DRIVE_CONTROL_CODE_UPDATE_BLOCK_SIZE,
                                                config,
                                                sizeof(*config),
                                                object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                    "%s Failed to send pvd config to pvd object\n",
                   __FUNCTION__);          
    }

    return status;
}

/**********************************************/
/*                                            */
/*             Virtual Drive                  */
/*                                            */
/**********************************************/
fbe_status_t fbe_database_set_vd_config(fbe_object_id_t object_id, fbe_database_control_vd_set_configuration_t *config)
{
    fbe_status_t status;
    fbe_raid_group_configuration_t raid_config;
    fbe_virtual_drive_configuration_t vd_config;

    /* init raid_set_config by config*/
    fbe_zero_memory(&raid_config, sizeof(fbe_raid_group_configuration_t));
    raid_config.width = config->width;
    raid_config.capacity = config->capacity;
    raid_config.chunk_size = config->chunk_size;
    raid_config.raid_type = config->raid_type;
    raid_config.element_size = config->element_size;
    raid_config.elements_per_parity = config->elements_per_parity;
    raid_config.debug_flags = config->debug_flags;
    raid_config.power_saving_idle_time_in_seconds = config->power_saving_idle_time_in_seconds;
    raid_config.power_saving_enabled = config->power_saving_enabled;
    raid_config.max_raid_latency_time_in_sec = config->max_raid_latency_time_in_sec;
    raid_config.generation_number = config->generation_number;
    raid_config.update_type = config->update_rg_type;

    /* Configure the raid group portion of the virtual drive. */
    status = fbe_database_send_packet_to_object(FBE_RAID_GROUP_CONTROL_CODE_SET_CONFIGURATION,
                                                &raid_config,
                                                sizeof(raid_config),
                                                object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);

    if(status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s Failed to send raid config to vd object\n",
                   __FUNCTION__);       
        return status;
    }

    /* init vd_set_config by config*/
    vd_config.configuration_mode = config->configuration_mode;
    vd_config.update_type = config->update_vd_type;
    vd_config.vd_default_offset = config->vd_default_offset;

    status = fbe_database_send_packet_to_object(FBE_VIRTUAL_DRIVE_CONTROL_CODE_SET_CONFIGURATION,
                                                &vd_config,
                                                sizeof(vd_config),
                                                object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                    "%s Failed to send vd config to vd object\n",
                   __FUNCTION__);          
    }

    return status;
}

fbe_status_t fbe_database_update_vd_config(fbe_object_id_t object_id, fbe_database_control_vd_set_configuration_t *config)
{
    fbe_status_t status;
    fbe_virtual_drive_configuration_t vd_config;

/* enable the following code when FBE_RAID_GROUP_CONTROL_CODE_UPDATE_CONFIGURATION is implemented*/
//  /* Configure the raid group portion of the virtual drive. */
//  status = fbe_database_send_packet_to_object(FBE_RAID_GROUP_CONTROL_CODE_UPDATE_CONFIGURATION,
//                                              &config->raid_set_configuration,
//                                              sizeof(config->raid_set_configuration),
//                                              object_id,
//                                              NULL,
//                                              0,
//                                              FBE_PACKET_FLAG_NO_ATTRIB,
//                                              FBE_PACKAGE_ID_SEP_0);
//
//  if(status != FBE_STATUS_OK)
//  {
//      database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM,
//                 FBE_TRACE_MESSAGE_ID_INFO,
//                 "%s Failed to send raid config to vd object\n",
//                 __FUNCTION__);
//      return status;
//  }

    /* init vd_set_config by config*/
    vd_config.configuration_mode = config->configuration_mode;
    vd_config.update_type = config->update_vd_type;

    status = fbe_database_send_packet_to_object(FBE_VIRTUAL_DRIVE_CONTROL_CODE_UPDATE_CONFIGURATION,
                                                &vd_config,
                                                sizeof(vd_config),
                                                object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                    "%s Failed to send vd config to vd object\n",
                   __FUNCTION__);          
    }

    return status;
}

fbe_status_t fbe_database_set_raid_config(fbe_object_id_t object_id,
                                          fbe_raid_group_configuration_t *config)
{
    fbe_status_t status;

    status = fbe_database_send_packet_to_object(FBE_RAID_GROUP_CONTROL_CODE_SET_CONFIGURATION,
                                                config,
                                                sizeof(*config),
                                                object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0); 
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                    "%s Failed to send config to raid object\n",
                   __FUNCTION__);         
    }
    return status;
}

fbe_status_t fbe_database_update_raid_config(fbe_object_id_t object_id,
                                          fbe_raid_group_configuration_t *config)
{
    fbe_status_t status;

    status = fbe_database_send_packet_to_object(FBE_RAID_GROUP_CONTROL_CODE_UPDATE_CONFIGURATION,
                                                config,
                                                sizeof(*config),
                                                object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0); 
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                    "%s Failed to send config to raid object\n",
                   __FUNCTION__);         
    }
    return status;
}


fbe_status_t fbe_database_set_lun_config(fbe_object_id_t object_id, fbe_database_lun_configuration_t *config)
{
    fbe_status_t status;

    status = fbe_database_send_packet_to_object(FBE_LUN_CONTROL_CODE_SET_CONFIGURATION,
                                                config,
                                                sizeof(*config),
                                                object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0); 
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                    "%s Failed to send config to lun object\n",
                   __FUNCTION__);         
    }
    return status;
}

/*!***************************************************************
 * @fn database_common_map_class_id_fbe_to_database
 *****************************************************************
 * @brief
 *   map fbe_class_id_t to database_class_id_t.
 *
 * @param fbe_class_id
 *
 * @return database_class_id
 *
 * @version
 *    3/22/2011:   created
 *
 ****************************************************************/
database_class_id_t database_common_map_class_id_fbe_to_database(fbe_class_id_t fbe_class)
{
    switch (fbe_class) {
    case FBE_CLASS_ID_BVD_INTERFACE:
        return DATABASE_CLASS_ID_BVD_INTERFACE;

    case FBE_CLASS_ID_LUN:
        return DATABASE_CLASS_ID_LUN;
        
    case FBE_CLASS_ID_MIRROR:
        return DATABASE_CLASS_ID_MIRROR;

    case FBE_CLASS_ID_PARITY:
        return DATABASE_CLASS_ID_PARITY;
        
    case FBE_CLASS_ID_STRIPER:
        return DATABASE_CLASS_ID_STRIPER;
        
    case FBE_CLASS_ID_VIRTUAL_DRIVE:
        return DATABASE_CLASS_ID_VIRTUAL_DRIVE;
        
    case FBE_CLASS_ID_PROVISION_DRIVE:
        return DATABASE_CLASS_ID_PROVISION_DRIVE;
        
    case FBE_CLASS_ID_EXTENT_POOL:
        return DATABASE_CLASS_ID_EXTENT_POOL;
        
    case FBE_CLASS_ID_EXTENT_POOL_LUN:
        return DATABASE_CLASS_ID_EXTENT_POOL_LUN;
        
    case FBE_CLASS_ID_EXTENT_POOL_METADATA_LUN:
        return DATABASE_CLASS_ID_EXTENT_POOL_METADATA_LUN;
        
    default:
        return DATABASE_CLASS_ID_INVALID;
    }
}

/*!***************************************************************
 * @fn database_common_map_class_id_database_to_fbe
 *****************************************************************
 * @brief
 *   map database_class_id_t to fbe_class_id_t.
 *
 * @param  database_class_id
 *
 * @return fbe_class_id
 *
 * @version
 *    3/22/2011:   created
 *
 ****************************************************************/
fbe_class_id_t database_common_map_class_id_database_to_fbe(database_class_id_t database_class)
{
    switch (database_class) {
    case DATABASE_CLASS_ID_BVD_INTERFACE:
        return FBE_CLASS_ID_BVD_INTERFACE;

    case DATABASE_CLASS_ID_LUN:
        return FBE_CLASS_ID_LUN;

    case DATABASE_CLASS_ID_MIRROR:
        return FBE_CLASS_ID_MIRROR;

    case DATABASE_CLASS_ID_PARITY:
        return FBE_CLASS_ID_PARITY;

    case DATABASE_CLASS_ID_STRIPER:
        return FBE_CLASS_ID_STRIPER;

    case DATABASE_CLASS_ID_VIRTUAL_DRIVE:
        return FBE_CLASS_ID_VIRTUAL_DRIVE;

    case DATABASE_CLASS_ID_PROVISION_DRIVE:
        return FBE_CLASS_ID_PROVISION_DRIVE;

    case DATABASE_CLASS_ID_EXTENT_POOL:
        return FBE_CLASS_ID_EXTENT_POOL;

    case DATABASE_CLASS_ID_EXTENT_POOL_LUN:
        return FBE_CLASS_ID_EXTENT_POOL_LUN;

    case DATABASE_CLASS_ID_EXTENT_POOL_METADATA_LUN:
        return FBE_CLASS_ID_EXTENT_POOL_METADATA_LUN;

    default:
        return FBE_CLASS_ID_INVALID;
    }
}

/*!***************************************************************
 * @fn database_common_init_user_entry
 *****************************************************************
 * @brief
 *   Initialize user entry.
 *   If initial value of the fileds in user entry is not zero,
 *   initialize it in this function.
 *
 * @param  in_entry_ptr - pointer to the entry memory
 *
 * @return fbe_status_t - status
 *
 * @version
 *    3/24/2011:   created
 *    4/25/2012:   add comments.
 *
 ****************************************************************/
fbe_status_t database_common_init_user_entry(database_user_entry_t *in_entry_ptr)
{
    if (in_entry_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                   FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                    "%s: NULL pointer!\n",
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;        
    }
    /* For all the elements, if all the default value is zero or NULL, 
     * initialise the memory with zero is enough. 
     * Else we should set the default initial value with the specific value.
     * When new version releases, change this function if needed
    */
    fbe_zero_memory(in_entry_ptr, sizeof(database_user_entry_t));
    return FBE_STATUS_OK;        
}
   
/*!***************************************************************
 * @fn database_common_init_object_entry
 *****************************************************************
 * @brief
 *   Initialize object entry.
 *   If initial value of the fileds in object entry is not zero,
 *   initialize it in this function.
 *
 * @param  in_entry_ptr - pointer to the entry memory
 *
 * @return fbe_status_t - status
 *
 * @version
 *    3/24/2011:   created
 *    4/25/2012:   add comments.
 *
 ****************************************************************/
fbe_status_t database_common_init_object_entry(database_object_entry_t *in_entry_ptr)
{
    if (in_entry_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                   FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                    "%s: NULL pointer!\n",
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;        
    }
    /* For all the elements, if all the default value is zero or NULL, 
     * initialise the memory with zero is enough. 
     * Else we should set the default initial value with the specific value.
     * When new version releases, change this function if needed
    */
    fbe_zero_memory(in_entry_ptr, sizeof(database_object_entry_t));
    return FBE_STATUS_OK;        
}

/*!***************************************************************
 * @fn database_common_init_edge_entry
 *****************************************************************
 * @brief
 *   Initialize edge entry.
 *   If initial value of the fileds in edge entry is not zero,
 *   initialize it in this function.
 *
 * @param  in_entry_ptr - pointer to the entry memory
 *
 * @return fbe_status_t - status
 *
 * @version
 *    3/24/2011:   created
 *    4/25/2012:   add comments.
 *
 ****************************************************************/
fbe_status_t database_common_init_edge_entry(database_edge_entry_t *in_entry_ptr)
{
    if (in_entry_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                   FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                    "%s: NULL pointer!\n",
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;        
    }
    /* For all the elements, if all the default value is zero or NULL, 
     * initialise the memory with zero is enough. 
     * Else we should set the default initial value with the specific value.
     * When new version releases, change this function if needed
    */
    fbe_zero_memory(in_entry_ptr, sizeof(database_edge_entry_t));
    return FBE_STATUS_OK;        
}

/*!***************************************************************
 * @fn database_common_init_global_info_entry
 *****************************************************************
 * @brief
 *   Initialize system setting entry.
 *   If initial value of the fileds in global info entry is not zero,
 *   initialize it in this function.
 *
 * @param  in_entry_ptr - pointer to the entry memory
 *
 * @return fbe_status_t - status
 *
 * @version
 *    05/10/2011: Arun S - created
 *    4/25/2012:   add comments.
 *
 ****************************************************************/
fbe_status_t database_common_init_global_info_entry(database_global_info_entry_t *in_entry_ptr)
{
    if (in_entry_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                   FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                    "%s: NULL pointer!\n",
                   __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;        
    }

    /* For all the elements, if all the default value is zero or NULL, 
     * initialise the memory with zero is enough. 
     * Else we should set the default initial value with the specific value.
     * When new version releases, change this function if needed
    */
    fbe_zero_memory(in_entry_ptr, sizeof(database_global_info_entry_t));
    return FBE_STATUS_OK;        
}

/*!*****************************************************************
 * @brief
 *   Initialize system spare entry.
 *   If initial value of the fileds in system spare entry is not zero,
 *   initialize it in this function.
 *
 * @param  in_entry_ptr - pointer to the entry memory
 *
 * @return fbe_status_t - status
 *
 * @version
 *    05/10/2011: Arun S - created
 *    4/25/2012:   add comments.
 *
 ****************************************************************/
fbe_status_t database_common_init_system_spare_entry(database_system_spare_entry_t *in_entry_ptr)
{
    if (in_entry_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                   FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                    "%s: NULL pointer!\n",
                   __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;        
    }

    /* For all the elements, if all the default value is zero or NULL, 
     * initialise the memory with zero is enough. 
     * Else we should set the default initial value with the specific value.
     * When new version releases, change this function if needed
    */
    fbe_zero_memory(in_entry_ptr, sizeof(database_system_spare_entry_t));
    return FBE_STATUS_OK;        
}

/*!***************************************************************
 * @fn database_common_create_global_info_from_table
 *****************************************************************
 * @brief
 *   create system settings from the given table.
 *
 * @param  in_table_ptr - pointer to the table
 * @param  size - size of the table
 *
 * @return fbe_status_t - status
 *
 * @version
 *    05/11/2011: Arun S - created
 *
 ****************************************************************/
fbe_status_t database_common_create_global_info_from_table(database_global_info_entry_t *in_table_ptr, 
                                                              database_table_size_t size)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_global_info_entry_t *current_entry = NULL;
    fbe_u32_t i = 0;

    if (in_table_ptr == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER, 
                       "%s: NULL pointer!\n", 
                       __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;        
    }

    current_entry = in_table_ptr;

    for (i = 0; i < size; i++ )
    {
        if ((current_entry->header.state != DATABASE_CONFIG_ENTRY_VALID) &&
            (current_entry->header.state != DATABASE_CONFIG_ENTRY_UNCOMMITTED))
        { 
            /* not a valid entry, skip it*/
            /* move down to the next entry*/
            current_entry++;
            continue;
        }
        
        status = database_common_update_global_info_from_table_entry(current_entry);
       
        /* move down to the next entry*/
        current_entry++;
    }

    return FBE_STATUS_OK;        
}

/*!***************************************************************
 * @fn database_common_update_global_info_from_table_entry
 *****************************************************************
 * @brief
 *   create system setting from the given table.
 *
 * @param  in_table_ptr - pointer to the table
 *
 * @return fbe_status_t - status
 *
 * @version
 *    05/11/2011:   created
 *
 ****************************************************************/
fbe_status_t database_common_update_global_info_from_table_entry(database_global_info_entry_t *in_table_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_global_info_entry_t *current_entry = NULL;
       
    if (in_table_ptr == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER, 
                       "%s: NULL pointer!\n", 
                       __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;        
    }

    current_entry = in_table_ptr;

    switch(current_entry->type){
    case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_POWER_SAVE:
        status = fbe_database_set_system_power_save_info(&current_entry->info_union.power_saving_info);
        if(status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER, 
                           "%s: Failed to SET system power save info!\n", 
                           __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;        
        }
        break;
    case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_SPARE:
        status = fbe_database_set_system_spare_info(&current_entry->info_union.spare_info);
        if(status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER, 
                           "%s: Failed to SET system spare info!\n", 
                           __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;        
        }
        break;
    case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_GENERATION:
        /* Don't need to send this info to anyone, just skip it */
        break;
    case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_TIME_THRESHOLD:
        /* Don't need to send this info to anyone, just skip it */
        status = FBE_STATUS_OK;
        break;
    case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_ENCRYPTION:
        status = fbe_database_set_system_encryption_info(&current_entry->info_union.encryption_info);
        if(status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER, 
                           "%s: Failed to SET system encryption info!\n", 
                           __FUNCTION__);

            return FBE_STATUS_GENERIC_FAILURE;        
        }
        break;
    case DATABASE_GLOBAL_INFO_TYPE_GLOBAL_PVD_CONFIG:
        /* Don't need to send this info to anyone, just skip it */
        break;
    default:
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER, 
                       "%s: Unsupported global info type 0x%x!\n", 
                       __FUNCTION__, current_entry->type);
        status = FBE_STATUS_GENERIC_FAILURE;
        break;    
    }
    return status;        
}

/*!***************************************************************
 * @fn database_common_create_object_from_table
 *****************************************************************
 * @brief
 *   create objects from the given range of a table.
 *
 * @param  in_table_ptr - pointer to the table
 * @param  strat_index
 * @param  end_index
 * 
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/06/2011:   created
 *
 ****************************************************************/
fbe_status_t database_common_create_object_from_table(database_table_t *in_table_ptr, fbe_u32_t start_index, fbe_u32_t number_of_entries)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t i;
    database_object_entry_t *current_entry = NULL;
    if (in_table_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                   FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                    "%s: NULL pointer!\n",
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;        
    }

    /*validate the index*/
    if ((in_table_ptr->table_size < start_index)
        ||(in_table_ptr->table_size < (start_index + number_of_entries))) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                       "%s: start 0x%x or end 0x%x exceed table_size 0x%x!\n",
                       __FUNCTION__, 
                       start_index, 
                       (start_index + number_of_entries),
                       in_table_ptr->table_size);
        return FBE_STATUS_GENERIC_FAILURE;        

    }

    current_entry = &in_table_ptr->table_content.object_entry_ptr[start_index];
    for (i = 0; i < number_of_entries; i++ ) {
        if (current_entry->header.state != DATABASE_CONFIG_ENTRY_VALID
            && current_entry->header.state != DATABASE_CONFIG_ENTRY_CORRUPT) { 
            /* not a valid entry, skip it*/
            /* move down to the next entry*/
            current_entry++;
            continue;
        }
        /* if the object entry is corrupt, the entry has an invalid class id and invalid object id 0*/
        if (current_entry->header.state == DATABASE_CONFIG_ENTRY_CORRUPT && current_entry->db_class_id == DATABASE_CLASS_ID_INVALID) {
            current_entry++;
            continue;
        }
        /* we have a valid entry, create the object */
        status = database_common_create_object_from_table_entry(current_entry);
        /* move down to the next entry*/
        current_entry++;
    }
    return FBE_STATUS_OK;        
}

/*!***************************************************************
 * @fn database_common_create_object_from_table_entry
 *****************************************************************
 * @brief
 *   create objects from the given table.
 *
 * @param  in_entry_ptr - pointer to the table
 * @param  size - size of the table
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/06/2011:   created
 *
 ****************************************************************/
fbe_status_t database_common_create_object_from_table_entry(database_object_entry_t *in_entry_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_class_id_t fbe_class_id;

    fbe_class_id = database_common_map_class_id_database_to_fbe(in_entry_ptr->db_class_id);
    
    status = fbe_database_create_object(fbe_class_id, &in_entry_ptr->header.object_id);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                   FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                    "%s: create object failed, obj_id 0x%x!\n",
                   __FUNCTION__, in_entry_ptr->header.object_id);
    }

    database_trace(FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: create object of fbe_class 0x%x, obj_id 0x%x!\n",
                   __FUNCTION__, fbe_class_id, in_entry_ptr->header.object_id);

    return status;
}

/*!***************************************************************
 * @fn database_common_create_edge_from_table_entry
 *****************************************************************
 * @brief
 *   create edges from the given table.
 *
 * @param  in_entry_ptr - pointer to the table
 * @param  size - size of the table
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/06/2011:   created
 *
 ****************************************************************/
fbe_status_t database_common_create_edge_from_table_entry(database_edge_entry_t *in_table_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_edge_entry_t *current_entry = NULL;
       
    if (in_table_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                   FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                    "%s: NULL pointer!\n",
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;        
    }
    current_entry = in_table_ptr;
         
    status = fbe_database_create_edge(current_entry->header.object_id,
                                      current_entry->server_id,
                                      current_entry->client_index,
                                      current_entry->capacity,
                                      current_entry->offset,
                                      current_entry->ignore_offset);   	
    return status;        
}
/*!***************************************************************
 * @fn database_common_destroy_object_from_table_entry
 *****************************************************************
 * @brief
 *   destroy objects from the given table.
 *
 * @param  in_entry_ptr - pointer to the table
 * @param  size - size of the table
 *
 * @return fbe_status_t - status
 *
 *
 ****************************************************************/
fbe_status_t database_common_destroy_object_from_table_entry(database_object_entry_t *in_entry_ptr)
{
    fbe_status_t         		 	status = FBE_STATUS_GENERIC_FAILURE;
    
    /*register for notification*/
    database_destroy_notification_context.object_id = in_entry_ptr->header.object_id; 
    fbe_semaphore_init( &database_destroy_notification_context.sem, 0, 1);
    /* settin up the notification_element*/
    database_destroy_notification_element.notification_function = database_destroy_notification_callback;
    database_destroy_notification_element.notification_context = &database_destroy_notification_context;
    database_destroy_notification_element.targe_package = FBE_PACKAGE_ID_SEP_0;
    database_destroy_notification_element.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL ;
    database_destroy_notification_element.notification_type = FBE_NOTIFICATION_TYPE_OBJECT_DESTROYED;

    status = database_common_register_notification ( &database_destroy_notification_element,
                                                     FBE_PACKAGE_ID_SEP_0);

    /* Set permanent destroy in base config.
     */ 
    status = fbe_database_send_packet_to_object(FBE_BASE_CONFIG_CONTROL_CODE_SET_PERMANENT_DESTROY,
                                                NULL,
                                                0,
                                                in_entry_ptr->header.object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0); 
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                   FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                    "%s: Error: 0x%x, setting perm destroy for 0x%x!\n",
                   __FUNCTION__, status, in_entry_ptr->header.object_id);
    }

   /* destroy object*/
    status = fbe_database_destroy_object(in_entry_ptr->header.object_id);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                   FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                    "%s: destroy failed, obj_id 0x%x!\n",
                   __FUNCTION__, in_entry_ptr->header.object_id);
    }

    return status;
}


/*!***************************************************************
 * @fn database_common_wait_destroy_object_complete
 *****************************************************************
 * @brief
 *   Waits for destroy objects operation to complete.
 *
 * @param  None
 *
 * @return fbe_status_t - status
 *
 *
 ****************************************************************/
fbe_status_t database_common_wait_destroy_object_complete(void)
{
    fbe_status_t status;

    status = fbe_semaphore_wait_ms(&database_destroy_notification_context.sem, 30000);
    if(status != FBE_STATUS_OK)  
    {   
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s Failed to destroy object 0x%x in 30s, status 0x%x\n",
                __FUNCTION__, database_destroy_notification_context.object_id, status);
    }
    /* Don't check the return value of following function. It fails only when the element is not existed */
    database_common_unregister_notification (&database_destroy_notification_element,
                                                       FBE_PACKAGE_ID_SEP_0);

    database_destroy_notification_element.notification_function = NULL;
    database_destroy_notification_element.notification_context = NULL;
    database_destroy_notification_element.registration_id = FBE_PACKAGE_ID_INVALID;
    /* NOTE: unregister notification before destorying the semaphore */
    fbe_semaphore_destroy( &database_destroy_notification_context.sem);
    return status;
}


/*!***************************************************************
 * @fn database_destroy_notification_callback
 *****************************************************************
 * @brief
 *   callback function for the system objects.
 *
 * @param  object_id - the id of the object that generates the notification
 * @param  notification_info - info of the notification
 * @param  context - context passed in at the timeof registering the notification.
 *
 * @return fbe_status_t - status
 *
 *
 ****************************************************************/
fbe_status_t database_destroy_notification_callback(fbe_object_id_t object_id, 
                                                    fbe_notification_info_t notification_info,
                                                    fbe_notification_context_t context)
{
    database_destroy_notification_context_t * ns_context = NULL;

    ns_context = (database_destroy_notification_context_t *)context;

    if ((ns_context->object_id == object_id)
        &&(notification_info.notification_type==FBE_NOTIFICATION_TYPE_OBJECT_DESTROYED)) {
        fbe_semaphore_release(&ns_context->sem, 0, 1, FBE_FALSE);
    }
    return FBE_STATUS_OK;
}


/*!***************************************************************
 * @fn database_common_destroy_edge_from_table_entry
 *****************************************************************
 * @brief
 *   destroy edges from the given table.
 *
 * @param  in_entry_ptr - pointer to the table
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/06/2011:   created
 *
 ****************************************************************/
fbe_status_t database_common_destroy_edge_from_table_entry(database_edge_entry_t *in_table_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_edge_entry_t *current_entry = NULL;
       
    if (in_table_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                   FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                    "%s: NULL pointer!\n",
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;        
    }
    current_entry = in_table_ptr;
         
    status = fbe_database_destroy_edge(current_entry->header.object_id,									  
                                       current_entry->client_index);			                                   
    return status;        
}

/*!***************************************************************
 * @fn database_common_set_config_from_table
 *****************************************************************
 * @brief
 *   create objects from the given table.
 *
 * @param  in_table_ptr - pointer to the table
 * @param  size - size of the table
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/06/2011:   created
 *
 ****************************************************************/
fbe_status_t database_common_set_config_from_table(database_table_t *in_table_ptr, fbe_u32_t start_index, fbe_u32_t number_of_entries)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t i;
    database_object_entry_t *current_entry = NULL;

    if (in_table_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                   FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                    "%s: NULL pointer!\n",
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;        
    }
    /*validate the index*/
    if ((in_table_ptr->table_size < start_index)
        ||(in_table_ptr->table_size < (start_index + number_of_entries))) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                       "%s: start 0x%x or end 0x%x exceed table_size 0x%x!\n",
                       __FUNCTION__, 
                       start_index, 
                       (start_index + number_of_entries),
                       in_table_ptr->table_size);
        return FBE_STATUS_GENERIC_FAILURE;        
    }

    current_entry = &in_table_ptr->table_content.object_entry_ptr[start_index];
    for (i = 0; i < number_of_entries; i++ ) {
        if (current_entry->header.state != DATABASE_CONFIG_ENTRY_VALID
            && current_entry->header.state != DATABASE_CONFIG_ENTRY_CORRUPT) { 
            /* not a valid entry, skip it*/
            /* move down to the next entry*/
            current_entry++;
            continue;
        }
        /* we have a valid entry,set config */
        status = database_common_set_config_from_table_entry(current_entry);

        status = database_common_update_encryption_mode_from_table_entry(current_entry);
        /* move down to the next entry*/
        current_entry++;
    }
    return FBE_STATUS_OK;        
}

/*!***************************************************************
 * @fn database_common_set_config_from_table_entry
 *****************************************************************
 * @brief
 *   create objects from the given table.
 *
 * @param  in_table_ptr - pointer to the table
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/06/2011:   created
 *
 ****************************************************************/
fbe_status_t database_common_set_config_from_table_entry(database_object_entry_t *in_entry_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_class_id_t fbe_class_id;

    /* we have a valid entry, create the object */
    fbe_class_id = database_common_map_class_id_database_to_fbe(in_entry_ptr->db_class_id);
    switch(fbe_class_id) {
    case FBE_CLASS_ID_PROVISION_DRIVE:
        status = fbe_database_set_pvd_config(in_entry_ptr->header.object_id, &in_entry_ptr->set_config_union.pvd_config);
        break;
    case FBE_CLASS_ID_VIRTUAL_DRIVE:
        status = fbe_database_set_vd_config(in_entry_ptr->header.object_id, &in_entry_ptr->set_config_union.vd_config);
        break;
    case FBE_CLASS_ID_STRIPER:
    case FBE_CLASS_ID_MIRROR:
    case FBE_CLASS_ID_PARITY:
        status = fbe_database_set_raid_config(in_entry_ptr->header.object_id, &in_entry_ptr->set_config_union.rg_config);
        break;
    case FBE_CLASS_ID_LUN:
        status = fbe_database_set_lun_config(in_entry_ptr->header.object_id, &in_entry_ptr->set_config_union.lu_config);
        break;
    case FBE_CLASS_ID_EXTENT_POOL:
        status = fbe_database_set_ext_pool_config(in_entry_ptr->header.object_id, &in_entry_ptr->set_config_union.ext_pool_config);
        break;
    case FBE_CLASS_ID_EXTENT_POOL_LUN:
    case FBE_CLASS_ID_EXTENT_POOL_METADATA_LUN:
        status = fbe_database_set_ext_pool_lun_config(in_entry_ptr->header.object_id, &in_entry_ptr->set_config_union.ext_pool_lun_config);
        break;
    default:
        break;
    }
    return status;        
}

/*!***************************************************************
 * @fn database_common_update_config_from_table_entry
 *****************************************************************
 * @brief
 *   create objects from the given table.
 *
 * @param  in_table_ptr - pointer to the table
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/06/2011:   created
 *
 ****************************************************************/
fbe_status_t database_common_update_config_from_table_entry(database_object_entry_t *in_entry_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_class_id_t fbe_class_id;

    /* we have a valid entry, create the object */
    fbe_class_id = database_common_map_class_id_database_to_fbe(in_entry_ptr->db_class_id);
    switch(fbe_class_id) {
    case FBE_CLASS_ID_PROVISION_DRIVE:
        status = fbe_database_update_pvd_config(in_entry_ptr->header.object_id, in_entry_ptr);
        break;
    case FBE_CLASS_ID_VIRTUAL_DRIVE:
        status = fbe_database_update_vd_config(in_entry_ptr->header.object_id, &in_entry_ptr->set_config_union.vd_config);
        break;
    case FBE_CLASS_ID_STRIPER:
    case FBE_CLASS_ID_MIRROR:
    case FBE_CLASS_ID_PARITY:
        status = fbe_database_update_raid_config(in_entry_ptr->header.object_id, &in_entry_ptr->set_config_union.rg_config);
        break;
//  case FBE_CLASS_ID_LUN:
//      status = fbe_database_update_lun_config(in_entry_ptr->header.object_id, &in_entry_ptr->set_config_union.lu_config);
//      break;
    default:
        break;
    }
    return status;
}

/*!***************************************************************
 * @fn database_common_update_config_from_table_entry
 *****************************************************************
 * @brief
 *   create objects from the given table.
 *
 * @param  in_table_ptr - pointer to the table
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/06/2011:   created
 *
 ****************************************************************/
fbe_status_t database_common_update_encryption_mode_from_table_entry(database_object_entry_t *in_entry_ptr)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_object_id_t  object_id;
    fbe_base_config_control_encryption_mode_t encryption_mode;

    object_id = in_entry_ptr->header.object_id;
    encryption_mode.encryption_mode = in_entry_ptr->base_config.encryption_mode;

    status = fbe_database_send_packet_to_object(FBE_BASE_CONFIG_CONTROL_CODE_SET_ENCRYPTION_MODE,
                                                &encryption_mode,
                                                sizeof(fbe_base_config_control_encryption_mode_t),
                                                object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                    "%s Failed to send pvd config to pvd object\n",
                   __FUNCTION__);          
    }

    return status;
}

/*!***************************************************************
 * @fn database_common_commit_object_from_table
 *****************************************************************
 * @brief
 *   commit objects from the given table in order of 
 *   bvd -> pvds -> vds -> rgs -> luns
 *
 * @param  in_table_ptr - pointer to the table
 * @param  size - size of the table
 * @param  is_initial_creation - is this object created for the very first time
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/06/2011:   created
 *
 ****************************************************************/
fbe_status_t database_common_commit_object_from_table(database_table_t *in_table_ptr, fbe_u32_t start_index, fbe_u32_t number_of_entries, fbe_bool_t is_initial_creation)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t i;
    database_object_entry_t *current_entry = NULL;
    fbe_u32_t commited_entries = 0;
    
    if (in_table_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                   FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                    "%s: NULL pointer!\n",
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;        
    }
    /*validate the index*/
    if ((in_table_ptr->table_size < start_index)
        ||(in_table_ptr->table_size < (start_index + number_of_entries))) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                       "%s: start 0x%x or end 0x%x exceed table_size 0x%x!\n",
                       __FUNCTION__, 
                       start_index, 
                       (start_index + number_of_entries),
                       in_table_ptr->table_size);
        return FBE_STATUS_GENERIC_FAILURE;        

    }

    /* 1. Find the bvd and commit that object */
    current_entry = &in_table_ptr->table_content.object_entry_ptr[start_index];
    for (i = 0; i < number_of_entries; i++ ) {
        if (current_entry->header.state == DATABASE_CONFIG_ENTRY_VALID &&
            current_entry->db_class_id == DATABASE_CLASS_ID_BVD_INTERFACE) {
            /* we have a valid entry, commit the object */
            status = fbe_database_commit_object(current_entry->header.object_id, current_entry->db_class_id, is_initial_creation);
            commited_entries++;
            break;
        }
        current_entry++;
    }

    /* 2. Commit all pvds. Increase the delay every 200 committed objects */
    current_entry = &in_table_ptr->table_content.object_entry_ptr[start_index];
    for (i = 0; i < number_of_entries; i++ ) {
        if (current_entry->header.state == DATABASE_CONFIG_ENTRY_VALID &&
            current_entry->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE) {
            /* we have a valid entry, commit the object */
            status = fbe_database_commit_object(current_entry->header.object_id, current_entry->db_class_id, is_initial_creation);
            commited_entries++;
            if (!(commited_entries % 10)) {
                /* wait time increases by a msec every 200 objects. The start is 15 msec
                 * if we don't do that, the CMI would be flooeded with messages*/
                fbe_thread_delay(15+(commited_entries/200));
            }
        }
        current_entry++;
    }

    /* 3. Commit all vds. Increase the delay every 200 committed objects */
    current_entry = &in_table_ptr->table_content.object_entry_ptr[start_index];
    for (i = 0; i < number_of_entries; i++ ) {
        if (current_entry->header.state == DATABASE_CONFIG_ENTRY_VALID &&
            current_entry->db_class_id == DATABASE_CLASS_ID_VIRTUAL_DRIVE) {
            /* we have a valid entry, commit the object */
            status = fbe_database_commit_object(current_entry->header.object_id, current_entry->db_class_id, is_initial_creation);
            commited_entries++;
            if (!(commited_entries % 10)) {
                /* wait time increases by a msec every 200 objects. The start is 15 msec
                 * if we don't do that, the CMI would be flooded with messages*/
                fbe_thread_delay(15+(commited_entries/200));
            }
        }
        current_entry++;
    }

    /* 4. Commit all raid groups. Increase the delay every 200 committed objects */
    current_entry = &in_table_ptr->table_content.object_entry_ptr[start_index];
    for (i = 0; i < number_of_entries; i++ ) {
        if (current_entry->header.state == DATABASE_CONFIG_ENTRY_VALID &&
            current_entry->db_class_id > DATABASE_CLASS_ID_RAID_START && 
            current_entry->db_class_id < DATABASE_CLASS_ID_RAID_END) {
            /* we have a valid entry, commit the object */
            status = fbe_database_commit_object(current_entry->header.object_id, current_entry->db_class_id, is_initial_creation);
            commited_entries++;
            if (!(commited_entries % 10)) {
                /* wait time increases by a msec every 200 objects. The start is 15 msec
                 * if we don't do that, the CMI would be flooded with messages*/
                fbe_thread_delay(15+(commited_entries/200));
            }
        }
        current_entry++;
    }

    /* 5. Commit all luns. Increase the delay every 200 committed objects */
    current_entry = &in_table_ptr->table_content.object_entry_ptr[start_index];
    for (i = 0; i < number_of_entries; i++ ) {
        if (current_entry->header.state == DATABASE_CONFIG_ENTRY_VALID &&
            current_entry->db_class_id == DATABASE_CLASS_ID_LUN) {
            /* we have a valid entry, commit the object */
            status = fbe_database_commit_object(current_entry->header.object_id, current_entry->db_class_id, is_initial_creation);
            commited_entries++;
            if (!(commited_entries % 10)) {
                /* wait time increases by a msec every 200 objects. The start is 15 msec
                 * if we don't do that, the CMI would be flooeded with messages*/
                fbe_thread_delay(15+(commited_entries/200));
            }
        }
        current_entry++;
    }

    /* 5. Commit all ext pools. Increase the delay every 200 committed objects */
    current_entry = &in_table_ptr->table_content.object_entry_ptr[start_index];
    for (i = 0; i < number_of_entries; i++ ) {
        if (current_entry->header.state == DATABASE_CONFIG_ENTRY_VALID &&
            current_entry->db_class_id == DATABASE_CLASS_ID_EXTENT_POOL) {
            /* we have a valid entry, commit the object */
            status = fbe_database_commit_object(current_entry->header.object_id, current_entry->db_class_id, is_initial_creation);
            commited_entries++;
            if (!(commited_entries % 10)) {
                /* wait time increases by a msec every 200 objects. The start is 15 msec
                 * if we don't do that, the CMI would be flooeded with messages*/
                fbe_thread_delay(15+(commited_entries/200));
            }
        }
        current_entry++;
    }

    /* 5. Commit all ext pool luns. Increase the delay every 200 committed objects */
    current_entry = &in_table_ptr->table_content.object_entry_ptr[start_index];
    for (i = 0; i < number_of_entries; i++ ) {
        if (current_entry->header.state == DATABASE_CONFIG_ENTRY_VALID &&
            (current_entry->db_class_id == DATABASE_CLASS_ID_EXTENT_POOL_LUN || 
             current_entry->db_class_id == DATABASE_CLASS_ID_EXTENT_POOL_METADATA_LUN)) {
            /* we have a valid entry, commit the object */
            status = fbe_database_commit_object(current_entry->header.object_id, current_entry->db_class_id, is_initial_creation);
            commited_entries++;
            if (!(commited_entries % 10)) {
                /* wait time increases by a msec every 200 objects. The start is 15 msec
                 * if we don't do that, the CMI would be flooeded with messages*/
                fbe_thread_delay(15+(commited_entries/200));
            }
        }
        current_entry++;
    }

    return FBE_STATUS_OK;        
}

/*!***************************************************************
 * @fn database_common_create_edge_from_table
 *****************************************************************
 * @brief
 *   create edges from the given table.
 *
 * @param  in_table_ptr - pointer to the table
 * @param  size - size of the table
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/06/2011:   created
 *
 ****************************************************************/
fbe_status_t database_common_create_edge_from_table(database_table_t *in_table_ptr, fbe_u32_t start_index, fbe_u32_t number_of_entries)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    database_edge_entry_t *current_entry = NULL;
    fbe_u32_t i;

    if (in_table_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                   FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                    "%s: NULL pointer!\n",
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;        
    }

    /*validate the index*/
    if ((in_table_ptr->table_size < start_index)
        ||(in_table_ptr->table_size < (start_index + number_of_entries))) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                       "%s: start 0x%x or end 0x%x exceed table_size 0x%x!\n",
                       __FUNCTION__, 
                       start_index, 
                       (start_index + number_of_entries),
                       in_table_ptr->table_size);
        return FBE_STATUS_GENERIC_FAILURE;        

    }

    current_entry = &in_table_ptr->table_content.edge_entry_ptr[start_index];
    for (i = 0; i < number_of_entries; i++ ) {
        if (current_entry->header.state != DATABASE_CONFIG_ENTRY_VALID) { 
            /* not a valid entry, skip it*/
            /* move down to the next entry*/
            current_entry++;
            continue;
        }
        
        status = database_common_create_edge_from_table_entry(current_entry);
       
        /* move down to the next entry*/
        current_entry++;
    }
    return FBE_STATUS_OK;        
}

/*!***************************************************************
 * @fn database_common_register_notification
 *****************************************************************
 * @brief
 *   register notification_element.
 *
 * @param  notification_element - describes the notification_element,
 *         which includes notification_function for the notification to callback,
 *         and context, which passes in the context for the callback to use.
 *         Be careful on the scope of the context, the memory should
 *         still be available when the callback occurs.
 * @param  package_id - which package the notification comes from.
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/11/2011:   created
 *
 ****************************************************************/
fbe_status_t database_common_register_notification ( fbe_notification_element_t *notification_element,
                                                     fbe_package_id_t            package_id)
{
    fbe_status_t status ;

    status = fbe_database_send_packet_to_service(FBE_NOTIFICATION_CONTROL_CODE_REGISTER,
                                                 notification_element,
                                                 sizeof(fbe_notification_element_t),
                                                 FBE_SERVICE_ID_NOTIFICATION,
                                                 NULL, /* no sg list*/
                                                 0,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 package_id);
    if (status != FBE_STATUS_OK)
    {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: failed to register notification, status 0x%x.\n",
                        __FUNCTION__, status);
    }
    return status;
}

/*!***************************************************************
 * @fn database_common_unregister_notification
 *****************************************************************
 * @brief
 *   unregister notification_element.
 *
 * @param  notification_element - describes the notification_element,
 *         which includes notification_function for the notification to callback,
 *         and context, which passes in the context for the callback to use.
 *         Be careful on the scope of the context, the memory should
 *         still be available when the callback occurs.
 * @param  package_id - which package the notification comes from.
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/11/2011:   created
 *
 ****************************************************************/
fbe_status_t database_common_unregister_notification(fbe_notification_element_t *notification_element,
                                                     fbe_package_id_t            package_id)
{
    fbe_status_t status;

    status = fbe_database_send_packet_to_service(FBE_NOTIFICATION_CONTROL_CODE_UNREGISTER,
                                                 notification_element,
                                                 sizeof(fbe_notification_element_t),
                                                 FBE_SERVICE_ID_NOTIFICATION,
                                                 NULL, /* no sg list*/
                                                 0,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 package_id);
    if (status != FBE_STATUS_OK)
    {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: failed to unregister notification, status 0x%x.\n",
                        __FUNCTION__, status);
    }
    return FBE_STATUS_OK;        
}

/*!***************************************************************
 * @fn database_common_cmi_is_active()
 *****************************************************************
 * @brief
 *   return the state of CMI.  If CMI is busy, wait until CMI returns a valid state.
 *
 * @param  none
 *
 * @return fbe_boot_t - FBE_TRUE for active and FBE_FALSE for passive.
 *
 * @version
 *    04/29/2011:   created
 *
 ****************************************************************/
fbe_bool_t database_common_cmi_is_active(void)
{
    fbe_cmi_sp_state_t sp_state;
    fbe_bool_t         is_active = FBE_TRUE;

    fbe_cmi_get_current_sp_state(&sp_state);
    while(sp_state != FBE_CMI_STATE_ACTIVE && sp_state != FBE_CMI_STATE_PASSIVE && sp_state != FBE_CMI_STATE_SERVICE_MODE)
    {
        fbe_thread_delay(100);
        fbe_cmi_get_current_sp_state(&sp_state);
    }
    if (sp_state != FBE_CMI_STATE_ACTIVE) {
        is_active = FBE_FALSE;
    }

    return is_active;
}

/*!***************************************************************
 * @fn database_common_map_table_type_to_persist_sector
 *****************************************************************
 * @brief
 *   map database_config_table_type_t to fbe_persist_sector_type_t.
 *
 * @param  table_type
 *
 * @return fbe_persist_sector_type
 *
 * @version
 *    4/29/2011:   created
 *
 ****************************************************************/
fbe_persist_sector_type_t database_common_map_table_type_to_persist_sector(database_config_table_type_t table_type)
{
    switch (table_type) {
    case DATABASE_CONFIG_TABLE_USER:
        return FBE_PERSIST_SECTOR_TYPE_SEP_ADMIN_CONVERSION;

    case DATABASE_CONFIG_TABLE_OBJECT:
        return FBE_PERSIST_SECTOR_TYPE_SEP_OBJECTS;

    case DATABASE_CONFIG_TABLE_EDGE:
        return FBE_PERSIST_SECTOR_TYPE_SEP_EDGES;

    case DATABASE_CONFIG_TABLE_GLOBAL_INFO:
        return FBE_PERSIST_SECTOR_TYPE_SYSTEM_GLOBAL_DATA;

    default:
        return FBE_PERSIST_SECTOR_TYPE_INVALID;
    }
}

/*!***************************************************************
 * @fn database_common_connect_pvd_from_table
 *****************************************************************
 * @brief
 *   add pvds to be connected.
 *
 * @param  in_table_ptr - pointer to the table
 * @param  size - size of the table
 *
 * @return fbe_status_t - status
 *
 * @version
 *    04/06/2011:   created
 *
 ****************************************************************/

fbe_status_t database_common_connect_pvd_from_table(database_table_t *in_table_ptr, fbe_u32_t start_index, fbe_u32_t number_of_entries)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t i;
    database_object_entry_t *current_entry = NULL;
    database_pvd_operation_t  *pvd_operation;


    if (in_table_ptr == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                   FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                    "%s: NULL pointer!\n",
                   __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;        
    }
    /*validate the index*/
    if ((in_table_ptr->table_size < start_index)
        ||(in_table_ptr->table_size < (start_index + number_of_entries))) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                       "%s: start 0x%x or end 0x%x exceed table_size 0x%x!\n",
                       __FUNCTION__, 
                       start_index, 
                       (start_index + number_of_entries),
                       in_table_ptr->table_size);
        return FBE_STATUS_GENERIC_FAILURE;        

    }

    database_get_pvd_operation(&pvd_operation);
    current_entry = &in_table_ptr->table_content.object_entry_ptr[start_index];
    for (i = 0; i < number_of_entries; i++ ) {
        if (current_entry->header.state != DATABASE_CONFIG_ENTRY_VALID) { 
            /* not a valid entry, skip it*/
            /* move down to the next entry*/
            current_entry++;
            continue;
        }
        if(current_entry->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE){
            if (!fbe_database_validate_pvd(current_entry))
            {
                /* PVD is invalid, skip this drive */
                current_entry++;
                continue;
            }
            /*If it's a PVD then connect it to the LDO*/
            status = fbe_database_add_pvd_to_be_connected(pvd_operation, current_entry->header.object_id);
            if (status != FBE_STATUS_OK)
            {
                database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Failed to connect PVD to LDO!  Status 0x%x\n",
                               __FUNCTION__,status);			
            }
        }
        /* move down to the next entry*/
        current_entry++;
    }
    return FBE_STATUS_OK;        
}

/*!***************************************************************
 * @fn fbe_database_set_system_power_save_info
 *****************************************************************
 * @brief
 *   This function sets the system power saving information in the
 *   base_config.
 *
 * @param  
 *
 * @return FBE_STATUS_GENERIC_FAILURE if any issues.
 *
 * @version
 *    5/06/2011: Arun S - created
 *
 ****************************************************************/
fbe_status_t fbe_database_set_system_power_save_info(fbe_system_power_saving_info_t *system_power_save_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /* update the power saving information */
    status = fbe_database_send_control_packet_to_class(system_power_save_info, 
                                                       sizeof(fbe_system_power_saving_info_t), 
                                                       FBE_BASE_CONFIG_CONTROL_CODE_SET_SYSTEM_POWER_SAVING_INFO, 
                                                       FBE_PACKAGE_ID_SEP_0,
                                                       FBE_SERVICE_ID_TOPOLOGY,
                                                       FBE_CLASS_ID_BASE_CONFIG,
                                                       NULL,
                                                       0,
                                                       FBE_FALSE);

    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM,  
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s Failed to SET system power save info to base config.\n", 
                       __FUNCTION__);         
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_database_set_system_encryption_info
 *****************************************************************
 * @brief
 *   This function sets the system encryption information in the
 *   base_config.
 *
 * @param  
 *
 * @return FBE_STATUS_GENERIC_FAILURE if any issues.
 *
 * @version
 *
 ****************************************************************/
fbe_status_t fbe_database_set_system_encryption_info(fbe_system_encryption_info_t *system_encryption_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /* update the power saving information */
    status = fbe_database_send_control_packet_to_class(system_encryption_info, 
                                                       sizeof(fbe_system_encryption_info_t), 
                                                       FBE_BASE_CONFIG_CONTROL_CODE_SET_SYSTEM_ENCRYPTION_INFO, 
                                                       FBE_PACKAGE_ID_SEP_0,
                                                       FBE_SERVICE_ID_TOPOLOGY,
                                                       FBE_CLASS_ID_BASE_CONFIG,
                                                       NULL,
                                                       0,
                                                       FBE_FALSE);

    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM,  
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s Failed to SET system encryption info to base config.\n", 
                       __FUNCTION__);         
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_database_set_system_spare_info
 *****************************************************************
 * @brief
 *   This function sets the system spare information in the
 *   virtual drive class.
 *
 * @param  
 *
 * @return FBE_STATUS_GENERIC_FAILURE if any issues.
 *
 * @version
 *    6/02/2011: created
 *
 ****************************************************************/
fbe_status_t fbe_database_set_system_spare_info(fbe_system_spare_config_info_t *system_spare_info)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    /* update the power saving information */
    status = fbe_database_send_control_packet_to_class(system_spare_info, 
                                                       sizeof(fbe_system_spare_config_info_t), 
                                                       FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_UPDATE_SPARE_CONFIG, 
                                                       FBE_PACKAGE_ID_SEP_0,
                                                       FBE_SERVICE_ID_TOPOLOGY,
                                                       FBE_CLASS_ID_VIRTUAL_DRIVE,
                                                       NULL,
                                                       0,
                                                       FBE_FALSE);

    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM,  
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s Failed to SET system spare config info to virtual drive class.\n", 
                       __FUNCTION__);         
    }

    return status;
}

/*!**************************************************************
 * fbe_database_common_get_virtual_drive_exported_size()
 ****************************************************************
 * @brief
 * This function obtains a virtual drive exported capacity
 *
 * @param in_provisional_drive_exported_capacity - IN provisional drive
 * exported capacity
 * @param out_virtual_drive_imported_capacity - OUT virtual drive
 * exported capacity
 *
 * @return status - of call to get virtual drive exported capacity.
 *
 * @author
 *  06/07/2011 - Created
 *
 ****************************************************************/
fbe_status_t fbe_database_common_get_virtual_drive_exported_size(
    fbe_lba_t in_provisional_drive_exported_capacity, 
    fbe_lba_t *out_virtual_drive_exported_capacity)
{
    fbe_status_t                                           status = FBE_STATUS_OK;
    fbe_virtual_drive_control_class_calculate_capacity_t   virtual_drive_capacity_info;
 
    /* create the VD objects */
    virtual_drive_capacity_info.imported_capacity = in_provisional_drive_exported_capacity;

    /* @todo: must implement way to get the logical block geometry from the pvd */
    virtual_drive_capacity_info.block_edge_geometry = FBE_BLOCK_EDGE_GEOMETRY_520_520;

    database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM,  
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s entry\n", 
                   __FUNCTION__);

    /* exported capacity is filled by calling the VD class */
    virtual_drive_capacity_info.exported_capacity = 0;

    status = fbe_database_send_control_packet_to_class (&virtual_drive_capacity_info,
                                                        sizeof(fbe_virtual_drive_control_class_calculate_capacity_t), 
                                                        FBE_VIRTUAL_DRIVE_CONTROL_CODE_CLASS_CALCULATE_CAPACITY, 
                                                        FBE_PACKAGE_ID_SEP_0,
                                                        FBE_SERVICE_ID_TOPOLOGY,
                                                        FBE_CLASS_ID_VIRTUAL_DRIVE,
                                                        NULL,
                                                        0,
                                                        FBE_FALSE);
    database_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: Exported virtual drive capacity 0x%llx, status 0x%x\n", 
                   __FUNCTION__,
                   (unsigned long long)virtual_drive_capacity_info.exported_capacity,
           status);
    *out_virtual_drive_exported_capacity = virtual_drive_capacity_info.exported_capacity;
    return status;
}
/*************************************************************
 * end fbe_database_common_get_virtual_drive_exported_size()
 *************************************************************/

/*!**************************************************************
 * fbe_database_common_get_provision_drive_exported_size()
 ****************************************************************
 * @brief
 * This function obtains a provision drive exported capacity
 *
 * @param in_pdo_exported_capacity - IN pdo exported capacity
 * @param block_edge_geometry      - IN edge geometry
 * @param out_provision_drive_exported_capacity - OUT provision drive
 *                                                    exported capacity
 *
 * @return status - of call to get provision drive exported capacity.
 *
 * @author
 *  06/08/2011 - Created
 *
 ****************************************************************/
fbe_status_t fbe_database_common_get_provision_drive_exported_size(fbe_lba_t in_pdo_exported_capacity, 
                                                                   fbe_block_edge_geometry_t block_edge_geometry,
                                                                   fbe_lba_t *out_provision_drive_exported_capacity)
{
    fbe_status_t                                           status = FBE_STATUS_OK;
    fbe_provision_drive_control_class_calculate_capacity_t provision_drive_capacity_info;
 
    provision_drive_capacity_info.imported_capacity = in_pdo_exported_capacity;
    provision_drive_capacity_info.block_edge_geometry = block_edge_geometry;

    database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM,  
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s entry\n", 
                   __FUNCTION__);

    /* exported capacity is filled by calling the PVD class */
    provision_drive_capacity_info.exported_capacity = 0;

    status = fbe_database_send_control_packet_to_class (&provision_drive_capacity_info,
                                                        sizeof(fbe_provision_drive_control_class_calculate_capacity_t), 
                                                        FBE_PROVISION_DRIVE_CONTROL_CODE_CLASS_CALCULATE_CAPACITY, 
                                                        FBE_PACKAGE_ID_SEP_0,
                                                        FBE_SERVICE_ID_TOPOLOGY,
                                                        FBE_CLASS_ID_PROVISION_DRIVE,
                                                        NULL,
                                                        0,
                                                        FBE_FALSE);
    database_trace(FBE_TRACE_LEVEL_DEBUG_LOW,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: Exported virtual drive capacity 0x%llx, status 0x%x\n", 
                   __FUNCTION__,
                   (unsigned long long)provision_drive_capacity_info.exported_capacity,
           status);
    *out_provision_drive_exported_capacity = provision_drive_capacity_info.exported_capacity;
    return status;
}
/*************************************************************
 * end fbe_database_common_get_provision_drive_exported_size()
 *************************************************************/

/*!***************************************************************
 * @fn database_common_peer_is_alive()
 *****************************************************************
 * @brief
 *   is the peer there
 *
 * @param  none
 *
 * @return fbe_boot_t - FBE_TRUE for active and FBE_FALSE for passive.
 *
 * @version
 *    06/08/2011:   created
 *
 ****************************************************************/
fbe_bool_t database_common_peer_is_alive(void)
{
    return fbe_cmi_is_peer_alive();
}


fbe_status_t fbe_database_get_logical_objects_limits(fbe_environment_limits_t *logical_objects_limits)
{
    fbe_memory_get_env_limits(logical_objects_limits);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * database_common_clone_object_nonpaged_metadata_data()
 ******************************************************************************
 * @brief
 * This function clone the nonpaged metadata of the source object,
 * update with the destination object info and
 * set the destination object non-paged in the metadata service.  
 *
 *
 * @param soucre_object_id         - source object
 * @param destination_object_id    - destination object
 * @param dest_obj_generation_number - generation number of the destination object
 *
 * @return status - status of the operation
 *
 * @author
 *  10/06/2011 - Created. 
 ******************************************************************************/
fbe_status_t 
database_common_clone_object_nonpaged_metadata_data(fbe_object_id_t soucre_object_id, 
                                                    fbe_object_id_t destination_object_id,
                                                    fbe_config_generation_t dest_obj_generation_number)
{
    fbe_status_t                             status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_base_config_control_metadata_get_info_t  metadata_get_info;
    fbe_packet_t * packet = NULL;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_cpu_id_t cpu_id;

    status = fbe_database_send_packet_to_object(FBE_BASE_CONFIG_CONTROL_CODE_METADATA_GET_INFO,
                                                &metadata_get_info,
                                                sizeof(fbe_base_config_control_metadata_get_info_t),
                                                soucre_object_id,
                                                NULL, /* no sg-list*/
                                                0, /* no sg-list*/
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Failed to get metadata info of obj 0x%x, status: 0x%x.\n", 
                       __FUNCTION__,
                       soucre_object_id, status);
        return status;
    }


    status = fbe_base_config_class_set_nonpaged_metadata(dest_obj_generation_number,
                                                         destination_object_id, 
                                                         (fbe_u8_t *)&metadata_get_info.nonpaged_data.data);

    /* send to metadata service to preset the nonpaged metadata */
    /* Allocate packet */
    packet = fbe_transport_allocate_packet();
    if(packet == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                       "%s: allocate_packet failed\n", 
                       __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(packet);
    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_allocate_metadata_operation(sep_payload);
    cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(packet, cpu_id);

    fbe_payload_metadata_build_nonpaged_preset(metadata_operation, 
                                               (fbe_u8_t *)&metadata_get_info.nonpaged_data.data,
                                               FBE_PAYLOAD_METADATA_MAX_DATA_SIZE,
                                               destination_object_id);

    fbe_transport_set_completion_function(packet, fbe_database_metadata_nonpaged_preset_completion, NULL);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload);

    status =  fbe_metadata_operation_entry(packet);
    return status;
}

static fbe_status_t 
fbe_database_metadata_nonpaged_preset_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_status_t                        status;
    fbe_payload_metadata_status_t       metadata_status;

    sep_payload = fbe_transport_get_payload_ex(packet);
    metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
    
    status = fbe_transport_get_status_code(packet);
    fbe_payload_metadata_get_status(metadata_operation, &metadata_status);

    fbe_payload_ex_release_metadata_operation(sep_payload, metadata_operation);
    /* free the memory allocated */
    fbe_transport_release_packet(packet);
    if((status != FBE_STATUS_OK) || (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                       "%s: status: 0x%x, metadata_status: 0x%x\n", 
                       __FUNCTION__, status, metadata_status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/******************************************************************************
 * end database_common_clone_object_nonpaged_metadata_data()
 ******************************************************************************/

/*!***************************************************************
 * @fn database_export_lun(fbe_object_id_t lun_id)
 *****************************************************************
 * @brief
 *   exports the LUN device to upper layers
 *
 * @param  lun_id - the object ID of the LUN to export
 *
 * @return fbe_status_t - status
 *
 *
 ****************************************************************/
static fbe_status_t database_export_lun(fbe_object_id_t lun_id)
{


    return fbe_database_send_packet_to_object(FBE_LUN_CONTROL_CODE_EXPORT_DEVICE,
                                              NULL,
                                              0,
                                              lun_id,
                                              NULL,
                                              0,
                                              FBE_PACKET_FLAG_NO_ATTRIB,
                                              FBE_PACKAGE_ID_SEP_0);
}

fbe_status_t database_get_lun_zero_status(fbe_object_id_t object_id, fbe_lun_get_zero_status_t *zero_status, fbe_packet_t *packet)
{
    fbe_status_t                                status;

    zero_status->zero_percentage = 0;
    zero_status->zero_checkpoint = 0;

    status = fbe_database_forward_packet_to_object(packet,
                                                   FBE_LUN_CONTROL_CODE_GET_ZERO_STATUS,
                                                   zero_status,
                                                   sizeof(fbe_lun_get_zero_status_t),
                                                   object_id,
                                                   NULL,
                                                   0,
                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                   FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK && status != FBE_STATUS_NO_OBJECT)
    {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: unable to get object: 0x%x zero status\n", 
                        __FUNCTION__, object_id); 
    } 

    return status;
}

void database_set_lun_rotational_rate(database_object_entry_t* rg_object, fbe_u16_t* rotational_rate)
{
    if (rg_object->set_config_union.rg_config.raid_group_drives_type == FBE_DRIVE_TYPE_SAS_FLASH_HE
        || rg_object->set_config_union.rg_config.raid_group_drives_type == FBE_DRIVE_TYPE_SATA_FLASH_HE
        || rg_object->set_config_union.rg_config.raid_group_drives_type == FBE_DRIVE_TYPE_SAS_FLASH_ME
        || rg_object->set_config_union.rg_config.raid_group_drives_type == FBE_DRIVE_TYPE_SAS_FLASH_LE
        || rg_object->set_config_union.rg_config.raid_group_drives_type == FBE_DRIVE_TYPE_SAS_FLASH_RI
        ) 
    {
        /* if the raid group is on flash drive, set rotational_rate to 1. */
        *rotational_rate = 1;
    }
    else
    {
        *rotational_rate = 0;
    } 

    return;
}

/*!***************************************************************
 * @fn database_common_object_entry_size
 *****************************************************************
 * @brief
 *    return the object entry size. 
 *
 * @param class_id - the database class id 
 *
 * @return fbe_u32_t - the size of the object entry 
 *
 * @version
 *    4/25/2012: gaohp - created
 *
 ****************************************************************/
fbe_u32_t database_common_object_entry_size(database_class_id_t class_id)
{
    fbe_u32_t size = 0;

    switch(class_id) {
    case DATABASE_CLASS_ID_BVD_INTERFACE:
    case DATABASE_CLASS_ID_PROVISION_DRIVE:
    case DATABASE_CLASS_ID_VIRTUAL_DRIVE:
    case DATABASE_CLASS_ID_STRIPER:
    case DATABASE_CLASS_ID_MIRROR:
    case DATABASE_CLASS_ID_PARITY:
    case DATABASE_CLASS_ID_LUN:
    case DATABASE_CLASS_ID_EXTENT_POOL:
    case DATABASE_CLASS_ID_EXTENT_POOL_LUN:
    case DATABASE_CLASS_ID_EXTENT_POOL_METADATA_LUN:
        size = sizeof(database_object_entry_t);
        break;
    default:
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: invalid class id: %d\n", __FUNCTION__, class_id);
        break;
    }

    return size;
}

/*!***************************************************************
 * @fn database_common_user_entry_size
 *****************************************************************
 * @brief
 *    return the user entry size. 
 *
 * @param class_id - the database class id 
 *
 * @return fbe_u32_t - the size of the user entry 
 *
 * @version
 *    4/25/2012: gaohp - created
 *
 ****************************************************************/
fbe_u32_t database_common_user_entry_size(database_class_id_t class_id)
{
    fbe_u32_t base_size;
    fbe_u32_t size = 0;

    base_size = (fbe_u32_t)&((struct database_user_entry_s *)0)->user_data_union;

    switch(class_id) {
    case DATABASE_CLASS_ID_PROVISION_DRIVE:
        size = base_size + sizeof(database_pvd_user_data_t);
        break;
    case DATABASE_CLASS_ID_STRIPER:
    case DATABASE_CLASS_ID_MIRROR:
    case DATABASE_CLASS_ID_PARITY:
        size = base_size + sizeof(database_rg_user_data_t);
        break;
    case DATABASE_CLASS_ID_LUN:
        size = base_size + sizeof(database_lu_user_data_t);
        break;
    case DATABASE_CLASS_ID_EXTENT_POOL:
        size = base_size + sizeof(database_ext_pool_user_data_t);
        break;
    case DATABASE_CLASS_ID_EXTENT_POOL_LUN:
    case DATABASE_CLASS_ID_EXTENT_POOL_METADATA_LUN:
        size = base_size + sizeof(database_ext_pool_lun_user_data_t);
        break;
    default:
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: invalid class id: %d\n", __FUNCTION__, class_id);
        break;
    }

    return size;
}

fbe_u32_t database_common_edge_entry_size(void)
{
    return sizeof(database_edge_entry_t);
}

/*!***************************************************************
 * @fn database_common_global_info_entry_size
 *****************************************************************
 * @brief
 *    return the global info entry size. 
 *
 * @param type  - the global info type
 *
 * @return fbe_u32_t - the size of the entry
 *
 * @version
 *    4/25/2012: gaohp - created
 *
 ****************************************************************/
fbe_u32_t database_common_global_info_entry_size(database_global_info_type_t type)
{
    fbe_u32_t base_size;
    fbe_u32_t size = 0;

    base_size = (fbe_u32_t)&((struct database_global_info_entry_s *)0)->info_union;

    switch(type) {
    case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_POWER_SAVE:
        size = base_size + sizeof(fbe_system_power_saving_info_t);
        break;
    case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_SPARE:
        size = base_size + sizeof(fbe_system_spare_config_info_t);
        break;
    case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_GENERATION:
        size = base_size + sizeof(fbe_system_generation_info_t);
        break;
    case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_TIME_THRESHOLD:
        size = base_size + sizeof(fbe_system_time_threshold_info_t);
        break;
    case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_ENCRYPTION:
        size = base_size + sizeof(fbe_system_encryption_info_t);
        break;
    case DATABASE_GLOBAL_INFO_TYPE_GLOBAL_PVD_CONFIG:
        size = base_size + sizeof(fbe_global_pvd_config_t);
        break;
    default:
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: invalid global info type: %d\n", __FUNCTION__, type);
        break;
    }

    return size;
}

fbe_u32_t database_common_system_spare_entry_size(void)
{
    return sizeof(database_system_spare_entry_t);
}

/*!***************************************************************
 * @fn database_common_get_committed_object_entry_size
 *****************************************************************
 * @brief
 *    return the committed object entry size. 
 *
 * @param db_class_id- the database class id 
 * @param ret_size - the size of committed object entry 
 *
 * @return fbe_status_t -  return FBE_STATUS_GENERIC_FAILURE if any issues.
 *
 * @version
 *    4/25/2012: gaohp - created
 *
 ****************************************************************/
fbe_status_t database_common_get_committed_object_entry_size(database_class_id_t db_class_id, fbe_u32_t *ret_size)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t size = 0;

    fbe_spinlock_lock(&fbe_database_service.system_db_header_lock);
    switch(db_class_id) {
    case DATABASE_CLASS_ID_BVD_INTERFACE:
        size = fbe_database_service.system_db_header.bvd_interface_object_entry_size;
        break;
    case DATABASE_CLASS_ID_PROVISION_DRIVE:
        size = fbe_database_service.system_db_header.pvd_object_entry_size;
        break;
    case DATABASE_CLASS_ID_VIRTUAL_DRIVE:
        size = fbe_database_service.system_db_header.vd_object_entry_size;
        break;
    case DATABASE_CLASS_ID_STRIPER:
    case DATABASE_CLASS_ID_MIRROR:
    case DATABASE_CLASS_ID_PARITY:
        size = fbe_database_service.system_db_header.rg_object_entry_size;
        break;
    case DATABASE_CLASS_ID_LUN:
        size = fbe_database_service.system_db_header.lun_object_entry_size;
        break;
    case DATABASE_CLASS_ID_EXTENT_POOL:
        size = fbe_database_service.system_db_header.ext_pool_object_entry_size;
        break;
    case DATABASE_CLASS_ID_EXTENT_POOL_LUN:
    case DATABASE_CLASS_ID_EXTENT_POOL_METADATA_LUN:
        size = fbe_database_service.system_db_header.ext_pool_lun_object_entry_size;
        break;
    default:
        status = FBE_STATUS_GENERIC_FAILURE;
        size = 0;
        break;
    }
    fbe_spinlock_unlock(&fbe_database_service.system_db_header_lock);
    if (FBE_STATUS_OK != status) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: invalid class id: %d\n", __FUNCTION__, db_class_id);
    }

    *ret_size = size;
    return status;
}

/*!***************************************************************
 * @fn database_common_get_committed_user_entry_size
 *****************************************************************
 * @brief
 *    return the committed user entry size. 
 *
 * @param db_class_id- the database class id 
 * @param ret_size - the size of committed user entry 
 *
 * @return fbe_status_t -  return FBE_STATUS_GENERIC_FAILURE if any issues.
 *
 * @version
 *    4/25/2012: gaohp - created
 *
 ****************************************************************/
fbe_status_t database_common_get_committed_user_entry_size(database_class_id_t db_class_id, fbe_u32_t *ret_size)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t size = 0;

    fbe_spinlock_lock(&fbe_database_service.system_db_header_lock);
    switch(db_class_id) {
    case DATABASE_CLASS_ID_PROVISION_DRIVE:
        size = fbe_database_service.system_db_header.pvd_user_entry_size;
        break;
    case DATABASE_CLASS_ID_STRIPER:
    case DATABASE_CLASS_ID_MIRROR:
    case DATABASE_CLASS_ID_PARITY:
        size = fbe_database_service.system_db_header.rg_user_entry_size;
        break;
    case DATABASE_CLASS_ID_LUN:
        size = fbe_database_service.system_db_header.lun_user_entry_size;
        break;
    case DATABASE_CLASS_ID_EXTENT_POOL:
        size = fbe_database_service.system_db_header.ext_pool_user_entry_size;
        break;
    case DATABASE_CLASS_ID_EXTENT_POOL_LUN:
    case DATABASE_CLASS_ID_EXTENT_POOL_METADATA_LUN:
        size = fbe_database_service.system_db_header.ext_pool_lun_user_entry_size;
        break;
    default:
        status = FBE_STATUS_GENERIC_FAILURE;
        size = 0;
        break;
    }
    fbe_spinlock_unlock(&fbe_database_service.system_db_header_lock);

    if (FBE_STATUS_OK != status) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: invalid class id: %d\n", __FUNCTION__, db_class_id);
    }

    *ret_size = size;
    return status;
}

fbe_status_t database_common_get_committed_edge_entry_size(fbe_u32_t *ret_size)
{
    fbe_u32_t size = 0;

    fbe_spinlock_lock(&fbe_database_service.system_db_header_lock);
    size = fbe_database_service.system_db_header.edge_entry_size;
    fbe_spinlock_unlock(&fbe_database_service.system_db_header_lock);

    if (size == 0) {
        *ret_size = 0;
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *ret_size = size;
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn database_common_get_committed_global_info_entry_size
 *****************************************************************
 * @brief
 *    return the committed global entry size. 
 *
 * @param type - the global info entry type 
 * @param ret_size - the size of committed global entry 
 *
 * @return fbe_status_t -  return FBE_STATUS_GENERIC_FAILURE if any issues.
 *
 * @version
 *    4/25/2012: gaohp - created
 *
 ****************************************************************/
fbe_status_t database_common_get_committed_global_info_entry_size(database_global_info_type_t type, fbe_u32_t *ret_size)
{
    fbe_u32_t status = FBE_STATUS_OK;
    fbe_u32_t size = 0;

    fbe_spinlock_lock(&fbe_database_service.system_db_header_lock);
    switch(type) {
    case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_POWER_SAVE:
        size = fbe_database_service.system_db_header.power_save_info_global_info_entry_size;
        break;
    case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_SPARE:
        size = fbe_database_service.system_db_header.spare_info_global_info_entry_size;
        break;
    case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_GENERATION:
        size = fbe_database_service.system_db_header.generation_info_global_info_entry_size;
        break;
    case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_TIME_THRESHOLD:
        size = fbe_database_service.system_db_header.time_threshold_info_global_info_entry_size;
        break;
    case DATABASE_GLOBAL_INFO_TYPE_SYSTEM_ENCRYPTION:
        size = fbe_database_service.system_db_header.encryption_info_global_info_entry_size;
        break;
    case DATABASE_GLOBAL_INFO_TYPE_GLOBAL_PVD_CONFIG:
        size = fbe_database_service.system_db_header.pvd_config_global_info_entry_size;
        break;
    default:
        status = FBE_STATUS_GENERIC_FAILURE;
        size = 0;
        break;
    }
    fbe_spinlock_unlock(&fbe_database_service.system_db_header_lock);

    if (FBE_STATUS_OK != status) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: invalid global info type: %d\n", __FUNCTION__, type);
    }

    *ret_size = size;
    return status;
}

/*!***************************************************************
 * @fn fbe_database_get_pvd_for_vd
 *****************************************************************
 * @brief
 *   This function get pvd object id for a given virtual drive.
 *
 * @param  fbe_object_id_t vd_object_id
 * @param  fbe_object_id_t* pvd

 *
 * @return FBE_STATUS_GENERIC_FAILURE if any issues.
 *
 * @version
 *    5/01/2012: Vera Wang - created

 *
 ****************************************************************/
fbe_status_t fbe_database_get_pvd_for_vd(fbe_object_id_t vd_object_id, fbe_object_id_t* pvd, fbe_packet_t * packet)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_config_downstream_object_list_t vd_downstream_list;
    fbe_virtual_drive_configuration_t   get_configuration;

    status = fbe_database_forward_packet_to_object(packet,
                                                   FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_OBJECT_LIST,
                                                   &vd_downstream_list,
                                                   sizeof(vd_downstream_list),
                                                   vd_object_id,
                                                   NULL,
                                                   0,
                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                   FBE_PACKAGE_ID_SEP_0);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get downstream list for object 0x%x\n", 
                       __FUNCTION__,
                       vd_object_id);
        return status;
    }

    if(vd_downstream_list.number_of_downstream_objects == 1) 
    {
        *pvd = vd_downstream_list.downstream_object_list[0];

    }
    else if (vd_downstream_list.number_of_downstream_objects == 2) 
    {
        status = fbe_database_forward_packet_to_object(packet,
                                                       FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_CONFIGURATION,
                                                       &get_configuration,
                                                       sizeof(get_configuration),
                                                       vd_object_id,
                                                       NULL,
                                                       0,
                                                       FBE_PACKET_FLAG_NO_ATTRIB,
                                                       FBE_PACKAGE_ID_SEP_0);
        if (status != FBE_STATUS_OK) 
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get downstream list for object 0x%x\n", 
                           __FUNCTION__,
                           vd_object_id);
            return status;             
         }

        if (get_configuration.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE ||
            get_configuration.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) 
        {
            *pvd = vd_downstream_list.downstream_object_list[0];

        }
        else if (get_configuration.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE || 
                 get_configuration.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE) 
        {
            *pvd = vd_downstream_list.downstream_object_list[1];

        }
        else 
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to find downstream PVD for VD object 0x%x\n", 
                           __FUNCTION__,
                           vd_object_id);
            return FBE_STATUS_GENERIC_FAILURE;            
        }
    }
    else if (vd_downstream_list.number_of_downstream_objects == 0) 
    {  /* when VD has no downstram edge, we need a more clear trace */
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Detect Virtual Drive object 0x%x doesn't have downstream edge. \n",
                       __FUNCTION__, vd_object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Detect Virtual Drive object 0x%x has more than 2 downstream edges.\n", 
                       __FUNCTION__,
                       vd_object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}


/*!***************************************************************
 * @fn fbe_database_get_all_pvd_for_vd
 *****************************************************************
 * @brief
 *   This function get pvd object id for a given virtual drive.
 *
 * @param  fbe_object_id_t vd_object_id
 * @param  fbe_object_id_t* pvd

 *
 * @return FBE_STATUS_GENERIC_FAILURE if any issues.
 *
 * @version
 *    01/28/2014: Lili Chen - created

 *
 ****************************************************************/
fbe_status_t 
fbe_database_get_all_pvd_for_vd(fbe_object_id_t vd_object_id, fbe_object_id_t *source, fbe_object_id_t *dest, fbe_packet_t *packet)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_config_downstream_object_list_t vd_downstream_list;
    fbe_virtual_drive_configuration_t   get_configuration;

    status = fbe_database_forward_packet_to_object(packet,
                                                   FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_OBJECT_LIST,
                                                   &vd_downstream_list,
                                                   sizeof(vd_downstream_list),
                                                   vd_object_id,
                                                   NULL,
                                                   0,
                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                   FBE_PACKAGE_ID_SEP_0);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get downstream list for object 0x%x\n", 
                       __FUNCTION__,
                       vd_object_id);
        return status;
    }

    if(vd_downstream_list.number_of_downstream_objects == 1) 
    {
        *source = vd_downstream_list.downstream_object_list[0];

    }
    else if (vd_downstream_list.number_of_downstream_objects == 2) 
    {
        status = fbe_database_forward_packet_to_object(packet,
                                                       FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_CONFIGURATION,
                                                       &get_configuration,
                                                       sizeof(get_configuration),
                                                       vd_object_id,
                                                       NULL,
                                                       0,
                                                       FBE_PACKET_FLAG_NO_ATTRIB,
                                                       FBE_PACKAGE_ID_SEP_0);
        if (status != FBE_STATUS_OK) 
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get downstream list for object 0x%x\n", 
                           __FUNCTION__,
                           vd_object_id);
            return status;             
        }

        if (get_configuration.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE ||
            get_configuration.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE) 
        {
            *source = vd_downstream_list.downstream_object_list[0];
            *dest = vd_downstream_list.downstream_object_list[1];
        }
        else if (get_configuration.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE || 
                 get_configuration.configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE) 
        {
            *source = vd_downstream_list.downstream_object_list[1];
            *dest = vd_downstream_list.downstream_object_list[0];
        }
        else 
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to find downstream PVD for VD object 0x%x\n", 
                           __FUNCTION__,
                           vd_object_id);
            return FBE_STATUS_GENERIC_FAILURE;            
         }
    }
    else
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Detect Virtual Drive object 0x%x has more than 2 downstream edges.\n", 
                       __FUNCTION__,
                       vd_object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}

/***************************************************************
 * @fn fbe_database_get_pvd_list_inter_phase
 *****************************************************************
 * @brief
 *   This function get pvd list for a given raid group. This function
 *   is called by fbe_database_get_pvd_list.
 *
 * @param  fbe_object_id_t object_id
 * @param  fbe_object_id_t* pvd_list

 *
 * @return FBE_STATUS_GENERIC_FAILURE if any issues.
 *
 * @version
 *    5/01/2012: Vera Wang - created

 *
 ****************************************************************/
fbe_status_t fbe_database_get_pvd_list_inter_phase(fbe_object_id_t object_id, fbe_object_id_t* pvd_list, fbe_packet_t *packet)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_config_downstream_object_list_t rg_downstream_list;
    fbe_u32_t               index;
    database_object_entry_t   *object_entry = NULL;

    status = fbe_database_forward_packet_to_object(packet,
                                                   FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_OBJECT_LIST,
                                                   &rg_downstream_list,
                                                   sizeof(rg_downstream_list),
                                                   object_id,
                                                   NULL,
                                                   0,
                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                   FBE_PACKAGE_ID_SEP_0);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get downstream list for object 0x%x\n", 
                       __FUNCTION__,
                       object_id);
        return status;
    }

    for (index = 0; index < rg_downstream_list.number_of_downstream_objects; index++) 
    {
        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table, 
                                                                  rg_downstream_list.downstream_object_list[index],
                                                                  &object_entry);
        if (status != FBE_STATUS_OK) 
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get user entry for object 0x%x\n", 
                       __FUNCTION__,
                       rg_downstream_list.downstream_object_list[index]);
            return status;
        }

        if (object_entry->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE)
        {
            pvd_list[index] = rg_downstream_list.downstream_object_list[index];

        }
        else if (object_entry->db_class_id == DATABASE_CLASS_ID_VIRTUAL_DRIVE) 
        {            
            status = fbe_database_get_pvd_for_vd(rg_downstream_list.downstream_object_list[index],&pvd_list[index], packet);
            if ( status != FBE_STATUS_OK )
            {
                /* set to WARNING since it could be in the middle of swapping out the edge */
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: failed to get pvd list for object 0x%x\n", 
                               __FUNCTION__,
                               rg_downstream_list.downstream_object_list[index]);
                return status;
            }
        }
    }

    return status;
}


/*!***************************************************************
 * @fn fbe_database_get_pvd_list
 *****************************************************************
 * @brief
 *   This function get pvd list for a given raid group.
 *
 * @param  fbe_database_raid_group_info_t *rg_info
 *
 * @return FBE_STATUS_GENERIC_FAILURE if any issues.
 *
 * @version
 *    5/01/2012: Vera Wang - created
 *
 ****************************************************************/
fbe_status_t fbe_database_get_pvd_list(fbe_database_raid_group_info_t *rg_info, fbe_packet_t *packet)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_config_downstream_object_list_t rg_downstream_list;
    fbe_base_config_downstream_object_list_t internal_rg_downstream_list;
    fbe_u32_t               index;
    database_object_entry_t    *object_entry = NULL;
    fbe_object_id_t            object_id = rg_info->rg_object_id;

    status = fbe_database_forward_packet_to_object(packet,
                                                   FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_OBJECT_LIST,
                                                   &rg_downstream_list,
                                                   sizeof(rg_downstream_list),
                                                   object_id,
                                                   NULL,
                                                   0,
                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                   FBE_PACKAGE_ID_SEP_0);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get downstream list for object 0x%x\n", 
                       __FUNCTION__,
                       object_id);
        return status;
    }
    
    for (index = 0; index < rg_downstream_list.number_of_downstream_objects; index++) 
    {
        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table, 
                                                                  rg_downstream_list.downstream_object_list[index],
                                                                  &object_entry);
        if (status != FBE_STATUS_OK) 
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get user entry for object 0x%x\n", 
                       __FUNCTION__,
                       rg_downstream_list.downstream_object_list[index]);
            return status;
        }

        if(object_entry->db_class_id > DATABASE_CLASS_ID_RAID_START && object_entry->db_class_id < DATABASE_CLASS_ID_RAID_END)
        {
            status = fbe_database_forward_packet_to_object(packet,
                                                           FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_OBJECT_LIST,
                                                           &internal_rg_downstream_list,
                                                           sizeof(internal_rg_downstream_list),
                                                           rg_downstream_list.downstream_object_list[index],
                                                           NULL,
                                                           0,
                                                           FBE_PACKET_FLAG_NO_ATTRIB,
                                                           FBE_PACKAGE_ID_SEP_0);
            if ( status != FBE_STATUS_OK )
            {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: failed to get downstream list for object 0x%x\n", 
                               __FUNCTION__,
                               object_id);
                return status;
            }

            status = fbe_database_get_pvd_list_inter_phase(rg_downstream_list.downstream_object_list[index], rg_info->pvd_list+index*internal_rg_downstream_list.number_of_downstream_objects, packet);
            if ( status != FBE_STATUS_OK )
            {
                /* This ends up calling fbe_database_get_pvd_for_vd() and may fail 
                 * since it could be in the middle of swapping out the edge.  Set the
                 * trace to WARNING.
                 */
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: failed to get pvd list for downstream obj: 0x%x\n", 
                               __FUNCTION__,
                               rg_downstream_list.downstream_object_list[index]);
                return status;
            }
            rg_info->pvd_count = internal_rg_downstream_list.number_of_downstream_objects * rg_downstream_list.number_of_downstream_objects;
        }
        else if (object_entry->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE)
        {
            rg_info->pvd_list[index] = rg_downstream_list.downstream_object_list[index];
            rg_info->pvd_count = rg_downstream_list.number_of_downstream_objects; 
        }
        else if (object_entry->db_class_id == DATABASE_CLASS_ID_VIRTUAL_DRIVE) 
        {
            status = fbe_database_get_pvd_for_vd(rg_downstream_list.downstream_object_list[index],&rg_info->pvd_list[index], packet);
            if ( status != FBE_STATUS_OK )
            {
                /* set to WARNING since it could be in the middle of swapping out the edge */
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: failed to get pvd list for downstream Obj:0x%x\n", 
                               __FUNCTION__,
                               rg_downstream_list.downstream_object_list[index]);
                return status;
            }
            rg_info->pvd_count = rg_downstream_list.number_of_downstream_objects; 
        }
    }


    return status;
}
       
/*!***************************************************************
 * @fn fbe_database_get_lun_list
 *****************************************************************
 * @brief
 *   This function get lun list for a given raid group.
 *
 * @param  fbe_database_raid_group_info_t *rg_info
 *
 * @return FBE_STATUS_GENERIC_FAILURE if any issues.
 *
 * @version
 *    5/01/2012: Vera Wang - created
 *
 ****************************************************************/       
fbe_status_t fbe_database_get_lun_list(fbe_database_raid_group_info_t *rg_info, fbe_packet_t *packet)
{
    fbe_status_t            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_config_upstream_object_list_t rg_upstream_list;/*this is about 1K so it's not perfect on the stack but better than allocating on the fly from the memory service for large config*/
    fbe_u32_t               index, skip_luns = 0;
    database_user_entry_t   *user_entry = NULL;
    fbe_object_id_t			object_id = rg_info->rg_object_id;
    //fbe_lifecycle_state_t   get_lifecycle_state;

    status = fbe_database_forward_packet_to_object(packet,
                                                   FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                                   &rg_upstream_list,
                                                   sizeof(fbe_base_config_upstream_object_list_t),
                                                   object_id,
                                                   NULL,
                                                   0,
                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                   FBE_PACKAGE_ID_SEP_0);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get upstream list for object 0x%x\n", 
                       __FUNCTION__,
                       object_id);
        return status;
    }


    rg_info->lun_count = rg_upstream_list.number_of_upstream_objects;

    for (index = 0; index < rg_upstream_list.number_of_upstream_objects; index++) 
    {
        status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table, 
                                                                       rg_upstream_list.upstream_object_list[index],
                                                                       &user_entry);
        if (status != FBE_STATUS_OK || user_entry->header.state != DATABASE_CONFIG_ENTRY_VALID) 
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get user entry for object 0x%x\n", 
                       __FUNCTION__,
                       rg_upstream_list.upstream_object_list[index]);
            
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if (user_entry->db_class_id == DATABASE_CLASS_ID_LUN) 
        {
            #if 0
            status = fbe_database_get_object_lifecycle_state(rg_upstream_list.upstream_object_list[index], &get_lifecycle_state, packet);
            if (status != FBE_STATUS_OK || get_lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE) {
                rg_info->lun_count--;
                skip_luns++;
                continue;
            }
            #endif
            rg_info->lun_list[index-skip_luns] = rg_upstream_list.upstream_object_list[index];
        }
#ifndef NO_EXT_POOL_ALIAS
        else if (user_entry->db_class_id == DATABASE_CLASS_ID_EXTENT_POOL_LUN) 
        {
            rg_info->lun_list[index-skip_luns] = rg_upstream_list.upstream_object_list[index];
        }
#endif
        else
        {
            /* we have to skip if it's not LUN */
            rg_info->lun_count--;
            skip_luns++;
            continue;
        }
    }

    return FBE_STATUS_OK;       
}

/*!***************************************************************
 * @fn fbe_database_get_rgs_on_top_of_pvd
 ***********************************************************************************************
 * @brief
 *   This function get raid group list on top of a given pvd.
 *
 * @param  fbe_object_id_t pvd_id
 * @param  fbe_object_id_t *rg_list
 * @param  fbe_raid_group_number_t *rg_number_list
 * @param  fbe_u32_t max_rg
 * @param  fbe_u32_t *returned_rg
 *
 * @return FBE_STATUS_GENERIC_FAILURE if any issues.
 *
 * @version
 *    5/02/2012: Vera Wang - created
 *    5/22/2013: Zhipeng Hu - Do not include the RG if the PVD is the destination drive in a copy progress in that RG
 *
 ***********************************************************************************************/
fbe_status_t fbe_database_get_rgs_on_top_of_pvd(fbe_object_id_t pvd_id,
                                                fbe_object_id_t *rg_list,
                                                fbe_raid_group_number_t *rg_number_list,
                                                fbe_u32_t max_rg,
                                                fbe_u32_t *returned_rg,
                                                fbe_packet_t *packet)
{

    fbe_status_t									status;
    fbe_base_config_upstream_object_list_t    	 	upstream_object_list;/*this is about 1K so it's not perfect on the stack but better than allocating on the fly from the memory service for large config*/
    fbe_u32_t 										total_discovered = 0;
    database_object_entry_t *						vd_object_entry = NULL; 
    database_user_entry_t               *			rg_user = NULL;
    fbe_object_id_t									source_pvd;
    fbe_object_id_t									destination_pvd;

    if(pvd_id == FBE_OBJECT_ID_INVALID || rg_list == NULL || rg_number_list == NULL || returned_rg == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (packet == NULL) {
        /*get the list of objects on top of this PVD*/
        status = fbe_database_send_packet_to_object(FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                                    &upstream_object_list,
                                                    sizeof(fbe_base_config_upstream_object_list_t),
                                                    pvd_id,
                                                    NULL,
                                                    0,
                                                    FBE_PACKET_FLAG_NO_ATTRIB,
                                                    FBE_PACKAGE_ID_SEP_0);
    }else{
        status = fbe_database_forward_packet_to_object(packet,
                                                       FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                                       &upstream_object_list,
                                                       sizeof(fbe_base_config_upstream_object_list_t),
                                                       pvd_id,
                                                       NULL,
                                                       0,
                                                       FBE_PACKET_FLAG_NO_ATTRIB,
                                                       FBE_PACKAGE_ID_SEP_0);
    }

    if ( status != FBE_STATUS_OK ){
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: failed to get upstream_list for PVD 0x%X\n", __FUNCTION__,pvd_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*for each one object, see what kind it is and act based on that*/
    while (upstream_object_list.number_of_upstream_objects != 0 && max_rg != 0) {

        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                  upstream_object_list.upstream_object_list[upstream_object_list.number_of_upstream_objects - 1],
                                                                  &vd_object_entry);

        if ( status != FBE_STATUS_OK || !database_is_entry_valid(&vd_object_entry->header)){
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: can't get object entry for Object 0x%X\n", __FUNCTION__,upstream_object_list.upstream_object_list[upstream_object_list.number_of_upstream_objects - 1]);
    
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /*when it's a VD we need to go up*/
        if (vd_object_entry->db_class_id == DATABASE_CLASS_ID_VIRTUAL_DRIVE) {

            status = database_get_source_destination_drive_in_vd(vd_object_entry->header.object_id, &source_pvd, &destination_pvd, packet);
            if(status != FBE_STATUS_OK)
            {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: failed to get PC info for object 0x%X\n", __FUNCTION__,vd_object_entry->header.object_id);
                
                return FBE_STATUS_GENERIC_FAILURE;
            }

            /*if the VD is not in COPY progress (destination_pvd=FBE_OBJECT_ID_INVALID) or the PVD in question is
             * the source drive, report the RG in the returned RG list*/
            if(destination_pvd != pvd_id)
            {
                /*this function will continue to fill the array*/
                status = fbe_database_get_rgs_on_top_of_vd(upstream_object_list.upstream_object_list[upstream_object_list.number_of_upstream_objects - 1],
                                                           &rg_list,
                                                           &rg_number_list,
                                                           &total_discovered,
                                                           &max_rg,
                                                           packet);
        
                if ( status != FBE_STATUS_OK ){
                    database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: failed to get RG for object 0x%X\n", __FUNCTION__,upstream_object_list.upstream_object_list[upstream_object_list.number_of_upstream_objects - 1]);
            
                    return FBE_STATUS_GENERIC_FAILURE;
                }
            }
            
        }else if ((vd_object_entry->db_class_id > DATABASE_CLASS_ID_RAID_START) && (vd_object_entry->db_class_id < DATABASE_CLASS_ID_RAID_END)){
            /*this is easy, not a VD it should be the RG on top of the drives which is the case for PSL RGs*/
            status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                                            upstream_object_list.upstream_object_list[upstream_object_list.number_of_upstream_objects - 1],
                                                                            &rg_user); 
    
            if (status != FBE_STATUS_OK || rg_user->header.state != DATABASE_CONFIG_ENTRY_VALID) {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: failed to get RG user table entry for 0x%X\n",__FUNCTION__, upstream_object_list.upstream_object_list[upstream_object_list.number_of_upstream_objects - 1]);
                return status;
            }

            *rg_list = upstream_object_list.upstream_object_list[upstream_object_list.number_of_upstream_objects - 1];
            *rg_number_list = rg_user->user_data_union.rg_user_data.raid_group_number;

            rg_list++;/*go to next one*/
            rg_number_list++;
            max_rg--;/*less one to fill*/
            total_discovered++;
        } else if (vd_object_entry->db_class_id == DATABASE_CLASS_ID_EXTENT_POOL) {
#ifndef NO_EXT_POOL_ALIAS
            status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                                            upstream_object_list.upstream_object_list[upstream_object_list.number_of_upstream_objects - 1],
                                                                            &rg_user); 
    
            if (status != FBE_STATUS_OK || rg_user->header.state != DATABASE_CONFIG_ENTRY_VALID) {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: failed to get ext pool user table entry for 0x%X\n",__FUNCTION__, upstream_object_list.upstream_object_list[upstream_object_list.number_of_upstream_objects - 1]);
                return status;
            }

            *rg_list = upstream_object_list.upstream_object_list[upstream_object_list.number_of_upstream_objects - 1];
            *rg_number_list = rg_user->user_data_union.ext_pool_user_data.pool_id;

            rg_list++;/*go to next one*/
            rg_number_list++;
            max_rg--;/*less one to fill*/
            total_discovered++;
#endif
        }else{
            /*what else can be on top of a PVD ????????????*/
            
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: Class Id on top of PVD does not make sense:%d\n",__FUNCTION__, vd_object_entry->db_class_id );
            
            return FBE_STATUS_GENERIC_FAILURE;
        }

        upstream_object_list.number_of_upstream_objects --;/*go to next object*/
    }

    *returned_rg = total_discovered;
    
    return FBE_STATUS_OK;

}

/*!***************************************************************
 * @fn fbe_database_get_rgs_on_top_of_vd
 *****************************************************************
 * @brief
 *   This function get raid group list on top of a given virtual drive.
 *
 * @param  fbe_object_id_t vd_id
 * @param  fbe_object_id_t *rg_list
 * @param  fbe_u32_t *total_discovered
 * @param  fbe_u32_t *max_rg
 *
 * @return FBE_STATUS_GENERIC_FAILURE if any issues.
 *
 * @version
 *    5/02/2012: Vera Wang - created
 *
 ****************************************************************/
fbe_status_t fbe_database_get_rgs_on_top_of_vd(fbe_object_id_t vd_id,
                                               fbe_object_id_t **rg_list,
                                               fbe_raid_group_number_t **rg_number_list,
                                               fbe_u32_t *total_discovered,
                                               fbe_u32_t *max_rg,
                                               fbe_packet_t *packet)
{
    fbe_status_t 								status;
    fbe_base_config_upstream_object_list_t   	vd_upstream_object_list;/*this is about 1K so it's not perfect on the stack but better than allocating on the fly from the memory service for large config*/
    database_object_entry_t             *		rg_object_entry = NULL; 
    fbe_object_id_t 							rg_id;
    database_user_entry_t               *		rg_user = NULL;
    fbe_raid_group_number_t						rg_number;

    if (packet == NULL) {
        /*get the list of objects on top of this VD*/
        status = fbe_database_send_packet_to_object(FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                                    &vd_upstream_object_list,
                                                    sizeof(fbe_base_config_upstream_object_list_t),
                                                    vd_id,
                                                    NULL,
                                                    0,
                                                    FBE_PACKET_FLAG_NO_ATTRIB,
                                                    FBE_PACKAGE_ID_SEP_0);
    }else{
        status = fbe_database_forward_packet_to_object(packet,
                                                       FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                                       &vd_upstream_object_list,
                                                       sizeof(fbe_base_config_upstream_object_list_t),
                                                       vd_id,
                                                       NULL,
                                                       0,
                                                       FBE_PACKET_FLAG_NO_ATTRIB,
                                                       FBE_PACKAGE_ID_SEP_0);

    }

    if ( status != FBE_STATUS_OK ){
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: failed to get upstream_list for VD 0x%X\n", __FUNCTION__,vd_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    

    /*for each one object, see what kind it is and act based on that*/
    while (vd_upstream_object_list.number_of_upstream_objects != 0 && (*max_rg) != 0) {

        /*check what kind of RG we got there*/
        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                  vd_upstream_object_list.upstream_object_list[vd_upstream_object_list.number_of_upstream_objects - 1],
                                                                  &rg_object_entry);  

        if ( status != FBE_STATUS_OK || !database_is_entry_valid(&rg_object_entry->header)){
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: failed to get DB entry for RG 0x%X\n", __FUNCTION__,vd_upstream_object_list.upstream_object_list[vd_upstream_object_list.number_of_upstream_objects - 1]);
    
            return FBE_STATUS_GENERIC_FAILURE;
        }
    
           
        /*this is a RG, but we need to make sure this is not a mirror under striper first*/
        if (rg_object_entry->set_config_union.rg_config.raid_type == FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER) {
            /*need to climb up*/
            fbe_base_config_upstream_object_list_t    	* 	rg_upstream_object_list = (fbe_base_config_upstream_object_list_t*)fbe_transport_allocate_buffer();

            if (rg_upstream_object_list == NULL) {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: No Memory for R10 RG\n", __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            if (packet == NULL) {
                /*get the list of objects on top of this RG*/
                status = fbe_database_send_packet_to_object(FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                                            rg_upstream_object_list,
                                                            sizeof(fbe_base_config_upstream_object_list_t),
                                                            vd_upstream_object_list.upstream_object_list[vd_upstream_object_list.number_of_upstream_objects - 1],
                                                            NULL,
                                                            0,
                                                            FBE_PACKET_FLAG_NO_ATTRIB,
                                                            FBE_PACKAGE_ID_SEP_0);
            }else{
                status = fbe_database_forward_packet_to_object(packet,
                                                               FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                                               rg_upstream_object_list,
                                                               sizeof(fbe_base_config_upstream_object_list_t),
                                                               vd_upstream_object_list.upstream_object_list[vd_upstream_object_list.number_of_upstream_objects - 1],
                                                               NULL,
                                                               0,
                                                               FBE_PACKET_FLAG_NO_ATTRIB,
                                                               FBE_PACKAGE_ID_SEP_0);

            }

            if ( status != FBE_STATUS_OK ){
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                             "%s: failed to get upstream_list for RG 0x%X\n", __FUNCTION__,vd_upstream_object_list.upstream_object_list[vd_upstream_object_list.number_of_upstream_objects - 1]);
        
                fbe_transport_release_buffer(rg_upstream_object_list);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            if (rg_upstream_object_list->number_of_upstream_objects != 1) {
                if (rg_upstream_object_list->number_of_upstream_objects == 0) {
                    *total_discovered = 0;
                    fbe_transport_release_buffer(rg_upstream_object_list);
                    return FBE_STATUS_OK;    
                }
                else{
                    database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: Illegal numer of edges %d for RG 0x%X\n", __FUNCTION__, vd_upstream_object_list.upstream_object_list[vd_upstream_object_list.number_of_upstream_objects - 1], 0);
                    fbe_transport_release_buffer(rg_upstream_object_list);
                    return FBE_STATUS_GENERIC_FAILURE;
                }	
            }          

            status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                                           rg_upstream_object_list->upstream_object_list[0],
                                                                           &rg_user); 
    
            if (status != FBE_STATUS_OK || !database_is_entry_valid(&rg_user->header)) {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: failed to get RG user table entry for 0x%X\n",__FUNCTION__, rg_upstream_object_list->upstream_object_list[0]);
                fbe_transport_release_buffer(rg_upstream_object_list);
                return status;
            }

            if (rg_user->header.state == DATABASE_CONFIG_ENTRY_VALID) {
                rg_id = rg_upstream_object_list->upstream_object_list[0];/*this is the RG on top*/
                rg_number = rg_user->user_data_union.rg_user_data.raid_group_number;
            }else{
                rg_id = FBE_OBJECT_ID_INVALID;
            }
            fbe_transport_release_buffer(rg_upstream_object_list);
        }else{
            /*this was not a R10 so this was the Id of the RG on top of the VD*/
            status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                                           vd_upstream_object_list.upstream_object_list[vd_upstream_object_list.number_of_upstream_objects - 1],
                                                                           &rg_user); 
    
            if (status != FBE_STATUS_OK || !database_is_entry_valid(&rg_user->header)) {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: failed to get RG user table entry for 0x%X\n",__FUNCTION__, vd_upstream_object_list.upstream_object_list[vd_upstream_object_list.number_of_upstream_objects - 1]);
                
                return status;
            }

            if (rg_user->header.state == DATABASE_CONFIG_ENTRY_VALID) {
                rg_id = vd_upstream_object_list.upstream_object_list[vd_upstream_object_list.number_of_upstream_objects - 1];
                rg_number = rg_user->user_data_union.rg_user_data.raid_group_number;;
            }else{
                rg_id = FBE_OBJECT_ID_INVALID;
            }
        }

        if (rg_id != FBE_OBJECT_ID_INVALID) {
            /*we are good, we got the RG we wanted*/
            *(*rg_list) = rg_id;
            *(*rg_number_list) = rg_number;
            (*total_discovered)++;
            (*rg_list)++;
            (*rg_number_list)++;
            (*max_rg)--;
        }

        /*let's increment everyting for next one*/
        vd_upstream_object_list.number_of_upstream_objects--;
    }

    return FBE_STATUS_OK;

}

/*!***************************************************************
 * @fn fbe_database_get_rg_power_save_capable
 *****************************************************************
 * @brief
 *   This function get power_save_capable for a given raid group.
 *
 * @param  fbe_database_raid_group_info_t *rg_info
 *
 * @return FBE_STATUS_GENERIC_FAILURE if any issues.
 *
 * @version
 *    7/03/2012: Vera Wang - created
 *
 ****************************************************************/       
fbe_status_t fbe_database_get_rg_power_save_capable(fbe_database_raid_group_info_t *rg_info, fbe_packet_t *packet)
{
    fbe_status_t                status = FBE_STATUS_OK;
    fbe_u32_t                   index = 0;
    fbe_object_id_t			    object_id = FBE_OBJECT_ID_INVALID;
    fbe_provision_drive_info_t  provision_drive_info;

    if (rg_info == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: No Memory\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    object_id = rg_info->rg_object_id;
    /* before setting it, we initialize it to false. */
    rg_info->power_save_capable = FBE_TRUE;
    for (index = 0; index < rg_info->pvd_count; index++) 
    {
        /*power_save_capable.*/
        status = fbe_database_forward_packet_to_object(packet,
                                                       FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PROVISION_DRIVE_INFO,
                                                       &provision_drive_info,
                                                       sizeof(fbe_provision_drive_info_t),
                                                       rg_info->pvd_list[index],
                                                       NULL,
                                                       0,
                                                       FBE_PACKET_FLAG_INTERNAL,
                                                       FBE_PACKAGE_ID_SEP_0);
        if ( status != FBE_STATUS_OK )
        {
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get provision drive info for object 0x%X\n", 
                           __FUNCTION__,
                           rg_info->pvd_list[index]);

            return FBE_STATUS_OK;/*we will fail here for specialized objects and should not fail the whole get_rg for this*/
        }
        if (provision_drive_info.power_save_capable == FBE_FALSE) 
        {
            rg_info->power_save_capable = provision_drive_info.power_save_capable;
            break;
        }     
    }

    return FBE_STATUS_OK;       
}

/*!***************************************************************
 * @fn fbe_database_set_ps_stats
 *****************************************************************
 * @brief
 *   This function set power saving statistics for a given PVD.
 *
 * @param  fbe_database_raid_group_info_t *rg_info
 *
 * @return FBE_STATUS_GENERIC_FAILURE if any issues.
 *
 * @version
 *    7/09/2012: Vera Wang - created
 *
 ****************************************************************/ 
fbe_status_t fbe_database_set_ps_stats(fbe_database_control_update_ps_stats_t *ps_stats)
{
    fbe_status_t status;

    status = fbe_database_send_packet_to_object(FBE_PHYSICAL_DRIVE_CONTROL_CODE_SET_POWER_SAVING_STATS,
                                                &ps_stats->ps_stats,
                                                sizeof(fbe_physical_drive_power_saving_stats_t),
                                                ps_stats->object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_TRAVERSE | FBE_PACKET_FLAG_EXTERNAL,
                                                FBE_PACKAGE_ID_SEP_0);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                    "%s Failed to set power saving statistics to pvd object\n",
                   __FUNCTION__);          
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_database_zero_lun
 *****************************************************************
 * @brief
 *   This function zero a LUN.
 *
 * @param  fbe_object_id_t lun_object_id  the lun need to zero
 *         fbe_bool_t force_init_zero  true means ignore ndb_b flag and force
 *                                     init zero
 *
 * @return FBE_STATUS_GENERIC_FAILURE if any issues.
 *
 * @version
 *    16/07/2012: Jingcheng Zhang - created
 *
 ****************************************************************/ 
fbe_status_t fbe_database_zero_lun(fbe_object_id_t lun_object_id, fbe_bool_t force_init_zero)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_lun_init_zero_t init_lun_zero_req;

    init_lun_zero_req.force_init_zero = force_init_zero;

    /*send a init zero request to the object*/
    status = fbe_database_send_packet_to_object(FBE_LUN_CONTROL_CODE_INITIATE_ZERO,
                                                &init_lun_zero_req,
                                                sizeof(fbe_lun_init_zero_t),
                                                lun_object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if ( status != FBE_STATUS_OK )
    {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to init zero lun 0x%X\n", 
                           __FUNCTION__,
                           lun_object_id);
    }

    return status;
}

/*!**************************************************************
 * fbe_database_get_lun_imported_capacity()
 ****************************************************************
 * @brief
 * This function asks the lun class for the imported capacity 
 * base on a given exported capacity.  
 *
 * @param exported_capacity - the exported capacity in blocks. (IN)
 * @param imported_capacity - calculated imported capacity in blocks. (OUT)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  08/10/2012 - Created. Vera Wang
 ****************************************************************/
fbe_status_t fbe_database_get_lun_imported_capacity (fbe_lba_t exported_capacity,
                                                 fbe_lba_t lun_align_size,
                                                 fbe_lba_t *imported_capacity)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_lun_control_class_calculate_capacity_t calculate_capacity;

    calculate_capacity.exported_capacity = exported_capacity;
    calculate_capacity.lun_align_size = lun_align_size;

    status = fbe_database_send_control_packet_to_class (&calculate_capacity,
                                                        sizeof(fbe_lun_control_class_calculate_capacity_t),
                                                        FBE_LUN_CONTROL_CODE_CLASS_CALCULATE_IMPORTED_CAPACITY,
                                                        FBE_PACKAGE_ID_SEP_0,                                                 
                                                        FBE_SERVICE_ID_TOPOLOGY,
                                                        FBE_CLASS_ID_LUN,
                                                        NULL,
                                                        0,
                                                        FBE_FALSE);
    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                       "%s: failed.  Error 0x%x\n", 
                       __FUNCTION__, 
                       status);
        return status;
    }

    *imported_capacity = calculate_capacity.imported_capacity;
    return status;
}

/*!**************************************************************
 * fbe_database_get_lun_exported_capacity()
 ****************************************************************
 * @brief
 * This function asks the lun class for the exported capacity 
 * base on a given imported capacity.  
 *
 * @param imported_capacity - the exported capacity in blocks. (IN)
 *        lun_align_size(IN)
 * @param exported_capacity - calculated imported capacity in blocks. (OUT)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/05/2012 - Created. Vera Wang
 ****************************************************************/
fbe_status_t fbe_database_get_lun_exported_capacity (fbe_lba_t imported_capacity,
                                                     fbe_lba_t lun_align_size,
                                                     fbe_lba_t *exported_capacity)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_lun_control_class_calculate_capacity_t calculate_capacity;

    calculate_capacity.imported_capacity = imported_capacity;
    calculate_capacity.lun_align_size = lun_align_size;

    if (imported_capacity ==0 || imported_capacity < lun_align_size) {
        *exported_capacity = 0;
        return FBE_STATUS_OK;
    }

    status = fbe_database_send_control_packet_to_class (&calculate_capacity,
                                                        sizeof(fbe_lun_control_class_calculate_capacity_t),
                                                        FBE_LUN_CONTROL_CODE_CLASS_CALCULATE_EXPORTED_CAPACITY,
                                                        FBE_PACKAGE_ID_SEP_0,                                                 
                                                        FBE_SERVICE_ID_TOPOLOGY,
                                                        FBE_CLASS_ID_LUN,
                                                        NULL,
                                                        0,
                                                        FBE_FALSE);
    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                       "%s: failed.  Error 0x%x\n", 
                       __FUNCTION__, 
                       status);
        return status;
    }

    *exported_capacity = calculate_capacity.exported_capacity;
    return status;
}

/*!***************************************************************
 * @fn fbe_database_is_pvd_consumed_by_raid_group
 *****************************************************************
 * @brief
 *   check a PVD is in at least one raid group or not
 *
 * @param  fbe_object_id_t  pvd_id          IN
 * @param  fbe_bool_t *is_consumed      OUT
 *
 * @return FBE_STATUS_GENERIC_FAILURE if any issues.
 *
 * @version
 *    7/24/2012: Zhipeng HU - created
 *
 ****************************************************************/       
fbe_status_t    fbe_database_is_pvd_consumed_by_raid_group(fbe_object_id_t  pvd_id, fbe_bool_t *is_consumed)
{
    fbe_status_t    status;
    fbe_object_id_t     rg_object_id_array[MAX_RG_COUNT_ON_TOP_OF_PVD];
    fbe_raid_group_number_t rg_number_array[MAX_RG_COUNT_ON_TOP_OF_PVD];
    fbe_u32_t   rg_number = 0;

    if(NULL == is_consumed)
        return FBE_STATUS_GENERIC_FAILURE;

    status = fbe_database_get_rgs_on_top_of_pvd(pvd_id, &rg_object_id_array[0], 
                                                &rg_number_array[0], MAX_RG_COUNT_ON_TOP_OF_PVD, 
                                                &rg_number, NULL);
    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: failed to get rgs on top of pvd\n", 
                                __FUNCTION__);
        return status;
    }

    if(0 == rg_number)
        *is_consumed = FBE_FALSE;
    else
        *is_consumed = FBE_TRUE;

    return FBE_STATUS_OK;
    
}



/*!***************************************************************
 * @fn fbe_database_get_pdo_object_id_by_location
 *****************************************************************
 * @brief
 *   Get LDO object id
 * 
 *
 * @param  pvd - object id for PVD
 * @param  bus - PDO bus location
 * @param  enc - PDO enclosure location
 * @param  slot - PDO slot location
 * @param  drive_info - PDO info.  This will be updated
 *
 * @return fbe_status_t
 *
 * @version
 *    08/21/2012:   created He,Wei
 *
 ****************************************************************/
fbe_status_t fbe_database_get_pdo_object_id_by_location(fbe_u32_t bus,
                                                         fbe_u32_t enclosure,
                                                         fbe_u32_t slot,
                                                         fbe_object_id_t*   pdo_obj_id)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_control_get_physical_drive_by_location_t    topology_location;

    if (pdo_obj_id == NULL)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: illegal parameter(NULL pointer)\n",
                        __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* get pdo id based on location*/
    topology_location.port_number      = bus;
    topology_location.enclosure_number = enclosure;
    topology_location.slot_number      = slot;

    status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_GET_PHYSICAL_DRIVE_BY_LOCATION,
                                                 &topology_location, 
                                                 sizeof(topology_location),
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 NULL,  /* no sg list*/
                                                 0,  /* no sg list*/
                                                 FBE_PACKET_FLAG_EXTERNAL,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "%s: failed to get ObjID from %u_%u_%u,status 0x%x\n", 
                     __FUNCTION__, bus, enclosure, slot, status);

        *pdo_obj_id = FBE_OBJECT_ID_INVALID;
        return status;
    }
    
    *pdo_obj_id = topology_location.physical_drive_object_id;
    
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_get_serial_number_from_pdo
 *****************************************************************
 * @brief
 *   Get serial number from PDO
 * 
 *
 * @param  pdo - object id for PDO
 * @param  serial_num - PDO serial number
 *
 * @return fbe_status_t
 *
 * @version
 *    08/21/2012:   created He,Wei
 *
 ****************************************************************/
fbe_status_t fbe_database_get_serial_number_from_pdo(fbe_object_id_t pdo, 
                                                     fbe_u8_t*       serial_num)
{
    fbe_physical_drive_attributes_t            drive_attr;
    fbe_status_t                               status;

    status = fbe_database_send_packet_to_object(FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_ATTRIBUTES,
                                                &drive_attr, 
                                                sizeof(drive_attr),
                                                pdo,
                                                NULL,  /* no sg list*/
                                                0,  /* no sg list*/
                                                FBE_PACKET_FLAG_EXTERNAL,
                                                FBE_PACKAGE_ID_PHYSICAL);
    if( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "Failed to get attr for PDO 0x%x, status 0x%x\n", pdo, status);

        return status;
    }

    fbe_copy_memory(serial_num, drive_attr.initial_identify.identify_info, sizeof(fbe_u8_t)*FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE );

    serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE] ='\0';

    return status;
}

fbe_status_t fbe_database_init_lu_and_rg_number_generation(fbe_database_service_t *database_service_ptr)
{
    fbe_status_t status;

    status = fbe_private_space_layout_get_smallest_lun_number(&database_service_ptr->smallest_psl_lun_number);
    if ( status != FBE_STATUS_OK ){
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: failed to get smallest lu number from psl\n",__FUNCTION__);
        return status;
    }

    status = fbe_private_space_layout_get_smallest_rg_number(&database_service_ptr->smallest_psl_rg_number);
    if ( status != FBE_STATUS_OK ){
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: failed to get smallest rg number from psl\n",__FUNCTION__);
        return status;
    }

    /*now that we got the numbers, let's allocate memory once so we can use it all the time
    and don't have to alloacte every time we generate a number*/
    database_service_ptr->free_lun_number_bitmap = fbe_memory_ex_allocate(sizeof(fbe_u64_t) * (database_service_ptr->smallest_psl_lun_number / 64));
    database_service_ptr->free_rg_number_bitmap = fbe_memory_ex_allocate(sizeof(fbe_u64_t) * (database_service_ptr->smallest_psl_rg_number / 64));

    if (database_service_ptr->free_lun_number_bitmap == NULL || database_service_ptr->free_rg_number_bitmap == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: failed to allocate memory for bitmaps\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

fbe_status_t fbe_database_destroy_lu_and_rg_number_generation(fbe_database_service_t *database_service_ptr)
{
    fbe_memory_ex_release(database_service_ptr->free_lun_number_bitmap);
    fbe_memory_ex_release(database_service_ptr->free_rg_number_bitmap);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_get_total_objects_of_class
 *****************************************************************
 * @brief
 *   Get the number of objects in a certain class type.
 * 
 *
 * @param  class_id - class id 
 * @param  package_id - the target package
 * @param  total_objects_p - pointer to output object count.
 *
 * @return fbe_status_t
 *
 * @version
 *    10/26/2012:   created He,Wei
 *
 ****************************************************************/
fbe_status_t fbe_database_get_total_objects_of_class(fbe_class_id_t class_id, 
                                                     fbe_package_id_t package_id, 
                                                     fbe_u32_t *total_objects_p)
{
    fbe_status_t                                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_topology_control_get_total_objects_of_class_t   total_objects = {0};

    if (total_objects_p == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s:Illegal Parameter\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    total_objects.total_objects = 0;
    total_objects.class_id = class_id;

    status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_GET_TOTAL_OBJECTS_OF_CLASS,
                                                 &total_objects,
                                                 sizeof(fbe_topology_control_get_total_objects_of_class_t),
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 NULL,
                                                 0,
                                                 FBE_PACKET_FLAG_EXTERNAL,
                                                 package_id);

    if (status != FBE_STATUS_OK)
    {
        return status;
    }
    
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                   "Found %d object in class %d\n", 
                   total_objects.total_objects, total_objects.class_id);

    *total_objects_p = total_objects.total_objects;
    return status;
}


/*!***************************************************************
 * @fn fbe_database_enumerate_class
 *****************************************************************
 * @brief
 *   Get all objects id in a class type of a package.
 * 
 *
 * @param  class_id - class id 
 * @param  package_id - the target package
 * @param  obj_array_p - pointer to output object id array.
 * @param  num_objects_p - pointer to output object count.
 *
 * @return fbe_status_t
 *
 * @version
 *    10/26/2012:   created He,Wei
 *
 ****************************************************************/
fbe_status_t fbe_database_enumerate_class(fbe_class_id_t class_id,
                                          fbe_package_id_t package_id,
                                          fbe_object_id_t ** obj_array_p, 
                                          fbe_u32_t *num_objects_p)
{
        fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
        fbe_topology_control_enumerate_class_t  enumerate_class;

        fbe_sg_element_t*                       sg_list = NULL;
        fbe_sg_element_t*                       sg_element = NULL;
        fbe_u32_t                               alloc_data_size = 0;
        fbe_u32_t                               alloc_sg_size = 0;

        if (obj_array_p == NULL || num_objects_p == NULL)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s:Illegal Parameter\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
        status = fbe_database_get_total_objects_of_class(class_id, package_id, num_objects_p);
        if ( status != FBE_STATUS_OK ){
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: Get objects number failed\n", 
                           __FUNCTION__);
            return status;
        }
    
        if ( *num_objects_p == 0 ){
            *obj_array_p  = NULL;
            return status;
        }

        alloc_data_size = *num_objects_p * sizeof (fbe_object_id_t);
        alloc_sg_size   = FBE_TOPLOGY_CONTROL_ENUMERATE_CLASS_SG_COUNT*sizeof (fbe_sg_element_t);
        sg_list = (fbe_sg_element_t *)fbe_memory_ex_allocate(alloc_sg_size);
        if (sg_list == NULL) {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: fail to allocate memory for sg list\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        
        *obj_array_p = (fbe_object_id_t *)fbe_memory_ex_allocate(alloc_data_size);
        if (*obj_array_p == NULL) {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                        "%s: fail to allocate memory for %d objects\n", __FUNCTION__, *num_objects_p);
            fbe_memory_ex_release(sg_list);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        sg_element = sg_list;
        fbe_sg_element_init(sg_element, alloc_data_size, (fbe_u8_t *)*obj_array_p);
        fbe_sg_element_increment(&sg_element);
        fbe_sg_element_terminate(sg_element);
        
        enumerate_class.number_of_objects = *num_objects_p;
        enumerate_class.class_id = class_id;
        enumerate_class.number_of_objects_copied = 0;
        
        status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_ENUMERATE_CLASS,
                                                     &enumerate_class,
                                                     sizeof(fbe_topology_control_enumerate_class_t),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     sg_list,  
                                                     FBE_TOPLOGY_CONTROL_ENUMERATE_CLASS_SG_COUNT,  
                                                     FBE_PACKET_FLAG_EXTERNAL,
                                                     package_id);
        if (status != FBE_STATUS_OK) {
            fbe_memory_ex_release(sg_list);
            /*fbe_memory_ex_release(*obj_array_p);*/
            return status;
        }

        if ( enumerate_class.number_of_objects_copied < *num_objects_p )
        {
            *num_objects_p = enumerate_class.number_of_objects_copied;
        }
        
        fbe_memory_ex_release(sg_list);
        return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_get_esp_object_id
 *****************************************************************
 * @brief
 *   Get all objects id in a class type of a package.
 * 
 *
 * @param  class_id - class id 
 * @param  object_id - pointer to output object id.
 *
 * @return fbe_status_t
 *
 * @version
 *    10/26/2012:   created He,Wei
 *
 ****************************************************************/

fbe_status_t fbe_database_get_esp_object_id(fbe_class_id_t esp_class_id,
                                            fbe_object_id_t *object_id)
{
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t           actual_objects = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t     *obj_list_p = NULL;

    if (object_id == NULL) 
    {
    
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s:Illegal Parameter\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_database_enumerate_class(esp_class_id,  
                                          FBE_PACKAGE_ID_ESP,
                                          &obj_list_p, 
                                          &actual_objects);

    if (status != FBE_STATUS_OK) 
    {
        if(obj_list_p != NULL) 
        {
            fbe_memory_ex_release(obj_list_p);
        }
        
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: packet error:%d\n", __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*any class in ESP has only one object*/
    if(actual_objects != 1) 
    {
    
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: ESP objects are singleton. Class:%d, No. of Objects:%d\n", 
                       __FUNCTION__, esp_class_id, actual_objects);

        fbe_memory_ex_release(obj_list_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *object_id = *obj_list_p;

    fbe_memory_ex_release(obj_list_p);
    return status;
}

/*!***************************************************************
 * @fn fbe_database_get_object_lifecycle_state()
 *****************************************************************
 * @brief
 *   Get the lifecycle state for a given object
 * 
 *
 * @param  object_id - class id(IN)
 *         lifecycle_state     (OUT)
 *
 * @return fbe_status_t
 *
 * @version
 *    11/19/2012:   created Vera Wang
 *
 ****************************************************************/
fbe_status_t fbe_database_get_object_lifecycle_state(fbe_object_id_t object_id, fbe_lifecycle_state_t * lifecycle_state, fbe_packet_t *packet)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_object_mgmt_get_lifecycle_state_t      get_lifecycle_state;   

    if (lifecycle_state == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s:Illegal Parameter\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_database_forward_packet_to_object(packet,
                                                   FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
                                                   &get_lifecycle_state,
                                                   sizeof(fbe_base_object_mgmt_get_lifecycle_state_t),
                                                   object_id,
                                                   NULL,
                                                   0,
                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                   FBE_PACKAGE_ID_SEP_0);

    if (status == FBE_STATUS_NO_OBJECT) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: requested object 0x%x does not exist\n", __FUNCTION__, object_id);
        return status;
    }

    /* If object is in SPECIALIZE STATE topology will return FBE_STATUS_BUSY */
    if (status == FBE_STATUS_BUSY) {
        *lifecycle_state = FBE_LIFECYCLE_STATE_SPECIALIZE;
        return FBE_STATUS_OK;
    }

    if (status != FBE_STATUS_OK) {
        database_trace (FBE_TRACE_LEVEL_WARNING, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                        "%s:fail to get lifecycle state for object 0x%x, status:%dn", 
                        __FUNCTION__,object_id, status);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    *lifecycle_state = get_lifecycle_state.lifecycle_state;

    return status;
}

fbe_status_t fbe_database_forward_packet_to_object(fbe_packet_t *packet,
                                                   fbe_payload_control_operation_opcode_t control_code,
                                                fbe_payload_control_buffer_t buffer,
                                                fbe_payload_control_buffer_length_t buffer_length,
                                                fbe_object_id_t object_id,
                                                fbe_sg_element_t *sg_list,
                                                fbe_u32_t sg_list_count,
                                                fbe_packet_attr_t attr,
                                                fbe_package_id_t package_id)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE, send_status;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_cpu_id_t  						cpu_id;		
    fbe_semaphore_t						sem;/* we use our own and our own completion function in case the packet is already using it's own semaphore*/
    fbe_packet_attr_t					original_attr;/*preserve the sender attribute*/

    fbe_semaphore_init(&sem, 0, 1);
    
    cpu_id = fbe_get_cpu_id();
    fbe_transport_set_cpu_id(packet, cpu_id);


    payload = fbe_transport_get_payload_ex(packet);
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
    
    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_get_packet_attr(packet, &original_attr);
    fbe_transport_clear_packet_attr(packet, original_attr);/*remove the old one, we'll restore it later*/
    fbe_transport_set_packet_attr(packet, attr);

    fbe_transport_set_completion_function(packet, database_forward_packate_completion, (fbe_packet_completion_context_t) &sem);

    send_status = fbe_service_manager_send_control_packet(packet);
    if ((send_status != FBE_STATUS_OK)&&(send_status != FBE_STATUS_NO_OBJECT) && (send_status != FBE_STATUS_PENDING) && (send_status != FBE_STATUS_MORE_PROCESSING_REQUIRED)) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: error in sending 0x%x to obj 0x%x, status:0x%x\n",
                       __FUNCTION__, control_code, object_id, send_status);  
    }

    fbe_semaphore_wait(&sem, NULL);

    /* Save the status for returning.*/
    status = fbe_transport_get_status_code(packet);
    database_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: 0x%x to obj 0x%x, packet status:0x%x\n",
                   __FUNCTION__, control_code, object_id, status);  

    fbe_payload_control_get_status(control_operation, &control_status);
    if (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK ) {
        database_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: 0x%x to obj 0x%x, return payload status:0x%x\n",
                       __FUNCTION__, control_code, object_id, control_status);

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
        database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Set return status to 0x%x\n",
                       __FUNCTION__, status);  
    }

    fbe_transport_set_packet_attr(packet, original_attr);
    fbe_semaphore_destroy(&sem);
    fbe_payload_ex_release_control_operation(payload, control_operation);
    return status;
}


static fbe_status_t database_forward_packate_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem = (fbe_semaphore_t *)context;
    fbe_semaphore_release(sem, 0, 1, FBE_FALSE);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;/*important, otherwise we'll complete the original one*/
}

/*!*******************************************************************
 * database_get_source_destination_drive_in_vd()
 *********************************************************************
 * @brief
 * This function gets the source drive's PVD and the destination drive's PVD from
 * a specified VD. If the VD is not in COPY, then the returned destination PVD would be
 * set to FBE_OBJECT_ID_INVALID.
 *
 * @param[in]	vd_object
 * @param[out]	src_pvd
 * @param[out]	dest_pvd
 * @param[in]	packet (this input can be NULL)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  05/22/2013 - Created. Zhipeng Hu
 *********************************************************************/
static fbe_status_t database_get_source_destination_drive_in_vd(fbe_object_id_t vd_object, fbe_object_id_t* src_pvd, fbe_object_id_t* dest_pvd, fbe_packet_t* packet)
{
    fbe_status_t	status;
    fbe_virtual_drive_get_info_t virtual_drive_info;

    if(vd_object == FBE_OBJECT_ID_INVALID || src_pvd == NULL || dest_pvd == NULL)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *src_pvd = *dest_pvd = FBE_OBJECT_ID_INVALID;

    if (packet == NULL)
    {
        status = fbe_database_send_packet_to_object(FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_INFO,
                                                    &virtual_drive_info,
                                                    sizeof(virtual_drive_info),
                                                    vd_object,
                                                    NULL,
                                                    0,
                                                    FBE_PACKET_FLAG_NO_ATTRIB,
                                                    FBE_PACKAGE_ID_SEP_0);
    }
    else
    {
        status = fbe_database_forward_packet_to_object(packet,
                                                   FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_INFO,
                                                   &virtual_drive_info,
                                                   sizeof(virtual_drive_info),
                                                   vd_object,
                                                   NULL,
                                                   0,
                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                   FBE_PACKAGE_ID_SEP_0);
    }
    
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to get vd info for object 0x%x\n", 
            __FUNCTION__,
            vd_object);
        return status;
    }

    *src_pvd = virtual_drive_info.original_pvd_object_id;
    *dest_pvd = virtual_drive_info.spare_pvd_object_id;
    
    return status;
}

/*!***************************************************************************
 *          fbe_database_determine_block_count_for_io()
 ***************************************************************************** 
 * 
 * @brief   Using the physical drive info determine and populate the number
 *          of block for the I/O.
 * 
 * @param   database_drive_info_p - Pointer to drive info used to determine
 *              request size.
 * @param   data_length - Number of data bytes required
 * @param   block_count_for_io_p - Address of block count to update
 *
 * @return  fbe_status_t
 *
 * @version
 *  12/03/2013  Ron Proulx  - Created.
 *
 ****************************************************************/
fbe_status_t fbe_database_determine_block_count_for_io(database_physical_drive_info_t *database_drive_info_p,
                                                       fbe_u64_t data_length,
                                                       fbe_block_count_t *block_count_for_io_p)
{
    fbe_block_count_t block_count;

    /* There are only (2) block geometry supported.
     */
    block_count = (data_length + FBE_BYTES_PER_BLOCK - 1) / FBE_BYTES_PER_BLOCK;
    switch(database_drive_info_p->block_geometry)
    {
        case FBE_BLOCK_EDGE_GEOMETRY_520_520:
            *block_count_for_io_p = block_count;
            break;
        case FBE_BLOCK_EDGE_GEOMETRY_4160_4160:
            block_count = ((block_count + 7) / 8) * 8;
            *block_count_for_io_p = block_count;
            break;
        default:
            /*! @note May want to change to FBE_TRACE_LEVEL_CRITICAL_ERROR 
             *        for debug purposes.
             */
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: %d_%d_%d unsupported block geometry: %d\n", 
                           __FUNCTION__, database_drive_info_p->port_number,
                           database_drive_info_p->enclosure_number, database_drive_info_p->slot_number,
                           database_drive_info_p->block_geometry);
            return FBE_STATUS_GENERIC_FAILURE;
    }
    return FBE_STATUS_OK;
}
/*************************************************
 * end fbe_database_determine_block_count_for_io()
 *************************************************/

/*!***************************************************************************
 *          fbe_database_send_encryption_change_notification()
 ***************************************************************************** 
 * 
 * @brief   generate encryption state change notification with specific reason
 * 
 * @param   reason - change reason
 *
 * @return  fbe_status_t
 *
 * @version
 *
 ****************************************************************/
fbe_status_t fbe_database_send_encryption_change_notification(database_encryption_notification_reason_t reason)
{
    fbe_notification_info_t             notification_info;
    fbe_status_t                        status;

    fbe_zero_memory(&notification_info, sizeof(fbe_notification_info_t));
    notification_info.notification_type = FBE_NOTIFICATION_TYPE_ENCRYPTION_STATE_CHANGED;    
    notification_info.object_type       = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
    notification_info.class_id          = FBE_CLASS_ID_INVALID;

    notification_info.notification_data.encryption_info.encryption_state = FBE_BASE_CONFIG_ENCRYPTION_STATE_INVALID;
    notification_info.notification_data.encryption_info.object_id        = FBE_OBJECT_ID_INVALID;
    notification_info.notification_data.encryption_info.control_number   = (fbe_u32_t) reason;

    status = fbe_notification_send(FBE_OBJECT_ID_INVALID, notification_info);
    if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Notification %d send Failed, status %d \n", __FUNCTION__, reason, status);
    }
    return status;
}
/*************************************************
 * end fbe_database_send_encryption_change_notification()
 *************************************************/

/*!****************************************************************************
 * fbe_database_shoot_drive()
 ******************************************************************************
 * @brief
 *  Shoot the specified drive
 *
 * @param pdo_object_id - object of the pdo to kill
 *
 * @return status
 *
 * @author 
 *   2/6/2014 - Created. Deanna Heng
 ******************************************************************************/
fbe_status_t fbe_database_shoot_drive(fbe_object_id_t pdo_object_id)
{
    fbe_base_physical_drive_death_reason_t reason = FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_VAULT_CONFIGURATION;
    fbe_status_t status = FBE_STATUS_OK;

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: shooting drive 0x%x!!!\n",
                   __FUNCTION__, pdo_object_id);

    status = fbe_database_send_packet_to_object(FBE_PHYSICAL_DRIVE_CONTROL_CODE_FAIL_DRIVE,
                                                &reason,
                                                sizeof(fbe_base_physical_drive_death_reason_t),
                                                pdo_object_id,
                                                NULL,  /* no sg list*/
                                                0,  /* no sg list*/
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_PHYSICAL);

    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to shoot pdo 0x%x. status 0x%x\n", 
                       __FUNCTION__, pdo_object_id, status);
        return status;
    }

    return status;
}

/*!****************************************************************************
 * fbe_database_send_kill_drive_request_to_peer()
 ******************************************************************************
 * @brief
 *  Tell the peer to shoot the specified drive if it is alive
 *
 * @param pdo_object_id - object of the pdo to kill
 *
 * @return status
 *
 * @author 
 *   2/6/2014 - Created. Deanna Heng
 ******************************************************************************/
fbe_status_t fbe_database_send_kill_drive_request_to_peer(fbe_object_id_t pdo_object_id) 
{
    if(is_peer_update_allowed(&fbe_database_service))
    {
        return fbe_database_cmi_send_kill_drive_request_to_peer(pdo_object_id); 
    }

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_database_process_killed_drive_needed
 ******************************************************************************
 * @brief
 *  determine if there is a drive the passive sp needs to kill
 *
 * @param pdo__id - object of the pdo to kill
 *
 * @return status
 *
 * @author 
 *   2/6/2014 - Created. Deanna Heng
 ******************************************************************************/
fbe_bool_t fbe_database_process_killed_drive_needed(fbe_object_id_t * pdo_id_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t bus = 0;
    fbe_u32_t encl = 0;
    fbe_u32_t slot = 0;
    fbe_discovery_transport_control_get_death_reason_t get_death_reason = {0};

    for (slot = 0; slot <= FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER; slot++) {
        fbe_database_get_pdo_object_id_by_location(bus, encl, slot, pdo_id_p);
        if (*pdo_id_p != FBE_OBJECT_ID_INVALID) {

            status = fbe_database_send_packet_to_object(FBE_DISCOVERY_TRANSPORT_CONTROL_CODE_GET_DEATH_REASON,
                                                 &get_death_reason, 
                                                 sizeof(get_death_reason),
                                                 *pdo_id_p,
                                                 NULL,
                                                 0,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 FBE_PACKAGE_ID_PHYSICAL);

            if( status != FBE_STATUS_OK )
            {
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s: failed to get death reason from %u_%u_%u pdo 0x%x,status 0x%x\n", 
                             __FUNCTION__, bus, encl, slot, *pdo_id_p, status);
                return FBE_FALSE;
            }

            if (get_death_reason.death_reason == FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_INVALID_VAULT_CONFIGURATION) {
                return FBE_TRUE;
            }
   
        }
    }

    return FBE_FALSE;
}

/*!****************************************************************************
 * fbe_database_is_4k_drive_committed
 ******************************************************************************
 * @brief
 *  This function determines if 4K drives are committed based on the commit level
 *
 * @param None
 *
 * @return FBE_TRUE - If 4K is supported. FBE_FALSE - Otherwise
 *
 * @author 
 *   04/18/2014 - Created. Ashok Tamilarasan
 ******************************************************************************/
fbe_bool_t fbe_database_is_4k_drive_committed(void)
{
    fbe_u64_t        current_commit_level;

    fbe_spinlock_lock(&fbe_database_service.system_db_header_lock);
    current_commit_level = fbe_database_service.system_db_header.persisted_sep_version;
    fbe_spinlock_unlock(&fbe_database_service.system_db_header_lock);

    if (current_commit_level >= FLARE_COMMIT_LEVEL_ROCKIES_4K_DRIVE_SUPPORT)
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}

/*!****************************************************************************
 * fbe_database_get_next_edge_index_for_extent_pool_lun
 ******************************************************************************
 * @brief
 *  This function gets the next edge index for extent pool lun.
 *
 * @param pool_id - pool id.
 *
 * @return fbe_u32_t
 *
 * @author 
 *   06/13/2014 - Created. Lili Chen
 ******************************************************************************/
fbe_status_t fbe_database_get_next_edge_index_for_extent_pool_lun(fbe_u32_t pool_id, fbe_u32_t *index, fbe_lba_t *offset)
{
    fbe_status_t                     		status = FBE_STATUS_GENERIC_FAILURE;
    database_object_entry_t *	            object_entry = NULL;
    database_user_entry_t *                 user_entry = NULL;
    fbe_object_id_t	                        object_id;
    fbe_lba_t                               last_offset = 0, last_capacity = 0;
    fbe_edge_index_t                        last_index = 0;

    /* go over the tables */
    for (object_id = 0; object_id < fbe_database_service.object_table.table_size; object_id++) {
        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                  object_id,
                                                                  &object_entry);  

        if (status != FBE_STATUS_OK || !database_is_entry_valid(&object_entry->header)){
            continue;
        }
        if ((object_entry->db_class_id != DATABASE_CLASS_ID_EXTENT_POOL_LUN) && 
            (object_entry->db_class_id != DATABASE_CLASS_ID_EXTENT_POOL_METADATA_LUN)) 
        {
            continue;
        }

        status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                                       object_id,
                                                                       &user_entry);
        if (status != FBE_STATUS_OK || user_entry == NULL) {
            continue;
        }
        if (pool_id == user_entry->user_data_union.ext_pool_lun_user_data.pool_id){
            if ((last_index == 0) ||
                (object_entry->set_config_union.ext_pool_lun_config.server_index > last_index)){
                last_index = object_entry->set_config_union.ext_pool_lun_config.server_index;
                last_offset = object_entry->set_config_union.ext_pool_lun_config.offset;
                last_capacity = object_entry->set_config_union.ext_pool_lun_config.capacity;
            }
        }
    }

    *offset = last_offset + last_capacity;
    *index = last_index + 1;
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_set_ext_pool_config
 *****************************************************************
 * @brief
 *   set ext pool config .
 *
 * @param object_id - the object id to set the config
 * @param config - the config
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *   06/20/2014:   created
 *
 ****************************************************************/
fbe_status_t fbe_database_set_ext_pool_config(fbe_object_id_t  object_id, fbe_extent_pool_configuration_t *config)
{
    fbe_status_t status;

    /* Configure the raid group portion of the virtual drive. */
    status = fbe_database_send_packet_to_object(FBE_EXTENT_POOL_CONTROL_CODE_SET_CONFIGURATION,
                                                config,
                                                sizeof(fbe_extent_pool_configuration_t),
                                                object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if(status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s Failed to send config to pvd object 0x%x\n",
                   __FUNCTION__, object_id);       
    }
    return status;
}

/*!***************************************************************
 * @fn fbe_database_set_ext_pool_lun_config
 *****************************************************************
 * @brief
 *   set ext pool lun config .
 *
 * @param object_id - the object id to set the config
 * @param config - the config
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *   06/20/2014:   created
 *
 ****************************************************************/
fbe_status_t fbe_database_set_ext_pool_lun_config(fbe_object_id_t  object_id, fbe_ext_pool_lun_configuration_t *config)
{
    fbe_status_t status;

    /* Configure the raid group portion of the virtual drive. */
    status = fbe_database_send_packet_to_object(FBE_LUN_CONTROL_CODE_SET_CONFIGURATION,
                                                config,
                                                sizeof(fbe_ext_pool_lun_configuration_t),
                                                object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if(status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s Failed to send config to pvd object 0x%x\n",
                   __FUNCTION__, object_id);       
    }
    return status;
}

/*!****************************************************************************
 * fbe_database_is_unmap_committed
 ******************************************************************************
 * @brief
 *  This function determines if unmap is committed based on the commit level
 *
 * @param None
 *
 * @return FBE_TRUE - If drive unmap is supported. FBE_FALSE - Otherwise
 *
 * @author 
 *   07/13/2015 - Created. Deanna Heng
 ******************************************************************************/
fbe_bool_t fbe_database_is_unmap_committed(void)
{
    fbe_u64_t        current_commit_level;

    fbe_spinlock_lock(&fbe_database_service.system_db_header_lock);
    current_commit_level = fbe_database_service.system_db_header.persisted_sep_version;
    fbe_spinlock_unlock(&fbe_database_service.system_db_header_lock);

    if (current_commit_level >= FLARE_COMMIT_LEVEL_ROCKIES_UNMAP)
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}


/*!****************************************************************************
 * fbe_database_get_pdo_drive_info()
 ******************************************************************************
 * @brief
 *  Get Drive Information from PDO
 *
 * @param pdo_object_id - object of the pdo to query
 * @param pdo_drive_info_p - pointer to the physical drive information
 *
 * @return status
 *
 * @author 
 *   8/15/2014 - Created. Wayne Garrett
 ******************************************************************************/
fbe_status_t fbe_database_get_pdo_drive_info(fbe_object_id_t pdo_object_id, 
                                             fbe_physical_drive_mgmt_get_drive_information_t *pdo_drive_info_p)
{
    fbe_status_t    status;

    status = fbe_database_send_packet_to_object(FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DRIVE_INFORMATION,
                                                pdo_drive_info_p,
                                                sizeof(*pdo_drive_info_p),
                                                pdo_object_id,
                                                NULL,  /* no sg list*/
                                                0,  /* no sg list*/
                                                FBE_PACKET_FLAG_EXTERNAL,
                                                FBE_PACKAGE_ID_PHYSICAL);
    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get drive tier information of PDO 0x%x.\n", 
                       __FUNCTION__, pdo_object_id);
        return status;
    }

    return status;   
}


/*!****************************************************************************
 * fbe_database_reactivate_pdos_of_drive_type()
 ******************************************************************************
 * @brief
 *  Reactivates PDOs by sending them back through their ACTIVATE state,  this
 *  will allow the database to re-process the PDOs for connection.
 *
 * @param pdo_object_id - object of the pdo to query
 * @param pdo_drive_info_p - pointer to the physical drive information
 *
 * @return status
 *
 * @author 
 *   8/15/2015 - Created. Wayne Garrett
 ******************************************************************************/
void fbe_database_reactivate_pdos_of_drive_type(fbe_database_additional_drive_types_supported_t drive_type)
{
    fbe_topology_control_get_physical_drive_objects_t *get_pdos_p;
    fbe_status_t status;
    fbe_u32_t i;
    fbe_physical_drive_mgmt_get_drive_information_t pdo_drive_info;
    fbe_bool_t do_reactivate;

    database_trace(FBE_TRACE_LEVEL_INFO,  FBE_TRACE_MESSAGE_ID_INFO,
           "%s reactivate PDOs of type 0x%x\n",
           __FUNCTION__, drive_type);

    get_pdos_p = (fbe_topology_control_get_physical_drive_objects_t*) fbe_memory_ex_allocate(sizeof(fbe_topology_control_get_physical_drive_objects_t));

    status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_GET_PHYSICAL_DRIVE_OBJECTS,
                                                 get_pdos_p,
                                                 sizeof(*get_pdos_p),
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 NULL,  
                                                 0,  
                                                 FBE_PACKET_FLAG_EXTERNAL,
                                                 FBE_PACKAGE_ID_PHYSICAL);
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,  FBE_TRACE_MESSAGE_ID_INFO,
               "%s failed to get PDOs. status:%d\n",
               __FUNCTION__, status);
        fbe_memory_ex_release(get_pdos_p);
        return;
    }

    for (i=0; i<get_pdos_p->number_of_objects; i++)
    {
        do_reactivate = FBE_FALSE;

        status = fbe_database_get_pdo_drive_info(get_pdos_p->pdo_list[i], &pdo_drive_info);
        if (status != FBE_STATUS_OK) 
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                            "%s Failed to get PDO drive info. PDO:0x%x, status:%d\n",
                            __FUNCTION__, get_pdos_p->pdo_list[i], status);                
            continue;
        }

        if ((drive_type == FBE_DATABASE_DRIVE_TYPE_SUPPORTED_LE)                      &&
            (pdo_drive_info.get_drive_info.drive_type == FBE_DRIVE_TYPE_SAS_FLASH_LE)    )
        {
            do_reactivate = FBE_TRUE;
        }
        else if ((drive_type == FBE_DATABASE_DRIVE_TYPE_SUPPORTED_RI)                      &&
                 (pdo_drive_info.get_drive_info.drive_type == FBE_DRIVE_TYPE_SAS_FLASH_RI)    )
        {
            do_reactivate = FBE_TRUE;
        }
        else if ((drive_type == FBE_DATABASE_DRIVE_TYPE_SUPPORTED_520_HDD)                 &&
                 !fbe_database_is_flash_drive(pdo_drive_info.get_drive_info.drive_type)    &&
                 pdo_drive_info.get_drive_info.block_size == FBE_BE_BYTES_PER_BLOCK)
        {
            do_reactivate = FBE_TRUE;
        }

        if (do_reactivate)
        {
            status  = fbe_database_send_packet_to_object(FBE_PHYSICAL_DRIVE_CONTROL_CODE_REACTIVATE,
                                                NULL, /* buff */
                                                0,    /* buff size */
                                                get_pdos_p->pdo_list[i],
                                                NULL, /* SGL*/
                                                0,    /* SGL count */
                                                FBE_PACKET_FLAG_EXTERNAL,
                                                FBE_PACKAGE_ID_PHYSICAL);            
            if (status != FBE_STATUS_OK) 
            {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s Failed to reactive PDO. PDO:0x%x, status:%d\n",
                            __FUNCTION__, get_pdos_p->pdo_list[i], status);              
                continue;
            }
        }

    }

    fbe_memory_ex_release(get_pdos_p);
}

/*!****************************************************************************
 * fbe_database_is_flash_drive()
 ******************************************************************************
 * @brief
 *  This function determines if drive is flash drive
 *
 * @param None
 *
 * @return FBE_TRUE - If drive is flash drive. FBE_FALSE - Otherwise
 *
 * @author 
 *   11/19/2015 - Created. Jibing Dong 
 ******************************************************************************/
fbe_bool_t fbe_database_is_flash_drive(fbe_drive_type_t drive_type)
{
    if ((drive_type == FBE_DRIVE_TYPE_SAS_FLASH_HE)  ||
        (drive_type == FBE_DRIVE_TYPE_SATA_FLASH_HE) ||
        (drive_type == FBE_DRIVE_TYPE_SAS_FLASH_ME)  ||
        (drive_type == FBE_DRIVE_TYPE_SAS_FLASH_LE)  ||
        (drive_type == FBE_DRIVE_TYPE_SAS_FLASH_RI))
    {
        return FBE_TRUE;
    }
    
    return FBE_FALSE;
}
    
/**************************************
 * end of module fbe_database_common.c
 **************************************/
