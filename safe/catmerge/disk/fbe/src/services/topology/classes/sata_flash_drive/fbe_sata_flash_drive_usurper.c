/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_sata_flash_drive_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the sata flash drive object usurper functions.
 * 
 * HISTORY
 *   2/22/2010:  Created. chenl6
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "sata_flash_drive_private.h"
#include "fbe_sas_port.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"

/* Forward declaration */
fbe_status_t sata_flash_drive_get_log_page(fbe_sata_flash_drive_t * sata_flash_drive, fbe_packet_t * packet);

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
fbe_sata_flash_drive_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_sata_flash_drive_t * sata_flash_drive = NULL;

    sata_flash_drive = (fbe_sata_flash_drive_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace(  (fbe_base_object_t*)sata_flash_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry.\n", __FUNCTION__);

    control_code = fbe_transport_get_control_code(packet);
    switch(control_code) {
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_LOG_PAGES:
            status = sata_flash_drive_get_log_page(sata_flash_drive, packet);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_VPD_PAGES:
            status = sata_flash_drive_get_log_page(sata_flash_drive, packet);
            break;
        default:
            status = fbe_sata_physical_drive_control_entry(object_handle, packet);
            break;
    }

    return status;
}

fbe_status_t 
sata_flash_drive_get_log_page(fbe_sata_flash_drive_t * sata_flash_drive, fbe_packet_t * packet)
{
    /* fbe_status_t status; */
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

    /* At the time being, we don't support this function yet. */
    fbe_payload_control_set_status (control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK; 
}
