/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_lun_create.c
 ***************************************************************************
 *
 * @brief
 *  This file contains routines for lun create used by job service.
 * 
 * @ingroup job_lib_files
 * 
 * @version
 *   01/04/2010:  Created. guov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_job_service.h"
#include "fbe_job_service_operations.h"
#include "fbe/fbe_base_config.h"
#include "fbe_create_private.h"
#include "fbe/fbe_event_log_api.h"                  // for fbe_event_log_write
#include "fbe/fbe_event_log_utils.h"                // for message codes
#include "fbe/fbe_topology_interface.h"
#include "fbe_private_space_layout.h"
#include "fbe_cmi.h"

/*!**************************************************************
 * fbe_create_lib_send_control_packet()
 ****************************************************************
 * @brief
 * This function send a packet to other services within the logical
 * packet scope
 *
 * @param control_code - the code to send to other services
 * @param buffer - buffer containing data for service 
 * @param buffer_length - length of buffer
 * @params package_id - package type
 * @param service_id - service to send buffer
 * @param class_id - what class the service belongs to
 * @param object_id - id of the object service will work on
 * @param attr - attributes of packet 
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/19/2010 - Created. guov
 ****************************************************************/
fbe_status_t fbe_create_lib_send_control_packet (
        fbe_payload_control_operation_opcode_t control_code,
        fbe_payload_control_buffer_t buffer,
        fbe_payload_control_buffer_length_t buffer_length,
        fbe_package_id_t package_id,
        fbe_service_id_t service_id,
        fbe_class_id_t class_id,
        fbe_object_id_t object_id,
        fbe_packet_attr_t attr)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                  packet_p = NULL;
    fbe_payload_ex_t *             sep_payload = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_payload_control_status_t    payload_control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    packet_p = fbe_transport_allocate_packet();
    if (packet_p == NULL) {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "create_lib_send_ctrl_packet can't allocate packet\n");
        return FBE_STATUS_GENERIC_FAILURE;

    }

    fbe_transport_initialize_sep_packet(packet_p);
    sep_payload = fbe_transport_get_payload_ex (packet_p);
    control_operation = fbe_payload_ex_allocate_control_operation(sep_payload);

    fbe_payload_control_build_operation (control_operation,
                                         control_code,
                                         buffer,
                                         buffer_length);

    /* Set packet address.*/
    fbe_transport_set_address(packet_p,
                              package_id,
                              service_id,
                              class_id,
                              object_id); 

    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet_p, attr);

    fbe_transport_set_sync_completion_type(packet_p,
            FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* Control packets should be sent via service manager. */
    status = fbe_service_manager_send_control_packet(packet_p);
    if (status != FBE_STATUS_OK && status != FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        if (control_code != FBE_DATABASE_CONTROL_CODE_LOOKUP_RAID_BY_NUMBER)
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "create_lib_send_ctrl_packet send_ctrl_packet fail! Ctrl Code:0x%x, stat: 0x%x\n",
                    control_code,
                    status);
        }
        else
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, 
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "create_lib_send_ctrl_packet lookup rg id not found, may be a new request to create a RG\n");
        }
    }
    fbe_transport_wait_completion(packet_p);

    fbe_payload_control_get_status(control_operation, &payload_control_status);
    if(payload_control_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "create_lib_send_ctrl_packet payload ctrl operation fail! Control Code:0x%x\n",
                          control_code);
    }

    status  = fbe_transport_get_status_code(packet_p);
    if(status != FBE_STATUS_OK){
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "create_lib_send_ctrl_packet packet failed. 0x%x\n",
                status);
    }

    fbe_payload_ex_release_control_operation(sep_payload, control_operation);
    fbe_transport_release_packet(packet_p);
    return status;
}
/**********************************************************
 * end fbe_create_send_control_packet()
 **********************************************************/

/*!**************************************************************
 * fbe_create_lib_send_control_packet_with_sg()
 ****************************************************************
 * @brief
 * This function send a   
 *
 * @param packet_p - packet pointer.
 * @param control_code - control code.
 * @param buffer - buffer for the payload.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/19/2010 - Created. guov
 ****************************************************************/
fbe_status_t fbe_create_lib_send_control_packet_with_sg (
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
    if (packet_p == NULL) {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s can't allocate packet\n", __FUNCTION__);
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
                              FBE_PACKAGE_ID_SEP_0,
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
/**********************************************************
 * end fbe_create_lib_send_control_packet_with_sg()
 **********************************************************/

/*!**************************************************************
 * fbe_create_lib_send_control_packet_with_cpu_id()
 ****************************************************************
 * @brief
 * This function send a packet with cpu id to other services within the logical
 * packet scope. Metadata operations require a cpu id. For example, setting
 * the zero checkpoint requires a nonpaged write whith goes through the 
 * fbe_metadata_operation_entry function.
 *
 * @param control_code - the code to send to other services
 * @param buffer - buffer containing data for service 
 * @param buffer_length - length of buffer
 * @params package_id - package type
 * @param service_id - service to send buffer
 * @param class_id - what class the service belongs to
 * @param object_id - id of the object service will work on
 * @param attr - attributes of packet 
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/02/2010 - Created. Deanna Heng
 ****************************************************************/
fbe_status_t fbe_create_lib_send_control_packet_with_cpu_id (
        fbe_payload_control_operation_opcode_t control_code,
        fbe_payload_control_buffer_t buffer,
        fbe_payload_control_buffer_length_t buffer_length,
        fbe_package_id_t package_id,
        fbe_service_id_t service_id,
        fbe_class_id_t class_id,
        fbe_object_id_t object_id,
        fbe_packet_attr_t attr)
{
    fbe_status_t                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_packet_t *                  packet_p = NULL;
    fbe_payload_ex_t *             sep_payload = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_payload_control_status_t    payload_control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_cpu_id_t cpu_id;

    packet_p = fbe_transport_allocate_packet();
    if (packet_p == NULL) {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "create_lib_send_ctrl_packet can't allocate packet\n");
        return FBE_STATUS_GENERIC_FAILURE;

    }

    fbe_transport_initialize_sep_packet(packet_p);
    sep_payload = fbe_transport_get_payload_ex (packet_p);
    control_operation = fbe_payload_ex_allocate_control_operation(sep_payload);

    fbe_payload_control_build_operation (control_operation,
                                         control_code,
                                         buffer,
                                         buffer_length);

    /* Set packet address.*/
    fbe_transport_set_address(packet_p,
                              package_id,
                              service_id,
                              class_id,
                              object_id); 

    /* Mark packet with the attribute, either traverse or not */
    fbe_transport_set_packet_attr(packet_p, attr);

    fbe_transport_set_sync_completion_type(packet_p,
            FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    fbe_transport_get_cpu_id(packet_p, &cpu_id);
    if(cpu_id == FBE_CPU_ID_INVALID){
        cpu_id = fbe_get_cpu_id();
        fbe_transport_set_cpu_id(packet_p, cpu_id);
    }

    /* Control packets should be sent via service manager. */
    status = fbe_service_manager_send_control_packet(packet_p);
    if (status != FBE_STATUS_OK && status != FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        if (control_code != FBE_DATABASE_CONTROL_CODE_LOOKUP_RAID_BY_NUMBER)
        {
            job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "create_lib_send_ctrl_packet send_ctrl_packet fail! Ctrl Code:0x%x, stat: 0x%x\n",
                    control_code,
                    status);
        }
        else
        {
            job_service_trace(FBE_TRACE_LEVEL_INFO, 
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "create_lib_send_ctrl_packet lookup rg id not found, may be a new request to create a RG\n");
        }
    }
    fbe_transport_wait_completion(packet_p);

    fbe_payload_control_get_status(control_operation, &payload_control_status);
    if(payload_control_status != FBE_PAYLOAD_CONTROL_STATUS_OK){
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "create_lib_send_ctrl_packet payload ctrl operation fail! Control Code:0x%x\n",
                          control_code);
    }

    status  = fbe_transport_get_status_code(packet_p);
    if(status != FBE_STATUS_OK){
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "create_lib_send_ctrl_packet packet failed. 0x%x\n",
                status);
    }

    fbe_payload_ex_release_control_operation(sep_payload, control_operation);
    fbe_transport_release_packet(packet_p);
    return status;
}
/**********************************************************
 * end fbe_create_send_control_packet_with_cpu_id()
 **********************************************************/

/*!**************************************************************
 * fbe_create_lib_start_database_transaction()
 ****************************************************************
 * @brief
 * This function starts a transaction operation with the
 * configuration service
 *
 * @param transaction_id - transaction id for configuration service
 * @param job_number - the number of job that starts this transaction
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/19/2010 - Created. guov
 ****************************************************************/
fbe_status_t fbe_create_lib_start_database_transaction (
        fbe_database_transaction_id_t *transaction_id,
        fbe_u64_t   job_number)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_transaction_info_t transaction_info;
    transaction_info.transaction_id = FBE_DATABASE_TRANSACTION_ID_INVALID;
    transaction_info.transaction_type = FBE_DATABASE_TRANSACTION_CREATE;
    transaction_info.job_number = job_number;

    status = fbe_create_lib_send_control_packet (
                              FBE_DATABASE_CONTROL_CODE_TRANSACTION_START,
                              &transaction_info,
                              sizeof(transaction_info),
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_DATABASE,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID,
                              FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s, Transaction start failed\n", __FUNCTION__);
         return FBE_STATUS_GENERIC_FAILURE;
    }

    *transaction_id = transaction_info.transaction_id;
    return status;
}
/******************************************************
 * end fbe_create_lib_start_database_transaction()
 *****************************************************/

/*!**************************************************************
 * fbe_create_lib_commit_database_transaction()
 ****************************************************************
 * @brief
 * This function commits a transaction with the configuration
 * service
 *
 * @param transaction_id - transaction id for configuration service
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/19/2010 - Created. guov
 ****************************************************************/
fbe_status_t fbe_create_lib_commit_database_transaction (
        fbe_database_transaction_id_t transaction_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 

    status = fbe_create_lib_send_control_packet (
                                FBE_DATABASE_CONTROL_CODE_TRANSACTION_COMMIT,
                                &transaction_id,
                                sizeof(transaction_id),
                                FBE_PACKAGE_ID_SEP_0,
                                FBE_SERVICE_ID_DATABASE,
                                FBE_CLASS_ID_INVALID,
                                FBE_OBJECT_ID_INVALID,
                                FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s, Transaction commit failed\n", __FUNCTION__);
    }
    return status;
}
/*******************************************************
 * end fbe_create_lib_commit_database_transaction()
 ******************************************************/

/*!**************************************************************
 * fbe_create_lib_abort_database_transaction()
 ****************************************************************
 * @brief
 * This function aborts a transaction with the configuration
 * service
 *
 * @param transaction_id - transaction id for configuration service
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/19/2010 - Created. guov
 ****************************************************************/
fbe_status_t fbe_create_lib_abort_database_transaction (
             fbe_database_transaction_id_t transaction_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 

    status = fbe_create_lib_send_control_packet (
                                FBE_DATABASE_CONTROL_CODE_TRANSACTION_ABORT,
                                &transaction_id,
                                sizeof(transaction_id),
                                FBE_PACKAGE_ID_SEP_0,
                                FBE_SERVICE_ID_DATABASE,
                                FBE_CLASS_ID_INVALID,
                                FBE_OBJECT_ID_INVALID,
                                FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s, Transaction abort failed\n", __FUNCTION__);
    }
    return status;
}
/*******************************************************
 * end fbe_create_lib_abort_database_transaction()
 ******************************************************/


/*!**************************************************************
 * fbe_create_lib_database_service_create_edge()
 ****************************************************************
 * @brief
 * This function creates an edge
 *
 * @param transaction_id - transaction id for configuration service
 * @param server_object_id - edge's server id
 * @param client_object_id - edge's client id
 * @param client_index - edge's index
 * @param client_imported_capacity - edge's capacity
 * @param offset - edge's offset
 * @param serial_number - SN of the drive. Only used for connecting PVD and LDO. In other cases just assign it to NULL
 * @return status - The status of the operation.
 *
 * @author
 *  01/19/2010 - Created. guov
 ****************************************************************/
fbe_status_t fbe_create_lib_database_service_create_edge(
                                  fbe_database_transaction_id_t transaction_id, 
                                  fbe_object_id_t server_object_id,
                                  fbe_object_id_t client_object_id,
                                  fbe_u32_t client_index,
                                  fbe_lba_t client_imported_capacity,
                                  fbe_lba_t offset,
                                  fbe_u8_t*  serial_number,
                                  fbe_edge_flags_t    edge_flags)

{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_create_edge_t create_edge;

    create_edge.transaction_id = transaction_id;
    create_edge.object_id = client_object_id;
    create_edge.client_index = client_index;
    create_edge.capacity = client_imported_capacity;
    create_edge.offset = offset;
    create_edge.server_id = server_object_id;
    create_edge.ignore_offset = edge_flags;
    fbe_zero_memory(create_edge.serial_num, sizeof(create_edge.serial_num));
    if(NULL != serial_number)
    {
        fbe_copy_memory(create_edge.serial_num, serial_number, sizeof(create_edge.serial_num));
    }

    status = fbe_create_lib_send_control_packet (
                              FBE_DATABASE_CONTROL_CODE_CREATE_EDGE,
                              &create_edge,
                              sizeof(fbe_database_control_create_edge_t),
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_DATABASE,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID,
                              FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s FBE_DATABASE_CONTROL_CODE_CREATE_EDGE failed\n", __FUNCTION__);
    }
    return status;
}

/********************************************************
 * end fbe_create_lib_database_service_create_edge()
 *******************************************************/

/*!**************************************************************
 * fbe_create_lib_database_service_update_pvd_config_type()
 ****************************************************************
 * @brief
 * This function is used to update the config type of the pvd 
 * object.
 *
 * @param transaction_id - transaction id for configuration service
 * @param pvd_object_id  - pvd object id.
 * @param config_type    - config type of the pvd object.

 * @return status - The status of the operation.
 *
 * @author
 *  01/19/2010 - Created. guov
 ****************************************************************/
fbe_status_t fbe_create_lib_database_service_update_pvd_config_type(
                                  fbe_database_transaction_id_t transaction_id, 
                                  fbe_object_id_t pvd_object_id,
                                  fbe_provision_drive_config_type_t config_type)

{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_update_pvd_t           update_provision_drive;
    fbe_base_config_control_encryption_mode_t   encryption_mode_info;
	fbe_system_encryption_mode_t	            system_encryption_mode;
    fbe_u32_t                                   number_of_upstream_objects = 0;
    fbe_object_id_t                             upstream_object_list[FBE_MAX_UPSTREAM_OBJECTS];
    fbe_u32_t                                   i;
    fbe_base_config_encryption_mode_t           encryption_mode;

    update_provision_drive.transaction_id = transaction_id;
    update_provision_drive.object_id = pvd_object_id;
    update_provision_drive.update_type = FBE_UPDATE_PVD_TYPE;
    update_provision_drive.update_type_bitmask = 0;
    update_provision_drive.config_type = config_type;
    update_provision_drive.update_opaque_data = FBE_FALSE;
    fbe_zero_memory(update_provision_drive.opaque_data, sizeof(update_provision_drive.opaque_data));

    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "crtlib_dbsvc_upd_pvdcfg: pvd obj: 0x%x confg_type: %d trans_id: 0x%llx\n",
                           pvd_object_id, config_type, (unsigned long long)transaction_id);

    /* when pvd becomes unconsumed, we need to update its encryption mode */
    /* when pvd becomes raid, we need to update its encryption mode */
    if (config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED || config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID)
    {
        fbe_database_get_system_encryption_mode(&system_encryption_mode);
        if (system_encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED)
        {
            /* update the proision drive configuration. */
            status = fbe_create_lib_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_ENCRYPTION_MODE,
                                                        &encryption_mode_info,
                                                        sizeof(fbe_base_config_control_encryption_mode_t),
                                                        FBE_PACKAGE_ID_SEP_0,
                                                        FBE_SERVICE_ID_TOPOLOGY,
                                                        FBE_CLASS_ID_INVALID,
                                                        pvd_object_id,
                                                        FBE_PACKET_FLAG_NO_ATTRIB);

            if (status != FBE_STATUS_OK) {
                fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                       FBE_TRACE_LEVEL_ERROR, 
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       "GET_ENCRYPTION_MODE failed for obj 0x%x\n", 
                                       pvd_object_id);
                return status;
            }

            /* if ((encryption_mode_info.encryption_mode != FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED) &&
                (encryption_mode_info.encryption_mode != FBE_BASE_CONFIG_ENCRYPTION_MODE_INVALID)) */
            {
                /* By default (non-system drives) we set the update_type as: */
                if (config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED) {
                    update_provision_drive.update_type = FBE_UPDATE_PVD_UNENCRYPTED;
                } else {
                    update_provision_drive.update_type = FBE_UPDATE_PVD_TYPE; /* This will set enc mode to ENCRYPTED */
                }

                if (fbe_database_is_object_system_pvd(pvd_object_id))
                {
                    status = fbe_create_lib_get_upstream_edge_list(pvd_object_id, 
                                                                   &number_of_upstream_objects,
                                                                   upstream_object_list);
                    if (status != FBE_STATUS_OK) 
                    {
                        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                                FBE_TRACE_LEVEL_ERROR, 
                                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                                "fbe_create_lib_get_upstream_edge_list call failed, status %d\n", status);
                        return status;
                    }

                    encryption_mode = FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED;
                    for (i = 0; i < number_of_upstream_objects; i++)
                    {
                        if (fbe_database_is_object_system_rg(upstream_object_list[i]))
                        {
                            /* update the proision drive configuration. */
                            status = fbe_create_lib_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_ENCRYPTION_MODE,
                                                                        &encryption_mode_info,
                                                                        sizeof(fbe_base_config_control_encryption_mode_t),
                                                                        FBE_PACKAGE_ID_SEP_0,
                                                                        FBE_SERVICE_ID_TOPOLOGY,
                                                                        FBE_CLASS_ID_INVALID,
                                                                        upstream_object_list[i],
                                                                        FBE_PACKET_FLAG_NO_ATTRIB);

                            if (status != FBE_STATUS_OK) {
                                fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                                                       FBE_TRACE_LEVEL_ERROR, 
                                                       FBE_TRACE_MESSAGE_ID_INFO,
                                                       "GET_ENCRYPTION_MODE failed for obj 0x%x\n", 
                                                       upstream_object_list[i]);
                                return status;
                            }  
                            if (encryption_mode_info.encryption_mode > encryption_mode) {
                                encryption_mode = encryption_mode_info.encryption_mode;
                            }
                        }
                    }

                    if (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) {
                        update_provision_drive.update_type = FBE_UPDATE_PVD_ENCRYPTION_IN_PROGRESS;
                    } else if (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS) {
                        update_provision_drive.update_type = FBE_UPDATE_PVD_REKEY_IN_PROGRESS;
                    } else if (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED) {
                        update_provision_drive.update_type = FBE_UPDATE_PVD_TYPE;
                    } 
                    /* if vault is unencrypted, we use the default value */ 
                } /* if (fbe_database_is_object_system_pvd(pvd_object_id)) */
            }
        } /* if (system_encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED) */
    }

    /* update the proision drive configuration. */
    status = fbe_create_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_UPDATE_PVD,
                                                &update_provision_drive,
                                                sizeof(fbe_database_control_update_pvd_t),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_DATABASE,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "crtlib_dbsvc_upd_pvdcfg FBE_DATABASE_CONTROL_CODE_UPDATE_PROVISION_DRIVE failed\n");
    }
    return status;
}

/*******************************************************************
 * end fbe_cretae_lib_database_service_update_pvd_config_type()
 *******************************************************************/

/*!**************************************************************
 * fbe_create_lib_database_service_update_pvd_pool_id()
 ****************************************************************
 * @brief
 * This function is used to update the pool id of the pvd 
 * object.
 *
 * @param transaction_id - transaction id for configuration service
 * @param pvd_object_id  - pvd object id.
 * @param pool_id        - pool id of the pvd object.

 * @return status - The status of the operation.
 *
 * @author
 *  06/06/2014 - Created. Lili Chen
 ****************************************************************/
fbe_status_t fbe_create_lib_database_service_update_pvd_pool_id(fbe_database_transaction_id_t transaction_id, 
                                                                fbe_object_id_t pvd_object_id, 
                                                                fbe_u32_t pool_id)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_update_pvd_t           update_provision_drive;

    fbe_zero_memory(&update_provision_drive, sizeof(fbe_database_control_update_pvd_t));
    update_provision_drive.transaction_id = transaction_id;
    update_provision_drive.object_id = pvd_object_id;
    update_provision_drive.update_type = FBE_UPDATE_PVD_POOL_ID;;
    update_provision_drive.update_type_bitmask = 0;
    update_provision_drive.pool_id = pool_id;
    update_provision_drive.update_opaque_data = FBE_FALSE;

    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "crtlib_dbsvc_upd_pvd_pool_id: pvd obj: 0x%x pool_id: %d trans_id: 0x%llx\n",
                           pvd_object_id, pool_id, (unsigned long long)transaction_id);


    /* update the proision drive configuration. */
    status = fbe_create_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_UPDATE_PVD,
                                                &update_provision_drive,
                                                sizeof(fbe_database_control_update_pvd_t),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_DATABASE,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "crtlib_dbsvc_upd_pvd_pool_id failed\n");
    }
    return status;
}
/*******************************************************************
 * end fbe_create_lib_database_service_update_pvd_pool_id()
 *******************************************************************

/*!**************************************************************
 * fbe_create_lib_database_service_update_encryption_mode()
 ****************************************************************
 * @brief
 * This function is used to update the config type of the pvd 
 * object.
 *
 * @param transaction_id - transaction id for configuration service
 * @param object_id  - object id.
 * @param config_type    - config type of the pvd object.

 * @return status - The status of the operation.
 *
 * @author
 *  01/19/2010 - Created. guov
 ****************************************************************/
fbe_status_t fbe_create_lib_database_service_update_encryption_mode(
                                  fbe_database_transaction_id_t transaction_id, 
                                  fbe_object_id_t object_id,
                                  fbe_base_config_encryption_mode_t mode)

{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_update_encryption_mode_t  encryption_mode;

    encryption_mode.transaction_id = transaction_id;
    encryption_mode.object_id = object_id;
    encryption_mode.base_config_encryption_mode = mode;
    
    fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "crtlib_dbsvc_upd_cfg: obj: 0x%x encryption mode: %d trans_id: 0x%llx\n",
                           object_id, mode, (unsigned long long)transaction_id);

    /* update the proision drive ocnfiguration. */
    status = fbe_create_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_UPDATE_ENCRYPTION_MODE,
                                                &encryption_mode,
                                                sizeof(fbe_database_control_update_encryption_mode_t),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_DATABASE,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "crtlib_dbsvc_upd_pvdcfg FBE_DATABASE_CONTROL_CODE_UPDATE_PROVISION_DRIVE failed\n");
    }
    return status;
}

/*******************************************************************
 * end fbe_cretae_lib_database_service_update_pvd_config_type()
 *******************************************************************

/*!**************************************************************
 * fbe_create_lib_database_service_scrub_old_user_data()
 ****************************************************************
 * @brief
 * This function is used to drive database to scrub all user data
 *

 * @return status - The status of the operation.
 *
 * @author
 ****************************************************************/
fbe_status_t fbe_create_lib_database_service_scrub_old_user_data(void)

{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    
    /* update the proision drive ocnfiguration. */
    status = fbe_create_lib_send_control_packet(FBE_DATABASE_CONTROL_CODE_SCRUB_OLD_USER_DATA,
                                                NULL,
                                                0,
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_DATABASE,
                                                FBE_CLASS_ID_INVALID,
                                                FBE_OBJECT_ID_INVALID,
                                                FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK) {
        fbe_base_library_trace(FBE_LIBRARY_ID_DRIVE_SPARING,
                               FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "FBE_DATABASE_CONTROL_CODE_SCRUB_OLD_USER_DATA failed\n");
    }
    return status;
}

/*******************************************************************
 * end fbe_create_lib_database_service_scrub_old_user_data()
 *******************************************************************
  
 /*!****************************************************************
 * fbe_cretae_lib_get_provision_drive_config_type()
 *******************************************************************
 * @brief
 * This function is used to get the provision drive config type.
 *
 * @param pvd_object_id  - pvd object id.
 * @param config_type    - pointer to config type of the pvd object.

 * @return status - The status of the operation.
 *
 * @author
 *  08/27/2010 - Created. Dhaval Patel
 *******************************************************************/
fbe_status_t fbe_cretae_lib_get_provision_drive_config_type(fbe_object_id_t pvd_object_id,
                                                            fbe_provision_drive_config_type_t * provision_drive_config_type_p,
                                                            fbe_bool_t *  end_of_life_state_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_provision_drive_info_t              provision_drive_info;

    /* initialize the config type as invalid.*/
    *provision_drive_config_type_p = FBE_PROVISION_DRIVE_CONFIG_TYPE_INVALID;

    /* issue the control packet to pvd object to get its drive information. */
    status = fbe_create_lib_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PROVISION_DRIVE_INFO,
                                                &provision_drive_info,
                                                sizeof(fbe_provision_drive_info_t),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_TOPOLOGY,
                                                FBE_CLASS_ID_INVALID,
                                                pvd_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_INFO, "%s get config type failed, status:0x%x\n",
                          __FUNCTION__, status);
    }
    else
    {    
        *provision_drive_config_type_p = provision_drive_info.config_type;
        *end_of_life_state_p = provision_drive_info.end_of_life_state;
    }
    return status;
}
/*******************************************************************
 * end fbe_cretae_lib_get_provision_drive_config_type()
 *******************************************************************

/*!**************************************************************
 * fbe_create_lib_database_service_lookup_raid_object_id()
 ****************************************************************
 * @brief
 * This function returns the logical package id of a raid group
 * user id
 *
 * @param raid_group_id - user raid group id
 * @param rg_object_id_p - logical package id
 *
 * @return status
 *
 * @author
 *  02/12/2010 - Created. Jesus Medina
 ****************************************************************/
fbe_status_t fbe_create_lib_database_service_lookup_raid_object_id(
        fbe_raid_group_number_t raid_group_id, fbe_object_id_t *rg_object_id_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_lookup_raid_t lookup_raid;

    lookup_raid.raid_id = raid_group_id;

    status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_LOOKUP_RAID_BY_NUMBER,
                                                 &lookup_raid,
                                                 sizeof(fbe_database_control_lookup_raid_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_DATABASE,
                                                 FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s, raid object id could not be located \n", 
                          __FUNCTION__);
        return status;
    }

    *rg_object_id_p = lookup_raid.object_id;

    return status;
}
/******************************************************************
 * end fbe_create_lib_database_service_lookup_raid_object_id()
 *****************************************************************/

/*!**************************************************************
 * fbe_create_lib_base_config_validate_capacity()
 ****************************************************************
 * @brief
 * This function finds out if the server object has free space for 
 * the capacity given.  If so, it returns success and valid offset
 * and client_index.
 *
 * @param server_object_id - server to check with (IN)
 * @param client_imported_capacity - capacity to check (IN)
 * @param placement - placement option. e.g. first fit or best fit. (IN)
 * @param b_ignore_offset - Offset will not be used/added for this object (IN)
 * @param offset - the offset the give capacity can start (OUT) 
 * @param client_index - the next available client index (OUT)
 * @param available_capacity - capacity available for 
 *                             SPECIFIC_LOCATION placement(OUT)
 *
 * @return status
 *
 * @author
 *  02/12/2010 - Created. guov
 ****************************************************************/
fbe_status_t fbe_create_lib_base_config_validate_capacity (fbe_object_id_t server_object_id,
        fbe_lba_t client_imported_capacity,
        fbe_block_transport_placement_t placement,
        fbe_bool_t b_ignore_offset,
        fbe_lba_t *offset,
        fbe_u32_t *client_index,
        fbe_lba_t *available_capacity)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_block_transport_control_validate_capacity_t validate_capacity;

    validate_capacity.capacity      = client_imported_capacity;
    validate_capacity.placement     = placement;
    validate_capacity.b_ignore_offset = b_ignore_offset;
    validate_capacity.block_offset  = *offset;

    status = fbe_create_lib_send_control_packet (
                                   FBE_BLOCK_TRANSPORT_CONTROL_CODE_VALIDATE_CAPACITY,
                                   &validate_capacity,
                                   sizeof(fbe_block_transport_control_validate_capacity_t),
                                   FBE_PACKAGE_ID_SEP_0,
                                   FBE_SERVICE_ID_TOPOLOGY,
                                   FBE_CLASS_ID_INVALID,
                                   server_object_id,
                                   FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s, validate capacity call failed\n", __FUNCTION__);
    }
    *offset = validate_capacity.block_offset;
    *client_index = validate_capacity.client_index;
    *available_capacity = validate_capacity.capacity;
    return status;
}
/******************************************************************
 * end fbe_create_lib_base_config_validate_capacity()
 *****************************************************************/

/*!**************************************************************
 * fbe_create_lib_get_upstream_edge_count()
 ****************************************************************
 * @brief
 * This function sends a packet to the base config to determine
 * of the raid group object has upstream objects connected to it,
 * if there are objects connected to it, it returns the number of
 * objects connected
 *
 * @param object_id -  id of raid group
 * @param number_of_upstream_objects - count of upstream objects for the
 * given raid group
 *
 * @return status - The status of the operation.
 *
 * @author
 *  03/16/2010 - Created. Jesus Medina
 ****************************************************************/

fbe_status_t fbe_create_lib_get_upstream_edge_count(
        fbe_object_id_t object_id, fbe_u32_t *number_of_upstream_edges)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_base_config_upstream_edge_count_t   upstream_object;

    upstream_object.number_of_upstream_edges = 0;
    status = fbe_create_lib_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_EDGE_COUNT,
                                                 &upstream_object,
                                                 sizeof(fbe_base_config_upstream_edge_count_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_INVALID,
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: failed.  Error: 0x%x\n", 
                          __FUNCTION__, status);
    }
    else
    {    
        *number_of_upstream_edges = upstream_object.number_of_upstream_edges;
    }
    return status;
}
/**********************************************
 * end fbe_create_lib_get_upstream_edge_count()
 **********************************************/

/*!**************************************************************
 * fbe_create_lib_get_upstream_edge_list()
 ****************************************************************
 * @brief
 * This function sends a packet to the base config to determine
 * of the raid group object has upstream objects connected to it,
 * if there are objects connected to it, it returns the number of
 * objects connected, as well as a list of objects
 *
 * @param object_id -  id of object to query if it has upstream edges
 *
 * @return status - The status of the operation.
 *
 * @author
 *  03/17/2010 - Created. Jesus Medina
 ****************************************************************/

fbe_status_t fbe_create_lib_get_upstream_edge_list(
        fbe_object_id_t object_id, fbe_u32_t *number_of_upstream_objects,
        fbe_object_id_t upstream_object_list_array[])
{
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_base_config_upstream_object_list_t   upstream_object;
    fbe_u32_t                                i = 0;

    upstream_object.number_of_upstream_objects = 0;
    status = fbe_create_lib_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                                 &upstream_object,
                                                 sizeof(fbe_base_config_upstream_object_list_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_INVALID,
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: failed.  Error: 0x%x\n", 
                          __FUNCTION__, status);
    }
    else
    {    
        if (upstream_object.number_of_upstream_objects > 0)
        {
            *number_of_upstream_objects = upstream_object.number_of_upstream_objects;
            for (i=0 ; i < *number_of_upstream_objects; i++)
            {
                upstream_object_list_array[i] = upstream_object.upstream_object_list[i];
            }
        }
    }
    return status;
}
/**********************************************
 * end fbe_create_lib_get_upstream_edge_list()
 **********************************************/


/*!**************************************************************
 * fbe_create_lib_get_pvd_upstream_edge_list()
 ****************************************************************
 * @brief
 * This function sends a packet to the base config to determine
 * of the raid group object has upstream objects connected to it,
 * if there are objects connected to it, it returns the number of
 * objects connected, as well as a list of objects
 *
 * @param object_id -  id of object to query if it has upstream edges
 *
 * @return status - The status of the operation.
 *
 * @author
 *  05/08/2012 - Created. Ashwin Tamilarasan
 ****************************************************************/

fbe_status_t fbe_create_lib_get_pvd_upstream_edge_list(
        fbe_object_id_t object_id, fbe_u32_t *number_of_upstream_objects,
        fbe_object_id_t upstream_rg_object_list_array[],
        fbe_object_id_t upstream_vd_object_list_array[])
{
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_base_config_upstream_object_list_t   upstream_object;
    fbe_base_config_upstream_object_list_t   upstream_user_rgs_objects;
    fbe_u32_t                                i = 0;

    upstream_object.number_of_upstream_objects = 0;
    status = fbe_create_lib_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                                 &upstream_object,
                                                 sizeof(fbe_base_config_upstream_object_list_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_INVALID,
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: failed.  Error: 0x%x\n", 
                          __FUNCTION__, status);
    }
    else
    {    
        if (upstream_object.number_of_upstream_objects > 0)
        {
            *number_of_upstream_objects = upstream_object.number_of_upstream_objects;
            for (i=0 ; i < *number_of_upstream_objects; i++)
            {
                upstream_vd_object_list_array[i] = FBE_OBJECT_ID_INVALID;
                /* If there are user rgs in the first 4 drives then we need to go up one
                   level more to get the raid objects 
                */
                if(!fbe_private_space_layout_object_id_is_system_raid_group(upstream_object.upstream_object_list[i]))
                {
                    status = fbe_create_lib_send_control_packet(FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                                 &upstream_user_rgs_objects,
                                                 sizeof(fbe_base_config_upstream_object_list_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_INVALID,
                                                 upstream_object.upstream_object_list[i],
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

                    upstream_rg_object_list_array[i] = upstream_user_rgs_objects.upstream_object_list[0];
                    upstream_vd_object_list_array[i] = upstream_object.upstream_object_list[i];

                }
                else
                {
                    upstream_rg_object_list_array[i] = upstream_object.upstream_object_list[i];
                }

            }
        }
    }
    return status;
}
/**********************************************
 * end fbe_create_lib_get_pvd_upstream_edge_list()
 **********************************************/

/*!**************************************************************
 * fbe_create_lib_get_edge_block_geometryt()
 ****************************************************************
 * @brief
 * This function sends a packet to the base config to determine
 * the block geometry of an edge
 *
 * @param object_id -  id of object to query its block geometry
 *
 * @return status - The status of the operation.
 *
 * @author
 *  03/03/2011 - Created. Jesus Medina
 ****************************************************************/

fbe_status_t fbe_create_lib_get_downstream_edge_geometry(
        fbe_object_id_t object_id, 
        fbe_block_edge_geometry_t *out_edge_block_geometry)
{
    fbe_status_t                             status = FBE_STATUS_OK;
    fbe_base_config_block_edge_geometry_t    edge_block_geometry;

    edge_block_geometry.block_geometry = FBE_BLOCK_EDGE_GEOMETRY_INVALID;
    status = fbe_create_lib_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_EDGE_GEOMETRY,
            &edge_block_geometry,
            sizeof(fbe_base_config_block_edge_geometry_t),
            FBE_PACKAGE_ID_SEP_0,
            FBE_SERVICE_ID_TOPOLOGY,
            FBE_CLASS_ID_INVALID,
            object_id,
            FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s: failed.  Error: 0x%x\n", 
                __FUNCTION__, status);
    }
    else
    {    
        *out_edge_block_geometry = edge_block_geometry.block_geometry;
    }
    return status;
}
/**********************************************
 * end fbe_create_lib_get_edge_block_geometryt()
 **********************************************/

/*!**************************************************************
 * fbe_create_lib_get_rg_type()
 ****************************************************************
 * @brief
 * This function sends a packet to the topology service to determine
 * of the raid group type
 *
 * @param object_id IN -  id of raid group object
 * @param raid_group_type OUT - RG type
 * @return status - The status of the operation.
 *
 * @author
 *  08/16/2010 - Created. Jesus Medina
 ****************************************************************/

fbe_status_t fbe_create_lib_get_raid_group_type(
        fbe_object_id_t object_id, fbe_raid_group_type_t *raid_group_type)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_raid_group_get_info_t               raid_group_info;

    raid_group_info.raid_type  = FBE_RAID_GROUP_TYPE_UNKNOWN;
    status = fbe_create_lib_send_control_packet (FBE_RAID_GROUP_CONTROL_CODE_GET_INFO,
                                                 &raid_group_info,
                                                 sizeof(fbe_raid_group_get_info_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_INVALID,
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: failed.  Error: 0x%x\n", 
                          __FUNCTION__, status);
    }
    else
    {    
        *raid_group_type = raid_group_info.raid_type;
    }
    return status;
}
/**********************************************
 * end fbe_create_lib_get_raid_group_type()
 **********************************************/

/*!**************************************************************
 * fbe_create_lib_get_packet_contents()
 ****************************************************************
 * @brief
 * This function checks for proper packet contents and 
 * returns a pointer to a job queue element
 *
 * @param packet_p - packet pointer.
 * @param out_job_queue_element_p - returning
 * job queue element.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/12/2010 - Created. Jesus Medina
 ****************************************************************/
fbe_status_t fbe_create_lib_get_packet_contents (fbe_packet_t *packet_p,
                                                 fbe_job_queue_element_t **out_job_queue_element_p)
{
    fbe_job_queue_element_t              *job_queue_element_p = NULL;
    fbe_payload_ex_t                        *payload_p = NULL;
    fbe_payload_control_operation_t      *control_operation = NULL;
    fbe_status_t                         status = FBE_STATUS_OK;
    fbe_payload_control_buffer_length_t  length = 0;

    /* TODO: this function must have event logs */
    job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /*! Get the request payload. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation, &job_queue_element_p);

    if (job_queue_element_p == NULL) 
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s entry, job queue element invalid", 
                __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_job_queue_element_t))
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s entry, invalid job queue element size", 
                __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* check if commad data is NULL */
    if (job_queue_element_p->command_data == NULL)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s entry, command_data for job service request is empty", 
                __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *out_job_queue_element_p = job_queue_element_p;

    return status;
}
/******************************************
 * end fbe_create_lib_get_packet_contents()
 ******************************************/

/*!**************************************************************
 * fbe_create_lib_get_object_state
 ****************************************************************
 * @brief
 * This function gets the lifecycle_state of an object in SEP.  
 *
 * @param object_id - object id. (IN)
 * @param lifecycle_state - lifecycle_state (OUT)
 *
 * @return status - The status of the operation.
 *
 * @author
 *  12/8/2010 - Created.
 ****************************************************************/
fbe_status_t fbe_create_lib_get_object_state(fbe_object_id_t object_id, fbe_lifecycle_state_t * lifecycle_state)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_object_mgmt_get_lifecycle_state_t      base_object_mgmt_get_lifecycle_state;   

    status = fbe_create_lib_send_control_packet (FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
                                                 &base_object_mgmt_get_lifecycle_state,
                                                 sizeof(fbe_base_object_mgmt_get_lifecycle_state_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_INVALID,
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);
    if (status == FBE_STATUS_NO_OBJECT) {
        job_service_trace (FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: requested object 0x%x does not exist\n", __FUNCTION__, object_id);
        return status;
    }

    /* If object is in SPECIALIZE STATE topology will return FBE_STATUS_BUSY */
    if (status == FBE_STATUS_BUSY) {
        *lifecycle_state = FBE_LIFECYCLE_STATE_SPECIALIZE;
        return FBE_STATUS_OK;
    }

    if (status != FBE_STATUS_OK) {
        job_service_trace (FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                           "%s:packet error:%d, \n", 
                           __FUNCTION__,
                           status);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    *lifecycle_state = base_object_mgmt_get_lifecycle_state.lifecycle_state;

    return status;

}
/**********************************************************
 * end fbe_create_lib_get_object_state()
 **********************************************************/


/*!****************************************************************************
 * fbe_pvd_event_log_sniff_verify_enable_disable 
 ******************************************************************************
 * @brief
 *   Functions logs message on sniff verify being enabled or disabled
 * 
 * @param pvd_object_id         - PVD's object ID 
 * @param sniff_verify_state    - TRUE - Sniff verify enabled; 
 *                                FALSE - Sniff verify disabled
 *
 * @return  fbe_status_t
 * 
 * @author
 *  12/8/2010 - Created.
 ******************************************************************************/
fbe_status_t fbe_pvd_event_log_sniff_verify_enable_disable(fbe_object_id_t pvd_object_id, 
                                                           fbe_bool_t sniff_verify_state)
{
    fbe_provision_drive_get_physical_drive_location_t physical_drive_location;
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t           message_code;
    
    physical_drive_location.port_number = FBE_PORT_NUMBER_INVALID; 
    physical_drive_location.enclosure_number = FBE_ENCLOSURE_NUMBER_INVALID; 
    physical_drive_location.slot_number = FBE_SLOT_NUMBER_INVALID;
    
    //send usurper to pvd object to get it's drive location.
    status = fbe_create_lib_send_control_packet(FBE_PROVISION_DRIVE_CONTROL_CODE_GET_DRIVE_LOCATION,
                                                &physical_drive_location,
                                                sizeof(fbe_provision_drive_get_physical_drive_location_t),
                                                FBE_PACKAGE_ID_SEP_0,
                                                FBE_SERVICE_ID_TOPOLOGY,
                                                FBE_CLASS_ID_INVALID,
                                                pvd_object_id,
                                                FBE_PACKET_FLAG_NO_ATTRIB);
    
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_INFO, "%s get drive location failed, status:0x%x\n",
                          __FUNCTION__, status);
        // not returning from here, as worse case scenario we can still log information 
        // about sniff verify state has changed using the object id for PVD we have
    }
    
    if (sniff_verify_state == FBE_TRUE)
    {
        message_code = SEP_INFO_SNIFF_VERIFY_ENABLED;
    }
    else
    {
        message_code = SEP_INFO_SNIFF_VERIFY_DISABLED;
    }
    
    //log message to the event-log  
    fbe_event_log_write(message_code, NULL, 0, "%x %d %d %d",
                        pvd_object_id, 
                        physical_drive_location.port_number, 
                        physical_drive_location.enclosure_number, 
                        physical_drive_location.slot_number);    
    
    return FBE_STATUS_OK;    
}

/*!**************************************************************
 * fbe_create_lib_lookup_lun_object_id()
 ****************************************************************
 * @brief
 * This function returns the logical object id of a LUN user id.
 *
 * !TODO consider moving this function to the appropriate utility file
 *
 * @param lun_number        - user LUN id (IN)
 * @param lun_object_id_p   - logical object id (OUT)
 *
 * @return status
 *
 * @author
 *  02/2010 - Created. rundbs
 ****************************************************************/
fbe_status_t fbe_create_lib_lookup_lun_object_id(fbe_lun_number_t lun_number, fbe_object_id_t *lun_object_id_p)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_lookup_lun_t      lookup_lun;

    /* Set the user LUN number for lookup */
    lookup_lun.lun_number       = lun_number;

    /* Get LUN object ID from the Config Service */
    status = fbe_create_lib_send_control_packet (
            FBE_DATABASE_CONTROL_CODE_LOOKUP_LUN_BY_NUMBER,
            &lookup_lun,
            sizeof(fbe_database_control_lookup_lun_t),
            FBE_PACKAGE_ID_SEP_0,
            FBE_SERVICE_ID_DATABASE,
            FBE_CLASS_ID_LUN,
            FBE_OBJECT_ID_INVALID,
            FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        job_service_trace(FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s entry; status %d\n", __FUNCTION__, status);
    }

    /* Set the LUN object ID returned from the Config Service */
    *lun_object_id_p = lookup_lun.object_id;

    return status;
}
/**********************************************************
 * end fbe_create_lib_lookup_lun_object_id()
 **********************************************************/

/*!**************************************************************
 * fbe_raid_group_create_check_if_sys_drive()
 ****************************************************************
 * @brief
 * This function checks whether the drive is a system drive or not
 *
 *
 * @param pvd_object_id       
 * @param is_sys_drive_p
 *
 * @return status
 *
 * @author
 *  04/23/2012 - Created. Ashwin Tamilarasan
 *  05/30/2012 - Modified. He, Wei
 ****************************************************************/
fbe_status_t fbe_raid_group_create_check_if_sys_drive(fbe_object_id_t pvd_object_id,
                                                      fbe_bool_t    *is_sys_drive_p)
{
    *is_sys_drive_p = fbe_private_space_layout_object_id_is_system_pvd(pvd_object_id);
    return FBE_STATUS_OK;
}

/**********************************************************
 * end fbe_raid_group_create_check_if_sys_drive()
 **********************************************************/


/*!**************************************************************
 * fbe_create_lib_validate_system_triple_mirror_healthy()
 ****************************************************************
 * @brief
 * This function checks whether the system triple mirror raid group is healthy.
 * If the triple mirror raid group is double degraded, return error.
 * 
 *
 * @ args: double_degraded: if more than one drive degraded, it will be set as true.
 * 
 * @return status : 
 * @author
 *  08/07/2012 - Created. gaoh1
 *
  ****************************************************************/
fbe_status_t fbe_create_lib_validate_system_triple_mirror_healthy(fbe_bool_t *double_degraded)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_raid_group_get_info_t   raid_group_info;
    fbe_u32_t   index;
    fbe_u32_t   degraded_drives = 0;

    if (double_degraded == NULL) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: invalid argument\n", 
                          __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_zero_memory(&raid_group_info, sizeof(fbe_raid_group_get_info_t));
    status = fbe_create_lib_send_control_packet (FBE_RAID_GROUP_CONTROL_CODE_GET_INFO,
                                                 &raid_group_info,
                                                 sizeof(fbe_raid_group_get_info_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_INVALID,
                                                 FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_TRIPLE_MIRROR,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);
    
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: failed.  Error: 0x%x\n", 
                          __FUNCTION__, status);
        return status;
    }

    for (index = 0; index < raid_group_info.width; index++)
    {
        if (raid_group_info.rebuild_checkpoint[index] != FBE_LBA_INVALID)
            degraded_drives++;
    }

    if (degraded_drives >= 2)
        *double_degraded = FBE_TRUE;
    else
        *double_degraded = FBE_FALSE;

    return FBE_STATUS_OK;

}


/*!**************************************************************
 * fbe_create_lib_connect_pvd_to_downstream_drive()
 ****************************************************************
 * @brief
 * This function connect a PVD to its downstream drive by serial number
 *
 * @param transaction_id - transaction id for configuration service
 * @param server_object_id - edge's server id
 * @param client_object_id - edge's client id
 * @param client_index - edge's index
 * @param client_imported_capacity - edge's capacity
 * @param offset - edge's offset
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/19/2010 - Created. guov
 ****************************************************************/
fbe_status_t fbe_create_lib_get_drive_info_by_serial_number(fbe_u8_t* serial_number, fbe_create_lib_physical_drive_info_t* drive_info)
{    
    fbe_physical_drive_get_location_t          disk_info;
    fbe_physical_drive_attributes_t            drive_attr;
    fbe_topology_control_get_physical_drive_by_sn_t  topology_search_sn;
    fbe_status_t                               status;

    /*first get the object id of the logical drive object*/
    topology_search_sn.sn_size = sizeof(topology_search_sn.product_serial_num) - 1;
    fbe_copy_memory(topology_search_sn.product_serial_num, serial_number, topology_search_sn.sn_size);
    topology_search_sn.product_serial_num[topology_search_sn.sn_size] = '\0';
    topology_search_sn.object_id = FBE_OBJECT_ID_INVALID;

    status = fbe_create_lib_send_control_packet (FBE_TOPOLOGY_CONTROL_CODE_GET_PHYSICAL_DRIVE_BY_SERIAL_NUMBER,
                                                 &topology_search_sn,
                                                 sizeof(topology_search_sn),
                                                 FBE_PACKAGE_ID_PHYSICAL,
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_EXTERNAL);
    if(FBE_STATUS_OK != status || FBE_OBJECT_ID_INVALID == topology_search_sn.object_id)
    {
            job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_INFO, "%s get pdo object id failed, status:0x%x\n",
                          __FUNCTION__, status);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /*then get the pdo info*/
    status = fbe_create_lib_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_LOCATION,
                                                 &disk_info,
                                                 sizeof(disk_info),
                                                 FBE_PACKAGE_ID_PHYSICAL,
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_INVALID,
                                                 topology_search_sn.object_id,
                                                 FBE_PACKET_FLAG_EXTERNAL);
    if( status != FBE_STATUS_OK )
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                      FBE_TRACE_MESSAGE_ID_INFO, "%s get pdo info failed, status:0x%x, objid:0x%x\n",
                      __FUNCTION__, status, topology_search_sn.object_id);
        return status;
    }

    status = fbe_create_lib_send_control_packet (FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_ATTRIBUTES,
                                                 &drive_attr,
                                                 sizeof(drive_attr),
                                                 FBE_PACKAGE_ID_PHYSICAL,
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_INVALID,
                                                 topology_search_sn.object_id,
                                                 FBE_PACKET_FLAG_EXTERNAL);
    if( status != FBE_STATUS_OK )
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                      FBE_TRACE_MESSAGE_ID_INFO, "%s get pdo attr failed, status:0x%x, objid:0x%x\n",
                      __FUNCTION__, status, topology_search_sn.object_id);
        return status;
    }

    
    drive_info->drive_object_id  = topology_search_sn.object_id;
    drive_info->bus              = disk_info.port_number;
    drive_info->enclosure        = disk_info.enclosure_number;
    drive_info->slot             = disk_info.slot_number;

    if ( drive_attr.physical_block_size == 520 )
    {
        drive_info->block_geometry = FBE_BLOCK_EDGE_GEOMETRY_520_520;
    }
    else if ( drive_attr.physical_block_size == 4160 )
    {
        drive_info->block_geometry = FBE_BLOCK_EDGE_GEOMETRY_4160_4160;
    }
    else
    {
        /* Unsupported physical block size.
         */
        job_service_trace(FBE_TRACE_LEVEL_ERROR,
                          FBE_TRACE_MESSAGE_ID_INFO, 
                          "%s get pdo attr obj: 0x%x unsupported physical block size: %d\n",
                          __FUNCTION__, topology_search_sn.object_id, drive_attr.physical_block_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_copy_memory(drive_info->serial_num, drive_attr.initial_identify.identify_info, sizeof(fbe_u8_t)*FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE );

    drive_info->serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE] ='\0';

    drive_info->exported_capacity = drive_attr.imported_capacity;

    return status;
}

/*!****************************************************************************
 * fbe_create_lib_get_raid_health(fbe_object_id_t object_id, fbe_bool_t *is_broken)
 ******************************************************************************
 * @brief
 * This function is used to get the health of the raid group.
 *
 * @param object_id: the raid group object id.
 * @param is_broken(out) : whether the raid group is broken.
 *
 * @return status                 - The status of the operation.
 *
 * @author
 *  11/01/2012 - Created. gaoh1
 ******************************************************************************/
fbe_status_t
fbe_create_lib_get_raid_health(fbe_object_id_t object_id, fbe_bool_t *is_broken)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_base_config_downstream_health_state_t downstream_health;

    if (object_id == FBE_OBJECT_ID_INVALID || is_broken == NULL) {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: invalid argument. object id: %d\n", 
                          __FUNCTION__, object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = fbe_create_lib_send_control_packet (FBE_RAID_GROUP_CONTROL_CODE_GET_HEALTH,
                                                 &downstream_health,
                                                 sizeof(fbe_base_config_downstream_health_state_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_INVALID,
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);
    
    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s: failed.  Error: 0x%x\n", 
                          __FUNCTION__, status);
        return status;
    }

    /* Broken edge will become disable edge ?*/
    if (downstream_health == FBE_DOWNSTREAM_HEALTH_BROKEN || downstream_health == FBE_DOWNSTREAM_HEALTH_DISABLED) {
        *is_broken = FBE_TRUE;
    } else {
        *is_broken = FBE_FALSE;
    }

    return FBE_STATUS_OK;
}

fbe_status_t fbe_create_lib_get_list_of_downstream_objects(fbe_object_id_t object_id,
                                                                          fbe_object_id_t downstream_object_list[],
                                                                          fbe_u32_t *number_of_downstream_objects)

{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_config_downstream_object_list_t        config_downstream_object_list;
    fbe_u16_t                                       index = 0;

    /* Initialize downstream object list before sending control command to get info. */
    *number_of_downstream_objects = 0; 
    config_downstream_object_list.number_of_downstream_objects = 0;
    for(index = 0; index < FBE_MAX_DOWNSTREAM_OBJECTS; index++)
    {
        config_downstream_object_list.downstream_object_list[index] = FBE_OBJECT_ID_INVALID;
    }
 
    status = fbe_create_lib_send_control_packet (FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_OBJECT_LIST,
                                                 &config_downstream_object_list,
                                                 sizeof(fbe_base_config_downstream_object_list_t),
                                                 FBE_PACKAGE_ID_SEP_0,                                                 
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 FBE_CLASS_ID_INVALID,
                                                 object_id,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status != FBE_STATUS_OK)
    {
        job_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s entry, get downstream object list call failed, status %d\n", __FUNCTION__,
                status);
        return status;
    }
    
    for(index = 0; index < config_downstream_object_list.number_of_downstream_objects; index++)
    {
        downstream_object_list[index] = config_downstream_object_list.downstream_object_list[index];
    }
    *number_of_downstream_objects = config_downstream_object_list.number_of_downstream_objects; 

    return status;
}

/*!**************************************************************
 * fbe_create_lib_database_service_lookup_ext_pool_object_id()
 ****************************************************************
 * @brief
 * This function returns the logical package id of a raid group
 * user id
 *
 * @param raid_group_id - user raid group id
 * @param rg_object_id_p - logical package id
 *
 * @return status
 *
 * @author
 *  06/26/2014 - Created. Lili Chen
 ****************************************************************/
fbe_status_t fbe_create_lib_database_service_lookup_ext_pool_object_id(fbe_u32_t pool_id, 
                                                                       fbe_object_id_t * object_id)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_lookup_ext_pool_t lookup_pool;

    lookup_pool.pool_id  = pool_id;

    status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_LOOKUP_EXT_POOL_BY_NUMBER,
                                                 &lookup_pool,
                                                 sizeof(fbe_database_control_lookup_ext_pool_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_DATABASE,
                                                 FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s, raid object id could not be located \n", 
                          __FUNCTION__);
        return status;
    }

    *object_id = lookup_pool.object_id;

    return status;
}
/******************************************************************
 * end fbe_create_lib_database_service_lookup_ext_pool_object_id()
 *****************************************************************/

/*!**************************************************************
 * fbe_create_lib_database_service_lookup_ext_pool_lun_object_id()
 ****************************************************************
 * @brief
 * This function returns the logical package id of a raid group
 * user id
 *
 * @param raid_group_id - user raid group id
 * @param rg_object_id_p - logical package id
 *
 * @return status
 *
 * @author
 *  06/26/2014 - Created. Lili Chen
 ****************************************************************/
fbe_status_t fbe_create_lib_database_service_lookup_ext_pool_lun_object_id(fbe_u32_t pool_id, fbe_u32_t lun_id,
                                                                           fbe_object_id_t * object_id)
{
	fbe_status_t status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_database_control_lookup_ext_pool_lun_t lookup_lun;

    lookup_lun.pool_id  = pool_id;
    lookup_lun.lun_id  = lun_id;

    status = fbe_create_lib_send_control_packet (FBE_DATABASE_CONTROL_CODE_LOOKUP_EXT_POOL_LUN_BY_NUMBER,
                                                 &lookup_lun,
                                                 sizeof(fbe_database_control_lookup_ext_pool_lun_t),
                                                 FBE_PACKAGE_ID_SEP_0,
                                                 FBE_SERVICE_ID_DATABASE,
                                                 FBE_CLASS_ID_INVALID,
                                                 FBE_OBJECT_ID_INVALID,
                                                 FBE_PACKET_FLAG_NO_ATTRIB);

    if (status == FBE_STATUS_GENERIC_FAILURE)
    {
        job_service_trace(FBE_TRACE_LEVEL_WARNING, 
                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                          "%s, raid object id could not be located \n", 
                          __FUNCTION__);
        return status;
    }

    *object_id = lookup_lun.object_id;

    return status;
}
/******************************************************************
 * end fbe_create_lib_database_service_lookup_ext_pool_lun_object_id()
 *****************************************************************/
