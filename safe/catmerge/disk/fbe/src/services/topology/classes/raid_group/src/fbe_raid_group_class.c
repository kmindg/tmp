/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_raid_group_class.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the raid group class code.
 * 
 * @ingroup fbe_raid_group_class_files
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
#include "fbe_raid_group_object.h"
#include "fbe/fbe_striper.h"
#include "fbe/fbe_mirror.h"
#include "fbe/fbe_parity.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe_database.h"
#include "fbe/fbe_registry.h"
#include "fbe_block_transport.h"
#include "fbe_private_space_layout.h"
#include "fbe/fbe_event_log_api.h"
#include "fbe/fbe_event_log_utils.h"
#include "fbe_raid_group_rebuild.h"



/*****************************************
 * LOCAL GLOBALS
 *****************************************/
static fbe_block_count_t    fbe_raid_group_blocks_per_drive_request = 0;
static fbe_u32_t            fbe_raid_group_update_peer_checkpoint_period_secs = FBE_RAID_GROUP_PEER_CHECKPOINT_UPDATE_SECS;

/* These are the raid group defaults for throttling. 
 * Throttling can be disabled and the default throttling can be set for either user or system raid groups. 
 */
static fbe_bool_t fbe_raid_group_throttle_enabled = FBE_TRUE;
static fbe_u32_t fbe_raid_group_user_queue_depth = FBE_RAID_GROUP_DEFAULT_QUEUE_DEPTH;
static fbe_u32_t fbe_raid_group_system_queue_depth = FBE_RAID_GROUP_MIN_QUEUE_DEPTH;

/* These override the default RG debug flags if a raid group is a user or system raid group.
 */
static fbe_raid_group_debug_flags_t fbe_raid_group_default_user_debug_flags = FBE_RAID_GROUP_DEBUG_FLAG_NONE;
static fbe_raid_group_debug_flags_t fbe_raid_group_default_system_debug_flags = FBE_RAID_GROUP_DEBUG_FLAG_NONE;

/* Extended Media Error Handling (EMEH) for raid group class.
 */
static fbe_u32_t fbe_raid_group_extended_media_error_handling_params = FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_DEFAULT;

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
static fbe_status_t fbe_raid_group_class_get_raid_library_debug_flags(fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_class_get_raid_group_debug_flags(fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_class_set_raid_library_debug_flags(fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_class_set_raid_group_debug_flags(fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_class_set_raid_library_error_testing(fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_class_get_raid_library_error_testing_stats(fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_class_reset_raid_library_error_testing_stats(fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_class_set_options(fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_class_get_raid_memory_stats(fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_class_reset_raid_memory_stats(fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_class_set_random_library_errors(fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_class_set_bg_op_speed(fbe_packet_t * packet);
static fbe_status_t fbe_raid_group_class_get_bg_op_speed(fbe_packet_t * packet);
static fbe_status_t fbe_raid_group_class_set_update_peer_checkpoint_interval(fbe_packet_t * packet);
static fbe_status_t fbe_raid_group_set_default_bts_params(fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_get_default_bts_params(fbe_packet_t * packet_p);
static void fbe_raid_group_class_persist_block_transport_parameters(void);
static fbe_status_t fbe_raid_group_set_default_debug_flags(fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_set_default_library_flags(fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_get_default_library_flags(fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_get_default_debug_flags(fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_class_get_extended_media_error_handling(fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_class_set_extended_media_error_handling(fbe_packet_t * packet_p);
static void fbe_raid_group_class_set_extended_media_error_handling_params(fbe_u32_t emeh_params);
static fbe_status_t fbe_raid_group_class_set_chunks_per_rebuild(fbe_packet_t * packet_p);

/*!***************************************************************
 * fbe_raid_group_class_is_max_drive_blocks_configured()
 ****************************************************************
 * @brief
 *  Simply determines if the maximum number of blocks that raid
 *  can send to a drive has been configured or not
 *
 * @param None
 *
 * @return bool - FBE_TRUE - We have configured the maximum number
 *                blocks we can send to a drive
 ****************************************************************/
static fbe_bool_t fbe_raid_group_class_is_max_drive_blocks_configured(void)
{
    if (fbe_raid_group_blocks_per_drive_request != 0)
    {
        return FBE_TRUE;
    }

    return FBE_FALSE;
}

/*!***************************************************************
 * fbe_raid_group_class_get_max_drive_blocks()
 ****************************************************************
 * @brief
 *  Simply return the maximum number of blocks that raid
 *  can send to a drive.
 *
 * @param None
 *
 * @return block count - The maximum number of block that raid can
 *          send to a drive in a single request.
 ****************************************************************/
static fbe_block_count_t fbe_raid_group_class_get_max_drive_blocks(void)
{
    return(fbe_raid_group_blocks_per_drive_request);
}

/*!***************************************************************
 * fbe_raid_group_class_set_max_drive_blocks()
 ****************************************************************
 * @brief
 *  Simply set the maximum number of blocks that raid can send to a drive.
 *
 * @param max_blocks_per_drive - The configured maximum number of
 *          blocks that raid should send to a drive. 
 *
 * @return none
 ****************************************************************/
static void fbe_raid_group_class_set_max_drive_blocks(fbe_block_count_t max_blocks_per_drive)
{
    fbe_raid_group_blocks_per_drive_request = max_blocks_per_drive;
    return;
}

static fbe_u32_t fbe_raid_group_class_get_user_queue_depth(void)
{
    return(fbe_raid_group_user_queue_depth);
}
static void fbe_raid_group_class_set_user_queue_depth(fbe_u32_t queue_depth)
{
    fbe_raid_group_user_queue_depth = queue_depth;
    return;
}
static fbe_u32_t fbe_raid_group_class_get_system_queue_depth(void)
{
    return(fbe_raid_group_system_queue_depth);
}

static void fbe_raid_group_class_set_system_queue_depth(fbe_u32_t queue_depth)
{
    fbe_raid_group_system_queue_depth = queue_depth;
    return;
}
fbe_bool_t fbe_raid_group_class_get_throttle_enabled(void)
{
    return(fbe_raid_group_throttle_enabled);
}
static void fbe_raid_group_class_set_throttle_enabled(fbe_bool_t enabled)
{
    fbe_raid_group_throttle_enabled = enabled;
    return;
}
/*!***************************************************************
 * fbe_raid_group_class_set_peer_update_checkpoint_interval_ms()
 ****************************************************************
 * @brief
 *  Sets the period (in milliseconds) between updates to the peer when 
 *  the checkpoint is updated locally.
 *
 * @param   peer_update_period - Peer update period in seconds
 *
 * @return status
 ****************************************************************/
static fbe_status_t fbe_raid_group_class_set_peer_update_checkpoint_interval_ms(fbe_u32_t peer_update_period_ms)
{
    if (peer_update_period_ms < 1000)
    {
        fbe_raid_group_update_peer_checkpoint_period_secs = 1;
    }
    else
    {
        fbe_raid_group_update_peer_checkpoint_period_secs = peer_update_period_ms / 1000;
    }

    return FBE_STATUS_OK;
}
/*******************************************************************
 * end fbe_raid_group_class_set_peer_update_checkpoint_interval_ms()
 *******************************************************************/

/*!***************************************************************
 * fbe_raid_group_class_get_peer_update_checkpoint_interval_ms()
 ****************************************************************
 * @brief
 *  Gets the period (in milliseconds) between updates to the peer when 
 *  the checkpoint is updated locally.
 *
 * @param   peer_update_period_p - Pointer to Peer update period in seconds
 *
 * @return status
 ****************************************************************/
fbe_status_t fbe_raid_group_class_get_peer_update_checkpoint_interval_ms(fbe_u32_t *peer_update_period_ms_p)
{
    *peer_update_period_ms_p = fbe_raid_group_update_peer_checkpoint_period_secs * 1000;

    return FBE_STATUS_OK;
}
/*******************************************************************
 * end fbe_raid_group_class_get_peer_update_checkpoint_interval_ms()
 ******************************************************************/

/*!***************************************************************
 * fbe_raid_group_class_control_entry()
 ****************************************************************
 * @brief
 *  This function is the management packet entry point for
 *  operations to the raid group class.
 *
 * @param packet - The packet that is arriving.
 *
 * @return fbe_status_t - Return status of the operation.
 * 
 * @author
 *  2/24/2010 - Created. Rob Foley
 ****************************************************************/
fbe_status_t 
fbe_raid_group_class_control_entry(fbe_packet_t * packet)
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
            status = fbe_raid_group_class_get_info(packet);
            break;

        case FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_RAID_LIBRARY_DEBUG_FLAGS:
            status = fbe_raid_group_class_get_raid_library_debug_flags(packet);
            break;        
        
        case FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_RAID_LIBRARY_DEBUG_FLAGS:
            status = fbe_raid_group_class_set_raid_library_debug_flags(packet);
            break;

        case FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_RAID_GROUP_DEBUG_FLAGS:
            status = fbe_raid_group_class_get_raid_group_debug_flags(packet);
            break;

        case FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_RAID_GROUP_DEBUG_FLAGS:
            status = fbe_raid_group_class_set_raid_group_debug_flags(packet);
            break;

        case FBE_RAID_GROUP_CONTROL_CODE_SET_DEFAULT_BTS_PARAMS:
            status = fbe_raid_group_set_default_bts_params(packet);
            break;

        case FBE_RAID_GROUP_CONTROL_CODE_GET_DEFAULT_BTS_PARAMS:
            status = fbe_raid_group_get_default_bts_params(packet);
            break;

        case FBE_RAID_GROUP_CONTROL_CODE_SET_DEFAULT_DEBUG_FLAGS:
            status = fbe_raid_group_set_default_debug_flags(packet);
            break;

        case FBE_RAID_GROUP_CONTROL_CODE_SET_DEFAULT_LIBRARY_FLAGS:
            status = fbe_raid_group_set_default_library_flags(packet);
            break;

        case FBE_RAID_GROUP_CONTROL_CODE_GET_DEFAULT_DEBUG_FLAGS:
            status = fbe_raid_group_get_default_debug_flags(packet);
            break;

        case FBE_RAID_GROUP_CONTROL_CODE_GET_DEFAULT_LIBRARY_FLAGS:
            status = fbe_raid_group_get_default_library_flags(packet);
            break;
        
        case FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_RAID_LIBRARY_ERROR_TESTING:
            status  = fbe_raid_group_class_set_raid_library_error_testing(packet);
            break;

        case FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_RAID_LIBRARY_RANDOM_ERRORS:
            status  = fbe_raid_group_class_set_random_library_errors(packet);
            break;

        case FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_RAID_LIBRARY_ERROR_TESTING_STATS:
            status  = fbe_raid_group_class_get_raid_library_error_testing_stats(packet);
            break;

        case FBE_RAID_GROUP_CONTROL_CODE_CLASS_RESET_RAID_LIBRARY_ERROR_TESTING_STATS:
            status  = fbe_raid_group_class_reset_raid_library_error_testing_stats(packet);
            break;

        case FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_RAID_MEMORY_STATS:
            status  = fbe_raid_group_class_get_raid_memory_stats(packet);
            break;

        case FBE_RAID_GROUP_CONTROL_CODE_CLASS_RESET_RAID_MEMORY_STATS:
            status = fbe_raid_group_class_reset_raid_memory_stats(packet);
            break;

        case FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_OPTIONS:
            status  = fbe_raid_group_class_set_options(packet);
            break;

        case FBE_RAID_GROUP_CONTROL_CODE_SET_BG_OP_SPEED:
            status = fbe_raid_group_class_set_bg_op_speed(packet);
            break;   
            
        case FBE_RAID_GROUP_CONTROL_CODE_GET_BG_OP_SPEED:
            status = fbe_raid_group_class_get_bg_op_speed(packet);
            break;            
            
        case FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_UPDATE_PEER_CHECKPOINT_INTERVAL:
            status =  fbe_raid_group_class_set_update_peer_checkpoint_interval(packet);
            break;

        case FBE_RAID_GROUP_CONTROL_CODE_CLASS_GET_EXTENDED_MEDIA_ERROR_HANDLING:
            status = fbe_raid_group_class_get_extended_media_error_handling(packet);
            break;

        case FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_EXTENDED_MEDIA_ERROR_HANDLING:
            status = fbe_raid_group_class_set_extended_media_error_handling(packet);
            break;

        case FBE_RAID_GROUP_CONTROL_CODE_CLASS_SET_CHUNKS_PER_REBUILD:
            status = fbe_raid_group_class_set_chunks_per_rebuild(packet);
            break;

        default:
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            status = fbe_transport_complete_packet(packet);
            break;
    }
    return status;
}
/********************************************
 * end fbe_raid_group_class_control_entry
 ********************************************/

/*!**************************************************************
 * fbe_raid_group_class_get_info()
 ****************************************************************
 * @brief
 *  calculate infomration about a possible raid group from a
 *  set of input parameters.
 *
 * @param packet_p - control packet.               
 *
 * @return fbe_status_t   
 *
 * @author
 *  2/26/2010 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_class_get_info(fbe_packet_t * packet_p)
{
    fbe_raid_group_class_get_info_t * get_info_p = NULL;  /* INPUT */
    fbe_status_t status;
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL; 
    fbe_u32_t length = 0;  
    fbe_u16_t data_disks;
    fbe_lba_t user_blocks_per_chunk;
    fbe_u32_t width;
    fbe_raid_group_metadata_positions_t raid_group_metadata_position;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  
    fbe_payload_control_get_buffer(control_operation_p, &get_info_p);
   
    if (get_info_p == NULL)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s input buffer is NULL\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if (length != sizeof(fbe_raid_group_class_get_info_t))
    {
        fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s input buffer length %d != %llu\n",
                                 __FUNCTION__, length,
				(unsigned long long)sizeof(fbe_raid_group_class_get_info_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_raid_geometry_validate_width(get_info_p->raid_type, get_info_p->width);
    if (status != FBE_STATUS_OK)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s raid type %d width %d invalid\n", __FUNCTION__, get_info_p->raid_type, get_info_p->width);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_raid_geometry_determine_element_size(get_info_p->raid_type, 
                                                      get_info_p->b_bandwidth,
                                                      &get_info_p->element_size,
                                                      &get_info_p->elements_per_parity);
    if (status != FBE_STATUS_OK)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s raid type %d width %d cannot determine element size status %d", 
                                 __FUNCTION__, get_info_p->raid_type, get_info_p->width, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if ((get_info_p->exported_capacity == FBE_LBA_INVALID) &&
        (get_info_p->single_drive_capacity == FBE_LBA_INVALID))
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s raid type %d width %d exported and single drive capacity not valid", 
                                 __FUNCTION__, get_info_p->raid_type, get_info_p->width);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* For a striped mirror, when we do the call to get data disks normally it is from 
     * the striper object, which has width set to the number of edges. 
     * In the get class info call for a striped mirror the get_info_p->width is the 
     * total number of disks. 
     * Thus, we need to modify the width in this call to be the number of edges.
     */
    if (get_info_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        width = get_info_p->width / 2;
    }
    else
    {
        width = get_info_p->width;
    }
    status = fbe_raid_type_get_data_disks(get_info_p->raid_type,
                                          width,
                                          &data_disks);
    if (status != FBE_STATUS_OK)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s raid type %d width %d cannot get data disks for this raid type and width.\n", 
                                 __FUNCTION__, get_info_p->raid_type, get_info_p->width);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    get_info_p->data_disks = data_disks;

    /* Calculate the number of blocks per chunk using number of data disks. */
    user_blocks_per_chunk = FBE_RAID_DEFAULT_CHUNK_SIZE * data_disks;

    if (get_info_p->exported_capacity != FBE_LBA_INVALID)
    {
        if (get_info_p->exported_capacity == 0)
        {
            /* We cannot export a capacity that is less than a block.
             */
            fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s exported capacity %llx is too small.\n",
				     __FUNCTION__,
				     (unsigned long long)get_info_p->exported_capacity);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return status;
        }
        /* User passed in an exported capacity and wants us to determine the overhead and 
         * the per drive capacity. 
         * The imported capacity starts with the exported capacity.
         */
        get_info_p->imported_capacity = get_info_p->exported_capacity;

        /* Round the imported capacity up to a chunk before calculating
         * metadata capacity.
         */
        if (get_info_p->imported_capacity % user_blocks_per_chunk)
        {
            get_info_p->imported_capacity += user_blocks_per_chunk - (get_info_p->exported_capacity % user_blocks_per_chunk);
        }

        /* Now determine the metadata overhead by using exported capacity. */
        status = fbe_raid_group_class_get_metadata_positions(get_info_p->imported_capacity,
                                                             FBE_LBA_INVALID, data_disks, &raid_group_metadata_position,
                                                             get_info_p->raid_type);
        if(status != FBE_STATUS_OK)
        {
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        get_info_p->imported_capacity = raid_group_metadata_position.imported_capacity;
        get_info_p->single_drive_capacity = get_info_p->imported_capacity / data_disks;
    }
    else
    {
        /* User passed in the per drive capacity they want to consume.
         * This is the max they want to consume so we will never round this up. 
         */
        if (get_info_p->single_drive_capacity < (FBE_RAID_DEFAULT_CHUNK_SIZE * 2))
        {
            /* We cannot consume something that is less than a two full chunks, one for
             * the user data and one for the metadata. 
             */
            fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s single drive capacity %llx is too small.\n", 
                                     __FUNCTION__,
				     (unsigned long long)get_info_p->single_drive_capacity);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return status;
        }
        /* We need to round single drive capacity down to a full chunk before
         * calculating metadata capacity.
         */
        get_info_p->single_drive_capacity &= ~(FBE_RAID_DEFAULT_CHUNK_SIZE - 1);
        get_info_p->imported_capacity = get_info_p->single_drive_capacity * data_disks;

        /* Now determine the metadata overhead by using imported capacity. */
        status = fbe_raid_group_class_get_metadata_positions(FBE_LBA_INVALID, get_info_p->imported_capacity,
                                                             data_disks, &raid_group_metadata_position, get_info_p->raid_type);
        if(status != FBE_STATUS_OK)
        {
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Calcuate the exported capacity. */
        get_info_p->exported_capacity = raid_group_metadata_position.exported_capacity;
    }

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}
/******************************************
 * end fbe_raid_group_class_get_info()
 ******************************************/

/*!**************************************************************
 *          fbe_raid_group_class_get_raid_library_debug_flags()
 ****************************************************************
 *
 * @brief   Retrieve the default raid library debug flags value 
 *          that is used for all newly created raid groups.
 *
 * @param   packet_p - control packet.               
 *
 * @return  fbe_status_t   
 *
 * @author
 *  03/17/2010  Ron Proulx  - Created
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_class_get_raid_library_debug_flags(fbe_packet_t * packet_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_raid_group_raid_library_debug_payload_t *get_debug_flags_p = NULL;  /* INPUT */
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_buffer_length_t length;
    fbe_payload_control_operation_t *control_operation_p = NULL; 

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  
    fbe_payload_control_get_buffer(control_operation_p, &get_debug_flags_p);
   
    /* Validate that a buffer was supplied
     */
    if (get_debug_flags_p == NULL)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d no buffer supplied.\n",
                                 __FUNCTION__, __LINE__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate length
     */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if (length != sizeof(*get_debug_flags_p))
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d expected buffer length: 0x%llx actual: 0x%x\n",
                                 __FUNCTION__, __LINE__,
				(unsigned long long)sizeof(*get_debug_flags_p),
				length);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now invoke raid geometry method that gets the default raid library
     * debug flags for all raid groups created after this point.
     */
    fbe_raid_geometry_get_default_raid_library_debug_flags(&get_debug_flags_p->raid_library_debug_flags);
    
    /* Success.
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/**********************************************************
 * end fbe_raid_group_class_get_raid_library_debug_flags()
 **********************************************************/

/*!**************************************************************
 *          fbe_raid_group_class_get_raid_group_debug_flags()
 ****************************************************************
 *
 * @brief   Retrieve the default raid group debug flags value 
 *          that is used for all newly created raid groups.
 *
 * @param   packet_p - control packet.               
 *
 * @return  fbe_status_t   
 *
 * @author
 *  03/17/2010  Ron Proulx  - Created
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_class_get_raid_group_debug_flags(fbe_packet_t * packet_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_raid_group_raid_group_debug_payload_t *get_debug_flags_p = NULL;  /* INPUT */
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_buffer_length_t length;
    fbe_payload_control_operation_t *control_operation_p = NULL; 

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  
    fbe_payload_control_get_buffer(control_operation_p, &get_debug_flags_p);
   
    /* Validate that a buffer was supplied
     */
    if (get_debug_flags_p == NULL)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d no buffer supplied\n",
                                 __FUNCTION__, __LINE__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate length
     */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if (length != sizeof(*get_debug_flags_p))
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d expected buffer length: 0x%llx actual: 0x%x\n",
                                 __FUNCTION__, __LINE__,
				(unsigned long long)sizeof(*get_debug_flags_p),
				length);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now invoke raid group method that gets the default raid group
     * debug flags for all raid groups created after this point.
     */
    fbe_raid_group_get_default_raid_group_debug_flags(&get_debug_flags_p->raid_group_debug_flags);
    
    /* Success.
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/**********************************************************
 * end fbe_raid_group_class_get_raid_group_debug_flags()
 **********************************************************/

/*!**************************************************************
 *          fbe_raid_group_class_set_raid_library_debug_flags()
 ****************************************************************
 *
 * @brief   Set the default raid library default for all NEW raid
 *          groups to the value specified.  This method does NOT
 *          change the raid library debug flags for existing raid
 *          groups.
 *
 * @param   packet_p - control packet.               
 *
 * @return  fbe_status_t   
 *
 * @author
 *  03/15/2010  Ron Proulx  - Created
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_class_set_raid_library_debug_flags(fbe_packet_t * packet_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_raid_group_raid_library_debug_payload_t *set_debug_flags_p = NULL;  /* INPUT */
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_buffer_length_t length;
    fbe_payload_control_operation_t *control_operation_p = NULL; 

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  
    fbe_payload_control_get_buffer(control_operation_p, &set_debug_flags_p);
   
    /* Validate that a buffer was supplied
     */
    if (set_debug_flags_p == NULL)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d no buffer supplied \n",
                                 __FUNCTION__, __LINE__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate length
     */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if (length != sizeof(*set_debug_flags_p))
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d expected buffer length: 0x%llx actual: 0x%x\n",
                                 __FUNCTION__, __LINE__,
				(unsigned long long)sizeof(*set_debug_flags_p),
				length);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now invoke raid geometry method that changes the default raid library
     * debug flags for all raid groups created after this point.
     */
    status = fbe_raid_geometry_set_default_raid_library_debug_flags(set_debug_flags_p->raid_library_debug_flags);
    if (status != FBE_STATUS_OK)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d set default raid library debug flags status: 0x%x \n",
                                 __FUNCTION__, __LINE__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Success.
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/**********************************************************
 * end fbe_raid_group_class_set_raid_library_debug_flags()
 **********************************************************/

/*!**************************************************************
 *          fbe_raid_group_class_set_raid_group_debug_flags()
 ****************************************************************
 *
 * @brief   Set the default raid group default for all NEW raid
 *          groups to the value specified.  This method does NOT
 *          change the raid group debug flags for existing raid
 *          groups.
 *
 * @param   packet_p - control packet.               
 *
 * @return  fbe_status_t   
 *
 * @author
 *  03/15/2010  Ron Proulx  - Created
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_class_set_raid_group_debug_flags(fbe_packet_t * packet_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_raid_group_raid_group_debug_payload_t *set_debug_flags_p = NULL;  /* INPUT */
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_buffer_length_t length;
    fbe_payload_control_operation_t *control_operation_p = NULL; 

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  
    fbe_payload_control_get_buffer(control_operation_p, &set_debug_flags_p);
   
    /* Validate that a buffer was supplied
     */
    if (set_debug_flags_p == NULL)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d no buffer supplied \n",
                                 __FUNCTION__, __LINE__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate length
     */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if (length != sizeof(*set_debug_flags_p))
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d expected buffer length: 0x%llx actual: 0x%x\n",
                                 __FUNCTION__, __LINE__,
				 (unsigned long long)sizeof(*set_debug_flags_p),
				 length);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now invoke raid group method that changes the default raid group
     * debug flags for all raid groups created after this point.
     */
    status = fbe_raid_group_set_default_raid_group_debug_flags(set_debug_flags_p->raid_group_debug_flags);
    if (status != FBE_STATUS_OK)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d set default raid group debug flags status: 0x%x \n",
                                 __FUNCTION__, __LINE__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Success.
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/******************************************************************************
 * end fbe_raid_group_class_set_raid_group_debug_flags()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_class_get_journal_capacity()
 ******************************************************************************
 * @brief
 *  This function is used to calculate the journal write log logical capacity.
 * 
 *  Note: If we calcuate the metadata capacity based on imported capacity then
 *  few of the paged bit at the end will remain unused (It is because metadata
 *  of metadata will be stored in non paged metadata).
 * 
 *  @todo Calculation for the RAID group metadata needs to be fine tuned for
 *  the exact calcuation of the overhead incase of the imported capacity is
 *  given by the user.
 *
 * @param raid_group_type - Raid group type
 * @param data_disks - Number of data positions for this raid group
 * @param exported_capacity - Exported capacity (request size of raid group)
 * @param imported_capacity - Amount of logical space available for raid group
 * @param page_metadata_capacity - Logical capacity of paged metadata
 * @param journal_capacity_p - Pointer to journal capacity to update
 *
 * @return fbe_status_t
 *
 * @author
 *  11/29/2011  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_class_get_journal_capacity(fbe_raid_group_type_t raid_group_type,
                                                              fbe_u32_t data_disks,
                                                              fbe_lba_t exported_capacity,
                                                              fbe_lba_t imported_capacity,
                                                              fbe_lba_t page_metadata_capacity,
                                                              fbe_lba_t *journal_capacity_p)
{
    fbe_block_count_t   journal_write_log_block_count = 0;
    fbe_lba_t           journal_capacity = 0;

    /*! @todo Currently we don't take into account the available space.
     */
    FBE_UNREFERENCED_PARAMETER(exported_capacity);
    FBE_UNREFERENCED_PARAMETER(imported_capacity);
    FBE_UNREFERENCED_PARAMETER(page_metadata_capacity);

    /* Only calculate the journal capacity for parity raid group types.
     */
    if ((raid_group_type == FBE_RAID_GROUP_TYPE_RAID5) ||
        (raid_group_type == FBE_RAID_GROUP_TYPE_RAID3) ||
        (raid_group_type == FBE_RAID_GROUP_TYPE_RAID6)    )
    {
        /* Get the journal capacity.
         */
        journal_write_log_block_count = FBE_RAID_GROUP_WRITE_LOG_SIZE;

        /* The write log block count is per position.  Now multiply by the
         * number of data positions to get the logical capacity.
         */
        journal_capacity = (journal_write_log_block_count * data_disks);
    }

    /* Populate the journal capacity.
     */
    *journal_capacity_p = journal_capacity;

    /* Always return success.
     */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_class_get_journal_capacity()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_class_get_metadata_positions()
 ******************************************************************************
 * @brief
 *  This function is used to calculate the metadata capacity and positions
 *  based on the input provided by the caller, if exprted capacity is given
 *  then calcuate the metadata capacity based on exported capacity else calcuate
 *  the metadata capacity based on imported capacity.
 * 
 *  Note: If we calcuate the metadata capacity based on imported capacity then
 *  few of the paged bit at the end will remain unused (It is because metadata
 *  of metadata will be stored in non paged metadata).
 * 
 *  @todo Calculation for the RAID group metadata needs to be fine tuned for
 *  the exact calcuation of the overhead in case of the imported capacity is
 *  given by the user.
 *
 * @param exported_capacity - the exported capacity of the raid group (OUT, if == FBE_LBA_INVALID)
 *        imported_capacity - the imported capacity of the raid group (OUT, if == FBE_LBA_INVALID)
 *        data_disks - the number of data disks for the given raid group (IN)
 *        raid_group_metadata_position_p - structure describes the metadata position (OUT)
 *        raid_group_type - raid group type(IN)
 *
 * @return fbe_status_t
 *
 * @author
 *  3/23/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_raid_group_class_get_metadata_positions(fbe_lba_t exported_capacity,
                                            fbe_lba_t imported_capacity,
                                            fbe_u32_t data_disks,
                                            fbe_raid_group_metadata_positions_t *raid_group_metadata_position_p,
                                            fbe_raid_group_type_t raid_group_type)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_lba_t   overall_chunks = 0;
    fbe_lba_t   bitmap_entries_per_block = 0;
    fbe_lba_t   paged_bitmap_blocks = 0;
    fbe_lba_t   user_blocks_per_chunk = 0;
    fbe_lba_t   metadata_capacity = 0;
    fbe_lba_t   paged_bitmap_capacity = 0;
    fbe_lba_t   journal_write_log_start_lba = FBE_LBA_INVALID;
    fbe_lba_t   journal_write_log_capacity = 0;
    fbe_lba_t   metadata_protected_capacity = 0;


    /* Validate the input parameters */
    if(raid_group_metadata_position_p == NULL)
    {
        /* If raid group metadata position is null then return error. */
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* if number of data disks are zero then return error. */
    if(data_disks == 0)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: # data disks cannot be 0 for the raid\n", 
                                 __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* 1) If user has given exported capacity then calculate the metadata capacity
     *    based on exported capacity.
     * 2) If user has given imported capacity then calcuate the metadata capacity 
     *    based on imported capacity.
     */
    if((exported_capacity != FBE_LBA_INVALID)&&(imported_capacity == FBE_LBA_INVALID))
    {
        metadata_protected_capacity = exported_capacity;
    }
    else if((imported_capacity != FBE_LBA_INVALID)&&(exported_capacity == FBE_LBA_INVALID))
    {
        metadata_protected_capacity = imported_capacity;
    }
    else
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: Invalid capacity combination! Expt = 0x%llx and Impt = 0x%llx\n", 
                                 __FUNCTION__,
				 (unsigned long long)exported_capacity,
				 (unsigned long long)imported_capacity);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Find the user blocks per physical chunk. */
    user_blocks_per_chunk = FBE_RAID_DEFAULT_CHUNK_SIZE * (fbe_lba_t)data_disks;

    /* Capacity needs to be rounded uppper side before starting 
     * metadata capacity calculation.
     */
    if(metadata_protected_capacity % user_blocks_per_chunk != 0)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP,
                                 FBE_TRACE_LEVEL_WARNING,
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: cap 0x%llx needs be aligned to 0x%llx.\n", 
                                 __FUNCTION__,
				 (unsigned long long)metadata_protected_capacity,
				 (unsigned long long)user_blocks_per_chunk);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Calcuate the overall number of physical chunks required for the capacity.
     * 1 phyiscal chunk = 1 logical chunk * data_disks
     */
    overall_chunks = metadata_protected_capacity / user_blocks_per_chunk;
    
    /* Compute how many chunk entries can be saved in (1) logical block of paged metadata.
     * A block must aligned with entry size.  This is checked at compile time, by fbe_raid_group_load();
     */
    bitmap_entries_per_block = FBE_METADATA_BLOCK_DATA_SIZE / FBE_RAID_GROUP_CHUNK_ENTRY_SIZE;

    /* Compute the number of blocks of paged metadata needed for the user data chunks.
     * May not end the full block, so round up to the next full block.
     */
    paged_bitmap_blocks = (overall_chunks + (bitmap_entries_per_block - 1)) / bitmap_entries_per_block;

    /* Round the number of paged metadata blocks to phyiscal chunks (chunk size * data_disks).
       So paged metadata is aligned to the end of the data disks */
    if(paged_bitmap_blocks % user_blocks_per_chunk)
    {
        paged_bitmap_blocks += user_blocks_per_chunk - (paged_bitmap_blocks % user_blocks_per_chunk);
    }
    
    /* Now multiply by the number of metadata copies for the total paged bitmap capacity in blocks. */
    if (raid_group_type == FBE_RAID_GROUP_TYPE_SPARE)
    {
        paged_bitmap_capacity = paged_bitmap_blocks * FBE_RAID_GROUP_NUMBER_OF_METADATA_COPIES_SPARE;
    }
    else
    {
        paged_bitmap_capacity = paged_bitmap_blocks * FBE_RAID_GROUP_NUMBER_OF_METADATA_COPIES;
    }

    /* Get the journal write log capacity 
     * Only a parity unit uses write log and have a non-zero journal_write_log_capacity. 
     */
    status = fbe_raid_group_class_get_journal_capacity(raid_group_type,
                                                       data_disks,
                                                       exported_capacity,
                                                       imported_capacity,
                                                       paged_bitmap_capacity,
                                                       &journal_write_log_capacity);
    if (status != FBE_STATUS_OK)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s Get journal capacity failed with status: 0x%x \n", 
                                 __FUNCTION__, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Sanity check: journal_write_log_capacity always multiple of data_disks */
    if ((journal_write_log_capacity % data_disks) != 0)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP,
                                 FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s: mis-aligned journal_capacity: 0x%llx, Should be multiple of 0x%x\n", 
                                 __FUNCTION__,
				 (unsigned long long)journal_write_log_capacity,
				 data_disks);
    }

    /* Total metadata capacity is equal to paged bitmap capacity and journal write log capacity. */
    metadata_capacity = paged_bitmap_capacity + journal_write_log_capacity;

    if((exported_capacity != FBE_LBA_INVALID)&&(imported_capacity == FBE_LBA_INVALID))
    {
        /* Calculate the imported capacity. */
        imported_capacity = exported_capacity + metadata_capacity;
    }
    else if((imported_capacity != FBE_LBA_INVALID)&&(exported_capacity == FBE_LBA_INVALID))
    {
        /* Calculate the exported capacity. */
        exported_capacity = imported_capacity - metadata_capacity;
    }

    /* Calculate the journal write log configuration.
     * The journal write log is immediately below the paged bitmap on EACH data_disk.
     */
    journal_write_log_start_lba = (exported_capacity + paged_bitmap_capacity) / data_disks;

    /* Fill out the raid group metadata positions. */
    raid_group_metadata_position_p->paged_metadata_lba = exported_capacity;
    raid_group_metadata_position_p->paged_metadata_block_count = (fbe_block_count_t) paged_bitmap_blocks;
    raid_group_metadata_position_p->paged_mirror_metadata_offset = paged_bitmap_blocks;

    if (raid_group_type == FBE_RAID_GROUP_TYPE_SPARE)
    {
         raid_group_metadata_position_p->paged_metadata_capacity = paged_bitmap_capacity / FBE_RAID_GROUP_NUMBER_OF_METADATA_COPIES_SPARE;
    }
    else
    {
        raid_group_metadata_position_p->paged_metadata_capacity = paged_bitmap_capacity / FBE_RAID_GROUP_NUMBER_OF_METADATA_COPIES;
    }

    raid_group_metadata_position_p->imported_capacity = imported_capacity;
    raid_group_metadata_position_p->exported_capacity = exported_capacity;

    /* This is the total blocks protected by Raid.  This contains user + paged metadata.
     */
    raid_group_metadata_position_p->raid_capacity = raid_group_metadata_position_p->paged_metadata_lba + 
                                                    raid_group_metadata_position_p->paged_metadata_capacity;

    /* Whenever we grow the amount consumed by the raid group, we need to change this calculation.
     */
    raid_group_metadata_position_p->imported_blocks_per_disk = (exported_capacity + metadata_capacity) / data_disks;

    raid_group_metadata_position_p->journal_write_log_pba = journal_write_log_start_lba;
    raid_group_metadata_position_p->journal_write_log_physical_capacity = journal_write_log_capacity / data_disks;
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_class_get_metadata_positions()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_class_validate_width()
 ******************************************************************************
 * 
 * @brief   This method simply validates that the specific raid group class 
 *          supports the width requested.  For instance the number of edges (and
 *          the space allocated for such) supported is specific to the raid class.
 * 
 * @param   raid_group_p - Pointer to the raid group object
 * @param   requested_width - The width requested
 *
 * @return  fbe_status_t
 * 
 ******************************************************************************/
fbe_status_t fbe_raid_group_class_validate_width(fbe_raid_group_t *raid_group_p,
                                                 fbe_u32_t requested_width)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_class_id_t  class_id = FBE_CLASS_ID_INVALID;

    /* The minimum and maximum width is based on the supported raid group
     * classs id.
     */
    class_id = fbe_raid_group_get_class_id(raid_group_p);
    switch(class_id)
    {
        case FBE_CLASS_ID_STRIPER:
            /* Use the striper width values to validate the width
             */
            if ((requested_width >= FBE_STRIPER_MIN_WIDTH) &&
                (requested_width <= FBE_STRIPER_MAX_WIDTH)     )
            {
                status = FBE_STATUS_OK;
            }
            break;

        case FBE_CLASS_ID_MIRROR:
            /* Use mirror width values to validate the width
             */
            if ((requested_width >= FBE_MIRROR_MIN_WIDTH) &&
                (requested_width <= FBE_MIRROR_MAX_WIDTH)     )
            {
                status = FBE_STATUS_OK;
            }
            break;

        case FBE_CLASS_ID_PARITY:
            /* Use parity width values to validate the width
             */
            if ((requested_width >= FBE_PARITY_MIN_WIDTH) &&
                (requested_width <= FBE_PARITY_MAX_WIDTH)     )
            {
                status = FBE_STATUS_OK;
            }
            break;

        case FBE_CLASS_ID_VIRTUAL_DRIVE:
            /* There is only (1) width supported for a virtual drive
             */
            if (requested_width == FBE_VIRTUAL_DRIVE_WIDTH)
            {
                status = FBE_STATUS_OK;
            }
            break;

        default:
            /* Not one of the valid raid groups types
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s: class id: 0x%x not supported.\n", 
                                  __FUNCTION__, class_id);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Always return the status
     */
    return(status);
}
/* end of fbe_raid_group_class_validate_width*/

/*!****************************************************************************
 *          fbe_raid_group_class_validate_downstream_capacity()
 ******************************************************************************
 *
 * @brief   This method validates that the capacity of each component of a raid
 *          group is sufficient.  It simply uses the downstream edge information
 *          to retrieve the offset and available capacity, then confirms that
 *          the requested capacity is available or not.
 *
 * @param   raid_group_p - Pointer to raid group that we will check all edges of.
 * @param   required_blocks_per_disk - The number of blocks per downstream component
 *                          required by the raid group.
 *
 * @return  status - FBE_STATUS_OK - There is sufficient downstream capacity
 *                   FBE_STATUS_INSUFFICIENT_RESOURCES - There is insufficient
 *                      downstream capacity.
 *
 * @author
 *  12/01/2011  Ron Proulx  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_class_validate_downstream_capacity(fbe_raid_group_t *raid_group_p,
                                                               fbe_lba_t required_blocks_per_disk)
{
    fbe_u32_t           edge_index;
    fbe_block_edge_t   *block_edge_p = NULL;

    /* Walk thru all of the downstream edges and validate sufficieint capacity.
     */
    for (edge_index = 0; edge_index < raid_group_p->base_config.width; edge_index++)
    {
        /* Get the block edge pointer.
         */
        fbe_raid_group_get_block_edge(raid_group_p, &block_edge_p, edge_index); 
         
        /* Only check the capacity if the edge if it has been configured.
         */
        if ( (block_edge_p->base_edge.path_state != FBE_PATH_STATE_INVALID)                       &&
             (block_edge_p->base_edge.path_state != FBE_PATH_STATE_GONE)                          &&
            !(block_edge_p->base_edge.path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IGNORE_OFFSET)     )
        {
            /* Valdiate that the capacity required is less than or equal the
             * capacity of this edge (the edge capacity does not include the offset
             * but is the capacity available on this edge).
             */
            if (required_blocks_per_disk > block_edge_p->capacity)
            {
                /* Trace an error (this is not a warning since it is up to the
                 * configuration service to not allow this to happen).
                 */
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "raid_group: Val cap - edge_index: %d requested: 0x%llx is greater than capacity: 0x%llx\n", 
                                      edge_index,
				      (unsigned long long)required_blocks_per_disk,
				      (unsigned long long)block_edge_p->capacity);
                return FBE_STATUS_INSUFFICIENT_RESOURCES;
            }
        }
    }

    /* If we made it this far return success.
     */
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end of fbe_raid_group_class_validate_downstream_capacity()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_class_get_drive_transfer_limits()
 ******************************************************************************
 * 
 * @brief   This method retrieves the per-drive transfer limits.  The limits are
 *          used by the raid algorithms to prevent a request from exceeded the
 *          transfer size that a drive (or port or miniport) can accept.
 * 
 * @param   raid_group_p - Pointer to the raid group object
 * @param   max_blocks_per_request_p - Address of maxiumum number of block per
 *              drive request to populate
 *
 * @return  fbe_status_t
 * 
 * @todo    Need to add support for maximum number of sg entries also
 *
 * @author
 *  07/14/2010  Ron Proulx  - Created
 *
 ******************************************************************************/
fbe_status_t fbe_raid_group_class_get_drive_transfer_limits(fbe_raid_group_t *raid_group_p,
                                                            fbe_block_count_t *max_blocks_per_request_p)
{
    fbe_status_t    status = FBE_STATUS_OK;

    /*! First determine if we need to generate the maximum blocks per drive
     *  or not.
     *  
     *  @note Current assumption is that the maximum number of blocks per drive
     *        only needs to be determined once per boot.
     */
    if (fbe_raid_group_class_is_max_drive_blocks_configured() == FBE_FALSE)
    {
        fbe_u32_t max_bytes_per_drive_request;
        fbe_u32_t max_sg_entries;

        /* Need to issue control request to get maximum number of bytes
         * per drive request.
         */
        status = fbe_database_get_drive_transfer_limits(&max_bytes_per_drive_request, &max_sg_entries);

        /* If the request succeeds convert the maximum bytes per block to maximum
         * blocks.
         */
        if (status == FBE_STATUS_OK)
        {
            fbe_block_count_t   max_blocks_per_drive;

            /* Trace some information and set, then return the maximum blocks
             * per drive
             */
            max_blocks_per_drive = max_bytes_per_drive_request / FBE_BE_BYTES_PER_BLOCK;
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: %s - Setting max blocks per drive to: %llu \n",
                                  __FUNCTION__,
				  (unsigned long long)max_blocks_per_drive);
            fbe_raid_group_class_set_max_drive_blocks(max_blocks_per_drive);
            *max_blocks_per_request_p = fbe_raid_group_class_get_max_drive_blocks();
        }
        else
        {
            /* Else report the error
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: %s - Request to get transfer limits failed with status: 0x%x\n",
                                  __FUNCTION__, status);
        }
    }
    else
    {
        /* Else simply set the return value to the configured value
         */
        *max_blocks_per_request_p = fbe_raid_group_class_get_max_drive_blocks();
    }

    /* Always return the execution status
     */
    return(status);
}
/* end of fbe_raid_group_class_get_drive_transfer_limits() */



/*!***************************************************************************
 *          fbe_raid_group_class_set_raid_library_error_tracing()
 *****************************************************************************
 * @brief   This function enables the error-testing 
 * 
 * @param   packet_p - The packet requesting this operation.
 *
 * @return  fbe_status_t
 *
 * @author  
 * 07/08/2010 - Created. Jyoti Ranjan
 *  
 ****************************************************************/
static fbe_status_t fbe_raid_group_class_set_raid_library_error_testing(fbe_packet_t * packet_p)
{
    fbe_status_t  status = FBE_STATUS_OK;

    /* Now invoke raid library function which enables the error-testing
     */
    status = fbe_raid_cond_set_library_error_testing();
    if (status != FBE_STATUS_OK)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d  Failed to enable errror testing. status = 0x%x \n",
                                 __FUNCTION__,
                                 __LINE__,
                                 status);
        
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Success.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/******************************************************************************
 * end fbe_raid_group_class_set_raid_library_error_testing()
 ******************************************************************************/

/*!***************************************************************************
 * fbe_raid_group_class_set_random_library_errors()
 *****************************************************************************
 * @brief   This function enables the randomized error-testing of
 *          software errors.
 * 
 * @param   packet_p - The packet requesting this operation.
 *
 * @return  fbe_status_t
 *
 * @author  
 *  7/2/2012 - Created. Rob Foley
 *  
 ****************************************************************/
static fbe_status_t fbe_raid_group_class_set_random_library_errors(fbe_packet_t * packet_p)
{
    fbe_status_t  status = FBE_STATUS_OK;
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL; 
    fbe_u32_t length = 0;
    fbe_raid_group_class_set_library_errors_t * set_errors_p = NULL;  /* INPUT */

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  
    fbe_payload_control_get_buffer(control_operation_p, &set_errors_p);
   
    if (set_errors_p == NULL)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s input buffer is NULL\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if (length != sizeof(fbe_raid_group_class_set_library_errors_t))
    {
        fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s input buffer length %d != %d\n",
                                 __FUNCTION__, length, (int)sizeof(fbe_raid_group_class_get_info_t));
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now invoke raid library function which enables the error-testing
     */
    status = fbe_raid_cond_set_library_random_errors(set_errors_p->inject_percentage,
                                                     !set_errors_p->b_only_inject_user_rgs);
    if (status != FBE_STATUS_OK)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d  Failed to enable errror testing. status = 0x%x \n",
                                 __FUNCTION__,
                                 __LINE__,
                                 status);
        
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Success.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/******************************************************************************
 * end fbe_raid_group_class_set_random_library_errors()
 ******************************************************************************/



/*!***************************************************************************
 *          fbe_raid_group_class_reset_raid_library_error_tracing_stats()
 *****************************************************************************
 *
 * @brief   This method activates the raid library error tracing
 *
 * @param   packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author  
 * 07/15/2010 - Created. Jyoti Ranjan
 *  
 ****************************************************************/
static fbe_status_t
fbe_raid_group_class_reset_raid_library_error_testing_stats(fbe_packet_t * packet_p)
{
    fbe_status_t    status = FBE_STATUS_OK; 

    /* Just call raid library function to reset the error-testing stats
     */
    status = fbe_raid_cond_reset_library_error_testing_stats();
    if (status != FBE_STATUS_OK)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d  Failed to reset error testing stats, status = 0x%x \n",
                                 __FUNCTION__,
                                 __LINE__,
                                 status);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Success.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/**************************************************
 * end fbe_raid_group_class_reset_raid_library_error_testing_stats()
 **************************************************/





/*!***************************************************************************
 *          fbe_raid_group_class_get_raid_library_error_testing_stats()
 *****************************************************************************
 *
 * @brief   This method gets the stats of testing done for un-expected error
 *          path at the point it is invoked.
 *
 * @param   packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author  
 * 07/15/2010 - Created. Jyoti Ranjan
 *  
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_class_get_raid_library_error_testing_stats(fbe_packet_t * packet_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_raid_group_raid_library_error_testing_stats_payload_t *error_testing_stats_payload_p = NULL;  /* INPUT */
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_buffer_length_t length;
    fbe_payload_control_operation_t *control_operation_p = NULL; 

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  
    fbe_payload_control_get_buffer(control_operation_p, &error_testing_stats_payload_p);
   
    /* Validate that a buffer was supplied
     */
    if (error_testing_stats_payload_p == NULL)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d no buffer supplied\n",
                                 __FUNCTION__, __LINE__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate length
     */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if (length != sizeof(*error_testing_stats_payload_p))
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d expected buffer length: 0x%llx actual: 0x%x\n",
                                 __FUNCTION__, __LINE__,
				(unsigned long long)sizeof(*error_testing_stats_payload_p), length);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now invoke raid group method that gets the default raid group
     * debug flags for all raid groups created after this point.
     */
    status = fbe_raid_cond_get_library_error_testing_stats(&error_testing_stats_payload_p->error_stats);
    if (status != FBE_STATUS_OK)
    {
        return status;
    }
    
    /* Success.
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/**************************************************
 * end fbe_raid_group_class_get_raid_library_error_testing_stats()
 **************************************************/
/*!**************************************************************
 * fbe_raid_group_class_set_options()
 ****************************************************************
 * @brief Set some raid group class specific options.
 *
 * @param   packet_p - control packet.               
 *
 * @return  fbe_status_t   
 *
 * @author
 *  8/4/2010 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_class_set_options(fbe_packet_t * packet_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_raid_group_class_set_options_t *set_options_p = NULL;  /* INPUT */
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_buffer_length_t length;
    fbe_payload_control_operation_t *control_operation_p = NULL; 

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  
    fbe_payload_control_get_buffer(control_operation_p, &set_options_p);
   
    /* Validate that a buffer was supplied
     */
    if (set_options_p == NULL)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d no buffer supplied \n",
                                 __FUNCTION__, __LINE__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate length
     */
    fbe_payload_control_get_buffer_length(control_operation_p, &length);
    if (length != sizeof(*set_options_p))
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d expected buffer length: 0x%llx actual: 0x%x\n",
                                 __FUNCTION__, __LINE__,
				(unsigned long long)sizeof(*set_options_p),
				length);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Use setters to set the various options.
     */
    status = fbe_raid_group_set_default_user_timeout_ms(set_options_p->user_io_expiration_time);
    if (status != FBE_STATUS_OK)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d setting user io expiration status: 0x%x \n",
                                 __FUNCTION__, __LINE__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_raid_group_set_paged_metadata_timeout_ms(set_options_p->paged_metadata_io_expiration_time);
    if (status != FBE_STATUS_OK)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s lie: %d setting metadata io expiration status: 0x%x \n",
                                 __FUNCTION__, __LINE__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (set_options_p->encrypt_vault_wait_time != 0) {
        status = fbe_raid_group_set_vault_wait_ms(set_options_p->encrypt_vault_wait_time);

        if (status != FBE_STATUS_OK) {
            fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "raid group: %s lie: %d setting vault wait_ms status: 0x%x \n",
                                     __FUNCTION__, __LINE__, status);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    if (set_options_p->encryption_blob_blocks != 0) {
        status = fbe_raid_group_set_encrypt_blob_blocks(set_options_p->encryption_blob_blocks);
        if (status != FBE_STATUS_OK) {
            fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "raid group: %s lie: %d setting encrypt blob blocks status: 0x%x \n",
                                     __FUNCTION__, __LINE__, status);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    /* Success.
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/******************************************************************************
 * end fbe_raid_group_class_set_options()
 ******************************************************************************/


/*!***************************************************************************
 *          fbe_raid_group_class_get_raid_memory_stats()
 *****************************************************************************
 *
 * @brief   This method gets the stats of raid memory
 *
 * @param   packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author  
 * 03/07/2011 - Created. Swati Fursule
 *  
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_class_get_raid_memory_stats(fbe_packet_t * packet_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_raid_memory_stats_t *raid_memory_stats_p = NULL;  /* INPUT */
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_buffer_length_t length;
    fbe_payload_control_operation_t *control_operation_p = NULL; 

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  
    fbe_payload_control_get_buffer(control_operation_p, &raid_memory_stats_p);
   
    /* Validate that a buffer was supplied
     */
    if (raid_memory_stats_p == NULL)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d no buffer supplied\n",
                                 __FUNCTION__, __LINE__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate length
     */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if (length != sizeof(*raid_memory_stats_p))
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d expected buffer length: 0x%llx actual: 0x%x\n",
                                 __FUNCTION__, __LINE__,
				(unsigned long long)sizeof(*raid_memory_stats_p),
				length);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now invoke raid group method that gets the default raid group
     * debug flags for all raid groups created after this point.
     */
    status = fbe_raid_memory_api_get_raid_memory_statistics(raid_memory_stats_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Success.
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/**************************************************
 * end fbe_raid_group_class_get_raid_memory_stats()
 **************************************************/

/*!***************************************************************************
 *          fbe_raid_group_class_reset_raid_memory_stats()
 *****************************************************************************
 *
 * @brief   This method resets the stats of raid memory allocation
 *
 * @param   packet_p - The packet requesting this operation.
 *
 * @return  status - The status of the operation.
 * 
 * @note    There is no lock / synchronization performed.

 * @author  
 *  02/11/2014  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_class_reset_raid_memory_stats(fbe_packet_t * packet_p)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_payload_ex_t *sep_payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL; 

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    /* No buffer */  
   
    /* Now invoke raid group method that resets the raid memory allocation
     * statistics.
     */
    status = fbe_raid_memory_api_reset_raid_memory_statistics();
    if (status != FBE_STATUS_OK)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid_group: %s reset memory stats failed - status: 0x%x\n",
                                 __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Success.
     */
    fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                             FBE_TRACE_LEVEL_INFO, 
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "raid_group: %s reset memory stats successful \n",
                             __FUNCTION__);
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return(status);
}
/****************************************************
 * end fbe_raid_group_class_reset_raid_memory_stats()
 ****************************************************/

/*!****************************************************************************
 *          fbe_raid_group_class_set_bg_op_speed()
 ******************************************************************************
 * @brief
 *  This function is used to set Background operation speed.
 *
 * @param  packet               - pointer to the packet.
 *
 * @return status               - status of the operation.
 *
 * @author
 *  08/06/2012 - Created. Amit Dhaduk
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_class_set_bg_op_speed(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                         payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_raid_group_control_set_bg_op_speed_t * set_op_speed = NULL;
    fbe_payload_control_buffer_length_t         length = 0;


    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &set_op_speed);
    if (set_op_speed == NULL) {

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If length of the buffer does not match with set get provision drive 
     * information structure lenght then return error.
     */
    fbe_payload_control_get_buffer_length (control_operation, &length);
    if(length != sizeof(fbe_raid_group_control_set_bg_op_speed_t)) {

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(set_op_speed->background_op == FBE_RAID_GROUP_BACKGROUND_OP_REBUILD)
    {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "RG: set rebuild speed to %d \n",set_op_speed->background_op_speed);
    
        fbe_raid_group_set_rebuild_speed(set_op_speed->background_op_speed);
    }
    else if(set_op_speed->background_op == FBE_RAID_GROUP_BACKGROUND_OP_RW_VERIFY) 
    {
        /* background operation speed for all verify type is common. */        
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                               "RG: set verify speed to %d \n",set_op_speed->background_op_speed);
    
        fbe_raid_group_set_verify_speed(set_op_speed->background_op_speed);
    }
    else
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} 
/******************************************************************************
 * end fbe_raid_group_class_set_bg_op_speed()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_class_get_bg_op_speed()
 ******************************************************************************
 * @brief
 *  This function is used to get Background operation speed
 *
 * @param  packet               - pointer to the packet.
 *
 * @return status               - status of the operation.
 *
 * @author
 *  08/06/2012 - Created. Amit Dhaduk
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_class_get_bg_op_speed(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                         payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_raid_group_control_get_bg_op_speed_t * get_op_speed = NULL;
    fbe_payload_control_buffer_length_t         length = 0;


    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &get_op_speed);
    if (get_op_speed == NULL) {

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If length of the buffer does not match with set get provision drive 
     * information structure lenght then return error.
     */
    fbe_payload_control_get_buffer_length (control_operation, &length);
    if(length != sizeof(fbe_raid_group_control_get_bg_op_speed_t)) {

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    get_op_speed->rebuild_speed = fbe_raid_group_get_rebuild_speed();
    get_op_speed->verify_speed = fbe_raid_group_get_verify_speed();

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} 
/******************************************************************************
 * end fbe_raid_group_class_get_bg_op_speed()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_class_set_update_peer_checkpoint_interval()
 ****************************************************************************** 
 * 
 * @brief   This usurper sets the period (in seconds) of how often we update
 *          peer checkpoint values.
 *
 * @param   packet               - pointer to the packet.
 *
 * @return  status               - status of the operation.
 * 
 * @note    Currently only the rebuild checkpoints use this interval.
 * 
 * @author
 *  03/22/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_class_set_update_peer_checkpoint_interval(fbe_packet_t * packet)
{
    fbe_payload_ex_t                   *payload = NULL;
    fbe_payload_control_operation_t    *control_operation = NULL;
    fbe_raid_group_class_set_update_peer_checkpoint_interval_t *peer_update_period_p = NULL;
    fbe_payload_control_buffer_length_t length = 0;
    fbe_u32_t                           current_update_period_ms = 0;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &peer_update_period_p);
    if (peer_update_period_p == NULL) 
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: set peer update interval NULL buffer\n");
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If length of the buffer does not match with set get provision drive 
     * information structure lenght then return error.
     */
    fbe_payload_control_get_buffer_length (control_operation, &length);
    if (length != sizeof(fbe_raid_group_class_set_update_peer_checkpoint_interval_t)) 
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: set peer update interval length: %d doesn't match expected: %d\n",
                                 length, (int)sizeof(fbe_raid_group_class_set_update_peer_checkpoint_interval_t));
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Must be 1 or greater.
     */
    if ((fbe_s32_t)peer_update_period_p->peer_update_seconds < 1)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_ERROR, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: set peer update interval: %d must be greater than 0\n",
                                 peer_update_period_p->peer_update_seconds);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Update the period.
     */
    fbe_raid_group_class_get_peer_update_checkpoint_interval_ms(&current_update_period_ms);
    fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                             FBE_TRACE_LEVEL_INFO, 
                             FBE_TRACE_MESSAGE_ID_INFO,
                             "raid group: set peer update from: %d to: %d seconds\n",
                             (current_update_period_ms / 1000), peer_update_period_p->peer_update_seconds);
    fbe_raid_group_class_set_peer_update_checkpoint_interval_ms((peer_update_period_p->peer_update_seconds * 1000));

    /* Return success
     */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
    
} 
/******************************************************************************
 * end fbe_raid_group_class_set_update_peer_checkpoint_interval()
 ******************************************************************************/


/*!**************************************************************
 * fbe_raid_group_get_dword_from_registry()
 ****************************************************************
 * @brief
 *  Read this flag from the registry.
 *
 * @param flag_p
 * @param key_p             
 *
 * @return None.
 *
 * @author
 *  4/26/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_raid_group_get_dword_from_registry(fbe_u32_t *flag_p, 
                                                           fbe_u8_t* key_p,
                                                           fbe_u32_t default_input_value)
{
    fbe_status_t status;
    fbe_u32_t flags;


    *flag_p = default_input_value;

    status = fbe_registry_read(NULL,
                               SEP_REG_PATH,
                               key_p,
                               &flags,
                               sizeof(fbe_u32_t),
                               FBE_REGISTRY_VALUE_DWORD,
                               &default_input_value,
                               sizeof(fbe_u32_t),
                               FALSE);

    if (status != FBE_STATUS_OK)
    {       
        return FBE_STATUS_GENERIC_FAILURE;        
    }
    *flag_p = flags;
    return status;
}
/******************************************
 * end fbe_raid_group_get_dword_from_registry()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_write_dword_to_registry()
 ****************************************************************
 * @brief
 *  Write this flag to the registry.
 *
 * @param flag
 * @param key_p
 *
 * @return None.
 *
 * @author
 *  4/26/2013 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_write_dword_to_registry(fbe_u32_t flag,
                                                    fbe_u8_t* key_p)
{
    fbe_status_t status;

    status = fbe_registry_write(NULL,
                                SEP_REG_PATH,
                                key_p,
                                FBE_REGISTRY_VALUE_DWORD,
                                &flag, 
                                sizeof(fbe_u32_t));

    if (status != FBE_STATUS_OK)
    {        
        return FBE_STATUS_GENERIC_FAILURE;        
    }
    return status;
}
/******************************************
 * end fbe_raid_group_write_dword_to_registry()
 ******************************************/
/*!***************************************************************************
 *          fbe_raid_group_set_default_bts_params()
 *****************************************************************************
 *
 * @brief Set the default values for block transport servers.
 *
 * @param   packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  4/26/2013 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_set_default_bts_params(fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_group_set_default_bts_params_t *params_p = NULL;
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_payload_control_buffer_length_t         length = 0;

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &params_p);
    if (params_p == NULL) {

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If length of the buffer does not match with set get provision drive 
     * information structure lenght then return error.
     */
    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_raid_group_set_default_bts_params_t)) {

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if (params_p == NULL)
    {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "RG set bts params null buffer.\n");
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else if (params_p->b_reload)
    {
        /* User just wants to load in the registry again.
         */
        fbe_raid_group_class_load_block_transport_parameters();
    }
    else
    {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "RG set BTS params: user qd: %u system qd: %u enabled: %s\n", 
                                    params_p->user_queue_depth, params_p->system_queue_depth, 
                                    (params_p->b_throttle_enabled) ? "YES" : "NO");

        if (params_p->user_queue_depth != 0)
        {
            fbe_raid_group_class_set_user_queue_depth(params_p->user_queue_depth);
        }
        else
        {
             fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO,
                                         FBE_TRACE_MESSAGE_ID_INFO,
                                         "RG value of 0 not allowed for user queue depth\n");
             status = FBE_STATUS_GENERIC_FAILURE;
        }
        if (params_p->system_queue_depth != 0)
        {
            fbe_raid_group_class_set_system_queue_depth(params_p->system_queue_depth);
        }
        else
        {
             fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO,
                                         FBE_TRACE_MESSAGE_ID_INFO,
                                         "RG value of 0 not allowed for user queue depth\n");
             status = FBE_STATUS_GENERIC_FAILURE;
        }

        if ((params_p->b_throttle_enabled == 0) || (params_p->b_throttle_enabled == 1))
        {
            fbe_raid_group_class_set_throttle_enabled(params_p->b_throttle_enabled);
        }
        else
        {
             fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO,
                                         FBE_TRACE_MESSAGE_ID_INFO,
                                         "RG value > 1 not allowed for throttle enable.\n");
             status = FBE_STATUS_GENERIC_FAILURE;
        }
        /* Now that we scanned successfully, push it out.
         */
        if ((status == FBE_STATUS_OK) && params_p->b_persist)
        {
            fbe_raid_group_class_persist_block_transport_parameters();
        }
    }

    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_raid_group_set_default_bts_params()
 **************************************************/
/*!***************************************************************************
 *          fbe_raid_group_get_default_bts_params()
 *****************************************************************************
 *
 * @brief Gets the default values for block transport servers.
 *
 * @param   packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  4/26/2013 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_get_default_bts_params(fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_group_get_default_bts_params_t *params_p = NULL;
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_payload_control_buffer_length_t         length = 0;

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &params_p);
    if (params_p == NULL) {

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If length of the buffer does not match with set get provision drive 
     * information structure lenght then return error.
     */
    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_raid_group_get_default_bts_params_t)) {

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if (params_p == NULL)
    {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "RG set bts params null buffer.\n");
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    params_p->b_throttle_enabled = fbe_raid_group_class_get_throttle_enabled();
    params_p->user_queue_depth = fbe_raid_group_class_get_user_queue_depth();
    params_p->system_queue_depth = fbe_raid_group_class_get_system_queue_depth();

    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_raid_group_get_default_bts_params()
 **************************************************/

/*!**************************************************************
 * fbe_raid_group_class_persist_block_transport_parameters()
 ****************************************************************
 * @brief
 *  Save the block transport parameters into the registry.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  4/29/2013 - Created. Rob Foley
 *
 ****************************************************************/

static void fbe_raid_group_class_persist_block_transport_parameters(void)
{
    fbe_status_t status;
    status = fbe_raid_group_write_dword_to_registry(fbe_raid_group_class_get_user_queue_depth(),
                                                    FBE_RAID_GROUP_REG_KEY_USER_QUEUE_DEPTH);
    if (status != FBE_STATUS_OK)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s failed save of user queue depth\n", __FUNCTION__);
        return;
    }

    status = fbe_raid_group_write_dword_to_registry(fbe_raid_group_class_get_system_queue_depth(),
                                                    FBE_RAID_GROUP_REG_KEY_SYSTEM_QUEUE_DEPTH);
    if (status != FBE_STATUS_OK)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s failed save of system queue depth\n", __FUNCTION__);
        return;
    }

    status = fbe_raid_group_write_dword_to_registry(fbe_raid_group_class_get_throttle_enabled(),
                                                    FBE_RAID_GROUP_REG_KEY_THROTTLE_ENABLED);
    if (status != FBE_STATUS_OK)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "%s failed save of throttle enabled\n", __FUNCTION__);
    }
    else
    {
        fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s successful save BTS parameters\n", __FUNCTION__);
    }
}
/******************************************
 * end fbe_raid_group_class_persist_block_transport_parameters()
 ******************************************/
/*!**************************************************************
 * fbe_raid_group_class_load_block_transport_parameters()
 ****************************************************************
 * @brief
 *  Fetch our block transport param defaults from the registry.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  4/29/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_raid_group_class_load_block_transport_parameters(void)
{
    fbe_status_t status;
    fbe_u32_t user_rg_queue_depth = FBE_RAID_GROUP_DEFAULT_QUEUE_DEPTH;
    fbe_u32_t system_rg_queue_depth = FBE_RAID_GROUP_MIN_QUEUE_DEPTH;
    fbe_bool_t b_throttle_enable = FBE_RAID_GROUP_THROTTLE_ENABLE_DEFAULT;
    fbe_u32_t rebuild_queue_ratio_addend = FBE_BLOCK_TRANSPORT_NORMAL_QUEUE_RATIO_ADDEND;
    fbe_u32_t rebuild_parity_read_multiplier = FBE_RAID_DEGRADED_PARITY_READ_MULTIPLIER;
    fbe_u32_t rebuild_parity_write_multiplier = FBE_RAID_DEGRADED_PARITY_WRITE_MULTIPLIER;
    fbe_u32_t rebuild_mirror_read_multiplier = FBE_RAID_DEGRADED_MIRROR_READ_MULTIPLIER;
    fbe_u32_t rebuild_mirror_write_multiplier = FBE_RAID_DEGRADED_MIRROR_WRITE_MULTIPLIER;
    fbe_u32_t rebuild_credit_ceiling_divisor = FBE_RAID_DEGRADED_CREDIT_CEILING_DIVISOR;
    fbe_u32_t extended_media_error_handling = FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_DEFAULT;
    fbe_u32_t chunks_per_rebuild = FBE_RAID_GROUP_REBUILD_BACKGROUND_OP_CHUNKS;

    status = fbe_raid_group_get_dword_from_registry(&user_rg_queue_depth, 
                                                    FBE_RAID_GROUP_REG_KEY_USER_QUEUE_DEPTH,
                                                    FBE_RAID_GROUP_DEFAULT_QUEUE_DEPTH);
    if (status == FBE_STATUS_OK) 
    { 
        if (user_rg_queue_depth > 0)
        {
            fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "RG found registry entry for rg user queue depth: %d\n", user_rg_queue_depth);
            fbe_raid_group_class_set_user_queue_depth(user_rg_queue_depth);
        }
        else
        {
            fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "RG found invalid registry entry for rg user queue depth: %d\n", user_rg_queue_depth);
        }
    }

    status = fbe_raid_group_get_dword_from_registry(&system_rg_queue_depth, 
                                                    FBE_RAID_GROUP_REG_KEY_SYSTEM_QUEUE_DEPTH,
                                                    FBE_RAID_GROUP_MIN_QUEUE_DEPTH);
    if (status == FBE_STATUS_OK) 
    { 
        if (system_rg_queue_depth > 0)
        {
            fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "RG found registry entry for rg system queue depth: %d\n", system_rg_queue_depth);
            fbe_raid_group_class_set_system_queue_depth(system_rg_queue_depth);
        }
        else
        {
            fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "RG found invalid registry entry for rg system queue depth: %d\n", system_rg_queue_depth);
        }
    }

    status = fbe_raid_group_get_dword_from_registry(&b_throttle_enable, 
                                                    FBE_RAID_GROUP_REG_KEY_THROTTLE_ENABLED,
                                                    FBE_RAID_GROUP_THROTTLE_ENABLE_DEFAULT);
    if (status == FBE_STATUS_OK)
    { 
        if ((b_throttle_enable == FBE_FALSE) || (b_throttle_enable == FBE_TRUE))
        {
            fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "RG found registry entry for throttle enable: %d\n", b_throttle_enable);
            fbe_raid_group_class_set_throttle_enabled(b_throttle_enable);
        }
        else
        {
            fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "RG found invalid registry entry for throttle enable: %d\n", b_throttle_enable);
        }
    }

    status = fbe_raid_group_get_dword_from_registry(&rebuild_queue_ratio_addend, 
                                                    FBE_RAID_GROUP_REG_KEY_REBUILD_QUEUE_RATIO_ADDEND,
                                                    FBE_BLOCK_TRANSPORT_NORMAL_QUEUE_RATIO_ADDEND);
    if (status == FBE_STATUS_OK)
    { 
            fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "RG found registry entry for rebuild queue ratio addend: %d\n", rebuild_queue_ratio_addend);
            fbe_block_transport_server_set_queue_ratio_addend((fbe_u8_t)rebuild_queue_ratio_addend);
    }  

    status = fbe_raid_group_get_dword_from_registry(&rebuild_parity_read_multiplier, 
                                                    FBE_RAID_GROUP_REG_KEY_REBUILD_PARITY_READ_MULTIPLIER,
                                                    FBE_RAID_DEGRADED_PARITY_READ_MULTIPLIER);
    if (status == FBE_STATUS_OK)
    { 
            fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "RG found registry entry for rebuild parity read multiplier: %d\n", rebuild_parity_read_multiplier);
            fbe_raid_geometry_set_parity_degraded_read_multiplier(rebuild_parity_read_multiplier);
    }

    status = fbe_raid_group_get_dword_from_registry(&rebuild_parity_write_multiplier, 
                                                    FBE_RAID_GROUP_REG_KEY_REBUILD_PARITY_WRITE_MULTIPLIER,
                                                    rebuild_parity_read_multiplier);

    if (status == FBE_STATUS_OK)
    {  
            fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "RG found registry entry for rebuild parity write multiplier: %d\n", rebuild_parity_write_multiplier);
            fbe_raid_geometry_set_parity_degraded_write_multiplier(rebuild_parity_write_multiplier);
    } else {
            fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "RG found invalid registry entry for rebuild parity write multiplier default: %d\n", rebuild_parity_write_multiplier);
            fbe_raid_geometry_set_parity_degraded_write_multiplier(rebuild_parity_write_multiplier);
    }

    status = fbe_raid_group_get_dword_from_registry(&rebuild_mirror_read_multiplier, 
                                                    FBE_RAID_GROUP_REG_KEY_REBUILD_MIRROR_READ_MULTIPLIER,
                                                    rebuild_parity_read_multiplier);
    if (status == FBE_STATUS_OK)
    { 
            fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "RG found registry entry for rebuild mirror read multiplier: %d\n", rebuild_mirror_read_multiplier);
            fbe_raid_geometry_set_mirror_degraded_read_multiplier(rebuild_mirror_read_multiplier);
    } else {

            fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "RG found invalid registry entry for mirror read multiplier default: %d\n", rebuild_mirror_read_multiplier);
            fbe_raid_geometry_set_mirror_degraded_read_multiplier(rebuild_mirror_read_multiplier);
    }

    status = fbe_raid_group_get_dword_from_registry(&rebuild_mirror_write_multiplier, 
                                                    FBE_RAID_GROUP_REG_KEY_REBUILD_MIRROR_WRITE_MULTIPLIER,
                                                    rebuild_mirror_read_multiplier);
    if (status == FBE_STATUS_OK)
    { 
            fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "RG found registry entry for rebuild mirror write multiplier: %d\n", rebuild_mirror_write_multiplier);
            fbe_raid_geometry_set_mirror_degraded_write_multiplier(rebuild_mirror_write_multiplier);
    } else {
            fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "RG found invalid registry entry for mirror write multiplier default: %d\n", rebuild_mirror_write_multiplier);
            fbe_raid_geometry_set_mirror_degraded_write_multiplier(rebuild_mirror_write_multiplier);
    }

    status = fbe_raid_group_get_dword_from_registry(&rebuild_credit_ceiling_divisor, 
                                                    FBE_RAID_GROUP_REG_KEY_REBUILD_CEILING_DIVISOR,
                                                    FBE_RAID_DEGRADED_CREDIT_CEILING_DIVISOR);
    if (status == FBE_STATUS_OK)
    { 
            fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "RG found registry entry for rebuild credit ceiling divisor: %d\n", rebuild_credit_ceiling_divisor);
            fbe_raid_geometry_set_credits_ceiling_divisor(rebuild_credit_ceiling_divisor);
    }

    status = fbe_raid_group_get_dword_from_registry(&extended_media_error_handling, 
                                                    FBE_RAID_GROUP_REG_KEY_EXTENDED_MEDIA_ERROR_HANDLING,
                                                    FBE_RAID_EXTENDED_MEDIA_ERROR_HANDLING_PARAMS_DEFAULT);
    if (status == FBE_STATUS_OK) {   
        fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                 "RG found registry entry for extended media error handling params: 0x%x\n", extended_media_error_handling);
        fbe_raid_group_class_set_extended_media_error_handling_params(extended_media_error_handling);
    }

    status = fbe_raid_group_get_dword_from_registry(&chunks_per_rebuild, 
                                                    FBE_RAID_GROUP_REG_KEY_CHUNKS_PER_REBUILD,
                                                    FBE_RAID_GROUP_REBUILD_BACKGROUND_OP_CHUNKS);
    if (status == FBE_STATUS_OK)
    { 
            fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                     "RG found registry entry for chunks per rebuild: %d\n", chunks_per_rebuild);
            fbe_raid_group_rebuild_set_background_op_chunks(chunks_per_rebuild);
    }
}
/******************************************
 * end fbe_raid_group_class_load_block_transport_parameters()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_class_get_queue_depth()
 ****************************************************************
 * @brief
 *  Fetch the queue depth for this raid group.
 * 
 *  We currently force system raid groups to have shorter
 *  queue depth to avoid contention by essentially distributing
 *  the available queue depth across all the RAID Groups
 *  on the system drives.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  4/29/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_raid_group_class_get_queue_depth(fbe_object_id_t object_id,
        fbe_u32_t width,
        fbe_u32_t *queue_depth_p)
{
    if (fbe_database_is_rg_with_system_drive(object_id)) {
        /* Check if it is syetem raid group */
        if(fbe_private_space_layout_object_id_is_system_object(object_id)) { /* This should be DB call */
            *queue_depth_p = fbe_raid_group_class_get_system_queue_depth() * width;
        } else {
            *queue_depth_p = 4 * width; /* This is actually KH fix */
        }
    }
    else {
        *queue_depth_p = fbe_raid_group_class_get_user_queue_depth() * width;
    }
}

/******************************************
 * end fbe_raid_group_class_get_queue_depth()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_get_default_object_debug_flags()
 ****************************************************************
 * @brief
 *  Get the default raid group debug flags for a given object id.
 *  This allows us to differentiate between system and user objects.
 *
 * @param object_id - Object id to get default debug flags for.
 * @param debug_flags_p - Debug flags to use.
 *
 * @return None.
 *
 * @author
 *  5/6/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_group_get_default_object_debug_flags(fbe_object_id_t object_id,
                                                   fbe_raid_group_debug_flags_t *debug_flags_p)
{
    /* First get the default which applies to all raid group types. 
     */
    fbe_raid_group_get_default_raid_group_debug_flags(debug_flags_p);

    /* Then add in the debug flag based upon the type of raid group.
     */
    if (object_id < FBE_RESERVED_OBJECT_IDS)
    {
        *debug_flags_p |= fbe_raid_group_default_system_debug_flags;
    }
    else
    {
        *debug_flags_p |= fbe_raid_group_default_user_debug_flags;
    }
    return;
}
/******************************************
 * end fbe_raid_group_get_default_object_debug_flags()
 ******************************************/
/*!**************************************************************
 * fbe_raid_group_get_default_system_debug_flags()
 ****************************************************************
 * @brief
 *  Get the default raid group debug flags for system objects.
 *
 * @param debug_flags_p - Debug flags to return.
 *
 * @return None.
 *
 * @author
 *  5/6/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_group_get_default_system_debug_flags(fbe_raid_group_debug_flags_t *debug_flags_p)
{
    *debug_flags_p = fbe_raid_group_default_system_debug_flags;
    return;
}
/******************************************
 * end fbe_raid_group_get_default_system_debug_flags()
 ******************************************/
/*!**************************************************************
 * fbe_raid_group_set_default_system_debug_flags()
 ****************************************************************
 * @brief
 *  Set the default raid group debug flags for system objects.
 *
 * @param debug_flags - Value to set debug flags to
 *
 * @return None.
 *
 * @author
 *  5/6/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_group_set_default_system_debug_flags(fbe_raid_group_debug_flags_t debug_flags)
{
    fbe_raid_group_default_system_debug_flags = debug_flags;
    return;
}
/******************************************
 * end fbe_raid_group_set_default_system_debug_flags()
 ******************************************/
/*!**************************************************************
 * fbe_raid_group_persist_default_system_debug_flags()
 ****************************************************************
 * @brief
 *  Set the default raid group debug flags for system objects.
 *
 *  We also persist this value to the registry.
 *
 * @param debug_flags - Value to set debug flags to
 *
 * @return None.
 *
 * @author
 *  5/6/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_group_persist_default_system_debug_flags(fbe_raid_group_debug_flags_t debug_flags)
{
    fbe_raid_group_write_dword_to_registry((fbe_u32_t)debug_flags, FBE_RAID_GROUP_REG_KEY_SYSTEM_DEBUG_FLAG);
    fbe_raid_group_default_system_debug_flags = debug_flags;
    return;
}
/******************************************
 * end fbe_raid_group_persist_default_system_debug_flags()
 ******************************************/
/*!**************************************************************
 * fbe_raid_group_get_default_user_debug_flags()
 ****************************************************************
 * @brief
 *  Get the default raid group debug flags for user objects.
 *
 * @param debug_flags_p - Current value of user rg debug flags.
 *
 * @return None.
 *
 * @author
 *  5/6/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_group_get_default_user_debug_flags(fbe_raid_group_debug_flags_t *debug_flags_p)
{
    *debug_flags_p = fbe_raid_group_default_user_debug_flags;
    return;
}
/******************************************
 * end fbe_raid_group_get_default_user_debug_flags()
 ******************************************/
/*!**************************************************************
 * fbe_raid_group_set_default_user_debug_flags()
 ****************************************************************
 * @brief
 *  Set the default raid group debug flags for user objects.
 *
 *  We also persist this value to the registry.
 *
 * @param debug_flags - Value to set debug flags to
 *
 * @return None.
 *
 * @author
 *  5/6/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_group_set_default_user_debug_flags(fbe_raid_group_debug_flags_t debug_flags)
{
    fbe_raid_group_default_user_debug_flags = debug_flags;
    return;
}
/******************************************
 * end fbe_raid_group_set_default_user_debug_flags()
 ******************************************/
/*!**************************************************************
 * fbe_raid_group_persist_default_user_debug_flags()
 ****************************************************************
 * @brief
 *  Set the default raid group debug flags for user objects.
 *
 *  We also persist this value to the registry.
 *
 * @param debug_flags - Value to set debug flags to
 *
 * @return None.
 *
 * @author
 *  5/6/2013 - Created. Rob Foley
 *
 ****************************************************************/
void fbe_raid_group_persist_default_user_debug_flags(fbe_raid_group_debug_flags_t debug_flags)
{
    fbe_raid_group_write_dword_to_registry((fbe_u32_t)debug_flags, FBE_RAID_GROUP_REG_KEY_USER_DEBUG_FLAG);
    fbe_raid_group_default_user_debug_flags = debug_flags;
    return;
}
/******************************************
 * end fbe_raid_group_persist_default_user_debug_flags()
 ******************************************/
/*!***************************************************************************
 *          fbe_raid_group_set_default_debug_flags()
 *****************************************************************************
 *
 * @brief 
 *   Handle a usurper packet which wants to set the default values 
 *   for debug flags of user and system raid groups.
 *
 * @param   packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  5/6/2013 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_set_default_debug_flags(fbe_packet_t * packet_p)
{
    fbe_raid_group_default_debug_flag_payload_t *set_debug_flags_p = NULL;
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_payload_control_buffer_length_t         length = 0;

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &set_debug_flags_p);
    if (set_debug_flags_p == NULL) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "fbe_raid_group_set_default_debug_flags buffer is null\n");
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If length of the buffer does not match with expected then return error.
     */
    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_raid_group_default_debug_flag_payload_t)) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if (set_debug_flags_p->system_debug_flags != FBE_RAID_GROUP_DEBUG_FLAG_RESERVED_IGNORE_SET){
        fbe_raid_group_persist_default_system_debug_flags(set_debug_flags_p->system_debug_flags);
    }
    if (set_debug_flags_p->user_debug_flags != FBE_RAID_GROUP_DEBUG_FLAG_RESERVED_IGNORE_SET){
        fbe_raid_group_persist_default_user_debug_flags(set_debug_flags_p->user_debug_flags);
    }

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_raid_group_set_default_debug_flags()
 **************************************************/

/*!***************************************************************************
 *          fbe_raid_group_set_default_library_flags()
 *****************************************************************************
 *
 * @brief
 *   Handle a usurper packet which wants to get the default values for
 *   library debug flags for user and system raid groups.
 *
 * @param   packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  5/6/2013 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_set_default_library_flags(fbe_packet_t * packet_p)
{
    fbe_raid_group_default_library_flag_payload_t *set_debug_flags_p = NULL;
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_payload_control_buffer_length_t length = 0;

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &set_debug_flags_p);
    if (set_debug_flags_p == NULL){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "fbe_raid_group_set_default_library_flags buffer is null\n");
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If length of the buffer does not match with expected then return error.
     */
    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_raid_group_default_debug_flag_payload_t)){
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    if (set_debug_flags_p->system_debug_flags != FBE_RAID_LIBRARY_DEBUG_FLAG_LAST){
        fbe_raid_geometry_persist_default_system_library_flags(set_debug_flags_p->system_debug_flags);
    }
    if (set_debug_flags_p->user_debug_flags != FBE_RAID_LIBRARY_DEBUG_FLAG_LAST){
        fbe_raid_geometry_persist_default_user_library_flags(set_debug_flags_p->user_debug_flags);
    }

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_raid_group_set_default_library_flags()
 **************************************************/
/*!***************************************************************************
 *          fbe_raid_group_get_default_debug_flags()
 *****************************************************************************
 *
 * @brief Get the debug flags defaults.
 *
 * @param   packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  5/6/2013 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_get_default_debug_flags(fbe_packet_t * packet_p)
{
    fbe_raid_group_default_debug_flag_payload_t *get_debug_flags_p = NULL;
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_payload_control_buffer_length_t         length = 0;

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &get_debug_flags_p);
    if (get_debug_flags_p == NULL) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "rg_get_raid_group_debug_flags null buffer.\n");
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If length of the buffer does not match with expected then return error.
     */
    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_raid_group_default_debug_flag_payload_t)) {

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    fbe_raid_group_get_default_user_debug_flags(&get_debug_flags_p->user_debug_flags);
    fbe_raid_group_get_default_system_debug_flags(&get_debug_flags_p->system_debug_flags);

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_raid_group_get_default_debug_flags()
 **************************************************/

/*!***************************************************************************
 *          fbe_raid_group_get_default_library_flags()
 *****************************************************************************
 *
 * @brief Gets the library flags defaults.
 *
 * @param   packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  5/6/2013 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_get_default_library_flags(fbe_packet_t * packet_p)
{
    fbe_raid_group_default_library_flag_payload_t *get_debug_flags_p = NULL;
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_payload_control_buffer_length_t         length = 0;

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &get_debug_flags_p);
    if (get_debug_flags_p == NULL) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "rg_get_raid_library_debug_flags null buffer.\n");
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If length of the buffer does not match with expected then return error.
     */
    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_raid_group_default_debug_flag_payload_t)) {

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    fbe_raid_geometry_get_default_user_library_flags(&get_debug_flags_p->user_debug_flags);
    fbe_raid_geometry_get_default_system_library_flags(&get_debug_flags_p->system_debug_flags);

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_raid_group_get_default_library_flags()
 **************************************************/

/*!**************************************************************
 * fbe_raid_group_class_load_debug_flags()
 ****************************************************************
 * @brief
 *  Fetch class specific debug flag defaults from registry.
 * 
 *  These will be used to add debug flags to user or system
 *  raid groups that exist in the system.
 *
 * @param None.               
 *
 * @return None.
 *
 * @author
 *  5/6/2013 - Created. Rob Foley
 *
 ****************************************************************/

void fbe_raid_group_class_load_debug_flags(void)
{
    fbe_status_t status;
    fbe_u32_t debug_flags;
    
    status = fbe_raid_group_get_dword_from_registry(&debug_flags, 
                                                    FBE_RAID_GROUP_REG_KEY_USER_DEBUG_FLAG,
                                                    FBE_RAID_GROUP_DEBUG_FLAG_NONE);
    if (status == FBE_STATUS_OK) 
    { 
        fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                 "RG load class found registry entry for user debug flags: 0x%x\n", debug_flags);
        fbe_raid_group_set_default_user_debug_flags((fbe_raid_group_debug_flags_t)debug_flags);
    }
    status = fbe_raid_group_get_dword_from_registry(&debug_flags, 
                                                    FBE_RAID_GROUP_REG_KEY_SYSTEM_DEBUG_FLAG,
                                                    FBE_RAID_GROUP_DEBUG_FLAG_NONE);
    if (status == FBE_STATUS_OK) 
    { 
        fbe_topology_class_trace(FBE_CLASS_ID_PARITY, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                 "RG load class found registry entry for rg system debug flags: 0x%x\n", debug_flags);
        fbe_raid_group_set_default_system_debug_flags((fbe_raid_group_debug_flags_t)debug_flags);
    }
}
/******************************************
 * end fbe_raid_group_class_load_debug_flags()
 ******************************************/

/*!****************************************************************************
 *          fbe_raid_group_class_get_extended_media_error_handling_params()
 ****************************************************************************** 
 * 
 * @brief   This function returns the address of the class values for the Extended Media
 *          Error Handling (EMEH).
 * 
 * @param   None
 *
 * @return  address of class extended media error handling parameters
 *
 * @author 
 *  03/11/2015  Ron Proulx - Created.
 * 
 ******************************************************************************/
fbe_u32_t *fbe_raid_group_class_get_extended_media_error_handling_params(void)
{
    return &fbe_raid_group_extended_media_error_handling_params;
}
/**********************************************************************
 * end fbe_raid_group_class_get_extended_media_error_handling_params()
 **********************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_class_set_extended_media_error_handling_params()
 ****************************************************************************** 
 * 
 * @brief   This function returns the current values for the Extended Media
 *          Error Handling (EMEH).  These EMEH values are sent to any remaining
 *          position in a raid group when one or more positions are failed/removed.
 * 
 * @param   None
 *
 * @return  address of class extended media error handling parameters
 *
 * @author 
 *  03/11/2015  Ron Proulx - Created.
 * 
 ******************************************************************************/
static void fbe_raid_group_class_set_extended_media_error_handling_params(fbe_u32_t emeh_params)
{
    fbe_raid_group_extended_media_error_handling_params = emeh_params;
}
/**********************************************************************
 * end fbe_raid_group_class_set_extended_media_error_handling_params()
 **********************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_class_get_extended_media_error_handling()
 ****************************************************************************** 
 * 
 * @brief   This function returns the current values for the Extended Media
 *          Error Handling (EMEH).  These EMEH values are sent to any remaining
 *          position in a raid group when one or more positions are failed/removed.
 * 
 * @param   packet_p - pointer to the packet
 *
 * @return status 
 *
 * @author 
 *  03/11/2015  Ron Proulx - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_raid_group_class_get_extended_media_error_handling(fbe_packet_t * packet_p)
{
    fbe_raid_group_class_get_extended_media_error_handling_t *get_emeh_info_p = NULL;
    fbe_payload_ex_t                                   *payload = NULL;
    fbe_payload_control_operation_t                    *control_operation = NULL;
    fbe_payload_control_buffer_length_t                 length = 0;

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &get_emeh_info_p);
    if (get_emeh_info_p == NULL) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "get_extended_media_error_handling null buffer.\n");
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If length of the buffer does not match with expected then return error.
     */
    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_raid_group_class_get_extended_media_error_handling_t)) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "get_extended_media_error_handling null length: %d doesnt match expected: %d\n", 
                                    length, (fbe_u32_t)sizeof(fbe_raid_group_class_get_extended_media_error_handling_t));
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    /* Now populate the request
     */
    fbe_raid_group_emeh_get_class_default_mode(&get_emeh_info_p->default_mode);
    fbe_raid_group_emeh_get_class_current_mode(&get_emeh_info_p->current_mode);
    fbe_raid_group_emeh_get_class_default_increase(&get_emeh_info_p->default_threshold_increase);
    fbe_raid_group_emeh_get_class_current_increase(&get_emeh_info_p->current_threshold_increase);

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************************************
 * end fbe_raid_group_class_get_extended_media_error_handling()
 **************************************************************/
/*!**************************************************************
 * fbe_raid_group_persist_extended_media_error_handling_params()
 ****************************************************************
 * @brief
 *  Set the default EMEH parameters (persist thru reboot).
 *
 *  We also persist this value to the registry.
 *
 * @param   persist_emeh_params - EMEH params to set
 *
 * @return None.
 *
 * @author
 *  03/11/2015  Ron Proulx  - Created
 *
 ****************************************************************/
static void fbe_raid_group_persist_extended_media_error_handling_params(fbe_u32_t persist_emeh_params)
{
    fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "persist_extended_media_error_handling_params: 0x%x \n",
                                persist_emeh_params); 
    fbe_raid_group_write_dword_to_registry(persist_emeh_params, FBE_RAID_GROUP_REG_KEY_EXTENDED_MEDIA_ERROR_HANDLING);
    return;
}

/*!****************************************************************************
 *          fbe_raid_group_class_set_extended_media_error_handling()
 ****************************************************************************** 
 * 
 * @brief   This function sets the current values for the Extended Media
 *          Error Handling (EMEH).  These EMEH values are sent to any remaining
 *          position in a raid group when one or more positions are failed/removed.
 *          If `b_persist' is set it also persist the values using the registy.
 * 
 * @param   packet_p - pointer to the packet
 *
 * @return status 
 *
 * @author 
 *  03/11/2015  Ron Proulx - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_raid_group_class_set_extended_media_error_handling(fbe_packet_t * packet_p)
{
    fbe_raid_group_class_set_extended_media_error_handling_t *set_emeh_info_p = NULL;
    fbe_payload_ex_t                                   *payload = NULL;
    fbe_payload_control_operation_t                    *control_operation = NULL;
    fbe_payload_control_buffer_length_t                 length = 0;
    fbe_raid_emeh_mode_t                                current_emeh_mode;
    fbe_u32_t                                          *new_emeh_params_p = NULL;

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &set_emeh_info_p);
    if (set_emeh_info_p == NULL) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "set_extended_media_error_handling null buffer.\n");
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If length of the buffer does not match with expected then return error.
     */
    fbe_payload_control_get_buffer_length(control_operation, &length);
    if(length != sizeof(fbe_raid_group_class_set_extended_media_error_handling_t)) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "emeh class set null length: %d doesnt match expected: %d\n", 
                                    length, (fbe_u32_t)sizeof(fbe_raid_group_class_set_extended_media_error_handling_t));
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
   
    /* Now set the EMEH values
     */
    fbe_raid_group_emeh_get_class_current_mode(&current_emeh_mode);
    fbe_base_config_class_trace(FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "emeh class set current mode: %d new mode: %d persist: %d change increase: %d percent increase: %d\n", 
                                current_emeh_mode, set_emeh_info_p->set_mode,
                                set_emeh_info_p->b_persist, 
                                set_emeh_info_p->b_change_threshold, set_emeh_info_p->percent_threshold_increase);

    /* There are only two support modes for the set EMEH raid group class usurper:
     *      o Enable EMEH for the SP
     *              OR
     *      o Disable EMEH for the SP
     */
    if ((set_emeh_info_p->set_mode != FBE_RAID_EMEH_MODE_ENABLED_NORMAL) &&
        (set_emeh_info_p->set_mode != FBE_RAID_EMEH_MODE_DISABLED)          ) {
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "set_extended_media_error_handling requested mode: %d not enable or disable\n", 
                                    set_emeh_info_p->set_mode);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Generate an event stating that EMEH is either disabled or enabled
     */
    if ((current_emeh_mode == FBE_RAID_EMEH_MODE_ENABLED_NORMAL)   &&
        (set_emeh_info_p->set_mode == FBE_RAID_EMEH_MODE_DISABLED)    )
    {
        /* Mode changed from enabled to disabled.  Generate the appropriate event log.
         */
        fbe_event_log_write(SEP_INFO_RAID_OBJECT_PROACTIVE_SPARE_RESILIENCY_DISABLED, NULL, 0, 
                            "%d", set_emeh_info_p->b_persist);
    } 
    else if ((current_emeh_mode == FBE_RAID_EMEH_MODE_DISABLED)               &&
             (set_emeh_info_p->set_mode == FBE_RAID_EMEH_MODE_ENABLED_NORMAL)    )
    {
        /* Mode changed from disabled to enabled.  Generate the appropriate event log.
         */
        fbe_event_log_write(SEP_INFO_RAID_OBJECT_PROACTIVE_SPARE_RESILIENCY_ENABLED, NULL, 0, 
                            "%d", set_emeh_info_p->b_persist);
    }

    /* Set the EMEH mode and optional parameters for this SP.
     */
    fbe_raid_group_emeh_set_class_current_mode(set_emeh_info_p->set_mode);
    if (set_emeh_info_p->b_change_threshold) {
        fbe_raid_group_emeh_set_class_current_increase(set_emeh_info_p->percent_threshold_increase,
                                                       set_emeh_info_p->set_mode);
    }

    /* If selected, persist the values.
     */
    if (set_emeh_info_p->b_persist) {
        new_emeh_params_p = fbe_raid_group_class_get_extended_media_error_handling_params();
        fbe_raid_group_persist_extended_media_error_handling_params(*new_emeh_params_p);
    }

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************************************
 * end fbe_raid_group_class_set_extended_media_error_handling()
 **************************************************************/



/*!****************************************************************************
 * fbe_raid_group_class_set_chunks_per_rebuild()
 ******************************************************************************
 * @brief
 *  Set the number of chunks to rebuild.
 *  Note: This is mainly used for testing purposes. It may not be safe to use
 *        when a rebuild is in progress
 *
 * @param packet_p - pointer to the packet
 *
 * @return status 
 *
 * @author 
 *   3/12/2015 - Created. Deanna Heng
 ******************************************************************************/
static fbe_status_t fbe_raid_group_class_set_chunks_per_rebuild(fbe_packet_t * packet_p)
{
    fbe_status_t                                        status = FBE_STATUS_OK;
    fbe_payload_control_operation_t                    *control_operation_p = NULL;
    fbe_payload_ex_t                                   *sep_payload_p = NULL;
    fbe_raid_group_class_set_chunks_per_rebuild_t      *set_chunks_per_rebuild_p = NULL;
    fbe_payload_control_buffer_length_t                 length = 0;

    /* get the control operation of the packet. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  
    fbe_payload_control_get_buffer(control_operation_p, &set_chunks_per_rebuild_p);
   
    /* Validate that a buffer was supplied
     */
    if (set_chunks_per_rebuild_p == NULL)
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d no buffer supplied \n",
                                 __FUNCTION__, __LINE__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Validate length
     */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if (length != sizeof(*set_chunks_per_rebuild_p))
    {
        fbe_topology_class_trace(FBE_CLASS_ID_RAID_GROUP, 
                                 FBE_TRACE_LEVEL_WARNING, 
                                 FBE_TRACE_MESSAGE_ID_INFO,
                                 "raid group: %s line: %d expected buffer length: 0x%llx actual: 0x%x\n",
                                 __FUNCTION__, __LINE__,
				(unsigned long long)sizeof(*set_chunks_per_rebuild_p),
				length);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_raid_group_rebuild_set_background_op_chunks(set_chunks_per_rebuild_p->chunks_per_rebuild);
    if(status != FBE_STATUS_OK)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    } 

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}
/******************************************************************************
 * end fbe_raid_group_class_set_chunks_per_rebuild()
 ******************************************************************************/


/******************************
 * end fbe_raid_group_class.c
 ******************************/


