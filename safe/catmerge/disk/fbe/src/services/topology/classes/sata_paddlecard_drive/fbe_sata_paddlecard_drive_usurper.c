/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_sata_paddlecard_drive_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the sata paddlecard drive object usurper functions.
 * 
 * HISTORY
 *   1/05/2011:  Created. Wayne Garrett
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_sas_port.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"

#include "sas_physical_drive_private.h"
#include "sata_paddlecard_drive_private.h"



/*!***************************************************************
 * fbe_sata_paddlecard_drive_control_entry()
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
 *   1/05/2011:  Created. Wayne Garrett
 *
 ****************************************************************/
fbe_status_t 
fbe_sata_paddlecard_drive_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_sata_paddlecard_drive_t * sata_paddlecard_drive = NULL;

    sata_paddlecard_drive = (fbe_sata_paddlecard_drive_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace(  (fbe_base_object_t*)sata_paddlecard_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry.\n", __FUNCTION__);

    control_code = fbe_transport_get_control_code(packet);
    switch(control_code) {
        default:
            status = fbe_sas_physical_drive_control_entry(object_handle, packet);
            break;
    }

    return status;
}

