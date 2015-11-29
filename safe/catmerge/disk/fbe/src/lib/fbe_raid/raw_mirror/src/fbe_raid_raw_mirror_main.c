/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_raid_raw_3way_mirror_main.c
 ***************************************************************************
 *
 * @brief
 *  This file contains code for sending I/O to the raw 3 way mirror area
 *  on the system drives.
 * 
 *  This mirror is used to contain the data for the system objects.
 *  Such as non-paged metadata and database config table entries
 * 
 *  This is needed since the system objects cannot access their data from Luns.
 *  For example, non-paged metadata can not be loaded from the system non-paged
 *  lun since this lun is not initialized at the point where these system objects
 *  need their non-paged metadata.
 *
 * @version
 *   5/25/2011:  Created. Rob Foley
 *   7/26/2011:  Moved from metadata service to raid lib
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_winddk.h"
#include "fbe_service_manager.h"
#include "fbe_raid_geometry.h"
#include "fbe_block_transport.h"
#include "fbe_raid_library.h"
#include "fbe_transport_memory.h"

#define FBE_RAW_MIRROR_ELEMENT_SIZE 128
#define FBE_RAW_MIRROR_MAX_BLOCKS_PER_DRIVE 0x800

#define FBE_RAW_MIRROR_CHUNK_SIZE 0x800
#define FBE_RAW_MAX_CLIENTS 2

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

static fbe_status_t fbe_raw_mirror_get_pvd_edge(fbe_raw_mirror_t *raw_mirror_p,
                                                fbe_packet_t *packet_p,
                                                fbe_u32_t edge_index,
                                                fbe_object_id_t pvd_object_id);

fbe_status_t fbe_raw_mirror_set_initialized(fbe_raw_mirror_t *raw_mirror)
{
    raw_mirror->b_initialized = FBE_TRUE;
    return FBE_STATUS_OK;
}
fbe_bool_t fbe_raw_mirror_is_initialized(fbe_raw_mirror_t *raw_mirror)
{
    return raw_mirror->b_initialized;
}

fbe_status_t fbe_raw_mirror_disable(fbe_raw_mirror_t *raw_mirror_p)
{
    fbe_status_t status;
    fbe_status_t return_status = FBE_STATUS_OK;
    fbe_spinlock_lock(&raw_mirror_p->raw_mirror_lock);
    fbe_raw_mirror_set_flag(raw_mirror_p, FBE_RAW_MIRROR_FLAG_DISABLED);
    if(raw_mirror_p->outstanding_io_count == 0)
    {
        /* nothing to quiesce, continue on */
        fbe_spinlock_unlock(&raw_mirror_p->raw_mirror_lock);
        return FBE_STATUS_OK;
    }
    /* mark quiesce (we don't need additional queisce_flag) */
    fbe_raw_mirror_set_flag(raw_mirror_p, FBE_RAW_MIRROR_FLAG_QUIESCE);
    fbe_spinlock_unlock(&raw_mirror_p->raw_mirror_lock);

    /* We need to switch each of the raw mirrors over to use the raw mirror rg. 
     * This conists of switching over and then waiting for any I/Os down the raw mirror path to finish. 
     */
    status = fbe_raw_mirror_wait_quiesced(raw_mirror_p);
    if (status != FBE_STATUS_OK)
    {
        return_status = status;
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: wait quiesced failed status: 0x%x\n", 
                       __FUNCTION__, status);
    }
    status = fbe_raw_mirror_unset_quiesce_flag(raw_mirror_p);
    if (status != FBE_STATUS_OK)
    {
        return_status = status;
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: unset quiesce flag status: 0x%x\n", 
                       __FUNCTION__, status);
    }
    return return_status;
}
fbe_bool_t fbe_raw_mirror_is_enabled(fbe_raw_mirror_t *raw_mirror_p)
{
    return !fbe_raw_mirror_is_flag_set(raw_mirror_p, FBE_RAW_MIRROR_FLAG_DISABLED);
}

/*!**************************************************************
 * fbe_raw_mirror_init()
 ****************************************************************
 * @brief
 *  Initialize the given raw_mirror structure, which is allocated
 *  and tracked by the client, such as metadata service and database service.
 *
 * @param raw_mirror_p - the struct to init.
 * @param offset - this is the starting lba of the raw mirror on the disk.
 *                 Ask the Database service for this information.
 * @param rg_offset - Offset of this area on it's raid group.
 *                    Allows raid library to generate rg relative lba stamp.
 * @param capacity - this is the capacity of the raw mirror.
 *                  Ask the Database service for this information.
 *
 * @return fbe_status_t
 *
 * @author
 *  5/25/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raw_mirror_init(fbe_raw_mirror_t *raw_mirror_p, 
                                 fbe_lba_t offset, 
                                 fbe_lba_t rg_offset, 
                                 fbe_block_count_t capacity)
{
    fbe_status_t status;
    fbe_raid_geometry_t *raid_geometry_p = &raw_mirror_p->geo;

    FBE_ASSERT_AT_COMPILE_TIME(sizeof(fbe_raw_mirror_sector_data_t) <= FBE_BYTES_PER_BLOCK);

    fbe_zero_memory(raw_mirror_p, sizeof(fbe_raw_mirror_t));
	/*initialize the spinlock and semaphore*/
    fbe_spinlock_init(&raw_mirror_p->raw_mirror_lock);
	fbe_semaphore_init(&raw_mirror_p->quiesce_io_semaphore,0,1);
    fbe_raid_geometry_init(raid_geometry_p,
                           0xFFFFFFFF, /* Object id to use. */
                           FBE_CLASS_ID_MIRROR,
                           NULL, /* No metadata element. */
                           (fbe_block_edge_t*)&raw_mirror_p->edge[0],
                           (fbe_raid_common_state_t)fbe_mirror_generate_start);

    /* Now configure the raid group geometry.
     */
    status = fbe_raid_geometry_set_configuration(raid_geometry_p,
                                                 FBE_RAW_MIRROR_WIDTH,
                                                 FBE_RAID_GROUP_TYPE_RAW_MIRROR,
                                                 FBE_RAW_MIRROR_ELEMENT_SIZE,
                                                 0, /* No elements per parity. */
                                                 capacity,
                                                 FBE_RAW_MIRROR_MAX_BLOCKS_PER_DRIVE);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID,FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: error initializing raw mirror config status: 0x%x\n", 
                       __FUNCTION__, status);
    }

    fbe_raid_geometry_set_flag(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_RAW_MIRROR);

    /* This flag indicates we are not an object and certain functionality will be provided by 
     * the raid library.
     */
    fbe_raid_geometry_set_flag(raid_geometry_p, FBE_RAID_GEOMETRY_NO_OBJECT);

    /*! @note When/if we support something other than 520 on the boot drives, this will need to change.
     */
    status = fbe_raid_geometry_set_block_sizes(raid_geometry_p,
                                               FBE_BE_BYTES_PER_BLOCK,
                                               FBE_BE_BYTES_PER_BLOCK,    /* Physical block size */
                                               1);
    fbe_raid_geometry_set_raw_mirror_offset(raid_geometry_p, offset);
    fbe_raid_geometry_set_raw_mirror_rg_offset(raid_geometry_p, rg_offset);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raw_mirror_init()
 ******************************************/

/*!**************************************************************
 * fbe_raw_mirror_init_edges()
 ****************************************************************
 * @brief
 *  
 * @param raw_mirror_p - the struct to that stores all raw mirror related stuff.
 * @param packet_p - packet to use to init edge info.
 * @param edge_index - Index of edge in raw mirror.
 *
 * @return fbe_status_t
 *
 * @author
 *  5/26/2011 - Created. Rob Foley
 *  3/12/2012 - Modified. Jingcheng Zhang. fix DE505
 ****************************************************************/
fbe_status_t fbe_raw_mirror_init_edges(fbe_raw_mirror_t *raw_mirror_p)
{
    fbe_status_t status;
    fbe_object_id_t pvd_object_id;
	fbe_u32_t edge_index;
    fbe_packet_t *packet_p = NULL;
    fbe_u32_t retry_count = 0;

    packet_p = fbe_transport_allocate_packet();
    if (!packet_p) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet(packet_p);
    do 
    {
        for(edge_index = 0; edge_index < FBE_RAW_MIRROR_WIDTH; edge_index ++){
            status = fbe_database_get_system_pvd(edge_index, &pvd_object_id);
            if (status != FBE_STATUS_OK) {
                fbe_base_library_trace(FBE_LIBRARY_ID_RAID, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: error getting system pvd 0x%x\n", 
                                    __FUNCTION__, status);
                break;
            }

            status = fbe_raw_mirror_get_pvd_edge(raw_mirror_p, packet_p, edge_index, pvd_object_id);
            if (status != FBE_STATUS_OK) {
                fbe_base_library_trace(FBE_LIBRARY_ID_RAID, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s: error getting system pvd %d, status 0x%x, retry count %d\n", 
                                    __FUNCTION__, pvd_object_id, status, retry_count);
                break;
            }
        }
        /* only retry if PVD is not created */
        if (status == FBE_STATUS_NO_OBJECT)
        {
            fbe_thread_delay(100);
            retry_count ++;
        }
    }
    while ((status == FBE_STATUS_NO_OBJECT)&&(retry_count < 600));  /* wait for sep initializing system PVDs */

    if (status==FBE_STATUS_OK)
    {
        /* fresh the 4k bitmask */
        fbe_raid_geometry_refresh_block_sizes(&raw_mirror_p->geo);

        fbe_raw_mirror_set_initialized(raw_mirror_p);
    }
    else 
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                            "raw mirror init failed, status %d\n", 
                            status);
    }
    fbe_transport_release_packet(packet_p);

    return status;
}
/******************************************
 * end fbe_raw_mirror_init_edges()
 ******************************************/

/*!**************************************************************
 * fbe_raw_mirror_get_pvd_edge()
 ****************************************************************
 * @brief
 * 
 * @param raw_mirror_p - the struct to that stores all raw mirror related stuff.
 * @param packet_p - Packet to use to get edge.
 * @param edge_index - This is the edge index we just fetched for.
 * @param pvd_object_id - This is the id of the pvd to get the edge for.   
 *
 * @return fbe_status_t
 *
 * @author
 *  5/26/2011 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_raw_mirror_get_pvd_edge(fbe_raw_mirror_t *raw_mirror_p,
                                                fbe_packet_t *packet_p,
                                                fbe_u32_t edge_index,
                                                fbe_object_id_t pvd_object_id)
{
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;
    fbe_status_t packet_status;
    fbe_payload_control_status_t control_status;

    raw_mirror_p->edge_info.base_edge_info.client_index = 0;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);

    fbe_payload_control_build_operation(control_p,
                                        FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO,
                                        &raw_mirror_p->edge_info, 
                                        sizeof(fbe_block_transport_control_get_edge_info_t));
    /* Set packet address. */
    fbe_transport_set_address(packet_p, FBE_PACKAGE_ID_SEP_0, FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID, pvd_object_id);

	fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    fbe_payload_ex_increment_control_operation_level(sep_payload_p);
    fbe_service_manager_send_control_packet(packet_p);

	fbe_transport_wait_completion(packet_p);

    packet_status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_p, &control_status);
    fbe_payload_ex_release_control_operation(sep_payload_p, control_p);
    if ((packet_status != FBE_STATUS_OK) || (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID,FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s get edge failed for pvd index %d pkt_st: 0x%x cont_st: 0x%x\n", 
                       __FUNCTION__, edge_index, packet_status, control_status);
        if (packet_status == FBE_STATUS_OK)
        {
            packet_status = FBE_STATUS_GENERIC_FAILURE;
            fbe_transport_set_status(packet_p, packet_status, 0);
        }
        return packet_status;
    }

    /* Save the edge we just received. */
    raw_mirror_p->edge[edge_index] = (fbe_block_edge_t*)(fbe_ptrhld_t)raw_mirror_p->edge_info.edge_p;
    if (raw_mirror_p->edge_info.edge_p == 0) {
        /* what if LDO does not exists? */
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID,FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s edge null for pvd index %d pkt_st: 0x%x cont_st: 0x%x\n", 
                       __FUNCTION__, edge_index, packet_status, control_status);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raw_mirror_get_pvd_edge()
 ******************************************/

/*!**************************************************************
 * fbe_raw_mirror_get_degraded_bitmask()
 ****************************************************************
 * @brief
 *  Collect the degraded information from the edges.
 * 
 * @param  bitmask_p - Returned bitmask of degraded positions.
 *
 * @return fbe_status_t
 *
 * @author
 *  5/25/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raw_mirror_get_degraded_bitmask(fbe_raw_mirror_t *raw_mirror_p,fbe_raid_position_bitmask_t *bitmask_p)
{
    fbe_u32_t edge_index;
    fbe_path_state_t path_state;
    fbe_block_edge_t *block_edge_p = NULL;

    /* Loop over all the edges and construct the bitmask of positions that are degraded.
     */ 
    *bitmask_p = 0;
    for ( edge_index = 0; edge_index < FBE_RAW_MIRROR_WIDTH; edge_index++)
    {
        block_edge_p = raw_mirror_p->edge[edge_index];
        fbe_block_transport_get_path_state(block_edge_p, &path_state);
        if (path_state != FBE_PATH_STATE_ENABLED)
        {
            *bitmask_p |= (1 << edge_index);
        }
    }
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raw_mirror_get_degraded_bitmask()
 **************************************/
/*!**************************************************************
 * fbe_raw_mirror_raid_callback()
 ****************************************************************
 * @brief
 *  Handle an iots completion from the raid library.
 *
 * @param packet_p - Packet that is completing.
 * @param iots_p - The iots that is completing.  This has
 *                 Status in it to tell us if it is done.
 *
 * @return fbe_status_t.
 *
 * @author
 *  5/25/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raw_mirror_raid_callback(void * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status;
    fbe_raid_iots_t *iots_p = (fbe_raid_iots_t *)context;
    /* We are done.  Set the status from the iots in the packet.
     */
    status = fbe_raid_iots_set_packet_status(iots_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID,FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: error setting packet status: 0x%x\n", 
                              __FUNCTION__, status);
    }

    /* Destroy the iots now that it is no longer on a queue.
     */
    status = fbe_raid_iots_destroy(iots_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID,FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: error destroying iots status: 0x%x\n", 
                              __FUNCTION__, status);
    }

    status = fbe_transport_complete_packet(packet_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID,FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: error completing packet status: 0x%x\n", 
                              __FUNCTION__, status);
    }
    return status;
}
/**************************************
 * end fbe_raw_mirror_raid_callback
 **************************************/

/*!**************************************************************
 * fbe_raw_mirror_send_io_packet_to_raid_library()
 ****************************************************************
 * @brief
 *  Send the io packet to the raw mirror that the raid library
 *  knows how to access.
 * @param  packet_p   
 *
 * @return fbe_status_t
 *
 * @author
 *  5/25/2011 - Created. Rob Foley
 *  3/12/2012 - Modified. Jingcheng Zhang. fix DE505
 ****************************************************************/

fbe_status_t fbe_raw_mirror_send_io_packet_to_raid_library(fbe_raw_mirror_t *raw_mirror_p,
                                                           fbe_packet_t *packet_p)
{
    fbe_status_t        status;
    fbe_raid_iots_t     *iots_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_lba_t lba;
    fbe_block_count_t blocks;
    fbe_raid_position_bitmask_t rl_bitmask;

    if (!fbe_raw_mirror_is_initialized(raw_mirror_p))
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet_p);		
        return FBE_STATUS_GENERIC_FAILURE;
    }

	/* If we are here - we are initialized */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);

    if (block_operation_p == NULL)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID,FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s block operation is null.\n", __FUNCTION__);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_PENDING;
    }

    /* Set the memory resource priority.  This code comes from fbe_raid_common_set_resource_priority().
     * Although a raw mirror is not an object, we are treating it as such from a memory resource perspective.
     */
    if (packet_p->resource_priority >= FBE_MEMORY_DPS_RAID_BASE_PRIORITY)
    {
        /* Generally in IO path packets flow down the stack from LUN to RG to VD to PVD.
         * In this case, resource priority will be less than Objects base priority and so
         * we bump it up. 
         * But in some cases (such as monitor, control, event operations, etc.) packets can some 
         * time move up the stack. In these cases packets's resource priority will already be greater
         * than Object's base priority. But since we should never reduce priority, we dont
         * update it.    
         */
    }
    else
    {
        packet_p->resource_priority = FBE_MEMORY_DPS_RAID_BASE_PRIORITY;
    }

    /* Initialize and kick off iots.
     * The iots is part of the payload. 
     */
    fbe_payload_ex_get_iots(payload_p, &iots_p);

    fbe_payload_block_get_lba(block_operation_p, &lba);
    fbe_payload_block_get_block_count(block_operation_p, &blocks);

    status = fbe_raid_iots_init(iots_p, packet_p, block_operation_p, 
                                &raw_mirror_p->geo, 
                                lba, blocks);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID,FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s init iots failed - status: 0x%x\n", 
                               __FUNCTION__, status);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_PENDING;
    }

    fbe_raid_iots_set_chunk_size(iots_p, FBE_RAW_MIRROR_CHUNK_SIZE);
    fbe_raid_iots_set_callback(iots_p, fbe_raw_mirror_raid_callback, iots_p);

    /* If we are degraded, let the library know by setting the rl bitmask.
     */
    fbe_raw_mirror_get_degraded_bitmask(raw_mirror_p, &rl_bitmask);
    fbe_raid_iots_set_rebuild_logging_bitmask(iots_p, rl_bitmask);

    fbe_base_library_trace(FBE_LIBRARY_ID_RAID, FBE_TRACE_LEVEL_DEBUG_HIGH, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: rl_bitmask: 0x%x\n", 
                          __FUNCTION__, rl_bitmask);

    /* fresh the 4k bitmask */
    fbe_raid_geometry_refresh_block_sizes(&raw_mirror_p->geo);

    /* Fill out the chunk info.  It is always blank since the metadata service does not have
     * any paged bitmap. 
     */
    fbe_set_memory(&iots_p->chunk_info[0], 0, FBE_RAID_IOTS_MAX_CHUNKS * sizeof(fbe_raid_group_paged_metadata_t));

    fbe_raid_iots_start(iots_p);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raw_mirror_send_io_packet_to_raid_library()
 ******************************************/
 
/*!**************************************************************
 * fbe_raw_mirror_mask_down_disk()
 ****************************************************************
 * @brief
 *  mask the down disk  in raw mirror down disk bitmask. 
 * 
 * @param  fbe_raw_mirror_t * raw_mirror_p
 *                 fbe_u32_t disk_index  - the down disk index
 *
 * @return fbe_status_t
 *
 * @author
 *  3/5/2012 - Created. zhangy
 *
 ****************************************************************/
fbe_status_t fbe_raw_mirror_mask_down_disk(fbe_raw_mirror_t *raw_mirror_p, fbe_u32_t disk_index)
{
    fbe_status_t status = FBE_STATUS_OK;
	if(raw_mirror_p == NULL){
      status = FBE_STATUS_GENERIC_FAILURE;
	}else{
	  raw_mirror_p->down_disk_bitmask |= (1 << disk_index);
	}
	return status;
}
/*!**************************************************************
 * fbe_raw_mirror_unmask_down_disk()
 ****************************************************************
 * @brief
 *  unmask the down disk  in raw mirror down disk bitmask. 
 * 
 * @param  fbe_raw_mirror_t * raw_mirror_p
 *                 fbe_u32_t disk_index  - the  disk index
 *
 * @return fbe_status_t
 *
 * @author
 *  3/5/2012 - Created. zhangy
 *
 ****************************************************************/
fbe_status_t fbe_raw_mirror_unmask_down_disk(fbe_raw_mirror_t *raw_mirror_p, fbe_u32_t disk_index)
{
    fbe_status_t status = FBE_STATUS_OK;
	if(raw_mirror_p == NULL){
      status = FBE_STATUS_GENERIC_FAILURE;
	}else{
	  raw_mirror_p->down_disk_bitmask &= ~(1 << disk_index);
	}
	return status;
}
/*!**************************************************************
 * fbe_raw_mirror_get_down_disk_bitmask()
 ****************************************************************
 * @brief
 *  get the down disk bitmask in raw mirror. 
 * 
 * @param  IN: fbe_raw_mirror_t * raw_mirror_p
 *                 OUT: fbe_u16_t* down_disk_bitmask
 *
 * @return fbe_status_t
 *
 * @author
 *  3/5/2012 - Created. zhangy
 *
 ****************************************************************/
fbe_status_t fbe_raw_mirror_get_down_disk_bitmask(fbe_raw_mirror_t *raw_mirror_p, fbe_u16_t* down_disk_bitmask)
{
    fbe_status_t status = FBE_STATUS_OK;
	*down_disk_bitmask = 0;
	if(raw_mirror_p == NULL){
      status = FBE_STATUS_GENERIC_FAILURE;
	}else{
	  *down_disk_bitmask = raw_mirror_p->down_disk_bitmask;
	}
	return status;
}

/*!**************************************************************
 * fbe_raw_mirror_unset_initialized()
 ****************************************************************
 * @brief
 * 
 * @param  IN: fbe_raw_mirror_t * raw_mirror_p   
 *
 * @return fbe_status_t
 *
 * @author
 *  3/5/2012 - Created. zhangy
 *
 ****************************************************************/
fbe_status_t fbe_raw_mirror_unset_initialized(fbe_raw_mirror_t *raw_mirror_p)
{
    fbe_status_t status = FBE_STATUS_OK;
	if(raw_mirror_p == NULL){
      status = FBE_STATUS_GENERIC_FAILURE;
	}else{
	  raw_mirror_p->b_initialized = FBE_FALSE;
	}
	return status;
}
/*!**************************************************************
 * fbe_raw_mirror_send_io_ex_packet_to_raid_library()
 ****************************************************************
 * @brief
 *  Send the io packet to the raw mirror that the raid library
 *  knows how to access.
 * @param  packet_p 
 *                 in_disk_bitmask - specify which disks we want to read/write
 * @return fbe_status_t
 *
 * @author
 *  3/5/2012 - Created. 
 *
 ****************************************************************/

fbe_status_t fbe_raw_mirror_ex_send_io_packet_to_raid_library(fbe_raw_mirror_t *raw_mirror_p, fbe_packet_t *packet_p, fbe_u16_t in_disk_bitmask)
{
    fbe_status_t        status;
	fbe_raid_iots_t 	*iots_p = NULL;
	fbe_payload_ex_t *payload_p = NULL;
	fbe_payload_block_operation_t *block_operation_p = NULL;
	fbe_lba_t lba;
	fbe_block_count_t blocks;
	fbe_raid_position_bitmask_t rl_bitmask;
	fbe_raid_position_bitmask_t user_bitmask = 0;
	
	if (!fbe_raw_mirror_is_initialized(raw_mirror_p))
	  {
		  fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
		  fbe_transport_complete_packet(packet_p);		  
		  return FBE_STATUS_GENERIC_FAILURE;
	  }
	
	/* If we are here - we are initialized */
	payload_p = fbe_transport_get_payload_ex(packet_p);
	block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
	
	if (block_operation_p == NULL)
	{
		fbe_base_library_trace(FBE_LIBRARY_ID_RAID,FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
						   "%s block operation is null.\n", __FUNCTION__);
		fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet_p);
		return FBE_STATUS_PENDING;
	}
	
	/* Set the memory resource priority.  This code comes from fbe_raid_common_set_resource_priority().
	* Although a raw mirror is not an object, we are treating it as such from a memory resource perspective.
	 */
	if (packet_p->resource_priority >= FBE_MEMORY_DPS_RAID_BASE_PRIORITY)
	{
		/* Generally in IO path packets flow down the stack from LUN to RG to VD to PVD.
		 * In this case, resource priority will be less than Objects base priority and so
		* we bump it up. 
		* But in some cases (such as monitor, control, event operations, etc.) packets can some 
		* time move up the stack. In these cases packets's resource priority will already be greater
		* than Object's base priority. But since we should never reduce priority, we dont
		 * update it.	 
		*/
	}
	else
	{
		packet_p->resource_priority = FBE_MEMORY_DPS_RAID_BASE_PRIORITY;
	}
	
	/*increase the outstanding IO count*/
	fbe_spinlock_lock(&raw_mirror_p->raw_mirror_lock);
    if (fbe_raw_mirror_is_flag_set(raw_mirror_p, FBE_RAW_MIRROR_FLAG_QUIESCE))
    {
        fbe_spinlock_unlock(&raw_mirror_p->raw_mirror_lock);
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID,FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s raw_mirror quiesce IO.\n", __FUNCTION__);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);          
        return FBE_STATUS_GENERIC_FAILURE;
    }
	
	raw_mirror_p->outstanding_io_count ++;
	fbe_spinlock_unlock(&raw_mirror_p->raw_mirror_lock);
    fbe_transport_set_completion_function(packet_p,fbe_raw_mirror_send_io_complete_function,raw_mirror_p);
	/* Initialize and kick off iots.
         * The iots is part of the payload. 
	*/
	fbe_payload_ex_get_iots(payload_p, &iots_p);
	
	fbe_payload_block_get_lba(block_operation_p, &lba);
	fbe_payload_block_get_block_count(block_operation_p, &blocks);
	
	status = fbe_raid_iots_init(iots_p, packet_p, block_operation_p, 
                                &raw_mirror_p->geo, 
                                lba, blocks);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID,FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s init iots failed - status: 0x%x\n", 
                               __FUNCTION__, status);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_PENDING;
    }

	fbe_raid_iots_set_chunk_size(iots_p, FBE_RAW_MIRROR_CHUNK_SIZE);
	
	fbe_raid_iots_set_callback(iots_p, fbe_raw_mirror_raid_callback, iots_p);
	
	/* If we are degraded, let the library know by setting the rl bitmask.
       */
	fbe_raw_mirror_get_degraded_bitmask(raw_mirror_p, &rl_bitmask);
	/*or the degraded_bitmask,the down disk bitmask and the in_disk_mask
	in_disk_bitmask indicates which disks the user what to send io to
	down_disk_mask indicates which disks are not accesssable due to destroying system pvd*/
    user_bitmask = (~in_disk_bitmask) & ((1 << FBE_RAW_MIRROR_WIDTH) - 1);
	rl_bitmask |= ( raw_mirror_p->down_disk_bitmask | user_bitmask);
	fbe_raid_iots_set_rebuild_logging_bitmask(iots_p, rl_bitmask);
	
	fbe_base_library_trace(FBE_LIBRARY_ID_RAID, FBE_TRACE_LEVEL_DEBUG_HIGH, 
							   FBE_TRACE_MESSAGE_ID_INFO,
							  "%s: rl_bitmask: 0x%x\n", 
							  __FUNCTION__, rl_bitmask);

    /* If we have no positions to do I/O to, just fail it now. 
     * The raid library never expects to see an rl bitmask set for all positions. 
     * Normally the object handles that, but since we don't have an object we need to 
     * handle this case here. 
     */
    if ( ((1 << FBE_RAW_MIRROR_WIDTH) - 1) == rl_bitmask)
    {
        fbe_base_library_trace(FBE_LIBRARY_ID_RAID, FBE_TRACE_LEVEL_WARNING, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: all rl set mirror is shutdown, rl_bitmask: 0x%x\n", 
                               __FUNCTION__, rl_bitmask);
        fbe_payload_block_set_status(block_operation_p, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_RETRY_NOT_POSSIBLE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);          
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* fresh the 4k bitmask */
    fbe_raid_geometry_refresh_block_sizes(&raw_mirror_p->geo);

	/* Fill out the chunk info.  It is always blank since the metadata service does not have
         * any paged bitmap. 
	*/
	fbe_set_memory(&iots_p->chunk_info[0], 0, FBE_RAID_IOTS_MAX_CHUNKS * sizeof(fbe_raid_group_paged_metadata_t));
	fbe_raid_iots_start(iots_p);
	return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raw_mirror_send_io_ex_packet_to_raid_library()
 **************************************/
/*!**************************************************************
 * fbe_raw_mirror_set_quiesce_flag()
 ****************************************************************
 * @brief
 * 
 * @param  IN: fbe_raw_mirror_t * raw_mirror_p   
 *
 * @return fbe_status_t
 *
 * @author
 *  3/5/2012 - Created. zhangy
 *
 ****************************************************************/
fbe_status_t fbe_raw_mirror_set_quiesce_flag(fbe_raw_mirror_t *raw_mirror_p)
{
    fbe_status_t status = FBE_STATUS_OK;
	if(raw_mirror_p == NULL){
      status = FBE_STATUS_GENERIC_FAILURE;
	}else{
	  fbe_spinlock_lock(&raw_mirror_p->raw_mirror_lock);
	  fbe_raw_mirror_set_flag(raw_mirror_p, FBE_RAW_MIRROR_FLAG_QUIESCE);
	  if(raw_mirror_p->outstanding_io_count == 0)
	  {
        fbe_semaphore_release(&raw_mirror_p->quiesce_io_semaphore,0,1,FALSE);
	  }
	  fbe_spinlock_unlock(&raw_mirror_p->raw_mirror_lock);
	}
	return status;
}
/*!**************************************************************
 * fbe_raw_mirror_unset_quiesce_flag()
 ****************************************************************
 * @brief
 * 
 * @param  IN: fbe_raw_mirror_t * raw_mirror_p   
 *
 * @return fbe_status_t
 *
 * @author
 *  3/5/2012 - Created. zhangy
 *
 ****************************************************************/
fbe_status_t fbe_raw_mirror_unset_quiesce_flag(fbe_raw_mirror_t *raw_mirror_p)
{
    fbe_status_t status = FBE_STATUS_OK;
	if(raw_mirror_p == NULL){
      status = FBE_STATUS_GENERIC_FAILURE;
	}else{
		fbe_spinlock_lock(&raw_mirror_p->raw_mirror_lock);
		fbe_raw_mirror_clear_flag(raw_mirror_p, FBE_RAW_MIRROR_FLAG_QUIESCE);
		fbe_spinlock_unlock(&raw_mirror_p->raw_mirror_lock);
	}
	return status;
}
/*!**************************************************************
 * fbe_raw_mirror_send_io_complete_function()
 ****************************************************************
 * @brief
 * 
 * @param  IN: fbe_raw_mirror_t * raw_mirror_p   
 *
 * @return fbe_status_t
 *
 * @author
 *  3/5/2012 - Created. zhangy
 *
 ****************************************************************/
fbe_status_t fbe_raw_mirror_send_io_complete_function(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t status = FBE_STATUS_OK;
	fbe_raw_mirror_t * raw_mirror_p = (fbe_raw_mirror_t*)context;
	if(raw_mirror_p == NULL){
        status = FBE_STATUS_GENERIC_FAILURE;
	}else{
        fbe_spinlock_lock(&raw_mirror_p->raw_mirror_lock);
	    raw_mirror_p->outstanding_io_count--;
	    if((raw_mirror_p->outstanding_io_count== 0) && 
	       (fbe_raw_mirror_is_flag_set(raw_mirror_p, FBE_RAW_MIRROR_FLAG_QUIESCE)))
	    {
	       fbe_semaphore_release(&raw_mirror_p->quiesce_io_semaphore,0,1,FALSE);
	    }
		fbe_spinlock_unlock(&raw_mirror_p->raw_mirror_lock);
	}
	return status;
}

/*!**************************************************************
 * fbe_raw_mirror_destroy()
 ****************************************************************
 * @brief
 * 
 * @param  IN: fbe_raw_mirror_t * raw_mirror_p   
 *
 * @return fbe_status_t
 *
 * @author
 *  3/5/2012 - Created. zhangy
 *
 ****************************************************************/
fbe_status_t fbe_raw_mirror_destroy(fbe_raw_mirror_t *raw_mirror_p)
{
    fbe_status_t status = FBE_STATUS_OK;
	if(raw_mirror_p == NULL){
      status = FBE_STATUS_GENERIC_FAILURE;
	}else{
	  fbe_raw_mirror_unset_initialized(raw_mirror_p);
	  fbe_semaphore_destroy(&raw_mirror_p->quiesce_io_semaphore);
	  fbe_spinlock_destroy(&raw_mirror_p->raw_mirror_lock);
	}
	return status;
}



/*************************
 * end file fbe_raw_3way_mirror_main.c
 *************************/
