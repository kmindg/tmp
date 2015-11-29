/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_parity_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the parity object code for the usurper.
 * 
 * @ingroup parity_class_files
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
#include "fbe_parity_private.h"
#include "fbe_raid_group_object.h"

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
static fbe_status_t fbe_parity_map_lba(fbe_parity_t * parity_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_parity_map_pba(fbe_parity_t * parity_p, fbe_packet_t * packet_p);

/*!***************************************************************
 * fbe_parity_control_entry()
 ****************************************************************
 * @brief
 *  This function is the management packet entry point for
 *  operations to the parity object.
 *
 * @param object_handle - The config handle.
 * @param packet_p - The packet that is arriving.
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t 
fbe_parity_control_entry(fbe_object_handle_t object_handle, 
                         fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;
    fbe_payload_control_operation_opcode_t opcode;
    fbe_parity_t *parity_p = NULL;

    parity_p = (fbe_parity_t *)fbe_base_handle_to_pointer(object_handle);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_opcode(control_p, &opcode);

    /* If object handle is NULL then use the raid group class control
     * entry.
     */
    if (object_handle == NULL)
    {
        return fbe_parity_class_control_entry(packet_p);
    }
    switch (opcode)
    {
        case FBE_RAID_GROUP_CONTROL_CODE_MAP_LBA:
            status = fbe_parity_map_lba(parity_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_MAP_PBA:
            status = fbe_parity_map_pba(parity_p, packet_p);
            break;
        default:
            status = fbe_raid_group_control_entry(object_handle, packet_p);

            /* If Traverse status is returned and Parity is the most derived
             * class then we have no derived class to handle Traverse status.
             * Complete the packet with Error.
             */
            if((FBE_STATUS_TRAVERSE == status) &&
               (FBE_CLASS_ID_PARITY == fbe_raid_group_get_class_id((fbe_raid_group_t *) parity_p)))
            {
                fbe_base_object_trace((fbe_base_object_t*) parity_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Can't Traverse for Most derived Parity class. Opcode: 0x%X\n",
                                      __FUNCTION__, opcode);

                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                status = fbe_transport_complete_packet(packet_p);
            }

            break;
    }
    return status;
}
/* end fbe_parity_control_entry() */


/*!**************************************************************
 * fbe_parity_map_lba()
 ****************************************************************
 * @brief
 *  This function maps a lba to pba.
 *
 * @param parity_p - The raid group.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  5/29/2012 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t
fbe_parity_map_lba(fbe_parity_t * parity_p, fbe_packet_t * packet_p)
{
    fbe_status_t             status;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_raid_group_map_info_t *map_info_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry((fbe_raid_group_t*)parity_p);
    fbe_raid_siots_geometry_t geo;
    fbe_u32_t num_parity;
    fbe_u32_t width;
    fbe_chunk_size_t chunk_size;
    fbe_block_edge_t    *block_edge_p = NULL;
    fbe_lba_t pba;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    chunk_size = fbe_raid_group_get_chunk_size((fbe_raid_group_t*)parity_p);

    status = fbe_raid_group_usurper_get_control_buffer((fbe_raid_group_t*)parity_p, packet_p, 
                                                       sizeof(fbe_raid_group_map_info_t),
                                                       (fbe_payload_control_buffer_t *)&map_info_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }
    
    status = fbe_parity_get_lun_geometry(raid_geometry_p, map_info_p->lba, &geo);
    if(status != FBE_STATUS_OK)
    {
         fbe_base_object_trace((fbe_base_object_t *)parity_p,
                               FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s error from get parity lun geometry status: 0x%x\n", __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* We will pick just one edge to get the offset from.
     */
    fbe_raid_geometry_get_block_edge(raid_geometry_p, &block_edge_p, 0);
    map_info_p->offset = fbe_block_transport_edge_get_offset(block_edge_p);

    fbe_raid_geometry_get_num_parity(raid_geometry_p, &num_parity);
    fbe_raid_geometry_get_width(raid_geometry_p, &width);

    /* We always use logical parity start + start offset relative to the parity stripe to get 
     * our offset on each drive. 
     * Then we add on the physical offset to get the pba. 
     */
    pba = geo.logical_parity_start + geo.start_offset_rel_parity_stripe;
    map_info_p->pba = pba + map_info_p->offset;
    map_info_p->data_pos = geo.position[geo.start_index];
    map_info_p->parity_pos = geo.position[width - num_parity];

    /* Take the raid group pba and get the chunk index.
     */
    map_info_p->chunk_index = (fbe_chunk_count_t)(pba / chunk_size);

    fbe_raid_group_get_metadata_lba((fbe_raid_group_t*)parity_p,
                                    map_info_p->chunk_index,
                                    &map_info_p->metadata_lba);

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_map_lba()
 **************************************/

/*!**************************************************************
 * fbe_parity_map_pba()
 ****************************************************************
 * @brief
 *  This function maps a pba to lba.
 *
 * @param parity_p - The raid group.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/1/2012 - Created. Deanna Heng
 *
 ****************************************************************/
static fbe_status_t
fbe_parity_map_pba(fbe_parity_t * parity_p, fbe_packet_t * packet_p)
{
    fbe_status_t             status;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_raid_group_map_info_t *map_info_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry((fbe_raid_group_t*)parity_p);
    fbe_raid_siots_geometry_t geo;
    fbe_u32_t num_parity;
    fbe_u32_t width;
    fbe_chunk_size_t chunk_size;
    fbe_block_edge_t    *block_edge_p = NULL;
    fbe_lba_t pba;
    fbe_lba_t lba;
    fbe_u32_t index;
    fbe_element_size_t sectors_per_element;
    fbe_u32_t blocks_per_stripe;
    fbe_bool_t is_parity = FBE_FALSE;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    chunk_size = fbe_raid_group_get_chunk_size((fbe_raid_group_t*)parity_p);

    status = fbe_raid_group_usurper_get_control_buffer((fbe_raid_group_t*)parity_p, packet_p, 
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

    fbe_raid_geometry_get_num_parity(raid_geometry_p, &num_parity);
    fbe_raid_geometry_get_width(raid_geometry_p, &width);

    /* get the logical pba for the disk
     */
    pba = map_info_p->pba - map_info_p->offset;

    status = fbe_parity_get_physical_geometry(raid_geometry_p, pba, &geo);

    if(status != FBE_STATUS_OK)
    {
         fbe_base_object_trace((fbe_base_object_t *)parity_p,
                               FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s error from get parity lun geometry status: 0x%x\n", __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Convert the pba that got passed in to a logical block address.
     * We convert to the first data position.
     */
    fbe_raid_geometry_get_element_size(raid_geometry_p, &sectors_per_element);
    blocks_per_stripe = sectors_per_element * (width - num_parity);
    lba = (pba / sectors_per_element)
          * blocks_per_stripe
          + (pba % sectors_per_element);

    

    /* adjust for the correct data position and parity position if the 
     * disk position passed in is a parity position 
     */
    for (index = 1; index <= num_parity; index++) {
        if (map_info_p->position == geo.position[width - index]) {
            map_info_p->lba = lba;
            map_info_p->data_pos = geo.position[geo.start_index];
            map_info_p->parity_pos = map_info_p->position;
            is_parity = FBE_TRUE;
            break;
        }
    }

    /* If the disk is a data disk adjust the lba to the correct disk position
     */
    if (!is_parity) {
        map_info_p->data_pos = map_info_p->position;
        map_info_p->parity_pos = geo.position[width - num_parity];
    
        for (index = 0; index < width - num_parity; index++) {
            if (map_info_p->position == geo.position[index]) {
                lba += index * sectors_per_element;
                map_info_p->lba = lba;
                break;
            }
        }
    }

    /* Take the raid group pba and get the chunk index.
     */
    map_info_p->chunk_index = (fbe_chunk_count_t)(pba / chunk_size);

    fbe_raid_group_get_metadata_lba((fbe_raid_group_t*)parity_p,
                                    map_info_p->chunk_index,
                                    &map_info_p->metadata_lba);

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_parity_map_pba()
 **************************************/

/******************************
 * end fbe_parity_usurper.c
 ******************************/
