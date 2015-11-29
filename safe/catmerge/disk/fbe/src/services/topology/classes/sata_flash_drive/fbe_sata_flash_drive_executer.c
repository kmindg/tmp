/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_sata_flash_drive_executer.c
 ***************************************************************************
 *
 * @brief
 *  This file contains executer functions for sata flash drive. 
 * 
 * HISTORY
 *   2/22/2010:  Created. chenl6
 *
 ***************************************************************************/
 
/*************************
  *   INCLUDE FILES
  *************************/
#include "fbe/fbe_types.h"
#include "sata_flash_drive_private.h"
#include "fbe_transport_memory.h"
#include "fbe_scsi.h"
#include "sata_physical_drive_private.h"

 /*************************
  *   FUNCTION DEFINITIONS
  *************************/
static fbe_status_t sata_flash_drive_handle_block_io_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);


/*!**************************************************************
 * @fn fbe_sata_flash_drive_event_entry(
 *            fbe_object_handle_t object_handle,
 *            fbe_event_type_t event_type,
 *            fbe_event_context_t event_context)
 ****************************************************************
 * @brief
 *   This is the event entry function for sata flash drive.
 *  
 * @param object_handle - object handle. 
 * @param event_type - event type.
 * @param event_context - event context.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  2/22/2010 - Created. chenl6
 *
 ****************************************************************/
fbe_status_t 
fbe_sata_flash_drive_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
    fbe_sata_flash_drive_t * sata_flash_drive = NULL;
    fbe_status_t status;

    sata_flash_drive = (fbe_sata_flash_drive_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)sata_flash_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Handle the event we have received. */
    switch (event_type)
    {
        default:
            status = fbe_sata_physical_drive_event_entry(object_handle, event_type, event_context);
            break;
    }

    return status;
}

/*!**************************************************************
 * @fn fbe_sata_flash_drive_io_entry(
 *            fbe_object_handle_t object_handle,
 *            fbe_packet_t * packet)
 ****************************************************************
 * @brief
 *   This is the IO entry function for sata flash drive.
 *  
 * @param object_handle - object handle. 
 * @param packet - pointer to packet.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  2/22/2010 - Created. chenl6
 *
 ****************************************************************/
fbe_status_t 
fbe_sata_flash_drive_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
     return fbe_sata_physical_drive_io_entry(object_handle, packet);
}

#if 0
fbe_status_t 
fbe_sata_flash_drive_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_transport_id_t transport_id;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_sata_flash_drive_t *sata_flash_drive;
    sata_flash_drive = (fbe_sata_flash_drive_t *)fbe_base_handle_to_pointer(object_handle);

    /* First we need to figure out to what transport this packet belong.
     */
    fbe_transport_get_transport_id(packet, &transport_id);
    switch (transport_id)
    {
        case FBE_TRANSPORT_ID_DISCOVERY:
            status = fbe_sata_physical_drive_io_entry(object_handle, packet);
            break;
        case FBE_TRANSPORT_ID_BLOCK:
            fbe_transport_set_completion_function(packet, sata_flash_drive_handle_block_io_completion, sata_flash_drive);
            status = fbe_sata_physical_drive_io_entry(object_handle, packet);
            break;
        default:
            break;
    };

    return status;
}

/*!**************************************************************
 * @fn sata_flash_drive_handle_block_io_completion(
 *            ffbe_packet_t * packet,
 *            fbe_packet_completion_context_t context)
 ****************************************************************
 * @brief
 *   This is the completion function for the packet sent to a drive.
 *  
 * @param packet - pointer to packet. 
 * @param context - completion context.
 * 
 * @return fbe_status_t
 *
 * HISTORY:
 *  2/22/2010 - Created. chenl6
 *
 ****************************************************************/
static fbe_status_t 
sata_flash_drive_handle_block_io_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_sata_flash_drive_t * sata_flash_drive = NULL;
    fbe_status_t status = FBE_STATUS_OK;
        
    sata_flash_drive = (fbe_sata_flash_drive_t *)context;
    
    /* Check status of packet. */
    status = fbe_transport_get_status_code (packet);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(  (fbe_base_object_t*)sata_flash_drive,
                                     FBE_TRACE_LEVEL_ERROR,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                     "%s, packet status failed:0x%X\n", __FUNCTION__, status);
                                     
       return status;       
    }

    return status;
}
#endif