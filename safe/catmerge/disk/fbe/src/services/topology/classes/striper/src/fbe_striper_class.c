/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_striper_class.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Striper class code.
 * 
 * @ingroup fbe_striper_class_files
 * 
 * @version
 *  2/24/2010 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "fbe_spare.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe_striper_private.h"

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
static fbe_status_t fbe_striper_class_get_info(fbe_packet_t * packet);


/*!***************************************************************
 * fbe_striper_class_control_entry()
 ****************************************************************
 * @brief
 *  This function is the management packet entry point for
 *  operations to the Striper class.
 *
 * @param packet - The packet that is arriving.
 *
 * @return fbe_status_t - Return status of the operation.
 * 
 * @author
 *  2/24/2010 - Created. Rob Foley
 ****************************************************************/
fbe_status_t fbe_striper_class_control_entry(fbe_packet_t * packet)
{
    fbe_status_t                            status;
    fbe_payload_ex_t *                     sep_payload = NULL;
    fbe_payload_control_operation_t *       control_operation = NULL;
    fbe_payload_control_operation_opcode_t  opcode;

    sep_payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);
    fbe_payload_control_get_opcode(control_operation, &opcode);

    switch (opcode)
    {
        case FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_INFO:
            status = fbe_striper_class_get_info(packet);
            break;
        default:
            status = fbe_raid_group_class_control_entry(packet);
            break;
    }
    return status;
}
/********************************************
 * end fbe_striper_class_control_entry
 ********************************************/

/*!***************************************************************
 * fbe_striper_class_get_info()
 ****************************************************************
 * @brief
 *  This function calculates class specific information.
 *
 * @param packet_p - The packet that is arriving.
 *
 * @return fbe_status_t - Return status of the operation.
 * 
 * @author
 *  2/24/2010 - Created. Rob Foley
 ****************************************************************/
static fbe_status_t 
fbe_striper_class_get_info(fbe_packet_t * packet_p)
{
    fbe_raid_group_class_get_info_t * get_info_p = NULL;  /* INPUT */
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL; 

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  

    fbe_payload_control_get_buffer(control_operation_p, &get_info_p);

    if ((get_info_p != NULL) &&
        (get_info_p->raid_type != FBE_RAID_GROUP_TYPE_RAID0) &&
        (get_info_p->raid_type != FBE_RAID_GROUP_TYPE_RAID10))
    {
        fbe_topology_class_trace(FBE_CLASS_ID_STRIPER, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s raid type %d unexpected\n", __FUNCTION__, get_info_p->raid_type);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Hand off to the raid group. This function always completes the packet.
     */
    fbe_raid_group_class_get_info(packet_p);
    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_striper_class_get_info
 ********************************************/

/************************
 * end fbe_striper_class.c
 ************************/


