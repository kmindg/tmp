/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_mirror_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the mirror object code for the usurper.
 * 
 * @ingroup mirror_class_files
 * 
 * @version
 *   05/20/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_object.h"
#include "fbe_raid_group_object.h"
#include "fbe_mirror_private.h"

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
static fbe_status_t fbe_mirror_map_lba(fbe_mirror_t * mirror_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_mirror_map_pba(fbe_mirror_t * mirror_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_internal_mirror_exit_hibernation(fbe_mirror_t * mirror_p, fbe_packet_t * packet_p);

/*!***************************************************************
 * fbe_mirror_control_entry()
 ****************************************************************
 * @brief
 *  This function is the management packet entry point for
 *  operations to the mirror object.
 *
 * @param object_handle - The config handle.
 * @param packet_p - The packet that is arriving.
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t 
fbe_mirror_control_entry(fbe_object_handle_t object_handle, 
                         fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;
    fbe_payload_control_operation_opcode_t opcode;
    fbe_mirror_t *mirror_p = NULL;
    fbe_raid_group_type_t       raid_type;
    fbe_raid_geometry_t *raid_geometry_p = NULL;

    mirror_p = (fbe_mirror_t *)fbe_base_handle_to_pointer(object_handle);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_opcode(control_p, &opcode);

    /* If object handle is NULL then use the raid group class control
     * entry.
     */
    if (object_handle == NULL)
    {
        return fbe_mirror_class_control_entry(packet_p);
    }

    switch (opcode)
    {
        case FBE_RAID_GROUP_CONTROL_CODE_MAP_LBA:
            status = fbe_mirror_map_lba(mirror_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_MAP_PBA:
            status = fbe_mirror_map_pba(mirror_p, packet_p);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_EXIT_HIBERNATION:

            raid_geometry_p = fbe_raid_group_get_raid_geometry((fbe_raid_group_t*)mirror_p);
            fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);
            if (raid_type == FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER){
                status = fbe_internal_mirror_exit_hibernation(mirror_p, packet_p);
                if (status != FBE_STATUS_OK){
                    fbe_transport_set_status(packet_p, status, 0);
                    fbe_transport_complete_packet(packet_p);
                    return status;
                }
            } 
            
            status = fbe_raid_group_control_entry(object_handle, packet_p);
            break;

        default:
            status = fbe_raid_group_control_entry(object_handle, packet_p);

            /* If Traverse status is returned and Mirror is the most derived
             * class then we have no derived class to handle Traverse status.
             * Complete the packet with Error.
             */
            if((FBE_STATUS_TRAVERSE == status) &&
               (FBE_CLASS_ID_MIRROR == fbe_raid_group_get_class_id((fbe_raid_group_t*)mirror_p)))
            {
                fbe_base_object_trace((fbe_base_object_t*) mirror_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Can't Traverse for Most derived Mirror class. Opcode: 0x%X\n",
                                      __FUNCTION__, opcode);

                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                status = fbe_transport_complete_packet(packet_p);
            }

            break;
    }

    return status;
}
/* end fbe_mirror_control_entry() */

/*!**************************************************************
 * fbe_mirror_map_lba()
 ****************************************************************
 * @brief
 *  This function maps a lba to pba.
 *
 * @param mirror_p - The raid group.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  5/30/2012 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t
fbe_mirror_map_lba(fbe_mirror_t * mirror_p, fbe_packet_t * packet_p)
{
    fbe_status_t             status;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_raid_group_map_info_t *map_info_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry((fbe_raid_group_t*)mirror_p);
    fbe_chunk_size_t chunk_size;
    fbe_block_edge_t    *block_edge_p = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    chunk_size = fbe_raid_group_get_chunk_size((fbe_raid_group_t*)mirror_p);

    status = fbe_raid_group_usurper_get_control_buffer((fbe_raid_group_t*)mirror_p, packet_p, 
                                                       sizeof(fbe_raid_group_map_info_t),
                                                       (fbe_payload_control_buffer_t *)&map_info_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* We will pick just one edge to get the offset from.
     */
    fbe_raid_geometry_get_block_edge(raid_geometry_p, &block_edge_p, 0);
    map_info_p->offset = fbe_block_transport_edge_get_offset(block_edge_p);

    /* We always use logical parity start + start offset relative to the parity stripe to get 
     * our offset on each drive. 
     * Then we add on the physical offset to get the pba. 
     */
    map_info_p->pba = map_info_p->lba + map_info_p->offset;

    /* Primary and secondary are always 0 and 1.
     */
    map_info_p->data_pos = 0;
    map_info_p->parity_pos = 1;

    /* Take the raid group pba and get the chunk index.
     */
    map_info_p->chunk_index = (fbe_chunk_count_t)(map_info_p->lba / chunk_size);

    fbe_raid_group_get_metadata_lba((fbe_raid_group_t*)mirror_p,
                                    map_info_p->chunk_index,
                                    &map_info_p->metadata_lba);

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_mirror_map_lba()
 **************************************/

/*!**************************************************************
 * fbe_mirror_map_pba()
 ****************************************************************
 * @brief
 *  This function maps a pba to lba.
 *
 * @param mirror_p - The raid group.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/1/2012 - Created. Deanna Heng
 *
 ****************************************************************/
static fbe_status_t
fbe_mirror_map_pba(fbe_mirror_t * mirror_p, fbe_packet_t * packet_p)
{
    fbe_status_t             status;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_raid_group_map_info_t *map_info_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry((fbe_raid_group_t*)mirror_p);
    fbe_chunk_size_t chunk_size;
    fbe_block_edge_t    *block_edge_p = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    chunk_size = fbe_raid_group_get_chunk_size((fbe_raid_group_t*)mirror_p);

    status = fbe_raid_group_usurper_get_control_buffer((fbe_raid_group_t*)mirror_p, packet_p, 
                                                       sizeof(fbe_raid_group_map_info_t),
                                                       (fbe_payload_control_buffer_t *)&map_info_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* We will pick just one edge to get the offset from.
     */
    fbe_raid_geometry_get_block_edge(raid_geometry_p, &block_edge_p, 0);
    map_info_p->offset = fbe_block_transport_edge_get_offset(block_edge_p);

    /* Subtract the physical offset to get the lba. 
     */
    map_info_p->lba = map_info_p->pba - map_info_p->offset;

    /* Primary and secondary are always 0 and 1.
     */
    map_info_p->data_pos = 0;
    map_info_p->parity_pos = 1;

    /* Take the raid group pba and get the chunk index.
     */
    map_info_p->chunk_index = (fbe_chunk_count_t)(map_info_p->lba / chunk_size);

    fbe_raid_group_get_metadata_lba((fbe_raid_group_t*)mirror_p,
                                    map_info_p->chunk_index,
                                    &map_info_p->metadata_lba);

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_mirror_map_pba()
 **************************************/


/*!**************************************************************
 * fbe_internal_mirror_exit_hibernation()
 ****************************************************************
 * @brief
 *  This function make internal mirror under striper exit hibernation.
 *
 * @param mirror_p - The raid group.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/25/2012 - Created. Vera Wang
 *
 ****************************************************************/
static fbe_status_t
fbe_internal_mirror_exit_hibernation(fbe_mirror_t * mirror_p, fbe_packet_t * packet_p)
{
    fbe_status_t                status;
    
    status = fbe_base_config_send_wake_up_usurper_to_servers((fbe_base_config_t*)mirror_p, packet_p);
  
    return status;
}
/**************************************
 * end fbe_internal_mirror_exit_hibernation()
 **************************************/
/******************************
 * end fbe_mirror_usurper.c
 ******************************/
