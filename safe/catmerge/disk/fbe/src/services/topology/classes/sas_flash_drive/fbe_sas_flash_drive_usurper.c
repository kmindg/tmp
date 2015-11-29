/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_sas_flash_drive_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the sas flash drive object usurper functions.
 * 
 * HISTORY
 *   2/22/2010:  Created. chenl6
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "sas_flash_drive_private.h"
#include "fbe_sas_port.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"

/* Forward declaration */
static fbe_status_t sas_flash_drive_get_log_page(fbe_sas_flash_drive_t * sas_flash_drive, fbe_packet_t * packet);
static fbe_status_t sas_flash_drive_usurper_sanitize_drive(fbe_sas_flash_drive_t * sas_flash_drive, fbe_packet_t * packet, fbe_scsi_sanitize_pattern_t pattern);
static fbe_status_t sas_flash_drive_usurper_get_sanitize_status(fbe_sas_flash_drive_t * sas_flash_drive, fbe_packet_t * packet);
static fbe_status_t sas_flash_drive_get_vpd_page(fbe_sas_flash_drive_t *sas_flash_drive, fbe_packet_t *packet);

/*!***************************************************************
 * fbe_sata_flash_drive_control_entry()
 ****************************************************************
 * @brief
 *  This function is the management packet entry point for
 *  operations to the logical drive object.
 *
 * @param object_handle - object handle.
 * @param packet - pointer to packet.
 *
 * @return fbe_status_t
 * 
 * @author
 *   2/22/2010:  Created. chenl6
 *
 ****************************************************************/
fbe_status_t 
fbe_sas_flash_drive_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_sas_flash_drive_t * sas_flash_drive = NULL;

    sas_flash_drive = (fbe_sas_flash_drive_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace(  (fbe_base_object_t*)sas_flash_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry.\n", __FUNCTION__);

    control_code = fbe_transport_get_control_code(packet);
    switch(control_code) {
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_LOG_PAGES:
            status = sas_flash_drive_get_log_page(sas_flash_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_VPD_PAGES:
            status = sas_flash_drive_get_vpd_page(sas_flash_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SANITIZE_DRIVE_ERASE_ONLY:
            status = sas_flash_drive_usurper_sanitize_drive(sas_flash_drive, packet, FBE_SCSI_FORMAT_UNIT_SANITIZE_PATTERN_ERASE_ONLY);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SANITIZE_DRIVE_OVERWRITE_AND_ERASE:
            status = sas_flash_drive_usurper_sanitize_drive(sas_flash_drive, packet, FBE_SCSI_FORMAT_UNIT_SANITIZE_PATTERN_OVERWRITE_AND_ERASE);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SANITIZE_DRIVE_AFSSI:
            status = sas_flash_drive_usurper_sanitize_drive(sas_flash_drive, packet, FBE_SCSI_FORMAT_UNIT_SANITIZE_PATTERN_AFSSI);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_SANITIZE_DRIVE_NSA:
            status = sas_flash_drive_usurper_sanitize_drive(sas_flash_drive, packet, FBE_SCSI_FORMAT_UNIT_SANITIZE_PATTERN_NSA);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_SANITIZE_STATUS:
            status = sas_flash_drive_usurper_get_sanitize_status(sas_flash_drive, packet);
            break;
        default:
            status = fbe_sas_physical_drive_control_entry(object_handle, packet);
            break;
    }

    return status;
}

static fbe_status_t 
sas_flash_drive_get_log_page(fbe_sas_flash_drive_t * sas_flash_drive, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_mgmt_get_log_page_t * get_log_page = NULL;
    fbe_payload_control_buffer_length_t len = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Check buffer in control payload */
    fbe_payload_control_get_buffer(control_operation, &get_log_page);
    if(get_log_page == NULL){
         fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
         fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_physical_drive_mgmt_get_log_page_t)){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Send to executer for inquiry. */
    status = fbe_sas_flash_drive_send_log_sense(sas_flash_drive, packet);

    return FBE_STATUS_PENDING;
}

static fbe_status_t 
sas_flash_drive_get_vpd_page(fbe_sas_flash_drive_t * sas_flash_drive, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_mgmt_get_log_page_t * get_log_page = NULL;
    fbe_payload_control_buffer_length_t len = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Check buffer in control payload */
    fbe_payload_control_get_buffer(control_operation, &get_log_page);
    if(get_log_page == NULL){
         fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
         fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_physical_drive_mgmt_get_log_page_t)){
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Send to executer for inquiry. */
    status = fbe_sas_flash_drive_send_vpd(sas_flash_drive, packet);

    return FBE_STATUS_PENDING;
}

/*!****************************************************************
 * sas_flash_drive_usurper_sanitize_drive()
 ******************************************************************
 * @brief
 *  This function securely erases (sanitizes) an SSD, including all
 *  the over-provisioned areas.
 *
 * @param sas_physical_drive - The SAS physical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @version
 *  07/01/2014 : Wayne Garrett  - Created.
 *
 ****************************************************************/
static fbe_status_t sas_flash_drive_usurper_sanitize_drive(fbe_sas_flash_drive_t * sas_flash_drive, fbe_packet_t * packet, fbe_scsi_sanitize_pattern_t pattern)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive, FBE_TRACE_LEVEL_INFO,
                        "%s pattern:0x%x\n",  __FUNCTION__, pattern);
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
            
    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != 0){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive, FBE_TRACE_LEVEL_ERROR,
                            "%s len != 0\n",  __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_sas_flash_drive_send_sanitize(sas_flash_drive, packet, pattern);

    return FBE_STATUS_PENDING;
}

/*!****************************************************************
 * sas_flash_drive_usurper_get_sanitize_status()
 ******************************************************************
 * @brief
 *  This function gets the sanitization status.  Status of OK
 *  indicates that santize was either never run, or it ran and
 *  was succesful, (i.e. it's not currently in progress).
 *
 * @param sas_physical_drive - The SAS physical drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @version
 *  07/23/2014 : Wayne Garrett  - Created.
 *
 ****************************************************************/
static fbe_status_t sas_flash_drive_usurper_get_sanitize_status(fbe_sas_flash_drive_t * sas_flash_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_physical_drive_sanitize_info_t * get_sanitize_status = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_status_t status = FBE_STATUS_OK;

    fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive, FBE_TRACE_LEVEL_DEBUG_HIGH,
                                               "%s entry\n",  __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Check buffer in control payload */
    fbe_payload_control_get_buffer(control_operation, &get_sanitize_status);
    if(get_sanitize_status == NULL){
         fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
         fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(*get_sanitize_status)){
        fbe_base_physical_drive_customizable_trace((fbe_base_physical_drive_t*)sas_flash_drive, FBE_TRACE_LEVEL_ERROR,
                                                   "%s Invalid len. expected/actual %d/%d\n",  __FUNCTION__, (int)sizeof(*get_sanitize_status), len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_sas_flash_drive_send_sanitize_tur(sas_flash_drive, packet);

    return FBE_STATUS_PENDING;
}


