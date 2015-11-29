/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_parity_class.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the Parity class code .
 * 
 * @ingroup fbe_parity_class_files
 * 
 * @version
 *   01/08/2010:  Created. Jesus Medina
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
#include "fbe_parity_private.h"

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
static fbe_status_t fbe_parity_calculate_capacity(fbe_packet_t * packet);
static fbe_status_t fbe_parity_class_get_info(fbe_packet_t * packet);


/*!***************************************************************
 * fbe_parity_class_control_entry()
 ****************************************************************
 * @brief
 *  This function is the management packet entry point for
 *  operations to the parity class.
 *
 * @param packet - The packet that is arriving.
 *
 * @return fbe_status_t - Return status of the operation.
 * 
 * @author
 *  01/08/2010 - Created. Jesus Medina
 ****************************************************************/
fbe_status_t 
fbe_parity_class_control_entry(fbe_packet_t * packet)
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
        case FBE_PARITY_CONTROL_CODE_CLASS_CALCULATE_CAPACITY:
            /* Calculate the position for the radi group metadata and
             * return the exported capacity to the caller.
             */
            status = fbe_parity_calculate_capacity(packet);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_INFO:
            status = fbe_parity_class_get_info(packet);
            break;
        default:
            status = fbe_raid_group_class_control_entry(packet);
            break;
    }
    return status;
}
/********************************************
 * end fbe_parity_class_control_entry
 ********************************************/
/*!***************************************************************
 * fbe_parity_class_get_info()
 ****************************************************************
 * @brief
 * This function Calculate the metadata position for the Parity
 * object and returns exported capacity to the caller of
 * this function.
 *
 * @param packet - The packet that is arriving.
 *
 * @return fbe_status_t - Return status of the operation.
 * 
 * @author
 *  01/08/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t 
fbe_parity_class_get_info(fbe_packet_t * packet_p)
{
    fbe_raid_group_class_get_info_t * get_info_p = NULL;  /* INPUT */
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL; 

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  
    fbe_payload_control_get_buffer(control_operation_p, &get_info_p);

    if ((get_info_p != NULL) &&
        (get_info_p->raid_type != FBE_RAID_GROUP_TYPE_RAID3) &&
        (get_info_p->raid_type != FBE_RAID_GROUP_TYPE_RAID5) &&
        (get_info_p->raid_type != FBE_RAID_GROUP_TYPE_RAID6))
    {
        fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s raid type %d unexpected", __FUNCTION__, get_info_p->raid_type);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Hand off to this function. This function always completes the packet.
     */
    fbe_raid_group_class_get_info(packet_p);
    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_parity_class_get_info
 ********************************************/

/*!***************************************************************
 * fbe_parity_calculate_capacity()
 ****************************************************************
 * @brief
 * This function Calculate the metadata position for the Parity
 * object and returns exported capacity to the caller of
 * this function.
 *
 * @param packet - The packet that is arriving.
 *
 * @return fbe_status_t - Return status of the operation.
 * 
 * @author
 *  01/08/2010 - Created. Jesus Medina
 ****************************************************************/
static fbe_status_t 
fbe_parity_calculate_capacity(fbe_packet_t * packet)
{
    fbe_parity_control_class_calculate_capacity_t * calculate_capacity = NULL;  /* INPUT */
    fbe_status_t                                status;
    fbe_payload_ex_t *                         sep_payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL; 
    fbe_u32_t                                   length = 0;  
    //fbe_block_size_t                          block_size;
    //fbe_parity_metadata_positions_t           parity_metadata_positions;
    
    status = FBE_STATUS_OK;

    sep_payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);  

    fbe_payload_control_get_buffer(control_operation, &calculate_capacity);
    if (calculate_capacity == NULL)
    {
        fbe_payload_control_set_status(control_operation, 
                FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length (control_operation, &length);
    if(length != sizeof(fbe_parity_control_class_calculate_capacity_t))
    {
        fbe_payload_control_set_status(control_operation, 
                FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! @todo 
     * This is not the expected exported capacity, it requires some work, for now
     * return the imported capacity
     */
    /*
    fbe_block_transport_get_exported_block_size(calculate_capacity->block_edge_geometry, &block_size);

    status = fbe_class_parity_get_metadata_positions(calculate_capacity->imported_capacity,
                                                            block_size,
                                                            &raid_group_metadata_positions);
     */
    calculate_capacity->exported_capacity = calculate_capacity->imported_capacity;
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_parity_calculate_capacity
 ********************************************/


/*!***************************************************************
 * fbe_class_parity_get_metadata_positions()
 ****************************************************************
 * @brief
 * This function is used to calculate the position of the different
 * metadata which includes signature of the object, paged metadata
 * and non paged metadata position.
 *
 * @param packet - The packet that is arriving.
 *
 * @return fbe_status_t - Return status of the operation.
 * 
 * @author
 *  01/08/2010 - Created. Jesus Medina
 *
 ****************************************************************/
fbe_status_t fbe_parity_class_get_metadata_positions (fbe_lba_t edge_capacity, 
        fbe_block_size_t block_size,
        fbe_parity_metadata_positions_t * parity_metadata_positions)
{
    return FBE_STATUS_OK;
}

/************************
 * end fbe_parity_class.c
 ************************/


