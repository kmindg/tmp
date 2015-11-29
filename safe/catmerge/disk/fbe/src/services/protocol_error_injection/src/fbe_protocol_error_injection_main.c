/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/
 
/*!*************************************************************************
 * @file fbe_protocol_error_injection_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the main entry point for the physical error injection 
 *  tool service.
 *
 * @version
 *   11/20/2009:  Created. guov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_protocol_error_injection_service.h"
#include "fbe_protocol_error_injection_private.h"
#include "fbe/fbe_transport.h"
#include "fbe_ssp_transport.h"
#include "fbe_stp_transport.h"
#include "swap_exports.h"



typedef struct fbe_protocol_error_injection_error_element_s {
    fbe_queue_element_t queue_element;
    fbe_protocol_error_injection_magic_number_t magic_number;
    fbe_protocol_error_injection_error_record_t protocol_error_injection_error_record;
}fbe_protocol_error_injection_error_element_t;

typedef struct fbe_protocol_error_injection_error_list_s{
//    void* prev;
    fbe_protocol_error_injection_error_record_t* protocol_error_injection_error_record;
    void* next;
}fbe_protocol_error_injection_error_list_t;

static fbe_protocol_error_injection_service_t   fbe_protocol_error_injection_service;

static fbe_bool_t protocol_error_injection_started = FBE_FALSE;

/* Declare our service methods */
fbe_status_t fbe_protocol_error_injection_service_control_entry(fbe_packet_t * packet); 
fbe_service_methods_t fbe_protocol_error_injection_service_methods = {FBE_SERVICE_ID_PROTOCOL_ERROR_INJECTION, fbe_protocol_error_injection_service_control_entry};


/*************************
 *   FUNCTION DEFINITIONS
 *************************/
static fbe_status_t fbe_protocol_error_injection_service_get_record (fbe_packet_t * packet_p);
static fbe_status_t fbe_protocol_error_injection_service_get_record_next (fbe_packet_t * packet_p);
static fbe_status_t fbe_protocol_error_injection_service_get_record_handle(fbe_packet_t * packet_p);

fbe_status_t protocol_error_injection_package_get_record(fbe_protocol_error_injection_record_handle_t protocol_error_injection_record_handle, fbe_protocol_error_injection_error_record_t * protocol_error_injection_error_record);
fbe_status_t protocol_error_injection_package_get_record_next(fbe_protocol_error_injection_record_handle_t * protocol_error_injection_record_handle, fbe_protocol_error_injection_error_record_t * protocol_error_injection_error_record);
fbe_status_t protocol_error_injection_package_get_record_handle(fbe_protocol_error_injection_error_record_t * protocol_error_injection_error_record, fbe_protocol_error_injection_record_handle_t * protocol_error_injection_record_handle);

static fbe_status_t fbe_protocol_error_injection_service_add_record (fbe_packet_t * packet);
static fbe_status_t fbe_protocol_error_injection_service_remove_record (fbe_packet_t * packet);
static fbe_status_t fbe_protocol_error_injection_service_remove_object (fbe_packet_t * packet);
static fbe_status_t fbe_protocol_error_injection_service_record_update_times_to_insert (fbe_packet_t * packet_p);

static fbe_status_t protocol_error_injection_add_record(fbe_protocol_error_injection_error_record_t * protocol_error_injection_error_record, fbe_protocol_error_injection_record_handle_t * protocol_error_injection_record_handle);
static fbe_status_t protocol_error_injection_remove_record(fbe_protocol_error_injection_record_handle_t protocol_error_injection_record_handle);
static fbe_status_t protocol_error_injection_remove_object(fbe_object_id_t object_id);
static fbe_status_t protocol_error_injection_package_record_update_times_to_insert(fbe_protocol_error_injection_record_handle_t * protocol_error_injection_record_handle, fbe_protocol_error_injection_error_record_t * protocol_error_injection_error_record);

static fbe_status_t protocol_error_injection_start(void);
static fbe_status_t protocol_error_injection_stop(void);

static fbe_status_t protocol_error_injection_edge_hook_function(fbe_packet_t * packet);
static fbe_status_t protocol_error_injection_handle_fis_error(fbe_payload_fis_operation_t * fis_operation, 
                                          fbe_protocol_error_injection_error_record_t * protocol_error_injection_error_record);
static fbe_status_t protocol_error_injection_handle_port_error(fbe_packet_t * packet, fbe_protocol_error_injection_error_record_t * error_record);
static fbe_status_t protocol_error_injection_insert_scsi_error(fbe_packet_t * packet_p, fbe_payload_ex_t* payload_p,fbe_payload_cdb_operation_t * cdb_operation, 
                                           fbe_protocol_error_injection_error_element_t * protocol_error_injection_error_element);
static fbe_status_t protocol_error_injection_check_error_record(fbe_payload_ex_t * payload_p,fbe_payload_cdb_operation_t * cdb_operation, 
                                            fbe_protocol_error_injection_error_element_t * protocol_error_injection_error_element);
static fbe_protocol_error_injection_error_element_t * protocol_error_injection_find_error_element_by_object_id(fbe_object_id_t object_id);
static fbe_status_t protocol_error_injection_notification_callback(fbe_object_id_t object_id, 
                                                    fbe_notification_info_t notification_info,
                                                    fbe_notification_context_t context);
fbe_bool_t protocol_error_injection_is_record_active(fbe_protocol_error_injection_error_record_t *error_record);
static fbe_status_t fbe_protocol_error_injection_service_remove_all_records(void);

/*!**************************************************************
 * fbe_protocol_error_injection_service_init()
 ****************************************************************
 * @brief
 *  Initialize the protocol_error_injection service.
 *
 * @param packet_p - The packet sent with the init request.
 *
 * @return fbe_status_t - Always FBE_STATUS_OK.
 *
 * @author
 *  11/19/2009 - Created. guov
 *
 ****************************************************************/
static fbe_status_t fbe_protocol_error_injection_service_init(fbe_packet_t * packet_p)
{
    fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s: entry\n", __FUNCTION__);

    fbe_base_service_set_service_id((fbe_base_service_t *) &fbe_protocol_error_injection_service, FBE_SERVICE_ID_PROTOCOL_ERROR_INJECTION);
    fbe_base_service_set_trace_level((fbe_base_service_t *) &fbe_protocol_error_injection_service, fbe_trace_get_default_trace_level());

    fbe_base_service_init((fbe_base_service_t *) &fbe_protocol_error_injection_service);

    fbe_queue_init(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue);
    fbe_spinlock_init(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);

    /*register with physical packge for notification. we are only interested in destroy, so that the records can be cleaned*/
//  status = protocol_error_injection_register_notification_element(FBE_PACKAGE_ID_PHYSICAL,
//                                                                  protocol_error_injection_notification_callback,
//                                                                  NULL);


    /* Always complete this request with good status.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_protocol_error_injection_service_init()
 ******************************************/

/*!**************************************************************
 * fbe_protocol_error_injection_service_destroy()
 ****************************************************************
 * @brief
 *  Destroy the protocol_error_injection service.
 *
 * @param packet_p - Packet for the destroy operation.          
 *
 * @return FBE_STATUS_OK - Destroy successful.
 *         FBE_STATUS_GENERIC_FAILURE - Destroy Failed.
 *
 * @author
 *  11/19/2009 - Created. guov
 *
 ****************************************************************/

static fbe_status_t fbe_protocol_error_injection_service_destroy(fbe_packet_t * packet_p)
{
    fbe_status_t status;

    fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                       "%s: entry\n", __FUNCTION__);

    protocol_error_injection_started = FBE_FALSE;

    fbe_protocol_error_injection_service_remove_all_records();

    fbe_queue_destroy(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue);
    fbe_spinlock_destroy(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);

    /*unregister with physical packge for notification*/
//  status = protocol_error_injection_unregister_notification_element(FBE_PACKAGE_ID_PHYSICAL,
//                                                                  protocol_error_injection_notification_callback,
//                                                                  NULL);

    status = fbe_base_service_destroy((fbe_base_service_t *) &fbe_protocol_error_injection_service);
    return status;
}
/******************************************
 * end fbe_protocol_error_injection_service_destroy()
 ******************************************/


/*!**************************************************************
 * fbe_protocol_error_injection_service_trace()
 ****************************************************************
 * @brief
 *  Trace function for use by the protocol_error_injection service.
 *  Takes into account the current global trace level and
 *  the locally set trace level.
 *
 * @param trace_level - trace level of this message.
 * @param message_id - generic identifier.
 * @param fmt... - variable argument list starting with format.
 *
 * @return None.  
 *
 * @author
 *  8/19/2009 - Created. guov
 *
 ****************************************************************/
void fbe_protocol_error_injection_service_trace(fbe_trace_level_t trace_level,
                         fbe_trace_message_id_t message_id,
                         const char * fmt, ...)
{
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    va_list args;

    /* Assume we are using the default trace level.
     */
    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();

    /* The global default trace level overrides the service trace level.
     */
    if (fbe_base_service_is_initialized(&fbe_protocol_error_injection_service.base_service))
    {
        service_trace_level = fbe_base_service_get_trace_level(&fbe_protocol_error_injection_service.base_service);
        if (default_trace_level > service_trace_level) 
        {
            /* Global trace level overrides the current service trace level.
             */
            service_trace_level = default_trace_level;
        }
    }

    /* If the service trace level passed in is greater than the
     * current chosen trace level then just return.
     */
    if (trace_level > service_trace_level) 
    {
        return;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
                     FBE_SERVICE_ID_PROTOCOL_ERROR_INJECTION,
                     trace_level,
                     message_id,
                     fmt, 
                     args);
    va_end(args);
    return;
}
/******************************************
 * end fbe_protocol_error_injection_service_trace()
 ******************************************/

/*!**************************************************************
 * fbe_protocol_error_injection_service_control_entry()
 ****************************************************************
 * @brief
 *  control entry for the protocol_error_injection service.
 *
 * @param packet - control packet to process.
 *
 * @return None.  
 *
 * @author
 *  11/19/2009 - Created. guov
 *
 ****************************************************************/
fbe_status_t fbe_protocol_error_injection_service_control_entry(fbe_packet_t * packet)
{    
    fbe_payload_control_operation_opcode_t control_code;
    fbe_status_t status;

    control_code = fbe_transport_get_control_code(packet);
    if (control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) {
        fbe_protocol_error_injection_service_init(packet);
        return FBE_STATUS_OK;
    }
    if (fbe_base_service_is_initialized((fbe_base_service_t*)&fbe_protocol_error_injection_service) == FALSE) {
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NOT_INITIALIZED;
    }
    switch(control_code) {
        case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
            status = fbe_protocol_error_injection_service_destroy(packet);
            break;
        case FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_ADD_RECORD:
            status = fbe_protocol_error_injection_service_add_record(packet);
            break;
        case FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_REMOVE_RECORD:
            status = fbe_protocol_error_injection_service_remove_record(packet);
            break;
        case FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_REMOVE_OBJECT:
            status = fbe_protocol_error_injection_service_remove_object(packet);
            break;
        case FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_START:
            status = protocol_error_injection_start();
            break;
        case FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_STOP:
            status = protocol_error_injection_stop();
            break;
        case FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_RECORD:
            status = fbe_protocol_error_injection_service_get_record(packet);
            break;
        case FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_RECORD_HANDLE:
            status = fbe_protocol_error_injection_service_get_record_handle(packet);
            break;
        case FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_GET_RECORD_NEXT:
            status = fbe_protocol_error_injection_service_get_record_next(packet);
            break;
        case FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_RECORD_UPDATE_TIMES_TO_INSERT:
            status = fbe_protocol_error_injection_service_record_update_times_to_insert(packet);
            break;
        case FBE_PROTOCOL_ERROR_INJECTION_SERVICE_CONTROL_CODE_REMOVE_ALL_RECORDS:
            status  = fbe_protocol_error_injection_service_remove_all_records();
            break;
        default:
            return fbe_base_service_control_entry((fbe_base_service_t *) &fbe_protocol_error_injection_service, packet);
            break;
    }
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}
/**************************************
 * end fbe_protocol_error_injection_service_control_entry()
 **************************************/
/*!**************************************************************
 * fbe_protocol_error_injection_service_add_record()
 ****************************************************************
 * @brief
 *  Take a new control packet and attempt to add a record.
 *
 * @param - packet_p - Packet.
 *
 * @return fbe_status_t status.
 *
 * @author
 *  11/19/2009 - Created. guov
 *
 ****************************************************************/
static fbe_status_t fbe_protocol_error_injection_service_add_record (fbe_packet_t * packet_p)
{
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_protocol_error_injection_control_add_record_t * add_record = NULL;
    fbe_u32_t len = 0;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &add_record);
    if (add_record == NULL)
    {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_protocol_error_injection_control_add_record_t))
    {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu ", __FUNCTION__, len, (unsigned long long)sizeof(fbe_protocol_error_injection_control_add_record_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = protocol_error_injection_add_record(&add_record->protocol_error_injection_error_record, &add_record->protocol_error_injection_record_handle);
    /* The caller is responsible for completing the packet. */
    return status;

}
/**************************************
 * end fbe_protocol_error_injection_service_add_record()
 **************************************/
static fbe_status_t 
protocol_error_injection_add_record(fbe_protocol_error_injection_error_record_t * protocol_error_injection_error_record, 
                fbe_protocol_error_injection_record_handle_t * protocol_error_injection_record_handle)
{
    fbe_status_t status;
    fbe_protocol_error_injection_error_element_t * protocol_error_injection_error_element = NULL;
    fbe_transport_control_set_edge_tap_hook_t hook_info;
    fbe_class_id_t class_id;

    *protocol_error_injection_record_handle = NULL;

    protocol_error_injection_error_element = fbe_allocate_contiguous_memory(sizeof(fbe_protocol_error_injection_error_element_t));
    if(protocol_error_injection_error_element == NULL){
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s Can't allocate memory\n", __FUNCTION__);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    protocol_error_injection_error_element->magic_number = FBE_PROTOCOL_ERROR_INJECTION_MAGIC_NUMBER;
  
    fbe_copy_memory(&protocol_error_injection_error_element->protocol_error_injection_error_record, protocol_error_injection_error_record, sizeof(fbe_protocol_error_injection_error_record_t));

    hook_info.edge_tap_hook = protocol_error_injection_edge_hook_function;

    /* Let's figure out if it is SAS or SATA drive or SAS enclosure */
    status = protocol_error_injection_get_object_class_id (protocol_error_injection_error_element->protocol_error_injection_error_record.object_id, &class_id, FBE_PACKAGE_ID_PHYSICAL);
    if((class_id > FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST) && (class_id < FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST)/* SAS drive */
       ||(class_id > FBE_CLASS_ID_SAS_ENCLOSURE_FIRST) && (class_id < FBE_CLASS_ID_SAS_ENCLOSURE_LAST)) /*SAS Enclosures */
    { 
        status = protocol_error_injection_set_ssp_edge_hook ( protocol_error_injection_error_element->protocol_error_injection_error_record.object_id, &hook_info);
    }else if((class_id > FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_FIRST) && (class_id < FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_LAST)){ /* SATA drive */
        status = protocol_error_injection_set_stp_edge_hook ( protocol_error_injection_error_element->protocol_error_injection_error_record.object_id, &hook_info);
    } else {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                   FBE_TRACE_MESSAGE_ID_INFO,
                                                   "%s Invalid class_id 0x%x object id: 0x%x\n", __FUNCTION__, class_id, 
                                                   protocol_error_injection_error_element->protocol_error_injection_error_record.object_id);
        fbe_release_contiguous_memory(protocol_error_injection_error_element);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);
    fbe_queue_push(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue, &protocol_error_injection_error_element->queue_element);
    fbe_spinlock_unlock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);
    *protocol_error_injection_record_handle = protocol_error_injection_error_element;
    return FBE_STATUS_OK;
}
/**************************************
 * end protocol_error_injection_add_record()
 **************************************/
/*!**************************************************************
 * fbe_protocol_error_injection_service_remove_record()
 ****************************************************************
 * @brief
 *  Take a new control packet and attempt to remove a record.
 *
 * @param - packet_p - Packet.
 *
 * @return fbe_status_t status.
 *
 * @author
 *  11/19/2009 - Created. guov
 *
 ****************************************************************/
fbe_status_t fbe_protocol_error_injection_service_remove_record (fbe_packet_t * packet_p)
{
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_protocol_error_injection_control_remove_record_t * remove_record = NULL;
    fbe_u32_t len = 0;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &remove_record);
    if (remove_record == NULL)
    {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_protocol_error_injection_control_remove_record_t))
    {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu ", __FUNCTION__, len, (unsigned long long)sizeof(fbe_protocol_error_injection_control_remove_record_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = protocol_error_injection_remove_record(remove_record->protocol_error_injection_record_handle);
    /* The caller is responsible for completing the packet. */
    return status;

}
/**************************************
 * end fbe_protocol_error_injection_service_remove_record()
 **************************************/
static fbe_status_t protocol_error_injection_remove_record(fbe_protocol_error_injection_record_handle_t protocol_error_injection_record_handle)
{
    fbe_status_t status = FBE_STATUS_CONTINUE;
    fbe_protocol_error_injection_error_element_t * protocol_error_injection_error_element = NULL;
    fbe_transport_control_set_edge_tap_hook_t hook_info;

//    PHYSICAL_ADDRESS  HighestAcceptableAddress;

//    HighestAcceptableAddress.QuadPart = 0xffffffffffffffff;

//    protocol_error_injection_error_element = fbe_allocate_contiguous_memory(sizeof(fbe_protocol_error_injection_error_element_t));

    protocol_error_injection_error_element = (fbe_protocol_error_injection_error_element_t *)protocol_error_injection_record_handle;

    if(protocol_error_injection_error_element->magic_number != FBE_PROTOCOL_ERROR_INJECTION_MAGIC_NUMBER) {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Invalid handle\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);
    fbe_queue_remove(&protocol_error_injection_error_element->queue_element);
    fbe_spinlock_unlock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);

    /* If we have not records for this object id we should disable the hook */
    if (NULL == protocol_error_injection_find_error_element_by_object_id(protocol_error_injection_error_element->protocol_error_injection_error_record.object_id))
    {
        hook_info.edge_tap_hook = NULL;

        /* Only remove the hook if the object id is valid */
        status = protocol_error_injection_set_ssp_edge_hook ( protocol_error_injection_error_element->protocol_error_injection_error_record.object_id, &hook_info);
    }

    fbe_release_contiguous_memory(protocol_error_injection_error_element);
    return FBE_STATUS_OK;

//    return status;
}
/**********************************************
 * end protocol_error_injection_remove_record()
 **********************************************/

/*!**************************************************************
 * fbe_protocol_error_injection_service_remove_object()
 ****************************************************************
 * @brief
 *  Take a new control packet and attempt to remove all records 
 *  of specified object.
 *
 * @param - packet_p - Packet.
 *
 * @return fbe_status_t status.
 *
 * @author
 *  11/19/2009 - Created. guov
 *
 ****************************************************************/
fbe_status_t fbe_protocol_error_injection_service_remove_object (fbe_packet_t * packet_p)
{
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_protocol_error_injection_control_remove_object_t * remove_object = NULL;
    fbe_u32_t len = 0;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &remove_object);
    if (remove_object == NULL)
    {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_protocol_error_injection_control_remove_object_t))
    {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu ", __FUNCTION__, len, (unsigned long long)sizeof(fbe_protocol_error_injection_control_remove_object_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = protocol_error_injection_remove_object(remove_object->object_id);
    /* The caller is responsible for completing the packet. */
    return status;

}
/**************************************
 * end fbe_protocol_error_injection_service_remove_object()
 **************************************/
/* Removes all records for particular object */
static fbe_status_t protocol_error_injection_remove_object(fbe_object_id_t object_id)
{
    fbe_protocol_error_injection_error_element_t *protocol_error_injection_error_element = NULL;
    fbe_protocol_error_injection_error_element_t *protocol_error_injection_error_element_next = NULL;
    fbe_transport_control_set_edge_tap_hook_t hook_info;
    fbe_status_t status;
    fbe_class_id_t class_id;

    fbe_spinlock_lock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);
    protocol_error_injection_error_element = (fbe_protocol_error_injection_error_element_t *)fbe_queue_front(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue);
    while(protocol_error_injection_error_element != NULL){
        protocol_error_injection_error_element_next = (fbe_protocol_error_injection_error_element_t *)fbe_queue_next(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue, (fbe_queue_element_t *)protocol_error_injection_error_element);
        if(protocol_error_injection_error_element->protocol_error_injection_error_record.object_id == object_id){ /* We found our record */
            fbe_queue_remove((fbe_queue_element_t *)protocol_error_injection_error_element);
            fbe_release_contiguous_memory(protocol_error_injection_error_element);
        }
        protocol_error_injection_error_element = protocol_error_injection_error_element_next;
    }/* while(protocol_error_injection_error_element != NULL) */
    fbe_spinlock_unlock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);

    hook_info.edge_tap_hook = NULL;
    /* Let's figure out if it is SAS or SATA drive */
    protocol_error_injection_get_object_class_id (object_id, &class_id, FBE_PACKAGE_ID_PHYSICAL); /*will this call fails when the object is in destroy state?*/
    if((class_id > FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST) && (class_id < FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST)/* SAS drive */
       ||(class_id > FBE_CLASS_ID_SAS_ENCLOSURE_FIRST) && (class_id < FBE_CLASS_ID_SAS_ENCLOSURE_LAST)) /*SAS Enclosures */
    { 
        status = protocol_error_injection_set_ssp_edge_hook ( object_id, &hook_info);
    }else if((class_id > FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_FIRST) && (class_id < FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_LAST)){ /* SATA drive */
        status = protocol_error_injection_set_stp_edge_hook ( object_id, &hook_info);
    }

    return FBE_STATUS_OK;
}
/**************************************
 * end protocol_error_injection_remove_object()
 **************************************/
/*!**************************************************************
 * fbe_protocol_error_injection_service_get_record()
 ****************************************************************
 * @brief
 *  Take a new control packet and attempt to get a record.
 *
 * @param - packet_p - Packet.
 *
 * @return fbe_status_t status.
 *
 * @author
 *  11/19/2009 - Created. guov
 *
 ****************************************************************/
static fbe_status_t fbe_protocol_error_injection_service_get_record (fbe_packet_t * packet_p)
{
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_protocol_error_injection_control_add_record_t * get_record = NULL;
    fbe_u32_t len = 0;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &get_record);
    if (get_record == NULL)
    {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_protocol_error_injection_control_add_record_t))
    {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu ", __FUNCTION__, len, (unsigned long long)sizeof(fbe_protocol_error_injection_control_add_record_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = protocol_error_injection_package_get_record(get_record->protocol_error_injection_record_handle, &get_record->protocol_error_injection_error_record);
    /* The caller is responsible for completing the packet. */
    return status;

}
/**************************************
 * end fbe_protocol_error_injection_service_get_record()
 **************************************/
/*!**************************************************************
 * protocol_error_injection_package_get_record()
 ****************************************************************
 * @brief
 *  This takes a handle as input and returns conrresponding record.
 *
 * @param - packet_p - Packet.
 *
 * @return fbe_status_t status.
 *
 * @author
 *  11/19/2009 - Created. Vinay Patki
 *
 ****************************************************************/
fbe_status_t 
protocol_error_injection_package_get_record(fbe_protocol_error_injection_record_handle_t protocol_error_injection_record_handle,
fbe_protocol_error_injection_error_record_t * protocol_error_injection_error_record)
{

    fbe_protocol_error_injection_error_element_t * protocol_error_injection_error_element;

    if(protocol_error_injection_record_handle == NULL){
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s Received Handle is NULL.\n", __FUNCTION__);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_spinlock_lock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);

    protocol_error_injection_error_element = (fbe_protocol_error_injection_error_element_t *)fbe_queue_front(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue);

    while(protocol_error_injection_error_element != NULL)
    {
        if(protocol_error_injection_error_element == protocol_error_injection_record_handle)
        {
                *protocol_error_injection_error_record = protocol_error_injection_error_element->protocol_error_injection_error_record;
                fbe_spinlock_unlock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);
                return FBE_STATUS_OK;
        }

        protocol_error_injection_error_element = (fbe_protocol_error_injection_error_element_t *)fbe_queue_next(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue, (fbe_queue_element_t *)protocol_error_injection_error_element);

    }

    fbe_spinlock_unlock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);

    return FBE_STATUS_CONTINUE;
}
/**************************************
 * end protocol_error_injection_package_get_record()
 **************************************/
/*!**************************************************************
 * fbe_protocol_error_injection_service_get_record_handle()
 ****************************************************************
 * @brief
 *  Take a new control packet and attempt to add a record.
 *
 * @param - packet_p - Packet.
 *
 * @return fbe_status_t status.
 *
 * @author
 *  07/21/2010 - Created. Vinay Patki
 *
 ****************************************************************/
static fbe_status_t fbe_protocol_error_injection_service_get_record_handle(fbe_packet_t * packet_p)
{
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_protocol_error_injection_control_add_record_t * get_record = NULL;
    fbe_u32_t len = 0;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &get_record);
    if (get_record == NULL)
    {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_protocol_error_injection_control_add_record_t))
    {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu ", __FUNCTION__, len, (unsigned long long)sizeof(fbe_protocol_error_injection_control_add_record_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = protocol_error_injection_package_get_record_handle(&get_record->protocol_error_injection_error_record, &get_record->protocol_error_injection_record_handle);
    /* The caller is responsible for completing the packet. */
    return status;

}
/**************************************
 * end fbe_protocol_error_injection_service_get_record_handle()
 **************************************/
/*!**************************************************************
 * protocol_error_injection_package_get_record_handle()
 ****************************************************************
 * @brief
 *  Take a new control packet and attempt to get a record.
 *
 * @param - packet_p - Packet.
 *
 * @return fbe_status_t status.
 *
 * @author
 *  11/19/2009 - Created. Vinay Patki
 *
 ****************************************************************/
fbe_status_t 
protocol_error_injection_package_get_record_handle(fbe_protocol_error_injection_error_record_t * protocol_error_injection_error_record, fbe_protocol_error_injection_record_handle_t *protocol_error_injection_record_handle)
{

    fbe_protocol_error_injection_error_element_t * protocol_error_injection_error_element;

    fbe_spinlock_lock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);

    protocol_error_injection_error_element = (fbe_protocol_error_injection_error_element_t *)fbe_queue_front(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue);

    while(protocol_error_injection_error_element != NULL)
    {
        if(protocol_error_injection_error_element->protocol_error_injection_error_record.object_id == protocol_error_injection_error_record->object_id)
        {
             //  Match other parameters of record found with the criteria.
            if(protocol_error_injection_error_element->protocol_error_injection_error_record.lba_start == protocol_error_injection_error_record->lba_start &&
                protocol_error_injection_error_element->protocol_error_injection_error_record.lba_end == protocol_error_injection_error_record->lba_end &&
            protocol_error_injection_error_element->protocol_error_injection_error_record.protocol_error_injection_error_type == protocol_error_injection_error_record->protocol_error_injection_error_type)
            {
                *protocol_error_injection_record_handle = (fbe_protocol_error_injection_record_handle_t *)protocol_error_injection_error_element;
                fbe_spinlock_unlock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);
                return FBE_STATUS_OK;
            }
        }

        protocol_error_injection_error_element = (fbe_protocol_error_injection_error_element_t *)fbe_queue_next(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue, (fbe_queue_element_t *)protocol_error_injection_error_element);

    }

    fbe_spinlock_unlock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);

    return FBE_STATUS_CONTINUE;
}
/**************************************
 * end protocol_error_injection_package_get_record_handle()
 **************************************/
/*!**************************************************************
 * fbe_protocol_error_injection_service_get_record_next()
 ****************************************************************
 * @brief
 *  Take a new control packet and attempt to add a record.
 *
 * @param - packet_p - Packet.
 *
 *
 *
 ****************************************************************/
static fbe_status_t fbe_protocol_error_injection_service_get_record_next (fbe_packet_t * packet_p)
{
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_protocol_error_injection_control_add_record_t * get_record = NULL;
    fbe_u32_t len = 0;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &get_record);
    if (get_record == NULL)
    {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_protocol_error_injection_control_add_record_t))
    {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu ", __FUNCTION__, len, (unsigned long long)sizeof(fbe_protocol_error_injection_control_add_record_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = protocol_error_injection_package_get_record_next(&get_record->protocol_error_injection_record_handle, &get_record->protocol_error_injection_error_record);
    /* The caller is responsible for completing the packet. */
    return status;

}
/**************************************
 * end fbe_protocol_error_injection_service_get_record_next()
 **************************************/
/*!**************************************************************
 * protocol_error_injection_package_get_record_next()
 ****************************************************************
 * @brief
 *  This takes a handle as input and returns conrresponding record.
 *
 * @param - packet_p - Packet.
 *
 * @return fbe_status_t status.
 *
 * @author
 *  11/19/2009 - Created. Vinay Patki
 *
 ****************************************************************/
fbe_status_t 
protocol_error_injection_package_get_record_next(fbe_protocol_error_injection_record_handle_t *protocol_error_injection_record_handle,
fbe_protocol_error_injection_error_record_t * protocol_error_injection_error_record)
{
    fbe_protocol_error_injection_error_element_t * protocol_error_injection_error_element = NULL;

    if(protocol_error_injection_error_record == NULL){
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s Received record is NULL.\n", __FUNCTION__);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_spinlock_lock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);

    if(*protocol_error_injection_record_handle == NULL)
    {
        protocol_error_injection_error_element = (fbe_protocol_error_injection_error_element_t *)fbe_queue_front(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue);
    }
    else
    {
        protocol_error_injection_error_element = (fbe_protocol_error_injection_error_element_t *)fbe_queue_next(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue, (fbe_queue_element_t *)*protocol_error_injection_record_handle);
    }

    if(protocol_error_injection_error_element != NULL)
    {
       *protocol_error_injection_error_record = protocol_error_injection_error_element->protocol_error_injection_error_record;
    }

   *protocol_error_injection_record_handle = (fbe_protocol_error_injection_record_handle_t *)protocol_error_injection_error_element;

    fbe_spinlock_unlock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);

    return FBE_STATUS_OK;
}
/**************************************
 * end protocol_error_injection_package_get_record_next()
 **************************************/
/*!**************************************************************
 * fbe_protocol_error_injection_service_record_update_time_to_insert()
 ****************************************************************
 * @brief
 *  Take a new control packet and attempt to update a record.
 *
 * @param - packet_p - Packet.
 *
 *
 *
 ****************************************************************/
static fbe_status_t fbe_protocol_error_injection_service_record_update_times_to_insert (fbe_packet_t * packet_p)
{
    fbe_payload_ex_t  *payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_protocol_error_injection_control_add_record_t * update_record = NULL;
    fbe_u32_t len = 0;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &update_record);
    if (update_record == NULL)
    {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s fbe_payload_control_get_buffer failed", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_protocol_error_injection_control_add_record_t))
    {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s Invalid len %d != %llu ", __FUNCTION__, len, (unsigned long long)sizeof(fbe_protocol_error_injection_control_add_record_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = protocol_error_injection_package_record_update_times_to_insert(&update_record->protocol_error_injection_record_handle, &update_record->protocol_error_injection_error_record);
    /* The caller is responsible for completing the packet. */
    return status;
}
/**************************************
 * end fbe_protocol_error_injection_service_record_update_time_to_insert()
 **************************************/
/*!**************************************************************
 * protocol_error_injection_package_record_update_time_to_insert()
 ****************************************************************
 * @brief
 *  This function updates a record.
 *
 * @param - packet_p - Packet.
 *
 *
 *
 ****************************************************************/
fbe_status_t 
protocol_error_injection_package_record_update_times_to_insert(fbe_protocol_error_injection_record_handle_t *protocol_error_injection_record_handle,
fbe_protocol_error_injection_error_record_t * protocol_error_injection_error_record)
{
    fbe_protocol_error_injection_error_element_t * protocol_error_injection_error_element = NULL;

    if(protocol_error_injection_record_handle == NULL){
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s Received Handle is NULL.\n", __FUNCTION__);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    fbe_spinlock_lock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);

    protocol_error_injection_error_element = (fbe_protocol_error_injection_error_element_t *)fbe_queue_next(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue, (fbe_queue_element_t *)*protocol_error_injection_record_handle);


    if(protocol_error_injection_error_element != NULL)
    {
       protocol_error_injection_error_element->protocol_error_injection_error_record.num_of_times_to_insert = protocol_error_injection_error_record->num_of_times_to_insert;
    }

    fbe_spinlock_unlock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);

    return FBE_STATUS_OK;
}
/**************************************
 * end protocol_error_injection_package_record_update_time_to_insert()
 **************************************/
static fbe_status_t 
protocol_error_injection_start(void)
{
    protocol_error_injection_started = FBE_TRUE;
    return FBE_STATUS_OK;
}
/**************************************
 * end protocol_error_injection_start()
 **************************************/
static fbe_status_t 
protocol_error_injection_stop(void)
{
    protocol_error_injection_started = FBE_FALSE;
    return FBE_STATUS_OK;
}
/**************************************
 * end protocol_error_injection_stop()
 **************************************/

/*!***************************************************************
 * protocol_error_injection_packet_completion()
 ****************************************************************
 * 
 * @brief   This is a test completion function for a
 *          packet we are injecting errors on.
 * 
 * @param packet_p - The packet being completed.
 * @param context - The fruts ptr.
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK.
 *
 * @author
 *  06/07/2012 - Created. Vamsi V
 *
 ****************************************************************/
fbe_status_t protocol_error_injection_packet_completion(fbe_packet_t * packet_p, 
                                                        fbe_packet_completion_context_t context)
{
    /* Find record by object_id */
    fbe_protocol_error_injection_error_element_t   *protocol_error_injection_error_element = NULL;
    fbe_protocol_error_injection_error_record_t    *record = NULL;
    fbe_base_edge_t * edge;
    fbe_object_id_t client_id;
    fbe_transport_id_t transport_id;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * cdb_operation = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_u8_t * sense_buffer = NULL;

    protocol_error_injection_error_element = (fbe_protocol_error_injection_error_element_t *)context;
    record = &protocol_error_injection_error_element->protocol_error_injection_error_record; 

    edge = fbe_transport_get_edge(packet_p);
    payload = fbe_transport_get_payload_ex(packet_p);
    fbe_base_transport_get_client_id(edge, &client_id);
    fbe_base_transport_get_transport_id(edge, &transport_id);

    if (transport_id == FBE_TRANSPORT_ID_SSP)
    {
        cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
        if (cdb_operation == NULL)
        {
            fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                       "%s: Get CDB Operation returned NULL\n", 
                                                       __FUNCTION__);
            return FBE_STATUS_OK;
        }
    }
    else if (transport_id == FBE_TRANSPORT_ID_STP)
    {
        fis_operation = fbe_payload_ex_get_fis_operation(payload);
        if (fis_operation == NULL)
        {
            fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                       "%s: Get FIS Operation returned NULL\n", 
                                                       __FUNCTION__);
            return FBE_STATUS_OK;
        }
    }
    else
    {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                   "%s Invalid transport_id %X\n", __FUNCTION__, transport_id);
        return FBE_STATUS_OK; /* We do not want to be too disruptive */
    }

    if (record->object_id != client_id)
    {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                   "%s Invalid client_id %X\n", __FUNCTION__, client_id);
        return FBE_STATUS_OK; /* We do not want to be too disruptive */
    }

    switch (record->protocol_error_injection_error_type)
    {
        case FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_PORT:  
            fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                       "%s Invalid client_id ASHOK %X\n", __FUNCTION__, client_id);    
            break;
    
        case FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_FIS:      
            break;
    
        case FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI:
            if (record->skip_blks != FBE_U32_MAX)
            {
                fbe_lba_t lba_start = 0;
                fbe_block_count_t blocks = 0;
                fbe_bool_t b_inserted = FBE_FALSE;

                if(cdb_operation != NULL)
                {
                    /* Get start lba_start and blocks in cdb payload. */
                    if (cdb_operation->cdb_length == 10)
                    {
                        lba_start = fbe_get_ten_byte_cdb_lba(&cdb_operation->cdb[0]);
                        blocks = fbe_get_ten_byte_cdb_blocks(&cdb_operation->cdb[0]);
        
                        blocks += record->skip_blks;
                        //sas_physical_drive_cdb_build_10_byte(cdb_operation, cdb_operation->timeout, lba_start, blocks);
                        fbe_set_ten_byte_cdb_blocks(&cdb_operation->cdb[0], blocks);
                        b_inserted = FBE_TRUE;
                    }
                    else if (cdb_operation->cdb_length == 16)
                    {
                        lba_start = fbe_get_sixteen_byte_cdb_lba(&cdb_operation->cdb[0]);
                        blocks = fbe_get_sixteen_byte_cdb_blocks(&cdb_operation->cdb[0]);
        
                        blocks += record->skip_blks;
                        //sas_physical_drive_cdb_build_16_byte(cdb_operation, cdb_operation->timeout, lba_start, blocks);
                        fbe_set_sixteen_byte_cdb_blocks(&cdb_operation->cdb[0], blocks); 
                        b_inserted = FBE_TRUE;
                    }
        
                    if (b_inserted)
                    {
        
                        record->times_inserted++;
        
                        if ( record->times_inserted >= record->num_of_times_to_insert )
                        {
                            /* Set the timestamp so that we know the exact time
                              * when we went over the threshold.
                              */
                            record->max_times_hit_timestamp = fbe_get_time();
                        }
        
                        fbe_payload_cdb_set_request_status(cdb_operation, record->protocol_error_injection_error.protocol_error_injection_scsi_error.port_status);
        
                        /* Insert scsi status and sense data. */
                        fbe_payload_cdb_set_scsi_status(cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
                        fbe_payload_cdb_operation_get_sense_buffer(cdb_operation, &sense_buffer);
        
        
                        sense_buffer[FBE_SCSI_SENSE_DATA_RESPONSE_CODE_OFFSET] = 0xf0;        
                        sense_buffer[FBE_SCSI_SENSE_DATA_SENSE_KEY_OFFSET] = record->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key;    
                        sense_buffer[FBE_SCSI_SENSE_DATA_ASC_OFFSET] = record->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code;    
                        sense_buffer[FBE_SCSI_SENSE_DATA_ASCQ_OFFSET] = record->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_additional_sense_code_qualifier;
                    }
                }
            }
            break;
    
        default: 
            fbe_spinlock_unlock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);
            fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                       "%s Invalid protocol_error_injection_error_type %X\n", __FUNCTION__, protocol_error_injection_error_element->protocol_error_injection_error_record.protocol_error_injection_error_type);
    }
    return FBE_STATUS_OK;
}
/***************************************************
 * end protocol_error_injection_packet_completion()
 ***************************************************/

static fbe_status_t 
protocol_error_injection_edge_hook_function(fbe_packet_t * packet)
{
    /* Find record by object_id */
    fbe_protocol_error_injection_error_element_t * protocol_error_injection_error_element = NULL;
    fbe_protocol_error_injection_error_element_t * main_protocol_error_injection_error_element = NULL;
    fbe_base_edge_t * edge;
    fbe_object_id_t client_id;
    fbe_transport_id_t transport_id;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * cdb_operation = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_status_t status;

    /* Check if this is the `remove hook' control code.
     */
	payload = fbe_transport_get_payload_ex(packet);
    if (((fbe_payload_operation_header_t *)payload->current_operation)->payload_opcode == FBE_PAYLOAD_OPCODE_CONTROL_OPERATION)
    {
        fbe_payload_control_operation_t                *control_operation = NULL; 
        fbe_payload_control_operation_opcode_t          opcode;
        fbe_transport_control_remove_edge_tap_hook_t   *remove_hook_p = NULL;

        control_operation = fbe_payload_ex_get_control_operation(payload);
        fbe_payload_control_get_opcode(control_operation, &opcode);
        if ((opcode == FBE_SSP_TRANSPORT_CONTROL_CODE_REMOVE_EDGE_TAP_HOOK) ||
            (opcode == FBE_STP_TRANSPORT_CONTROL_CODE_REMOVE_EDGE_TAP_HOOK)    )
        {
            fbe_payload_control_get_buffer(control_operation, &remove_hook_p);
            fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, 
                                                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                      "%s request to remove hook for obj: 0x%x\n", 
                                                      __FUNCTION__, remove_hook_p->object_id);

            /* Now remove all records associated with this object and then
             * remove the object.
             */
            status = protocol_error_injection_remove_object(remove_hook_p->object_id);
            return status;
        }
    }

    if (!protocol_error_injection_started)
    {
        return FBE_STATUS_CONTINUE;
    }

    edge = fbe_transport_get_edge(packet);
    fbe_base_transport_get_client_id(edge, &client_id);
    fbe_base_transport_get_transport_id(edge, &transport_id);

    /* Check the transport type */
    if(transport_id == FBE_TRANSPORT_ID_SSP){
        cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
        if(cdb_operation == NULL)
        {
            fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                       "%s: Get CDB Operation returned NULL\n", 
                                                       __FUNCTION__);
            return FBE_STATUS_CONTINUE;
        }
    }else if(transport_id == FBE_TRANSPORT_ID_STP){
        fis_operation = fbe_payload_ex_get_fis_operation(payload);
        if(fis_operation == NULL)
        {
            fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                       "%s: Get FIS Operation returned NULL\n", 
                                                       __FUNCTION__);
            return FBE_STATUS_CONTINUE;
        }
    } else {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Invalid transport_id %X\n", __FUNCTION__, transport_id);
        return FBE_STATUS_CONTINUE; /* We do not want to be too disruptive */
    }

    fbe_spinlock_lock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);
    protocol_error_injection_error_element = (fbe_protocol_error_injection_error_element_t *)fbe_queue_front(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue);
    main_protocol_error_injection_error_element = protocol_error_injection_error_element;
    while(protocol_error_injection_error_element != NULL){
        if(protocol_error_injection_error_element->protocol_error_injection_error_record.object_id == client_id){ /* We found our record */
                       
            switch(protocol_error_injection_error_element->protocol_error_injection_error_record.protocol_error_injection_error_type)
            {
                case FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_PORT:
                    status = protocol_error_injection_handle_port_error(packet, &protocol_error_injection_error_element->protocol_error_injection_error_record);
                    fbe_spinlock_unlock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);
                    if(status != FBE_STATUS_CONTINUE){
                        fbe_transport_set_status(packet, status, 0);
                        fbe_transport_complete_packet_async(packet);                        
                    }
                    return status; 
                    
                    break;
                case FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_FIS:
                    if (protocol_error_injection_error_element->protocol_error_injection_error_record.num_of_times_to_insert > protocol_error_injection_error_element->protocol_error_injection_error_record.times_inserted)
                    {
                        if(fis_operation != NULL)
                        {
                            status = protocol_error_injection_handle_fis_error(fis_operation, &protocol_error_injection_error_element->protocol_error_injection_error_record);
                            fbe_spinlock_unlock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);
                            if (status == FBE_STATUS_OK)
                            {
                                fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
                                fbe_transport_complete_packet_async(packet);
                            }
                            return status;
                        }
                    }
                    break;

                case FBE_PROTOCOL_ERROR_INJECTION_ERROR_TYPE_SCSI:
                    if (protocol_error_injection_error_element->protocol_error_injection_error_record.b_inc_error_on_all_pos)
                    {
                        fbe_protocol_error_injection_error_record_t *error_record_main = &(protocol_error_injection_error_element->protocol_error_injection_error_record);
                        if (protocol_error_injection_error_element->protocol_error_injection_error_record.num_of_times_to_insert > protocol_error_injection_error_element->protocol_error_injection_error_record.times_inserted)
                        {
                            fbe_protocol_error_injection_error_element_t *main_protocol_error_injection_error_element_temp = main_protocol_error_injection_error_element;
                            if(cdb_operation != NULL)
                            {
                                status = protocol_error_injection_insert_scsi_error(packet, payload, cdb_operation, protocol_error_injection_error_element);
                                if (status != FBE_STATUS_CONTINUE) {   
                                    while(main_protocol_error_injection_error_element_temp != NULL)
                                    {
                                        fbe_protocol_error_injection_error_record_t *error_record = &(main_protocol_error_injection_error_element_temp->protocol_error_injection_error_record);
                                        /*Increment the error counters of the record for which tag value is similar */
                                        if( (main_protocol_error_injection_error_element_temp != protocol_error_injection_error_element) && 
                                            (error_record->tag == error_record_main->tag))
                                        {
                                            error_record->times_inserted++;
                                            /*Split trace to two lines*/
                                            fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_INFO,
                                                                           FBE_TRACE_MESSAGE_ID_INFO,
                                                                           "%s Increment times insert for err record tmp pos 0x%x>\n", 
                                                                           __FUNCTION__, error_record->times_inserted);
                                            fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_INFO,
                                                                           FBE_TRACE_MESSAGE_ID_INFO,
                                                                           "%s err_rec_tmp->num_of_times_to_ins 0x%x obj_id:0x%x<\n", 
                                                                           __FUNCTION__, error_record->num_of_times_to_insert, 
                                                                           protocol_error_injection_error_element->protocol_error_injection_error_record.object_id);
                                        }
                                        main_protocol_error_injection_error_element_temp = (fbe_protocol_error_injection_error_element_t *)fbe_queue_next(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue, (fbe_queue_element_t *)main_protocol_error_injection_error_element_temp);
                                    }
                                    fbe_spinlock_unlock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);  
                                    fbe_transport_set_status(packet, status, 0);
                                    fbe_transport_complete_packet_async(packet);
                                    return status;
                                }
                            }
                        }
                        break;
                    }
                    if(cdb_operation != NULL)
                    {
                        status = protocol_error_injection_insert_scsi_error(packet, payload, cdb_operation, protocol_error_injection_error_element);
                        if (status == FBE_STATUS_NO_ACTION) {   
                            fbe_spinlock_unlock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock); 
                            return status;
                        }
                        if (status != FBE_STATUS_CONTINUE) {   
                            fbe_spinlock_unlock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);  
                            fbe_transport_set_status(packet, status, 0);
                            fbe_transport_complete_packet_async(packet);
                            return status;
                        }
                    }
                    /* Otherwise continue loop searching for an error to inject.
                     */
                    break;

                default: 
                    fbe_spinlock_unlock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);
                    fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                           "%s Invalid protocol_error_injection_error_type %X\n", __FUNCTION__, protocol_error_injection_error_element->protocol_error_injection_error_record.protocol_error_injection_error_type);
                    return FBE_STATUS_CONTINUE; /* We do not want to be too disruptive */
            }
        } /* It is not our record */

        protocol_error_injection_error_element = (fbe_protocol_error_injection_error_element_t *)fbe_queue_next(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue, (fbe_queue_element_t *)protocol_error_injection_error_element);
    }/* while(protocol_error_injection_error_element != NULL) */

    fbe_spinlock_unlock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);

    return FBE_STATUS_CONTINUE;
}
/**************************************
 * end protocol_error_injection_edge_hook_function()
 **************************************/
static fbe_protocol_error_injection_error_element_t * 
protocol_error_injection_find_error_element_by_object_id(fbe_object_id_t object_id)
{
    fbe_protocol_error_injection_error_element_t * protocol_error_injection_error_element = NULL;

    fbe_spinlock_lock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);
    protocol_error_injection_error_element = (fbe_protocol_error_injection_error_element_t *)fbe_queue_front(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue);
    while(protocol_error_injection_error_element != NULL){
        if(protocol_error_injection_error_element->protocol_error_injection_error_record.object_id == object_id){ /* We found our record */
            fbe_spinlock_unlock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);
            return protocol_error_injection_error_element;
        } else {
            protocol_error_injection_error_element = (fbe_protocol_error_injection_error_element_t *)fbe_queue_next(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue, (fbe_queue_element_t *)protocol_error_injection_error_element);
        }
    }/* while(protocol_error_injection_error_element != NULL) */

    fbe_spinlock_unlock(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue_lock);
    return NULL;
}
/**************************************
 * end protocol_error_injection_find_error_element_by_object_id()
 **************************************/
static fbe_status_t 
protocol_error_injection_handle_fis_error (fbe_payload_fis_operation_t * fis_operation, fbe_protocol_error_injection_error_record_t * protocol_error_injection_error_record)
{
    if ((protocol_error_injection_error_record->protocol_error_injection_error.protocol_error_injection_sata_error.sata_command == 0) || 
        (protocol_error_injection_error_record->protocol_error_injection_error.protocol_error_injection_sata_error.sata_command == fis_operation->fis[FBE_SATA_COMMAND_FIS_COMMAND_OFFSET]))
    {
        if(protocol_error_injection_error_record->protocol_error_injection_error.protocol_error_injection_sata_error.port_status == FBE_PORT_REQUEST_STATUS_SATA_NCQ_ERROR)
        {
            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_LOW_OFFSET] = fis_operation->fis[FBE_SATA_RESPONSE_FIS_LBA_LOW_OFFSET];
            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_MID_OFFSET] = fis_operation->fis[FBE_SATA_RESPONSE_FIS_LBA_MID_OFFSET];
            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_HIGH_OFFSET] = fis_operation->fis[FBE_SATA_RESPONSE_FIS_LBA_HIGH_OFFSET];
            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_LOW_EXT_OFFSET] = fis_operation->fis[FBE_SATA_RESPONSE_FIS_LBA_LOW_EXT_OFFSET];
            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_MID_EXT_OFFSET] = fis_operation->fis[FBE_SATA_RESPONSE_FIS_LBA_MID_EXT_OFFSET];
            fis_operation->response_buffer[FBE_SATA_RESPONSE_FIS_LBA_HIGH_EXT_OFFSET] = fis_operation->fis[FBE_SATA_RESPONSE_FIS_LBA_HIGH_EXT_OFFSET];
        }
        else
        {
        fbe_payload_fis_set_request_status(fis_operation, protocol_error_injection_error_record->protocol_error_injection_error.protocol_error_injection_sata_error.port_status);
        fbe_copy_memory(fis_operation->response_buffer, protocol_error_injection_error_record->protocol_error_injection_error.protocol_error_injection_sata_error.response_buffer, FBE_PAYLOAD_FIS_RESPONSE_BUFFER_SIZE);                       
        }
        protocol_error_injection_error_record->times_inserted++;
        return FBE_STATUS_OK;
    }
    return FBE_STATUS_CONTINUE;
}
/**************************************
 * end protocol_error_injection_handle_fis_error()
 **************************************/
static fbe_status_t 
protocol_error_injection_insert_scsi_error( fbe_packet_t * packet_p, fbe_payload_ex_t * payload_p,fbe_payload_cdb_operation_t * cdb_operation,
                       fbe_protocol_error_injection_error_element_t * protocol_error_injection_error_element)
{
    fbe_u8_t * sense_buffer = NULL;
    fbe_lba_t lba, bad_lba;
    fbe_u32_t record_count = 0;
    fbe_protocol_error_injection_error_record_t *error_record = &(protocol_error_injection_error_element->protocol_error_injection_error_record);
    fbe_protocol_error_injection_scsi_error_t *scsi_error_record = &(protocol_error_injection_error_element->protocol_error_injection_error_record.protocol_error_injection_error.protocol_error_injection_scsi_error);
    fbe_status_t status;
    fbe_bool_t is_active = FBE_TRUE;

    lba = error_record->lba_start;

    record_count = (fbe_u32_t)(error_record->lba_end - error_record->lba_start);

    /* Check error rules */
    status = protocol_error_injection_check_error_record(payload_p,
                                                         cdb_operation,
                                                         protocol_error_injection_error_element);

    if(status != FBE_STATUS_OK){
        return FBE_STATUS_CONTINUE;
    }

    if(error_record->frequency != 0)
    {
        if((error_record->times_inserted % error_record->frequency) != 0)
        {
            error_record->times_inserted++;
            return FBE_STATUS_CONTINUE;
        }
    }
    if (error_record->b_drop) {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, 
                                                   FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                   "PEI Drop err. obj_id: 0x%x \n", error_record->object_id);
        error_record->times_inserted++;
        return FBE_STATUS_NO_ACTION;
    }
    if (error_record->skip_blks != FBE_U32_MAX)
    {
        fbe_lba_t lba_start = 0;
        fbe_block_count_t blocks = 0;
        fbe_bool_t b_inserted = FBE_FALSE;

        /* Get start lba_start and blocks in cdb payload. */
        if (cdb_operation->cdb_length == 10)
        {
            lba_start = fbe_get_ten_byte_cdb_lba(&cdb_operation->cdb[0]);
            blocks = fbe_get_ten_byte_cdb_blocks(&cdb_operation->cdb[0]);

            if (blocks > error_record->skip_blks)
            {
                blocks -= error_record->skip_blks;
                //sas_physical_drive_cdb_build_10_byte(cdb_operation, cdb_operation->timeout, lba_start, blocks);
                fbe_set_ten_byte_cdb_blocks(&cdb_operation->cdb[0], blocks);
                b_inserted = FBE_TRUE;
            }
        }
        else if (cdb_operation->cdb_length == 16)
        {
            lba_start = fbe_get_sixteen_byte_cdb_lba(&cdb_operation->cdb[0]);
            blocks = fbe_get_sixteen_byte_cdb_blocks(&cdb_operation->cdb[0]);

            if (blocks > error_record->skip_blks)
            {
                blocks -= error_record->skip_blks;
                //sas_physical_drive_cdb_build_16_byte(cdb_operation, cdb_operation->timeout, lba_start, blocks);
                fbe_set_sixteen_byte_cdb_blocks(&cdb_operation->cdb[0], blocks); 
                b_inserted = FBE_TRUE;
            }
        }

        if (b_inserted)
        {
            fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_INFO, 
                                                       FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                       "PEI Incomplete write err. Skip_blocks: 0x%x obj_id: 0x%x times/lmit: %d/%d\n",
                                                       error_record->skip_blks, error_record->object_id, 
                                                       error_record->times_inserted, error_record->num_of_times_to_insert);

            /*Put ourselves on the completion stack.
             */
            status = fbe_transport_set_completion_function(packet_p,
                                                           protocol_error_injection_packet_completion,
                                                           protocol_error_injection_error_element);
            if (status != FBE_STATUS_OK)
            {
                fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR, 
                                                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                                          "%s unable to set completion %p\n", __FUNCTION__, packet_p);
                fbe_transport_set_status(packet_p, status, 0);
                fbe_transport_complete_packet_async(packet_p);

                return status;
            }
        }

        /* Let the operation continue downstream. If error was injected and completion function was set,
         * we will reset block count and set appropriate status when operation is complete. 
         */
        return FBE_STATUS_CONTINUE;
    }
    
    error_record->times_inserted++;

    if ( error_record->times_inserted >= error_record->num_of_times_to_insert )
    {
        /* Set the timestamp so that we know the exact time
          * when we went over the threshold.
          */
        error_record->max_times_hit_timestamp = fbe_get_time();
    }    

    if (error_record->b_test_for_media_error != FBE_FALSE)
    {
        /* We are injecting errorrs for this lba range.
         */
        switch (cdb_operation->cdb[0])
        {
            case FBE_SCSI_WRITE_6:
            case FBE_SCSI_WRITE_10:
            case FBE_SCSI_WRITE_16:
            case FBE_SCSI_READ_6:
            case FBE_SCSI_READ_10:
            case FBE_SCSI_READ_16:
            case FBE_SCSI_WRITE_VERIFY:    /* Ten Byte. */
            case FBE_SCSI_WRITE_VERIFY_16:
                /* error record will be active until it gets reassigned.
                 */
                error_record->b_active[error_record->reassign_count] = FBE_TRUE;
                break;
            case FBE_SCSI_REASSIGN_BLOCKS:    /* Ten Byte. */
                
                /* Simulate the fixing of the media error at the first lba of the
                 * reassign that has an error. 
                 * We simulate the fixing of the error by simply clearing the active 
                 * bit. 
                 * We do not support reassigning an entire range since the 
                 * PDO does not reassign more than one block at a time. 
                 * Increment the reassign count to keep track the number of
                 * blocks remapped.
                 */
                error_record->b_active[error_record->reassign_count] = FBE_FALSE;
                error_record->reassign_count++;
                break;
            default:
                break;
        }; /* end switch on opcode */

        is_active = protocol_error_injection_is_record_active(error_record);
        if ((is_active == FBE_TRUE)&&(cdb_operation->cdb[0] == FBE_SCSI_REASSIGN_BLOCKS) &&
            (record_count > PROTOCOL_ERROR_INJECTION_MAX_REASSIGN_BLOCKS))
        {
            return FBE_STATUS_CONTINUE;
        }
        else if (cdb_operation->cdb[0] == FBE_SCSI_REASSIGN_BLOCKS)
        {
            if (record_count == error_record->reassign_count)
            {
                error_record->reassign_count = 0;
                return FBE_STATUS_OK;
            }
            if (record_count != error_record->reassign_count)
            {
                return FBE_STATUS_CONTINUE;
            }
        }
    }

    /* Insert port/scsi status and sense data. */
    fbe_payload_cdb_set_request_status(cdb_operation, scsi_error_record->port_status);
    fbe_payload_cdb_set_scsi_status(cdb_operation, FBE_PAYLOAD_CDB_SCSI_STATUS_CHECK_CONDITION);
    fbe_payload_cdb_operation_get_sense_buffer(cdb_operation, &sense_buffer);

    sense_buffer[FBE_SCSI_SENSE_DATA_RESPONSE_CODE_OFFSET] = 0xf0;        
    sense_buffer[FBE_SCSI_SENSE_DATA_SENSE_KEY_OFFSET] = scsi_error_record->scsi_sense_key;    
    sense_buffer[FBE_SCSI_SENSE_DATA_ASC_OFFSET] = scsi_error_record->scsi_additional_sense_code;    
    sense_buffer[FBE_SCSI_SENSE_DATA_ASCQ_OFFSET] = scsi_error_record->scsi_additional_sense_code_qualifier;

   if(error_record->b_test_for_media_error != FBE_FALSE)
   {
        if (error_record->reassign_count == 0)
        {
            /* Get start lba in cdb payload. */
            if (cdb_operation->cdb_length == 6)
            {
                lba = fbe_get_six_byte_cdb_lba(&cdb_operation->cdb[0]);
            }
            else if (cdb_operation->cdb_length == 10)
            {
                lba = fbe_get_ten_byte_cdb_lba(&cdb_operation->cdb[0]);
            }
            else if (cdb_operation->cdb_length == 16)
            {
                lba = fbe_get_sixteen_byte_cdb_lba(&cdb_operation->cdb[0]);
            }
        }
        else
        {
            lba = error_record->lba_start +error_record->reassign_count;
        }
    }
    else
    {
        /* Get start lba in cdb payload. */
        if (cdb_operation->cdb_length == 6)
        {
            lba = fbe_get_six_byte_cdb_lba(&cdb_operation->cdb[0]);
        }
        else if (cdb_operation->cdb_length == 10)
        {
            lba = fbe_get_ten_byte_cdb_lba(&cdb_operation->cdb[0]);
        }
        else if (cdb_operation->cdb_length == 16)
        {
            lba = fbe_get_sixteen_byte_cdb_lba(&cdb_operation->cdb[0]);
        }
    }
    
    /* Get bad lba. */
    if ( lba <= error_record->lba_start )
    {
          bad_lba = error_record->lba_start;
    }
    else 
    {
          bad_lba = lba;
    }
 
    /* Fill up the LBA value. This will help to be the bad LBA for media errors. */
    sense_buffer[6] = (fbe_u8_t)(0xFF & bad_lba);    
    sense_buffer[5] = (fbe_u8_t)(0xFF & (bad_lba >> 8));    
    sense_buffer[4] = (fbe_u8_t)(0xFF & (bad_lba >> 16));    
    sense_buffer[3] = (fbe_u8_t)(0xFF & (bad_lba >> 24));
    
    return FBE_STATUS_OK;
}
/**************************************
 * end protocol_error_injection_insert_scsi_error()
 **************************************/

static fbe_bool_t 
protocol_error_injection_check_cmd_match(fbe_u8_t * cmd, fbe_payload_cdb_operation_t * cdb_operation)
{
	fbe_u32_t i = 0;

    if (cmd[0] == 0xFF) {
	    /* This is the case when any opcode is allowed. */
        return FBE_TRUE;
    }


	while ((i < MAX_CMDS) && (cmd[i]!= 0xFF)) {
		if (cmd[i] == cdb_operation->cdb[0]) {
			/* We got an exact match here. */
			return FBE_TRUE;
		}

		/* The following function translates to xx_16 cmd, but we need to support xx_10 as well */
		/* fbe_test_sep_util_get_scsi_command(fbe_payload_block_operation_opcode_t opcode) */

		switch (cdb_operation->cdb[0]) {
			case FBE_SCSI_READ_10:
			case FBE_SCSI_READ_16:
				if(cmd[i] == FBE_SCSI_READ_16){ return FBE_TRUE; }
				break;

			case FBE_SCSI_WRITE_10:
			case FBE_SCSI_WRITE_16:
				if(cmd[i] == FBE_SCSI_WRITE_16){ return FBE_TRUE; }
				break;
			
			case FBE_SCSI_WRITE_SAME_10:
			case FBE_SCSI_WRITE_SAME_16:
				if(cmd[i] == FBE_SCSI_WRITE_SAME_16){ return FBE_TRUE; }
				break;

			case FBE_SCSI_WRITE_VERIFY_10:
			case FBE_SCSI_WRITE_VERIFY_16:
				if(cmd[i] == FBE_SCSI_WRITE_VERIFY_16){ return FBE_TRUE; }
				break;

			case FBE_SCSI_VERIFY_10:
			case FBE_SCSI_VERIFY_16:
				if(cmd[i] == FBE_SCSI_VERIFY_16){ return FBE_TRUE; }
				break;

		};

		i++;
	}
	return FBE_FALSE;
}

static fbe_bool_t protocol_error_injection_is_media_operation(fbe_payload_cdb_operation_t * cdb_operation)
{
    fbe_bool_t  b_is_media = FBE_FALSE; /* By default it is not a media operation*/

    /* Switch on opcode.  Only supported media operations are media.
     */
     switch (cdb_operation->cdb[0]) {
         case FBE_SCSI_READ_6:
         case FBE_SCSI_READ_10:
         case FBE_SCSI_READ_12:
         case FBE_SCSI_READ_16:
         case FBE_SCSI_VERIFY_10:
         case FBE_SCSI_VERIFY_16:
         case FBE_SCSI_WRITE_6:
         case FBE_SCSI_WRITE_10:
         case FBE_SCSI_WRITE_12:
         case FBE_SCSI_WRITE_16:
         case FBE_SCSI_WRITE_SAME_10:
         case FBE_SCSI_WRITE_SAME_16:
         case FBE_SCSI_WRITE_VERIFY_10:
         case FBE_SCSI_WRITE_VERIFY_16:
             /* The above are media requests. */
             b_is_media = FBE_TRUE;
             break;
         default:
             /* Not media request*/
             b_is_media = FBE_FALSE;
             break;
     }
     return b_is_media;
}

static fbe_status_t 
protocol_error_injection_check_error_record(fbe_payload_ex_t * payload_p, fbe_payload_cdb_operation_t * cdb_operation,
                               fbe_protocol_error_injection_error_element_t * protocol_error_injection_error_element)
{
    fbe_lba_t lba_start = 0;
    fbe_block_count_t blocks = 0;
    fbe_bool_t b_is_media_operation = FBE_TRUE;
    fbe_protocol_error_injection_error_record_t *error_record = &(protocol_error_injection_error_element->protocol_error_injection_error_record);
    fbe_u32_t elapsed_seconds = 0;
    fbe_u8_t *cmd;
    fbe_u8_t cdb_opcode;

    /* Determine if it is a media request (i.e. is lba and blocks valid)*/
    b_is_media_operation = protocol_error_injection_is_media_operation(cdb_operation);

    /* If it is media get start lba_start and blocks in cdb payload. */
    if (b_is_media_operation) {
        if (cdb_operation->cdb_length == 6)
        {
            lba_start = fbe_get_six_byte_cdb_lba(&cdb_operation->cdb[0]);
            blocks = fbe_get_six_byte_cdb_blocks(&cdb_operation->cdb[0]);
        }
        else if (cdb_operation->cdb_length == 10)
        {
            lba_start = fbe_get_ten_byte_cdb_lba(&cdb_operation->cdb[0]);
            blocks = fbe_get_ten_byte_cdb_blocks(&cdb_operation->cdb[0]);
        }
        else if (cdb_operation->cdb_length == 16)
        {
            lba_start = fbe_get_sixteen_byte_cdb_lba(&cdb_operation->cdb[0]);
            blocks = fbe_get_sixteen_byte_cdb_blocks(&cdb_operation->cdb[0]);
        }
    } else {
        /* Else the lba and blocks are invalid */
        lba_start = FBE_LBA_INVALID;
        blocks = FBE_CDB_BLOCKS_INVALID;
    }
  
    /* This error record may no longer be active but
     * may be eligible to be reactivated.  
     */
    elapsed_seconds = fbe_get_elapsed_seconds(error_record->max_times_hit_timestamp);

    fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, error_record->object_id,
                                               "obj pl: times inserted: %d times to insert: %d elapsed secs: %d times reset: %d num to reactivate: %d\n",
                                               error_record->times_inserted, error_record->num_of_times_to_insert,
                                               elapsed_seconds, error_record->times_reset, error_record->num_of_times_to_reactivate);
    if ((error_record->times_inserted >= error_record->num_of_times_to_insert) &&
        (error_record->secs_to_reactivate != FBE_PROTOCOL_ERROR_INJECTION_INVALID) &&
        (elapsed_seconds >= error_record->secs_to_reactivate) &&
        (error_record->times_reset < error_record->num_of_times_to_reactivate))
    {
        /* This record should be reactivated now by resetting
         * times_inserted.
         */
        error_record->times_inserted = 0;
        /* Increment the number of times we have reset
         * in the times_reset field.
         */
        error_record->times_reset++;
    }

    if(error_record->b_test_for_media_error != FBE_FALSE)
    {
        /* Fetch the opcode from the cdb.  It is always the first entry.  
         */ 
        cdb_opcode = cdb_operation->cdb[0];

        switch (cdb_opcode)
        {
            case FBE_SCSI_WRITE_6:
            case FBE_SCSI_READ_6:
                error_record->lba_start = fbe_get_six_byte_cdb_lba(&cdb_operation->cdb[0]);
                blocks = fbe_get_six_byte_cdb_blocks(&cdb_operation->cdb[0]);
                break;
            case FBE_SCSI_REASSIGN_BLOCKS: /* Ten Byte. */
                blocks = 1;
                lba_start = error_record->lba_start; 
                break;
            case FBE_SCSI_READ_10:
            case FBE_SCSI_WRITE_10:
            case FBE_SCSI_WRITE_VERIFY:    /* Ten Byte. */
                /* Calculate the lba and blocks.
                 */
                blocks = fbe_get_ten_byte_cdb_blocks(&cdb_operation->cdb[0]);
                break;
            case FBE_SCSI_READ_16:
            case FBE_SCSI_WRITE_16:
            case FBE_SCSI_WRITE_VERIFY_16:
                /* Calculate the lba and blocks.
                 */
                blocks = fbe_get_sixteen_byte_cdb_blocks(&cdb_operation->cdb[0]);
                break;
            default:
                /* We do not inject errors for these opcodes.
                 * Return error so the caller knows to inject errors. 
                 */
                break;
        }

        /* There is a match if the records overlap in some way
         * AND the record is active
         */
        if ((lba_start <= error_record->lba_end) &&                        
            (error_record->lba_start <= (lba_start + blocks - 1)) &&
            (error_record->b_active[error_record->reassign_count] == FBE_TRUE))
        {
            /* There is a match of the specific LBA range. Now look for
             * specific opcode. If there is no match, check if there is
             * any thing with any range.  
             */
            cmd = error_record->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command;

			if(protocol_error_injection_check_cmd_match(cmd, cdb_operation)){
				return FBE_STATUS_OK;
			}

        }
    }
    else
    {
        /* There is a match if the records overlap in some way
          * AND the record is active
          */
        if ((!b_is_media_operation || 
            ((lba_start <= error_record->lba_end) && 
             (error_record->lba_start <= (lba_start + blocks - 1)))) &&
            (error_record->times_inserted < error_record->num_of_times_to_insert))
        {
            fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, error_record->object_id,
                                                       "obj pl: %p err match: 0x%llx/0x%llx rec: 0x%llx/0x%llx\n", 
                                                       payload_p,
						       (unsigned long long)lba_start,
						       (unsigned long long)(lba_start + blocks - 1),
						       (unsigned long long)error_record->lba_start,
						       (unsigned long long)error_record->lba_end);
            fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, error_record->object_id,
                                                       "obj pl: %p err match: insrted: 0x%x tmes to insrt: 0x%x sk:0x%x\n",
                                                       payload_p,
                                                       error_record->times_inserted, error_record->num_of_times_to_insert,
                                                       error_record->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key);
            /* There is a match of the specific LBA range. Now look for
             * specific opcode. If there is no match, check if there is
             * any thing with any range.  
             */
            cmd = error_record->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_command;

			if(protocol_error_injection_check_cmd_match(cmd, cdb_operation)){
				return FBE_STATUS_OK;
			}

        }
        else
        {
            fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, error_record->object_id,
                                                       "pl: %p no match: 0x%llx/0x%llx rec: 0x%llx/0x%llx\n", 
                                                       payload_p,
						       (unsigned long long)lba_start,
						       (unsigned long long)(lba_start + blocks - 1),
						       (unsigned long long)error_record->lba_start,
						       (unsigned long long)error_record->lba_end);
            fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, error_record->object_id,
                                                       "pl: %p no match: insrted: 0x%x tmes to insrt: 0x%x sk:0x%x\n",
                                                       payload_p,
                                                       error_record->times_inserted, error_record->num_of_times_to_insert,
                                                       error_record->protocol_error_injection_error.protocol_error_injection_scsi_error.scsi_sense_key);
        }
   }
    return FBE_STATUS_CONTINUE;
}
/**************************************
 * end protocol_error_injection_check_error_record()
 **************************************/
static fbe_status_t 
protocol_error_injection_handle_port_error(fbe_packet_t * packet, fbe_protocol_error_injection_error_record_t * error_record)
{
    fbe_base_edge_t * edge = NULL;
    fbe_transport_id_t transport_id;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_cdb_operation_t * cdb_operation = NULL;
    fbe_payload_fis_operation_t * fis_operation = NULL;
    fbe_lba_t lba_start = 0;
    fbe_block_count_t blocks = 0;
    fbe_u8_t * cmd;

    edge = fbe_transport_get_edge(packet);
    payload = fbe_transport_get_payload_ex(packet);
    fbe_base_transport_get_transport_id(edge, &transport_id);

    if(transport_id == FBE_TRANSPORT_ID_SSP){
        cdb_operation = fbe_payload_ex_get_cdb_operation(payload);
        if(cdb_operation == NULL)
        {
            fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                       "%s: Get CDB Operation returned NULL\n", 
                                                       __FUNCTION__);
            return FBE_STATUS_CONTINUE;
        }
    }else if(transport_id == FBE_TRANSPORT_ID_STP){
        fis_operation = fbe_payload_ex_get_fis_operation(payload);
        if(fis_operation == NULL)
        {
            fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                       "%s: Get FIS Operation returned NULL\n", 
                                                       __FUNCTION__);
            return FBE_STATUS_CONTINUE;
        }
    } else {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                   "%s Invalid transport_id %X\n", __FUNCTION__, transport_id);
        return FBE_STATUS_CONTINUE; /* We do not want to be too disruptive */
    }

    /* Check num_of_times_to_insert */
    if(error_record->num_of_times_to_insert == 0){
        return FBE_STATUS_CONTINUE;
    }

    error_record->times_inserted++;

    if( error_record->num_of_times_to_insert < error_record->times_inserted){
        return FBE_STATUS_CONTINUE;
    }

    if(error_record->frequency != 0){
        if((error_record->times_inserted % error_record->frequency) != 0){
            return FBE_STATUS_CONTINUE;
        }
    }

    
    /* Check if the LBA is in the range that we want */
    if(error_record->protocol_error_injection_error.protocol_error_injection_port_error.check_ranges){
        if (cdb_operation != NULL) {
            lba_start = fbe_get_cdb_lba(cdb_operation);
            blocks = fbe_get_cdb_blocks(cdb_operation);
        } else if (fis_operation != NULL) {
            fbe_payload_fis_get_lba(fis_operation, &lba_start);
            fbe_payload_fis_get_block_count(fis_operation, &blocks);
        } else {
            fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                       "%s: Both CDB/FIS Operations returned NULL\n",
                                                       __FUNCTION__);
            return FBE_STATUS_CONTINUE;
        }

        cmd = error_record->protocol_error_injection_error.protocol_error_injection_port_error.scsi_command;
        if ((lba_start <= error_record->lba_end) && 
            (error_record->lba_start <= (lba_start + blocks - 1)) &&
            protocol_error_injection_check_cmd_match(cmd, cdb_operation)) {

            /* we found something in the range we want. So go ahead and insert the error*/
            fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_INFO,
                                                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                       "%s Insert Port Err:%d,cmd:%d,lba:0x%llx\n", 
                                                       __FUNCTION__, 
                                                       error_record->protocol_error_injection_error.protocol_error_injection_port_error.port_status,
                                                       cmd[0],
                                                       (unsigned long long) lba_start);
        } else {
            return FBE_STATUS_CONTINUE;
        }
    }

    if(transport_id == FBE_TRANSPORT_ID_SSP){                   
        fbe_payload_cdb_set_request_status(cdb_operation, error_record->protocol_error_injection_error.protocol_error_injection_port_error.port_status);
        return FBE_STATUS_OK;
    }else if(transport_id == FBE_TRANSPORT_ID_STP){                 
        fbe_payload_fis_set_request_status(fis_operation, error_record->protocol_error_injection_error.protocol_error_injection_port_error.port_status);
        return FBE_STATUS_OK;
    } else {
        fbe_protocol_error_injection_service_trace(FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s Invalid transport_id %X\n", __FUNCTION__, transport_id);
        return FBE_STATUS_CONTINUE; /* We do not want to be too disruptive */
    }
}
/**************************************
 * end protocol_error_injection_handle_port_error()
 **************************************/
/*********************************************************************
 *            protocol_error_injection_notification_callback ()
 *********************************************************************
 *
 *  Description: callback function for the object destroy notification
 *
 *  Inputs: completing packet
            completion context with relevant completion information (sepahore to take down)
 *
 *  Return Value: success or failure
 *
 *  History:
 *    11/23/09: Created  guov
 *    
 *********************************************************************/
static fbe_status_t protocol_error_injection_notification_callback(fbe_object_id_t object_id, 
                                                    fbe_notification_info_t notification_info,
                                                    fbe_notification_context_t context)
{
    fbe_status_t status = FBE_STATUS_OK;
    if ((notification_info.source_package == FBE_PACKAGE_ID_PHYSICAL)&&
        (notification_info.notification_type & FBE_NOTIFICATION_TYPE_LIFECYCLE_ANY_STATE_CHANGE))
    {
        if ((notification_info.notification_type & FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_DESTROY)
            ||(notification_info.notification_type & FBE_NOTIFICATION_TYPE_LIFECYCLE_STATE_PENDING_DESTROY))
        {
            status = protocol_error_injection_remove_object(object_id);
        }
    }
    return status;
}
/**************************************
 * end protocol_error_injection_notification_callback()
 **************************************/

fbe_bool_t protocol_error_injection_is_record_active(fbe_protocol_error_injection_error_record_t *error_record)
{
    fbe_u8_t i;
    fbe_u32_t error_record_count;
    error_record_count = (fbe_u32_t )((error_record->lba_end - error_record->lba_start)-1 );
    for (i =0; i <= error_record_count; i++)
    {
        if (error_record->b_active[i] == FBE_TRUE)
        {
            return FBE_TRUE;
        }
    }
    return FBE_FALSE;
}


static fbe_status_t fbe_protocol_error_injection_service_remove_all_records(void)
{
    fbe_protocol_error_injection_error_element_t * protocol_error_injection_error_element = NULL;

    protocol_error_injection_error_element = (fbe_protocol_error_injection_error_element_t *)fbe_queue_front(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue);
    while(protocol_error_injection_error_element != NULL){
        protocol_error_injection_remove_record(protocol_error_injection_error_element);
        protocol_error_injection_error_element = (fbe_protocol_error_injection_error_element_t *)fbe_queue_front(&fbe_protocol_error_injection_service.protocol_error_injection_error_queue);
    }

    
    /* The caller is responsible for completing the packet. */
    return FBE_STATUS_OK;

}
/*************************
 * end file fbe_protocol_error_injection_main.c
 *************************/
