/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_raid_group_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the raid group object code for the usurper.
 * 
 * @ingroup raid_group_class_files
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
#include "fbe_raid_group_rebuild.h"
#include "fbe_raid_library.h"
#include "fbe_raid_verify.h"
#include "fbe_raid_group_bitmap.h"
#include "fbe/fbe_winddk.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "fbe_database.h"
#include "fbe_raid_group_needs_rebuild.h"
#include "fbe_private_space_layout.h"
#include "fbe_raid_group_expansion.h"

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

static fbe_status_t
fbe_raid_group_set_configuration(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p);

static fbe_status_t
fbe_raid_group_update_configuration(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p);

static fbe_status_t
fbe_raid_group_usurper_get_lun_verify_status(fbe_raid_group_t* in_raid_group_p, fbe_packet_t* in_packet_p);


static fbe_status_t
fbe_raid_group_usurper_get_flush_error_counters(fbe_raid_group_t* in_raid_group_p, fbe_packet_t* in_packet_p);  
static fbe_status_t
fbe_raid_group_usurper_clear_flush_error_counters(fbe_raid_group_t* in_raid_group_p, fbe_packet_t* in_packet_p);

static fbe_status_t
fbe_raid_group_quiesce(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p);
static fbe_status_t
fbe_raid_group_unquiesce(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p);
static fbe_status_t
fbe_raid_group_get_info(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p);
static fbe_status_t
fbe_raid_group_get_write_log_info(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p);
static fbe_status_t
fbe_raid_group_get_raid_library_debug_flags(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p);
static fbe_status_t
fbe_raid_group_set_raid_library_debug_flags(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p);
static fbe_status_t
fbe_raid_group_get_raid_group_debug_flags(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p);
static fbe_status_t
fbe_raid_group_set_raid_group_debug_flags(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_raid_group_initialize_power_saving(fbe_raid_group_t *raid_group_p, fbe_raid_group_configuration_t * set_config_p);
static fbe_status_t fbe_raid_group_get_power_saving_parameters(fbe_raid_group_t* in_raid_group_p, fbe_packet_t *in_packet_p);
static fbe_status_t fbe_raid_group_change_power_save_idle_time(fbe_raid_group_t* in_raid_group_p, fbe_packet_t *in_packet_p);
static fbe_status_t fbe_raid_group_enable_object_power_save(fbe_raid_group_t* in_raid_group_p, fbe_packet_t *in_packet_p);
static fbe_status_t fbe_raid_group_disable_object_power_save(fbe_raid_group_t* in_raid_group_p, fbe_packet_t *in_packet_p);
static fbe_status_t fbe_raid_group_get_lun_align_size(fbe_raid_group_t * raid_group_p, fbe_lba_t * lun_align_size_p);


static fbe_status_t fbe_raid_group_calculate_zero_checkpoint_for_raid_extent(fbe_raid_group_t * raid_group_p,
                                                                             fbe_raid_group_get_zero_checkpoint_downstream_object_t * zero_checkpoint_downstream_object_p,
                                                                             fbe_raid_group_get_zero_checkpoint_raid_extent_t *get_zero_checkpoint);
static fbe_status_t fbe_raid_group_calculate_zero_checkpoint_for_raid10_extent(fbe_raid_group_t * raid_group_p,
                                                                             fbe_raid_group_get_zero_checkpoint_downstream_object_t * zero_checkpoint_downstream_object_p,
                                                                             fbe_raid_group_get_zero_checkpoint_raid_extent_t *get_zero_checkpoint);


fbe_status_t fbe_raid_group_get_zero_checkpoint_raid_extent(fbe_raid_group_t * raid_group_p,
                                                                   fbe_packet_t * packet_p);
fbe_status_t fbe_raid_group_get_zero_checkpoint_raid_extent_completion(fbe_packet_t * packet_p,
                                                                              fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_get_zero_checkpoint_for_downstream_objects(fbe_raid_group_t * raid_group_p,
                                                                              fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_get_zero_checkpoint_downstream_object_allocation_completion(fbe_memory_request_t * memory_request_p,
                                                                                               fbe_memory_completion_context_t context);
static fbe_status_t  fbe_raid_group_get_zero_checkpoint_downstream_object_subpacket_completion(fbe_packet_t * packet_p,
                                                                                               fbe_packet_completion_context_t context);

static fbe_status_t fbe_raid_group_get_io_info(fbe_raid_group_t*   in_raid_group_p,
                                               fbe_packet_t*       in_packet_p);
static fbe_status_t fbe_raid_group_stop_verify(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_control_metadata_paged_operation(fbe_raid_group_t *raid_group_p, 
                                                                    fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_control_metadata_paged_operation_completion(fbe_packet_t * packet, 
                                                                               fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_usurper_set_rebuild_checkpoint(fbe_raid_group_t *raid_group_p, 
                                                                  fbe_packet_t * packet_p);
static fbe_status_t 
fbe_raid_group_set_rebuild_checkpoint_completion_function(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_usurper_set_verify_checkpoint(fbe_raid_group_t *raid_group_p, 
                                                                 fbe_packet_t * packet_p);
static fbe_status_t
fbe_raid_group_usurper_set_nonpaged_metadata(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p);

static fbe_status_t 
fbe_raid_group_set_verify_checkpoint_completion_function(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

static fbe_status_t fbe_raid_group_usurper_can_pvd_be_reinitialized(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_raid_group_usurper_mark_nr(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_usurper_get_degraded_info(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_raid_group_usurper_get_rebuild_status(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_usurper_get_lun_percent_rebuilt(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p);
static fbe_status_t fbe_raid_group_usurper_set_mirror_prefered_position(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_usurper_get_spindown_qualified(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_usurper_get_health(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_control_set_throttle_info(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_control_get_throttle_info(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_control_get_stats(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_control_init_key_handle(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_control_change_encryption_state(fbe_raid_group_t * raid_group_p,
                                                                   fbe_packet_t *packet_p);
static fbe_status_t fbe_raid_group_control_remove_key_handle(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_control_update_drive_keys(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_usurper_get_lowest_drive_tier(fbe_raid_group_t *raid_group_p,
                                                                 fbe_packet_t *packet_p);
static fbe_status_t fbe_raid_group_query_for_lowest_drive_tier_allocation_completion(fbe_memory_request_t * memory_request_p,
                                                          fbe_memory_completion_context_t context);
static fbe_status_t fbe_raid_group_query_drive_tier_subpacket_completion(fbe_packet_t * packet_p,
                                                         fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_usurper_get_drive_pos_by_server_id(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_raid_group_usurper_get_extended_media_error_handling(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p);
static fbe_status_t fbe_raid_group_usurper_set_extended_media_error_handling(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p);
static fbe_status_t fbe_raid_group_usurper_set_debug_state(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p);

static fbe_status_t fbe_raid_group_get_wear_level_allocation_completion(fbe_memory_request_t * memory_request_p,
                                                                        fbe_memory_completion_context_t context);
static fbe_status_t fbe_raid_group_get_wear_level_subpacket_completion(fbe_packet_t * packet_p,
                                                                       fbe_packet_completion_context_t context);
static fbe_status_t fbe_raid_group_usurper_get_wear_level(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p);
static fbe_status_t fbe_raid_group_usurper_set_lifecycle_timer_interval(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p);

/*!***************************************************************
 * fbe_raid_group_control_entry()
 ****************************************************************
 * @brief
 *  This function is the management packet entry point for
 *  operations to the config object.
 *
 * @param object_handle - The config handle.
 * @param packet_p - The packet that is arriving.
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t 
fbe_raid_group_control_entry(fbe_object_handle_t object_handle, 
                             fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_raid_group_t * raid_group_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;
    fbe_payload_control_operation_opcode_t opcode;

    raid_group_p = (fbe_raid_group_t *)fbe_base_handle_to_pointer(object_handle);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_opcode(control_p, &opcode);

    /* Handle operations to a specific object.
     */
    switch (opcode)
    {
        case FBE_RAID_GROUP_CONTROL_CODE_SET_CONFIGURATION:
            status = fbe_raid_group_set_configuration(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_UPDATE_CONFIGURATION:
            status = fbe_raid_group_update_configuration(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_GET_LUN_VERIFY_STATUS:
            status = fbe_raid_group_usurper_get_lun_verify_status(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_GET_FLUSH_ERROR_COUNTERS:
            status = fbe_raid_group_usurper_get_flush_error_counters(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_CLEAR_FLUSH_ERROR_COUNTERS:
            status = fbe_raid_group_usurper_clear_flush_error_counters(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_QUIESCE:
            status = fbe_raid_group_quiesce(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_UNQUIESCE:
            status = fbe_raid_group_unquiesce(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_GET_INFO:
            status = fbe_raid_group_get_info(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_GET_WRITE_LOG_INFO:
            status = fbe_raid_group_get_write_log_info(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_GET_IO_INFO:
            status = fbe_raid_group_get_io_info(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_GET_RAID_LIBRARY_DEBUG_FLAGS:
            status = fbe_raid_group_get_raid_library_debug_flags(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_SET_RAID_LIBRARY_DEBUG_FLAGS:
            status = fbe_raid_group_set_raid_library_debug_flags(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_GET_RAID_GROUP_DEBUG_FLAGS:
            status = fbe_raid_group_get_raid_group_debug_flags(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_SET_RAID_GROUP_DEBUG_FLAGS:
            status = fbe_raid_group_set_raid_group_debug_flags(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_GET_POWER_SAVING_PARAMETERS:
            status = fbe_raid_group_get_power_saving_parameters(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_GET_ZERO_CHECKPOINT_RAID_EXTENT:
            status = fbe_raid_group_get_zero_checkpoint_raid_extent(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_GET_ZERO_CHECKPOINT_FOR_DOWNSTREAM_OBJECT:
            status = fbe_raid_group_get_zero_checkpoint_for_downstream_objects(raid_group_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_SET_OBJECT_POWER_SAVE_IDLE_TIME:
            status  = fbe_raid_group_change_power_save_idle_time(raid_group_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_ENABLE_OBJECT_POWER_SAVE:
            status  = fbe_raid_group_enable_object_power_save(raid_group_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_DISABLE_OBJECT_POWER_SAVE:
            status  = fbe_raid_group_disable_object_power_save(raid_group_p, packet_p);
            break;
            /* Raid needs to handle these here in order to add the locking info.
             */
        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_GET_BITS:
        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_SET_BITS:
        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_CLEAR_BITS:
            fbe_raid_group_control_metadata_paged_operation(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_SET_RB_CHECKPOINT:
            status = fbe_raid_group_usurper_set_rebuild_checkpoint(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_SET_VERIFY_CHECKPOINT:
            status = fbe_raid_group_usurper_set_verify_checkpoint(raid_group_p, packet_p);
            break;
       case FBE_RAID_GROUP_CONTROL_CODE_CAN_PVD_GET_REINTIALIZED:
            status = fbe_raid_group_usurper_can_pvd_be_reinitialized(raid_group_p, packet_p);
            break;
      case FBE_RAID_GROUP_CONTROL_CODE_MARK_NR:
            status = fbe_raid_group_usurper_mark_nr(raid_group_p, packet_p);
            break;
            /* Attach an upstream edge to our block transport server. */
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_ATTACH_EDGE:
            status = fbe_base_config_attach_upstream_block_edge((fbe_base_config_t*)raid_group_p, packet_p);
            /* Allow the downstream medic priority to propagate up.
             */
            fbe_raid_group_attribute_changed(raid_group_p, NULL);
            break;
        
        /* This is for fixing object which is stuck in SPECIALIZED when NP load failed */
        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_SET_NONPAGED:
            status = fbe_raid_group_usurper_set_nonpaged_metadata(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_GET_DEGRADED_INFO:
            status = fbe_raid_group_usurper_get_degraded_info(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_GET_REBUILD_STATUS:
            status = fbe_raid_group_usurper_get_rebuild_status(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_GET_LUN_PERCENT_REBUILT:
            status = fbe_raid_group_usurper_get_lun_percent_rebuilt(raid_group_p, packet_p);
            break;

        case FBE_RAID_GROUP_CONTROL_CODE_SET_PREFERED_POSITION:
            status = fbe_raid_group_usurper_set_mirror_prefered_position(raid_group_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_GET_SPINDOWN_QUALIFIED:
            status = fbe_raid_group_usurper_get_spindown_qualified(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_GET_HEALTH:
            status = fbe_raid_group_usurper_get_health(raid_group_p, packet_p);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_THROTTLE_INFO:
            status = fbe_raid_group_control_get_throttle_info(raid_group_p, packet_p);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_SET_THROTTLE_INFO:
            status = fbe_raid_group_control_set_throttle_info(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_GET_STATS:
            status = fbe_raid_group_control_get_stats(raid_group_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_INIT_KEY_HANDLE:
            status = fbe_raid_group_control_init_key_handle(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_SET_ENCRYPTION_STATE:
            fbe_raid_group_control_change_encryption_state(raid_group_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_REMOVE_KEY_HANDLE:
            status = fbe_raid_group_control_remove_key_handle(raid_group_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_UPDATE_DRIVE_KEYS:
            status = fbe_raid_group_control_update_drive_keys(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_GET_LOWEST_DRIVE_TIER:
            status = fbe_raid_group_usurper_get_lowest_drive_tier(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_GET_LOWEST_DRIVE_TIER_DOWNSTREAM:
            status = fbe_raid_group_query_for_lowest_drive_tier(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_SET_RAID_ATTRIBUTE:
            status = fbe_raid_group_usurper_set_raid_attribute(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_GET_DRIVE_POS_BY_SERVER_ID:
            status = fbe_raid_group_usurper_get_drive_pos_by_server_id(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_GET_EXTENDED_MEDIA_ERROR_HANDLING:
            status = fbe_raid_group_usurper_get_extended_media_error_handling(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_SET_EXTENDED_MEDIA_ERROR_HANDLING:
            status = fbe_raid_group_usurper_set_extended_media_error_handling(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_SET_DEBUG_STATE:
            status = fbe_raid_group_usurper_set_debug_state(raid_group_p, packet_p);
            break;

        case FBE_RAID_GROUP_CONTROL_CODE_GET_WEAR_LEVEL_DOWNSTREAM:
            status = fbe_raid_group_usurper_get_wear_level_downstream(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_GET_WEAR_LEVEL:
            status = fbe_raid_group_usurper_get_wear_level(raid_group_p, packet_p);
            break;
        case FBE_RAID_GROUP_CONTROL_CODE_SET_LIFECYCLE_TIMER:
            status = fbe_raid_group_usurper_set_lifecycle_timer_interval(raid_group_p, packet_p);
            break;
        default:
            /* Allow the base config object to handle all other ioctls.
             */
            status = fbe_base_config_control_entry(object_handle, packet_p);

            /* If Traverse status is returned and RaidGroup is the most derived
             * class then we have no derived class to handle Traverse status.
             * Complete the packet with Error.
             */
            if((FBE_STATUS_TRAVERSE == status) &&
               (FBE_CLASS_ID_RAID_GROUP == fbe_raid_group_get_class_id(raid_group_p)))
            {
                fbe_base_object_trace((fbe_base_object_t*) raid_group_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Can't Traverse for Most derived RaidGroup class. Opcode: 0x%X\n",
                                      __FUNCTION__, opcode);

                fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                status = fbe_transport_complete_packet(packet_p);
            }

            return status;
    }

    return status;
}
/* end fbe_raid_group_control_entry() */

/*!**************************************************************
 * fbe_raid_group_set_configuration()
 ****************************************************************
 * @brief
 *  This function sets up the basic configuration info
 *  for this raid group object.
 *
 * @param raid_group_p - The raid group.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  05/21/2009 - Created. rfoley
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_set_configuration(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p)
{
    fbe_raid_group_configuration_t *    set_config_p = NULL;    /* INPUT */
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_raid_group_metadata_positions_t     raid_group_metadata_positions;
    fbe_lba_t                               rounded_exported_capacity  = FBE_LBA_INVALID;
    fbe_lba_t                               user_blocks_per_chunk = FBE_LBA_INVALID;
    fbe_raid_geometry_t *                   raid_geometry_p = NULL;
    fbe_lba_t                               configured_total_capacity = FBE_LBA_INVALID;
    fbe_u16_t                               data_disks = 0;
    fbe_block_count_t                       max_blocks_per_request = 0;
    fbe_raid_emeh_mode_t                    emeh_mode;

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    // Get the control buffer pointer from the packet payload.
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p, packet_p,
                                                       sizeof(fbe_raid_group_configuration_t),
                                                       (fbe_payload_control_buffer_t *)&set_config_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Validate that this width is supported for the class specified
     */
    status = fbe_raid_group_class_validate_width(raid_group_p,
                                                 set_config_p->width);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: width: %d isn't valid for raid group class: 0x%x status: 0x%x\n", 
                              __FUNCTION__, set_config_p->width,
                              fbe_raid_group_get_class_id(raid_group_p), status);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the maximum number of blocks that can be sent to any position (a.k.a drive)
     */
    status = fbe_raid_group_class_get_drive_transfer_limits(raid_group_p,
                                                            &max_blocks_per_request);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: calculation of max per drive request failed\n", __FUNCTION__);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_raid_group_lock(raid_group_p);

    /*  Set the width in the base config and set the chunk size
     */
    fbe_base_config_set_width((fbe_base_config_t*)raid_group_p, set_config_p->width);
    fbe_raid_group_set_chunk_size(raid_group_p, set_config_p->chunk_size);

    /* Set the generation number. */
    fbe_base_config_set_generation_num((fbe_base_config_t*)raid_group_p, set_config_p->generation_number);

    /* Copy the debug flags from the configuration request */
    fbe_raid_group_set_debug_flag(raid_group_p, set_config_p->debug_flags);

    /* Get the number of data disks. */
    fbe_raid_type_get_data_disks(set_config_p->raid_type, set_config_p->width, &data_disks);

    /* Get the raid object metadata positions. */        
    fbe_raid_group_initialize_metadata_positions_with_default_values(&raid_group_metadata_positions);
    
    /* Round the exported capacity up to a chunk before calculating metadata capacity. */
    user_blocks_per_chunk = FBE_RAID_DEFAULT_CHUNK_SIZE * data_disks;
    if (set_config_p->capacity % user_blocks_per_chunk)
    {
        rounded_exported_capacity = set_config_p->capacity + user_blocks_per_chunk - (set_config_p->capacity % user_blocks_per_chunk);
    }
    else
    {
        rounded_exported_capacity = set_config_p->capacity;
    }   

    /* Set the exported capacity on block transport server. */
    fbe_base_config_set_capacity((fbe_base_config_t*)raid_group_p, rounded_exported_capacity);

    /* Calculate the metadata positions based on the exported capacity. */
    status = fbe_raid_group_class_get_metadata_positions(rounded_exported_capacity,
                                                         FBE_LBA_INVALID,
                                                         data_disks,
                                                         &raid_group_metadata_positions,
                                                         set_config_p->raid_type);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s: calculation of imported capacity failed.\n", __FUNCTION__);
        fbe_raid_group_unlock(raid_group_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Calculate the configured total capacity (exported capacity + metadata capacity). */
    configured_total_capacity = rounded_exported_capacity + raid_group_metadata_positions.paged_metadata_capacity;    

    /* If the maximum per-drive request size is less than the chunk size
     * generate a warning (it is allowed but background operation performance
     * will suffer
     */
    if (max_blocks_per_request < set_config_p->chunk_size)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: Max per-drive size: 0x%llx is less than chunk size: 0x%x\n", 
                              __FUNCTION__,
			      (unsigned long long)max_blocks_per_request,
			      set_config_p->chunk_size);
    }

    /* Configuring the raid geometry could fail if the configuration isn't
     * supported for this type of raid group etc.
     */
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    status = fbe_raid_geometry_set_configuration(raid_geometry_p,
                                                 set_config_p->width,
                                                 set_config_p->raid_type,
                                                 set_config_p->element_size,
                                                 set_config_p->elements_per_parity,
                                                 configured_total_capacity,
                                                 max_blocks_per_request);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_raid_group_unlock(raid_group_p);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }
    /* Set flag to disable mirror sequential optimization if the drive type is flash.
     */
    if (set_config_p->raid_group_drives_type == FBE_DRIVE_TYPE_SAS_FLASH_HE ||
        set_config_p->raid_group_drives_type == FBE_DRIVE_TYPE_SATA_FLASH_HE ||
        set_config_p->raid_group_drives_type == FBE_DRIVE_TYPE_SAS_FLASH_ME ||
        set_config_p->raid_group_drives_type == FBE_DRIVE_TYPE_SAS_FLASH_LE ||
        set_config_p->raid_group_drives_type == FBE_DRIVE_TYPE_SAS_FLASH_RI)
    {
        fbe_raid_geometry_set_flag(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_MIRROR_SEQUENTIAL_DISABLED);
    }

    /*Set the metadata start lba in the raid geometry*/
    fbe_raid_geometry_set_metadata_configuration(raid_geometry_p, 
                                                 raid_group_metadata_positions.paged_metadata_lba,
                                                 raid_group_metadata_positions.paged_metadata_capacity,
                                                 FBE_LBA_INVALID,
                                                 raid_group_metadata_positions.journal_write_log_pba);
    fbe_raid_group_unlock(raid_group_p);

    /*Power saving related stuff(no reason for this to be under lock)*/
    fbe_raid_group_initialize_power_saving(raid_group_p, set_config_p);

    /* Initialize the extended media error handling from class value.
     */
    fbe_raid_group_emeh_get_class_current_mode(&emeh_mode);

    /* Schedule monitor */
    if (status == FBE_STATUS_OK)
    {
        status = fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const,
                                        (fbe_base_object_t *) raid_group_p,
                                        (fbe_lifecycle_timer_msec_t) 0);    /* Immediate reschedule */
    }
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}
/**************************************
 * end fbe_raid_group_set_configuration()
 **************************************/
/*!**************************************************************
 * fbe_raid_group_update_configuration()
 ****************************************************************
 * @brief
 *  This function updates the raid group object.
 *
 * @param raid_group_p - The raid group.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 *
 ****************************************************************/

static fbe_status_t
fbe_raid_group_update_configuration(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p)
{
    fbe_raid_group_configuration_t *    raid_config_p = NULL;    /* INPUT */
    fbe_class_id_t                              my_class_id = FBE_CLASS_ID_INVALID;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_raid_geometry_t *                   raid_geometry_p = NULL;
    fbe_raid_group_type_t                   raid_type;
	fbe_u32_t								client_count;

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s called\n", __FUNCTION__);

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);

    my_class_id = fbe_raid_group_get_class_id(raid_group_p);

    // Get the control buffer pointer from the packet payload.
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p, packet_p,
                                                       sizeof(fbe_raid_group_configuration_t),
                                                       (fbe_payload_control_buffer_t *)&raid_config_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s:Update: class_id=0x%x, raid_type=0x%x, update_type=0x%x\n", __FUNCTION__, 
                           fbe_raid_group_get_class_id(raid_group_p), raid_type, raid_config_p->update_type);

	fbe_block_transport_server_get_number_of_clients(&raid_group_p->base_config.block_transport_server, &client_count);


    if(raid_config_p->update_type == FBE_UPDATE_RAID_TYPE_ENABLE_POWER_SAVE)
    { 
		/*first of all, let's set up the  base config*/
        status = fbe_base_config_enable_object_power_save((fbe_base_config_t *)raid_group_p);
		if (client_count != 0) {
            fbe_block_transport_server_set_path_attr_all_servers(&raid_group_p->base_config.block_transport_server, FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_CAN_SAVE_POWER);
        }

    }
    else if (raid_config_p->update_type == FBE_UPDATE_RAID_TYPE_DISABLE_POWER_SAVE)
    {        
        /*first of all, let's set up base config*/
        status = fbe_base_config_disable_object_power_save((fbe_base_config_t *)raid_group_p);  

		if (client_count != 0) {
            fbe_block_transport_server_clear_path_attr_all_servers(&raid_group_p->base_config.block_transport_server, FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_CAN_SAVE_POWER);
        }
    }
    else if(raid_config_p->update_type == FBE_UPDATE_RAID_TYPE_CHANGE_IDLE_TIME)
    {
        fbe_base_config_set_power_saving_idle_time((fbe_base_config_t *)raid_group_p, raid_config_p->power_saving_idle_time_in_seconds);

		/*anyone but PVD and VD should keep sending down */
		if (my_class_id != FBE_CLASS_ID_PROVISION_DRIVE && my_class_id != FBE_CLASS_ID_VIRTUAL_DRIVE ) {
			status = fbe_base_config_set_power_saving_idle_time_usurper_to_servers((fbe_base_config_t *)raid_group_p, packet_p);
	
			if (status != FBE_STATUS_OK) {
				fbe_base_object_trace(  (fbe_base_object_t *)raid_group_p,
										FBE_TRACE_LEVEL_WARNING,
										FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
										"failed to set power saving idle time on some or all servers, status: 0x%X\n", status);
		   }            
		}
		
    }

	else if(raid_config_p->update_type == FBE_UPDATE_RAID_TYPE_EXPAND_RG)
    {
        /*handle RG expansion by sending the new capacity to the RG object*/
		if (my_class_id == FBE_CLASS_ID_VIRTUAL_DRIVE) {

			fbe_base_object_trace(  (fbe_base_object_t *)raid_group_p,
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"RG Expansion request sent to VD, this is illegal\n");
			
			fbe_transport_set_status(packet_p, FBE_TRACE_LEVEL_ERROR, 0);
			fbe_transport_complete_packet(packet_p);
			return status;
		}

		status = fbe_raid_group_usurper_start_expansion(raid_group_p, packet_p);
		if (status != FBE_STATUS_OK) {
			fbe_base_object_trace(  (fbe_base_object_t *)raid_group_p,
									FBE_TRACE_LEVEL_WARNING,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"failed to expand RG, status: 0x%X\n", status);
		}  
        else {
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
		
    }

    if (my_class_id != FBE_CLASS_ID_VIRTUAL_DRIVE && raid_type != FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER) {
	
		fbe_block_transport_server_power_saving_config_changed(&raid_group_p->base_config.block_transport_server);
    }

    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}

/**************************************
 * end fbe_raid_group_update_configuration()
 **************************************/

/*!**************************************************************
 * fbe_raid_group_usurper_get_lun_verify_status()
 ****************************************************************
 * @brief
 *  This function gets the status of a background verify 
 *  operation for the lun edge that initiated this command.
 *
 * @param in_raid_group_p   - The raid group.
 * @param in_packet_p       - The packet requesting a
 *                            FBE_RAID_GROUP_CONTROL_CODE_GET_LUN_VERIFY_STATUS operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/07/2009 - Created. mvalois
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_usurper_get_lun_verify_status(
                                            fbe_raid_group_t*   in_raid_group_p,
                                            fbe_packet_t*       in_packet_p)
{
    fbe_raid_group_get_lun_verify_status_t* lun_verify_status_p;    // verify status information
    fbe_block_edge_t*       lun_edge_p;         // pointer to the LUN edge
    fbe_chunk_index_t       chunk_index;        // index to the next chunk in the LUN
    fbe_chunk_index_t       last_chunk;         // index of the last chunk in the LUN
    fbe_chunk_count_t       total_chunks;       // total number of chunks in LUN
    fbe_chunk_count_t       marked_chunks;      // number of marked chunks in LUN
    fbe_status_t            status;


    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);

    // Initializa local variables
    marked_chunks = 0;

    // Get the control buffer pointer from the packet payload.
    status = fbe_raid_group_usurper_get_control_buffer(in_raid_group_p, in_packet_p, 
                                            sizeof(fbe_raid_group_get_lun_verify_status_t),
                                            (fbe_payload_control_buffer_t *)&lun_verify_status_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(in_packet_p, status, 0);
        fbe_transport_complete_packet(in_packet_p);
        return status;
    }

    // Get the LUN edge pointer out of the packet
    lun_edge_p = (fbe_block_edge_t*)fbe_transport_get_edge(in_packet_p);

    // Get the bitmap chunk index and count for this edge
    status = fbe_raid_group_bitmap_get_lun_edge_extent_data(in_raid_group_p, lun_edge_p, &chunk_index, &total_chunks);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(in_packet_p, status, 0);
        fbe_transport_complete_packet(in_packet_p);
        return status;
    }

    // Calculate the index to the last chunk in this LUN extent
    last_chunk = chunk_index + total_chunks - 1;

    // Find the number of chunks that still need to be verified
    while (chunk_index <= last_chunk)
    {
        fbe_bool_t              is_marked_b;

        // Check if any user verify flags are set on this chunk
        is_marked_b = fbe_raid_group_bitmap_is_chunk_marked_for_verify(in_raid_group_p, chunk_index, 
                        (FBE_RAID_BITMAP_VERIFY_USER_READ_WRITE | FBE_RAID_BITMAP_VERIFY_USER_READ_ONLY));

        if (is_marked_b == FBE_TRUE)
        {
            // Count this chunk;
            marked_chunks++;
        }

        chunk_index++;
    }

    // Update the verify status with the gathered information
    lun_verify_status_p->total_chunks = total_chunks;
    lun_verify_status_p->marked_chunks = marked_chunks;

    fbe_transport_set_status(in_packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(in_packet_p);
    return FBE_STATUS_OK;
    
}   // End fbe_raid_group_usurper_get_lun_verify_status()

/*!**************************************************************
 * fbe_raid_group_usurper_get_flush_error_counters()
 ****************************************************************
 * @brief
 *  This function gets the parity write log flush error counters.
 *  These counters count verify and other errors which occur during
 *    the parity group flush operation on SP startup or failover
 *
 * @param in_raid_group_p   - The raid group.
 * @param in_packet_p       - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  5/10/2012 - Created. Dave Agans
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_usurper_get_flush_error_counters(fbe_raid_group_t* in_raid_group_p,
                                                                    fbe_packet_t* in_packet_p)
{
    fbe_raid_flush_error_counts_t*          req_buffer_p;
    fbe_status_t                            status;
    fbe_raid_flush_error_counts_t *         rg_report_p;

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);

    // Get the control buffer pointer from the packet payload.
    status = fbe_raid_group_usurper_get_control_buffer(in_raid_group_p, in_packet_p, 
                                            sizeof(fbe_raid_flush_error_counts_t),
                                            (fbe_payload_control_buffer_t)&req_buffer_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(in_packet_p, status, 0);
        status = fbe_transport_complete_packet(in_packet_p);
        return status;
    }

    // Copy the the RG flush error data into the requester's buffer.
    rg_report_p = fbe_raid_group_get_flush_error_counters_ptr(in_raid_group_p);
    fbe_copy_memory(req_buffer_p, rg_report_p, sizeof(fbe_raid_flush_error_counts_t));
    
    fbe_transport_set_status(in_packet_p, status, 0);
    status = fbe_transport_complete_packet(in_packet_p);
    return status;
}
// End fbe_raid_group_usurper_get_flush_error_counters()

/*!**************************************************************
 * fbe_raid_group_usurper_clear_flush_error_counters()
 ****************************************************************
 * @brief
 *  This function clears the raid group flush error report data.
 *
 * @param in_raid_group_p   - The raid group.
 * @param in_packet_p       - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  5/10/2012 - Created. Dave Agans
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_usurper_clear_flush_error_counters(fbe_raid_group_t* in_raid_group_p, 
                                                                      fbe_packet_t*     in_packet_p)
{
    fbe_raid_flush_error_counts_t*    rg_report_p;
    fbe_status_t                      status = FBE_STATUS_OK;

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);

    // Get a pointer to the verify report
    rg_report_p = fbe_raid_group_get_flush_error_counters_ptr(in_raid_group_p);

    // Clear the counters
    fbe_zero_memory(rg_report_p, sizeof(fbe_raid_flush_error_counts_t));

    fbe_transport_set_status(in_packet_p, status, 0);
    status = fbe_transport_complete_packet(in_packet_p);
    return status;
}
// End fbe_raid_group_usurper_clear_flush_error_counters()


/*!**************************************************************
 * fbe_raid_group_get_io_info()
 ****************************************************************
 * @brief
 *  This function gets the raid group I/O information
 *
 * @param raid_group_p   
 * @param packet_p       
 *                            
 *
 * @return status - The status of the operation.
 *
 * @author
 *  08/26/2010 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_get_io_info(fbe_raid_group_t *raid_group_p,
                                               fbe_packet_t *packet_p)
{
    fbe_status_t                    status;
    fbe_raid_group_get_io_info_t   *get_rg_io_info_p = NULL;
    fbe_u32_t                       number_of_ios_outstanding = 0;
    fbe_bool_t                      b_is_quiesced = FBE_FALSE;
    fbe_u32_t                       quiesced_count = 0;

    /* Validate the request buffer.
     */
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                                       packet_p, 
                                                       sizeof(*get_rg_io_info_p),
                                                       (fbe_payload_control_buffer_t)&get_rg_io_info_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Take the raid group lock.
     */
    fbe_raid_group_lock(raid_group_p);

    /* Get the information.
     */
    b_is_quiesced = fbe_base_config_is_any_clustered_flag_set((fbe_base_config_t *)raid_group_p, 
                        (FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED|FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCING));
    quiesced_count = raid_group_p->quiesced_count;
    status = fbe_base_config_get_quiesced_io_count((fbe_base_config_t*)raid_group_p, &number_of_ios_outstanding);

    /* Unlock the raid group
     */
    fbe_raid_group_unlock(raid_group_p);

    /* If the request failed return the error
     */
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s get outstanding failed - status: 0x%x \n",
                              __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Populate the payload and return success
     */
    get_rg_io_info_p->b_is_quiesced = b_is_quiesced;
    get_rg_io_info_p->quiesced_io_count = quiesced_count;
    get_rg_io_info_p->outstanding_io_count = number_of_ios_outstanding;

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/*********************************** 
 * end fbe_raid_group_get_io_info()
 ***********************************/

/*!**************************************************************
 * fbe_raid_group_quiesce()
 ****************************************************************
 * @brief
 *  This function sets up the basic configuration info
 *  for this raid group object.
 *
 * @param raid_group_p - The raid group.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/26/2009 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t 
fbe_raid_group_quiesce(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t status = fbe_raid_group_quiesce_implementation(raid_group_p);

	fbe_transport_set_status(packet_p, status, 0);
	fbe_transport_complete_packet(packet_p);
	
    return status;
}
/**************************************
 * end fbe_raid_group_quiesce()
 **************************************/

/*!**************************************************************
 * fbe_raid_group_unquiesce()
 ****************************************************************
 * @brief
 *  This function unquiesces the raid group.
 *
 * @param raid_group_p - The raid group.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/26/2009 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_unquiesce(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_raid_group_lock(raid_group_p);

    /* Only set the unquiesce condition if we are already quiesced.
     */
    if (fbe_base_config_is_clustered_flag_set((fbe_base_config_t *) raid_group_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED))
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s FBE_RAID_GROUP_CONTROL_CODE_UNQUIESCE received. Un-Quiescing...\n", __FUNCTION__);
        fbe_base_config_clear_flag((fbe_base_config_t *) raid_group_p, FBE_BASE_CONFIG_FLAG_USER_INIT_QUIESCE);
        fbe_base_config_clear_clustered_flag((fbe_base_config_t *) raid_group_p, FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD);
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const,
                               (fbe_base_object_t*)raid_group_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_UNQUIESCE);
        fbe_raid_group_unlock(raid_group_p);

        /* Schedule monitor.
         */
        status = fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const,
                                          (fbe_base_object_t *) raid_group_p,
                                          (fbe_lifecycle_timer_msec_t) 0);    /* Immediate reschedule */
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s FBE_RAID_GROUP_CONTROL_CODE_UNQUIESCE received. Not quiesced.\n", __FUNCTION__);
        fbe_raid_group_unlock(raid_group_p);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}
/**************************************
 * end fbe_raid_group_unquiesce()
 **************************************/

/*!**************************************************************
 * fbe_raid_group_get_info()
 ****************************************************************
 * @brief
 *  This function returns information about the raid group.
 *
 * @param raid_group_p - The raid group.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/26/2009 - Created. Rob Foley
 *  11/19/2009 - Get more fields from the raid geometry
 *  structure and fill get info structure - Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_get_info(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p)
{
    fbe_raid_group_get_info_t * get_info_p = NULL;
    fbe_lba_t capacity;
    fbe_u32_t width;
    fbe_element_size_t element_size;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_u32_t           position_index;         // index for a specific disk position 
    fbe_raid_group_metadata_positions_t             raid_group_metadata_positions;
    fbe_raid_group_nonpaged_metadata_t*       raid_group_non_paged_metadata_p;
    fbe_base_config_flags_t base_config_flags;
    fbe_base_config_clustered_flags_t base_config_clustered_flags;
    fbe_base_config_clustered_flags_t base_config_peer_clustered_flags;
    fbe_u32_t md_verify_pass_count;
    fbe_status_t status;
    fbe_lba_t     rebuild_checkpoint;
    fbe_lba_t     exported_disk_capacity;
    fbe_object_id_t my_object_id;
    fbe_block_edge_t *block_edge_p = NULL;
    fbe_lba_t     journal_log_start_lba;

    fbe_raid_group_get_metadata_positions(raid_group_p, &raid_group_metadata_positions);

    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                                       packet_p,
                                                       sizeof(fbe_raid_group_get_info_t),
                                                       (fbe_payload_control_buffer_t)&get_info_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    exported_disk_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);

    get_info_p->background_op_seconds = fbe_get_elapsed_seconds(raid_group_p->background_start_time);

    /* Validation checks are complete. 
     * Collect the raid group information. 
     */
    fbe_raid_group_lock(raid_group_p);

    fbe_base_config_get_capacity((fbe_base_config_t*)raid_group_p, &capacity);
    get_info_p->capacity = capacity;

    fbe_base_config_get_width((fbe_base_config_t*)raid_group_p, &width);
    get_info_p->width = width;

    if (fbe_raid_geometry_is_raid10(raid_geometry_p))
    {
        /* The geo width is the number of mirrors for a raid 10.
         */
        get_info_p->physical_width = width * 2;
    }
    else
    {
        get_info_p->physical_width = width;
    }
 
    get_info_p->imported_blocks_per_disk = raid_group_metadata_positions.imported_blocks_per_disk;
    get_info_p->raid_capacity = raid_group_metadata_positions.raid_capacity;
    get_info_p->paged_metadata_start_lba = raid_group_metadata_positions.paged_metadata_lba;
    get_info_p->paged_metadata_capacity = raid_group_metadata_positions.paged_metadata_capacity;
    get_info_p->write_log_start_pba = raid_group_metadata_positions.journal_write_log_pba;
    get_info_p->write_log_physical_capacity = raid_group_metadata_positions.journal_write_log_physical_capacity;

    fbe_raid_geometry_get_block_edge(raid_geometry_p, &block_edge_p, 0);
    get_info_p->physical_offset = fbe_block_transport_edge_get_offset(block_edge_p);

    get_info_p->debug_flags = raid_group_p->debug_flags;
    fbe_raid_geometry_get_debug_flags(raid_geometry_p, &get_info_p->library_debug_flags);
    fbe_base_config_get_flags((fbe_base_config_t *) raid_group_p, &base_config_flags);

    get_info_p->flags = raid_group_p->flags;
    get_info_p->base_config_flags = base_config_flags;

    fbe_base_config_get_clustered_flags((fbe_base_config_t *) raid_group_p, &base_config_clustered_flags);
    get_info_p->base_config_clustered_flags = base_config_clustered_flags;
    fbe_base_config_get_peer_clustered_flags((fbe_base_config_t *) raid_group_p, &base_config_peer_clustered_flags);
    get_info_p->base_config_peer_clustered_flags = base_config_peer_clustered_flags;
    get_info_p->metadata_element_state = raid_group_p->base_config.metadata_element.metadata_element_state;

    fbe_raid_geometry_get_element_size(raid_geometry_p, &element_size);
    get_info_p->element_size = element_size;

    fbe_raid_geometry_get_raid_type(raid_geometry_p, &get_info_p->raid_type);
    fbe_raid_geometry_get_exported_size(raid_geometry_p, &get_info_p->exported_block_size);
    fbe_raid_geometry_get_imported_size(raid_geometry_p, &get_info_p->imported_block_size);
    fbe_raid_geometry_get_optimal_size(raid_geometry_p, &get_info_p->optimal_block_size);

    fbe_raid_geometry_get_data_disks(raid_geometry_p, &get_info_p->num_data_disk);
    get_info_p->sectors_per_stripe = element_size * get_info_p->num_data_disk;

    get_info_p->stripe_count = (fbe_u32_t)(capacity / (get_info_p->sectors_per_stripe));
    fbe_raid_geometry_get_4k_bitmask(raid_geometry_p, &get_info_p->bitmask_4k);

    // Get elements per parity
    fbe_raid_geometry_get_elements_per_parity(raid_geometry_p, &get_info_p->elements_per_parity_stripe);
    fbe_raid_geometry_get_journal_log_start_lba(raid_geometry_p, &journal_log_start_lba);

    //  Get the rebuild logging setting and rebuild checkpoint for each disk position
    // For RAID 10, we'll have to get it from the objects below.
    for (position_index = 0; position_index < width; position_index++)
    {
        if (get_info_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10) 
        {
            // Unlock the RG before getting the Rebuilt chkpt
            fbe_raid_group_unlock(raid_group_p);
            status = fbe_raid_group_get_raid10_rebuild_info(packet_p, raid_group_p, position_index, 
                                                            &get_info_p->rebuild_checkpoint[position_index * 2],
                                                            &get_info_p->b_rb_logging[position_index * 2]);
            // Lock the RG again
            fbe_raid_group_lock(raid_group_p);

            if (status != FBE_STATUS_OK) {
                fbe_raid_group_unlock(raid_group_p);

                fbe_transport_set_status(packet_p, status, 0);
                fbe_transport_complete_packet(packet_p);
                return FBE_STATUS_OK;
            }
        }else{
            fbe_raid_group_get_rb_logging(raid_group_p, position_index, &get_info_p->b_rb_logging[position_index]);
            fbe_raid_group_get_rebuild_checkpoint(raid_group_p, position_index, 
                                                  &rebuild_checkpoint);
            /* This check is needed since if we are rebuilding metadata, rebuild checkpoint will be changing */
            if((rebuild_checkpoint != FBE_LBA_INVALID) && (rebuild_checkpoint > exported_disk_capacity))
            {
                if (rebuild_checkpoint < journal_log_start_lba)
                {
                    /* metadata rebuild in progress */
                    get_info_p->rebuild_checkpoint[position_index] = 0;
                }
                else
                {
                    /* journal rebuild in progress */
                    get_info_p->rebuild_checkpoint[position_index] = exported_disk_capacity;
                }
            }
            else
            {
                get_info_p->rebuild_checkpoint[position_index] = rebuild_checkpoint;                    
            }
        }
    }

    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)raid_group_p, (void **)&raid_group_non_paged_metadata_p);
    if (raid_group_non_paged_metadata_p != NULL)
    {
        if((raid_group_non_paged_metadata_p->ro_verify_checkpoint != FBE_LBA_INVALID) && 
           (raid_group_non_paged_metadata_p->ro_verify_checkpoint >= exported_disk_capacity))
        {    
            get_info_p->ro_verify_checkpoint = 0;
        }
        else
        {
            get_info_p->ro_verify_checkpoint = raid_group_non_paged_metadata_p->ro_verify_checkpoint;
            
        }
        if((raid_group_non_paged_metadata_p->rw_verify_checkpoint != FBE_LBA_INVALID) && 
           (raid_group_non_paged_metadata_p->rw_verify_checkpoint >= exported_disk_capacity))
        {    
            get_info_p->rw_verify_checkpoint = 0;
        }
        else
        {
            get_info_p->rw_verify_checkpoint = raid_group_non_paged_metadata_p->rw_verify_checkpoint;
            
        }
        if((raid_group_non_paged_metadata_p->error_verify_checkpoint != FBE_LBA_INVALID) && 
           (raid_group_non_paged_metadata_p->error_verify_checkpoint >= exported_disk_capacity))
        {    
            get_info_p->error_verify_checkpoint = 0;
        }
        else
        {
            get_info_p->error_verify_checkpoint = raid_group_non_paged_metadata_p->error_verify_checkpoint;
            
        }
        if((raid_group_non_paged_metadata_p->incomplete_write_verify_checkpoint != FBE_LBA_INVALID) && 
           (raid_group_non_paged_metadata_p->incomplete_write_verify_checkpoint >= exported_disk_capacity))
        {    
            get_info_p->incomplete_write_verify_checkpoint = 0;
        }
        else
        {
            get_info_p->incomplete_write_verify_checkpoint = raid_group_non_paged_metadata_p->incomplete_write_verify_checkpoint;
            
        }
        if((raid_group_non_paged_metadata_p->system_verify_checkpoint != FBE_LBA_INVALID) && 
           (raid_group_non_paged_metadata_p->system_verify_checkpoint >= exported_disk_capacity))
        {    
            get_info_p->system_verify_checkpoint = 0;
        }
        else
        {
            get_info_p->system_verify_checkpoint = raid_group_non_paged_metadata_p->system_verify_checkpoint;
            
        }        

        /* Just populate the get info with the journal verify checkpoint value */
        get_info_p->journal_verify_checkpoint = raid_group_non_paged_metadata_p->journal_verify_checkpoint;
        
        /* Just populate the get info with the np metadata flags */
        get_info_p->raid_group_np_md_flags = raid_group_non_paged_metadata_p->raid_group_np_md_flags;           
        get_info_p->raid_group_np_md_extended_flags = raid_group_non_paged_metadata_p->raid_group_np_md_extended_flags;           
    }
    else
    {
            get_info_p->ro_verify_checkpoint = 
            get_info_p->rw_verify_checkpoint = 
            get_info_p->error_verify_checkpoint = 
            get_info_p->journal_verify_checkpoint =  
            get_info_p->system_verify_checkpoint =              
            get_info_p->incomplete_write_verify_checkpoint = FBE_LBA_INVALID;
            
            get_info_p->raid_group_np_md_flags = 0; 
            get_info_p->raid_group_np_md_extended_flags = 0;
            
    }
    fbe_raid_group_get_lun_align_size(raid_group_p, &get_info_p->lun_align_size);

    get_info_p->local_state = raid_group_p->local_state;
    get_info_p->peer_state = raid_group_p->raid_group_metadata_memory_peer.base_config_metadata_memory.lifecycle_state;

    fbe_raid_group_get_paged_metadata_verify_pass_count(raid_group_p,
                                                        &md_verify_pass_count);
    get_info_p->paged_metadata_verify_pass_count = md_verify_pass_count;

    if (((get_info_p->capacity / get_info_p->num_data_disk) / FBE_RAID_DEFAULT_CHUNK_SIZE) > FBE_U32_MAX)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "rg_get_info (cap: 0x%llx / dsks: %d) / chnksz: 0x%x  > FBE_U32_MAX\n",
                              (unsigned long long)get_info_p->capacity, get_info_p->num_data_disk, FBE_RAID_DEFAULT_CHUNK_SIZE);
    }
    get_info_p->total_chunks = (fbe_chunk_count_t)(get_info_p->capacity / get_info_p->num_data_disk) / FBE_RAID_DEFAULT_CHUNK_SIZE;
    fbe_base_object_get_object_id((fbe_base_object_t*)raid_group_p, &my_object_id);
    fbe_database_get_rg_user_private(my_object_id, &get_info_p->user_private);

    /* Retrieve if anything is in the raid group objects event q.
     * We already have the rg config lock, so don't get it again.
     */
    get_info_p->b_is_event_q_empty = fbe_base_config_event_queue_is_empty_no_lock((fbe_base_config_t*)raid_group_p);
    
    get_info_p->rekey_checkpoint = fbe_raid_group_get_rekey_checkpoint(raid_group_p);

    fbe_base_config_get_encryption_mode((fbe_base_config_t*)raid_group_p, &get_info_p->encryption_mode);
    fbe_base_config_get_encryption_state((fbe_base_config_t*)raid_group_p, &get_info_p->encryption_state);

    fbe_raid_group_unlock(raid_group_p);

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_get_info()
 **************************************/

/*!**************************************************************
 * fbe_raid_group_get_write_log_info()
 ****************************************************************
 * @brief
 *  This function returns information about the raid group write log.
 *
 * @param raid_group_p - The raid group.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  07/10/2012 - Created. Dave Agans
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_get_write_log_info(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p)
{
    fbe_parity_get_write_log_info_t * write_log_info_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_u32_t i;
    fbe_u32_t journal_header_size = 0;
    fbe_status_t status;

    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                                       packet_p,
                                                       sizeof(fbe_parity_get_write_log_info_t),
                                                       (fbe_payload_control_buffer_t)&write_log_info_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    if (fbe_raid_geometry_is_parity_type(raid_geometry_p))
    {
        fbe_raid_group_lock(raid_group_p);

        write_log_info_p->start_lba   = raid_geometry_p->raid_type_specific.journal_info.write_log_info_p->start_lba;
        write_log_info_p->slot_size   = raid_geometry_p->raid_type_specific.journal_info.write_log_info_p->slot_size;
        write_log_info_p->slot_count  = raid_geometry_p->raid_type_specific.journal_info.write_log_info_p->slot_count;
        write_log_info_p->quiesced    =  fbe_parity_write_log_is_flag_set(raid_geometry_p->raid_type_specific.journal_info.write_log_info_p,
                                                                         FBE_PARITY_WRITE_LOG_FLAGS_QUIESCE);
        write_log_info_p->needs_remap = fbe_parity_write_log_is_flag_set(raid_geometry_p->raid_type_specific.journal_info.write_log_info_p,
                                                                         FBE_PARITY_WRITE_LOG_FLAGS_NEEDS_REMAP);
        write_log_info_p->flags       = raid_geometry_p->raid_type_specific.journal_info.write_log_info_p->flags;
        for (i = 0; i<write_log_info_p->slot_count; i++)
        {
            write_log_info_p->slot_array[i].state            = 
                                raid_geometry_p->raid_type_specific.journal_info.write_log_info_p->slot_array[i].state;
            write_log_info_p->slot_array[i].invalidate_state = 
                                raid_geometry_p->raid_type_specific.journal_info.write_log_info_p->slot_array[i].invalidate_state;
        }
        write_log_info_p->is_request_queue_empty = fbe_queue_is_empty(&raid_geometry_p->raid_type_specific.journal_info.write_log_info_p->request_queue_head);
        fbe_raid_geometry_get_journal_log_hdr_blocks(raid_geometry_p, &journal_header_size); 
        write_log_info_p->header_size = journal_header_size;
        fbe_raid_group_unlock(raid_group_p);
    }

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_raid_group_get_info()
 **************************************/

/*!***************************************************************************
 *          fbe_raid_group_get_raid_library_debug_flags()
 *****************************************************************************
 *
 * @brief   This method retrieves the value of the raid library debug flags
 *          for the raid group object specified.
 *
 * @param   raid_group_p - The raid group.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_get_raid_library_debug_flags(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_raid_group_raid_library_debug_payload_t *get_debug_flags_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    
    /* Get a pointer to the debug flags to populate
     */
    fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                              packet_p, 
                                              sizeof(fbe_raid_group_raid_library_debug_payload_t),
                                              (fbe_payload_control_buffer_t *)&get_debug_flags_p);

    if (get_debug_flags_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "rg_get_raid_library_debug_flags null buffer.\n");
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        /* Base object trace will trace the object id etc.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "rg_get_raid_library_debug_flags received.\n");
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "\t raid library debug flags: 0x%08x \n", 
                              get_debug_flags_p->raid_library_debug_flags);

        /* Lock the raid group object to syncrounize udpate.
         */
        fbe_raid_group_lock(raid_group_p);
        fbe_raid_geometry_get_debug_flags(raid_geometry_p,
                                          &get_debug_flags_p->raid_library_debug_flags);
        fbe_raid_group_unlock(raid_group_p);
        status = FBE_STATUS_OK;
    }

    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_raid_group_get_raid_library_debug_flags()
 **************************************************/

/*!***************************************************************************
 *          fbe_raid_group_get_raid_group_debug_flags()
 *****************************************************************************
 *
 * @brief   This method retrieves the value of the raid group debug flags
 *          for the raid group object specified.
 *
 * @param   raid_group_p - The raid group.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_get_raid_group_debug_flags(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_raid_group_raid_group_debug_payload_t *get_debug_flags_p = NULL;
    
    /* Get a pointer to the debug flags to populate
     */
    fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                              packet_p, 
                                              sizeof(fbe_raid_group_raid_group_debug_payload_t),
                                              (fbe_payload_control_buffer_t)&get_debug_flags_p);

    if (get_debug_flags_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "fbe_raid_group_get_raid_group_debug_flags  null buffer.\n");
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        /* Base object trace will trace the object id etc.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "fbe_raid_group_get_raid_group_debug_flags received.\n");
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "\t raid group debug flags: 0x%08x \n", 
                            get_debug_flags_p->raid_group_debug_flags);

        /* Lock the raid group object to syncrounize udpate.
         */
        fbe_raid_group_lock(raid_group_p);
        fbe_raid_group_get_debug_flags(raid_group_p,
                                       &get_debug_flags_p->raid_group_debug_flags);
        fbe_raid_group_unlock(raid_group_p);
        status = FBE_STATUS_OK;
    }

    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_raid_group_get_raid_group_debug_flags()
 **************************************************/

/*!***************************************************************************
 *          fbe_raid_group_set_raid_library_debug_flags()
 *****************************************************************************
 *
 * @brief   This method sets the raid library (i.e. the raid geometry) debug
 *          flags to the value passed for the associated raid group geometry.
 *
 * @param   raid_group_p - The raid group.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_set_raid_library_debug_flags(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_raid_group_raid_library_debug_payload_t *set_debug_flags_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_library_debug_flags_t  old_debug_flags;
    
    
    /* Get old debug flags.
     */
    fbe_raid_geometry_get_debug_flags(raid_geometry_p, &old_debug_flags);

    /* Now retrieve the value of the debug flags to set from the buffer.
     */
    fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                              packet_p, 
                                              sizeof(fbe_raid_group_raid_library_debug_payload_t),
                                              (fbe_payload_control_buffer_t*)&set_debug_flags_p);

    if (set_debug_flags_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "rg_set_raid_library_debug_flags null buffer.\n");
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        /* Base object trace will trace the object id etc.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "rg_set_raid_library_debug_flags received.\n");
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "\t Old raid library debug flags: 0x%08x new: 0x%08x \n", 
                              old_debug_flags, set_debug_flags_p->raid_library_debug_flags);

        /* Lock the raid group object to syncrounize udpate.
         */
        fbe_raid_group_lock(raid_group_p);
        fbe_raid_geometry_set_debug_flags(raid_geometry_p,
                                          set_debug_flags_p->raid_library_debug_flags);
        fbe_raid_group_unlock(raid_group_p);
        status = FBE_STATUS_OK;
    }

    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_raid_group_set_raid_library_debug_flags()
 **************************************************/

/*!***************************************************************************
 *          fbe_raid_group_set_raid_group_debug_flags()
 *****************************************************************************
 *
 * @brief   This method sets the raid group debug
 *          flags to the value passed for the associated raid group.
 *
 * @param   raid_group_p - The raid group.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_set_raid_group_debug_flags(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_raid_group_raid_group_debug_payload_t *set_debug_flags_p = NULL;
    fbe_raid_group_debug_flags_t  old_debug_flags;
    
    /* Get old debug flags.
     */
    fbe_raid_group_get_debug_flags(raid_group_p, &old_debug_flags);

    /* Now retrieve the value of the debug flags to set from the buffer.
     */
    fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                              packet_p, 
                                              sizeof(fbe_raid_group_raid_group_debug_payload_t),
                                              (fbe_payload_control_buffer_t)&set_debug_flags_p);

    if (set_debug_flags_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "rg_set_raid_group_debug_flags null buffer.\n");
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        /* Base object trace will trace the object id etc.
         */
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "rg_set_raid_group_debug_flags: FBE_RAID_GROUP_CONTROL_CODE_SET_RAID_GROUP_DEBUG_FLAGS receive\n");
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                        FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                        "\t Old raid group debug flags: 0x%08x new: 0x%08x \n", 
                        old_debug_flags, set_debug_flags_p->raid_group_debug_flags);

        /* Lock the raid group object to syncrounize udpate.
         */
        fbe_raid_group_lock(raid_group_p);
        fbe_raid_group_set_debug_flags(raid_group_p,
                                       set_debug_flags_p->raid_group_debug_flags);
        fbe_raid_group_unlock(raid_group_p);
        status = FBE_STATUS_OK;
    }

    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_raid_group_set_raid_group_debug_flags()
 **************************************************/

/*!**************************************************************
 * fbe_raid_group_usurper_get_control_buffer()
 ****************************************************************
 * @brief
 *  This function gets the buffer pointer out of the packet
 *  and validates it. It returns error status if any of the
 *  pointer validations fail or the buffer size doesn't match
 *  the specifed buffer length.
 *
 * @param in_raid_group_p   - Pointer to the raid group.
 * @param in_packet_p       - Pointer to the packet.
 * @param in_buffer_length  - Expected length of the buffer.
 * @param out_buffer_pp     - Pointer to the buffer pointer. 
 *
 * @return status - The status of the operation. 
 *
 * @author
 *  10/30/2009 - Created. mvalois
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_usurper_get_control_buffer(
                                        fbe_raid_group_t*                   in_raid_group_p,
                                        fbe_packet_t*                       in_packet_p,
                                        fbe_payload_control_buffer_length_t in_buffer_length,
                                        fbe_payload_control_buffer_t*       out_buffer_pp)
{
    fbe_payload_ex_t*                  payload_p;
    fbe_payload_control_operation_t*    control_operation_p;  
    fbe_payload_control_buffer_length_t buffer_length;

    *out_buffer_pp = NULL;
    // Get the control op buffer data pointer from the packet payload.
    payload_p = fbe_transport_get_payload_ex(in_packet_p);
    if (payload_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    if (control_operation_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s control operation is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation_p, out_buffer_pp);
    if (*out_buffer_pp == NULL)
    {
        // The buffer pointer is NULL!
        fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s buffer pointer is NULL\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    };

    fbe_payload_control_get_buffer_length(control_operation_p, &buffer_length);
    if (buffer_length != in_buffer_length)
    {
        // The buffer length doesn't match!
        fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s expected len: %d actual len: %d\n", 
                              __FUNCTION__, in_buffer_length, buffer_length);
        *out_buffer_pp = NULL;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;

} //    End fbe_raid_group_usurper_get_control_buffer()

/*!**************************************************************
 * fbe_raid_group_initialize_power_saving()
 ****************************************************************
 * @brief
 *  initialize power saving related information
 *
 * @param in_raid_group_p   - Pointer to the raid group.
 * @param set_config_p       - Pointer to the structure that has the info
 
 * @return status - The status of the operation. 
 *
 * @author
 *  04/29/2010 - Created. sharel
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_initialize_power_saving(fbe_raid_group_t *raid_group_p, fbe_raid_group_configuration_t * set_config_p)
{
    fbe_status_t    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_class_id_t                          my_class_id = FBE_CLASS_ID_INVALID;
    fbe_raid_geometry_t *                   raid_geometry_p = NULL;
    fbe_raid_group_type_t                   raid_type;

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);

    /*now, let's let the lun know about it but now if we are a VD*/
    my_class_id = fbe_base_object_get_class_id(fbe_base_pointer_to_handle((fbe_base_t *) raid_group_p));
    if (my_class_id == FBE_CLASS_ID_VIRTUAL_DRIVE) {
        return FBE_STATUS_OK;/*no need to touch*/
    }

    status = fbe_base_config_set_power_saving_idle_time((fbe_base_config_t *)raid_group_p, set_config_p->power_saving_idle_time_in_seconds); 
    if (status != FBE_STATUS_OK) {
        return status;
    }

    if (!set_config_p->power_saving_enabled) {
        status = base_config_disable_object_power_save((fbe_base_config_t *)raid_group_p);
    }else{
        status = fbe_base_config_enable_object_power_save((fbe_base_config_t *)raid_group_p);
    }

    fbe_raid_group_set_max_latency_time(raid_group_p, set_config_p->max_raid_latency_time_in_sec);

    /*let lun know about our changes (if there is any lun there of course)*/
    my_class_id = fbe_base_object_get_class_id(fbe_base_pointer_to_handle((fbe_base_t *) raid_group_p));
    if (my_class_id != FBE_CLASS_ID_VIRTUAL_DRIVE && raid_type != FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER) {
        fbe_block_transport_server_power_saving_config_changed(&raid_group_p->base_config.block_transport_server);
    }

    return status;
                 
} //    End fbe_raid_group_initialize_power_saving()


/*!**************************************************************
 * fbe_raid_group_get_power_saving_parameters()
 ****************************************************************
 * @brief
 *  returns the power saving related parameters the user configured in RG
 *
 * @param in_raid_group_p   - Pointer to the raid group.
 * @param in_packet_p       - Pointer to packet
 
 * @return status - The status of the operation. 
 *
 * @author
 *  05/03/2010 - Created. sharel
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_get_power_saving_parameters(fbe_raid_group_t* in_raid_group_p, fbe_packet_t *in_packet_p)
{
    fbe_raid_group_get_power_saving_info_t *    get_power_save_info;
    fbe_status_t                                status;


    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);

    // Get the control buffer pointer from the packet payload.
    status = fbe_raid_group_usurper_get_control_buffer(in_raid_group_p, in_packet_p, 
                                            sizeof(fbe_raid_group_get_power_saving_info_t),
                                            (fbe_payload_control_buffer_t)&get_power_save_info);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(in_packet_p, status, 0);
        fbe_transport_complete_packet(in_packet_p);
        return status; 
    }

    /*populate the information the user asked for*/
    fbe_base_config_get_power_saving_idle_time((fbe_base_config_t *)in_raid_group_p,
                                               &get_power_save_info->idle_time_in_sec);

    get_power_save_info->max_raid_latency_time_is_sec = in_raid_group_p->max_raid_latency_time_is_sec;
    get_power_save_info->power_saving_enabled = base_config_is_object_power_saving_enabled((fbe_base_config_t *)in_raid_group_p);
    
    fbe_transport_set_status(in_packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(in_packet_p);

    return FBE_STATUS_OK;

}//    End fbe_raid_group_get_power_saving_parameters()

/*!**************************************************************
 * fbe_raid_group_change_power_save_idle_time()
 ****************************************************************
 * @brief
 *  change the idle time before hibernation
 *
 * @param in_raid_group_p   - Pointer to the raid group.
 * @param in_packet_p       - Pointer to packet
 
 * @return status - The status of the operation. 
 *
 * @author
 *  05/03/2010 - Created. sharel
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_change_power_save_idle_time(fbe_raid_group_t* in_raid_group_p, fbe_packet_t *in_packet_p)
{
    fbe_base_config_set_object_idle_time_t *    set_idle_time = NULL;    /* INPUT */
    fbe_status_t                                status;
    fbe_class_id_t                              my_class_id = FBE_CLASS_ID_INVALID;
    fbe_raid_geometry_t *                       raid_geometry_p = NULL;
    fbe_raid_group_type_t                       raid_type;

    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(in_raid_group_p);

    // Get the control buffer pointer from the packet payload.
    status = fbe_raid_group_usurper_get_control_buffer(in_raid_group_p, in_packet_p, 
                                            sizeof(fbe_base_config_set_object_idle_time_t),
                                            (fbe_payload_control_buffer_t)&set_idle_time);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(in_packet_p, status, 0);
        fbe_transport_complete_packet(in_packet_p);
        return status; 
    }

    /*first of all, let's set up the idle time in the base config*/
    fbe_base_config_set_power_saving_idle_time((fbe_base_config_t *)in_raid_group_p, set_idle_time->power_save_idle_time_in_seconds);

    /*now, let's let the lun know about it but now if we are a VD*/
    my_class_id = fbe_base_object_get_class_id(fbe_base_pointer_to_handle((fbe_base_t *) in_raid_group_p));
    if (my_class_id != FBE_CLASS_ID_VIRTUAL_DRIVE && raid_type != FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER) {
        fbe_block_transport_server_power_saving_config_changed(&in_raid_group_p->base_config.block_transport_server);
    }
   
    fbe_transport_set_status(in_packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(in_packet_p);

    return FBE_STATUS_OK;

}//    End fbe_raid_group_change_power_save_idle_time()

/*!**************************************************************
 * fbe_raid_group_enable_object_power_save()
 ****************************************************************
 * @brief
 *  enable power saving on rg and propagate to lu
 *
 * @param in_raid_group_p   - Pointer to the raid group.
 * @param in_packet_p       - Pointer to packet
 
 * @return status - The status of the operation. 
 *
 * @author
 *  05/04/2010 - Created. sharel
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_enable_object_power_save(fbe_raid_group_t* in_raid_group_p, fbe_packet_t *in_packet_p)
{
    fbe_class_id_t                              my_class_id = FBE_CLASS_ID_INVALID;
    fbe_raid_geometry_t *                       raid_geometry_p = NULL;
    fbe_raid_group_type_t                       raid_type;

    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);

    /*first of all, let's set up the  base config*/
    fbe_base_config_enable_object_power_save((fbe_base_config_t *)in_raid_group_p);

    /*now, let's let the lun know about it but now if we are a VD*/
    my_class_id = fbe_base_object_get_class_id(fbe_base_pointer_to_handle((fbe_base_t *) in_raid_group_p));
    if (my_class_id != FBE_CLASS_ID_VIRTUAL_DRIVE && raid_type != FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER) {
        fbe_block_transport_server_power_saving_config_changed(&in_raid_group_p->base_config.block_transport_server);
    }

    fbe_transport_set_status(in_packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(in_packet_p);

    return FBE_STATUS_OK;
}//    End fbe_raid_group_enable_object_power_save()

/*!**************************************************************
 * fbe_raid_group_disable_object_power_save()
 ****************************************************************
 * @brief
 *  disable power saving on rg and propagate to lu
 *
 * @param in_raid_group_p   - Pointer to the raid group.
 * @param in_packet_p       - Pointer to packet
 
 * @return status - The status of the operation. 
 *
 * @author
 *  05/04/2010 - Created. sharel
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_disable_object_power_save(fbe_raid_group_t* in_raid_group_p, fbe_packet_t *in_packet_p)
{
    fbe_class_id_t                              my_class_id = FBE_CLASS_ID_INVALID;
    fbe_raid_geometry_t *                       raid_geometry_p = NULL;
    fbe_raid_group_type_t                       raid_type;

    raid_geometry_p = fbe_raid_group_get_raid_geometry(in_raid_group_p);
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);

    /*first of all, let's set up base config*/
    fbe_base_config_disable_object_power_save((fbe_base_config_t *)in_raid_group_p);

    /*now, let's let the lun know about it but now if we are a VD*/
    my_class_id = fbe_base_object_get_class_id(fbe_base_pointer_to_handle((fbe_base_t *) in_raid_group_p));
    if (my_class_id != FBE_CLASS_ID_VIRTUAL_DRIVE && raid_type != FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER) {
        fbe_block_transport_server_power_saving_config_changed(&in_raid_group_p->base_config.block_transport_server);
    }

    fbe_transport_set_status(in_packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(in_packet_p);

    return FBE_STATUS_OK;
}//    End fbe_raid_group_disable_object_power_save()

/*!**************************************************************
 * fbe_raid_group_get_lun_align_size()
 ****************************************************************
 * @brief
 *  This function calcuates the alignment size for the lun.
 *
 * @param raid_group_p - The raid group.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  4/27/2010 - Created. guov
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_get_lun_align_size(fbe_raid_group_t * raid_group_p, fbe_lba_t * lun_align_size_p)
{
    fbe_status_t          status = FBE_STATUS_GENERIC_FAILURE;
    fbe_chunk_size_t      chunk_size;
    fbe_raid_geometry_t * raid_geometry_p;
    fbe_u16_t             data_disks;

    if (lun_align_size_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s lun_align_size null buffer.\n", __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
        return status;
    }
    /* Get the chunk size */
    chunk_size = fbe_raid_group_get_chunk_size(raid_group_p);

    // Get the number of data disks in this raid object
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    status = fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);

    *lun_align_size_p = (fbe_lba_t)data_disks * chunk_size;
    return status;
}
/**************************************
 * end fbe_raid_group_get_lun_align_size()
 **************************************/

/*!****************************************************************************
 * fbe_raid_group_calculate_zero_checkpoint_for_raid_extent()
 ******************************************************************************
 * @brief
 *  This function is used to calculate the zero checkpoint for LUN.
 *
 * @return status       - status of the operation.
 *
 * @author
 *  10/11/2010 - Created. Dhaval Patel
 *  05/16/2011 - Modified Sandeep Chaudhari
 *  07/12/2012 - Modified Prahlad Purohit
 *
 ******************************************************************************/
static fbe_status_t
fbe_raid_group_calculate_zero_checkpoint_for_raid_extent(fbe_raid_group_t * raid_group_p,
                                                         fbe_raid_group_get_zero_checkpoint_downstream_object_t * zero_checkpoint_downstream_object_p,
                                                         fbe_raid_group_get_zero_checkpoint_raid_extent_t * get_zero_checkpoint_p)
{
    fbe_block_edge_t *      block_edge_p = NULL;
    fbe_lba_t               rg_edge_offset = 0;
    fbe_lba_t               zero_checkpoint = 0;
    fbe_u16_t               data_disks;
    fbe_u32_t               width;
    fbe_u32_t               index = 0;
    fbe_raid_geometry_t *   raid_geometry_p = NULL;
    fbe_lba_t               lun_cap_per_disk = 0;
    fbe_lba_t               lun_offset_on_disk;
    fbe_lba_t               lun_capacity;
  
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* get the data disk for raid group object. */
    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &data_disks);


    /* initialize the checkpoint as zero. */
    get_zero_checkpoint_p->zero_checkpoint = 0;

    /* get the lun offset and capacity sent by lun */
    lun_offset_on_disk = get_zero_checkpoint_p->lun_offset/data_disks;
    lun_capacity = get_zero_checkpoint_p->lun_capacity;

    /* find the per disk lun capacity */
    lun_cap_per_disk = lun_capacity / data_disks;

    /* loop through RG width and normalize the lu extent zero checkpoint */
    for(index = 0; index < width; index++)
    {
        /* get the edge offset for the raid extent. */
        /*!@todo ideally we should traverse the block transport edge till pvd to
         * calculate the edege offset.
         */
        fbe_base_config_get_block_edge((fbe_base_config_t *) raid_group_p, &block_edge_p, index);
        rg_edge_offset = fbe_block_transport_edge_get_offset(block_edge_p);    

        if(zero_checkpoint_downstream_object_p->zero_checkpoint[index] <= (rg_edge_offset + lun_offset_on_disk))
        {
            /*Zeroing is not started to actual LUN area so nothing to add */
            continue;
        }

        if(zero_checkpoint_downstream_object_p->zero_checkpoint[index] > (rg_edge_offset + lun_offset_on_disk + lun_cap_per_disk))
        {
            /* zeroing get completed on entire lun space for this disk so add lun capacity per disk */
            zero_checkpoint += lun_cap_per_disk;
        }
        else
        {
            /* disk zero checkpoint is somewhere in between LUN area on this disk */
            zero_checkpoint = zero_checkpoint + (zero_checkpoint_downstream_object_p->zero_checkpoint[index] - (rg_edge_offset + lun_offset_on_disk));
        }

    }
    

    /* Above loop considered all disks, scale the LU zero checkpoint for data disks.
     */
    get_zero_checkpoint_p->zero_checkpoint = (zero_checkpoint * data_disks) / width;

    return FBE_STATUS_OK;
}


/*!****************************************************************************
 * fbe_raid_group_calculate_zero_checkpoint_for_raid10_extent()
 ******************************************************************************
 * @brief
 *  This function is used to calculate the zero checkpoint for R1_0.
 *  
 *
 * @return status       - status of the operation.
 *
 * @author
 *  10/11/2010 - Created. Dhaval Patel
 *  05/16/2011 - Modified Sandeep Chaudhari
 *  07/12/2012 - Modified Prahlad Purohit
 *
 ******************************************************************************/
static fbe_status_t
fbe_raid_group_calculate_zero_checkpoint_for_raid10_extent(fbe_raid_group_t * raid_group_p,
                                                         fbe_raid_group_get_zero_checkpoint_downstream_object_t * zero_checkpoint_downstream_object_p,
                                                         fbe_raid_group_get_zero_checkpoint_raid_extent_t * get_zero_checkpoint_p)
{

    fbe_lba_t               zero_checkpoint = 0;
    fbe_u32_t               width;
    fbe_u32_t               index = 0;

    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    for(index = 0; index < width; index++)
    {
        /* zero checkpoint get normalize at mirror RG level          
          cumulative the zero checkpoint received from mirror objects */
        zero_checkpoint += zero_checkpoint_downstream_object_p->zero_checkpoint[index];
    }

    get_zero_checkpoint_p->zero_checkpoint = zero_checkpoint;

    return FBE_STATUS_OK;
}


/*!****************************************************************************
 * fbe_raid_group_get_zero_checkpoint_raid_extent()
 ******************************************************************************
 * @brief
 *  This function is used to get the zero checkpoint for the raid group extent,
 *  it allocates new control operation to track the checkpoint for all the 
 *  downstream edges and later it calculates minimum checkpoint from all the
 *  edges to report the zeroing status..
 *
 * @param raid_group_p  - raid group object.
 * @param packet_p      - Pointer to the packet.
 *
 * @return status       - status of the operation.
 *
 * @author
 *  10/11/2010 - Created. Dhaval Patel
 *  06/23/2011 - Modified Sandeep Chaudhari
 ******************************************************************************/
fbe_status_t
fbe_raid_group_get_zero_checkpoint_raid_extent(fbe_raid_group_t * raid_group_p,
                                               fbe_packet_t * packet_p)
{
    fbe_status_t                                                status;
    fbe_raid_group_get_zero_checkpoint_raid_extent_t *          zero_checkpoint_raid_extent_p = NULL;
    fbe_raid_group_get_zero_checkpoint_downstream_object_t *    zero_checkpoint_downstream_object_p = NULL;
    fbe_payload_ex_t *                                          sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           control_operation_p = NULL;

    /* get the sep payload and control operation/ */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the control buffer pointer from the packet payload. */
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                                       packet_p, 
                                                       sizeof(fbe_raid_group_get_zero_checkpoint_raid_extent_t),
                                                       (fbe_payload_control_buffer_t *)&zero_checkpoint_raid_extent_p);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;  
    }

    /* initialize the raid extent zero checkpoint as zero. */
    zero_checkpoint_raid_extent_p->zero_checkpoint = 0;

    /* allocate the buffer to send the raid group zero percent raid extent usurpur. */
    zero_checkpoint_downstream_object_p = 
        (fbe_raid_group_get_zero_checkpoint_downstream_object_t *) fbe_transport_allocate_buffer();
    if (zero_checkpoint_downstream_object_p == NULL)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* allocate the new control operation to buil. */
    control_operation_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);
    fbe_payload_control_build_operation(control_operation_p,
                                        FBE_RAID_GROUP_CONTROL_CODE_GET_ZERO_CHECKPOINT_FOR_DOWNSTREAM_OBJECT,
                                        zero_checkpoint_downstream_object_p,
                                        sizeof(fbe_raid_group_get_zero_checkpoint_downstream_object_t));

    fbe_payload_ex_increment_control_operation_level(sep_payload_p);

    fbe_transport_set_completion_function(packet_p,
                                          fbe_raid_group_get_zero_checkpoint_raid_extent_completion,
                                          raid_group_p);

    /* call the function to get the zero percentage for raid extent. */
    fbe_raid_group_get_zero_checkpoint_for_downstream_objects(raid_group_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_raid_group_get_zero_checkpoint_raid_extent()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_get_zero_checkpoint_raid_extent_completion()
 ******************************************************************************
 * @brief
 *  This function is used to update the zero checkpoint of the raid extent 
 *  from the downstream edges checkpoint.
 *
 * @param raid_group_p  - raid group object.
 * @param packet_p      - Pointer to the packet.
 *
 * @return status       - status of the operation.
 *
 * @author
 *  10/11/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t
fbe_raid_group_get_zero_checkpoint_raid_extent_completion(fbe_packet_t * packet_p,
                                                          fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                                              raid_group_p = NULL;
    fbe_payload_ex_t *                                             sep_payload_p = NULL;
    fbe_payload_control_operation_t *                               control_operation_p = NULL;
    fbe_payload_control_operation_t *                               prev_control_operation_p = NULL;
    fbe_payload_control_status_t                                    control_status;
    fbe_status_t                                                    status;
    fbe_raid_group_get_zero_checkpoint_raid_extent_t *              zero_checkpoint_raid_extent_p = NULL;
    fbe_raid_group_get_zero_checkpoint_downstream_object_t *        zero_checkpoint_downstream_object_p = NULL;
    fbe_raid_geometry_t*            raid_geometry_p = NULL;   
    fbe_raid_group_type_t           raid_type;    

    /* Cast the context to base config object pointer. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &zero_checkpoint_downstream_object_p);

    /* get the previous control operation. */
    prev_control_operation_p = fbe_payload_ex_get_prev_control_operation(sep_payload_p);
    fbe_payload_control_get_buffer(prev_control_operation_p, &zero_checkpoint_raid_extent_p);

    /* get the status of the subpacket operation. */
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    /* if any of the subpacket failed with bad status then update the master status. */
    if((status != FBE_STATUS_OK) ||
       (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        /* release the allocated buffer and current control operation. */
        fbe_transport_release_buffer(zero_checkpoint_downstream_object_p);
        fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_payload_control_set_status(prev_control_operation_p, control_status);
        return status;
    }

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);                  
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);                      

    if(raid_type == FBE_RAID_GROUP_TYPE_RAID10)
    {
        fbe_raid_group_calculate_zero_checkpoint_for_raid10_extent(raid_group_p,
                                                             zero_checkpoint_downstream_object_p,
                                                             zero_checkpoint_raid_extent_p);
    }
    else
    {

        /* calculate the zero checkpoint and percent for rg extent. */
        fbe_raid_group_calculate_zero_checkpoint_for_raid_extent(raid_group_p,
                                                                 zero_checkpoint_downstream_object_p,
                                                                 zero_checkpoint_raid_extent_p);
    }

    /* release the allocated buffer and control operation. */
    fbe_transport_release_buffer(zero_checkpoint_downstream_object_p);
    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);

    /* complete the packet with good status. */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_payload_control_set_status(prev_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_get_zero_percent_for_lun_extent_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_get_zero_checkpoint_for_downstream_objects()
 ******************************************************************************
 * @brief
 *  This function is used to issue the request to all the downstream edges to
 *  gather the zero status.
 *
 * @param raid_group_p  - raid group object.
 * @param packet_p      - Pointer to the packet.
 *
 * @return status       - status of the operation.
 *
 * @author
 *  10/11/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_raid_group_get_zero_checkpoint_for_downstream_objects(fbe_raid_group_t * raid_group_p,
                                                          fbe_packet_t * packet_p)
{
    fbe_status_t                                                status = FBE_STATUS_OK;
    fbe_raid_group_get_zero_checkpoint_downstream_object_t *    zero_checkpoint_downstream_object_p = NULL;
    fbe_u32_t                                                   width = 0;
    fbe_payload_ex_t *                                         sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           control_operation_p = NULL;
    fbe_u32_t                                                   index = 0;
    fbe_base_config_memory_allocation_chunks_t                  mem_alloc_chunks = {0};

    /* get the sep payload and control operation/ */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the control buffer pointer from the packet payload. */
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                                       packet_p, 
                                                       sizeof(fbe_raid_group_get_zero_checkpoint_downstream_object_t),
                                                       (fbe_payload_control_buffer_t)&zero_checkpoint_downstream_object_p);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    /* initialize the checkpoint to zero before sending the request down. */
    for(index = 0; index < FBE_RAID_MAX_DISK_ARRAY_WIDTH; index++)
    {
        zero_checkpoint_downstream_object_p->zero_checkpoint[index] = 0;
    }

    /* get the width of the raid group object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* Set the various counts and buffer size */
    mem_alloc_chunks.buffer_count = width;
    mem_alloc_chunks.number_of_packets = width;
    mem_alloc_chunks.sg_element_count = 0;
    mem_alloc_chunks.buffer_size = sizeof(fbe_provision_drive_get_zero_checkpoint_t) + sizeof(fbe_edge_index_t);
    mem_alloc_chunks.alloc_pre_read_desc = FALSE;
    
    /* allocate the subpackets to collect the zero checkpoint for all the edges. */
    status = fbe_base_config_memory_allocate_chunks((fbe_base_config_t *) raid_group_p,
                                                    packet_p,
                                                    mem_alloc_chunks,
                                                    (fbe_memory_completion_function_t)fbe_raid_group_get_zero_checkpoint_downstream_object_allocation_completion); /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s memory allocation failed, status:0x%x\n", __FUNCTION__, status);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }
    return status;
}
/******************************************************************************
 * end fbe_raid_group_get_zero_checkpoint_for_downstream_objects()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_get_zero_checkpoint_downstream_object_allocation_completion()
 ******************************************************************************
 * @brief
 *  This function is used to get the zero checkpoint for all the downstream
 *  objects.
 *
 * @param memory_request_p  - memory request.
 * @param context           - context passed to acquire the memory.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  10/11/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_raid_group_get_zero_checkpoint_downstream_object_allocation_completion(fbe_memory_request_t * memory_request_p,
                                                                           fbe_memory_completion_context_t context)
{
    fbe_packet_t *                                  packet_p = NULL;
    fbe_packet_t **                                 new_packet_p = NULL;
    fbe_u8_t **                                     buffer_p = NULL;
    fbe_payload_ex_t *                              sep_payload_p = NULL;
    fbe_payload_ex_t *                              new_sep_payload_p = NULL;
    fbe_payload_control_operation_t *               control_operation_p = NULL;
    fbe_payload_control_operation_t *               new_control_operation_p = NULL;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_provision_drive_get_zero_checkpoint_t *     pvd_zero_checkpoint_p = NULL;
    fbe_raid_group_t *                              raid_group_p = NULL;
    fbe_u32_t                                       width = 0;
    fbe_edge_index_t *                              edge_index_p = NULL;
    fbe_u32_t                                       index = 0;
    fbe_block_edge_t *                              block_edge_p = NULL;
    fbe_base_config_memory_alloc_chunks_address_t   mem_alloc_chunks_addr = {0};
    fbe_sg_element_t                               *pre_read_sg_list_p = NULL;
    fbe_payload_pre_read_descriptor_t              *pre_read_desc_p = NULL;
    fbe_packet_attr_t                               packet_attr;


    /* get the pointer to original packet. */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);

    /* get the pointer to raid group object. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* get the payload and metadata operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p =  fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: memory request: 0x%p failed. request state: %d \n",
                              __FUNCTION__, memory_request_p, memory_request_p->request_state);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_FAILED;
    }

    /* get the width of the base config object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* subpacket control buffer size. */
    mem_alloc_chunks_addr.buffer_size = sizeof(fbe_provision_drive_get_zero_checkpoint_t) + sizeof(fbe_edge_index_t);
    mem_alloc_chunks_addr.number_of_packets = width;
    mem_alloc_chunks_addr.buffer_count = width;
    mem_alloc_chunks_addr.sg_element_count = 0;
    mem_alloc_chunks_addr.sg_list_p = NULL;
    mem_alloc_chunks_addr.assign_pre_read_desc = FALSE;
    mem_alloc_chunks_addr.pre_read_sg_list_p = &pre_read_sg_list_p;
    mem_alloc_chunks_addr.pre_read_desc_p = &pre_read_desc_p;
    
    /* get the zod sgl pointer and and subpacket pointer from the allocated memory. */
    status = fbe_base_config_memory_assign_address_from_allocated_chunks((fbe_base_config_t *) raid_group_p,
                                                                         memory_request_p,
                                                                         &mem_alloc_chunks_addr);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s memory distribution failed, status:0x%x, width:0x%x, buffer_size:0x%x\n",
                              __FUNCTION__, status, width, mem_alloc_chunks_addr.buffer_size);
        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(memory_request_p);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_INSUFFICIENT_RESOURCES);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    new_packet_p = mem_alloc_chunks_addr.packet_array_p;
    buffer_p = mem_alloc_chunks_addr.buffer_array_p;
     
    while(index < width)
    {
        /* Initialize the sub packets. */
        fbe_transport_initialize_sep_packet(new_packet_p[index]);
        new_sep_payload_p = fbe_transport_get_payload_ex(new_packet_p[index]);

        /* initialize the control buffer. */
        pvd_zero_checkpoint_p = (fbe_provision_drive_get_zero_checkpoint_t *) buffer_p[index];
        pvd_zero_checkpoint_p->zero_checkpoint = 0;

        /* initialize the edge index as a context data. */
        edge_index_p = (fbe_edge_index_t *) (buffer_p[index] + sizeof(fbe_provision_drive_get_zero_checkpoint_t));
        *edge_index_p = index;

        /* allocate and initialize the control operation. */
        new_control_operation_p = fbe_payload_ex_allocate_control_operation(new_sep_payload_p);
        fbe_payload_control_build_operation(new_control_operation_p,
                                            FBE_PROVISION_DRIVE_CONTROL_CODE_GET_ZERO_CHECKPOINT,
                                            pvd_zero_checkpoint_p,
                                            sizeof(fbe_provision_drive_get_zero_checkpoint_t));

        /* Initialize the new packet with FBE_PACKET_FLAG_EXTERNAL same as the master packet */
        fbe_transport_get_packet_attr(packet_p, &packet_attr);
        if (packet_attr & FBE_PACKET_FLAG_EXTERNAL) 
        {
            fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_EXTERNAL);            
        }

        /* set completion functon. */
        fbe_transport_set_completion_function(new_packet_p[index],
                                              fbe_raid_group_get_zero_checkpoint_downstream_object_subpacket_completion,
                                              raid_group_p);

        /* add the subpacket to master packet queue. */
        fbe_transport_add_subpacket(packet_p, new_packet_p[index]);
        index++;
    }

    /* send all the subpackets together. */
    for(index = 0; index < width; index++)
    {
        fbe_base_config_get_block_edge((fbe_base_config_t *)raid_group_p, &block_edge_p, index);
        fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_TRAVERSE);
        fbe_block_transport_send_control_packet(block_edge_p, new_packet_p[index]);
    }

    /* send all the subpackets. */
     return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_raid_group_get_zero_checkpoint_downstream_object_allocation_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_get_zero_checkpoint_downstream_object_subpacket_completion()
 ******************************************************************************
 * @brief
 *  This function is used to handle the subpacket completion to get the zero
 *  checkpoint for all the downstream objects.
 *
 * @param memory_request_p  - memory request.
 * @param context           - context passed to acquire the memory.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  10/11/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_get_zero_checkpoint_downstream_object_subpacket_completion(fbe_packet_t * packet_p,
                                                                          fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                                          raid_group_p = NULL;
    fbe_packet_t *                                              master_packet_p = NULL;
    fbe_payload_ex_t *                                         master_sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           master_control_operation_p = NULL;
    fbe_payload_ex_t *                                         sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           control_operation_p = NULL;
    fbe_payload_control_status_t                                control_status;
    fbe_payload_control_status_qualifier_t                      control_qualifier;
    fbe_status_t                                                status;
    fbe_raid_group_get_zero_checkpoint_downstream_object_t *    zero_checkpoint_downstream_object_p = NULL;
    fbe_provision_drive_get_zero_checkpoint_t *                 pvd_zero_checkpoint_p = NULL;
    fbe_edge_index_t *                                          edge_index_p = NULL;
    fbe_bool_t is_empty;

    /* Cast the context to base config object pointer. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* Get the master packet, its payload and associated control operation. */
    master_packet_p = fbe_transport_get_master_packet(packet_p);
    master_sep_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    master_control_operation_p = fbe_payload_ex_get_control_operation(master_sep_payload_p);

    /* get the control buffer of the master packet. */
    fbe_payload_control_get_buffer(master_control_operation_p, &zero_checkpoint_downstream_object_p);

    /* get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the status of the subpacket operation. */
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation_p, &control_qualifier);

    /* if any of the subpacket failed with bad status then update the master status. */
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(master_packet_p, status, 0);
    }

    /* if any of the subpacket failed with failure status then update the master status. */
    /*!@todo we can use error predence here if required. */
    if(control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_payload_control_set_status(master_control_operation_p, control_status);
        fbe_payload_control_set_status_qualifier(master_control_operation_p, control_qualifier);
    }

    if((status == FBE_STATUS_OK) &&
       (control_status == FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        /* get the control buffer of the master packet. */
        fbe_payload_control_get_buffer(control_operation_p, &pvd_zero_checkpoint_p);

        /* get the edge index of the subpacket. */
        edge_index_p = (fbe_edge_index_t *) (((fbe_u8_t *) pvd_zero_checkpoint_p) + sizeof(fbe_provision_drive_get_zero_checkpoint_t));

        /* update the master packet with zero checkpoint. */
        zero_checkpoint_downstream_object_p->zero_checkpoint[*edge_index_p] = pvd_zero_checkpoint_p->zero_checkpoint;
    }

    /* remove the sub packet from master queue. */
    //fbe_transport_remove_subpacket(packet_p);
    fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);

    /* destroy the subpacket. */
    //fbe_transport_destroy_packet(packet_p);

    /* when the queue is empty, all subpackets have completed. */

    if(is_empty) {
        fbe_transport_destroy_subpackets(master_packet_p);
        //fbe_memory_request_get_entry_mark_free(&master_packet_p->memory_request, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(&master_packet_p->memory_request);

        fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
		// Fix for AR 541516
        fbe_payload_control_set_status (master_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_complete_packet(master_packet_p);
    }
    else
    {
        /* Not done with processing sub packets.
         */
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}

/*!**************************************************************
 * fbe_raid_group_allocate_and_build_control_operation()
 ****************************************************************
 * @brief
 *  This function builds a control operation in the packet.
 *  It returns error status if the payload pointer is invalid or 
 *  a control operation cannot be allocated.
 *
 * @param in_lun_p          - Pointer to the lun.
 * @param in_packet_p       - Pointer to the packet.
 * @param in_op_code        - Operation code
 * @param in_buffer_p       - Pointer to the buffer. 
 * @param in_buffer_length  - Expected length of the buffer.
 *
 * @return status - The status of the operation. 
 *
 * @author
 *  02/26/2011 - Created. Ashwin Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_allocate_and_build_control_operation(
                                fbe_raid_group_t*                       in_raid_group_p,
                                fbe_packet_t*                           in_packet_p,
                                fbe_payload_control_operation_opcode_t  in_op_code,
                                fbe_payload_control_buffer_t            in_buffer_p,
                                fbe_payload_control_buffer_length_t     in_buffer_length)
{
    fbe_payload_ex_t*                  payload_p;
    fbe_payload_control_operation_t*    control_operation_p;


    // Get the control op buffer data pointer from the packet payload.
    payload_p = fbe_transport_get_payload_ex(in_packet_p);
    if (payload_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE NULL payload pointer line %d", __FUNCTION__, __LINE__);
        fbe_transport_set_status(in_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(in_packet_p);
        return FBE_STATUS_OK;
    }

    control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);
    if (control_operation_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)in_raid_group_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s FBE NULL control op pointer line %d", __FUNCTION__, __LINE__);
        fbe_transport_set_status(in_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(in_packet_p);
        return FBE_STATUS_OK;
    }

    // Build the control operation.
    fbe_payload_control_build_operation(control_operation_p, in_op_code, in_buffer_p, in_buffer_length);

    return FBE_STATUS_OK;

}   // End fbe_raid_group_allocate_and_build_control_operation()
/*!**************************************************************
 * fbe_raid_group_control_metadata_paged_operation()
 ****************************************************************
 * @brief
 *  Handle a paged metadata usurper.
 *  Raid needs to calculate the stripe lock range and then
 *  send it to the paged metadata service.
 *
 * @param raid_group_p - Current rg.
 * @param packet_p - Packet for this usurper.
 *
 * @return fbe_status_t
 *
 * @author
 *  5/18/2011 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t
fbe_raid_group_control_metadata_paged_operation(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_payload_metadata_operation_t * metadata_operation_p = NULL;
    fbe_payload_control_operation_opcode_t opcode;
    fbe_payload_control_buffer_t * buffer_p = NULL;
    fbe_raid_geometry_t *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_lba_t           metadata_start_lba;
    fbe_block_count_t   metadata_block_count;
    fbe_u64_t           metadata_stripe_number;
    fbe_u64_t           metadata_stripe_count;
    fbe_u8_t * metadata_record_data = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    fbe_base_object_trace( (fbe_base_object_t *)raid_group_p,
                           FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s entry\n", __FUNCTION__);

    fbe_payload_control_get_buffer(control_operation_p, &buffer_p);
    if (buffer_p == NULL) 
    {
        fbe_base_object_trace( (fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate the paged metadata request*/
    metadata_operation_p =  fbe_payload_ex_allocate_metadata_operation(sep_payload_p);
    if (metadata_operation_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s unable to allocate metadata operation\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_opcode(control_operation_p, &opcode);

    if (opcode == FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_SET_BITS)
    {
        fbe_base_config_control_metadata_paged_change_bits_t * change_bits = NULL;
        fbe_payload_control_get_buffer(control_operation_p, &change_bits);
 
        /* Always trace a set bits request.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: usurper paged metadata set bits packet: 0x%x offset: %lld data[0]: 0x%x size: %d \n",
                              (fbe_u32_t)packet_p, change_bits->metadata_offset, 
                              *((fbe_u32_t *)&change_bits->metadata_record_data[0]),
                              change_bits->metadata_record_data_size);

        status = fbe_payload_metadata_build_paged_set_bits(metadata_operation_p, 
                                                           &raid_group_p->base_config.metadata_element, 
                                                           change_bits->metadata_offset,
                                                           change_bits->metadata_record_data,
                                                           change_bits->metadata_record_data_size,
                                                           change_bits->metadata_repeat_count);
        metadata_record_data = &change_bits->metadata_record_data[0];

    }
    else if (opcode == FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_CLEAR_BITS)
    {
        fbe_base_config_control_metadata_paged_change_bits_t * change_bits = NULL;
        fbe_payload_control_get_buffer(control_operation_p, &change_bits);

        status = fbe_payload_metadata_build_paged_clear_bits(metadata_operation_p, 
                                                             &raid_group_p->base_config.metadata_element, 
                                                             change_bits->metadata_offset,
                                                             change_bits->metadata_record_data,
                                                             change_bits->metadata_record_data_size,
                                                             change_bits->metadata_repeat_count);
        metadata_record_data = &change_bits->metadata_record_data[0];
    }
    else if (opcode == FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_GET_BITS)
    {
        fbe_base_config_control_metadata_paged_get_bits_t * get_bits = NULL;
        fbe_payload_control_get_buffer(control_operation_p, &get_bits);
        status = fbe_payload_metadata_build_paged_get_bits(metadata_operation_p, 
                                                           &raid_group_p->base_config.metadata_element, 
                                                           get_bits->metadata_offset,
                                                           get_bits->metadata_record_data,
                                                           get_bits->metadata_record_data_size,
                                                           sizeof(fbe_raid_group_paged_metadata_t));
        metadata_record_data = &get_bits->metadata_record_data[0];
        
        /* If FUA is requested, set the proper flag in the metadata operation.
         */
        if ((get_bits->get_bits_flags & FBE_BASE_CONFIG_CONTROL_METADATA_PAGED_GET_BITS_FLAGS_FUA_READ) != 0)
        {
            metadata_operation_p->u.metadata.operation_flags |= FBE_PAYLOAD_METADATA_OPERATION_FLAGS_FUA_READ;
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s opcode %d unexpected\n", __FUNCTION__, opcode );

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Mark this as a monitor operation so it will be failed if quiesce
     */
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_MONITOR_OP);   

    /* Get the range of the paged metadata we will be accessing.
     */
    status = fbe_metadata_paged_get_lock_range(metadata_operation_p, &metadata_start_lba, &metadata_block_count);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s get lock range failed with status: 0x%x", 
                              __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We calculate the lock range using a logical address.
     */
    fbe_raid_geometry_calculate_lock_range(raid_geometry_p, 
                                           metadata_start_lba, metadata_block_count,
                                           &metadata_stripe_number, &metadata_stripe_count);

    fbe_payload_metadata_set_metadata_stripe_offset(metadata_operation_p, metadata_stripe_number);
    fbe_payload_metadata_set_metadata_stripe_count(metadata_operation_p, metadata_stripe_count);
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_control_metadata_paged_operation_completion, raid_group_p);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);

    status = fbe_metadata_operation_entry(packet_p);

    return status;
}
/*******************************************************
 * end fbe_raid_group_control_metadata_paged_operation()
 *******************************************************/
/*!**************************************************************
 * fbe_raid_group_control_get_throttle_info()
 ****************************************************************
 * @brief
 *  Get the parameters for throttling I/O to the rg.*  
 *
 * @param raid_group_p - Current rg.
 * @param packet_p - Packet for this usurper.
 *
 * @return fbe_status_t
 *
 * @author
 *  3/20/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t
fbe_raid_group_control_get_throttle_info(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p)
{
    fbe_block_transport_get_throttle_info_t *get_throttle_info = NULL;
    fbe_status_t status;

    // Get the control buffer pointer from the packet payload.
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p, packet_p, 
                                                       sizeof(fbe_block_transport_get_throttle_info_t),
                                                       (fbe_payload_control_buffer_t)&get_throttle_info);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }
    fbe_block_transport_server_get_throttle_info(&raid_group_p->base_config.block_transport_server,
                                                 get_throttle_info);
    
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/*******************************************************
 * end fbe_raid_group_control_get_throttle_info()
 *******************************************************/
/*!**************************************************************
 * fbe_raid_group_control_set_throttle_info()
 ****************************************************************
 * @brief
 *  Set the parameters for throttling I/O to the rg.
 *
 * @param raid_group_p - Current rg.
 * @param packet_p - Packet for this usurper.
 *
 * @return fbe_status_t
 *
 * @author
 *  3/20/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t
fbe_raid_group_control_set_throttle_info(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p)
{
    fbe_block_transport_set_throttle_info_t *set_throttle_info = NULL;
    fbe_status_t status;

    // Get the control buffer pointer from the packet payload.
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p, packet_p, 
                                                       sizeof(fbe_block_transport_set_throttle_info_t),
                                                       (fbe_payload_control_buffer_t)&set_throttle_info);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }
    status = fbe_block_transport_server_set_throttle_info(&raid_group_p->base_config.block_transport_server,
                                                          set_throttle_info);
    
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/*******************************************************
 * end fbe_raid_group_control_set_throttle_info()
 *******************************************************/

/*!**************************************************************
 * fbe_raid_group_control_get_stats()
 ****************************************************************
 * @brief
 *  Get outstanding I/O statistics for the raid group.
 *
 * @param raid_group_p - Current rg.
 * @param packet_p - Packet for this usurper.
 *
 * @return fbe_status_t
 *
 * @author
 *  4/30/2013 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t
fbe_raid_group_control_get_stats(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p)
{
    fbe_raid_group_get_statistics_t *get_stats = NULL;
    fbe_status_t status;

    // Get the control buffer pointer from the packet payload.
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p, packet_p, 
                                                       sizeof(fbe_raid_group_get_statistics_t),
                                                       (fbe_payload_control_buffer_t)&get_stats);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }
    fbe_zero_memory(get_stats, sizeof(fbe_raid_group_get_statistics_t));
    status = fbe_raid_group_count_ios(raid_group_p, get_stats);
    
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/*******************************************************
 * end fbe_raid_group_control_get_stats()
 *******************************************************/
/*!**************************************************************
 * fbe_raid_group_control_init_key_handle()
 ****************************************************************
 * @brief
 *  This function initialize the object Key handle and then registers
 *  the keys with the underlying PVD.
 *
 * @param raid_group_p - Current rg.
 * @param packet_p - Packet for this usurper.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/05/2013 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_control_init_key_handle(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t * sep_payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_base_config_init_key_handle_t *init_key_handle = NULL;
    fbe_status_t status;
    fbe_u32_t width;
    fbe_encryption_setup_encryption_keys_t *key_handle = NULL;
    fbe_base_config_encryption_state_t encryption_state;
    fbe_base_config_encryption_mode_t mode;
    fbe_raid_group_encryption_flags_t flags = 0;
    fbe_lba_t checkpoint = FBE_LBA_INVALID;
    fbe_raid_group_nonpaged_metadata_t *nonpaged_metadata_p = NULL;
    fbe_lba_t exported_capacity ;

    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) raid_group_p, (void **)&nonpaged_metadata_p);

    // Get the control buffer pointer from the packet payload.
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p, packet_p, 
                                                       sizeof(fbe_base_config_init_key_handle_t),
                                                       (fbe_payload_control_buffer_t)&init_key_handle);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    fbe_base_config_get_key_handle((fbe_base_config_t *)raid_group_p, &key_handle);

    /* Make sure we did not establish keys before */
    if(key_handle != NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Key already setup\n", 
                              __FUNCTION__);
#if 0
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
#endif
    }

#if 0 /* Too early for this check */
    status = fbe_base_config_get_generation_num((fbe_base_config_t *)raid_group_p, &generation_number); 

    if(generation_number != init_key_handle->key_handle->generation_number)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Gen num does not match Exp:0x%llx actual:0x%llx\n", 
                              __FUNCTION__, 
                              (unsigned long long)generation_number, 
                              (unsigned long long) init_key_handle->key_handle->generation_number);
        /* The generation ID does not match so something wrong happened(e.g. The RG got deleted and recreated)*/
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }
#endif

    /* get the width of the raid group object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* For RAID10, key_handle may be NULL */
    if(init_key_handle->key_handle != NULL &&
       width != init_key_handle->key_handle->num_of_keys)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Number of keys unexpected Exp:%d actual:%d\n", 
                              __FUNCTION__, width, init_key_handle->key_handle->num_of_keys);
#if 0
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
#endif
    }

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Init the key handle\n", 
                          __FUNCTION__);


	if(fbe_raid_geometry_is_raid10(fbe_raid_group_get_raid_geometry(raid_group_p)) && init_key_handle->key_handle != NULL){
		fbe_base_config_class_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
				FBE_TRACE_MESSAGE_ID_INFO,
				"%s Valid key handle for striper ? \n", __FUNCTION__);
	}	


    fbe_raid_group_lock(raid_group_p);
    /* Save the key handle */
    status = fbe_base_config_set_key_handle((fbe_base_config_t *)raid_group_p, init_key_handle->key_handle);

    fbe_base_config_get_encryption_state((fbe_base_config_t *)raid_group_p, &encryption_state);
    fbe_base_config_get_encryption_mode((fbe_base_config_t*)raid_group_p, &mode);

    if (nonpaged_metadata_p != NULL){
        /* Only try to get these values if NP is available.  If we are early in specialize, these are not set yet.
         */
        fbe_raid_group_encryption_get_state(raid_group_p, &flags);
        checkpoint = fbe_raid_group_get_rekey_checkpoint(raid_group_p);
    }

    if (encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE) {
        /* Change the state so that the keys will be pushed down from the monitor context */
        fbe_base_config_set_encryption_state((fbe_base_config_t *)raid_group_p, 
                                             FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_KEYS);
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption: REKEY_COMPLETE -> REKEY_COMPLETE_PUSH_KEYS cp: %llx fl: 0x%x md: %d\n",
                              checkpoint, flags, mode);
    } else if (encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_ENCRYPTION_IN_PROGRESS) {
        checkpoint = fbe_raid_group_get_rekey_checkpoint(raid_group_p);
        exported_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);
        /* It is possible that we have not updated our state after the encryption is complete. If that is the
         * case just change the state directly */
        if (checkpoint >= exported_capacity) {
            /* Change the state so that the keys will be pushed down from the monitor context */
            fbe_base_config_set_encryption_state((fbe_base_config_t *)raid_group_p, 
                                                 FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_KEYS);
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "Encryption: IN_PROGRESS -> REKEY_COMPLETE_PUSH_KEYS cp: %llx fl: 0x%x md: %d\n", 
                                  checkpoint, flags, mode);
        }
	} else {
        /* Change the state so that the keys will be pushed down from the monitor context */
        fbe_base_config_set_encryption_state((fbe_base_config_t *)raid_group_p, 
                                              FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_PUSH_KEYS_DOWNSTREAM);
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption: %d -> LOCKED_PUSH_KEYS_DOWNSTREAM cp: %llx fl: 0x%x md: %d\n", 
                              encryption_state, checkpoint, flags, mode);
    }
    fbe_raid_group_unlock(raid_group_p);

    status = fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const,
                                      (fbe_base_object_t *) raid_group_p,
                                      (fbe_lifecycle_timer_msec_t) 0);    /* Immediate reschedule */

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return status; 
    
}

/*!**************************************************************
 * fbe_raid_group_control_remove_key_handle()
 ****************************************************************
 * @brief
 *  This function removes the object Key handle.
 *
 * @param raid_group_p - Current rg.
 * @param packet_p - Packet for this usurper.
 *
 * @return fbe_status_t
 *
 * @author
 *  12/16/2013 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_control_remove_key_handle(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t * sep_payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_encryption_setup_encryption_keys_t *key_handle;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    fbe_base_config_get_key_handle((fbe_base_config_t *)raid_group_p, &key_handle);

    /* Make sure we established keys before */
    if(key_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Key handle NULL\n", 
                              __FUNCTION__);
#if 0
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
#endif
    }

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Destroy the key handle\n", 
                          __FUNCTION__);

    fbe_base_config_set_flag((fbe_base_config_t *)raid_group_p, FBE_BASE_CONFIG_FLAG_REMOVE_ENCRYPTION_KEYS);

    status = fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const,
                                      (fbe_base_object_t *) raid_group_p,
                                      (fbe_lifecycle_timer_msec_t) 0);    /* Immediate reschedule */

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return status; 
    
}


/*!**************************************************************
 * fbe_raid_group_control_update_keys_subpacket_completion()
 ****************************************************************
 * @brief
 *  This is the completion function for updating keys for the underlying PVD.
 *
 * @param packet_p - Packet for this usurper.
 * @param context - Current rg.
 *
 * @return fbe_status_t
 *
 * @author
 *  01/06/2014 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t 
fbe_raid_group_control_update_keys_subpacket_completion(fbe_packet_t * packet_p,
                                                         fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                                          raid_group_p = NULL;
    fbe_packet_t *                                              master_packet_p = NULL;
    fbe_payload_ex_t *                                         master_sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           master_control_operation_p = NULL;
    fbe_payload_ex_t *                                         sep_payload_p = NULL;
    fbe_payload_control_operation_t *                           control_operation_p = NULL;
    fbe_payload_control_status_t                                control_status;
    fbe_payload_control_status_qualifier_t                      control_qualifier;
    fbe_bool_t                                                  is_empty;
    fbe_status_t                                                status;

    /* Cast the context to base config object pointer. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* Get the master packet, its payload and associated control operation. */
    master_packet_p = fbe_transport_get_master_packet(packet_p);
    master_sep_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    master_control_operation_p = fbe_payload_ex_get_control_operation(master_sep_payload_p);

     /* get the status of the subpacket operation. */
    status = fbe_transport_get_status_code(packet_p);

    /* Get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation_p, &control_qualifier);

    /* if any of the subpacket failed with bad status then update the master status. */
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(master_packet_p, status, 0);
        /* only control operation status is processed later */
        fbe_payload_control_set_status(master_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }

    /* if any of the subpacket failed with failure status then update the master status. */
    /*!@todo we can use error predence here if required. */
    if(control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_payload_control_set_status(master_control_operation_p, control_status);
        fbe_payload_control_set_status_qualifier(master_control_operation_p, control_qualifier);
    }

    /* remove the sub packet from master queue. */ 
    fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);

    /* when the queue is empty, all subpackets have completed. */
    if(is_empty) {
        fbe_transport_destroy_subpackets(master_packet_p);
        //fbe_memory_request_get_entry_mark_free(&master_packet_p->memory_request, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(&master_packet_p->memory_request);

        fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(master_packet_p);
    }
    else
    {
        /* Not done with processing sub packets.
         */
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}
/**************************************************
 * end fbe_raid_group_control_update_keys_subpacket_completion()
 **************************************************/

/*!**************************************************************
 * fbe_raid_group_control_update_keys_allocation_completion()
 ****************************************************************
 * @brief
 *  This is the completion function for allocation.
 *
 * @param memory_request_p - Pointer to memory request.
 * @param context - Context.
 *
 * @return fbe_status_t
 *
 * @author
 *  01/06/2014 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_control_update_keys_allocation_completion(fbe_memory_request_t * memory_request_p,
                                                         fbe_memory_completion_context_t context)
{
    fbe_packet_t *                                  packet_p = NULL;
    fbe_packet_t **                                 new_packet_p = NULL;
    fbe_u8_t **                                     buffer_p = NULL;
    fbe_payload_ex_t *                              new_sep_payload_p = NULL;
    fbe_payload_control_operation_t *               new_control_operation_p = NULL;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_raid_group_t *                              raid_group_p = NULL;
    fbe_u32_t                                       width = 0;
    fbe_u32_t                                       index = 0;
    fbe_block_edge_t *                              block_edge_p = NULL;
    fbe_base_config_memory_alloc_chunks_address_t   mem_alloc_chunks_addr = {0};
    fbe_sg_element_t                               *pre_read_sg_list_p = NULL;
    fbe_payload_pre_read_descriptor_t              *pre_read_desc_p = NULL;
    fbe_packet_attr_t                               packet_attr;
    fbe_provision_drive_control_register_keys_t     *key_info_handle_p;
    fbe_encryption_setup_encryption_keys_t          *key_handle = NULL;
    fbe_base_config_update_drive_keys_t             *update_keys = NULL;
    fbe_u32_t                                       i, num_of_drives = 0; 
    fbe_u32_t                                       drive_index[FBE_RAID_MAX_DISK_ARRAY_WIDTH + 1];
    fbe_bool_t                                      is_paco_slot_valid = FBE_FALSE;
    
    /* get the pointer to original packet. */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);

    /* get the pointer to raid group object. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: memory request: 0x%p failed. request state: %d \n",
                              __FUNCTION__, memory_request_p, memory_request_p->request_state);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_FAILED;
    }

    /* get the width of the base config object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* Get the control buffer pointer from the packet payload. */
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p, packet_p, 
                                                       sizeof(fbe_base_config_update_drive_keys_t),
                                                       (fbe_payload_control_buffer_t)&update_keys);

    for (i = 0; i < width; i++)
    {
        if (update_keys->mask & (1 << i))
        {
            drive_index[num_of_drives] = i;
            num_of_drives++;
        }
    }
    if (update_keys->mask & (1 << FBE_RAID_MAX_DISK_ARRAY_WIDTH)) {
        is_paco_slot_valid = FBE_TRUE;
    }

    /* subpacket control buffer size. */
    mem_alloc_chunks_addr.buffer_size = sizeof(fbe_provision_drive_control_register_keys_t);
    mem_alloc_chunks_addr.number_of_packets = num_of_drives;
    mem_alloc_chunks_addr.buffer_count = num_of_drives;
    mem_alloc_chunks_addr.sg_element_count = 0;
    mem_alloc_chunks_addr.sg_list_p = NULL;
    mem_alloc_chunks_addr.assign_pre_read_desc = FALSE;
    mem_alloc_chunks_addr.pre_read_sg_list_p = &pre_read_sg_list_p;
    mem_alloc_chunks_addr.pre_read_desc_p = &pre_read_desc_p;
    
    /* get subpacket pointer from the allocated memory. */
    status = fbe_base_config_memory_assign_address_from_allocated_chunks((fbe_base_config_t *) raid_group_p,
                                                                         memory_request_p,
                                                                         &mem_alloc_chunks_addr);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s memory distribution failed, status:0x%x, width:0x%x, buffer_size:0x%x\n",
                              __FUNCTION__, status, width, mem_alloc_chunks_addr.buffer_size);
        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(memory_request_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    new_packet_p = mem_alloc_chunks_addr.packet_array_p;
    buffer_p = mem_alloc_chunks_addr.buffer_array_p;
    status = fbe_base_config_get_key_handle((fbe_base_config_t *)raid_group_p, &key_handle);

    while(index < num_of_drives)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                              FBE_TRACE_LEVEL_INFO, 
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Push Key down for index:%d\n", 
                              __FUNCTION__, drive_index[index]);

        /* Initialize the sub packets. */
        fbe_transport_initialize_sep_packet(new_packet_p[index]);
        new_sep_payload_p = fbe_transport_get_payload_ex(new_packet_p[index]);

        /* initialize the control buffer. */
        key_info_handle_p = (fbe_provision_drive_control_register_keys_t *) buffer_p[index];
        if (key_handle == NULL)
        {
            key_info_handle_p->key_handle = NULL;
            key_info_handle_p->key_handle_paco = NULL;
        }
        else
        {
            key_info_handle_p->key_handle = fbe_encryption_get_key_info_handle(key_handle, drive_index[index]);
            key_info_handle_p->key_handle_paco = NULL;
            if (is_paco_slot_valid){
                key_info_handle_p->key_handle_paco = fbe_encryption_get_key_info_handle(key_handle, FBE_RAID_MAX_DISK_ARRAY_WIDTH);
            }
        }

        /* allocate and initialize the control operation. */
        new_control_operation_p = fbe_payload_ex_allocate_control_operation(new_sep_payload_p);
        fbe_payload_control_build_operation(new_control_operation_p,
                                            FBE_BASE_CONFIG_CONTROL_CODE_UPDATE_DRIVE_KEYS,
                                            key_info_handle_p,
                                            sizeof(fbe_provision_drive_control_register_keys_t));

        /* Initialize the new packet with FBE_PACKET_FLAG_EXTERNAL same as the master packet */
        fbe_transport_get_packet_attr(packet_p, &packet_attr);
        if (packet_attr & FBE_PACKET_FLAG_EXTERNAL) 
        {
            fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_EXTERNAL);            
        }

        /* set completion functon. */
        fbe_transport_set_completion_function(new_packet_p[index],
                                              fbe_raid_group_control_update_keys_subpacket_completion,
                                              raid_group_p);

        /* add the subpacket to master packet queue. */
        fbe_transport_add_subpacket(packet_p, new_packet_p[index]);
        index++;
    }

    /* send all the subpackets together. */
    for(index = 0; index < num_of_drives; index++)
    {
        fbe_base_config_get_block_edge((fbe_base_config_t *)raid_group_p, &block_edge_p, drive_index[index]);
        fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_TRAVERSE);
        fbe_block_transport_send_control_packet(block_edge_p, new_packet_p[index]);
    }

    /* send all the subpackets. */
     return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************************
 * end fbe_raid_group_control_update_keys_allocation_completion()
 **************************************************/

/*!**************************************************************
 * fbe_raid_group_control_update_drive_keys()
 ****************************************************************
 * @brief
 *  This function updates keys for the underlying PVD.
 *
 * @param raid_group_p - Current rg.
 * @param packet_p - Packet for this usurper.
 *
 * @return fbe_status_t
 *
 * @author
 *  01/06/2014 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_control_update_drive_keys(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t * sep_payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_base_config_update_drive_keys_t *update_keys = NULL;
    fbe_status_t status;
    fbe_u32_t width, num_of_drives = 0;
    fbe_base_config_memory_allocation_chunks_t mem_alloc_chunks;

    fbe_encryption_setup_encryption_keys_t *key_handle;
    fbe_u32_t i;

    /* Get the control buffer pointer from the packet payload. */
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p, packet_p, 
                                                       sizeof(fbe_base_config_update_drive_keys_t),
                                                       (fbe_payload_control_buffer_t)&update_keys);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Make sure we already established keys before */
    fbe_base_config_get_key_handle((fbe_base_config_t *)raid_group_p, &key_handle);
    if(key_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Keys are not setup yet\n", __FUNCTION__);
    }
    else if (key_handle != update_keys->key_handle)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Key handle not equal\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    fbe_base_object_trace((fbe_base_object_t*)raid_group_p, 
                          FBE_TRACE_LEVEL_INFO, 
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Update the keys\n", 
                          __FUNCTION__);

    /* get the width of the raid group object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);
    for (i = 0; i < width; i++)
    {
        if (update_keys->mask & (1 << i))
        {
            num_of_drives++;
        }
    }

    if (num_of_drives == 0)
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s No edge to send mask 0x%x\n", 
                              __FUNCTION__, update_keys->mask);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    mem_alloc_chunks.buffer_count = num_of_drives;
    mem_alloc_chunks.number_of_packets = num_of_drives;
    mem_alloc_chunks.sg_element_count = 0;
    mem_alloc_chunks.buffer_size = sizeof(fbe_provision_drive_control_register_keys_t);
    mem_alloc_chunks.alloc_pre_read_desc = FALSE;
    
    /* allocate the subpackets to collect the zero checkpoint for all the edges. */
    status = fbe_base_config_memory_allocate_chunks((fbe_base_config_t *) raid_group_p,
                                                    packet_p,
                                                    mem_alloc_chunks,
                                                    (fbe_memory_completion_function_t)fbe_raid_group_control_update_keys_allocation_completion); /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s memory allocation failed, status:0x%x\n", __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }
    return status;
}
/**************************************************
 * end fbe_raid_group_control_update_drive_keys()
 **************************************************/

/*!**************************************************************
 * fbe_raid_group_control_change_encryption_state()
 ****************************************************************
 * @brief
 *  This function handles the usurper for starting a rekey.
 *
 * @param raid_group_p - Current rg.
 * @param packet_p - Packet for this usurper.
 *
 * @return fbe_status_t
 *
 * @author
 *  10/25/2013 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_control_change_encryption_state(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_config_encryption_mode_t mode;
    fbe_base_config_encryption_state_t state;

    fbe_base_config_get_encryption_mode((fbe_base_config_t*)raid_group_p, &mode);
    fbe_base_config_get_encryption_state((fbe_base_config_t*)raid_group_p, &state);
    fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                          FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "change encryption mode: 0x%x state: 0x%x\n", mode, state);
    if (mode != FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "RG Rekey error mode: %d not allowed\n", mode);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    if (state == FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s rekey started with status: 0x%x\n", __FUNCTION__, status);
        fbe_raid_group_encryption_start(raid_group_p, packet_p);
    }
    else if (state == FBE_BASE_CONFIG_ENCRYPTION_STATE_ENCRYPTION_IN_PROGRESS) {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s complete encryption status: 0x%x\n", __FUNCTION__, status);
        //fbe_raid_group_encryption_start(raid_group_p, packet_p);
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_raid_group_control_change_encryption_state()
 **************************************/
/*!**************************************************************
 * fbe_raid_group_control_metadata_paged_operation_completion()
 ****************************************************************
 * @brief
 *  Handle completion of the metdata op.
 *  All we need to do is copy back the data and relase the metadata operation.
 *
 * @param packet_p - Usurper packet.
 * @param context - Ptr to our data.
 *
 * @return fbe_status_t 
 *
 * @author
 *  5/18/2011 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t 
fbe_raid_group_control_metadata_paged_operation_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_raid_group_t * raid_group_p = (fbe_raid_group_t*)context;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_metadata_operation_t * metadata_operation = NULL;
    fbe_u8_t *  metadata_record_data = NULL;
    fbe_payload_metadata_status_t metadata_status;
    fbe_base_config_control_metadata_paged_get_bits_t * get_bits = NULL;    /* INPUT */
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_status_t    packet_status = FBE_STATUS_OK;
    fbe_u32_t       packet_qualifier = 0;

    packet_status = fbe_transport_get_status_code(packet_p);
    if (packet_status == FBE_STATUS_OK) {
        sep_payload = fbe_transport_get_payload_ex(packet_p);
        metadata_operation =  fbe_payload_ex_get_metadata_operation(sep_payload);
        fbe_payload_ex_release_metadata_operation(sep_payload, metadata_operation);

        fbe_payload_metadata_get_status(metadata_operation, &metadata_status);

        if (metadata_status == FBE_PAYLOAD_METADATA_STATUS_OK)
        {
            control_operation = fbe_payload_ex_get_control_operation(sep_payload);  
            fbe_payload_control_get_buffer(control_operation, &get_bits);
            metadata_record_data = get_bits->metadata_record_data;

            fbe_copy_memory(metadata_record_data,
                            metadata_operation->u.metadata.record_data, 
                            metadata_operation->u.metadata.record_data_size);
        }
        else 
        {
            fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                                  FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s error metadata status: 0x%x\n", __FUNCTION__, metadata_status);
            packet_status = FBE_STATUS_GENERIC_FAILURE;
            packet_qualifier = 0;
        }
    }
    else 
    {
        fbe_base_object_trace((fbe_base_object_t*)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s error packet status: 0x%x\n", __FUNCTION__, packet_status);
        fbe_payload_ex_release_metadata_operation(sep_payload, metadata_operation);
    }

    /* Set the status of the metdata request.
     */
    fbe_transport_set_status(packet_p, packet_status, packet_qualifier);

    return FBE_STATUS_OK;
}
/*******************************************************************
 * end fbe_raid_group_control_metadata_paged_operation_completion()
 *******************************************************************/

/*!**************************************************************
 * fbe_raid_group_set_usurper_rebuild_checkpoint()
 ****************************************************************
 * @brief
 *  This function sets the specified RG rebuild checkpoint. This
 *  checkpoint lives in the RG's non-paged metadata memory.
 *  Setting the checkpoint data from usurper is used for testing purposes.
 *
 * @param raid_group_p - The raid group.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  12/2011 - Created. Susan Rundbaken
 *
 ****************************************************************/

static fbe_status_t
fbe_raid_group_usurper_set_rebuild_checkpoint(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_raid_group_set_rb_checkpoint_t *    raid_group_set_chkpt_p = NULL;   /* INPUT */
    fbe_raid_group_nonpaged_metadata_t *    nonpaged_metadata_p;
    fbe_raid_group_rebuild_nonpaged_info_t  rebuild_nonpaged_data;
    fbe_u32_t                               index;
    fbe_u64_t                               metadata_offset;
    fbe_bool_t                              found_entry_b = FBE_FALSE;


    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "usurper_set_rb_chkpt called\n");

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p, packet_p,
                                                       sizeof(fbe_raid_group_set_rb_checkpoint_t),
                                                       (fbe_payload_control_buffer_t *)&raid_group_set_chkpt_p);
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (raid_group_set_chkpt_p->position != FBE_LBA_INVALID)
    {    
        /* We have a position to update.
         */

        /* Get a pointer to the RG nonpaged metadata.
         */
        fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t*) raid_group_p, (void **)&nonpaged_metadata_p);

        /* Initialize the checkpoint data.
         */
        for (index = 0; index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; index++)
        {
            if (nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[index].checkpoint == FBE_LBA_INVALID)
            {
                rebuild_nonpaged_data.rebuild_checkpoint_info[index].checkpoint = FBE_LBA_INVALID;
                rebuild_nonpaged_data.rebuild_checkpoint_info[index].position = FBE_RAID_INVALID_DISK_POSITION;      
            }
        }

        /* Set the checkpoint data.
         */
        for (index = 0; index < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; index++)
        {
            if (nonpaged_metadata_p->rebuild_info.rebuild_checkpoint_info[index].checkpoint == FBE_LBA_INVALID)
            {
                rebuild_nonpaged_data.rebuild_checkpoint_info[index].checkpoint = raid_group_set_chkpt_p->rebuild_checkpoint;
                rebuild_nonpaged_data.rebuild_checkpoint_info[index].position = raid_group_set_chkpt_p->position;      
                found_entry_b = FBE_TRUE;  
                break;        
            }
        }
                
        if (found_entry_b == FBE_FALSE)
        {
            fbe_base_object_trace(  (fbe_base_object_t *)raid_group_p,
                                    FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "set_rb_ckpt_for_test: could not set checkpoint\n");

            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        /* Make sure rebuild logging is off.
         */
        rebuild_nonpaged_data.rebuild_logging_bitmask = 0x0;
    
        /* Set completion function for this metadata write.
         */
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_set_rebuild_checkpoint_completion_function, raid_group_p);

        /* Update the nonpaged metadata.
         */
        metadata_offset = (fbe_u64_t) (&((fbe_raid_group_nonpaged_metadata_t*)0)->rebuild_info);
        status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*) raid_group_p, packet_p, 
                                                                 metadata_offset, (fbe_u8_t*) &rebuild_nonpaged_data, 
                                                                 sizeof(rebuild_nonpaged_data));
    }

    return status;
}

/*****************************************************
 * end fbe_raid_group_set_usurper_rebuild_checkpoint()
 *****************************************************/

/*!**************************************************************
 * fbe_raid_group_set_rebuild_checkpoint_completion_function()
 ****************************************************************
 * @brief
 *  Handle completion of the set rebuild checkpoint operation.
 *
 * @param packet_p - Usurper packet.
 * @param context - Ptr to our data.
 *
 * @return fbe_status_t 
 *
 * @author
 *  12/2011 - Created. Susan Rundbaken
 *
 ****************************************************************/

static fbe_status_t 
fbe_raid_group_set_rebuild_checkpoint_completion_function(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_raid_group_t *                  raid_group_p = (fbe_raid_group_t*)context;
    fbe_raid_group_set_rb_checkpoint_t * raid_group_set_chkpt_p = NULL;   /* INPUT */
    fbe_payload_ex_t *                  payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL; 

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "usurper_set_rb_chkpt_completion_func called\n");

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p); 

    fbe_payload_control_get_buffer(control_operation_p, &raid_group_set_chkpt_p);
    if (raid_group_set_chkpt_p == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "usurper_set_rb_chkpt_completion_func: fbe_payload_control_get_buffer failed\n");

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set status in packet.
     */
    status = fbe_transport_get_status_code(packet_p);
    fbe_transport_set_status(packet_p, status, 0);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_set_rebuild_checkpoint_completion_function()
 ******************************************/

/*!**************************************************************
 * fbe_raid_group_usurper_set_verify_checkpoint()
 ****************************************************************
 * @brief
 *  This function sets the specified RG verify checkpoint. This
 *  checkpoint lives in the RG's non-paged metadata memory.
 *  Setting the checkpoint from usurper is used for testing purposes.
 *
 * @param raid_group_p - The raid group.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  12/2011 - Created. Susan Rundbaken
 *
 ****************************************************************/

static fbe_status_t
fbe_raid_group_usurper_set_verify_checkpoint(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_raid_group_set_verify_checkpoint_t *raid_group_set_chkpt_p = NULL;   /* INPUT */
    fbe_u64_t                               metadata_offset;
    fbe_u64_t                               checkpoint;


    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "fbe_raid_group_set_verify_chkpt_for_test called\n");

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p, packet_p,
                                                       sizeof(fbe_raid_group_set_verify_checkpoint_t),
                                                       (fbe_payload_control_buffer_t *)&raid_group_set_chkpt_p);
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (raid_group_set_chkpt_p->verify_checkpoint != FBE_LBA_INVALID)
    {    
        /* Get the checkpoint.
         */
        checkpoint = raid_group_set_chkpt_p->verify_checkpoint;
        
        /* Get the metadata offset.
         */
        status = fbe_raid_group_metadata_get_verify_chkpt_offset(raid_group_p, raid_group_set_chkpt_p->verify_flags, &metadata_offset);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t*) raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: Unknown verify flag: 0x%x status: 0x%x\n", __FUNCTION__, raid_group_set_chkpt_p->verify_flags, status);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }

        /*  Set completion function for this metadata write.
         */
        fbe_transport_set_completion_function(packet_p, fbe_raid_group_set_verify_checkpoint_completion_function, raid_group_p);

        /* Update the nonpaged metadata.
         */
        status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t*) raid_group_p, packet_p, 
                                                                 metadata_offset, (fbe_u8_t*) &checkpoint, 
                                                                 sizeof(checkpoint));
    }

    return status;
}

/**************************************
 * end fbe_raid_group_usurper_set_verify_checkpoint()
 **************************************/

/*!**************************************************************
 * fbe_raid_group_set_verify_checkpoint_completion_function()
 ****************************************************************
 * @brief
 *  Handle completion of the set verify checkpoint operation.
 *
 * @param packet_p - Usurper packet.
 * @param context - Ptr to our data.
 *
 * @return fbe_status_t 
 *
 * @author
 *  12/2011 - Created. Susan Rundbaken
 *
 ****************************************************************/

static fbe_status_t 
fbe_raid_group_set_verify_checkpoint_completion_function(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_raid_group_t *                  raid_group_p = (fbe_raid_group_t*)context;
    fbe_raid_group_set_verify_checkpoint_t * raid_group_set_chkpt_p = NULL;   /* INPUT */
    fbe_payload_ex_t *                  payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL; 

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "fbe_raid_group_set_verify_chkpt_for_test_completion_func called\n");

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p); 

    fbe_payload_control_get_buffer(control_operation_p, &raid_group_set_chkpt_p);
    if (raid_group_set_chkpt_p == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "fbe_raid_group_set_verify_chkpt_for_test_completion_func: fbe_payload_control_get_buffer failed\n");

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Set status in packet.
     */
    status = fbe_transport_get_status_code(packet_p);
    fbe_transport_set_status(packet_p, status, 0);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_set_verify_checkpoint_completion_function()
 ******************************************/

/*!***************************************************************************
 *          fbe_raid_group_usurper_can_pvd_be_reinitialized()
 *****************************************************************************
 *
 * @brief   This method checks whether the rg is broken or not. If it is broken
 * then it return saying that pvd cannot be reinitialized
 *
 * @param   raid_group_p - The raid group.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  05/08/2012 Created - Ashwin Tamilarasan
 *
 *******************************************************************************/
static fbe_status_t
fbe_raid_group_usurper_can_pvd_be_reinitialized(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_bool_t                           *can_pvd_reinit_p = NULL;
    fbe_base_config_downstream_health_state_t  downstream_health;
    
    fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                              packet_p, 
                                              sizeof(fbe_bool_t),
                                              (fbe_payload_control_buffer_t)&can_pvd_reinit_p);

    if (can_pvd_reinit_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "fbe_rg_can_pvd_be_reinitialized  null buffer.\n");
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        /* Base object trace will trace the object id etc.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "fbe_rg_can_pvd_be_reinitialized received.\n");

        /* If the rg health is broken, then we cannot reinitialize the pvd */
        downstream_health = fbe_raid_group_verify_downstream_health(raid_group_p);
        if(downstream_health == FBE_DOWNSTREAM_HEALTH_BROKEN)
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "fbe_rg_can_pvd_be_reinitialized: downstream is broken. cannot proceed.\n");

            *can_pvd_reinit_p = FBE_FALSE;
        }
        else
        {
            *can_pvd_reinit_p = FBE_TRUE;
        }
        
        status = FBE_STATUS_OK;
    }

    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_raid_group_usurper_can_pvd_be_reinitialized()
 **************************************************/


/*!***************************************************************************
 *          fbe_raid_group_usurper_mark_nr()
 *****************************************************************************
 *
 * @brief   This method checks whether NR can be marked and marks NR
 *
 * @param   raid_group_p - The raid group.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  05/08/2012 Created - Ashwin Tamilarasan
 *
 ****************************************************************/
static fbe_status_t
fbe_raid_group_usurper_mark_nr(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_base_config_drive_to_mark_nr_t      *drive_to_mark_nr_p = NULL;
    fbe_base_config_downstream_health_state_t  downstream_health;
    fbe_object_id_t                             downstream_object_id;
    fbe_bool_t                                 is_sys_rg = FBE_FALSE;
    fbe_u32_t                                  disk_position = FBE_EDGE_INDEX_INVALID;
    fbe_bool_t                                 is_rl_set = FBE_FALSE;
    fbe_lba_t                                  configured_capacity = FBE_LBA_INVALID;
    fbe_raid_group_needs_rebuild_context_t*     needs_rebuild_context_p = NULL;
    fbe_cpu_id_t                                cpu_id;
    fbe_bool_t                                  ok_to_mark_nr = FBE_FALSE;

    fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                              packet_p, 
                                              sizeof(fbe_base_config_drive_to_mark_nr_t),
                                              (fbe_payload_control_buffer_t)&drive_to_mark_nr_p);

    if (drive_to_mark_nr_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "fbe_rg_mark_nr  null buffer.\n");
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }
    else
    {
        /* Base object trace will trace the object id etc.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "fbe_rg_mark_nr received.\n");

        /* If the rg health is broken, then we cannot reinitialize the pvd */
        downstream_health = fbe_raid_group_verify_downstream_health(raid_group_p);
        if(downstream_health == FBE_DOWNSTREAM_HEALTH_BROKEN)
        {
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "fbe_rg_mark_nr: Downstream health broken \n");
            drive_to_mark_nr_p->nr_status = FBE_FALSE;
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_OK;
        }
        else
        {
            /* If the Rg is not broken, get the disk position corresponding to
               the pvd object id and check whether rebuild logging is set
               on the drive. If it is set, mark NR on the entire drive
            */
            fbe_raid_group_utils_is_sys_rg(raid_group_p, &is_sys_rg);
            if(is_sys_rg)
            {
                downstream_object_id = drive_to_mark_nr_p->pvd_object_id;
            }
            else
            {
                downstream_object_id = drive_to_mark_nr_p->vd_object_id;
            }

            status = fbe_raid_group_get_disk_pos_by_server_id(raid_group_p, downstream_object_id,
                                                              &disk_position);
            if(disk_position == FBE_EDGE_INDEX_INVALID)
            {
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "fbe_rg_mark_nr: Disk Position invalid for Object ID:0x%x \n",
                                  downstream_object_id);
                drive_to_mark_nr_p->nr_status = FBE_FALSE;
                fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(packet_p);
                return FBE_STATUS_OK;
            }

            /* We dont do any of the validation if forced NR is set since this is a recovery operation */
            if(!drive_to_mark_nr_p->force_nr)
            {
                status = fbe_raid_group_rebuild_ok_to_mark_nr(raid_group_p, downstream_object_id, &ok_to_mark_nr);

                if(ok_to_mark_nr == FBE_FALSE)
                {
                    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "fbe_rg_mark_nr: Not ok to mark NR for Obj:0x%x Disk Position:%d\n",
                                          downstream_object_id, disk_position);
                    drive_to_mark_nr_p->nr_status = FBE_FALSE;
                    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
                    fbe_transport_complete_packet(packet_p);
                    return FBE_STATUS_OK;
                }
                fbe_raid_group_get_rb_logging(raid_group_p, disk_position, &is_rl_set);
    
                if(is_rl_set == FBE_FALSE)
                {
                    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                          FBE_TRACE_LEVEL_INFO,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "fbe_rg_mark_nr: RL not set for 0x%x Disk Position:%d\n",
                                          downstream_object_id, disk_position);
                    drive_to_mark_nr_p->nr_status = FBE_FALSE;
                    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
                    fbe_transport_complete_packet(packet_p);
                    return FBE_STATUS_OK;
                }
            }
            
            //  Allocate the needs rebuild control operation to manage the needs rebuild context
            fbe_raid_group_needs_rebuild_allocate_needs_rebuild_context(raid_group_p, packet_p, &needs_rebuild_context_p);
            if(needs_rebuild_context_p == NULL)
            {
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "fbe_rg_mark_nr: RL not set\n");
                drive_to_mark_nr_p->nr_status = FBE_FALSE;
                fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(packet_p);
                return FBE_STATUS_OK;
            }

            /* Its packet coming from job service. The cpu id might not be
               filled out. Fill it up.
            */
            fbe_transport_get_cpu_id(packet_p, &cpu_id);
            if(cpu_id == FBE_CPU_ID_INVALID)
            {
                cpu_id = fbe_get_cpu_id();
                fbe_transport_set_cpu_id(packet_p, cpu_id);
            }
            //  Set the condition completion function before we mark NR
            fbe_transport_set_completion_function(packet_p, fbe_rg_needs_rebuild_mark_nr_from_usurper_completion, raid_group_p);
            if(drive_to_mark_nr_p->force_nr)
            {
                /* If force NR, we need to set the checkpoint to Zero so that rebuild can start */
                fbe_transport_set_completion_function(packet_p, fbe_rg_needs_rebuild_force_mark_nr_set_checkpoint, raid_group_p);
            }
            configured_capacity = fbe_raid_group_get_disk_capacity(raid_group_p);
            fbe_rg_needs_rebuild_mark_nr_from_usurper(raid_group_p, packet_p,
                                                      disk_position, 0, configured_capacity,
                                                      needs_rebuild_context_p);

            return FBE_STATUS_MORE_PROCESSING_REQUIRED;

        }
    }

    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_raid_group_usurper_mark_nr()
 **************************************************/
/*!**************************************************************
 * fbe_raid_group_usurper_set_nonpaged_metadata()
 ****************************************************************
 * @brief
 *  This function sets the nonpaged MD for RG.
 * it will set the np according to caller input in control buffer.
 *
 * @param raid_group_p - The raid group.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  5/2012 - Created. zhangy26
 *
 ****************************************************************/

static fbe_status_t
fbe_raid_group_usurper_set_nonpaged_metadata(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_base_config_control_set_nonpaged_metadata_t* control_set_nonpaged_p = NULL;

    fbe_payload_ex_t *                      payload_p = NULL;
    fbe_payload_control_operation_t *       control_operation_p = NULL; 

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);
    // Get the control buffer pointer from the packet payload.
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p, packet_p, 
                                            sizeof(fbe_base_config_control_set_nonpaged_metadata_t),
                                            (fbe_payload_control_buffer_t*)&control_set_nonpaged_p);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }
    
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    if(control_set_nonpaged_p->set_default_nonpaged_b)
    {

        status = fbe_raid_group_metadata_set_default_nonpaged_metadata(raid_group_p,packet_p);
    }else{
        //  Send the nonpaged metadata write request to metadata service. 
        status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) raid_group_p,
                                     packet_p,
                                     0, // offset is zero for the write of entire non paged metadata record. 
                                     control_set_nonpaged_p->np_record_data, // non paged metadata memory. 
                                     sizeof(fbe_raid_group_nonpaged_metadata_t)); // size of the non paged metadata. 

    }
    if(status != FBE_STATUS_OK)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}


    



/*!***************************************************************************
 *          fbe_raid_group_usurper_get_degraded_info()
 *****************************************************************************
 *
 * @brief   Get some information about the RG degraded positions and their percent of rebuilding
 *
 * @param   raid_group_p - The raid group.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 *
 ****************************************************************/
static fbe_status_t fbe_raid_group_usurper_get_degraded_info(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_raid_group_get_degraded_info_t *get_degraded_info = NULL;
    fbe_raid_geometry_t                *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    
    /* Validate the request.
     */
    fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                              packet_p, 
                                              sizeof(fbe_raid_group_get_degraded_info_t),
                                              (fbe_payload_control_buffer_t)&get_degraded_info);

    if (get_degraded_info == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "fbe_rg_mark_nr  null buffer.\n");
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        /* If this is a RAID-10 we need to get the degraded information from
         * each of the downstream mirror raid groups.
         */
        if (fbe_raid_geometry_is_striped_mirror(raid_geometry_p) == FBE_TRUE)
        {
            /* Use the return buffer and invoke the method to populate the 
             * buffer.
             */
            status = fbe_raid_group_get_raid10_degraded_info(packet_p,
                                                             raid_group_p,
                                                             get_degraded_info);
        }
        else
        {
            /* Else get the degraded information directly from this raid group.
             */
            /*first, let make everything as potentially completed and good*/
            get_degraded_info->rebuild_percent = 100;

            /*are we even rebuilding ?*/
            if (fbe_raid_group_background_op_is_rebuild_need_to_run(raid_group_p))
            {
                /* Simply get the zero checkpoint and compare it to capacity to see the zero percentage.
                */
                get_degraded_info->rebuild_percent = fbe_raid_group_get_percent_rebuilt(raid_group_p);
            }

            /* Invoke common method tha determines if this raid group is degraded or not.
             */
            get_degraded_info->b_is_raid_group_degraded = fbe_raid_group_is_degraded(raid_group_p);
        }
    }

    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}
/**************************************************
 * end fbe_raid_group_usurper_get_degraded_info()
 **************************************************/

/*!****************************************************************************
 * fbe_raid_group_usurper_get_rebuild_status()
 ******************************************************************************
 * @brief
 *  This function is used to get the rebuild status for the raid group extent,
 *  it gets the minimum checkpoint and calculates the rebuild status.
 *
 * @param raid_group_p  - raid group object.
 * @param packet_p      - Pointer to the packet.
 *
 * @return status       - status of the operation.
 *
 * @author
 *  06/12/2012 - Created Vera Wang
 ******************************************************************************/
static fbe_status_t fbe_raid_group_usurper_get_rebuild_status(fbe_raid_group_t * raid_group_p,
                                                              fbe_packet_t * packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_raid_geometry_t                *raid_geometry_p  = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_group_rebuild_status_t    *rebuild_status_p = NULL;
    fbe_u32_t                           width;
    fbe_raid_group_type_t               raid_type;
    fbe_lba_t                           exported_disk_capacity;
    fbe_lba_t                           min_rebuild_checkpoint = 0;
    fbe_lba_t                           max_rebuild_checkpoint = 0;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_payload_ex_t                   *sep_payload_p = NULL;

    /* get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the control buffer pointer from the packet payload. */
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                                       packet_p, 
                                                       sizeof(fbe_raid_group_rebuild_status_t),
                                                       (fbe_payload_control_buffer_t)&rebuild_status_p);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    /* Get the raid group geometry
     */
    exported_disk_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);
    fbe_base_config_get_width((fbe_base_config_t*)raid_group_p, &width);

    /* Get the minimum rebuild checkpoint for the RG. 
     */
    fbe_raid_group_get_min_rebuild_checkpoint(raid_group_p, &min_rebuild_checkpoint);
    
    /* Special case where there are/were multiple positions rebuilding.
     * In this case we must 
     */   
    if (((raid_type == FBE_RAID_GROUP_TYPE_RAID1) &&
         (width > 2)                                 ) ||
        (raid_type == FBE_RAID_GROUP_TYPE_RAID6)          )
    {
        /* Get the maximum rebuild checkpoint.
         */
        fbe_raid_group_get_max_rebuild_checkpoint(raid_group_p, &max_rebuild_checkpoint);

        /* If the minimum does not match the maximum, then average them to
         * determine the checkpoint and percent rebuilt for the raid group.
         */
        if (min_rebuild_checkpoint != max_rebuild_checkpoint)
        {
            rebuild_status_p->rebuild_checkpoint = (max_rebuild_checkpoint + min_rebuild_checkpoint) / 2;
        }
        else
        {
            rebuild_status_p->rebuild_checkpoint = min_rebuild_checkpoint;
        }
    }
    else
    {
        /* Else use the minimum
         */
        rebuild_status_p->rebuild_checkpoint = min_rebuild_checkpoint;
    }

    /* calculating the rebuild percentage value for RG*/ 
    if (rebuild_status_p->rebuild_checkpoint == FBE_LBA_INVALID)
    {
        rebuild_status_p->rebuild_percentage = 100;
       
    }
    else
    {
        /* Compute the rebuilt percentage.
         */
        rebuild_status_p->rebuild_percentage = (fbe_u32_t)((rebuild_status_p->rebuild_checkpoint * 100) / exported_disk_capacity);

        /* If the percent is greater than 100 something went wrong.
         */
        if (rebuild_status_p->rebuild_percentage > 100)
        {
            /* Generate an error trace and change it to 100 to recover from the
             * error.
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "raid_group: usurper get rebuild status percent: %d per disk cap: 0x%llx chkpt: 0x%llx bad\n",
                                  rebuild_status_p->rebuild_percentage,
                                  (unsigned long long)exported_disk_capacity,
                                  (unsigned long long)rebuild_status_p->rebuild_checkpoint);
            rebuild_status_p->rebuild_percentage = 100;
        }
    }   
    
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_usurper_get_rebuild_status()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_usurper_get_lun_percent_rebuilt()
 ******************************************************************************
 *
 * @brief   This function is used to get the `percent' rebuilt for the lun.  The
 *          lun object supplies the logical offset and logical capacity.  This
 *          function returns the logical values (relative to the lun):
 *              o Rebuild checkpoint (logical for the lun specified)
 *              o Percent rebuilt the percentage of the lun that has been rebuilt
 *
 * @param   raid_group_p  - raid group object.
 * @param   packet_p      - Pointer to the packet.
 *
 * @return  status       - status of the operation.
 *
 * @author
 *  05/03/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_usurper_get_lun_percent_rebuilt(fbe_raid_group_t *raid_group_p,
                                                                   fbe_packet_t *packet_p)
{
    fbe_status_t                                status;
    fbe_raid_geometry_t                        *raid_geometry_p  = fbe_raid_group_get_raid_geometry(raid_group_p);
    fbe_raid_group_get_lun_percent_rebuilt_t   *lun_percent_rebuilt_p = NULL;
    fbe_u32_t                                   width;
    fbe_u16_t                                   num_data_disk;
    fbe_raid_group_type_t                       raid_type;
    fbe_lba_t                                   exported_disk_capacity;
    fbe_lba_t                                   min_rebuild_checkpoint;
    fbe_lba_t                                   max_rebuild_checkpoint;
    fbe_payload_control_operation_t            *control_operation_p = NULL;
    fbe_payload_ex_t                           *sep_payload_p = NULL;
    fbe_lba_t                                   lun_start_lba;
    fbe_lba_t                                   lun_end_lba;
    fbe_lba_t                                   lun_capacity;
    fbe_lba_t                                   first_rebuild_position_blocks_rebuilt = 0;
    fbe_lba_t                                   second_rebuild_position_blocks_rebuilt = 0;
    fbe_raid_position_bitmask_t                 currently_rebuilding_bitmask = 0;

    /* get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the control buffer pointer from the packet payload. */
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                                       packet_p, 
                                                       sizeof(*lun_percent_rebuilt_p),
                                                       (fbe_payload_control_buffer_t)&lun_percent_rebuilt_p);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    fbe_raid_group_rebuild_get_all_rebuild_position_mask(raid_group_p, &currently_rebuilding_bitmask);

    if (currently_rebuilding_bitmask == 0)
    {
        /* If the client's previous request resulted in `rebuild in progress'
         * we need to report rebuild in progress and 100 percent complete.
         */
        if (!lun_percent_rebuilt_p->is_rebuild_in_progress)
        {
            lun_percent_rebuilt_p->is_rebuild_in_progress = FBE_FALSE;
        }
    }
    else
    {
        lun_percent_rebuilt_p->is_rebuild_in_progress = FBE_TRUE;
    }

    /* Get the raid group geometry
     */
    exported_disk_capacity = fbe_raid_group_get_exported_disk_capacity(raid_group_p);
    fbe_raid_geometry_get_data_disks(raid_geometry_p, &num_data_disk);
    fbe_raid_geometry_get_raid_type(raid_geometry_p, &raid_type);
    fbe_base_config_get_width((fbe_base_config_t*)raid_group_p, &width);

    /* Convert the logical lun range to it's physical.
     */
    lun_start_lba = lun_percent_rebuilt_p->lun_offset / num_data_disk;
    lun_end_lba = lun_start_lba + (lun_percent_rebuilt_p->lun_capacity / num_data_disk) - 1;
    lun_capacity = lun_percent_rebuilt_p->lun_capacity / num_data_disk;

    /* Get the minimum rebuild checkpoint for the RG. 
     */
    fbe_raid_group_get_min_rebuild_checkpoint(raid_group_p, &min_rebuild_checkpoint);
    
    /* Special case where there are/were multiple positions rebuilding.
     * In this case we must 
     */   
    if (((raid_type == FBE_RAID_GROUP_TYPE_RAID1) &&
         (width > 2)                                 ) ||
        (raid_type == FBE_RAID_GROUP_TYPE_RAID6)          )
    {
        /* Get the maximum rebuild checkpoint.
         */
        fbe_raid_group_get_max_rebuild_checkpoint(raid_group_p, &max_rebuild_checkpoint);

        /* If the minimum does not match the maximum then we must take into
         * account that there were (2) positions rebuilding.
         */
        if (min_rebuild_checkpoint != max_rebuild_checkpoint)
        {
            /* Get the overlap with the first position rebuilt.
             */
            if (lun_end_lba < max_rebuild_checkpoint)
            {
                first_rebuild_position_blocks_rebuilt = lun_capacity;
            }
            else if (lun_start_lba >= max_rebuild_checkpoint)
            {
                first_rebuild_position_blocks_rebuilt = 0;
            }
            else
            {
                first_rebuild_position_blocks_rebuilt = max_rebuild_checkpoint - lun_start_lba;
            }
            if (lun_end_lba < min_rebuild_checkpoint)
            {
                second_rebuild_position_blocks_rebuilt = lun_capacity;
            }
            else if (lun_start_lba >= min_rebuild_checkpoint)
            {
                second_rebuild_position_blocks_rebuilt = 0;
            }
            else
            {
                second_rebuild_position_blocks_rebuilt = min_rebuild_checkpoint - lun_start_lba;
            }

            /*! @note Now average the first and second positions to get the lun 
             *        rebuild checkpoint. 
             */
            lun_percent_rebuilt_p->lun_rebuild_checkpoint = (first_rebuild_position_blocks_rebuilt + second_rebuild_position_blocks_rebuilt) / 2;
            if (lun_percent_rebuilt_p->lun_rebuild_checkpoint >= lun_capacity)
            {
                lun_percent_rebuilt_p->lun_rebuild_checkpoint = FBE_LBA_INVALID;
                lun_percent_rebuilt_p->lun_percent_rebuilt = 100;
            }
            else
            {
                /*! @note Note the lun rebuild checkpoint is logical.  Therefore
                 *        we must multiply by the data positions.
                 */
                lun_percent_rebuilt_p->lun_percent_rebuilt = (fbe_u32_t)((lun_percent_rebuilt_p->lun_rebuild_checkpoint * 100) / lun_capacity);
                lun_percent_rebuilt_p->lun_rebuild_checkpoint *= num_data_disk;
            }
        }
        else
        {
            /* Else compute the data assuming only (1) rebuilding position
             */
            if (lun_end_lba < min_rebuild_checkpoint)
            {
                lun_percent_rebuilt_p->lun_rebuild_checkpoint = FBE_LBA_INVALID;
                lun_percent_rebuilt_p->lun_percent_rebuilt = 100;
            }
            else if (lun_start_lba >= min_rebuild_checkpoint)
            {
                lun_percent_rebuilt_p->lun_rebuild_checkpoint = 0;
                lun_percent_rebuilt_p->lun_percent_rebuilt = 0;
            }
            else
            {
                /*! @note Note the lun rebuild checkpoint is logical.  Therefore
                 *        we must multiply by the data positions.
                 */
                lun_percent_rebuilt_p->lun_rebuild_checkpoint = (min_rebuild_checkpoint - lun_start_lba) * num_data_disk;
                lun_percent_rebuilt_p->lun_percent_rebuilt = (fbe_u32_t)((lun_percent_rebuilt_p->lun_rebuild_checkpoint * 100) / lun_percent_rebuilt_p->lun_capacity);
            }
        }
    }
    else
    {
        /* Else compute the data assuming only (1) rebuilding position
         */
        if (lun_end_lba < min_rebuild_checkpoint)
        {
            lun_percent_rebuilt_p->lun_rebuild_checkpoint = FBE_LBA_INVALID;
            lun_percent_rebuilt_p->lun_percent_rebuilt = 100;
        }
        else if (lun_start_lba >= min_rebuild_checkpoint)
        {
            lun_percent_rebuilt_p->lun_rebuild_checkpoint = 0;
            lun_percent_rebuilt_p->lun_percent_rebuilt = 0;
        }
        else
        {
            /*! @note Note the lun rebuild checkpoint is logical.  Therefore
             *        we must multiply by the data positions.
             */
            lun_percent_rebuilt_p->lun_rebuild_checkpoint = (min_rebuild_checkpoint - lun_start_lba) * num_data_disk;
            lun_percent_rebuilt_p->lun_percent_rebuilt = (fbe_u32_t)((lun_percent_rebuilt_p->lun_rebuild_checkpoint * 100) / lun_percent_rebuilt_p->lun_capacity);
        }
    }

    /* If the percent is greater than 100 something went wrong.
     */
    if (lun_percent_rebuilt_p->lun_percent_rebuilt > 100)
    {
        /* Generate an error trace and change it to 100 to recover from the
         * error.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: usurper get lun rebuilt percent: %d lun cap: 0x%llx chkpt: 0x%llx bad\n",
                              lun_percent_rebuilt_p->lun_percent_rebuilt,
                              (unsigned long long)lun_capacity,
                              (unsigned long long)lun_percent_rebuilt_p->lun_rebuild_checkpoint);
        lun_percent_rebuilt_p->lun_percent_rebuilt = 100;
    }
    
    /* If enabled trace some information
     */
    if (fbe_raid_group_is_debug_flag_set(raid_group_p, FBE_RAID_GROUP_DEBUG_FLAG_LUN_PERCENT_REBUILT_NOTIFICATION_TRACING))
    {
        /* Set the debug flags and trace some information.
         */
        lun_percent_rebuilt_p->debug_flags |= (fbe_u32_t)FBE_RAID_GROUP_DEBUG_FLAG_LUN_PERCENT_REBUILT_NOTIFICATION_TRACING;
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "raid_group: usurper get lun rebuilt ofst: 0x%llx cap: 0x%llx prct: %d chkpt: 0x%llx multi: %d\n",
                              (unsigned long long)lun_percent_rebuilt_p->lun_offset,
                              (unsigned long long)lun_percent_rebuilt_p->lun_capacity,
                              lun_percent_rebuilt_p->lun_percent_rebuilt,
                              (unsigned long long)lun_percent_rebuilt_p->lun_rebuild_checkpoint,
                              (first_rebuild_position_blocks_rebuilt == 0) ? FBE_FALSE : FBE_TRUE);
    }

    /* Return success
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_usurper_get_lun_percent_rebuilt()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_set_raid10_mirror_prefered_position()
 ******************************************************************************
 * @brief
 *  This function is send packet to R10 downstream mirror raid group.
 *
 * @param raid_group_p  - raid group object.
 * @param packet_p      - Pointer to the packet.
 * @param position      - the position of ds RG.
 * @param prefered_position - mirror prefered position.
 *
 * @return status       - status of the operation.
 *
 * @author
 *  06/22/2012 - Created Lili Chen
 ******************************************************************************/
fbe_status_t fbe_raid_group_set_raid10_mirror_prefered_position(fbe_raid_group_t *raid_group_p,
                                                                fbe_packet_t *packet_p,
                                                                fbe_u32_t position,
                                                                fbe_mirror_prefered_position_t prefered_position)
{
    fbe_payload_ex_t*                   payload_p;
    fbe_block_edge_t *                  block_edge_p = NULL;
    fbe_status_t                        status;

    /* Get the control op buffer data pointer from the packet payload.*/
    payload_p = fbe_transport_get_payload_ex(packet_p);

    /*find the object id of our mirror server*/
    fbe_base_config_get_block_edge((fbe_base_config_t*)raid_group_p, &block_edge_p, position);
    if (block_edge_p == NULL){
        return FBE_STATUS_OK;/*no edge to process*/
    }
    
    /* Set packet address.*/
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              block_edge_p->base_edge.server_id); 


    /* allocate the new control operation*/
    /*control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);
    fbe_payload_control_build_operation(control_operation_p,
                                        FBE_RAID_GROUP_CONTROL_CODE_GET_INFO,
                                        &get_info,
                                        sizeof(fbe_raid_group_get_info_t));

    fbe_payload_ex_increment_control_operation_level(payload_p); */
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /*send it*/
    fbe_block_transport_send_control_packet(block_edge_p, packet_p);

    fbe_transport_wait_completion(packet_p);

    /* fbe_payload_ex_release_control_operation(payload_p, control_operation_p); */

    /*when we get here the data is there for us to use(in case the packet completed nicely*/
    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed to set mirror prefered position from mirror id 0x%llX\n", 
                                __FUNCTION__,(unsigned long long)block_edge_p->base_edge.server_id);
        return status;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_set_raid10_mirror_prefered_position()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_usurper_set_mirror_prefered_position()
 ******************************************************************************
 * @brief
 *  This function is used to set prefered position in raid geometry.
 *
 * @param raid_group_p  - raid group object.
 * @param packet_p      - Pointer to the packet.
 *
 * @return status       - status of the operation.
 *
 * @author
 *  06/22/2012 - Created Lili Chen
 ******************************************************************************/
static fbe_status_t
fbe_raid_group_usurper_set_mirror_prefered_position(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p)
{
    fbe_raid_geometry_t *                   raid_geometry_p = NULL;
    fbe_mirror_prefered_position_t *        position;
    fbe_status_t                            status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "fbe_raid_group_usurper_set_mirror_prefered_position called\n");

    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p, packet_p,
                                                       sizeof(fbe_mirror_prefered_position_t),
                                                       (fbe_payload_control_buffer_t *)&position);
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    if((((fbe_base_object_t *)raid_group_p)->class_id == FBE_CLASS_ID_STRIPER) &&
       (raid_geometry_p->raid_type == FBE_RAID_GROUP_TYPE_RAID10))
    {
        fbe_u32_t pos;
        for (pos = 0; pos < raid_geometry_p->width; pos++)
        {
            status = fbe_raid_group_set_raid10_mirror_prefered_position(raid_group_p, packet_p, pos, *position);
            if (status != FBE_STATUS_OK)
            {
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s send to pos %d failed\n", __FUNCTION__, pos);
                break;
            }
        }
    }
    else
    {
        fbe_raid_geometry_set_mirror_prefered_position(raid_geometry_p, *position);
    }

    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_usurper_set_mirror_prefered_position()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_usurper_get_spindown_qualified()
 ******************************************************************************
 * @brief
 *  This function is used to get power saving capability if all drives belongs to
 *  this raid group are spindown qualified.
 *
 * @param raid_group_p  - raid group object.
 * @param packet_p      - Pointer to the packet.
 *
 * @return status       - status of the operation.
 *
 * @author
 *  09/24/2012 - Created Vera Wang
 ******************************************************************************/
static fbe_status_t
fbe_raid_group_usurper_get_spindown_qualified(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                           FBE_TRACE_LEVEL_DEBUG_HIGH,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "fbe_raid_group_usurper_get_spindown_qualified called\n");


    status = base_config_send_spindown_qualified_usurper_to_servers((fbe_base_config_t*)raid_group_p, packet_p);
                                                                                                                
    if (status == FBE_STATUS_OK) {
        raid_group_p->base_config.power_save_capable.spindown_qualified = FBE_TRUE;
    }
    else if (status == FBE_STATUS_INSUFFICIENT_RESOURCES){
        raid_group_p->base_config.power_save_capable.spindown_qualified = FBE_FALSE;
    }

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_raid_group_usurper_get_spindown_qualified()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_raid_group_usurper_get_health()
 *****************************************************************************
 *
 * @brief   This function get the downstream health.
 *
 * @param   raid_group_p - The raid group.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/01/2012 Created - gaoh1
 *
 *******************************************************************************/
static fbe_status_t
fbe_raid_group_usurper_get_health(fbe_raid_group_t * raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_base_config_downstream_health_state_t  *downstream_health = NULL;
    
    fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                              packet_p, 
                                              sizeof(fbe_base_config_downstream_health_state_t),
                                              (fbe_payload_control_buffer_t)&downstream_health);

    if (downstream_health == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "downstream_health pointer from packet is null buffer.\n");
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        /* Base object trace will trace the object id etc.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: get raid downstream health received.\n",
                            __FUNCTION__);

        /* If the rg health is broken, then we cannot reinitialize the pvd */
        *downstream_health = fbe_raid_group_verify_downstream_health(raid_group_p);
        status = FBE_STATUS_OK;
    }

    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_raid_group_usurper_get_health()
 **************************************************/

/*!***************************************************************************
 *          fbe_raid_group_quiesce_implementation()
 *****************************************************************************
 *
 * @brief   This function does the actual quiesce
 *
 * @param   raid_group_p - The raid group.
 *
 * @return status - The status of the operation.
 *
 *
 *******************************************************************************/
fbe_status_t fbe_raid_group_quiesce_implementation(fbe_raid_group_t * raid_group_p)
{
	fbe_status_t status;

	fbe_raid_group_lock(raid_group_p);

    /* Only set the quiesce condition if we are not already quiesced.
     */
    if (!fbe_base_config_is_any_clustered_flag_set((fbe_base_config_t *) raid_group_p, 
                            (FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED|FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCING)))
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s FBE_RAID_GROUP_CONTROL_CODE_QUIESCE received. Quiescing...\n", __FUNCTION__);
        /* this prevents unquiesce from happening automatically */
        fbe_base_config_set_flag((fbe_base_config_t *) raid_group_p, FBE_BASE_CONFIG_FLAG_USER_INIT_QUIESCE);
        fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const,
                               (fbe_base_object_t*)raid_group_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_QUIESCE);
        fbe_raid_group_unlock(raid_group_p);
        /* Schedule monitor.
         */
        status = fbe_lifecycle_reschedule(&fbe_raid_group_lifecycle_const,
                                          (fbe_base_object_t *) raid_group_p,
                                          (fbe_lifecycle_timer_msec_t) 0);    /* Immediate reschedule */
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s FBE_RAID_GROUP_CONTROL_CODE_QUIESCE received. Already quiesced or quiescing.  fl:0x%x.\n", 
                              __FUNCTION__, raid_group_p->flags);
        fbe_raid_group_unlock(raid_group_p);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/**************************************************
 * end fbe_raid_group_quiesce_implementation()
 **************************************************/

/*!****************************************************************************
 *          fbe_raid_group_usurper_get_lowest_drive_tier()
 ******************************************************************************
 *
 * @brief   This function is used to get the lowest drive tier in the raid group
 *
 * @param   raid_group_p  - raid group object.
 * @param   packet_p      - Pointer to the packet.
 *
 * @return  status       - status of the operation.
 *
 * @author
 *  01/27/2014  Deanna Heng  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_usurper_get_lowest_drive_tier(fbe_raid_group_t *raid_group_p,
                                                                 fbe_packet_t *packet_p)
{
    fbe_status_t                                status;
    fbe_get_lowest_drive_tier_t                *lowest_drive_tier_p = NULL;
    fbe_payload_control_operation_t            *control_operation_p = NULL;
    fbe_payload_ex_t                           *sep_payload_p = NULL;
    fbe_raid_group_nonpaged_metadata_t*        raid_group_non_paged_metadata_p = NULL;


    /* get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the control buffer pointer from the packet payload. */
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                                       packet_p, 
                                                       sizeof(*lowest_drive_tier_p),
                                                       (fbe_payload_control_buffer_t)&lowest_drive_tier_p);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    fbe_base_config_get_nonpaged_metadata_ptr((fbe_base_config_t *)raid_group_p, (void **)&raid_group_non_paged_metadata_p);
    if (raid_group_non_paged_metadata_p != NULL)
    {
        lowest_drive_tier_p->drive_tier = raid_group_non_paged_metadata_p->drive_tier;
        /* Return success
         */
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    } else {
        /* Return failure if the nonpaged is null
         */
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
  
}
/******************************************************************************
 * end fbe_raid_group_usurper_get_lowest_drive_tier()
 ******************************************************************************/

/*!**************************************************************
 * fbe_raid_group_query_for_lowest_drive_speed()
 ****************************************************************
 * @brief
 *  This function allocates the memory for issuing queries to the 
 *  provision drive to get the physical drive properties
 *
 * @param raid_group_p - raid group
 * @param packet_p - Packet for this usurper.
 *
 * @return fbe_status_t
 *
 * @author
 *  2/06/2014 - Created. Deanna Heng
 *
 ****************************************************************/
fbe_status_t
fbe_raid_group_query_for_lowest_drive_tier(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t                                status;
    fbe_u32_t                                   width;
    fbe_u32_t                                   index;
    fbe_base_config_memory_allocation_chunks_t  mem_alloc_chunks;
    fbe_raid_group_get_drive_tier_downstream_t  *drive_tier_downstream_p = NULL;
    fbe_payload_ex_t                            *sep_payload_p = NULL;
    fbe_payload_control_operation_t             *control_operation_p = NULL;


    /* get the sep payload and control operation/ */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the control buffer pointer from the packet payload. */
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                                       packet_p, 
                                                       sizeof(fbe_raid_group_get_drive_tier_downstream_t),
                                                       (fbe_payload_control_buffer_t)&drive_tier_downstream_p);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    /* initialize the checkpoint to zero before sending the request down. */
    for(index = 0; index < FBE_RAID_MAX_DISK_ARRAY_WIDTH; index++)
    {
        drive_tier_downstream_p->drive_tier[index] = 0;
    }


    /* get the width of the raid group object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* We need to send the request down to all the edges and so get the width and allocate
     * packet for every edge */
    /* Set the various counts and buffer size */
    mem_alloc_chunks.buffer_count = width;
    mem_alloc_chunks.number_of_packets = width;
    mem_alloc_chunks.sg_element_count = 0;
    mem_alloc_chunks.buffer_size = sizeof(fbe_physical_drive_mgmt_get_drive_information_t) + sizeof(fbe_edge_index_t);
    mem_alloc_chunks.alloc_pre_read_desc = FALSE;
    
    /* allocate the subpackets to collect the zero checkpoint for all the edges. */
    status = fbe_base_config_memory_allocate_chunks((fbe_base_config_t *) raid_group_p,
                                                    packet_p,
                                                    mem_alloc_chunks,
                                                    (fbe_memory_completion_function_t)fbe_raid_group_query_for_lowest_drive_tier_allocation_completion); /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s memory allocation failed, status:0x%x\n", __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}
/******************************************************************************
 * end fbe_raid_group_query_for_lowest_drive_tier(()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_query_for_lowest_drive_tier_allocation_completion()
 ******************************************************************************
 * @brief
 *  This function gets called after memory has been allocated and sends the 
 *  request down to the the block edge
 *
 * @param memory_request_p  - memory request.
 * @param context           - context passed to acquire the memory.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  2/06/2014 - Created. Deanna Heng
 *
 ******************************************************************************/
static fbe_status_t
fbe_raid_group_query_for_lowest_drive_tier_allocation_completion(fbe_memory_request_t * memory_request_p,
                                                          fbe_memory_completion_context_t context)
{
    fbe_packet_t *                                  packet_p = NULL;
    fbe_packet_t **                                 new_packet_p = NULL;
    fbe_u8_t **                                     buffer_p = NULL;
    fbe_payload_ex_t *                              new_sep_payload_p = NULL;
    fbe_payload_control_operation_t *               new_control_operation_p = NULL;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_raid_group_t *                              raid_group_p = NULL;
    fbe_u32_t                                       width = 0;
    fbe_u32_t                                       index = 0;
    fbe_block_edge_t *                              block_edge_p = NULL;
    fbe_base_config_memory_alloc_chunks_address_t   mem_alloc_chunks_addr = {0};
    fbe_sg_element_t                               *pre_read_sg_list_p = NULL;
    fbe_payload_pre_read_descriptor_t              *pre_read_desc_p = NULL;
    fbe_packet_attr_t                               packet_attr;
    fbe_physical_drive_mgmt_get_drive_information_t *drive_info_handle_p;
    fbe_edge_index_t *                  edge_index_p = NULL;
    
    /* get the pointer to original packet. */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);

    /* get the pointer to raid group object. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: memory request: 0x%p failed. request state: %d \n",
                              __FUNCTION__, memory_request_p, memory_request_p->request_state);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    /* get the width of the base config object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* subpacket control buffer size. */
    mem_alloc_chunks_addr.buffer_size = sizeof(fbe_physical_drive_mgmt_get_drive_information_t) + sizeof(fbe_edge_index_t);
    mem_alloc_chunks_addr.number_of_packets = width;
    mem_alloc_chunks_addr.buffer_count = width;
    mem_alloc_chunks_addr.sg_element_count = 0;
    mem_alloc_chunks_addr.sg_list_p = NULL;
    mem_alloc_chunks_addr.assign_pre_read_desc = FALSE;
    mem_alloc_chunks_addr.pre_read_sg_list_p = &pre_read_sg_list_p;
    mem_alloc_chunks_addr.pre_read_desc_p = &pre_read_desc_p;
    
    /* get subpacket pointer from the allocated memory. */
    status = fbe_base_config_memory_assign_address_from_allocated_chunks((fbe_base_config_t *) raid_group_p,
                                                                         memory_request_p,
                                                                         &mem_alloc_chunks_addr);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s memory distribution failed, status:0x%x, width:0x%x, buffer_size:0x%x\n",
                              __FUNCTION__, status, width, mem_alloc_chunks_addr.buffer_size);
        fbe_memory_free_request_entry(memory_request_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    new_packet_p = mem_alloc_chunks_addr.packet_array_p;
    buffer_p = mem_alloc_chunks_addr.buffer_array_p;

    while(index < width)
    {
        /* Initialize the sub packets. */
        fbe_transport_initialize_sep_packet(new_packet_p[index]);
        new_sep_payload_p = fbe_transport_get_payload_ex(new_packet_p[index]);

        /* initialize the control buffer. */
        drive_info_handle_p = (fbe_physical_drive_mgmt_get_drive_information_t *) buffer_p[index];

        /* initialize the edge index as a context data. */
        edge_index_p = (fbe_edge_index_t *) (buffer_p[index] + sizeof(fbe_physical_drive_mgmt_get_drive_information_t));
        *edge_index_p = index;

        /* allocate and initialize the control operation. */
        new_control_operation_p = fbe_payload_ex_allocate_control_operation(new_sep_payload_p);
        fbe_payload_control_build_operation(new_control_operation_p,
                                            FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DRIVE_INFORMATION,
                                            drive_info_handle_p,
                                            sizeof(fbe_physical_drive_mgmt_get_drive_information_t));

        /* Initialize the new packet with FBE_PACKET_FLAG_EXTERNAL same as the master packet */
        fbe_transport_get_packet_attr(packet_p, &packet_attr);
        if (packet_attr & FBE_PACKET_FLAG_EXTERNAL) 
        {
            fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_EXTERNAL);            
        }

        /* set completion functon. */
        fbe_transport_set_completion_function(new_packet_p[index],
                                              fbe_raid_group_query_drive_tier_subpacket_completion,
                                              raid_group_p);

        /* add the subpacket to master packet queue. */
        fbe_transport_add_subpacket(packet_p, new_packet_p[index]);
        index++;
    }

    /* set the parent packet status to good before sending subpackets */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);

    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_DRIVE_TIER_TRACING,
                         "%s: raid_group: Send all subpackets for drive query\n",
                         __FUNCTION__);

    /* send all the subpackets together. */
    for(index = 0; index < width; index++)
    {
        fbe_base_config_get_block_edge((fbe_base_config_t *)raid_group_p, &block_edge_p, index);
        fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_TRAVERSE);
        fbe_block_transport_send_control_packet(block_edge_p, new_packet_p[index]);
    }

    /* send all the subpackets. */
     return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_raid_group_quer_for_lowest_drive_tier_allocation_completion()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_raid_group_query_drive_tier_subpacket_completion()
 ******************************************************************************
 * @brief
 *  Handle the completion of a subpacket that was sent downstream to query for the 
 *  physical drive information. Update the drive information from to the subpacket to 
 *  the master packet.
 *
 * @param packet_p          - packet.
 * @param context           - context passed to request.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  2/06/2014 - Created. Deanna Heng
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_query_drive_tier_subpacket_completion(fbe_packet_t * packet_p,
                                                     fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                              raid_group_p = NULL;
    fbe_packet_t *                                  master_packet_p = NULL;
    fbe_payload_ex_t *                              master_sep_payload_p = NULL;
    fbe_payload_control_operation_t *               master_control_operation_p = NULL;
    fbe_payload_ex_t *                              sep_payload_p = NULL;
    fbe_payload_control_operation_t *               control_operation_p = NULL;
    fbe_payload_control_status_t                    control_status;
    fbe_payload_control_status_qualifier_t          control_qualifier;
    fbe_bool_t                                      is_empty;
    fbe_status_t                                    status;
    fbe_raid_group_get_drive_tier_downstream_t *    drive_tier_downstream_p = NULL;
    fbe_physical_drive_mgmt_get_drive_information_t *drive_info_p = NULL;
    fbe_edge_index_t *                              edge_index_p = NULL;
	

    /* Cast the context to base config object pointer. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* Get the master packet, its payload and associated control operation. */
    master_packet_p = fbe_transport_get_master_packet(packet_p);
    master_sep_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    master_control_operation_p = fbe_payload_ex_get_control_operation(master_sep_payload_p);

    /* get the control buffer of the master packet. */
    fbe_payload_control_get_buffer(master_control_operation_p, &drive_tier_downstream_p);

     /* get the status of the subpacket operation. */
    status = fbe_transport_get_status_code(packet_p);

    /* Get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation_p, &control_qualifier);

    /* if any of the subpacket failed with bad status then update the master status. */
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(master_packet_p, status, 0);
        /* only control operation status is processed later */
        fbe_payload_control_set_status(master_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }

    /* if any of the subpacket failed with failure status then update the master status. */
    /*!@todo we can use error predence here if required. */
    if(control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_payload_control_set_status(master_control_operation_p, control_status);
        fbe_payload_control_set_status_qualifier(master_control_operation_p, control_qualifier);
    }

    /* The subpacket was successful. Update the rg nonpaged with the subpacket's information */
	if((status == FBE_STATUS_OK) &&
       (control_status == FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        /* get the control buffer of the subpacket. */
        fbe_payload_control_get_buffer(control_operation_p, &drive_info_p);
	
        /* get the edge index of the subpacket. */
        edge_index_p = (fbe_edge_index_t *) (((fbe_u8_t *) drive_info_p) + sizeof(fbe_physical_drive_mgmt_get_drive_information_t));

        /* update the master packet with the drive's tier type. */
        drive_tier_downstream_p->drive_tier[*edge_index_p] = fbe_database_determine_drive_performance_tier_type(drive_info_p);

        fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_DRIVE_TIER_TRACING,
                             "%s drive tier of index: 0x%x returned with value: 0x%x\n", 
                             __FUNCTION__, *edge_index_p, drive_tier_downstream_p->drive_tier[*edge_index_p]);

    }


    /* remove the sub packet from master queue. */ 
    fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);

    /* when the queue is empty, all subpackets have completed. */
    if(is_empty) {
        fbe_transport_destroy_subpackets(master_packet_p);
        /* remove the master packet from terminator queue and complete it. */
        //fbe_base_object_remove_from_terminator_queue((fbe_base_object_t *) raid_group_p, master_packet_p);
 
        fbe_memory_free_request_entry(&master_packet_p->memory_request);
        fbe_transport_complete_packet(master_packet_p);
    }
    else
    {
        /* Not done with processing sub packets.
         */
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}

/******************************************************************************
 * end fbe_raid_group_query_drive_tier_subpacket_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_raid_group_usurper_set_raid_attribute()
 ******************************************************************************
 * @brief
 *  Set the an attribute on the raid group's raid geometry. 
 *  For example, the vault attribute was set on a user raid group to mask
 *  this raid group as the vault raid group for testing
 *
 * @param context  - raid group.
 * @param packet_p - pointer to the packet
 *
 * @return status 
 *
 * @author 
 *   2/6/2014 - Created. Deanna Heng
 ******************************************************************************/
fbe_status_t fbe_raid_group_usurper_set_raid_attribute(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p)
{ 

    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_geometry_t *raid_geometry_p = NULL;
    fbe_raid_group_set_raid_attributes_t*   set_attribute_flags_p = NULL;

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p, packet_p, 
                                              sizeof(fbe_raid_group_set_raid_attributes_t),
                                              (fbe_payload_control_buffer_t)&set_attribute_flags_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    fbe_raid_geometry_set_attribute(raid_geometry_p,
                                    set_attribute_flags_p->raid_attribute_flags);

    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}
/******************************************************************************
 * end fbe_raid_group_usurper_set_raid_attribute()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_raid_group_usurper_get_drive_pos_by_server_id()
 ******************************************************************************
 * @brief
 *  This function returns the position of the drive in the RG based on the passed
 * in server ID
 *
 * @param context  - raid group.
 * @param packet_p - pointer to the packet
 *
 * @return status 
 *
 * @author 
 *   5/5/2014 - Created. Ashok Tamilarasan
 ******************************************************************************/
static fbe_status_t fbe_raid_group_usurper_get_drive_pos_by_server_id(fbe_raid_group_t *raid_group_p, fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_raid_group_get_disk_pos_by_server_id_t*   server_id_info = NULL;
    fbe_payload_control_operation_t            *control_operation_p = NULL;
    fbe_payload_ex_t                           *sep_payload_p = NULL;

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    /* get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p, packet_p, 
                                                       sizeof(fbe_raid_group_get_disk_pos_by_server_id_t),
                                                       (fbe_payload_control_buffer_t)&server_id_info);
    if (status != FBE_STATUS_OK){ 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    server_id_info->position = FBE_EDGE_INDEX_INVALID;
    status = fbe_raid_group_get_disk_pos_by_server_id(raid_group_p, 
                                                      server_id_info->server_id,
                                                      &server_id_info->position);
    if(server_id_info->position == FBE_EDGE_INDEX_INVALID)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_rg_get_server_id: Disk Position invalid for Object ID:0x%x \n",
                              server_id_info->server_id);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    } else {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}

/*!****************************************************************************
 *          fbe_raid_group_usurper_get_emeh_completion()
 ****************************************************************************** 
 * 
 * @brief   This is the completion function after we have sent all the:
 *          `Get DIEH Media Thresholds' downstream.  The data for each position
 *          has been populated.  Now we need to update the EMEH reliability
 *          for this raid group.
 * 
 * @param   packet_p - pointer to the packet
 * @param   context - pointer to raid group object to get thresholds for
 *
 * @return status 
 *
 * @author 
 *  05/07/2015  Ron Proulx - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_raid_group_usurper_get_emeh_completion(fbe_packet_t *packet_p,
                                                               fbe_packet_completion_context_t context)
{
    fbe_status_t                                        status = FBE_STATUS_OK;
    fbe_raid_group_t                                   *raid_group_p = NULL;
    fbe_u32_t                                           width;
    fbe_u32_t                                           position;
    fbe_raid_group_get_extended_media_error_handling_t *get_emeh_info_p = NULL;
    fbe_payload_control_operation_t                    *control_operation_p = NULL;
    fbe_payload_ex_t                                   *sep_payload_p = NULL;
    fbe_raid_emeh_reliability_t                         emeh_reliability = FBE_RAID_EMEH_RELIABILITY_UNKNOWN;
    fbe_u32_t                                           paco_position = FBE_EDGE_INDEX_INVALID; 

    /* Get the raid group
     */
    raid_group_p = (fbe_raid_group_t *)context;
    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    /* get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p, packet_p, 
                                                       sizeof(fbe_raid_group_get_extended_media_error_handling_t),
                                                       (fbe_payload_control_buffer_t)&get_emeh_info_p);
    if (status != FBE_STATUS_OK){ 
        fbe_transport_set_status(packet_p, status, 0);
        return status; 
    }

    /* First locate the proactive copy position so that it is ignored when
     * determining the overall raid group drive reliability.
     */
    fbe_raid_group_get_width(raid_group_p, &width);
    for (position = 0; position < width; position++)
    {
        /*! @note If the `paco_position' is FBE_EDGE_INDEX_INVALID then set it.
         *        There should only one (1) position that is in proactive copy.
         */
        if (get_emeh_info_p->vd_paco_position[position] != FBE_EDGE_INDEX_INVALID)
        {
            /* Can only be one but trace and continue.
             */
            if (paco_position != FBE_EDGE_INDEX_INVALID)
            {
                fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "get emeh pos: %d with paco pos: %d already set \n",
                                      position, paco_position);
            }
            else
            {
                /* Else set the proactive copy position.
                 */
                paco_position = get_emeh_info_p->vd_paco_position[position];
            }
        }
    }

    /* Walk thru the responses and update the EMEH reliability for this raid 
     * group. Then complete the packet.
     */
    for (position = 0; position < width; position++)
    {
        /* Don't look at positions with no response.  Also ignore the
         * proactive copy position (if any).
         */
        if ((get_emeh_info_p->dieh_reliability[position] != FBE_U32_MAX) &&
            (position != paco_position)                                     )
        {
            /*! @note If the drive reliability is not unknown, very high or high
             *        then reduce the EMEH reliability.
             */
            if (get_emeh_info_p->dieh_reliability[position] != FBE_DRIVE_MEDIA_RELIABILITY_UNKNOWN)
            {
                /* Use method that determine reliability fro mdrive DIEH reliability.
                 */
                fbe_raid_group_emeh_get_reliability_from_dieh_info(raid_group_p,
                                                                   get_emeh_info_p->dieh_reliability[position],
                                                                   &emeh_reliability);
            }
        }
    }

    /* If the reliability or proactive copy position has changed generate an 
     * information message.
     */
    if ((emeh_reliability != get_emeh_info_p->reliability) ||
        (paco_position != get_emeh_info_p->paco_position)     )
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "get emeh mode: %d command: %d orig reliability: %d current: %d paco pos: %d\n", 
                              get_emeh_info_p->current_mode, get_emeh_info_p->active_command,
                              get_emeh_info_p->reliability, emeh_reliability,
                              paco_position);

        /* Update the reliability and if this is a proactive copy request, update
         * proactive copy position.
         */
        fbe_raid_group_lock(raid_group_p);
        fbe_raid_group_set_emeh_reliability(raid_group_p, emeh_reliability);
        if (get_emeh_info_p->active_command == FBE_RAID_EMEH_COMMAND_PACO_STARTED)
        {
            fbe_raid_group_set_emeh_paco_position(raid_group_p, paco_position);
        }
        get_emeh_info_p->reliability = emeh_reliability;
        get_emeh_info_p->paco_position = paco_position;
        fbe_raid_group_unlock(raid_group_p);
    }
    else
    {
        /* If enabled trace the result.
         */
        fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_RAID_GROUP_DEBUG_FLAG_EXTENDED_MEDIA_ERROR_HANDLING,
                             "get emeh mode: %d enabled mode: %d active command: %d reliability: %d paco pos: %d\n",
                             get_emeh_info_p->current_mode, get_emeh_info_p->enabled_mode,
                             get_emeh_info_p->active_command, get_emeh_info_p->reliability,
                             get_emeh_info_p->paco_position);
    }

    /* Complete the request*/
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    return FBE_STATUS_OK;
}
/*************************************************** 
 * end fbe_raid_group_usurper_get_emeh_completion()
 ***************************************************/

/*!****************************************************************************
 *          fbe_raid_group_usurper_get_extended_media_error_handling()
 ****************************************************************************** 
 * 
 * @brief   This function returns the current values for the Extended Media
 *          Error Handling (EMEH) for the raid group specified.
 * 
 * @param   raid_group_p - pointer to raid group object to get thresholds for
 * @param   packet_p - pointer to the packet
 *
 * @return status 
 *
 * @author 
 *  05/07/2015  Ron Proulx - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_raid_group_usurper_get_extended_media_error_handling(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_status_t                                        status = FBE_STATUS_OK;
    fbe_lifecycle_state_t                               lifecycle_state;
    fbe_u32_t                                           width;
    fbe_u32_t                                           position;
    fbe_raid_position_bitmask_t                         rl_bitmask;
    fbe_raid_position_bitmask_t                         positions_to_send_to;  
    fbe_raid_group_get_extended_media_error_handling_t *get_emeh_info_p = NULL;
    fbe_payload_control_operation_t                    *control_operation_p = NULL;
    fbe_payload_ex_t                                   *sep_payload_p = NULL;
    fbe_physical_drive_get_dieh_media_threshold_t       local_dieh;

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    /* get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p, packet_p, 
                                                       sizeof(fbe_raid_group_get_extended_media_error_handling_t),
                                                       (fbe_payload_control_buffer_t)&get_emeh_info_p);
    if (status != FBE_STATUS_OK){ 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }
   
    /* Can only send this is raid group is ready.
     */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)raid_group_p, &lifecycle_state);
    if (lifecycle_state != FBE_LIFECYCLE_STATE_READY)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s - lifecycle state: %d is not READY\n",
                              __FUNCTION__, lifecycle_state);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    /* Invoke the method that will update the raid group values to the class values
     * as neccessary.
     */
    fbe_raid_group_is_extended_media_error_handling_capable(raid_group_p);

    /* Simply send the request to all positions.  First populate the
     * raid group class information.
     */
    fbe_raid_group_get_width(raid_group_p, &width);
    fbe_raid_group_lock(raid_group_p);
    get_emeh_info_p->width = width;
    fbe_raid_group_get_emeh_enabled_mode(raid_group_p, &get_emeh_info_p->enabled_mode);
    fbe_raid_group_get_emeh_current_mode(raid_group_p, &get_emeh_info_p->current_mode);
    fbe_raid_group_emeh_get_class_default_increase(&get_emeh_info_p->default_threshold_increase);
    fbe_raid_group_get_emeh_reliability(raid_group_p, &get_emeh_info_p->reliability);
    fbe_raid_group_get_emeh_request(raid_group_p, &get_emeh_info_p->active_command);
    fbe_raid_group_get_emeh_paco_position(raid_group_p, &get_emeh_info_p->paco_position);
    fbe_raid_group_get_rb_logging_bitmask(raid_group_p, &rl_bitmask);
    fbe_raid_group_get_valid_bitmask(raid_group_p, &positions_to_send_to);
    fbe_raid_group_unlock(raid_group_p);
    for (position = 0; position < width; position++)
    {
        /* Initialize to invalid.
         */
        get_emeh_info_p->dieh_reliability[position] = FBE_U32_MAX;
        get_emeh_info_p->dieh_mode[position] = FBE_U32_MAX;
        get_emeh_info_p->media_weight_adjust[position] = FBE_U32_MAX;
        get_emeh_info_p->vd_paco_position[position] = FBE_EDGE_INDEX_INVALID;
    }

    /* Set the completion function to return to caller.
     */
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_usurper_get_emeh_completion, raid_group_p);

    /*!  Allocate and populate the `send usurper to position' control operation
     *   from the packet.
     *
     *   @note If you need a buffer to track each position etc. include the
     *         size required here. 
     */
    local_dieh.dieh_mode = 0;
    local_dieh.dieh_reliability = 0;
    local_dieh.media_weight_adjust = 0;
    local_dieh.vd_paco_position = FBE_EDGE_INDEX_INVALID;
    positions_to_send_to &= ~rl_bitmask;
    status = fbe_raid_group_build_send_to_downstream_positions(raid_group_p, packet_p, positions_to_send_to, 
                                                               sizeof(fbe_physical_drive_get_dieh_media_threshold_t));
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: build send downstream failed - status: %d\n",
                              __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /*! Now send the request to each non-degraded position and call the emeh
     *  completion for each.
     *
     *   @note If you need a buffer to track each position etc. Need to add
     *         control operation here.
     */
    fbe_raid_group_send_to_downstream_positions(raid_group_p, packet_p,
                                                FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DIEH_MEDIA_THRESHOLD,
                                                (fbe_u8_t *)&local_dieh,
                                                sizeof(fbe_physical_drive_get_dieh_media_threshold_t),
                                                (fbe_packet_completion_function_t)fbe_raid_group_emeh_get_dieh_completion,
                                                (fbe_packet_completion_context_t)get_emeh_info_p /* Context for get */);

    /* Completion will be invoked.
     */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************************************
 * end fbe_raid_group_usurper_get_extended_media_error_handling()
 **************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_usurper_set_extended_media_error_handling()
 ****************************************************************************** 
 * 
 * @brief   This function sets the current values for the Extended Media
 *          Error Handling (EMEH).  These EMEH values are sent to any remaining
 *          position in a raid group when one or more positions are failed/removed.
 *          If `b_persist' is set it also persist the values using the registy.
 * 
 * @param   raid_group_p - pointer to raid group object to increase thresholds
 *              for
 * @param   packet_p - pointer to the packet
 *
 * @return status 
 *
 * @author 
 *  03/11/2015  Ron Proulx - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_raid_group_usurper_set_extended_media_error_handling(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_status_t                                        status;
    fbe_raid_group_set_extended_media_error_handling_t *set_emeh_info_p = NULL;
    fbe_payload_ex_t                                   *sep_payload_p = NULL;
    fbe_payload_control_operation_t                    *control_operation_p = NULL;
    fbe_raid_emeh_mode_t                                emeh_enabled_mode = FBE_RAID_EMEH_MODE_DEFAULT;
    fbe_raid_emeh_mode_t                                emeh_class_mode = FBE_RAID_EMEH_MODE_DEFAULT;
    fbe_raid_emeh_command_t                             emeh_request = FBE_RAID_EMEH_COMMAND_INVALID;

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    /* get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p, packet_p, 
                                                       sizeof(fbe_raid_group_set_extended_media_error_handling_t),
                                                       (fbe_payload_control_buffer_t)&set_emeh_info_p);
    if (status != FBE_STATUS_OK)
    { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }


    /* If there is already an EMEH request in progress simply fail this request.
     * This is technically not an error but we generate an error trace to indicate
     * that the EMEH mode was never set.
     */
    fbe_raid_group_lock(raid_group_p);
    fbe_raid_group_get_emeh_request(raid_group_p, &emeh_request);
    if (emeh_request != FBE_RAID_EMEH_COMMAND_INVALID)
    {
        /* Simply trace a warning and return busy.
         */
        fbe_raid_group_unlock(raid_group_p);
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "set emeh control: %d failed request: %d already in progress \n",
                              set_emeh_info_p->set_control, emeh_request);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_BUSY);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    /* Populate the raid group with the necessary information.
     */
    fbe_raid_group_get_emeh_enabled_mode(raid_group_p, &emeh_enabled_mode);
    fbe_raid_group_emeh_get_class_current_mode(&emeh_class_mode);
    fbe_raid_group_set_emeh_request(raid_group_p, set_emeh_info_p->set_control);
    fbe_raid_group_unlock(raid_group_p);

    /* Determine which condition to signal
     */
    switch(set_emeh_info_p->set_control)
    {
        case FBE_RAID_EMEH_COMMAND_RAID_GROUP_DEGRADED:
        case FBE_RAID_EMEH_COMMAND_PACO_STARTED:
            /* For internally generated request EMEH must be enabled at the class.
             */
            if (!fbe_raid_group_is_extended_media_error_handling_capable(raid_group_p))
            {
                /* If this raid group doesn't support EMEH trace a informational message and
                 * return success.
                 */
                fbe_raid_group_lock(raid_group_p);
                fbe_raid_group_set_emeh_request(raid_group_p, FBE_RAID_EMEH_COMMAND_INVALID);
                fbe_raid_group_unlock(raid_group_p);
                fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
                fbe_raid_group_trace(raid_group_p,
                             FBE_TRACE_LEVEL_INFO,
                             FBE_RAID_GROUP_DEBUG_FLAG_EXTENDED_MEDIA_ERROR_HANDLING,
                             "set emeh control: %d EMEH is not supported\n",
                             set_emeh_info_p->set_control);
                fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(packet_p);
                return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
            }

            /* Set the condition for EMEH.
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "set emeh control: %d set emeh condition\n",
                                  set_emeh_info_p->set_control);
            fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*)raid_group_p,
                                   FBE_RAID_GROUP_LIFECYCLE_COND_EMEH_REQUEST);
            break;

        case FBE_RAID_EMEH_COMMAND_RESTORE_NORMAL_MODE:
            /* Restore can be either internally or user generated.  In either case EMEH
             * must be enabled for this raid group.
             */
            if ((emeh_enabled_mode == FBE_RAID_EMEH_MODE_INVALID)  ||
                (emeh_enabled_mode == FBE_RAID_EMEH_MODE_DISABLED)    )
            {
                /* If this request was internally generated (i.e. b_is_paco) then
                 * it is not an error.  If it is a user request it is an error.
                 *
                 */
                fbe_raid_group_lock(raid_group_p);
                fbe_raid_group_set_emeh_request(raid_group_p, FBE_RAID_EMEH_COMMAND_INVALID);
                fbe_raid_group_unlock(raid_group_p);

                /* If the request is from paco completing then just return success.
                 */
                if (set_emeh_info_p->b_is_paco)
                {
                    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
                }
                else
                {
                    /* Else the user attempted a restore without EMEH being enabled.
                     */
                    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_INFO,
                                          "set emeh control: %d EMEH is not supported\n",
                                          set_emeh_info_p->set_control);
                }
                fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(packet_p);
                return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
            }

            /* Set the condition for EMEH.
             */
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "set emeh control: %d set emeh condition\n",
                                  set_emeh_info_p->set_control);
            fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*) raid_group_p,
                                   FBE_RAID_GROUP_LIFECYCLE_COND_EMEH_REQUEST);
            break;

        case FBE_RAID_EMEH_COMMAND_INCREASE_THRESHOLDS:
            /* This is a user request to increase the thresholds.  If EMEH is not enabled
             * for this raid group enable it.
             */
            if (!fbe_raid_group_is_extended_media_error_handling_capable(raid_group_p))
            {
                /* Enable EMEH for this raid group.
                 */
                fbe_raid_group_lock(raid_group_p);
                fbe_raid_group_set_emeh_enabled_mode(raid_group_p, FBE_RAID_EMEH_MODE_THRESHOLDS_INCREASED);
                fbe_raid_group_unlock(raid_group_p);
            }

            /* Set the condition to send dieh usurper to all positions.
             */
            if (set_emeh_info_p->b_change_threshold)
            {
                /* We have been asked to change the thresholds.*/
                fbe_raid_group_lock(raid_group_p);
                fbe_raid_group_emeh_set_class_current_increase(set_emeh_info_p->percent_threshold_increase,
                                                               set_emeh_info_p->set_control);
                fbe_raid_group_unlock(raid_group_p);
            }
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "set emeh control: %d set emeh condition\n",
                                  set_emeh_info_p->set_control);
            fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*)raid_group_p,
                                   FBE_RAID_GROUP_LIFECYCLE_COND_EMEH_REQUEST);
            break;
            
        case FBE_RAID_EMEH_COMMAND_DISABLE_EMEH:
            /* Request to disable EMEH for this raid group.
             */
            fbe_raid_group_lock(raid_group_p);
            fbe_raid_group_set_emeh_enabled_mode(raid_group_p, FBE_RAID_EMEH_MODE_DISABLED);
            fbe_raid_group_set_emeh_request(raid_group_p, FBE_RAID_EMEH_COMMAND_INVALID);
            fbe_raid_group_unlock(raid_group_p);
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "set emeh control: %d EMEH has been disabled for this raid group\n", 
                          set_emeh_info_p->set_control);
            break;

        case FBE_RAID_EMEH_COMMAND_ENABLE_EMEH:
            /* Request to enable EMEH for this raid group.
             */
            fbe_raid_group_lock(raid_group_p);
            fbe_raid_group_set_emeh_enabled_mode(raid_group_p, FBE_RAID_EMEH_MODE_ENABLED_NORMAL);
            fbe_raid_group_set_emeh_request(raid_group_p, FBE_RAID_EMEH_COMMAND_INVALID);
            fbe_raid_group_unlock(raid_group_p);
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_WARNING,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "set emeh control: %d EMEH has been enabled for this raid group\n", 
                          set_emeh_info_p->set_control);
            break;

        case FBE_RAID_EMEH_COMMAND_INVALID:
        default:
            /* These commands are not supported.*/
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s - unsupported emeh control: %d \n",
                              __FUNCTION__, set_emeh_info_p->set_control);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            break;
    }

    /* Now set the emeh for this raid group
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_raid_group_trace(raid_group_p,
                         FBE_TRACE_LEVEL_INFO,
                         FBE_RAID_GROUP_DEBUG_FLAG_EXTENDED_MEDIA_ERROR_HANDLING,
                         "set emeh control: %d is paco: %d vd obj: 0x%x change increase: %d percent increase: %d\n", 
                         set_emeh_info_p->set_control, set_emeh_info_p->b_is_paco, set_emeh_info_p->paco_vd_object_id,
                         set_emeh_info_p->b_change_threshold, 
                         set_emeh_info_p->percent_threshold_increase);

    /* Complete the packet
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************************************
 * end fbe_raid_group_usurper_set_extended_media_error_handling()
 **************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_usurper_set_debug_state()
 ****************************************************************************** 
 * 
 * @brief   This function sets various fields, conditions in the specified raid
 *          group object for debugging and testing purposes.  Currently the
 *          fields and conditions that can be set are:
 *              o Local state
 *              o Set any raid group condition
 *              o Set any local clustered raid group flag
 * 
 * @param   raid_group_p - pointer to raid group object to increase thresholds
 *              for
 * @param   packet_p - pointer to the packet
 *
 * @return  status
 * 
 * @note    This is for debugging and test purposes only.  It should not be used
 *          by field service etc.
 *
 * @author 
 *  08/05/2015  Ron Proulx - Created.
 * 
 ******************************************************************************/
static fbe_status_t fbe_raid_group_usurper_set_debug_state(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_raid_group_set_debug_state_t   *set_debug_state_p = NULL;
    fbe_payload_ex_t                   *sep_payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_class_id_t                      class_id = FBE_CLASS_ID_RAID_GROUP;

    FBE_RAID_GROUP_TRACE_FUNC_ENTRY(raid_group_p);

    /* get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p, packet_p, 
                                                       sizeof(fbe_raid_group_set_debug_state_t),
                                                       (fbe_payload_control_buffer_t)&set_debug_state_p);
    if (status != FBE_STATUS_OK)
    { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    /* First validate the requested values. Validate the local state mask.
     */
    if ((set_debug_state_p->local_state_mask != 0ULL)                                                          &&
        ((set_debug_state_p->local_state_mask & ~((fbe_u64_t)(FBE_RAID_GROUP_LOCAL_STATE_ALL_MASKS))) != 0ULL)    )
    {
        /* Generate an error trace and complete the request.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "set debug state: local state mask: 0x%llx is not valid\n",
                              set_debug_state_p->local_state_mask);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    /* Validate the clustered flags.
     */
    if ((set_debug_state_p->clustered_flags != 0ULL)                              &&
        ((set_debug_state_p->clustered_flags & 
         ~((fbe_u64_t)(FBE_RAID_GROUP_CLUSTERED_FLAG_ALL_MASKS | 
                       FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS_MASK | 
                       FBE_RAID_GROUP_CLUSTERED_FLAG_CHANGE_CONFIG_MASK))) != 0ULL)    )
    {
        /* Generate an error trace and complete the request.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "set debug state: clustered flags: 0x%llx is not valid\n",
                              set_debug_state_p->clustered_flags);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    /* Validate the condition.
     */
    if ((set_debug_state_p->raid_group_condition != FBE_LIFECYCLE_INVALID_COND)                                 &&
        (set_debug_state_p->raid_group_condition >= FBE_LIFECYCLE_COND_MAX(FBE_RAID_GROUP_LIFECYCLE_COND_LAST))    )
    {
        /* Generate an error trace and complete the request.
         */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "set debug state: raid group condition: %d is invalid\n",
                              set_debug_state_p->raid_group_condition);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    /* Take the raid group lock and set th requested values.
     */
    fbe_raid_group_lock(raid_group_p);

    /* Set the local state mask.
     */
    if (set_debug_state_p->local_state_mask != 0ULL)
    {
        fbe_raid_group_set_local_state(raid_group_p, 
                                       (fbe_raid_group_local_state_t)set_debug_state_p->local_state_mask);
    }

    /* Set the clustered flags.
     */
    if (set_debug_state_p->clustered_flags != 0ULL)
    {
        status = fbe_base_config_set_clustered_flag((fbe_base_config_t *)raid_group_p, 
                                                    (fbe_base_config_clustered_flags_t)set_debug_state_p->clustered_flags);
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_group_unlock(raid_group_p);
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "set debug state: set clustered flags: 0x%llx failed - status: %d\n",
                                  set_debug_state_p->clustered_flags, status);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
        }
    }

    /* Set the raid group condition and release the lock.
     */
    if (set_debug_state_p->raid_group_condition != FBE_LIFECYCLE_INVALID_COND)
    {
        /* Change into a valid raid group condition.
         */
        set_debug_state_p->raid_group_condition += FBE_LIFECYCLE_COND_FIRST_ID(class_id);
        status = fbe_lifecycle_set_cond(&fbe_raid_group_lifecycle_const, (fbe_base_object_t*)raid_group_p,
                                        set_debug_state_p->raid_group_condition);
        if (status != FBE_STATUS_OK)
        {
            fbe_raid_group_unlock(raid_group_p);
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "set debug state: set raid group condition: 0x%x failed - status: %d\n",
                                  set_debug_state_p->raid_group_condition, status);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
        }
    }
    fbe_raid_group_unlock(raid_group_p);

    /* Trace some information and return success.
     */
    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "set debug state: local state mask: 0x%llx clustered flags: 0x%llx raid group condition: 0x%x success\n",
                          set_debug_state_p->local_state_mask,
                          set_debug_state_p->clustered_flags,
                          set_debug_state_p->raid_group_condition);

    /* Complete the packet
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**********************************************
 * end fbe_raid_group_usurper_set_debug_state()
 **********************************************/

/*!****************************************************************************
 *          fbe_raid_group_usurper_get_wear_level_downstream()
 ****************************************************************************** 
 * 
 * @brief   This function gets the max wear level collected from the drives
 *          in the raid group
 * 
 * @param   raid_group_p - pointer to raid group object to query
 *              for
 * @param   packet_p - pointer to the packet
 *
 * @return status 
 *
 * @author 
 *  06/251/2015  Deanna Heng - Created.
 * 
 ******************************************************************************/
fbe_status_t fbe_raid_group_usurper_get_wear_level_downstream(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_status_t                                status;
    fbe_u32_t                                   width;
    fbe_u32_t                                   index;
    fbe_base_config_memory_allocation_chunks_t  mem_alloc_chunks;
    fbe_raid_group_get_wear_level_downstream_t  *wear_level_downstream_p = NULL;
    fbe_payload_ex_t                            *sep_payload_p = NULL;
    fbe_payload_control_operation_t             *control_operation_p = NULL;


    /* get the sep payload and control operation/ */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the control buffer pointer from the packet payload. */
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                                       packet_p, 
                                                       sizeof(fbe_raid_group_get_wear_level_downstream_t),
                                                       (fbe_payload_control_buffer_t)&wear_level_downstream_p);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    /* initialize data zero before sending the request down. */
    wear_level_downstream_p->max_wear_level_percent = 0;
    wear_level_downstream_p->edge_index = 0;
    wear_level_downstream_p->max_erase_count = 0;
    wear_level_downstream_p->eol_PE_cycles = 0;
    wear_level_downstream_p->power_on_hours = 0;
    for(index = 0; index < FBE_RAID_MAX_DISK_ARRAY_WIDTH; index++)
    {
        wear_level_downstream_p->drive_wear_level[index].current_pe_cycle = 0;
        wear_level_downstream_p->drive_wear_level[index].max_pe_cycle = 0;
        wear_level_downstream_p->drive_wear_level[index].power_on_hours = 0;
    }


    /* get the width of the raid group object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* We need to send the request down to all the edges and so get the width and allocate
     * packet for every edge */
    /* Set the various counts and buffer size */
    mem_alloc_chunks.buffer_count = width;
    mem_alloc_chunks.number_of_packets = width;
    mem_alloc_chunks.sg_element_count = 0;
    mem_alloc_chunks.buffer_size = sizeof(fbe_provision_drive_get_ssd_statistics_t) + sizeof(fbe_edge_index_t);
    mem_alloc_chunks.alloc_pre_read_desc = FALSE;
    
    /* allocate the subpackets to collect the zero checkpoint for all the edges. */
    status = fbe_base_config_memory_allocate_chunks((fbe_base_config_t *) raid_group_p,
                                                    packet_p,
                                                    mem_alloc_chunks,
                                                    (fbe_memory_completion_function_t)fbe_raid_group_get_wear_level_allocation_completion); /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s memory allocation failed, status:0x%x\n", __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 

}
/**************************************************************
 * end fbe_raid_group_usurper_get_wear_level_downstream()
 **************************************************************/


/*!****************************************************************************
 * fbe_raid_group_get_wear_level_allocation_completion()
 ******************************************************************************
 * @brief
 *  This function gets called after memory has been allocated and sends the 
 *  request down to the the block edge
 *
 * @param memory_request_p  - memory request.
 * @param context           - context passed to acquire the memory.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  06/25/2015 - Created. Deanna Heng
 *
 ******************************************************************************/
static fbe_status_t
fbe_raid_group_get_wear_level_allocation_completion(fbe_memory_request_t * memory_request_p,
                                                        fbe_memory_completion_context_t context)
{
    fbe_packet_t                                   *packet_p = NULL;
    fbe_packet_t                                  **new_packet_p = NULL;
    fbe_u8_t                                      **buffer_p = NULL;
    fbe_payload_ex_t                               *new_sep_payload_p = NULL;
    fbe_payload_control_operation_t                *new_control_operation_p = NULL;
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_raid_group_t                               *raid_group_p = NULL;
    fbe_u32_t                                       width = 0;
    fbe_u32_t                                       index = 0;
    fbe_block_edge_t                               *block_edge_p = NULL;
    fbe_base_config_memory_alloc_chunks_address_t   mem_alloc_chunks_addr = {0};
    fbe_sg_element_t                               *pre_read_sg_list_p = NULL;
    fbe_payload_pre_read_descriptor_t              *pre_read_desc_p = NULL;
    fbe_packet_attr_t                               packet_attr;
    fbe_provision_drive_get_ssd_statistics_t       *drive_ssd_stats_p;
    fbe_edge_index_t                               *edge_index_p = NULL;
    
    /* get the pointer to original packet. */
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);

    /* get the pointer to raid group object. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE)
    {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: memory request: 0x%p failed. request state: %d \n",
                              __FUNCTION__, memory_request_p, memory_request_p->request_state);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    /* get the width of the base config object. */
    fbe_base_config_get_width((fbe_base_config_t *) raid_group_p, &width);

    /* subpacket control buffer size. */
    mem_alloc_chunks_addr.buffer_size = sizeof(fbe_provision_drive_get_ssd_statistics_t) + sizeof(fbe_edge_index_t);
    mem_alloc_chunks_addr.number_of_packets = width;
    mem_alloc_chunks_addr.buffer_count = width;
    mem_alloc_chunks_addr.sg_element_count = 0;
    mem_alloc_chunks_addr.sg_list_p = NULL;
    mem_alloc_chunks_addr.assign_pre_read_desc = FALSE;
    mem_alloc_chunks_addr.pre_read_sg_list_p = &pre_read_sg_list_p;
    mem_alloc_chunks_addr.pre_read_desc_p = &pre_read_desc_p;
    
    /* get subpacket pointer from the allocated memory. */
    status = fbe_base_config_memory_assign_address_from_allocated_chunks((fbe_base_config_t *) raid_group_p,
                                                                         memory_request_p,
                                                                         &mem_alloc_chunks_addr);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s memory distribution failed, status:0x%x, width:0x%x, buffer_size:0x%x\n",
                              __FUNCTION__, status, width, mem_alloc_chunks_addr.buffer_size);
        fbe_memory_free_request_entry(memory_request_p);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    }

    new_packet_p = mem_alloc_chunks_addr.packet_array_p;
    buffer_p = mem_alloc_chunks_addr.buffer_array_p;

    while(index < width)
    {
        /* Initialize the sub packets. */
        fbe_transport_initialize_sep_packet(new_packet_p[index]);
        new_sep_payload_p = fbe_transport_get_payload_ex(new_packet_p[index]);

        /* initialize the control buffer. */
        drive_ssd_stats_p = (fbe_provision_drive_get_ssd_statistics_t *) buffer_p[index];

        /* initialize the edge index as a context data. */
        edge_index_p = (fbe_edge_index_t *) (buffer_p[index] + sizeof(fbe_provision_drive_get_ssd_statistics_t));
        *edge_index_p = index;

        /* allocate and initialize the control operation. */
        new_control_operation_p = fbe_payload_ex_allocate_control_operation(new_sep_payload_p);
        fbe_payload_control_build_operation(new_control_operation_p,
                                            FBE_PROVISION_DRIVE_CONTROL_CODE_GET_SSD_STATISTICS,
                                            drive_ssd_stats_p,
                                            sizeof(fbe_provision_drive_get_ssd_statistics_t));

        /* Initialize the new packet with FBE_PACKET_FLAG_EXTERNAL same as the master packet */
        fbe_transport_get_packet_attr(packet_p, &packet_attr);
        if (packet_attr & FBE_PACKET_FLAG_EXTERNAL) 
        {
            fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_EXTERNAL);            
        }

        /* set completion functon. */
        fbe_transport_set_completion_function(new_packet_p[index],
                                              fbe_raid_group_get_wear_level_subpacket_completion,
                                              raid_group_p);

        /* add the subpacket to master packet queue. */
        fbe_transport_add_subpacket(packet_p, new_packet_p[index]);
        index++;
    }

    /* set the parent packet status to good before sending subpackets */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);

    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_DRIVE_TIER_TRACING,
                         "%s: raid_group: Send all subpackets for drive wear level query\n",
                         __FUNCTION__);

    /* send all the subpackets together. */
    for(index = 0; index < width; index++)
    {
        fbe_base_config_get_block_edge((fbe_base_config_t *)raid_group_p, &block_edge_p, index);
        fbe_transport_set_packet_attr(new_packet_p[index], FBE_PACKET_FLAG_TRAVERSE);
        fbe_block_transport_send_control_packet(block_edge_p, new_packet_p[index]);
    }

    /* send all the subpackets. */
     return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_raid_group_get_wear_level_allocation_completion()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_raid_group_get_wear_level_subpacket_completion()
 ******************************************************************************
 * @brief
 *  Handle the completion of a subpacket that was sent downstream to query for the 
 *  physical drive information. Update the drive information from to the subpacket to 
 *  the master packet.
 *
 * @param packet_p          - packet.
 * @param context           - context passed to request.
 *
 * @return status           - status of the operation.
 *
 * @author
 *  2/06/2014 - Created. Deanna Heng
 *
 ******************************************************************************/
static fbe_status_t 
fbe_raid_group_get_wear_level_subpacket_completion(fbe_packet_t * packet_p,
                                                   fbe_packet_completion_context_t context)
{
    fbe_raid_group_t *                              raid_group_p = NULL;
    fbe_packet_t *                                  master_packet_p = NULL;
    fbe_payload_ex_t *                              master_sep_payload_p = NULL;
    fbe_payload_control_operation_t *               master_control_operation_p = NULL;
    fbe_payload_ex_t *                              sep_payload_p = NULL;
    fbe_payload_control_operation_t *               control_operation_p = NULL;
    fbe_payload_control_status_t                    control_status;
    fbe_payload_control_status_qualifier_t          control_qualifier;
    fbe_bool_t                                      is_empty;
    fbe_status_t                                    status;
    fbe_raid_group_get_wear_level_downstream_t *    wear_level_downstream_p = NULL;
    fbe_provision_drive_get_ssd_statistics_t *      drive_ssd_stats_p = NULL;
    fbe_edge_index_t *                              edge_index_p = NULL;
    fbe_u64_t                                       max_pe_cycles = 20000;
    fbe_u32_t                                       max_wear_level_percent = 0;
	

    /* Cast the context to base config object pointer. */
    raid_group_p = (fbe_raid_group_t *) context;

    /* Get the master packet, its payload and associated control operation. */
    master_packet_p = fbe_transport_get_master_packet(packet_p);
    master_sep_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    master_control_operation_p = fbe_payload_ex_get_control_operation(master_sep_payload_p);

    /* get the control buffer of the master packet. */
    fbe_payload_control_get_buffer(master_control_operation_p, &wear_level_downstream_p);

     /* get the status of the subpacket operation. */
    status = fbe_transport_get_status_code(packet_p);

    /* Get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_status_qualifier(control_operation_p, &control_qualifier);

    /* if any of the subpacket failed with bad status then update the master status. */
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(master_packet_p, status, 0);
        /* only control operation status is processed later */
        fbe_payload_control_set_status(master_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }

    fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_DRIVE_TIER_TRACING,
                         "%s drive wear level control status: 0x%x qualifier 0x%x status 0x%x\n", 
                         __FUNCTION__, control_status, control_qualifier, status);



    /* if any of the subpacket failed with failure status then update the master status. */
    /*!@todo we can use error predence here if required. */
    if(control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)
    {
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_payload_control_set_status(master_control_operation_p, control_status);
        fbe_payload_control_set_status_qualifier(master_control_operation_p, control_qualifier);
    }

    /* The subpacket was successful. Update the rg nonpaged with the subpacket's information */
	if((status == FBE_STATUS_OK) &&
       (control_status == FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        /* get the control buffer of the subpacket. */
        fbe_payload_control_get_buffer(control_operation_p, &drive_ssd_stats_p);
	
        /* get the edge index of the subpacket. */
        edge_index_p = (fbe_edge_index_t *) (((fbe_u8_t *) drive_ssd_stats_p) + sizeof(fbe_provision_drive_get_ssd_statistics_t));

        if (drive_ssd_stats_p->eol_PE_cycles == 0)
        {
            drive_ssd_stats_p->eol_PE_cycles = max_pe_cycles;
        }

        fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_DRIVE_TIER_TRACING,
                             "drive wear index: 0x%x erase_count: 0x%llx eol pe cycles:0x%llx\n", 
                             *edge_index_p, drive_ssd_stats_p->max_erase_count, drive_ssd_stats_p->eol_PE_cycles);

        max_wear_level_percent = (fbe_u32_t)((100 * drive_ssd_stats_p->max_erase_count) / drive_ssd_stats_p->eol_PE_cycles);
        /* update the master packet with the drive's tier type. */
        wear_level_downstream_p->drive_wear_level[*edge_index_p].current_pe_cycle = drive_ssd_stats_p->max_erase_count;
        wear_level_downstream_p->drive_wear_level[*edge_index_p].max_pe_cycle = drive_ssd_stats_p->eol_PE_cycles;
        wear_level_downstream_p->drive_wear_level[*edge_index_p].power_on_hours = drive_ssd_stats_p->power_on_hours;
        fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_DRIVE_TIER_TRACING,
                             "drive wear level of index: 0x%x returned with value: 0x%x\n", 
                             *edge_index_p, max_wear_level_percent);


        if (max_wear_level_percent > wear_level_downstream_p->max_wear_level_percent ||
            wear_level_downstream_p->max_wear_level_percent == 0)
        {
            fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_DRIVE_TIER_TRACING,
                                 "max wear level changed 0x%x -> 0x%x. mec: 0x%llx pec: 0x%llx poh: 0x%llx\n", 
                                 wear_level_downstream_p->max_wear_level_percent, max_wear_level_percent,
                                 wear_level_downstream_p->max_erase_count, wear_level_downstream_p->eol_PE_cycles,
                                 wear_level_downstream_p->power_on_hours);
            wear_level_downstream_p->max_wear_level_percent = max_wear_level_percent;
            wear_level_downstream_p->edge_index = *edge_index_p;
            wear_level_downstream_p->max_erase_count = drive_ssd_stats_p->max_erase_count;
            wear_level_downstream_p->eol_PE_cycles = drive_ssd_stats_p->eol_PE_cycles;
            wear_level_downstream_p->power_on_hours = drive_ssd_stats_p->power_on_hours;

        }
        
    }


    /* remove the sub packet from master queue. */ 
    fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);

    /* when the queue is empty, all subpackets have completed. */
    if(is_empty) {
        fbe_transport_destroy_subpackets(master_packet_p);
        /* remove the master packet from terminator queue and complete it. */
        fbe_raid_group_trace(raid_group_p, FBE_TRACE_LEVEL_INFO, FBE_RAID_GROUP_DEBUG_FLAG_DRIVE_TIER_TRACING,
                                 "wear level subpacket queue is empty\n");

        fbe_memory_free_request_entry(&master_packet_p->memory_request);
        fbe_transport_complete_packet(master_packet_p);
    }
    else
    {
        /* Not done with processing sub packets.
         */
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
}

/******************************************************************************
 * end fbe_raid_group_get_wear_level_subpacket_completion()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_usurper_get_wear_level()
 ******************************************************************************
 *
 * @brief   This function is used to get the wear level
 *          information of the raid group
 *
 * @param   raid_group_p  - raid group object.
 * @param   packet_p      - Pointer to the packet.
 *
 * @return  status       - status of the operation.
 *
 * @author
 *  06/29/2015  Deanna Heng  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_usurper_get_wear_level(fbe_raid_group_t *raid_group_p,
                                                          fbe_packet_t *packet_p)
{
    fbe_status_t                                status;
    fbe_raid_group_get_wear_level_t            *wear_level_p = NULL;
    fbe_payload_control_operation_t            *control_operation_p = NULL;
    fbe_payload_ex_t                           *sep_payload_p = NULL;
    fbe_raid_geometry_t                        *raid_geometry_p = fbe_raid_group_get_raid_geometry(raid_group_p);

    /* get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the control buffer pointer from the packet payload. */
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                                       packet_p, 
                                                       sizeof(*wear_level_p),
                                                       (fbe_payload_control_buffer_t)&wear_level_p);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    /* only return values from the raid group if the raid group consists of ssd drives */
    if (fbe_raid_geometry_is_flag_set(raid_geometry_p, FBE_RAID_GEOMETRY_FLAG_MIRROR_SEQUENTIAL_DISABLED))
    {
        wear_level_p->current_pe_cycle = raid_group_p->current_pe_cycle;
        wear_level_p->max_pe_cycle = raid_group_p->max_pe_cycle;
        wear_level_p->power_on_hours = raid_group_p->power_on_hours;
    } 
    else 
    {
        wear_level_p->current_pe_cycle = 0;
        wear_level_p->max_pe_cycle = 0;
        wear_level_p->power_on_hours = 0;
    }

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
  
}
/******************************************************************************
 * end fbe_raid_group_usurper_get_wear_level()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_raid_group_usurper_set_lifecycle_timer_interval()
 ******************************************************************************
 *
 * @brief   This function sets the interval (s) of the lifcycle timer condition
 *
 * @param   raid_group_p  - raid group object.
 * @param   packet_p      - Pointer to the packet.
 *
 * @return  status       - status of the operation.
 *
 * @author
 *  07/07/2015  Deanna Heng  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_raid_group_usurper_set_lifecycle_timer_interval(fbe_raid_group_t *raid_group_p, fbe_packet_t *packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_lifecycle_timer_info_t                 *lifecycle_timer_info_p = NULL;
    fbe_payload_control_operation_t            *control_operation_p = NULL;
    fbe_payload_ex_t                           *sep_payload_p = NULL;
    fbe_raid_group_lifecycle_cond_id_t          lifecycle_condition = FBE_LIFECYCLE_COND_MAX_TIMER;

    /* get the control operation of the subpacket. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the control buffer pointer from the packet payload. */
    status = fbe_raid_group_usurper_get_control_buffer(raid_group_p,
                                                       packet_p, 
                                                       sizeof(*lifecycle_timer_info_p),
                                                       (fbe_payload_control_buffer_t)&lifecycle_timer_info_p);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    switch (lifecycle_timer_info_p->lifecycle_condition)
    {
        case FBE_RAID_GROUP_SET_LIFECYCLE_TIMER_COND_WEAR_LEVEL:
            lifecycle_condition = FBE_RAID_GROUP_LIFECYCLE_COND_WEAR_LEVEL;
            break;
        default:
            fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "set_lifecycle_timer: invalid raid group condition: 0x%X first: 0x%x last: 0x%x\n",
                                  lifecycle_timer_info_p->lifecycle_condition,
                                  FBE_RAID_GROUP_SET_LIFECYCLE_TIMER_COND_INVALID,
                                  FBE_RAID_GROUP_SET_LIFECYCLE_TIMER_COND_LAST); 
            fbe_transport_set_status(packet_p, status, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = fbe_lifecycle_set_interval_timer_cond(&FBE_LIFECYCLE_CONST_DATA(raid_group),
                                                   (fbe_base_object_t *)raid_group_p,
                                                   lifecycle_condition,
                                                   lifecycle_timer_info_p->interval);

    if (status != FBE_STATUS_OK) 
    { 
       fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                             FBE_TRACE_LEVEL_WARNING,
                             FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "Unable to set lifecycle timer. cond:0x%x interval:0x%x status:0x%x\n", 
                             lifecycle_condition,
                             lifecycle_timer_info_p->interval,
                             status);

        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    fbe_base_object_trace((fbe_base_object_t *)raid_group_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "Lifecycle timer interval set. cond:0x%x interval:0x%x\n", 
                          lifecycle_condition,
                          lifecycle_timer_info_p->interval);

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}
/******************************************************************************
 * end fbe_raid_group_usurper_set_lifecycle_timer_interval()
 ******************************************************************************/

/******************************
 * end fbe_raid_group_usurper.c
 ******************************/
