/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_provision_drive_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the provision_drive object code for the usurper.
 * 
 * @ingroup provision_drive_class_files
 * 
 * @version
 *   07/23/2009:  Created. Peter Puhov
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_object.h"
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_provision_drive_private.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "fbe_spare.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe_database.h"
#include "fbe_raid_group_object.h"
#include "fbe_raid_library.h"
#include "fbe_cmi.h"
#include "fbe_private_space_layout.h"
#include "fbe_notification.h"


/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
static fbe_status_t fbe_provision_drive_create_edge(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_provision_drive_send_attach_edge(fbe_provision_drive_t *provision_drive_p,
                                                         fbe_block_transport_control_attach_edge_t * attach_edge) ;

static fbe_status_t fbe_provision_drive_get_drive_info(fbe_provision_drive_t * provision_drive_p,
                                                       fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_map_lba_to_chunk(fbe_provision_drive_t * provision_drive_p,
                                                         fbe_packet_t * packet_p);

static fbe_status_t fbe_provision_drive_usurper_get_spare_info(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_usurper_get_drive_location(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_provision_drive_get_spare_drive_location_completion(fbe_packet_t * packet_p,
                                                                            fbe_packet_completion_context_t context);

static fbe_status_t fbe_provision_drive_get_spare_physical_drive_info_completion(fbe_packet_t * packet_p, 
                                                                                 fbe_packet_completion_context_t context);

static fbe_status_t fbe_provision_drive_get_zero_checkpoint(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet);
static fbe_status_t fbe_provision_drive_set_zero_checkpoint(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet);
static fbe_status_t fbe_provision_drive_initiate_disk_zeroing(fbe_provision_drive_t*   provision_drive, fbe_packet_t*            packet_p);

static fbe_status_t fbe_provision_drive_control_register_keys(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet);
static fbe_status_t fbe_provision_drive_get_miniport_key_handle(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet);

static fbe_status_t fbe_provision_drive_port_unregister_keys_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

static fbe_status_t fbe_provision_drive_get_verify_invalidate_checkpoint(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet);
static fbe_status_t fbe_provision_drive_set_verify_invalidate_checkpoint(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet);

static fbe_status_t fbe_provision_drive_attach_upstream_block_edge(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet);
static fbe_status_t fbe_provision_drive_attach_upstream_block_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_provision_drive_detach_upstream_block_edge(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet);
static fbe_status_t fbe_provision_drive_detach_upstream_block_edge_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);


static fbe_status_t fbe_provision_drive_control_metadata_paged_write(fbe_provision_drive_t*   provision_drive_p, fbe_packet_t* packet_p);
static fbe_status_t fbe_provision_drive_control_metadata_paged_write_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

static fbe_status_t fbe_provision_drive_set_background_priorities(fbe_provision_drive_t* provision_drive, fbe_packet_t * packet);
static fbe_status_t fbe_provision_drive_get_background_priorities(fbe_provision_drive_t* provision_drive, fbe_packet_t * packet);

static fbe_status_t fbe_provision_drive_control_get_metadata_memory(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_control_get_versioned_metadata_memory(fbe_provision_drive_t* provision_drive_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_provision_drive_usurper_set_debug_flags(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet);
static fbe_status_t fbe_provision_drive_usurper_set_eol_state(fbe_provision_drive_t* provision_drive,fbe_packet_t* packet_p);
static fbe_status_t fbe_provision_drive_usurper_clear_eol_state(fbe_provision_drive_t *provision_drive_p, fbe_packet_t *packet_p);
static fbe_status_t fbe_provision_drive_download_ack(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_get_download_info(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_get_health_check_info(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_get_opaque_data(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet);
static fbe_status_t fbe_provision_drive_get_pool_id(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet);
static fbe_status_t fbe_provision_drive_set_pool_id(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet);
static fbe_status_t fbe_provision_drive_disable_verify(fbe_provision_drive_t*              in_provision_drive_p);
static fbe_status_t fbe_provision_drive_enable_verify(fbe_provision_drive_t*              in_provision_drive_p);

static fbe_status_t fbe_provision_drive_control_metadata_paged_operation(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_control_metadata_paged_operation_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_clear_logical_errors(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_get_unconsumed_mark_range(fbe_provision_drive_t *provision_drive_p,
                                                                  fbe_lba_t *lba_p,
                                                                  fbe_block_count_t *blocks_p);
static fbe_status_t fbe_provision_drive_initiate_disk_zeroing_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_initiate_zeroing_allocation_completion(fbe_memory_request_t * memory_request_p,
                                                                               fbe_memory_completion_context_t context);
static fbe_status_t fbe_provision_drive_initiate_consumed_area_zeroing_allocation_completion(fbe_memory_request_t * memory_request_p,
                                                                                             fbe_memory_completion_context_t context);
// get control buffer from packet 
static fbe_status_t fbe_provision_drive_get_removed_timestamp(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet);
static fbe_status_t fbe_provision_drive_set_removed_timestamp(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet);
static fbe_status_t fbe_provision_drive_get_checkpoint(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_usurper_set_nonpaged_metadata(fbe_provision_drive_t *provision_drive_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_test_slf(fbe_provision_drive_t* in_provision_drive_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_test_slf_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_usurper_clear_drive_fault(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_usurper_get_spindown_qualified(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_update_logical_error_stats(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_usurper_set_percent_rebuilt(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_port_register_keys_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_port_register_keys_unregister_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t provision_drive_update_configuration_per_type(fbe_provision_drive_t *provision_drive_p, 
                                                                  fbe_packet_t * packet_p,
                                                                  fbe_update_pvd_type_t update_type, 
                                                                  database_object_entry_t *database_entry_p);
static fbe_status_t fbe_provision_drive_control_reconstruct_paged(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet);
static fbe_status_t fbe_provision_drive_usurper_validate_block_encryption(fbe_provision_drive_t *provision_drive, fbe_packet_t *packet_p);
static fbe_status_t 
fbe_provision_drive_validate_block_encryption_memory_completion(fbe_memory_request_t * memory_request_p, 
                                                                fbe_memory_completion_context_t in_context);
static fbe_status_t 
fbe_provision_drive_usurper_validate_block_encryption_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_u32_t fbe_provision_drive_calculate_memory_chunk_size(fbe_block_count_t blocks, 
                                                                 fbe_u32_t *chunks, 
                                                                 fbe_memory_chunk_size_t *chunk_size);

static fbe_status_t fbe_provision_drive_usurper_set_encryption_mode(fbe_provision_drive_t *provision_drive, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_set_scrub_complete_allocation_completion(fbe_memory_request_t * memory_request_p,
                                                                                 fbe_memory_completion_context_t context);
static fbe_status_t fbe_provision_drive_setup_consumed_area_zeroing(fbe_provision_drive_t*   provision_drive, fbe_packet_t* packet_p);
static fbe_status_t fbe_provision_drive_set_scrub_intent_allocation_completion(fbe_memory_request_t * memory_request_p,
                                                                               fbe_memory_completion_context_t context);
static fbe_status_t fbe_provision_drive_set_scrub_intent_bits(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_set_scrub_bits_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_usurper_get_key_info(fbe_provision_drive_t *provision_drive, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_write_default_paged_metadata_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_check_scrub_needed(fbe_packet_t * packet_p,
                                                           fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_usurper_quiesce(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_usurper_unquiesce(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_usurper_get_clustered_flags(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_usurper_disable_paged_cache(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_usurper_get_paged_cache_info(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_provision_drive_usurper_set_swap_pending(fbe_provision_drive_t *provision_drive_p, 
                                                                       fbe_packet_t *packet_p);
static fbe_status_t fbe_provision_drive_usurper_clear_swap_pending(fbe_provision_drive_t *provision_drive_p, 
                                                                        fbe_packet_t *packet_p);
static fbe_status_t fbe_provision_drive_usurper_get_swap_pending(fbe_provision_drive_t *provision_drive_p, 
                                                                       fbe_packet_t *packet_p);
static fbe_status_t fbe_provision_drive_usurper_set_eas_start(fbe_provision_drive_t *provision_drive_p, fbe_packet_t *packet_p);
static fbe_status_t fbe_provision_drive_usurper_set_eas_complete(fbe_provision_drive_t *provision_drive_p, fbe_packet_t *packet_p);
static fbe_status_t fbe_provision_drive_usurper_get_eas_state(fbe_provision_drive_t *provision_drive_p, fbe_packet_t *packet_p);
static fbe_status_t fbe_provision_drive_metadata_clr_sniff_pass_persist_completion(fbe_packet_t * packet_p, 
                                                                       fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_get_unconsumed_mark_range(fbe_provision_drive_t *provision_drive_p,
                                                                  fbe_lba_t *lba_p,
                                                                  fbe_block_count_t *blocks_p);
static fbe_status_t fbe_provision_drive_usurper_get_ssd_statitics_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                                                    fbe_memory_completion_context_t context);
static fbe_status_t fbe_provision_drive_usurper_get_ssd_statitics_completion(fbe_packet_t * packet_p, 
                                                                             fbe_packet_completion_context_t context);
static void fbe_provision_drive_usurper_parse_ssd_statistics(fbe_provision_drive_t * provision_drive_p, 
                                                             fbe_sg_element_t * sg_list, 
                                                             fbe_provision_drive_get_ssd_statistics_t * get_stats_p,
                                                             fbe_u16_t transfer_count);
static fbe_status_t fbe_provision_drive_usurper_get_ssd_block_limits(fbe_provision_drive_t *provision_drive_p, 
                                                                  fbe_packet_t *packet_p);
static fbe_status_t fbe_provision_drive_usurper_get_ssd_block_limits_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                                                    fbe_memory_completion_context_t context);
static fbe_status_t fbe_provision_drive_usurper_get_ssd_block_limits_completion(fbe_packet_t * packet_p, 
                                                                             fbe_packet_completion_context_t context);
static void fbe_provision_drive_usurper_parse_ssd_block_limits(fbe_provision_drive_t * provision_drive_p, 
                                                             fbe_sg_element_t * sg_list, 
                                                             fbe_provision_drive_get_ssd_block_limits_t * get_block_limits_p,
                                                             fbe_u16_t transfer_count);
static fbe_status_t fbe_provision_drive_usurper_set_unmap_enabled_disabled(fbe_provision_drive_t * provision_drive_p, 
                                                                           fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_usurper_set_wear_leveling_timer(fbe_provision_drive_t * provision_drive_p, 
                                                                        fbe_packet_t * packet_p);

static fbe_status_t fbe_provision_drive_propagate_block_geometry(fbe_provision_drive_t *provision_drive_p);

// clear sniff verify report on specified disk 
static fbe_status_t fbe_provision_drive_usurper_clear_verify_report
(
    fbe_provision_drive_t*              in_provision_drive_p,
    fbe_packet_t*                       in_packet_p
);

// get sniff verify report for specified disk 
static fbe_status_t fbe_provision_drive_usurper_get_verify_report
(
    fbe_provision_drive_t*              in_provision_drive_p,
    fbe_packet_t*                       in_packet_p
);

// get sniff verify status for specified disk 
static fbe_status_t fbe_provision_drive_usurper_get_verify_status
(
    fbe_provision_drive_t*              in_provision_drive_p,
    fbe_packet_t*                       in_packet_p
);

// set sniff verify checkpoint for specified disk 
static fbe_status_t fbe_provision_drive_usurper_set_verify_checkpoint
(
    fbe_provision_drive_t*              in_provision_drive_p,
    fbe_packet_t*                       in_packet_p
);

// handle sniff verify checkpoint update completion
static fbe_status_t fbe_provision_drive_usurper_set_verify_checkpoint_complete
(
    fbe_packet_t * in_packet_p, 
    fbe_packet_completion_context_t context
);

static fbe_status_t fbe_provision_drive_enable_background_zeroing_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);
static fbe_status_t fbe_provision_drive_get_slf_state(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_provision_drive_update_validation_all_completion(fbe_packet_t * packet_p,
                                                                         fbe_packet_completion_context_t context);

/*!***************************************************************
 * fbe_provision_drive_control_entry()
 ****************************************************************
 * @brief
 *  This function is the management packet entry point for
 *  operations to the provision_drive object.
 *
 * @param object_handle - The config handle.
 * @param packet_p - The packet that is arriving.
 *
 * @return fbe_status_t
 *
 ****************************************************************/
fbe_status_t 
fbe_provision_drive_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_provision_drive_t                  *provision_drive = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_p = NULL;
    fbe_payload_control_operation_opcode_t  opcode;
    fbe_edge_index_t                        upstream_edge_index;
    fbe_object_id_t                         upstream_object_id;
    fbe_block_edge_t                       *upstream_edge_p = NULL;

    if(object_handle == NULL){
        return  fbe_provision_drive_class_control_entry(packet_p);
    }

    provision_drive = (fbe_provision_drive_t *)fbe_base_handle_to_pointer(object_handle);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_opcode(control_p, &opcode);

    switch (opcode)
    {
        /* provision_drive specific handling here.
         */
        /* We have to overwrite create edge functionality, 
         * because we creating interpackage edges.
         */
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_CREATE_EDGE:
            status = fbe_provision_drive_create_edge(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PROVISION_DRIVE_INFO:
            status = fbe_provision_drive_get_drive_info(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_SET_CONFIGURATION:
            status = fbe_provision_drive_set_configuration(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_UPDATE_CONFIGURATION:
            /* update the provision drive configuration. */
            status = fbe_provision_drive_update_configuration(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_SPARE_INFO:
            /* it retrives required information from provision drive configuration and nonpaged metadata. */
            status = fbe_provision_drive_usurper_get_spare_info(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_DRIVE_LOCATION:
            /* it retrives required information from provision drive configuration and nonpaged metadata. */
            status = fbe_provision_drive_usurper_get_drive_location(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PHYSICAL_DRIVE_LOCATION:
            /* it collects required information from pdo object. */
            status = fbe_provision_drive_get_physical_drive_location(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_SET_ZERO_CHECKPOINT:
            status = fbe_provision_drive_set_zero_checkpoint(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_ZERO_CHECKPOINT:
            status = fbe_provision_drive_get_zero_checkpoint(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_REGISTER_KEYS:
        case FBE_BASE_CONFIG_CONTROL_CODE_UPDATE_DRIVE_KEYS:
            status = fbe_provision_drive_control_register_keys(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_UNREGISTER_KEYS:
            status = fbe_provision_drive_unregister_keys(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_MINIPORT_KEY_HANDLE:
            status = fbe_provision_drive_get_miniport_key_handle(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_RECONSTRUCT_PAGED:
            status = fbe_provision_drive_control_reconstruct_paged(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_SET_VERIFY_INVALIDATE_CHECKPOINT:
            status = fbe_provision_drive_set_verify_invalidate_checkpoint(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_VERIFY_INVALIDATE_CHECKPOINT:
            status = fbe_provision_drive_get_verify_invalidate_checkpoint(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_INITIATE_DISK_ZEROING:
            /* Initiate disk zeroing operation */
            status = fbe_provision_drive_initiate_disk_zeroing(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_INITIATE_CONSUMED_AREA_ZEROING:
            /* Initiate zeroing operation to the consumed area */
            status = fbe_provision_drive_setup_consumed_area_zeroing(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_CLEAR_VERIFY_REPORT:
            /* clear sniff verify report on specified disk */
            status = fbe_provision_drive_usurper_clear_verify_report(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_VERIFY_REPORT:
            /* get sniff verify report for specified disk */
            status = fbe_provision_drive_usurper_get_verify_report(provision_drive, packet_p );
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_VERIFY_STATUS:
            /* get sniff verify status for specified disk */
            status = fbe_provision_drive_usurper_get_verify_status(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_SET_VERIFY_CHECKPOINT:
            /* set sniff verify checkpoint for specified disk */
            status = fbe_provision_drive_usurper_set_verify_checkpoint(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_METADATA_PAGED_WRITE:
            status = fbe_provision_drive_control_metadata_paged_write(provision_drive, packet_p);
            break;

        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_ATTACH_EDGE:
            status = fbe_provision_drive_attach_upstream_block_edge(provision_drive, packet_p);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_DETACH_EDGE:
            status = fbe_provision_drive_detach_upstream_block_edge(provision_drive, packet_p);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_CLEAR_LOGICAL_ERRORS:
            status = fbe_provision_drive_clear_logical_errors(provision_drive, packet_p);
            break;
        case FBE_PROVISION_DRIVE_CONTROL_CODE_SET_BACKGROUND_PRIORITIES:
            status  = fbe_provision_drive_set_background_priorities(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_BACKGROUND_PRIORITIES:
            status  = fbe_provision_drive_get_background_priorities(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_METADATA_MEMORY:
            status = fbe_provision_drive_control_get_metadata_memory(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_VERSIONED_METADATA_MEMORY:
            status = fbe_provision_drive_control_get_versioned_metadata_memory(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_SET_DEBUG_FLAGS:
            status = fbe_provision_drive_usurper_set_debug_flags(provision_drive, packet_p);
            break;
        case FBE_PROVISION_DRIVE_CONTROL_CODE_SET_EOL_STATE:
            status = fbe_provision_drive_usurper_set_eol_state(provision_drive, packet_p);
            break;
        case FBE_PROVISION_DRIVE_CONTROL_CODE_CLEAR_EOL_STATE:
            status = fbe_provision_drive_usurper_clear_eol_state(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_DOWNLOAD_ACK:
            status = fbe_provision_drive_download_ack(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_DOWNLOAD_INFO:
            status = fbe_provision_drive_get_download_info(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_HEALTH_CHECK_INFO:
            status = fbe_provision_drive_get_health_check_info(provision_drive, packet_p);
            break;

        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_GET_BITS:
        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_SET_BITS:
        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_CLEAR_BITS:
        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_WRITE:
            status = fbe_provision_drive_control_metadata_paged_operation(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_OPAQUE_DATA:
            status = fbe_provision_drive_get_opaque_data(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_POOL_ID:
            status = fbe_provision_drive_get_pool_id(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_SET_POOL_ID:
            status = fbe_provision_drive_set_pool_id(provision_drive, packet_p);
            break;
    
        case FBE_PROVISION_DRIVE_CONTROL_CODE_SET_REMOVED_TIMESTAMP:
            status = fbe_provision_drive_set_removed_timestamp(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_REMOVED_TIMESTAMP:
            status = fbe_provision_drive_get_removed_timestamp(provision_drive, packet_p);
            break;
        /* This one is more of a stub for virtual drive get checkpoint for first 4 drives */
        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_CHECKPOINT:
            status = fbe_provision_drive_get_checkpoint(provision_drive, packet_p);
            break;
        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_SLF_STATE:
            status = fbe_provision_drive_get_slf_state(provision_drive, packet_p);
            break;
       
        /* This is for fixing object which is stuck in SPECIALIZED when NP load failed */
        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_SET_NONPAGED:
            status = fbe_provision_drive_usurper_set_nonpaged_metadata(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_TEST_SLF:
            status = fbe_provision_drive_test_slf(provision_drive, packet_p);
            break;
        case FBE_PROVISION_DRIVE_CONTROL_CODE_MAP_LBA_TO_CHUNK:
            status = fbe_provision_drive_map_lba_to_chunk(provision_drive, packet_p);
            break;
        case FBE_PROVISION_DRIVE_CONTROL_CODE_CLEAR_DRIVE_FAULT:
            status = fbe_provision_drive_usurper_clear_drive_fault(provision_drive, packet_p);
            break; 
        case FBE_BASE_CONFIG_CONTROL_CODE_GET_SPINDOWN_QUALIFIED:
            status = fbe_provision_drive_usurper_get_spindown_qualified(provision_drive, packet_p);
            break;
        /* this is control code to handle the logical error from raid object
         */
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_UPDATE_LOGICAL_ERROR_STATS:
            status = fbe_provision_drive_update_logical_error_stats(provision_drive, packet_p);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_EXIT_HIBERNATION:
            /* Get the upstream edge since the usurper will clear the upstream
             * hibernation attribute.
             */
            fbe_block_transport_find_first_upstream_edge_index_and_obj_id(&provision_drive->base_config.block_transport_server,
                                                                  &upstream_edge_index, &upstream_object_id);
            if (upstream_object_id != FBE_OBJECT_ID_INVALID)
            {
                upstream_edge_p = (fbe_block_edge_t *)fbe_base_transport_server_get_client_edge_by_client_id((fbe_base_transport_server_t *)&provision_drive->base_config.block_transport_server,
                                                                             upstream_object_id);
                packet_p->base_edge = (fbe_base_edge_t *)upstream_edge_p;
            }
            status = fbe_base_config_control_entry(object_handle, packet_p);
            break;
        case FBE_PROVISION_DRIVE_CONTROL_CODE_SET_PERCENT_REBUILT:
            /* Set the `percent rebuilt' for this provision drive.
             */
            status = fbe_provision_drive_usurper_set_percent_rebuilt(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_VALIDATE_BLOCK_ENCRYPTION:
            status = fbe_provision_drive_usurper_validate_block_encryption(provision_drive, packet_p);
            break;

        case FBE_BASE_CONFIG_CONTROL_CODE_SET_ENCRYPTION_MODE:
            status = fbe_provision_drive_usurper_set_encryption_mode(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_DEK_INFO:
            status = fbe_provision_drive_usurper_get_key_info(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_QUIESCE:
            status = fbe_provision_drive_usurper_quiesce(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_UNQUIESCE:
            status = fbe_provision_drive_usurper_unquiesce(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_CLUSTERED_FLAG:
            status = fbe_provision_drive_usurper_get_clustered_flags(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_DISABLE_PAGED_CACHE:
            status = fbe_provision_drive_usurper_disable_paged_cache(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PAGED_CACHE_INFO:
            status = fbe_provision_drive_usurper_get_paged_cache_info(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_SET_SWAP_PENDING:
            status = fbe_provision_drive_usurper_set_swap_pending(provision_drive, packet_p); 
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CLEAR_SWAP_PENDING:
            status = fbe_provision_drive_usurper_clear_swap_pending(provision_drive, packet_p); 
            break;

        case FBE_PROVISION_DRIVE_CONTROL_GET_SWAP_PENDING:
            status = fbe_provision_drive_usurper_get_swap_pending(provision_drive, packet_p); 
            break;

        case FBE_PROVISION_DRIVE_CONTROL_SET_EAS_START:
            status = fbe_provision_drive_usurper_set_eas_start(provision_drive, packet_p); 
            break;

        case FBE_PROVISION_DRIVE_CONTROL_SET_EAS_COMPLETE:
            status = fbe_provision_drive_usurper_set_eas_complete(provision_drive, packet_p); 
            break;

        case FBE_PROVISION_DRIVE_CONTROL_GET_EAS_STATE:
            status = fbe_provision_drive_usurper_get_eas_state(provision_drive, packet_p); 
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_UPDATE_BLOCK_SIZE:
            /* update the provision drive configuration. */
            status = fbe_provision_drive_update_block_size(provision_drive, packet_p);
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_SSD_STATISTICS:
            status = fbe_provision_drive_usurper_get_ssd_statitics(provision_drive, packet_p); 
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_GET_SSD_BLOCK_LIMITS:
            status = fbe_provision_drive_usurper_get_ssd_block_limits(provision_drive, packet_p); 
            break;

        case FBE_PROVISION_DRIVE_CONTROL_CODE_SET_UNMAP_ENABLED_DISABLED:
            status = fbe_provision_drive_usurper_set_unmap_enabled_disabled(provision_drive, packet_p); 
            break;

        default:

            /* Allow the  object to handle all other ioctls.
             */
            status = fbe_base_config_control_entry(object_handle, packet_p);

            /* If traverse flag is set then send the control packet down to 
             * block edge.
             */
            if (status == FBE_STATUS_TRAVERSE)
            {
                if ((provision_drive->base_config.block_edge_p->server_package_id == FBE_PACKAGE_ID_INVALID) ||
                    (provision_drive->base_config.block_edge_p->base_edge.server_id == FBE_OBJECT_ID_INVALID) ||
                    (provision_drive->base_config.block_edge_p->base_edge.path_state == FBE_PATH_STATE_INVALID))
                {
                    fbe_payload_ex_t *                     sep_payload_p = NULL;
                    fbe_payload_control_operation_t *       control_operation_p = NULL;
                    fbe_base_object_trace((fbe_base_object_t *)provision_drive, 
                                          FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                          "PVD: traverse with no edge opcode: 0x%x pkg: 0x%x sv_id: 0x%x path_s: 0x%x\n",
                                          opcode,
                                          provision_drive->base_config.block_edge_p->server_package_id,
                                          provision_drive->base_config.block_edge_p->base_edge.server_id,
                                          provision_drive->base_config.block_edge_p->base_edge.path_state); 
                    
                    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
                    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  
                    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                    fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
                    fbe_transport_complete_packet(packet_p);
                    return FBE_STATUS_GENERIC_FAILURE;
                }
                status = fbe_block_transport_send_control_packet (
                            ((fbe_base_config_t *)provision_drive)->block_edge_p,
                            packet_p);
            }
            break;
    }
    return status;
}
/* end fbe_provision_drive_control_entry() */

/*!****************************************************************************
 * fbe_provision_drive_create_edge()
 ******************************************************************************
 * @brief
 *  This function sets up the basic edge configuration info for this provision
 *  drive object.
 *  We also attempt to attach the edge. If the edge cannot be attached or the 
 *  edge info is invalid then we will fail.
 *
 * @param provision_drive_p - The provision drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/10/2009 - Created. Peter Puhov
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_create_edge(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_block_transport_control_create_edge_t * create_edge_p = NULL;    /* INPUT */
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_payload_ex_t *                         payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;  
    fbe_u32_t                                   width;
    fbe_block_edge_t *                          edge_p = NULL;
    fbe_object_id_t                             my_object_id;
    fbe_block_transport_control_attach_edge_t   attach_edge;
    fbe_package_id_t                            my_package_id;

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);  

    /* Get the usurpur control buffer */
    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p, 
                                                    sizeof(fbe_block_transport_control_create_edge_t),
                                                    (fbe_payload_control_buffer_t *)&create_edge_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Do some basic validation on the edge info received. */
    fbe_base_config_get_width((fbe_base_config_t *)provision_drive_p, &width);

    if (create_edge_p->client_index >= width) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s edge index invalid %d\n", __FUNCTION__ , create_edge_p->client_index);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now attach the edge. */
    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &my_object_id);
    fbe_base_config_get_block_edge((fbe_base_config_t *)provision_drive_p, &edge_p, create_edge_p->client_index);
    fbe_block_transport_set_transport_id(edge_p);
    fbe_block_transport_set_server_id(edge_p, create_edge_p->server_id);
    /*! @note There is only (1) client for an logical drive object. 
     *
     *  @todo What about the ppfd!
     */
    fbe_block_transport_set_server_index(edge_p, 0);
    fbe_block_transport_set_client_id(edge_p, my_object_id);
    fbe_block_transport_set_client_index(edge_p, create_edge_p->client_index);

    /* Capacity and offset of the edge is configured, configuration service sets these
     * values based on capacity of the drives and private space layout.
     */
    fbe_block_transport_edge_set_capacity(edge_p, create_edge_p->capacity);
    fbe_block_transport_edge_set_offset(edge_p, create_edge_p->offset);

    attach_edge.block_edge = edge_p;

    fbe_get_package_id(&my_package_id);
    fbe_block_transport_edge_set_client_package_id(edge_p, my_package_id);

    status = fbe_provision_drive_send_attach_edge(provision_drive_p, &attach_edge) ;
    if(status != FBE_STATUS_OK){
        fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_provision_drive_attach_downstream_block_edge failed\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                    (fbe_base_object_t*)provision_drive_p,
                                    FBE_PROVISION_DRIVE_LIFECYCLE_COND_UPDATE_PHYSICAL_DRIVE_INFO);

    /*status = fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const,
                                        (fbe_base_object_t *) provision_drive_p,
                                        (fbe_lifecycle_timer_msec_t) 0);*/    /* Immediate reschedule */

    /* We need to check logical state and send usurper to PDO if necessary. */
    fbe_provision_drive_set_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_CHECK_LOGICAL_STATE);

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_create_edge()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_update_location()
 ******************************************************************************
 * @brief
 *  This function update BES information for PVD.
 *
 * @param  provision_drive_p - The base config object.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  2014-05-26 - Created. Jamin Kang
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_update_location(fbe_provision_drive_t *provision_drive_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_packet_t *                      packet_p = NULL;
    fbe_payload_ex_t *                     payload_p = NULL;
    fbe_payload_control_operation_t *   control_p = NULL;
    fbe_physical_drive_mgmt_get_drive_information_t *   get_physical_drive_info_p = NULL;

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);

    /* allocate the memory to get the drive location. */
    get_physical_drive_info_p = (fbe_physical_drive_mgmt_get_drive_information_t *) fbe_transport_allocate_buffer();
    if (get_physical_drive_info_p == NULL) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s allocate drive info memory failed\n",
                              __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* initialize nonpaged drive information as invalid. */
    get_physical_drive_info_p->get_drive_info.port_number = FBE_PORT_NUMBER_INVALID;
    get_physical_drive_info_p->get_drive_info.enclosure_number = FBE_ENCLOSURE_NUMBER_INVALID;
    get_physical_drive_info_p->get_drive_info.slot_number = FBE_SLOT_NUMBER_INVALID;
    get_physical_drive_info_p->get_drive_info.drive_type = FBE_DRIVE_TYPE_INVALID;
    get_physical_drive_info_p->get_drive_info.spin_down_qualified = FBE_FALSE;

    packet_p = fbe_transport_allocate_packet();
    fbe_transport_initialize_packet(packet_p);
    /* Allocate control operation. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_allocate_control_operation(payload_p);

    fbe_payload_control_build_operation(control_p,
                                        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DRIVE_INFORMATION,
                                        get_physical_drive_info_p,
                                        sizeof(fbe_physical_drive_mgmt_get_drive_information_t));

    /* Set the traverse attribute which allows traverse to this packet
     * till port to get the required information.
     */
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_TRAVERSE);
    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);
    fbe_block_transport_send_control_packet(((fbe_base_config_t *)provision_drive_p)->block_edge_p, packet_p);

    fbe_transport_wait_completion(packet_p);

    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed status 0x%X\n",
                                __FUNCTION__, status);
        fbe_payload_ex_release_control_operation(payload_p, control_p);
        fbe_transport_release_packet(packet_p);
        fbe_transport_release_buffer(get_physical_drive_info_p);
        return status;
    }

    fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s: updated BES: %u_%u_%u\n",
                           __FUNCTION__,
                           get_physical_drive_info_p->get_drive_info.port_number,
                           get_physical_drive_info_p->get_drive_info.enclosure_number,
                           get_physical_drive_info_p->get_drive_info.slot_number);

    fbe_provision_drive_set_drive_location(provision_drive_p,
                                           get_physical_drive_info_p->get_drive_info.port_number,
                                           get_physical_drive_info_p->get_drive_info.enclosure_number,
                                           get_physical_drive_info_p->get_drive_info.slot_number);
    fbe_payload_ex_release_control_operation(payload_p, control_p);
    fbe_transport_release_packet(packet_p);
    fbe_transport_release_buffer(get_physical_drive_info_p);

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_provision_drive_send_attach_edge() 
 ******************************************************************************
 * @brief
 *  This function attaches the block edge.
 *
 * @param  provision_drive_p - The base config object.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/10/2009 - Created. Peter Puhov
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_send_attach_edge(fbe_provision_drive_t *provision_drive_p,
                                     fbe_block_transport_control_attach_edge_t * attach_edge) 
{   
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_packet_t *                      packet_p = NULL;
    fbe_payload_ex_t *                     payload_p = NULL;
    fbe_payload_control_operation_t *   control_p = NULL;

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);


    packet_p = fbe_transport_allocate_packet();
    fbe_transport_initialize_packet(packet_p);

    /* Allocate control operation. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_allocate_control_operation(payload_p);

    fbe_payload_control_build_operation(control_p,
                                        FBE_BLOCK_TRANSPORT_CONTROL_CODE_ATTACH_EDGE,
                                        attach_edge,
                                        sizeof(fbe_block_transport_control_attach_edge_t));

    /* We are sending control packet, so we have to form address manually. */
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              attach_edge->block_edge->base_edge.server_id);

    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* Control packets should be sent via service manager. */
    status = fbe_service_manager_send_control_packet(packet_p);

    fbe_transport_wait_completion(packet_p);

    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed status %X\n", 
                                __FUNCTION__, status);
        fbe_payload_ex_release_control_operation(payload_p, control_p);
        fbe_transport_release_packet(packet_p);
        return status;
    }

    /* Free the resources we allocated previously.*/
    fbe_payload_ex_release_control_operation(payload_p, control_p);
    fbe_transport_release_packet(packet_p);


    /* Now that edges are connected to LDO we can notify server objects that we are logically online. 
    */
    status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t *)provision_drive_p,
                                    FBE_PROVISION_DRIVE_LIFECYCLE_COND_NOTIFY_SERVER_LOGICALLY_ONLINE );
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_ERROR,  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                        "%s Couldn't set the condition. status:0x%x\n", __FUNCTION__,status);
    }

    if (attach_edge->block_edge->object_handle == NULL) {
        fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_ERROR,  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                               "%s Couldn't attach to PDO.\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_provision_drive_update_location(provision_drive_p);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_send_attach_edge()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_set_configuration()
 ******************************************************************************
 * @brief
 *  This function sets up the basic configuration info for this provision 
 *  drive object.
 *
 * @param provision_drive_p - Pointer to provision drive object
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/19/2009  Dhaval Patel - Created
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_set_configuration(fbe_provision_drive_t *provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_provision_drive_configuration_t *   set_config_p = NULL;    /* INPUT */
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_payload_ex_t *                         payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL; 
    fbe_block_count_t                           max_blocks_per_request = 0;
    fbe_provision_drive_set_configuration_t *   config_p;

    fbe_provision_drive_utils_trace( provision_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                            "%s entry\n", __FUNCTION__);

    /* Get the payload to set the configuration information.
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);  

    /* Get the usurpur control buffer */
    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p, 
                                                    sizeof(fbe_provision_drive_set_configuration_t),
                                                    (fbe_payload_control_buffer_t *)&config_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    set_config_p = &config_p->db_config;

   /* Get the maximum number of blocks that can be sent to any position (a.k.a drive). */
    status = fbe_provision_drive_class_get_drive_transfer_limits(provision_drive_p, &max_blocks_per_request);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: calculation of max per drive request failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /*!@todo Currently PVD zeroing code uses chunk size to break the request and until we change
     * the zero code to use the maximum drive xfer limit to break the zero request, we do not allow
     * max drive xfer limit to set if it is not in multiple of chunk size.
     */
    if(max_blocks_per_request < FBE_PROVISION_DRIVE_CHUNK_SIZE)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: Max drive xfer limit: 0x%llx is less than chunk size: 0x%x\n", 
                              __FUNCTION__,
                  (unsigned long long)max_blocks_per_request,
                  FBE_PROVISION_DRIVE_CHUNK_SIZE);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_provision_drive_lock(provision_drive_p);

    if ((set_config_p->update_type_bitmask & FBE_UPDATE_PVD_AFTER_ENCRYPTION_ENABLED) == FBE_UPDATE_PVD_AFTER_ENCRYPTION_ENABLED) {
        fbe_provision_drive_set_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_CREATE_AFTER_ENCRYPTION);
    }

    /* Set the provision drive configuration type. */
    fbe_provision_drive_set_config_type(provision_drive_p, set_config_p->config_type);

    /* Set the configured physical block_size type. */
    fbe_provision_drive_set_configured_physical_block_size(provision_drive_p, set_config_p->configured_physical_block_size);

    /* set the maximum number of blocks we can transfer to downstream object. */
    fbe_provision_drive_set_max_drive_transfer_limit(provision_drive_p, max_blocks_per_request);

    /* Set the configured capacity. */
    fbe_base_config_set_capacity((fbe_base_config_t *)provision_drive_p, set_config_p->configured_capacity);

    /* Set the original exported capacity. */
    fbe_provision_drive_set_user_capacity(provision_drive_p, config_p->user_capacity);

    /* Set the generation number. */
    fbe_base_config_set_generation_num((fbe_base_config_t*)provision_drive_p, set_config_p->generation_number);

    /* Set the serial number. */
    fbe_provision_drive_set_serial_num(provision_drive_p, set_config_p->serial_num);

    /* Set/clear sniff enable */
    if(set_config_p->sniff_verify_state == FBE_TRUE)
        fbe_provision_drive_set_verify_enable( provision_drive_p );
    else
        fbe_provision_drive_clear_verify_enable( provision_drive_p );

    fbe_provision_drive_unlock(provision_drive_p);

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_set_configuration()
 ******************************************************************************/


/*!****************************************************************************
 * fbe_provision_drive_metadata_set_np_flag_completion()
 ******************************************************************************
 * @brief
 *  This is the completion function for setting/clearing NP NEEDS_ZERO flag. 
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/17/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_metadata_set_np_flag_completion(fbe_packet_t * packet_p,
                                                    fbe_packet_completion_context_t context)
{
    fbe_status_t                            status;
    fbe_provision_drive_t                  *provision_drive_p = NULL;

    provision_drive_p = (fbe_provision_drive_t *)context;
    status = fbe_transport_get_status_code(packet_p);

    /* If status is not good then complete the packet with error. */
    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: mark of NP encryption start failed stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_transport_set_status(packet_p, status, 0);
        return status;
    }

    /* Go to activate state */
    if (!fbe_provision_drive_metadata_is_any_np_flag_set(provision_drive_p,
                                                         FBE_PROVISION_DRIVE_NP_FLAG_SWAP_PENDING_BITS)) {
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                           (fbe_base_object_t *) provision_drive_p,
                           FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_DISABLED);        

        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                           (fbe_base_object_t *) provision_drive_p,
                           FBE_PROVISION_DRIVE_LIFECYCLE_COND_PAGED_METADATA_RECONSTRUCT);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_set_np_flag_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_metadata_set_np_flag_need_zero()
 ******************************************************************************
 * @brief
 *  This function starts to set NP NEEDS_ZERO flag. 
 *
 * @param packet_p          - Packet requesting operation.
 * @param provision_drive_p - Point to provision drive object.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/17/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_metadata_set_np_flag_need_zero(fbe_provision_drive_t *provision_drive_p, 
                                                   fbe_packet_t * packet_p)
{
    /* For passive side if `swap pending' is not set enter Activate
     */
    if (!fbe_base_config_is_active((fbe_base_config_t *)provision_drive_p)                             &&
        !fbe_provision_drive_metadata_is_any_np_flag_set(provision_drive_p,
                                                         FBE_PROVISION_DRIVE_NP_FLAG_SWAP_PENDING_BITS)    ) {
        /* Go to activate state */
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *) provision_drive_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_DISABLED);        
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *) provision_drive_p,
                               FBE_PROVISION_DRIVE_LIFECYCLE_COND_PAGED_METADATA_RECONSTRUCT);

        return FBE_STATUS_OK;
    }

    /* For active side */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_metadata_set_np_flag_completion, provision_drive_p);
    fbe_provision_drive_get_NP_lock(provision_drive_p, packet_p, fbe_provision_drive_metadata_set_np_flag_need_zero_paged);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_set_np_flag_need_zero()
 ******************************************************************************/

/*!****************************************************************************
 * provision_drive_update_configuration_per_type()
 ******************************************************************************
 * @brief
 *  This function updates the provision drive object.
 *
 * @param provision_drive_p - Pointer to provision drive object
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 ******************************************************************************/
static fbe_status_t 
provision_drive_update_configuration_per_type(fbe_provision_drive_t *provision_drive_p, 
                                              fbe_packet_t * packet_p,
                                              fbe_update_pvd_type_t update_type, 
                                              database_object_entry_t *database_entry_p)
{
    fbe_provision_drive_config_type_t           current_config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_INVALID;
    fbe_base_config_downstream_health_state_t   downstream_health;
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_system_encryption_mode_t				system_encryption_mode;
    fbe_object_id_t								object_id;
    fbe_bool_t									is_system_drive;
    fbe_provision_drive_configuration_t *pvd_config_p = &database_entry_p->set_config_union.pvd_config;
    fbe_u32_t index;
    fbe_provision_drive_key_info_t *key_info;
    fbe_base_edge_t * base_edge;


    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
    is_system_drive = fbe_database_is_object_system_pvd(object_id);

    fbe_provision_drive_lock(provision_drive_p);

    fbe_database_get_system_encryption_mode(&system_encryption_mode);
    if ((update_type == FBE_UPDATE_PVD_TYPE) ||
        (update_type == FBE_UPDATE_PVD_UNENCRYPTED) ||
        (update_type == FBE_UPDATE_PVD_ENCRYPTION_IN_PROGRESS) ||
        (update_type == FBE_UPDATE_PVD_REKEY_IN_PROGRESS)) {
        fbe_provision_drive_get_config_type(provision_drive_p, &current_config_type);
        fbe_provision_drive_set_config_type(provision_drive_p, pvd_config_p->config_type);
        fbe_provision_drive_utils_trace( provision_drive_p,
                                         FBE_TRACE_LEVEL_INFO,
                                         FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                         FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                         "Change config_type from 0x%x to 0x%x\n", 
                                         current_config_type, pvd_config_p->config_type);
        if ((update_type == FBE_UPDATE_PVD_TYPE)               &&
            (current_config_type == pvd_config_p->config_type)    ) {
            /* Trace and error but continue. */
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s: update type: %d current config type: %d is same as new \n",
                                  __FUNCTION__, update_type, current_config_type);
        }

        else /* Config type changed, clear the paged MD cache */
        {
            status = fbe_base_config_metadata_paged_clear_cache((fbe_base_config_t *)provision_drive_p);
            fbe_provision_drive_metadata_cache_invalidate_bgz_slot(provision_drive_p, FBE_TRUE);
        }

        if (((current_config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED) && (pvd_config_p->config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_EXT_POOL)) ||
            ((current_config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_EXT_POOL) && (pvd_config_p->config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED))) {
            fbe_provision_drive_unlock(provision_drive_p);
            fbe_base_object_trace((fbe_base_object_t *) provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "provision_drive: pool config set to %d (was %d)\n", 
                                  pvd_config_p->config_type, current_config_type);
            return (fbe_provision_drive_metadata_set_np_flag_need_zero(provision_drive_p, packet_p));
        }

        /* We need to handle this command differently on encrypted systems */
        if((system_encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED) && (is_system_drive == FBE_FALSE)) {
            if((current_config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID) && 
               (pvd_config_p->config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID)){
                /* This PVD is about to become a part of RG */
                fbe_provision_drive_unlock(provision_drive_p);
                        
                /* Make shure there are no key left behind */
                /* Invalidate encryption keys */
                fbe_provision_drive_invalidate_keys(provision_drive_p);

                /* Our system is encrypted and PVD is about to become bound.
                   Use the encryption mode that the database service determined.
                 */
                fbe_base_object_trace((fbe_base_object_t *) provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "provision_drive: Encryption mode set to %d (was %d)\n", 
                                      database_entry_p->base_config.encryption_mode, provision_drive_p->base_config.encryption_mode);
                fbe_base_config_set_encryption_mode((fbe_base_config_t *) provision_drive_p, 
                                                    database_entry_p->base_config.encryption_mode);
                return (fbe_provision_drive_metadata_set_np_flag_need_zero(provision_drive_p, packet_p));
            } /* PVD is about to become a part of RG */

            if ((current_config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID) && 
                (pvd_config_p->config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID)){
                /* This PVD is about to become unconsumed */
                fbe_provision_drive_unlock(provision_drive_p);
                                                
                fbe_base_object_trace((fbe_base_object_t *) provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "provision_drive: Encryption mode set to %d (was %d)\n", 
                                      database_entry_p->base_config.encryption_mode, provision_drive_p->base_config.encryption_mode);
                fbe_base_config_set_encryption_mode((fbe_base_config_t *) provision_drive_p, 
                                                    database_entry_p->base_config.encryption_mode);
                return (fbe_provision_drive_metadata_set_np_flag_need_zero(provision_drive_p, packet_p));
            } /* PVD is about to unbound */

            /* On encrypted system we may need to Invalidate keys if we become unbound */

        }/* System mode ENCRYPTED */
        else if((system_encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED) && (is_system_drive == FBE_TRUE)){
            fbe_lifecycle_state_t   lifecycle_state;
            fbe_base_object_get_lifecycle_state((fbe_base_object_t *)provision_drive_p, &lifecycle_state);

            if ((current_config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID) && 
                (pvd_config_p->config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID)) {
                /* This PVD is about to become a part of RG,
                 * Note that in this case we need to handle this entirely inline since
                 * we cannot take the system PVD out of ready to init paged.
                 */
                fbe_provision_drive_unlock(provision_drive_p);

                /* Our system is encrypted and PVD is about to become bound.
                 * Use the encryption mode that the database service determined.
                 */
                fbe_base_object_trace((fbe_base_object_t *) provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "provision_drive: Encryption mode set to %d (was %d)\n", 
                                      database_entry_p->base_config.encryption_mode, provision_drive_p->base_config.encryption_mode);
                fbe_base_config_set_encryption_mode((fbe_base_config_t *) provision_drive_p, 
                                                    database_entry_p->base_config.encryption_mode);
                if (lifecycle_state == FBE_LIFECYCLE_STATE_READY) {
                    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_write_default_paged_metadata_set_np_flag, provision_drive_p);
                    return fbe_provision_drive_write_default_system_paged(provision_drive_p, packet_p);
                } else {
                    return fbe_provision_drive_metadata_set_np_flag_need_zero(provision_drive_p, packet_p);
                }
            }    /* PVD is about to become a part of RG */

            if ((current_config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID) && 
                (pvd_config_p->config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID)) {
                /* This PVD is about to become unconsumed. */
                fbe_provision_drive_unlock(provision_drive_p);

                fbe_base_object_trace((fbe_base_object_t *) provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                      "provision_drive: Encryption mode set to %d (was %d)\n", 
                                      database_entry_p->base_config.encryption_mode, provision_drive_p->base_config.encryption_mode);
                fbe_base_config_set_encryption_mode((fbe_base_config_t *) provision_drive_p, 
                                                    database_entry_p->base_config.encryption_mode);
                fbe_transport_set_completion_function(packet_p, fbe_provision_drive_update_validation_all_completion, provision_drive_p);
                if (lifecycle_state == FBE_LIFECYCLE_STATE_READY) {
                    if (!fbe_provision_drive_metadata_is_any_np_flag_set(provision_drive_p,
                                                                         FBE_PROVISION_DRIVE_NP_FLAG_SWAP_PENDING_BITS)) {
                        fbe_transport_set_completion_function(packet_p, fbe_provision_drive_write_default_paged_metadata_completion, provision_drive_p);
                        fbe_transport_set_completion_function(packet_p, fbe_provision_drive_write_default_paged_metadata_set_np_flag, provision_drive_p);
                        return fbe_provision_drive_write_default_system_paged(provision_drive_p, packet_p);
                    } else {
                        return FBE_STATUS_OK;
                    }
                } else {
                    return fbe_provision_drive_metadata_set_np_flag_need_zero(provision_drive_p, packet_p);
                }
            }    /* PVD is about to unbound */
        }
        
    } else if (update_type == FBE_UPDATE_PVD_SNIFF_VERIFY_STATE) {
         /* Set/clear sniff enable */
        if(pvd_config_p->sniff_verify_state == FBE_TRUE) {
            fbe_provision_drive_enable_verify( provision_drive_p );
        } else {
            fbe_provision_drive_disable_verify( provision_drive_p );
        }
    } else if (update_type == FBE_UPDATE_PVD_SERIAL_NUMBER) {
        /* Set serial numbe to be updated */
        fbe_copy_memory(provision_drive_p->serial_num, 
                       pvd_config_p->serial_num, 
                       FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1);

        fbe_provision_drive_utils_trace( provision_drive_p,
                                         FBE_TRACE_LEVEL_INFO,
                                         FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                         FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                         "Change serial num to %s\n", 
                                         pvd_config_p->serial_num);
    } else if(update_type == FBE_UPDATE_PVD_CONFIG) {
        fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s New system PVD. update config\n", __FUNCTION__);

        /* Set the configured physical block_size type. */
        fbe_provision_drive_set_configured_physical_block_size(provision_drive_p, pvd_config_p->configured_physical_block_size);

        /* Update the upsteam edges geomortry. */
        fbe_provision_drive_propagate_block_geometry(provision_drive_p);

        /* Set the configured capacity. */
        fbe_base_config_set_capacity((fbe_base_config_t *)provision_drive_p, pvd_config_p->configured_capacity);

        /* Set the original exported capacity. */
        fbe_provision_drive_set_user_capacity(provision_drive_p, pvd_config_p->configured_capacity);

        /* Set the serial number. */
        fbe_provision_drive_set_serial_num(provision_drive_p, pvd_config_p->serial_num);

        fbe_provision_drive_unlock(provision_drive_p);

        //status = fbe_provision_drive_clear_nonpaged(provision_drive_p, packet_p);

        /* If the health is broken, then we cannot reinitialize the pvd */
        downstream_health = fbe_provision_drive_verify_downstream_health(provision_drive_p);
        if(downstream_health != FBE_DOWNSTREAM_HEALTH_BROKEN) {
            fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s PVD is not broken. health:0x%x\n", __FUNCTION__,downstream_health);

            return FBE_STATUS_GENERIC_FAILURE;
        }

        if(system_encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED)
        {
            /* We will have to invalidate the existing Key Handles so that new ones can be pushed */
            for(index = 0; index < FBE_PROVISION_DRIVE_KEY_INFO_TABLE_MAX ; index++)
            {
                fbe_provision_drive_get_key_info(provision_drive_p, index, &key_info);
                if(key_info->key_handle != NULL)
                {
                    fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                                           FBE_TRACE_LEVEL_INFO,
                                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                           "%s Invalidated Key Mask Index:%d\n", __FUNCTION__,index);
                    /* Make the key invalid since this is an old key and we dont want to process this if RG
                     * pushes it by mistake */
                    key_info->key_handle->key_mask = 0;
                    base_edge = fbe_base_transport_server_get_client_edge_by_server_index((fbe_base_transport_server_t *)&provision_drive_p->base_config.block_transport_server,
                                                                                          index);
                    key_info->key_handle = NULL;
                    base_edge->path_state = FBE_PATH_STATE_BROKEN;
                }
            }
            status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, 
                                            (fbe_base_object_t *)provision_drive_p,
                                            FBE_BASE_CONFIG_LIFECYCLE_COND_ENCRYPTION);
        }

        fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_REINIT_PVD);
        fbe_base_object_add_to_usurper_queue((fbe_base_object_t*)provision_drive_p, packet_p);
        status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *)provision_drive_p,
                                FBE_BASE_CONFIG_LIFECYCLE_COND_CLEAR_NP);

        if(status != FBE_STATUS_OK) {
            fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s Couldn't set the condition. status:0x%x\n", __FUNCTION__,status);
        }
        
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else if (pvd_config_p->update_type == FBE_UPDATE_PVD_NONPAGED_BY_DEFALT_VALUE)
    {
        status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                                       (fbe_base_object_t *)provision_drive_p,
                                        FBE_BASE_CONFIG_LIFECYCLE_COND_REINIT_NP);
        if(status != FBE_STATUS_OK) {
            fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s Couldn't set the condition. status:0x%x\n", __FUNCTION__,status);
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s Unsupported update type: 0x%x\n", 
                              __FUNCTION__, pvd_config_p->update_type);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_provision_drive_unlock(provision_drive_p);

    return status;
}
/******************************************************************************
 * end provision_drive_update_configuration_per_type()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_provision_drive_update_configuration()
 ******************************************************************************
 * @brief
 *  This function updates the provision drive object.
 *
 * @param provision_drive_p - Pointer to provision drive object
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_update_configuration(fbe_provision_drive_t *provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_payload_ex_t *                          payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL; 
    fbe_update_pvd_type_t                       individual_type;
    fbe_status_t                                individual_status = FBE_STATUS_OK;	
    fbe_provision_drive_configuration_t        *pvd_config_p = NULL;
    database_object_entry_t                    *database_entry_p = NULL;

    fbe_provision_drive_utils_trace( provision_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                            "%s entry\n", __FUNCTION__);

    /* Get the payload to update the configuration information. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);  

    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p, 
                                                    sizeof(database_object_entry_t),
                                                    (fbe_payload_control_buffer_t *)&database_entry_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    pvd_config_p = &database_entry_p->set_config_union.pvd_config;

    fbe_provision_drive_utils_trace( provision_drive_p,
                                     FBE_TRACE_LEVEL_INFO,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                     FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                     "update pvd: update type: 0x%x type mask 0x%x config_type 0x%x \n", 
                                     pvd_config_p->update_type, pvd_config_p->update_type_bitmask, pvd_config_p->config_type);

    if (pvd_config_p->update_type != FBE_UPDATE_PVD_USE_BITMASK)
    {
        status = provision_drive_update_configuration_per_type(provision_drive_p, packet_p, pvd_config_p->update_type, database_entry_p);
    }
    else
    {
        /* now we need to work on all update requests */
        individual_type = FBE_UPDATE_PVD_TYPE;
        do
        {
            /* if match, do update */
            if ((individual_type&pvd_config_p->update_type_bitmask)==individual_type)
            {
                individual_status = provision_drive_update_configuration_per_type(provision_drive_p, packet_p, individual_type, database_entry_p);
                if (individual_status == FBE_STATUS_MORE_PROCESSING_REQUIRED) 
                {
                    status = FBE_STATUS_MORE_PROCESSING_REQUIRED;
                }
                else if (individual_status == FBE_STATUS_GENERIC_FAILURE)
                {
                    /* not override the more processing required status */
                    if (status != FBE_STATUS_MORE_PROCESSING_REQUIRED)
                    {
                        status = FBE_STATUS_GENERIC_FAILURE;
                    }
                    /* stop the loop */
                    break;
                }
                
            }
            individual_type <<= 1;  /* next type */
        }
        while (individual_type<FBE_UPDATE_PVD_LAST);
    }

    if (status != FBE_STATUS_MORE_PROCESSING_REQUIRED)
    {
        /* complete packet */
        if (status == FBE_STATUS_OK)
        {
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
            fbe_transport_set_status(packet_p, status, 0);
        }
        else
        {
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        }
        fbe_transport_complete_packet(packet_p);
    }
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_update_configuration()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_update_block_size()
 ******************************************************************************
 * @brief
 *  Engineering Only function to change block size for testing.
 *
 * @param provision_drive_p - Pointer to provision drive object
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 ******************************************************************************/

fbe_status_t fbe_provision_drive_update_block_size(fbe_provision_drive_t *provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_payload_ex_t *                          payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL; 
    fbe_provision_drive_configuration_t        *pvd_config_p = NULL;
    database_object_entry_t                    *database_entry_p = NULL;

    fbe_provision_drive_utils_trace( provision_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                            "%s entry\n", __FUNCTION__);

    /* Get the payload to update the configuration information. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);  

    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(database_object_entry_t),
                                                    (fbe_payload_control_buffer_t *)&database_entry_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(	(fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s get buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    pvd_config_p = &database_entry_p->set_config_union.pvd_config;

    fbe_provision_drive_utils_trace( provision_drive_p,
                                     FBE_TRACE_LEVEL_INFO,
                                     FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                     FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                     "update pvd block size: %d\n", 
                                     pvd_config_p->configured_physical_block_size);



    /* Set the configured physical block_size type. */
    fbe_provision_drive_lock(provision_drive_p);
    fbe_provision_drive_set_configured_physical_block_size(provision_drive_p, pvd_config_p->configured_physical_block_size);
    fbe_provision_drive_unlock(provision_drive_p);

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);

    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_provision_drive_get_drive_info()
 ****************************************************************
 * @brief
 *  This function is use to get the provision drive information,
 *  it is mainly used by topology service to collect the drive 
 *  information.

 * @param provision_drive_p - Provision drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  08/27/2009 - Created. Dhaval Patel
 *
 ****************************************************************/
static fbe_status_t
fbe_provision_drive_get_drive_info(fbe_provision_drive_t * provision_drive_p,
                                   fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                                 payload_p = NULL;
    fbe_payload_control_operation_t *                   control_operation_p = NULL;
    fbe_provision_drive_info_t *                        provision_drive_info_p;
    fbe_provision_drive_metadata_positions_t            metadata_positions;
    fbe_provision_drive_nonpaged_metadata_drive_info_t  nonpaged_drive_info;
    fbe_status_t                                        status;
    fbe_u8_t                                           *serial_num = NULL;
    fbe_object_id_t                                     object_id = FBE_OBJECT_ID_INVALID;
    fbe_block_edge_t                                   *block_edge_p = NULL;
    fbe_bool_t                                          is_metadata_initialized;

    /* Retrive the spare info request from the packet. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Get the usurpur control buffer */
    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p, 
                                                    sizeof(fbe_provision_drive_info_t),
                                                    (fbe_payload_control_buffer_t *)&provision_drive_info_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }
    
    /* get the serial number info of provision drive object. */
    status = fbe_provision_drive_get_serial_num(provision_drive_p, &(serial_num));
    
    fbe_copy_memory(provision_drive_info_p->serial_num,
                     serial_num, 
                     FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1);

    provision_drive_info_p->port_number = FBE_PORT_NUMBER_INVALID;
    provision_drive_info_p->enclosure_number = FBE_ENCLOSURE_NUMBER_INVALID;
    provision_drive_info_p->slot_number = FBE_SLOT_NUMBER_INVALID;

    /* Get the nonpaged metadata drive information */
    status = fbe_provision_drive_metadata_get_nonpaged_metadata_drive_info(provision_drive_p, &nonpaged_drive_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    fbe_base_config_metadata_is_initialized((fbe_base_config_t *)provision_drive_p, &is_metadata_initialized);
    if (is_metadata_initialized)
    {
        /* copy nonpaged metadata drive information. */
        provision_drive_info_p->port_number = nonpaged_drive_info.port_number;
        provision_drive_info_p->enclosure_number = nonpaged_drive_info.enclosure_number;
        provision_drive_info_p->slot_number = nonpaged_drive_info.slot_number;
        provision_drive_info_p->drive_type = nonpaged_drive_info.drive_type;
    }
    else
    {
        fbe_u32_t peer_port_num, peer_encl_num, peer_slot_num;
        fbe_provision_drive_get_peer_drive_location(provision_drive_p, &peer_port_num, &peer_encl_num, &peer_slot_num);
        /* if the metadata isn't initialized, while peer PVD has the location, copy from peer. */
        if (!fbe_base_config_is_active((fbe_base_config_t *)provision_drive_p) &&
            peer_port_num != FBE_PORT_NUMBER_INVALID && 
            peer_encl_num != FBE_ENCLOSURE_NUMBER_INVALID &&
            peer_slot_num != FBE_SLOT_NUMBER_INVALID)
        {
            provision_drive_info_p->port_number = peer_port_num;
            provision_drive_info_p->enclosure_number = peer_encl_num;
            provision_drive_info_p->slot_number = peer_slot_num;
        }
        else
        {
            fbe_transport_set_status(packet_p, status, 0);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_complete_packet(packet_p);
            return status;
        }
    }

    /* get the metadata position for the provision drive object. */
    fbe_provision_drive_get_metadata_positions(provision_drive_p, &metadata_positions);

    /* return the metadata information for the provision drive object. */
    provision_drive_info_p->paged_metadata_lba = metadata_positions.paged_metadata_lba;
    provision_drive_info_p->paged_metadata_capacity = metadata_positions.paged_metadata_capacity;
    provision_drive_info_p->paged_mirror_offset = metadata_positions.paged_mirror_metadata_offset;

    fbe_provision_drive_lock(provision_drive_p);

    /* get the config type, physical block size, offset and capacity. */
    fbe_provision_drive_get_config_type(provision_drive_p, &(provision_drive_info_p->config_type));

    /* get configured physical block size. */
    fbe_provision_drive_get_configured_physical_block_size(provision_drive_p, &provision_drive_info_p->configured_physical_block_size);

    /* get max drive transfer limit. */
    fbe_provision_drive_get_max_drive_transfer_limit(provision_drive_p,&provision_drive_info_p->max_drive_xfer_limit);
    
    /* get debug flags. */
    fbe_provision_drive_get_debug_flag(provision_drive_p,&provision_drive_info_p->debug_flags);
    
    /* get end of life state flag of provision drive object. */
    fbe_provision_drive_metadata_get_end_of_life_state(provision_drive_p,&provision_drive_info_p->end_of_life_state);
   
    /* get drive fault state flag of provision drive object. */
    fbe_provision_drive_metadata_get_drive_fault_state(provision_drive_p,&provision_drive_info_p->drive_fault_state);
  
    /* get swap pending state flag of provision drive object. */
    if (fbe_provision_drive_metadata_is_any_np_flag_set(provision_drive_p,
                                                        FBE_PROVISION_DRIVE_NP_FLAG_SWAP_PENDING_BITS)) {
        provision_drive_info_p->swap_pending = FBE_TRUE;
    } else {
        provision_drive_info_p->swap_pending = FBE_FALSE;
    }

    /* get media error lba of provision drive object. */
    fbe_provision_drive_metadata_get_sniff_media_error_lba(provision_drive_p,&provision_drive_info_p->media_error_lba);
    
    /* get zero on demand flag. */
    //fbe_provision_drive_metadata_get_zero_on_demand_flag(provision_drive_p,&provision_drive_info_p->zero_on_demand);

    /* determine if this is s system drive or not. */
    fbe_base_object_get_object_id((fbe_base_object_t*)provision_drive_p, &object_id);
    provision_drive_info_p->is_system_drive = fbe_database_is_object_system_pvd(object_id);

    /* get the exported capacity of provision drive object. */
    fbe_base_config_get_capacity((fbe_base_config_t*)provision_drive_p, &provision_drive_info_p->capacity);
    provision_drive_info_p->total_chunks = (fbe_chunk_count_t)(provision_drive_info_p->capacity / FBE_PROVISION_DRIVE_CHUNK_SIZE);
    provision_drive_info_p->chunk_size = FBE_PROVISION_DRIVE_CHUNK_SIZE;

    /* get the imported capacity */
    fbe_base_config_get_block_edge_ptr((fbe_base_config_t*)provision_drive_p, &block_edge_p);
    if (block_edge_p != NULL)
    {
        provision_drive_info_p->imported_capacity = block_edge_p->capacity;
    }
    else
    {
        provision_drive_info_p->imported_capacity = 0;
    }

    /* get the sniff_verify_state */
    fbe_provision_drive_get_verify_enable( provision_drive_p, &provision_drive_info_p->sniff_verify_state );

    fbe_base_config_get_default_offset((fbe_base_config_t*)provision_drive_p, &provision_drive_info_p->default_offset);
    provision_drive_info_p->default_chunk_size = FBE_PROVISION_DRIVE_CHUNK_SIZE;

    status = fbe_provision_drive_metadata_get_background_zero_checkpoint(provision_drive_p, &provision_drive_info_p->zero_checkpoint);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_complete_packet(packet_p);
        fbe_provision_drive_unlock(provision_drive_p);
        return status;
    }
    
    if(provision_drive_info_p->zero_checkpoint == FBE_LBA_INVALID){
        provision_drive_info_p->zero_on_demand = FBE_FALSE; 
    } else {
        provision_drive_info_p->zero_on_demand = FBE_TRUE; 
    }

    /* get slf state */
    if (fbe_provision_drive_is_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF))
    {
        if (fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF))
        {
            provision_drive_info_p->slf_state = FBE_PROVISION_DRIVE_SLF_BOTH_SP;
        }
        else
        {
            provision_drive_info_p->slf_state = FBE_PROVISION_DRIVE_SLF_THIS_SP;
        }
    }
    else
    {
        if (fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF))
        {
            provision_drive_info_p->slf_state = FBE_PROVISION_DRIVE_SLF_PEER_SP;
        }
        else
        {
            provision_drive_info_p->slf_state = FBE_PROVISION_DRIVE_SLF_NONE;
        }
    }

    provision_drive_info_p->spin_down_qualified = fbe_provision_drive_is_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_SPIN_DOWN_QUALIFIED);

    /* if drive is not spin_down_qualified, it's not power save capable.
       if drive is spin_down_qualified and it's a system drive, it's still not power save capable.
       if drive is spin_down_qualified and not a system drive, it's power save capable.*/
    if(provision_drive_info_p->spin_down_qualified == FBE_FALSE) {
        provision_drive_info_p->power_save_capable = FBE_FALSE;
    }
    else if (fbe_private_space_layout_object_id_is_system_pvd(object_id)) {
        provision_drive_info_p->power_save_capable = FBE_FALSE;
    }
    else {
        provision_drive_info_p->power_save_capable = FBE_TRUE;
    }

    /* Get `percent_rebuilt' which is pushed from parent raid group. */
    fbe_provision_drive_get_percent_rebuilt(provision_drive_p, &provision_drive_info_p->percent_rebuilt);

    provision_drive_info_p->created_after_encryption_enabled = fbe_provision_drive_is_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_CREATE_AFTER_ENCRYPTION);
    if ((fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_NEEDED)) ||
        (fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_INTENT)))
    {
        provision_drive_info_p->scrubbing_in_progress = FBE_TRUE;
    }
    else
    {
        provision_drive_info_p->scrubbing_in_progress = FBE_FALSE;
    }

    provision_drive_info_p->flags = provision_drive_p->flags;

    fbe_provision_drive_unlock(provision_drive_p);

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_get_drive_info()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_usurper_get_spare_info()
 ******************************************************************************
 * @brief
 *  This function is use to get the spare drive information, it is mainly used
 *  by drive spare library to collect the drive information to find the best 
 *  suitable spare.
 *
 *  Provision drive object does not store this information and so it always 
 *  send the request to the downstream object with TRAVERSE option set.
 *
 * @param provision_drive_p - The provision drive.
 * @param packet_p          - The packet requesting this operation.
 *
 * @return status           - The status of the operation.
 *
 * @author
 *  02/12/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_usurper_get_spare_info(fbe_provision_drive_t * provision_drive_p,
                                           fbe_packet_t * packet_p)
{
    fbe_object_id_t                                     my_object_id = FBE_OBJECT_ID_INVALID;
    fbe_payload_ex_t                                   *payload_p = NULL;
    fbe_payload_control_operation_t                    *control_operation_p = NULL;
    fbe_spare_drive_info_t                             *spare_drive_info_p = NULL;
    fbe_provision_drive_nonpaged_metadata_drive_info_t  nonpaged_drive_info;
    fbe_status_t                                        status;
    fbe_lba_t                                           exported_offset = FBE_LBA_INVALID;
    fbe_lba_t                                           default_offset = FBE_LBA_INVALID;
    fbe_lba_t                                           total_exported_capacity = FBE_LBA_INVALID;

    /* Retrive the spare info request from the packet. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Get the usurpur control buffer */
    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p, 
                                                    sizeof(fbe_spare_drive_info_t),
                                                    (fbe_payload_control_buffer_t *)&spare_drive_info_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Get the nonpaged metadata drive information */
    status = fbe_provision_drive_metadata_get_nonpaged_metadata_drive_info(provision_drive_p, &nonpaged_drive_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* copy nonpaged metadata drive information to spare drive info. */
    spare_drive_info_p->port_number = nonpaged_drive_info.port_number;
    spare_drive_info_p->enclosure_number = nonpaged_drive_info.enclosure_number;
    spare_drive_info_p->slot_number = nonpaged_drive_info.slot_number;
    spare_drive_info_p->drive_type = nonpaged_drive_info.drive_type;
    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &my_object_id);

    status = fbe_database_get_pvd_pool_id(my_object_id, &spare_drive_info_p->pool_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s: Failed to get the pool_id of pvd 0x%x from database.\n", 
                              __FUNCTION__, my_object_id);
    }

    fbe_provision_drive_get_configured_physical_block_size(provision_drive_p, &spare_drive_info_p->configured_block_size);

    /*! @note This is the exported capacity.  That is the `consumable' capacity.
     *        Thus it does not include the provision drive metadata capacity.
     *        The capacity on the downstream edge is the imported capacity.
     */
    fbe_base_config_get_capacity((fbe_base_config_t *)provision_drive_p, &spare_drive_info_p->configured_capacity);
    

    /*! @note Get the current exported capacity and offset (if any).
     */
    status = fbe_block_transport_server_get_minimum_capacity_required(&provision_drive_p->base_config.block_transport_server,
                                                                      &exported_offset,
                                                                      &total_exported_capacity);

    fbe_base_config_get_default_offset((fbe_base_config_t *)provision_drive_p, &default_offset);

    /* It is perfectly valid to not have any upstream objects.  
     * If there are no upstream objects return the current default offset.
     */
    if ((status != FBE_STATUS_OK)                    ||
        (total_exported_capacity == FBE_LBA_INVALID)    )
    {
        /* Simply use the default offset.
         */        
        spare_drive_info_p->capacity_required = default_offset;
        spare_drive_info_p->exported_offset = default_offset;
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "provision_drive: get spare info pool id: 0x%x no upstream def ofst: 0x%llx cap: 0x%llx\n",
                              spare_drive_info_p->pool_id,
                              (unsigned long long)spare_drive_info_p->exported_offset, 
                              (unsigned long long)spare_drive_info_p->configured_capacity);
    }
    else
    {
        /* Else use the total exported capacity as the exported offset.
         */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "provision_drive: get spare info pool id: 0x%x upstream exp ofst: 0x%llx cap: 0x%llx\n",
                              spare_drive_info_p->pool_id,
                              (unsigned long long)total_exported_capacity, 
                              (unsigned long long)spare_drive_info_p->configured_capacity);
        if(total_exported_capacity < default_offset)
        {
            spare_drive_info_p->capacity_required = default_offset;
            spare_drive_info_p->exported_offset = default_offset;
        }
        else
        {
            spare_drive_info_p->capacity_required = total_exported_capacity;
            spare_drive_info_p->exported_offset = total_exported_capacity;
        }
    }

    /* Determine if this is a system drive or not.*/
    spare_drive_info_p->b_is_system_drive = fbe_database_is_object_system_pvd(my_object_id);

    /* Determine if End-Of-Lide has been set on this drive.*/
    fbe_provision_drive_metadata_get_end_of_life_state(provision_drive_p, &spare_drive_info_p->b_is_end_of_life);

    /* Determine if this drive is in SLF state.*/
    if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_SLF) ||
        fbe_provision_drive_is_peer_clustered_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_SLF))
    {
        spare_drive_info_p->b_is_slf = FBE_TRUE;
    }
    else
    {
        spare_drive_info_p->b_is_slf = FBE_FALSE;
    }

    /* If any of the `special' flags are set trace some information.  We
     * don't want to do this for all drives since we will flood the trace
     * logs.
     */
    if ((spare_drive_info_p->b_is_system_drive != FBE_FALSE) ||
        (spare_drive_info_p->b_is_end_of_life != FBE_FALSE)  ||
        (spare_drive_info_p->b_is_slf != FBE_FALSE)             )
    {
        /* Trace the `special' flag values
         */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "provision_drive: get spare info is system drive: %d  EOL: %d  SLF: %d\n",
                              spare_drive_info_p->b_is_system_drive, 
                              spare_drive_info_p->b_is_end_of_life,
                              spare_drive_info_p->b_is_slf);
    }

    /* complete the packet with good status. */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_usurper_get_spare_info()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_usurper_get_drive_location()
 ******************************************************************************
 * @brief
 *  This function is use to get the provision drive location.
 *
 * @param provision_drive_p     - Provision drive object.
 * @param packet_p              - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  04/30/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_usurper_get_drive_location(fbe_provision_drive_t * provision_drive_p,
                                                             fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                                     payload_p = NULL;
    fbe_payload_control_operation_t *                       control_operation_p = NULL;
    fbe_provision_drive_get_physical_drive_location_t *     get_physical_drive_location_p = NULL;
    fbe_provision_drive_nonpaged_metadata_drive_info_t      nonpaged_drive_info;
    fbe_status_t                                            status;

    /* Retrive the spare info request from the packet. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* get the control buffer for the drive location. */
    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p, 
                                                    sizeof(fbe_provision_drive_get_physical_drive_location_t),
                                                    (fbe_payload_control_buffer_t *)&get_physical_drive_location_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Get the nonpaged metadata drive information */
    status = fbe_provision_drive_metadata_get_nonpaged_metadata_drive_info(provision_drive_p, &nonpaged_drive_info);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* copy port, enclosure and slot information. */
    get_physical_drive_location_p->port_number = nonpaged_drive_info.port_number;
    get_physical_drive_location_p->enclosure_number = nonpaged_drive_info.enclosure_number;
    get_physical_drive_location_p->slot_number = nonpaged_drive_info.slot_number;

    /* complete the packet with good status. */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_usurper_get_drive_location()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_get_zero_checkpoint()
 ******************************************************************************
 * @brief
 *  This function is used to get the zeroing checkpoint.
 * 
 * @param  provision_drive      - provision drive object.
 * @param  packet               - pointer to the packet.
 *
 * @return status               - status of the operation.
 *
 * @author
 *  05/26/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_get_zero_checkpoint(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t *                          payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_provision_drive_get_zero_checkpoint_t * get_zero_checkpoint = NULL;
    fbe_status_t                                status;

    fbe_provision_drive_utils_trace(provision_drive,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  


    status = fbe_provision_drive_get_control_buffer(provision_drive, packet,
                                                    sizeof(fbe_provision_drive_get_zero_checkpoint_t),
                                                    (fbe_payload_control_buffer_t *)&get_zero_checkpoint);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)provision_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }


    status = fbe_provision_drive_metadata_get_background_zero_checkpoint(provision_drive, &get_zero_checkpoint->zero_checkpoint);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "pvd_get_zero_chkpt: Failed to get checkpoint. Status: 0x%X\n", status);

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
 * end fbe_provision_drive_get_zero_checkpoint()
*******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_set_zero_checkpoint()
 ******************************************************************************
 * @brief
 *  This function is used to set the zeroing checkpoint.
 * 
 * @param  provision_drive      - provision drive object.
 * @param  packet               - pointer to the packet.
 *
 * @return status               - status of the operation.
 *
 * @author
 *  02/12/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_set_zero_checkpoint(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet)
{
    fbe_status_t                                status;
    fbe_payload_ex_t *                         payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_provision_drive_control_set_zero_checkpoint_t * set_zero_checkpoint = NULL;
    fbe_lba_t                           exported_capacity = FBE_LBA_INVALID;
    fbe_lba_t                           zero_checkpoint = FBE_LBA_INVALID;
    fbe_bool_t                          b_is_zero_request_aligned_to_chunk = FBE_FALSE;
    fbe_lba_t                           default_offset = 0;

    fbe_provision_drive_utils_trace(provision_drive,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  


    status = fbe_provision_drive_get_control_buffer(provision_drive, packet, 
                                                    sizeof(fbe_provision_drive_control_set_zero_checkpoint_t),
                                                    (fbe_payload_control_buffer_t *)&set_zero_checkpoint);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)provision_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }


    /* Validate the zero checkpoint request
     */

    /* get the paged metadatas start lba. */
    fbe_base_config_get_capacity((fbe_base_config_t *) provision_drive, &exported_capacity);

    /* check if disk zeroing completed */
    zero_checkpoint = set_zero_checkpoint->background_zero_checkpoint;

    /* Invalid zero checkpoint should bypass all the rules */
    if(zero_checkpoint == FBE_LBA_INVALID){	    
        status = fbe_provision_drive_metadata_set_bz_chkpt_without_persist(provision_drive, packet, zero_checkpoint);   
        return status;
    }

    if(zero_checkpoint  > exported_capacity)
    {
        /* disk zeroing completed */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s:checkpoint cannot be updated beyond capacity, zero_chk:0x%llx, export_cap:0x%llx\n",
                              __FUNCTION__,
                  (unsigned long long)zero_checkpoint,
                  (unsigned long long)exported_capacity);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now validate that the checkpoint is aligned to the provision drive chunk.
     */
    status = fbe_provision_drive_utils_is_io_request_aligned(zero_checkpoint, FBE_PROVISION_DRIVE_CHUNK_SIZE, 
                                                             FBE_PROVISION_DRIVE_CHUNK_SIZE, &b_is_zero_request_aligned_to_chunk);
    if((status != FBE_STATUS_OK) || 
       (!b_is_zero_request_aligned_to_chunk))
    {
        /* Zero checkpoint must be aligned to chunk size.
         */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: zero_checkpoint: 0x%llx is not aligned to pvd chunk size: 0x%x\n",
                              __FUNCTION__, (unsigned long long)zero_checkpoint,
                  FBE_PROVISION_DRIVE_CHUNK_SIZE);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! @todo Currently we do not allow the background zero checkpoint to be 
     *        set below the default offset.
     */
    fbe_base_config_get_default_offset((fbe_base_config_t *)provision_drive, &default_offset);
    if (zero_checkpoint < default_offset)
    {
        /*! @todo Currently we don't allow the background zero checkpoint to be
         *        less than the default_offset.
         */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: zero_checkpoint: 0x%llx is less than the default_offset: 0x%llx\n",
                              __FUNCTION__, (unsigned long long)zero_checkpoint,
                  (unsigned long long)default_offset);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now set the completion function and set the non-paged value (without persist).
     */    
    status = fbe_provision_drive_metadata_set_bz_chkpt_without_persist(provision_drive, packet, zero_checkpoint);   

    return status;
}
/******************************************************************************
 * end fbe_provision_drive_set_zero_checkpoint()
*******************************************************************************/


/*!****************************************************************************
 * fbe_provision_drive_is_unregister_keys_needed()
 ******************************************************************************
 * @brief
 *  This function checks whether the PVD needs to unregister keys.
 * 
 * @param  key_info             - key info saved in PVD.
 * @param  key_handle           - keys pushed down from RG.
 * @param  port_unregister_keys - params sending to port.
 *
 * @return status               - status of the operation.
 *
 * @author
 *  01/07/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_is_unregister_keys_needed(fbe_provision_drive_key_info_t *key_info,
                                              fbe_encryption_key_info_t *key_handle,
                                              fbe_base_port_mgmt_unregister_keys_t * port_unregister_keys)
{
    fbe_encryption_key_mask_t mask0, mask1;

    fbe_encryption_key_mask_get(&key_handle->key_mask, 0, &mask0);
    fbe_encryption_key_mask_get(&key_handle->key_mask, 1, &mask1);

    /* Initialize the structure */
    fbe_zero_memory(port_unregister_keys, sizeof(fbe_base_port_mgmt_unregister_keys_t));
    port_unregister_keys->num_of_keys = 0;

    if ((key_info->mp_key_handle_1 != FBE_INVALID_KEY_HANDLE) &&
        !(mask0 & FBE_ENCRYPTION_KEY_MASK_VALID)) 
    {
        /* We have a key that needs to be unregistered */
        port_unregister_keys->mp_key_handle[port_unregister_keys->num_of_keys] = key_info->mp_key_handle_1;
        port_unregister_keys->num_of_keys++;
    } 
    else if ((key_info->mp_key_handle_1 != FBE_INVALID_KEY_HANDLE) &&
             (mask0 & FBE_ENCRYPTION_KEY_MASK_VALID) &&
             (mask0 & FBE_ENCRYPTION_KEY_MASK_UPDATE)) 
    {
        /* We need to unregister the key first */
        port_unregister_keys->mp_key_handle[port_unregister_keys->num_of_keys] = key_info->mp_key_handle_1;
        port_unregister_keys->num_of_keys++;
    }

    if ((key_info->mp_key_handle_2 != FBE_INVALID_KEY_HANDLE) &&
        !(mask1 & FBE_ENCRYPTION_KEY_MASK_VALID)) 
    {
        /* We have a key that needs to be unregistered */
        port_unregister_keys->mp_key_handle[port_unregister_keys->num_of_keys] = key_info->mp_key_handle_2;
        port_unregister_keys->num_of_keys++;
    } 
    else if ((key_info->mp_key_handle_2 != FBE_INVALID_KEY_HANDLE) &&
             (mask1 & FBE_ENCRYPTION_KEY_MASK_VALID) &&
             (mask1 & FBE_ENCRYPTION_KEY_MASK_UPDATE)) 
    {
        /* We need to unregister the key first */
        port_unregister_keys->mp_key_handle[port_unregister_keys->num_of_keys] = key_info->mp_key_handle_2;
        port_unregister_keys->num_of_keys++;
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_is_unregister_keys_needed()
*******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_is_register_keys_needed()
 ******************************************************************************
 * @brief
 *  This function checks whether the PVD needs to register keys.
 * 
 * @param  key_info             - key info saved in PVD.
 * @param  register_keys        - keys pushed down from RG.
 * @param  port_register_keys   - params sending to port.
 *
 * @return status               - status of the operation.
 *
 * @author
 *  01/07/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_is_register_keys_needed(fbe_provision_drive_key_info_t *key_info,
                                            fbe_encryption_key_info_t *key_handle,
                                            fbe_base_port_mgmt_register_keys_t * port_register_keys,
                                            fbe_u32_t *first_handle)
{
    fbe_encryption_key_mask_t mask0, mask1;

    fbe_encryption_key_mask_get(&key_handle->key_mask, 0, &mask0);
    fbe_encryption_key_mask_get(&key_handle->key_mask, 1, &mask1);

    /* Initialize the structure */
    fbe_zero_memory(port_register_keys, sizeof(fbe_base_port_mgmt_register_keys_t));
    port_register_keys->num_of_keys = 0;

    if ((key_info->mp_key_handle_1 == FBE_INVALID_KEY_HANDLE) &&
        (mask0 & FBE_ENCRYPTION_KEY_MASK_VALID)) 
    {
        /* We have a key that needs to be registered */
        port_register_keys->key_ptr = (fbe_u64_t *)&key_handle->key1;
        *first_handle = 0;
        port_register_keys->num_of_keys++;
    } 
    else if ((key_info->mp_key_handle_1 != FBE_INVALID_KEY_HANDLE) &&
             (mask0 & FBE_ENCRYPTION_KEY_MASK_VALID) &&
             (mask0 & FBE_ENCRYPTION_KEY_MASK_UPDATE)) 
    {
        /* We need to register the key now */
        port_register_keys->key_ptr = (fbe_u64_t *)&key_handle->key1;
        *first_handle = 0;
        port_register_keys->num_of_keys++;
    }

    if ((key_info->mp_key_handle_2 == FBE_INVALID_KEY_HANDLE) &&
        (mask1 & FBE_ENCRYPTION_KEY_MASK_VALID)) 
    {
        port_register_keys->num_of_keys++;
        /* If we have 2 keys we leave the ptr to be to the first key. 
         * Otherwise we'll set the ptr to the 2nd key. 
         */
        if (port_register_keys->num_of_keys == 1) {
            port_register_keys->key_ptr = (fbe_u64_t *)&key_handle->key2;
            *first_handle = 1;
        }
    } 
    else if ((key_info->mp_key_handle_2 != FBE_INVALID_KEY_HANDLE) &&
             (mask1 & FBE_ENCRYPTION_KEY_MASK_VALID) &&
             (mask1 & FBE_ENCRYPTION_KEY_MASK_UPDATE)) 
    {
        port_register_keys->num_of_keys++;
        /* If we have 2 keys we leave the ptr to be to the first key. 
         * Otherwise we'll set the ptr to the 2nd key. 
         */
        if (port_register_keys->num_of_keys == 1) {
            port_register_keys->key_ptr = (fbe_u64_t *)&key_handle->key2;
            *first_handle = 1;
        }
    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_is_register_keys_needed()
*******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_unregister_keys_send_packet()
 ******************************************************************************
 * @brief
 *  This function is used to send unregister key operation to port.
 * 
 * @param  provision_drive      - provision drive object.
 * @param  packet               - pointer to the packet.
 * @param  port_unregister_keys - params sending to port.
 *
 * @return status               - status of the operation.
 *
 * @author
 *  01/07/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_unregister_keys_send_packet(fbe_provision_drive_t * provision_drive_p, 
                                                fbe_packet_t * packet_p, 
                                                fbe_base_port_mgmt_unregister_keys_t * port_unregister_keys,
                                                fbe_edge_index_t server_index,
                                                fbe_packet_completion_function_t completion_func)
{
    fbe_payload_ex_t *                          sep_payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_status_t                                status;
    fbe_scheduler_hook_status_t hook_status;

    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Unregister %d key(s) down for index:%d. \n",
                                __FUNCTION__, port_unregister_keys->num_of_keys, server_index);

    /* get the current control operation for the port information. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* No hook status is handled since we can only log here.
     */
     fbe_provision_drive_check_hook(provision_drive_p,
                                    SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION, 
                                    FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_UNREGISTER_KEYS, 
                                    0,  &hook_status);

    /* Build the control packet to get the physical drive inforamtion. */
    fbe_payload_control_build_operation(control_operation_p,
                                        FBE_BASE_PORT_CONTROL_CODE_UNREGISTER_KEYS,
                                        port_unregister_keys,
                                        sizeof(fbe_base_port_mgmt_unregister_keys_t));

    /* Set our completion function. */
    fbe_transport_set_completion_function(packet_p, 
                                          completion_func,
                                          provision_drive_p);

    if (provision_drive_p->key_info_table[server_index].port_object_id != FBE_OBJECT_ID_INVALID)
    {
        /* Set packet address */
        fbe_transport_set_address(  packet_p,
                                    FBE_PACKAGE_ID_PHYSICAL,
                                    FBE_SERVICE_ID_TOPOLOGY,
                                    FBE_CLASS_ID_INVALID,
                                    provision_drive_p->key_info_table[server_index].port_object_id);

        /* Control packets should be sent via service manager */
        status = fbe_service_manager_send_control_packet(packet_p);
    } else {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s no port object id\n", __FUNCTION__);
        /* Set the traverse attribute which allows traverse to this packet
         * till port to get the required information.
         */
        fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_TRAVERSE);
        status = fbe_block_transport_send_control_packet(((fbe_base_config_t *)provision_drive_p)->block_edge_p, packet_p);
    }

    return status;
}
/******************************************************************************
 * end fbe_provision_drive_unregister_keys_send_packet()
*******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_register_keys_send_packet()
 ******************************************************************************
 * @brief
 *  This function is used to send register key operation to port.
 * 
 * @param  provision_drive      - provision drive object.
 * @param  packet               - pointer to the packet.
 * @param  port_register_keys   - params sending to port.
 *
 * @return status               - status of the operation.
 *
 * @author
 *  01/07/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_register_keys_send_packet(fbe_provision_drive_t * provision_drive_p, 
                                              fbe_packet_t * packet_p, 
                                              fbe_base_port_mgmt_register_keys_t * port_register_keys)
{
    fbe_payload_ex_t *                          sep_payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_status_t                                status;

    /* get the current control operation for the port information. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Send %d key(s) down. \n",
                                __FUNCTION__, port_register_keys->num_of_keys);

    /* Build the control packet to get the physical drive inforamtion. */
    fbe_payload_control_build_operation(control_operation_p,
                                        FBE_BASE_PORT_CONTROL_CODE_REGISTER_KEYS,
                                        port_register_keys,
                                        sizeof(fbe_base_port_mgmt_register_keys_t));

    /* Set our completion function. */
    fbe_transport_set_completion_function(packet_p, 
                                          fbe_provision_drive_port_register_keys_completion,
                                          provision_drive_p);

    /* Set the traverse attribute which allows traverse to this packet
     * till port to get the required information.
     */
    fbe_transport_set_packet_attr(packet_p, FBE_PACKET_FLAG_TRAVERSE);
    status = fbe_block_transport_send_control_packet(((fbe_base_config_t *)provision_drive_p)->block_edge_p, packet_p);
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_register_keys_send_packet()
*******************************************************************************/
/*!****************************************************************************
 * fbe_provision_drive_register_release_memory()
 ******************************************************************************
 * @brief
 *  Release the control playload and the transport buffer.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  3/4/2014 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_register_release_memory(fbe_packet_t * packet_p,
                                            fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t                        *provision_drive_p = NULL;
    fbe_payload_ex_t                             *payload_p = NULL;
    fbe_payload_control_operation_t              *control_operation_p = NULL;
    fbe_payload_control_operation_opcode_t       opcode;
    fbe_base_port_mgmt_register_keys_t           *port_register_keys = NULL;
    fbe_base_port_mgmt_unregister_keys_t         *port_unregister_keys = NULL;
    fbe_u8_t *                                    buffer_p = NULL;
    fbe_provision_drive_register_keys_context_t  *context_p = NULL;
    fbe_base_config_encryption_state_t encryption_state;

    provision_drive_p = (fbe_provision_drive_t *)context;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_opcode(control_operation_p, &opcode);

    fbe_payload_control_get_buffer(control_operation_p, &buffer_p);
    if (opcode == FBE_BASE_PORT_CONTROL_CODE_REGISTER_KEYS) {
        port_register_keys = (fbe_base_port_mgmt_register_keys_t *) buffer_p;
        context_p = (fbe_provision_drive_register_keys_context_t*) (port_register_keys + 1);
    } else if (opcode == FBE_BASE_PORT_CONTROL_CODE_UNREGISTER_KEYS) {
        port_unregister_keys = (fbe_base_port_mgmt_unregister_keys_t *)buffer_p;
        context_p = (fbe_provision_drive_register_keys_context_t*) (port_unregister_keys + 1);
    } else {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s control opcode not correct 0x%x\n", __FUNCTION__, opcode);
    }

    fbe_provision_drive_lock( provision_drive_p );
    fbe_provision_drive_get_encryption_state(provision_drive_p, context_p->client_index, &encryption_state);
    if (encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_PUSH_KEYS_DOWNSTREAM) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "provision_drive: %d -> KEYS_PROVIDED server_idx: %d\n", encryption_state, context_p->client_index);
        fbe_provision_drive_set_encryption_state(provision_drive_p,
                                                 context_p->client_index,
                                                 FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED);
    }
    fbe_provision_drive_unlock( provision_drive_p );

    /* Release original context.
     */
    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
    fbe_transport_release_buffer(buffer_p);
    return FBE_STATUS_OK;
}
/********************************************************* 
* end fbe_provision_drive_register_release_memory()
**********************************************************/
/*!****************************************************************************
 * fbe_provision_drive_register_keys()
 ******************************************************************************
 * @brief
 *  This function is used to register the keys with miniport
 * 
 * @param  provision_drive      - provision drive object.
 * @param  packet               - pointer to the packet.
 *
 * @return status               - status of the operation.
 *
 * @author
 *  10/05/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_register_keys(fbe_provision_drive_t * provision_drive, 
                                  fbe_packet_t * packet,
                                  fbe_encryption_key_info_t *key_handle,
                                  fbe_edge_index_t server_index)
{
    fbe_payload_ex_t *                          payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_base_port_mgmt_register_keys_t *        port_register_keys;
    fbe_status_t                                status;
    fbe_payload_control_operation_t *           new_control_operation_p = NULL;
    fbe_provision_drive_register_keys_context_t * context_p = NULL;
    fbe_provision_drive_register_keys_context_t * unregister_context_p = NULL;
    fbe_base_port_mgmt_unregister_keys_t *      port_unregister_keys;
    fbe_base_config_encryption_state_t          encryption_state;
    fbe_scheduler_hook_status_t hook_status;
    fbe_path_attr_t path_attr;

    fbe_provision_drive_utils_trace(provision_drive,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    if(server_index >= FBE_PROVISION_DRIVE_KEY_INFO_TABLE_MAX)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s server Index %d greater than supported%d.\n",
                                __FUNCTION__, server_index, FBE_PROVISION_DRIVE_KEY_INFO_TABLE_MAX);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    new_control_operation_p = fbe_payload_ex_allocate_control_operation(payload);
    if(new_control_operation_p == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_payload_ex_increment_control_operation_level(payload);

    /* Allocate the memory for the physical drive management information. */
    port_register_keys = (fbe_base_port_mgmt_register_keys_t *) fbe_transport_allocate_buffer();
    context_p = (fbe_provision_drive_register_keys_context_t*) (port_register_keys + 1);
    
    port_unregister_keys = (fbe_base_port_mgmt_unregister_keys_t *)port_register_keys;
    unregister_context_p = (fbe_provision_drive_register_keys_context_t*) (port_unregister_keys + 1);

    if (port_register_keys == NULL)
    {
        fbe_payload_ex_release_control_operation(payload, new_control_operation_p);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* First check whether this is key registration already in progress */
    fbe_provision_drive_lock( provision_drive );
    fbe_provision_drive_get_encryption_state(provision_drive, server_index, &encryption_state);
    if (encryption_state == FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_PUSH_KEYS_DOWNSTREAM) {
        fbe_provision_drive_unlock( provision_drive );
        fbe_base_object_trace((fbe_base_object_t *)provision_drive, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "provision_drive: %d Keys in progress server_idx: %d\n", encryption_state, server_index);
        fbe_transport_release_buffer(port_register_keys);
        fbe_payload_ex_release_control_operation(payload, new_control_operation_p);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);

        return FBE_STATUS_OK;
    }

    /* Save the key handle */
    provision_drive->key_info_table[server_index].key_handle = key_handle;

    /* When we get a key push, change our encryption state so we will register keys.
     */
    fbe_base_object_trace((fbe_base_object_t *)provision_drive, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "provision_drive: %d -> PUSH_KEYS_DOWNSTREAM server_idx: %d\n", encryption_state, server_index);
    fbe_provision_drive_set_encryption_state(provision_drive,
                                             server_index,
                                             FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_PUSH_KEYS_DOWNSTREAM);
    fbe_provision_drive_unlock( provision_drive );


    fbe_base_object_trace((fbe_base_object_t *)provision_drive, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "register keys server: %d handle 1 0x%llx 2 0x%llx mask 0x%llx \n",
                          server_index,
                          provision_drive->key_info_table[server_index].mp_key_handle_1, 
                          provision_drive->key_info_table[server_index].mp_key_handle_2, 
                          key_handle->key_mask);

     /* No hook status is handled since we can only log here.
     */
     fbe_provision_drive_check_hook(provision_drive,
                                    SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION, 
                                    FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_REGISTER_KEYS, 
                                    0,  &hook_status);

    fbe_provision_drive_is_unregister_keys_needed(&provision_drive->key_info_table[server_index], 
                                                  key_handle, port_unregister_keys);
    if (port_unregister_keys->num_of_keys)
    {
        fbe_transport_set_completion_function(packet, fbe_provision_drive_register_release_memory, provision_drive);
        unregister_context_p->client_index = server_index;
        unregister_context_p->key_handle = key_handle;
        status = fbe_provision_drive_unregister_keys_send_packet(provision_drive, packet, port_unregister_keys, server_index, fbe_provision_drive_port_register_keys_unregister_completion);
        return status;
    }

    fbe_provision_drive_is_register_keys_needed(&provision_drive->key_info_table[server_index], key_handle, 
                                                port_register_keys, &context_p->first_handle);
    if (port_register_keys->num_of_keys)
    {
        fbe_path_state_t                            path_state; 
        fbe_block_transport_get_path_state(((fbe_base_config_t *)provision_drive)->block_edge_p, &path_state);
        if (path_state == FBE_PATH_STATE_INVALID) 
        {
            fbe_provision_drive_utils_trace(provision_drive, FBE_TRACE_LEVEL_INFO,
                                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                            "%s Edge not available. Save handle %p for server_index %x\n",
                                            __FUNCTION__, key_handle, server_index);

            fbe_provision_drive_lock( provision_drive );
            fbe_provision_drive_set_encryption_state(provision_drive,
                                                     server_index,
                                                     FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED);
            fbe_provision_drive_unlock( provision_drive );
            fbe_transport_release_buffer(port_register_keys);
            fbe_payload_ex_release_control_operation(payload, new_control_operation_p);
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
        fbe_transport_set_completion_function(packet, fbe_provision_drive_register_release_memory, provision_drive);
        context_p->client_index = server_index;
        context_p->key_handle = key_handle;
        port_register_keys->key_size = FBE_ENCRYPTION_KEY_SIZE;
        status = fbe_provision_drive_register_keys_send_packet(provision_drive, packet, port_register_keys);
        return status;
    }

    fbe_provision_drive_lock( provision_drive );
    fbe_provision_drive_set_encryption_state(provision_drive,
                                             server_index,
                                             FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED);
    fbe_provision_drive_unlock( provision_drive );
    fbe_base_object_trace((fbe_base_object_t *)provision_drive,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s Nothing to register. server_idx: %d\n",
                            __FUNCTION__, server_index);
    fbe_block_transport_server_get_path_attr(&((fbe_base_config_t *)provision_drive)->block_transport_server,
                                                         server_index,
                                                         &path_attr);
    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED)
    {
        fbe_provision_drive_utils_trace(provision_drive,
                                        FBE_TRACE_LEVEL_INFO,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "%s: Clear path attr: 0x%08x for index:%d\n", __FUNCTION__, path_attr, server_index);
        fbe_block_transport_server_clear_path_attr(&((fbe_base_config_t *)provision_drive)->block_transport_server,
                                                   server_index,
                                                   FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED);
    }
    fbe_transport_release_buffer(port_register_keys);
    fbe_payload_ex_release_control_operation(payload, new_control_operation_p);
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_register_keys()
*******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_control_register_keys()
 ******************************************************************************
 * @brief
 *  This function is used to register the keys with miniport
 * 
 * @param  provision_drive      - provision drive object.
 * @param  packet               - pointer to the packet.
 *
 * @return status               - status of the operation.
 *
 * @author
 *  10/05/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_control_register_keys(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet)
{
    fbe_provision_drive_control_register_keys_t * register_keys = NULL;
    fbe_status_t                                status;
    fbe_edge_index_t  server_index;
    fbe_lifecycle_state_t lifecycle_state;
    fbe_payload_ex_t *                          payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_transport_get_server_index(packet, &server_index);
    
    

    status = fbe_provision_drive_get_control_buffer(provision_drive, packet, 
                                                    sizeof(fbe_provision_drive_control_register_keys_t),
                                                    (fbe_payload_control_buffer_t *)&register_keys);
    if(status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    status = fbe_lifecycle_get_state(&fbe_provision_drive_lifecycle_const,(fbe_base_object_t*)provision_drive, &lifecycle_state);

    fbe_provision_drive_utils_trace(provision_drive, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "provision_drive_control_register keys server: %d state: 0x%x\n", server_index, lifecycle_state);
    /* When we are on system drives, we expect that the key will be provided by the client 
     * even if it is NULL. 
     * All edges need to inform us of the keys (or lack thereof) before we will 
     * transition the edge state to good. 
     */
    if (register_keys->key_handle == NULL) { /* This particular RG does not have a keys */
        fbe_path_state_t path_state;
                
        provision_drive->key_info_table[server_index].key_handle = NULL;

        status = fbe_base_transport_server_lifecycle_to_path_state(lifecycle_state, &path_state);

        /* We set the keys required first
         */
        fbe_block_transport_server_set_edge_path_state(&provision_drive->base_config.block_transport_server, 
                                                       server_index,
                                                       path_state);
        fbe_block_transport_server_clear_path_attr(&provision_drive->base_config.block_transport_server,
                                                   server_index,
                                                   FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED);
        fbe_provision_drive_utils_trace(provision_drive, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_ENCRYPTION,
                                        "provision_drive: register keys switch edge %d to path_state %d, Index:%d\n", 
                                        server_index, path_state, server_index);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* If we are in specialize state - just update the handle and get out of here */

//#if 0
    if(lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE){
        provision_drive->key_info_table[server_index].key_handle = register_keys->key_handle;
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
//#endif

    return fbe_provision_drive_register_keys(provision_drive, packet, register_keys->key_handle, server_index);
}
/******************************************************************************
 * end fbe_provision_drive_control_register_keys()
*******************************************************************************/
/*!****************************************************************************
 * fbe_provision_drive_port_register_keys_unregister_completion()
 ******************************************************************************
 * @brief
 *  This is the completion funtion for unregistering in a register operation.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/07/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_port_register_keys_unregister_completion(fbe_packet_t * packet_p,
                                                             fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                                 provision_drive_p = NULL;
    fbe_payload_ex_t *                                      sep_payload_p = NULL;
    fbe_payload_control_operation_t *                       control_operation_p = NULL;
    fbe_payload_control_operation_t *                       prev_control_operation_p = NULL;
    fbe_base_port_mgmt_unregister_keys_t *                  port_unregister_keys = NULL;
    fbe_base_port_mgmt_register_keys_t *                    port_register_keys = NULL;
    fbe_status_t                                            status;
    fbe_payload_control_status_t                            control_status;
    fbe_u8_t *                                              buffer_p = NULL;
    fbe_provision_drive_register_keys_context_t *           context_p = NULL;
    fbe_edge_index_t                                        server_index;
    fbe_u32_t                                               i;
    fbe_provision_drive_key_info_t *                        key_info;
    fbe_encryption_key_info_t *key_handle = NULL;

    provision_drive_p = (fbe_provision_drive_t *)context;

    /* get the current control operation for the port information. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the status of the control operation. */
    fbe_payload_control_get_status(control_operation_p, &control_status);
    status = fbe_transport_get_status_code(packet_p);
    /* get the buffer of the current control operation. */
    fbe_payload_control_get_buffer(control_operation_p, &buffer_p);
    port_unregister_keys = (fbe_base_port_mgmt_unregister_keys_t *) buffer_p;
    context_p = (fbe_provision_drive_register_keys_context_t*) (port_unregister_keys + 1);
    server_index = context_p->client_index;

    /* get the previous control operation and associated buffer. */
    prev_control_operation_p = fbe_payload_ex_get_prev_control_operation(sep_payload_p);
    
    key_handle = context_p->key_handle;

    /* If status is not good then complete the packet with error. */
    if((status != FBE_STATUS_OK) ||
       (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: unregister failed, ctl_stat:0x%x, stat:0x%x, \n",
                               __FUNCTION__, control_status, status);
        status = (status != FBE_STATUS_OK) ? status : FBE_STATUS_GENERIC_FAILURE;
        control_status = (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ? control_status : FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        fbe_transport_set_status(packet_p, status, 0);
        fbe_payload_control_set_status(prev_control_operation_p, control_status);
        return status;
    }

    /* Need to save the unregistered handle */
    key_info = &provision_drive_p->key_info_table[server_index];
    for (i = 0; i < port_unregister_keys->num_of_keys; i++)
    {
        if (port_unregister_keys->mp_key_handle[i] == key_info->mp_key_handle_1)
        {
            key_info->mp_key_handle_1 = FBE_INVALID_KEY_HANDLE;
        }
        else if (port_unregister_keys->mp_key_handle[i] == key_info->mp_key_handle_2)
        {
            key_info->mp_key_handle_2 = FBE_INVALID_KEY_HANDLE;
        }
    }

    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                          FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s Unregister complete server 0x%x\n", __FUNCTION__, server_index);

    /* Check whether we need to register keys */
    port_register_keys = (fbe_base_port_mgmt_register_keys_t *)port_unregister_keys;
    context_p = (fbe_provision_drive_register_keys_context_t*) (port_register_keys + 1);
    context_p->client_index = server_index;
    fbe_provision_drive_is_register_keys_needed(&provision_drive_p->key_info_table[server_index], key_handle, port_register_keys, &context_p->first_handle);
    if (port_register_keys->num_of_keys)
    {
        context_p->client_index = server_index;
        context_p->key_handle = key_handle;
        port_register_keys->key_size = FBE_ENCRYPTION_KEY_SIZE;
        status = fbe_provision_drive_register_keys_send_packet(provision_drive_p, packet_p, port_register_keys);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* We do not need to register. Re-set the client_index since it was wiped out already. */
    context_p = (fbe_provision_drive_register_keys_context_t*) (port_unregister_keys + 1);
    context_p->client_index = server_index;

    fbe_payload_control_set_status(prev_control_operation_p, control_status);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_port_register_keys_unregister_completion()
*******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_port_register_keys_completion()
 ******************************************************************************
 * @brief
 *  This function is used as a completion function for the port number and it
 *  issues another operation to get the enclosure number info.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  12/10/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_port_register_keys_completion(fbe_packet_t * packet_p,
                                                  fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                                 provision_drive_p = NULL;
    fbe_payload_ex_t *                                     sep_payload_p = NULL;
    fbe_payload_control_operation_t *                       control_operation_p = NULL;
    fbe_payload_control_operation_t *                       prev_control_operation_p = NULL;
    fbe_base_port_mgmt_register_keys_t *                    port_register_keys = NULL;
    fbe_status_t                                            status;
    fbe_payload_control_status_t                            control_status;
    fbe_u8_t *                                              buffer_p = NULL;
    fbe_provision_drive_register_keys_context_t * context_p = NULL;
    fbe_encryption_key_info_t *key_handle = NULL;
    fbe_object_id_t								    object_id;
    fbe_bool_t									    is_system_drive;
    fbe_system_encryption_mode_t system_encryption_mode;
    fbe_path_attr_t path_attr;

    fbe_database_get_system_encryption_mode(&system_encryption_mode);

    provision_drive_p = (fbe_provision_drive_t *)context;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    fbe_payload_control_get_status(control_operation_p, &control_status);
    status = fbe_transport_get_status_code(packet_p);

    prev_control_operation_p = fbe_payload_ex_get_prev_control_operation(sep_payload_p);
    
    fbe_payload_control_get_buffer(control_operation_p, &buffer_p);
    port_register_keys = (fbe_base_port_mgmt_register_keys_t *) buffer_p;
    context_p = (fbe_provision_drive_register_keys_context_t*) (port_register_keys + 1);
    key_handle = context_p->key_handle;

    /* If status is not good then complete the packet with error. */
    if((status != FBE_STATUS_OK) ||
       (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: get drive location failed, ctl_stat:0x%x, stat:0x%x, \n",
                               __FUNCTION__, control_status, status);
        status = (status != FBE_STATUS_OK) ? status : FBE_STATUS_GENERIC_FAILURE;
        control_status = (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ? control_status : FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        fbe_transport_set_status(packet_p, status, 0);
        fbe_payload_control_set_status(prev_control_operation_p, control_status);
        return status;
    }

    fbe_provision_drive_lock(provision_drive_p);
    /* Save the appropriate handle.  We may have been pushing down one or two keys and
     * the first index to save the handle might be the first or second index. 
     */
    if (context_p->first_handle == 0){
        provision_drive_p->key_info_table[context_p->client_index].mp_key_handle_1 =  port_register_keys->mp_key_handle[0];
        if (port_register_keys->num_of_keys == 2){
            /* There are 2 keys, save second handle.
             */
            provision_drive_p->key_info_table[context_p->client_index].mp_key_handle_2 =  port_register_keys->mp_key_handle[1];
        }
    } else{
        /* We were only getting one handle, but it is the second one.
         */
        provision_drive_p->key_info_table[context_p->client_index].mp_key_handle_2 =  port_register_keys->mp_key_handle[0];
    }

    port_register_keys->num_of_keys = 0;

    /* Save the port object id. */
    provision_drive_p->key_info_table[context_p->client_index].port_object_id = packet_p->object_id;

    fbe_provision_drive_unlock(provision_drive_p);

    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "PVD: reg complete index:%d key_handle:%p mp1:%llx mp2:%llx\n",
                          context_p->client_index,
                          provision_drive_p->key_info_table[context_p->client_index].key_handle,
                          provision_drive_p->key_info_table[context_p->client_index].mp_key_handle_1,
                          provision_drive_p->key_info_table[context_p->client_index].mp_key_handle_2);

    fbe_block_transport_server_get_path_attr(&((fbe_base_config_t *)provision_drive_p)->block_transport_server,
                                                         context_p->client_index,
                                                         &path_attr);
    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED) 
    {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_INFO,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "%s: Clear path attr: 0x%08x for index:%d\n", __FUNCTION__, path_attr, context_p->client_index);
        fbe_block_transport_server_clear_path_attr(&((fbe_base_config_t *)provision_drive_p)->block_transport_server,
                                                   context_p->client_index,
                                                   FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED);
    }

    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
    is_system_drive = fbe_database_is_object_system_pvd(object_id);

    /* Only user drives encrypt paged, so validate paged for user RGs.
     */
    if ( !is_system_drive &&
         fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_NP_FLAG_PAGED_VALID) &&
         fbe_base_config_is_active((fbe_base_config_t*)provision_drive_p) && /* We will do it on active side ONLY */
        (fbe_base_config_is_encrypted_mode((fbe_base_config_t *)provision_drive_p) ||
         fbe_base_config_is_rekey_mode((fbe_base_config_t *)provision_drive_p))) {
        
        fbe_provision_drive_verify_paged(provision_drive_p, packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const,
                             (fbe_base_object_t*)provision_drive_p,
                             (fbe_lifecycle_timer_msec_t)0);

    /* We did not transition the path state on attach, update it now.
     */
    if ((system_encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED) &&
        is_system_drive) {

        if (fbe_provision_drive_validation_area_needs_init(provision_drive_p, context_p->client_index)) {
            /* Initialize the area.
             */
            status = fbe_provision_drive_get_NP_lock(provision_drive_p, packet_p, fbe_provision_drive_init_validation_area);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        } else {
            /* validate the area.
             */
            fbe_provision_drive_verify_paged(provision_drive_p, packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }

    return FBE_STATUS_OK;
}
/********************************************************* 
* end fbe_provision_drive_port_register_keys_completion()
**********************************************************/

/*!****************************************************************************
 * fbe_provision_drive_register_keys()
 ******************************************************************************
 * @brief
 *  This function is used to register the keys with miniport
 * 
 * @param  provision_drive      - provision drive object.
 * @param  packet               - pointer to the packet.
 *
 * @return status               - status of the operation.
 *
 * @author
 *  10/05/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_get_miniport_key_handle(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t *                          payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_provision_drive_control_get_mp_key_handle_t * mp_key_handle = NULL;
    fbe_edge_index_t                            client_index;
    fbe_status_t                                status;
    
    fbe_provision_drive_utils_trace(provision_drive,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);

    status = fbe_provision_drive_get_control_buffer(provision_drive, packet, 
                                                    sizeof(fbe_provision_drive_control_get_mp_key_handle_t),
                                                    (fbe_payload_control_buffer_t *)&mp_key_handle);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    client_index = mp_key_handle->index;
    if(client_index >= FBE_PROVISION_DRIVE_KEY_INFO_TABLE_MAX)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Client Index %d greater than supported%d.\n",
                                __FUNCTION__, client_index, FBE_PROVISION_DRIVE_KEY_INFO_TABLE_MAX);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    mp_key_handle->mp_handle_1 = provision_drive->key_info_table[client_index].mp_key_handle_1; 
    mp_key_handle->mp_handle_2 = provision_drive->key_info_table[client_index].mp_key_handle_2;

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_register_keys()
*******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_get_verify_invalidate_checkpoint()
 ******************************************************************************
 * @brief
 *  This function is used to get the verify invalidate checkpoint.
 * 
 * @param  provision_drive      - provision drive object.
 * @param  packet               - pointer to the packet.
 *
 * @return status               - status of the operation.
 *
 * @author
 *  03/17/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_get_verify_invalidate_checkpoint(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t *                          payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_provision_drive_get_verify_invalidate_checkpoint_t * get_verify_invalidate_checkpoint = NULL;
    fbe_status_t                                status;

    fbe_provision_drive_utils_trace(provision_drive,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    status = fbe_provision_drive_get_control_buffer(provision_drive, packet,
                                                    sizeof(fbe_provision_drive_get_verify_invalidate_checkpoint_t),
                                                    (fbe_payload_control_buffer_t *)&get_verify_invalidate_checkpoint);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)provision_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    status = fbe_provision_drive_metadata_get_verify_invalidate_checkpoint(provision_drive, &get_verify_invalidate_checkpoint->verify_invalidate_checkpoint);
    if (status != FBE_STATUS_OK) 
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "pvd_get_verify_invalidate_chkpt: Failed to get checkpoint. Status: 0x%X\n", status);

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
 * end fbe_provision_drive_get_verify_invalidate_checkpoint()
*******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_set_verify_invalidate_checkpoint()
 ******************************************************************************
 * @brief
 *  This function is used to set the verify_invalidate checkpoint.
 * 
 * @param  provision_drive      - provision drive object.
 * @param  packet               - pointer to the packet.
 *
 * @return status               - status of the operation.
 *
 * @author
 *  03/17/2012 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_set_verify_invalidate_checkpoint(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet)
{
    fbe_status_t                                status;
    fbe_payload_ex_t *                         payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_provision_drive_set_verify_invalidate_checkpoint_t * set_verify_invalidate_checkpoint = NULL;
    fbe_lba_t                           exported_capacity = FBE_LBA_INVALID;
    fbe_lba_t                           verify_invalidate_checkpoint = FBE_LBA_INVALID;
    fbe_bool_t                          b_is_verify_invalidate_request_aligned_to_chunk = FBE_FALSE;
    fbe_lba_t                           default_offset = 0;

    fbe_provision_drive_utils_trace(provision_drive,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  


    status = fbe_provision_drive_get_control_buffer(provision_drive, packet,
                                                    sizeof(fbe_provision_drive_set_verify_invalidate_checkpoint_t),
                                                    (fbe_payload_control_buffer_t *)&set_verify_invalidate_checkpoint);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)provision_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }
    /* Validate the verify_invalidate checkpoint request
     */

    /* get the paged metadatas start lba. */
    fbe_base_config_get_capacity((fbe_base_config_t *) provision_drive, &exported_capacity);

    /* check if disk verify_invalidateing completed */
    verify_invalidate_checkpoint = set_verify_invalidate_checkpoint->verify_invalidate_checkpoint;

    /* Invalid verify_invalidate checkpoint should bypass all the rules */
    if(verify_invalidate_checkpoint == FBE_LBA_INVALID){	    
        status = fbe_provision_drive_metadata_set_verify_invalidate_chkpt_without_persist(provision_drive, packet, verify_invalidate_checkpoint);   
        return status;
    }

    if(verify_invalidate_checkpoint  > exported_capacity)
    {
        /* disk verify_invalidateing completed */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s:checkpoint cannot be updated beyond capacity, verify_invalidate_chk:0x%llx, export_cap:0x%llx\n",
                              __FUNCTION__, (unsigned long long)verify_invalidate_checkpoint, (unsigned long long)exported_capacity);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now validate that the checkpoint is aligned to the provision drive chunk.
     */
    status = fbe_provision_drive_utils_is_io_request_aligned(verify_invalidate_checkpoint, FBE_PROVISION_DRIVE_CHUNK_SIZE, 
                                                             FBE_PROVISION_DRIVE_CHUNK_SIZE, &b_is_verify_invalidate_request_aligned_to_chunk);
    if((status != FBE_STATUS_OK) || 
       (!b_is_verify_invalidate_request_aligned_to_chunk))
    {
        /* Zero checkpoint must be aligned to chunk size.
         */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: verify_invalidate_checkpoint: 0x%llx is not aligned to pvd chunk size: 0x%x\n",
                              __FUNCTION__, (unsigned long long)verify_invalidate_checkpoint, FBE_PROVISION_DRIVE_CHUNK_SIZE);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! @todo Currently we do not allow the background verify_invalidate checkpoint to be 
     *        set below the default offset.
     */
    fbe_base_config_get_default_offset((fbe_base_config_t *)provision_drive, &default_offset);
    if (verify_invalidate_checkpoint < default_offset)
    {
        /*! @todo Currently we don't allow the background verify_invalidate checkpoint to be
         *        less than the default_offset.
         */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: verify_invalidate_checkpoint: 0x%llx is less than the default_offset: 0x%llx\n",
                              __FUNCTION__, (unsigned long long)verify_invalidate_checkpoint, (unsigned long long)default_offset);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now set the completion function and set the non-paged value (without persist).
     */    
    status = fbe_provision_drive_metadata_set_verify_invalidate_chkpt_without_persist(provision_drive, packet, verify_invalidate_checkpoint);   

    return status;
}
/******************************************************************************
 * end fbe_provision_drive_set_verify_invalidate_checkpoint()
*******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_initiate_disk_zeroing()
 ******************************************************************************
 * @brief
 *  This function sets up a disk zeroing operation for this provision drive
 *  object.
 *  PVD performs following while handling user zero_disk request
 *   1. set the following bitmap pattern for entire disk.
 *      NZ - 1, UZ - 0, CU - 0
 *   2. backward the zero checkpoint to default offset
 *   3. transition disk into ZOD state if it is not.
 *
 * @param provision_drive   - The provision drive.
 * @param in_packet_p        - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/15/2010 - Amit Dhaduk
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_initiate_disk_zeroing(fbe_provision_drive_t*   provision_drive_p,
                                          fbe_packet_t*            packet_p)
{
    fbe_status_t    status;
    fbe_u32_t number_of_clients = 0;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_block_transport_server_t *block_transport_server_p = &provision_drive_p->base_config.block_transport_server;
    fbe_memory_request_t *           memory_request_p = NULL;
    fbe_memory_request_priority_t    memory_request_priority = 0;
    
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Disk zeroing is only allowed on a drive that is not bound.
     */
    status = fbe_block_transport_server_get_number_of_clients(block_transport_server_p,
                                                              &number_of_clients);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: error getting number of clients status: 0x%x\n", __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
    if (number_of_clients != 0)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: There are %d attached clients, no zero allowed.\n", __FUNCTION__, number_of_clients);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    /* We need a subpacket since we ran out of completion function stack in the packet.
     */
    memory_request_p = fbe_transport_get_memory_request(packet_p);
    memory_request_priority = fbe_transport_get_resource_priority(packet_p);
    memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;

    /* build the memory request for allocation.  We add (1) to the memory
     * request priority of the packet. */
    fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                   1,
                                   memory_request_priority,
                                   fbe_transport_get_io_stamp(packet_p),
                                   (fbe_memory_completion_function_t)fbe_provision_drive_initiate_zeroing_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   (fbe_memory_completion_context_t)provision_drive_p);

    fbe_transport_memory_request_set_io_master(memory_request_p,  packet_p);

    fbe_memory_request_entry(memory_request_p);
    /* Completion function is always called.
     */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_initiate_disk_zeroing()
*******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_initiate_zeroing_allocation_completion()
 ******************************************************************************
 * @brief
 *  This is the completion for allocating memory for the packet we use for zeroing.
 *
 * @param memory_request - Pointer to the memory request.
 * @param context        - Pointer to the provision drive object.
 *
 * @return fbe_status_t.
 * 
 * @author
 *  12/13/2011 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_initiate_zeroing_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                                           fbe_memory_completion_context_t context)
{
    fbe_status_t status;
    fbe_provision_drive_t *provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_packet_t *master_packet_p = NULL;
    fbe_packet_t *sub_packet_p = NULL;
    fbe_payload_ex_t *master_payload_p = NULL;
    fbe_payload_ex_t *sub_payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_lba_t default_offset;
    fbe_block_count_t capacity; 
    fbe_block_transport_server_t *block_transport_server_p = &provision_drive_p->base_config.block_transport_server;
    fbe_block_edge_t *block_edge_p = NULL;
    fbe_block_edge_geometry_t edge_geometry;
    fbe_optimum_block_size_t optimum_block_size;
    fbe_memory_header_t * master_memory_header = NULL; 

    master_packet_p = fbe_transport_memory_request_to_packet(memory_request_p);
    master_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(master_payload_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE) {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_provision_drive_utils_trace(provision_drive_p, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "pvd initiate disk zeroing memory request: 0x%p state: %d allocation failed \n",
                                        memory_request_p, memory_request_p->request_state);
        fbe_payload_block_set_status(block_operation_p,
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_IO_FAILED,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNEXPECTED_ERROR);

        fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(master_packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    master_memory_header = memory_request_p->ptr;   
    sub_packet_p = (fbe_packet_t*)fbe_memory_header_to_data(master_memory_header);
    fbe_transport_initialize_packet(sub_packet_p);
    sub_payload_p = fbe_transport_get_payload_ex(sub_packet_p);

    block_operation_p = fbe_payload_ex_allocate_block_operation(sub_payload_p);
    if (block_operation_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: error alloc null block op\n", __FUNCTION__);

        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(memory_request_p);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(master_packet_p);
        return FBE_STATUS_OK;
    }

    fbe_base_config_get_default_offset((fbe_base_config_t*)provision_drive_p, &default_offset);
    fbe_base_config_get_capacity((fbe_base_config_t*)provision_drive_p, &capacity);
    fbe_base_config_get_block_edge((fbe_base_config_t*)provision_drive_p, &block_edge_p, 0);

    /* Sanity check to make sure the block count is a multiple of chunk size */
    if((capacity - default_offset) % FBE_PROVISION_DRIVE_CHUNK_SIZE)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Blk count not a mult of chunk size\n", __FUNCTION__);
        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(memory_request_p);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(master_packet_p);
        return FBE_STATUS_OK;
    }

    if (block_edge_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: no block edges.\n", __FUNCTION__);
        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(memory_request_p);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(master_packet_p);
        return FBE_STATUS_OK;
    }
    fbe_block_transport_edge_get_geometry(block_edge_p, &edge_geometry);
    fbe_block_transport_get_optimum_block_size(edge_geometry, &optimum_block_size);

    /* We will zero from the start offset all the way up to the end of the exported area of the PVD.
     */
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                            FBE_TRACE_LEVEL_INFO, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "MARK_DISK_ZERO: lba: 0x%llX bc: 0x%llX\n", (unsigned long long)default_offset, (unsigned long long)capacity - default_offset);

    status = fbe_payload_block_build_operation(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_MARK_DISK_ZERO,
                                               default_offset, capacity - default_offset, FBE_BE_BYTES_PER_BLOCK, optimum_block_size, NULL);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: error build block op status: 0x%x\n", __FUNCTION__, status);
        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(memory_request_p);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(master_packet_p, status, 0);
        fbe_transport_complete_packet(master_packet_p);
        return FBE_STATUS_OK;
    }

    status = fbe_payload_ex_increment_block_operation_level(sub_payload_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: error inc block level status: 0x%x\n", __FUNCTION__, status);
        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(memory_request_p);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(master_packet_p, status, 0);
        fbe_transport_complete_packet(master_packet_p);
        return FBE_STATUS_OK;
    }
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "pvd start disk zeroing mark lba: 0x%llx bl: 0x%llx for zero.\n", 
                          (unsigned long long)block_operation_p->lba,
              (unsigned long long)block_operation_p->block_count);

    fbe_transport_add_subpacket(master_packet_p, sub_packet_p);
    fbe_transport_set_completion_function(sub_packet_p, fbe_provision_drive_initiate_disk_zeroing_completion, provision_drive_p);

    status = fbe_block_transport_server_bouncer_entry(block_transport_server_p, 
                                                      sub_packet_p, 
                                                      (fbe_base_object_t *)provision_drive_p);
    if(block_transport_server_p->attributes & FBE_BLOCK_TRANSPORT_ENABLE_STACK_LIMIT){
        if(status == FBE_STATUS_OK){
            block_transport_server_p->block_transport_const->block_transport_entry((fbe_base_object_t *)provision_drive_p, sub_packet_p);
        }
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_provision_drive_initiate_zeroing_allocation_completion()
 **************************************/

/*!**************************************************************
 * fbe_provision_drive_initiate_disk_zeroing_completion()
 ****************************************************************
 * @brief
 *  Completion for when we have kicked off the user zero of
 *  the entire capacity.
 *
 * @param paket_p -  current usurper op.
 * @param context - the provision drive.             
 *
 * @return -   
 *
 * @author
 *  12/9/2011 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t
fbe_provision_drive_initiate_disk_zeroing_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_payload_ex_t *sub_payload_p = NULL;
    fbe_payload_ex_t *master_payload_p = NULL;
    fbe_payload_block_operation_status_t block_status;
    fbe_status_t status;
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_packet_t *master_packet_p = fbe_transport_get_master_packet(packet_p);
    fbe_memory_request_t *memory_request_p = NULL;
    fbe_u32_t number_of_clients = 0;
    fbe_block_transport_server_t *block_transport_server_p = &provision_drive_p->base_config.block_transport_server;

    sub_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(sub_payload_p);
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_block_get_status(block_operation_p, &block_status);

    fbe_payload_ex_release_block_operation(sub_payload_p, block_operation_p);

    master_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(master_payload_p);

    /* We need to remove packet from master queue */
    fbe_transport_remove_subpacket(packet_p);

    fbe_transport_destroy_packet(packet_p);

    memory_request_p = fbe_transport_get_memory_request(master_packet_p);
    //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
    //fbe_memory_free_entry(memory_ptr);
    fbe_memory_free_request_entry(memory_request_p);

    if ((status != FBE_STATUS_OK) || (block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "start disk zero, user zero failed, status:%d bl_status: 0x%x.\n",
                              status, block_status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(master_packet_p, status, 0);
        fbe_transport_complete_packet(master_packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);

    status = fbe_block_transport_server_get_number_of_clients(block_transport_server_p,
                                                              &number_of_clients);
    if ((status == FBE_STATUS_OK) && (number_of_clients == 0))
    {
        fbe_provision_drive_event_log_disk_zero_start_or_complete(provision_drive_p, FBE_TRUE, master_packet_p);
    }

    fbe_transport_complete_packet(master_packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
} 
/******************************************
 * end fbe_provision_drive_initiate_disk_zeroing_completion()
 ******************************************/

/*!****************************************************************************
 * fbe_provision_drive_initiate_consumed_area_zeroing()
 ******************************************************************************
 * @brief
 *  This function sets up a disk zeroing operation for this provision drive
 *  object.
 *  PVD performs following while handling user zero_disk request
 *   1. set the following bitmap pattern for area where CU is set.
 *      NZ - 1, UZ - 0, CU - 0
 *   2. backward the zero checkpoint to default offset
 *   3. transition disk into ZOD state if it is not.
 *
 * @param provision_drive   - The provision drive.
 * @param in_packet_p        - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_initiate_consumed_area_zeroing(fbe_provision_drive_t*   provision_drive_p,
                                                   fbe_packet_t*            packet_p)
{
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_memory_request_t *           memory_request_p = NULL;
    fbe_memory_request_priority_t    memory_request_priority = 0;
    
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* We need a subpacket since we ran out of completion function stack in the packet.
     */
    memory_request_p = fbe_transport_get_memory_request(packet_p);
    memory_request_priority = fbe_transport_get_resource_priority(packet_p);
    memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;

    /* build the memory request for allocation.  We add (1) to the memory
     * request priority of the packet. */
    fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_64_BLOCKS_64_BLOCKS,
                                   2,    /* SG, packet*/
                                   memory_request_priority,
                                   fbe_transport_get_io_stamp(packet_p),
                                   (fbe_memory_completion_function_t)fbe_provision_drive_initiate_consumed_area_zeroing_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   (fbe_memory_completion_context_t)provision_drive_p);

    fbe_transport_memory_request_set_io_master(memory_request_p,  packet_p);

    fbe_memory_request_entry(memory_request_p);
    /* Completion function is always called.
     */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_initiate_consumed_area_zeroing()
*******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_initiate_consumed_area_zeroing_allocation_completion()
 ******************************************************************************
 * @brief
 *  This is the completion for allocating memory for the packet we use for zeroing.
 *
 * @param memory_request - Pointer to the memory request.
 * @param context        - Pointer to the provision drive object.
 *
 * @return fbe_status_t.
 * 
 * @author
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_initiate_consumed_area_zeroing_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                                                         fbe_memory_completion_context_t context)
{
    fbe_status_t status;
    fbe_provision_drive_t *provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_packet_t *master_packet_p = NULL;
    fbe_packet_t *sub_packet_p = NULL;
    fbe_payload_ex_t *master_payload_p = NULL;
    fbe_payload_ex_t *sub_payload_p = NULL;    
    fbe_payload_block_operation_t *block_operation_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_lba_t default_offset;
    fbe_block_count_t capacity; 
    fbe_block_transport_server_t *block_transport_server_p = &provision_drive_p->base_config.block_transport_server;
    fbe_block_edge_t *block_edge_p = NULL;
    fbe_block_edge_geometry_t edge_geometry;
    fbe_optimum_block_size_t optimum_block_size;
    fbe_memory_header_t * master_memory_header = NULL; 
    fbe_lba_t                         lba;
    fbe_block_count_t                 blocks;
    fbe_provision_drive_config_type_t current_config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_INVALID;

    master_packet_p = fbe_transport_memory_request_to_packet(memory_request_p);
    master_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(master_payload_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE) {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_provision_drive_utils_trace(provision_drive_p, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "pvd initiate consumed zeroing request: 0x%p state: %d allocation failed \n",
                                        memory_request_p, memory_request_p->request_state);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);

        fbe_transport_complete_packet(master_packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    master_memory_header = memory_request_p->ptr;   
    sub_packet_p = (fbe_packet_t*)fbe_memory_header_to_data(master_memory_header);
    fbe_transport_initialize_packet(sub_packet_p);
    sub_payload_p = fbe_transport_get_payload_ex(sub_packet_p);

    fbe_base_config_get_default_offset((fbe_base_config_t*)provision_drive_p, &default_offset);
    fbe_base_config_get_capacity((fbe_base_config_t*)provision_drive_p, &capacity);
    fbe_base_config_get_block_edge((fbe_base_config_t*)provision_drive_p, &block_edge_p, 0);

    fbe_provision_drive_get_config_type(provision_drive_p, &current_config_type);
    if (current_config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID) {
        fbe_provision_drive_get_unconsumed_mark_range(provision_drive_p, &lba, &blocks);
        if (blocks < FBE_PROVISION_DRIVE_CHUNK_SIZE) {
            /* The extent is totally consumed.  Clear the intent (so that scrub
             * does not run) and set the scrub needed.
             */
            fbe_provision_drive_utils_trace(provision_drive_p, 
                                  FBE_TRACE_LEVEL_INFO, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  FBE_PROVISION_DRIVE_DEBUG_FLAG_BGZ_TRACING,
                                  "pvd initiate consumed zeroing blocks: 0x%llx extent is entirely consumed. No zeroing required.\n",
                                  blocks);

            fbe_transport_add_subpacket(master_packet_p, sub_packet_p);
            fbe_transport_set_completion_function(sub_packet_p, fbe_provision_drive_set_scrub_bits_completion, provision_drive_p);
            fbe_transport_set_completion_function(sub_packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);
            fbe_provision_drive_get_NP_lock(provision_drive_p, sub_packet_p, fbe_provision_drive_set_scrub_intent_bits);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        } else if (lba <= default_offset) {
            /* Small window between the provision drive being marked consumed and
             * edges being attached. Just return and wait for edge.
             */
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                  FBE_TRACE_LEVEL_INFO, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "pvd initiate consumed zeroing lba: 0x%llx blocks: 0x%llx wait for edge attach.\n",
                                  lba, blocks);
            fbe_memory_free_request_entry(memory_request_p);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
            fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(master_packet_p);
            return FBE_STATUS_OK;
        }
    } else {
        lba = default_offset;
        blocks = capacity - default_offset;
    }

    block_operation_p = fbe_payload_ex_allocate_block_operation(sub_payload_p);
    if (block_operation_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: error alloc null block op\n", __FUNCTION__);

        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(memory_request_p);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(master_packet_p);
        return FBE_STATUS_OK;
    }

    /* Sanity check to make sure the block count is a multiple of chunk size */
    if(blocks % FBE_PROVISION_DRIVE_CHUNK_SIZE)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: Blk count not a mult of chunk size\n", __FUNCTION__);
        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(memory_request_p);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(master_packet_p);
        return FBE_STATUS_OK;
    }

    if (block_edge_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: no block edges.\n", __FUNCTION__);
        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(memory_request_p);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(master_packet_p);
        return FBE_STATUS_OK;
    }
    fbe_block_transport_edge_get_geometry(block_edge_p, &edge_geometry);
    fbe_block_transport_get_optimum_block_size(edge_geometry, &optimum_block_size);

    /* We will zero from the start offset all the way up to the end of the exported area of the PVD.
     */
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                            FBE_TRACE_LEVEL_INFO, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "MARK_CONSUMED_ZERO: lba: 0x%llX bc: 0x%llX\n", (unsigned long long)lba, (unsigned long long)blocks);

    status = fbe_payload_block_build_operation(block_operation_p, FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_MARK_CONSUMED_ZERO,
                                               lba, blocks, FBE_BE_BYTES_PER_BLOCK, optimum_block_size, NULL);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: error build block op status: 0x%x\n", __FUNCTION__, status);
        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(memory_request_p);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(master_packet_p, status, 0);
        fbe_transport_complete_packet(master_packet_p);
        return FBE_STATUS_OK;
    }

    status = fbe_payload_ex_increment_block_operation_level(sub_payload_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: error inc block level status: 0x%x\n", __FUNCTION__, status);
        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);
        fbe_memory_free_request_entry(memory_request_p);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(master_packet_p, status, 0);
        fbe_transport_complete_packet(master_packet_p);
        return FBE_STATUS_OK;
    }
    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "pvd start disk zeroing mark lba: 0x%llx bl: 0x%llx for zero.\n", 
                          (unsigned long long)block_operation_p->lba,
              (unsigned long long)block_operation_p->block_count);

    fbe_transport_add_subpacket(master_packet_p, sub_packet_p);
    fbe_transport_set_completion_function(sub_packet_p, fbe_provision_drive_initiate_disk_zeroing_completion, provision_drive_p);

    status = fbe_block_transport_server_bouncer_entry(block_transport_server_p, 
                                                      sub_packet_p, 
                                                      (fbe_base_object_t *)provision_drive_p);
    if(block_transport_server_p->attributes & FBE_BLOCK_TRANSPORT_ENABLE_STACK_LIMIT){
        if(status == FBE_STATUS_OK){
            block_transport_server_p->block_transport_const->block_transport_entry((fbe_base_object_t *)provision_drive_p, sub_packet_p);
        }
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/***********************************************************************************
 * end fbe_provision_drive_initiate_consumed_area_zeroing_allocation_completion()
 ***********************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_enable_background_zeroing_completion
 ******************************************************************************
 *
 * @brief
 *    This function handles enable background zeroing operation completion. 
 *    It validates the status and complete the operation. 
 *    
 *
 * @param   in_packet_p           -  pointer to control packet requesting this
 *                                   operation
 * @param   context               -  context, which is a pointer to the base config
 *                                   object
 *
 * @return  fbe_status_t          -  status of this operation
 *
 * @version
 *    08/16/2011 - Created. Amit Dhaduk
 *
 ******************************************************************************/

static fbe_status_t
fbe_provision_drive_enable_background_zeroing_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{

    fbe_provision_drive_t *                 provision_drive_p = NULL;
    fbe_payload_ex_t *                 sep_payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_payload_control_status_t        control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_status_t                        status;  
    
    provision_drive_p = (fbe_provision_drive_t *)context;

     /* Get sep payload. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);

    /* Get control operation. */
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Get the packet status */
    status = fbe_transport_get_status_code(packet_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s background operation enable failed, status:%d.",
                                __FUNCTION__, status);
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;

        fbe_payload_control_set_status(control_operation_p, control_status);
        fbe_transport_set_status(packet_p, status, 0);
    }
    
    return status;

} 
/******************************************************************************
 * end fbe_provision_drive_enable_background_zeroing_completion()
*******************************************************************************/


/*!****************************************************************************
 * fbe_provision_drive_get_control_buffer()
 ******************************************************************************
 *
 * @brief
 *    This function retrieves the control buffer pointer out of the specified
 *    packet and validates it.  An error is returned if the buffer pointer is
 *    null or if the buffer size does not match the specified buffer length.
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 * @param   in_packet_p           -  pointer to control packet requesting this
 *                                   operation
 * @param   in_buffer_length      -  expected length of buffer
 * @param   out_buffer_pp         -  pointer to buffer pointer
 *
 * @return  fbe_status_t          -  status of this operation
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/

fbe_status_t fbe_provision_drive_get_control_buffer(fbe_provision_drive_t * in_provision_drive_p,
                                                    fbe_packet_t * in_packet_p,
                                                    fbe_payload_control_buffer_length_t in_buffer_length,
                                                    fbe_payload_control_buffer_t * out_buffer_pp)
{
    fbe_payload_control_buffer_length_t buffer_length;        // requestor's buffer length
    fbe_payload_control_operation_t*    control_operation_p;  // control operation
    fbe_payload_ex_t*                  sep_payload_p;        // sep payload

    // get control op buffer from sep packet payload
    sep_payload_p = fbe_transport_get_payload_ex( in_packet_p );
    if(sep_payload_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)in_provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s payload is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation_p = fbe_payload_ex_get_any_control_operation( sep_payload_p );
    if(control_operation_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)in_provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s control operation is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // check if buffer pointer is null
    fbe_payload_control_get_buffer( control_operation_p, out_buffer_pp);
    if (*out_buffer_pp == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)in_provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s buffer pointer is NULL\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    // check for buffer length mismatch
    fbe_payload_control_get_buffer_length(control_operation_p, &buffer_length);
    if(buffer_length != in_buffer_length)
    {
        fbe_base_object_trace((fbe_base_object_t *)in_provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s buffer length mismatch\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    // return success
    return FBE_STATUS_OK;

}   // end fbe_provision_drive_get_control_buffer()

/*!****************************************************************************
 * fbe_provision_drive_metadata_clr_sniff_pass_persist_completion
 ******************************************************************************
 *
 * @brief
 *   This function handles NP's sniff verify pass count update completion.
 *   It completes the operation with appropriate status.
 *   
 * @param in_packet_p      - pointer to a control packet from the scheduler
 * @param in_context       - context, which is a pointer to the provision drive
 *                           object
 *
 * @version
 *    07/01/2014 - Created. SaranyaDevi Ganesan
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_metadata_clr_sniff_pass_persist_completion(
                                    fbe_packet_t *packet_p, 
                                    fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *provision_drive_p = NULL;
    fbe_status_t status;
    fbe_u32_t pass_count = 5; //Set some dummy value to see if this is really cleared

    provision_drive_p = (fbe_provision_drive_t *)context;

    /* Get the packet status */
    status = fbe_transport_get_status_code(packet_p);

    /* Get the NP metdata sniff verify pass count*/
    fbe_provision_drive_metadata_get_sniff_pass_count(provision_drive_p, &pass_count);

    /* Check if sniff pass count cleared successfully */
    if (status != FBE_STATUS_OK || pass_count != 0)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s sniff verify pass count clear failed, status:%d.",
                                __FUNCTION__, status);

        return status;

    }

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_metadata_clr_sniff_pass_persist_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_usurper_clear_verify_report
 ******************************************************************************
 *
 * @brief
 *    This function clears the sniff verify report on the specified provision
 *    drive.
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 * @param   in_packet_p           -  pointer to control packet requesting this
 *                                   operation
 *
 * @return  fbe_status_t          -  status of this operation
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/

static fbe_status_t
fbe_provision_drive_usurper_clear_verify_report(
                                                 fbe_provision_drive_t* in_provision_drive_p,
                                                 fbe_packet_t*          in_packet_p
                                               )
{
    fbe_provision_drive_verify_report_t* verify_report_p;   // pointer to sniff verify report

    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( in_provision_drive_p );

    // get pointer to sniff verify report
    fbe_provision_drive_get_verify_report_ptr( in_provision_drive_p, &verify_report_p );

    // lock access to sniff verify report
    fbe_provision_drive_lock( in_provision_drive_p );

    // clear sniff verify report
    fbe_zero_memory( (void *)verify_report_p, sizeof (fbe_provision_drive_verify_report_t) );

    // set sniff verify report revision
    verify_report_p->revision = FBE_PROVISION_DRIVE_VERIFY_REPORT_REV;

    // unlock access to sniff verify report
    fbe_provision_drive_unlock( in_provision_drive_p );

    // set completion function for clear sniff pass count
    fbe_transport_set_completion_function(in_packet_p, fbe_provision_drive_metadata_clr_sniff_pass_persist_completion, in_provision_drive_p);

    // get NP lock and clear sniff pass count in the nonpaged metadata
    fbe_provision_drive_get_NP_lock(in_provision_drive_p, in_packet_p, fbe_provision_drive_metadata_clear_sniff_pass_count);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}   // end fbe_provision_drive_usurper_clear_verify_report()


/*!****************************************************************************
 * fbe_provision_drive_usurper_get_verify_report
 ******************************************************************************
 *
 * @brief
 *    This function gets the sniff verify report for the specified provision
 *    drive.
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 * @param   in_packet_p           -  pointer to control packet requesting this
 *                                   operation
 *
 * @return  fbe_status_t          -  status of this operation
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/

static fbe_status_t
fbe_provision_drive_usurper_get_verify_report(
                                               fbe_provision_drive_t* in_provision_drive_p,
                                               fbe_packet_t*          in_packet_p
                                             )
{
    fbe_provision_drive_verify_report_t*  req_buffer_p;    // pointer to requestor's buffer
    fbe_provision_drive_verify_report_t*  verify_report_p; // pointer to sniff verify report
    fbe_status_t                          status;          // fbe status

    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( in_provision_drive_p );

    // get control buffer pointer from sep packet payload
    status = fbe_provision_drive_get_control_buffer( in_provision_drive_p,
                                                     in_packet_p,
                                                     sizeof (fbe_provision_drive_verify_report_t),
                                                     (fbe_payload_control_buffer_t) &req_buffer_p
                                                   );
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)in_provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_transport_set_status(in_packet_p, status, 0);
        fbe_transport_complete_packet(in_packet_p);
        return status;
    }

    // get sniff verify report pointer for this disk
    fbe_provision_drive_get_verify_report_ptr( in_provision_drive_p, &verify_report_p );

    // update verify pass count from non-paged metadata
    fbe_provision_drive_metadata_get_sniff_pass_count(in_provision_drive_p, &verify_report_p->pass_count);

    // lock access to sniff verify report
    fbe_provision_drive_lock( in_provision_drive_p );

    // now, copy sniff verify report information into requestor's buffer
    *req_buffer_p = *verify_report_p;

    // unlock access to sniff verify report
    fbe_provision_drive_unlock( in_provision_drive_p );

    // set success status in packet and complete operation
    fbe_transport_set_status( in_packet_p, FBE_STATUS_OK, 0 );
    fbe_transport_complete_packet( in_packet_p );

    // return success
    return FBE_STATUS_OK;

}   // end fbe_provision_drive_usurper_get_verify_report()


/*!****************************************************************************
 * fbe_provision_drive_usurper_get_verify_status
 ******************************************************************************
 *
 * @brief
 *    This function gets the verify status for the specified provision drive.
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 * @param   in_packet_p           -  pointer to control packet requesting this
 *                                   operation
 *
 * @return  fbe_status_t          -  status of this operation
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/

static fbe_status_t
fbe_provision_drive_usurper_get_verify_status(
                                               fbe_provision_drive_t* in_provision_drive_p,
                                               fbe_packet_t*          in_packet_p
                                             )
{
    fbe_status_t                             status;             // fbe status
    fbe_provision_drive_get_verify_status_t* verify_status_p;    // pointer to requestor's buffer

    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( in_provision_drive_p );


    // get control buffer pointer from sep packet payload
    status = fbe_provision_drive_get_control_buffer( in_provision_drive_p,
                                                     in_packet_p,
                                                     sizeof (fbe_provision_drive_get_verify_status_t),
                                                     (fbe_payload_control_buffer_t) &verify_status_p
                                                   );
    if ( status != FBE_STATUS_OK )
    {
        fbe_base_object_trace(  (fbe_base_object_t *)in_provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s get buffer failed\n", __FUNCTION__);
        fbe_transport_set_status(in_packet_p, status, 0);
        fbe_transport_complete_packet(in_packet_p);
        return status;
    }

    // lock access to verify enable flag and checkpoint
    fbe_provision_drive_lock( in_provision_drive_p );

    // now, store verify status information into requestor's buffer
    fbe_provision_drive_get_verify_enable( in_provision_drive_p,
                                           &verify_status_p->verify_enable );

    fbe_provision_drive_metadata_get_sniff_verify_checkpoint(in_provision_drive_p,
                                                             &verify_status_p->verify_checkpoint );

    fbe_base_config_get_capacity((fbe_base_config_t *)in_provision_drive_p,&verify_status_p->exported_capacity );

    // unlock access to verify enable flag and checkpoint
    fbe_provision_drive_unlock( in_provision_drive_p );
    
    verify_status_p->precentage_completed = (fbe_u8_t)(((double)verify_status_p->verify_checkpoint/(double)verify_status_p->exported_capacity)*100);

    // set success status in packet and complete operation
    fbe_transport_set_status( in_packet_p, FBE_STATUS_OK, 0 );
    fbe_transport_complete_packet( in_packet_p );

    // return success
    return FBE_STATUS_OK;
}   // end fbe_provision_drive_usurper_get_verify_status()


/*!****************************************************************************
 * fbe_provision_drive_usurper_set_verify_checkpoint
 ******************************************************************************
 *
 * @brief
 *    This function requests the metadata service to set a new sniff verify checkpoint 
 *    for the specified provision drive.
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 * @param   in_packet_p           -  pointer to control packet requesting this
 *                                   operation
 *
 * @return  fbe_status_t          -  status of this operation
 *
 * @version
 *    02/12/2010 - Created. Randy Black
 *
 ******************************************************************************/

static fbe_status_t
fbe_provision_drive_usurper_set_verify_checkpoint(
                                                   fbe_provision_drive_t* in_provision_drive_p,
                                                   fbe_packet_t*          in_packet_p
                                                 )
{
    fbe_status_t                                 status;              // fbe status
    fbe_provision_drive_set_verify_checkpoint_t* verify_checkpoint_p; // pointer to requestor's buffer

    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( in_provision_drive_p );

    // get control buffer pointer from sep packet payload
    status = fbe_provision_drive_get_control_buffer( in_provision_drive_p,
                                                     in_packet_p,
                                                     sizeof (fbe_provision_drive_set_verify_checkpoint_t),
                                                     (fbe_payload_control_buffer_t) &verify_checkpoint_p
                                                   );
    if ( status != FBE_STATUS_OK )
    {
        // set failure status in packet and complete operation
        fbe_transport_set_status( in_packet_p, status, 0 );
        fbe_transport_complete_packet( in_packet_p );
        return status;
    }

    /* !@todo - need to revisit sniff verify lock mechanism while update non paged metadata
      * lock access to verify checkpoint
      * fbe_provision_drive_lock( in_provision_drive_p );
      */

    /* set the completion function which handles sniff verify checkpoint update completion */
    fbe_transport_set_completion_function(in_packet_p, fbe_provision_drive_usurper_set_verify_checkpoint_complete, in_provision_drive_p);

    /* Sending request to metadata service to update sniff verify checkpoint */  
    status = fbe_provision_drive_metadata_set_sniff_verify_checkpoint(in_provision_drive_p,
                                                                      in_packet_p,
                                                                      verify_checkpoint_p->verify_checkpoint);
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;

}   // end fbe_provision_drive_usurper_set_verify_checkpoint()


/*!****************************************************************************
 * fbe_provision_drive_usurper_set_verify_checkpoint_complete
 ******************************************************************************
 *
 * @brief
 *    This function handles sniff verify checkpoint update completion. It validates the status and complete 
 *    the operation. 
 *    
 *
 * @param   in_packet_p           -  pointer to control packet requesting this
 *                                   operation
 * @param   context               -  context, which is a pointer to the provision drive
 *                                   object
 *
 * @return  fbe_status_t          -  status of this operation
 *
 * @version
 *    07/26/2010 - Created. Amit Dhaduk
 *
 ******************************************************************************/

static fbe_status_t
fbe_provision_drive_usurper_set_verify_checkpoint_complete(
                                                fbe_packet_t * in_packet_p, 
                                                fbe_packet_completion_context_t context)    
{

    fbe_provision_drive_t *            provision_drive_p = NULL;
    fbe_payload_ex_t *                sep_payload_p = NULL;
    fbe_payload_control_operation_t *  control_operation_p = NULL;
    fbe_payload_control_status_t       control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_status_t                       status;  
    
    provision_drive_p = (fbe_provision_drive_t *)context;

     /* Get sep payload. */
    sep_payload_p = fbe_transport_get_payload_ex(in_packet_p);

    /* Get control operation. */
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Get the packet status */
    status = fbe_transport_get_status_code(in_packet_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s sniff verify checkpoint update failed, status:%d.",
                                __FUNCTION__, status);
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;

        fbe_payload_control_set_status(control_operation_p, control_status);
        fbe_transport_set_status(in_packet_p, status, 0);
    }
    
    return status;

} //  End fbe_provision_drive_usurper_set_verify_checkpoint_complete


/*!****************************************************************************
 * fbe_provision_drive_control_metadata_paged_write()
 ******************************************************************************
 * @brief
 *   This function is used to write the paged metadata bits.
 *
 * @param packet_p          - pointer to a control packet 
 * @param in_context        - context, which is a pointer to the provision drive
 *                            object
 *
 *
 * @return  fbe_status_t  
 *
 * @author
 *   02/15/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_control_metadata_paged_write(fbe_provision_drive_t*   provision_drive_p, fbe_packet_t* packet_p)
{
    fbe_provision_drive_control_paged_metadata_t *    paged_data_p = NULL;    /* INPUT */
    fbe_payload_control_operation_t *                   control_operation_p = NULL;
    fbe_status_t                                        status = FBE_STATUS_GENERIC_FAILURE; 
    fbe_provision_drive_paged_metadata_t                paged_set_bits;
    fbe_payload_metadata_operation_t *                  metadata_operation_p = NULL;
    fbe_payload_ex_t *                                 sep_payload_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  

    /* get payload buffer */
    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p, 
                                                    sizeof(fbe_provision_drive_control_paged_metadata_t),
                                                    (fbe_payload_control_buffer_t *)&paged_data_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }


    /* populate the bit paged metadata */
    paged_set_bits.valid_bit = paged_data_p->metadata_bits.valid_bit;
    paged_set_bits.need_zero_bit = paged_data_p->metadata_bits.need_zero_bit;
    paged_set_bits.user_zero_bit = paged_data_p->metadata_bits.user_zero_bit;
    paged_set_bits.consumed_user_data_bit = paged_data_p->metadata_bits.consumed_user_data_bit;
    paged_set_bits.unused_bit = paged_data_p->metadata_bits.unused_bit;

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_METADATA_TRACING,
                                    "%s writing md v:%d nz:%d uz:%d cu:%d\n", 
                                    __FUNCTION__ , paged_set_bits.valid_bit, paged_set_bits.need_zero_bit
                                    ,paged_set_bits.user_zero_bit, paged_set_bits.consumed_user_data_bit);

  /* set completion function */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_control_metadata_paged_write_completion, provision_drive_p);

    /* Allocate the metadata operation. */
    metadata_operation_p =  fbe_payload_ex_allocate_metadata_operation(sep_payload_p);

    /* Build the paged metadata write request. */
    status = fbe_payload_metadata_build_paged_write(metadata_operation_p, 
                                                    &(((fbe_base_config_t *) provision_drive_p)->metadata_element),
                                                    paged_data_p->metadata_offset,
                                                    (fbe_u8_t *)&paged_set_bits,
                                                    sizeof(fbe_provision_drive_paged_metadata_t),
                                                    paged_data_p->metadata_repeat_count);
    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);

    /* Issue the metadata operation. */
    status =  fbe_metadata_operation_entry(packet_p);
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_control_metadata_paged_write()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_control_metadata_paged_write_completion()
 ******************************************************************************
 * @brief
 *   This function handles the completion of paged metadata write request.
 *
 * @param packet_p          - pointer to a control packet 
 * @param in_context        - context, which is a pointer to the provision drive
 *                            object
 *
 *
 * @return  fbe_status_t  
 *
 * @author
 *   02/15/2010 - Created. Amit Dhaduk
 *
 ******************************************************************************/                                                 
static fbe_status_t
fbe_provision_drive_control_metadata_paged_write_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_metadata_operation_t *  metadata_operation_p = NULL;
    fbe_payload_ex_t *                 sep_payload_p = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_payload_metadata_status_t       metadata_status;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_payload_control_status_t        control_status;
    
    provision_drive_p = (fbe_provision_drive_t *) context;

    /* Get the payload and control operation. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);
    
    /* Get the status of the paged metadata operation. */
    fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);
    status = fbe_transport_get_status_code(packet_p);

    /* Release the metadata operation. */
    fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);

    /* Based on the metadata status set the appropriate control status. */
    control_operation_p =  fbe_payload_ex_get_control_operation(sep_payload_p);
    if((status != FBE_STATUS_OK) ||
       (metadata_status != FBE_PAYLOAD_METADATA_STATUS_OK))
    {
        fbe_provision_drive_utils_trace(provision_drive_p,
                                        FBE_TRACE_LEVEL_INFO,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "%s paged metadata write failed. status:%d metadata_status:%d\n",
                                        __FUNCTION__, status, metadata_status);

        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    }
    else{
        control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    }
    
    fbe_payload_control_set_status(control_operation_p, control_status);
    fbe_transport_set_status(packet_p, status, 0);
    return status;                                                                                                                                   
}
/******************************************************************************
 * end fbe_provision_drive_control_metadata_paged_write_completion()
 ******************************************************************************/
/*!****************************************************************************
 * fbe_provision_drive_attach_upstream_block_edge()
 ******************************************************************************
 * @brief
 *   This function is used to attach the upstream block edge.
 *
 * @param provision_drive_p  -  pointer to provising drive object.
 * @param packet_p           -  pointer to the packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  08/25/2010 - Created. Peter Puhov
 ******************************************************************************/  
static fbe_status_t
fbe_provision_drive_attach_upstream_block_edge(fbe_provision_drive_t * provision_drive_p,
                                               fbe_packet_t * packet)
{
    fbe_status_t status;

    /* set the completion function before calling base config to attach upstream edge. */
    fbe_transport_set_completion_function(packet,
                                          fbe_provision_drive_attach_upstream_block_edge_completion,
                                          provision_drive_p);
 
    /* call the base config upstream edge. */
    status = fbe_base_config_attach_upstream_block_edge((fbe_base_config_t *) provision_drive_p, packet);

    return status;
}
/******************************************************************************
 * end fbe_provision_drive_attach_upstream_block_edge()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_attach_upstream_block_edge_completion()
 ******************************************************************************
 * @brief
 *   This function handles the completion of attaching upstream block edge.
 *
 * @param packet_p          - pointer to a control packet 
 * @param in_context        - context, which is a pointer to the provision drive
 *                            object
 *
 *
 * @return  fbe_status_t  
 *
 * @author
 *  08/25/2010 - Created. Peter Puhov
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_attach_upstream_block_edge_completion(fbe_packet_t * packet,
                                                          fbe_packet_completion_context_t context)                                          
{           
    fbe_status_t                                            status;
    fbe_provision_drive_t                                  *provision_drive_p = NULL;
    fbe_block_transport_control_attach_edge_t              *block_transport_control_attach_edge = NULL;
    fbe_payload_ex_t                                       *sep_payload = NULL;
    fbe_payload_control_operation_t                        *control_operation = NULL;
    fbe_payload_control_status_t                            control_status;
    fbe_provision_drive_configured_physical_block_size_t    configured_physical_block_size;
    fbe_object_id_t								    object_id;
    fbe_bool_t									    is_system_drive;
    fbe_system_encryption_mode_t system_encryption_mode;
    fbe_base_config_encryption_mode_t encryption_mode;

    fbe_database_get_system_encryption_mode(&system_encryption_mode);

    /* cast the base object to a virtual drive object */
    provision_drive_p = (fbe_provision_drive_t *)context;

    sep_payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);

    /* verify the status of the attach edge operation. */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_control_get_status(control_operation, &control_status);
    if((status != FBE_STATUS_OK) ||
       (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *) provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s:attach upstream edge failed, status:0x%x, control_status:0x%x\n",
                              __FUNCTION__, status, control_status);
        return status;
    }

    fbe_payload_control_get_buffer(control_operation, &block_transport_control_attach_edge);

    /* update the block edge geometry. */
    fbe_provision_drive_get_configured_physical_block_size(provision_drive_p,
                                                           &configured_physical_block_size);

    if (configured_physical_block_size == FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_520)
    {
        fbe_block_transport_edge_set_geometry(block_transport_control_attach_edge->block_edge,
                                              FBE_BLOCK_EDGE_GEOMETRY_520_520);
    }
    else if (configured_physical_block_size == FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_4160)
    {
        fbe_block_transport_edge_set_geometry(block_transport_control_attach_edge->block_edge,
                                              FBE_BLOCK_EDGE_GEOMETRY_4160_4160);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *) provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: configured physical block size: %d not supported\n",
                              __FUNCTION__, configured_physical_block_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
    is_system_drive = fbe_database_is_object_system_pvd(object_id);
    fbe_base_config_get_encryption_mode((fbe_base_config_t*)provision_drive_p, &encryption_mode);

    /* Do not allow the path state to be usable until the keys are provided.
     */
    if ((system_encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED) &&
        is_system_drive && 
        ((encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED) ||
          (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS) ||
          (encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS) ) ) {
        /* Cannot use FBE_PATH_STATE_INVALID since the upper levels will think there is no edge, which prevents the keys
         * from getting pushed. 
         */
        fbe_block_transport_server_set_path_attr(&((fbe_base_config_t *)provision_drive_p)->block_transport_server,
                                                 block_transport_control_attach_edge->block_edge->base_edge.server_index,
                                                 FBE_BLOCK_PATH_ATTR_FLAGS_KEYS_REQUIRED);
        block_transport_control_attach_edge->block_edge->base_edge.path_state = FBE_PATH_STATE_BROKEN;

        fbe_provision_drive_utils_trace(provision_drive_p, FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_ENCRYPTION,
                                        "provision_drive: attach upstream edge server: 0x%x keys required, edge state broken\n", 
                                        block_transport_control_attach_edge->block_edge->base_edge.server_index);
    }
    /* set the condition to verify the end of life state of the object, it will verify
     * the end of life state and update the upstream edge with end of life attr.
     */
    fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                           (fbe_base_object_t *)provision_drive_p,
                           FBE_PROVISION_DRIVE_LIFECYCLE_COND_VERIFY_END_OF_LIFE_STATE);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_attach_upstream_block_edge_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_detach_upstream_block_edge()
 ******************************************************************************
 * @brief
 *   This function is used to detach the upstream block edge.
 *
 * @param provision_drive_p  -  pointer to provising drive object.
 * @param packet_p           -  pointer to the packet.
 *
 * @return  fbe_status_t  
 *
 * @author
 *  08/25/2010 - Created. Dhaval Patel
 ******************************************************************************/  
static fbe_status_t
fbe_provision_drive_detach_upstream_block_edge(fbe_provision_drive_t * provision_drive_p,
                                               fbe_packet_t * packet)
{
    fbe_status_t status;

    /* set the completion function before calling base config to detach upstream edge. */
    fbe_transport_set_completion_function(packet,
                                          fbe_provision_drive_detach_upstream_block_edge_completion,
                                          provision_drive_p);
 
    /* involve the base config detach upstream block edge. */
    status = fbe_base_config_detach_upstream_block_edge((fbe_base_config_t *) provision_drive_p, packet);

    return status;
}
/******************************************************************************
 * end fbe_provision_drive_detach_upstream_block_edge()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_attach_upstream_block_edge_completion()
 ******************************************************************************
 * @brief
 *   This function handles the completion of detaching upstream block edge.
 *
 * @param packet_p          - pointer to a control packet 
 * @param in_context        - context, which is a pointer to the provision drive
 *                            object
 *
 *
 * @return  fbe_status_t  
 *
 * @author
 *  08/25/2010 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_detach_upstream_block_edge_completion(fbe_packet_t * packet,
                                                          fbe_packet_completion_context_t context)
{           
    fbe_provision_drive_t *                                 provision_drive_p = NULL;
    fbe_payload_ex_t *                                     	sep_payload = NULL;
    fbe_payload_control_operation_t *                       control_operation = NULL;
    fbe_payload_control_status_t                            control_status;
    fbe_status_t                                            status;
    fbe_notification_info_t 								notify_info;

    provision_drive_p = (fbe_provision_drive_t *)context;   

    sep_payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(sep_payload);

    /* verify the status of the attach edge operation. */
    status = fbe_transport_get_status_code(packet);
    fbe_payload_control_get_status(control_operation, &control_status);
    if((status != FBE_STATUS_OK) ||
       (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *) provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s:attach upstream edge failed, status:0x%x, control_status:0x%x\n",
                              __FUNCTION__, status, control_status);
        return status;
    }

    /*If successfully detaching the edge, we should send notification to upper layers to clear this drive's rebuilding state.
     * Added: Zhipeng Hu for AR560834*/
    notify_info.notification_type = FBE_NOTIFICATION_TYPE_DATA_RECONSTRUCTION;
    notify_info.class_id = FBE_CLASS_ID_PROVISION_DRIVE;
    notify_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PROVISIONED_DRIVE;
    notify_info.notification_data.data_reconstruction.state = DATA_RECONSTRUCTION_END;
    notify_info.notification_data.data_reconstruction.percent_complete = 100;

    fbe_notification_send(((fbe_base_object_t*)provision_drive_p)->object_id, notify_info);

    /* Rebuild ends.  Set percent rebuilt to 0 (by default, not rebuilding). */
    provision_drive_p->percent_rebuilt = 0;

    /* set the condition to verify the end of life state of the object, it will verify
     * the end of life state and set the health broken condition.
     */
    fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                           (fbe_base_object_t *)provision_drive_p,
                           FBE_PROVISION_DRIVE_LIFECYCLE_COND_VERIFY_END_OF_LIFE_STATE);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_detach_upstream_block_edge_completion()
 ******************************************************************************/

static fbe_status_t fbe_provision_drive_set_background_priorities(fbe_provision_drive_t* provision_drive_p, fbe_packet_t *packet_p)
{
    fbe_payload_ex_t *                                 payload_p = NULL;
    fbe_payload_control_operation_t *                   control_operation_p = NULL;
    fbe_provision_drive_set_priorities_t *              set_priorities_p = NULL;
    fbe_status_t                                        status = FBE_STATUS_OK;

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_provision_drive_set_priorities_t),
                                                    (fbe_payload_control_buffer_t *)&set_priorities_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(	(fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    provision_drive_p->verify_priority = set_priorities_p->verify_priority;
    provision_drive_p->zero_priority = set_priorities_p->zero_priority;
    provision_drive_p->verify_invalidate_priority = set_priorities_p->verify_invalidate_priority;


    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_provision_drive_get_background_priorities(fbe_provision_drive_t* provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                                 payload_p = NULL;
    fbe_payload_control_operation_t *                   control_operation_p = NULL;
    fbe_provision_drive_set_priorities_t *              get_priorities_p = NULL;
    fbe_status_t                                        status = FBE_STATUS_OK;

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_provision_drive_set_priorities_t),
                                                    (fbe_payload_control_buffer_t *)&get_priorities_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(	(fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    get_priorities_p->verify_priority = provision_drive_p->verify_priority;
    get_priorities_p->zero_priority = provision_drive_p->zero_priority;
    get_priorities_p->verify_invalidate_priority = provision_drive_p->verify_invalidate_priority;

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_provision_drive_control_get_metadata_memory(fbe_provision_drive_t* provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                                         payload_p = NULL;
    fbe_payload_control_operation_t *                           control_operation_p = NULL;
    fbe_provision_drive_control_get_metadata_memory_t *         get_metadata_memory = NULL;
    fbe_status_t                                                status;

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_provision_drive_control_get_metadata_memory_t),
                                                    (fbe_payload_control_buffer_t *)&get_metadata_memory);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(	(fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }


    /* TODO provide relevant getter functions */
    get_metadata_memory->lifecycle_state = provision_drive_p->provision_drive_metadata_memory.base_config_metadata_memory.lifecycle_state;
    get_metadata_memory->power_save_state = provision_drive_p->provision_drive_metadata_memory.base_config_metadata_memory.power_save_state;

    /* Peer */
    get_metadata_memory->lifecycle_state_peer = provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.lifecycle_state;
    get_metadata_memory->power_save_state_peer = provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.power_save_state;

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;

}

/*!***************************************************************
 * @fn fbe_provision_drive_control_get_versioned_metadata_memory
 ****************************************************************
 * @brief
 *  This function is for internal (debugging and test) use ONLY. it is used to get
 *  the PVD metadata memory with version header (its and peer's)
 *
 * @param provision_drive_p - provision drive
 * @param packet_p - packet
 *
 * @return
 *  fbe_status_t - FBE_STATUS_OK - if no error.
 *
 * @version
 *  04/24/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
static fbe_status_t 
fbe_provision_drive_control_get_versioned_metadata_memory(fbe_provision_drive_t* provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                                         payload_p = NULL;
    fbe_payload_control_operation_t *                           control_operation_p = NULL;
    fbe_provision_drive_control_get_versioned_metadata_memory_t *         get_metadata_memory = NULL;
    fbe_status_t                                                status;

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_provision_drive_control_get_versioned_metadata_memory_t),
                                                    (fbe_payload_control_buffer_t *)&get_metadata_memory);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(	(fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /*copy the version header*/
    fbe_copy_memory(&get_metadata_memory->ver_header, 
                    &provision_drive_p->provision_drive_metadata_memory.base_config_metadata_memory.version_header,
                    sizeof (fbe_sep_version_header_t));
    fbe_copy_memory(&get_metadata_memory->peer_ver_header, 
                    &provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.version_header,
                    sizeof (fbe_sep_version_header_t));

    get_metadata_memory->lifecycle_state = provision_drive_p->provision_drive_metadata_memory.base_config_metadata_memory.lifecycle_state;
    get_metadata_memory->power_save_state = provision_drive_p->provision_drive_metadata_memory.base_config_metadata_memory.power_save_state;

    /* Peer state*/
    get_metadata_memory->lifecycle_state_peer = provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.lifecycle_state;
    get_metadata_memory->power_save_state_peer = provision_drive_p->provision_drive_metadata_memory_peer.base_config_metadata_memory.power_save_state;

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;

}

/*!***************************************************************************
 *          fbe_provision_drive_usurper_set_debug_flags()
 *****************************************************************************
 *
 * @brief   This method sets the raid group debug
 *          flags to the value passed for the associated raid group.
 *
 * @param   provision_drive - The raid group.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  09/07/2010 - Created. Amit Dhaduk   
 *
 *
 ****************************************************************/
static fbe_status_t
fbe_provision_drive_usurper_set_debug_flags(fbe_provision_drive_t *provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_provision_drive_set_debug_trace_flag_t *   set_debug_flag_p = NULL;
    fbe_status_t                            status;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  

    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_provision_drive_set_debug_trace_flag_t),
                                                    (fbe_payload_control_buffer_t *)&set_debug_flag_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(	(fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s get buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* set the PVD debug trace flag at object level
     * lock PVD access to set debug flags
     */
    fbe_provision_drive_lock( provision_drive_p );
    fbe_provision_drive_set_debug_flag(provision_drive_p, set_debug_flag_p->trace_flag);
    fbe_provision_drive_unlock( provision_drive_p );

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/**************************************************
 * end fbe_provision_drive_usurper_set_debug_flags()
 **************************************************/

/*!****************************************************************************
 * fbe_provision_drive_usurper_set_eol_state()
 ******************************************************************************
 * @brief
 *  This function sets the end of life state for a provision drive object.
 *
 * @param provision_drive_p   - The provision drive.
 * @param in_packet_p        - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  02/28/2011 - Vishnu Sharma
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_usurper_set_eol_state(
                                        fbe_provision_drive_t*   provision_drive_p,
                                        fbe_packet_t*            packet_p)
{

    /* set a condition to set EOL state */
    fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                           (fbe_base_object_t*)provision_drive_p,
                           FBE_PROVISION_DRIVE_LIFECYCLE_COND_SET_END_OF_LIFE);
    
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
    
}   
/******************************************************************************
 * end fbe_provision_drive_usurper_set_eol_state()
*******************************************************************************/

/*!****************************************************************************
 *          fbe_provision_drive_usurper_clear_eol_state()
 ******************************************************************************
 *
 * @brief   This function clears the end of life state for a provision drive 
 *          object.
 *
 * @param   provision_drive_p   - The provision drive.
 * @param   packet_p        - The packet requesting this operation.
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  03/27/2012  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_usurper_clear_eol_state(fbe_provision_drive_t *provision_drive_p,
                                                                fbe_packet_t *packet_p)
{
    fbe_bool_t                             end_of_life_state = FBE_FALSE;

    /* Get end of life state */
    fbe_provision_drive_metadata_get_end_of_life_state(provision_drive_p, &end_of_life_state);
    if (end_of_life_state == FBE_TRUE)
    {
        /* Set the flag to inform peer */
        fbe_provision_drive_set_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_CLEAR_END_OF_LIFE);

        /* Set the flag to indicate that we are clearing EOL */
        fbe_provision_drive_set_local_state(provision_drive_p, FBE_PVD_LOCAL_STATE_CLEAR_EOL_REQUEST);

        /* set a condition to clear EOL state */
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t*)provision_drive_p,
                               FBE_PROVISION_DRIVE_LIFECYCLE_COND_CLEAR_END_OF_LIFE);

        /* Jump to Activate state */            
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *)provision_drive_p, FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);
    }

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
    
}   
/******************************************************************************
 * end fbe_provision_drive_usurper_clear_eol_state()
*******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_download_ack()
 ******************************************************************************
 * @brief
 *  This function handles the raid group ack for the firmware download request.
 *
 * @param provision_drive_p - The provision drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/02/2011 - Created. Ruomu Gao
 *
 ******************************************************************************/
fbe_u32_t
fbe_provision_drive_download_ack(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t               *payload_p;
    fbe_payload_control_operation_t *control_operation_p;
    fbe_block_edge_t                *block_edge_p = (fbe_block_edge_t *)fbe_transport_get_edge(packet_p);
    fbe_path_attr_t                 path_attr;
    fbe_block_transport_server_t    *block_transport_server;
    fbe_u32_t                       i = 0;
    fbe_base_edge_t                 *base_edge = NULL;
    fbe_bool_t                      b_is_system_drive = FBE_FALSE;
    fbe_object_id_t                 object_id;
    fbe_status_t                    status = FBE_STATUS_OK;

    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s PVD download ack from RAID.\n",
                          __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);


    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
    b_is_system_drive = fbe_database_is_object_system_pvd(object_id);
    if(b_is_system_drive)
    {
        /* Set GRANT bit on the edge.
         */
        if (block_edge_p != NULL)
        {
            fbe_block_transport_set_path_attributes(block_edge_p, FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_GRANT);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s no edge in packet %p\n", __FUNCTION__, packet_p);
        }
    
        block_transport_server = &((fbe_base_config_t *)provision_drive_p)->block_transport_server;
    
        fbe_block_transport_server_lock(block_transport_server);
        base_edge = block_transport_server->base_transport_server.client_list;
        while(base_edge != NULL) {
            if(base_edge->client_id != FBE_OBJECT_ID_INVALID){
                if ((base_edge->path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_GRANT) == 0)
                {
                    status = FBE_STATUS_PENDING;
                    break;
                }
            }
            i++;
            if(i > 0x0000FFFF){
                fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        "%s Critical error looped client list.\n",
                                        __FUNCTION__);
    
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
            }
            base_edge = base_edge->next_edge;
        }
        fbe_block_transport_server_unlock(block_transport_server);

    }
    
    if(status != FBE_STATUS_OK)
    {
        return FBE_STATUS_OK;
    }


    /* Check whether the request is already aborted */
    fbe_block_transport_get_path_attributes(block_edge_p, &path_attr);
    if ((path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK) == 0)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s REQ already aborted.\n",
                              __FUNCTION__);
        return FBE_STATUS_OK;
    }

    /* Permission granted by RAID. */
    fbe_block_transport_server_propagate_path_attr_with_mask_all_servers(
                &((fbe_base_config_t *)provision_drive_p)->block_transport_server, 
                FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_GRANT,
                FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_MASK);

    /* Set condition to start download. */
    fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                           (fbe_base_object_t *) provision_drive_p,
                           FBE_PROVISION_DRIVE_LIFECYCLE_COND_DOWNLOAD);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_download_ack()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_get_download_info()
 ******************************************************************************
 * @brief
 *  This function handles the request to get drive download information.
 *
 * @param provision_drive_p - The provision drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/31/2011 - Created. chenl6
 *
 ******************************************************************************/
fbe_u32_t
fbe_provision_drive_get_download_info(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                                         payload_p = NULL;
    fbe_payload_control_operation_t *                           control_operation_p = NULL;
    fbe_provision_drive_get_download_info_t *                   get_download_info_p = NULL;
    fbe_provision_drive_local_state_t                           local_state;
    fbe_provision_drive_clustered_flags_t                       clustered_flags;
    fbe_provision_drive_clustered_flags_t                       peer_clustered_flags;
    fbe_status_t                                                status;

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_provision_drive_get_download_info_t),
                                                    (fbe_payload_control_buffer_t *)&get_download_info_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(	(fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s get buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Get download information */
    get_download_info_p->download_originator = fbe_provision_drive_is_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_DOWNLOAD_ORIGINATOR);
    fbe_provision_drive_get_local_state(provision_drive_p, &local_state);
    fbe_provision_drive_get_clustered_flags(provision_drive_p, &clustered_flags);
    fbe_provision_drive_get_peer_clustered_flags(provision_drive_p, &peer_clustered_flags);
    get_download_info_p->local_state = local_state & FBE_PVD_LOCAL_STATE_DOWNLOAD_MASK;
    get_download_info_p->clustered_flag = clustered_flags & FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_MASK;
    get_download_info_p->peer_clustered_flag = peer_clustered_flags & FBE_PROVISION_DRIVE_CLUSTERED_FLAG_DOWNLOAD_MASK;

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_get_download_info()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_get_health_check_info()
 ******************************************************************************
 * @brief
 *  This function get the PVD related health check info.
 *
 * @param provision_drive_p - The provision drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/26/2013 - Created.  Wayne Garrett
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_get_health_check_info(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                                          payload_p = NULL;
    fbe_payload_control_operation_t *                           control_operation_p = NULL;
    fbe_provision_drive_get_health_check_info_t *               get_health_check_info_p = NULL;
    fbe_provision_drive_local_state_t                           local_state;
    fbe_provision_drive_clustered_flags_t                       clustered_flags;
    fbe_provision_drive_clustered_flags_t                       peer_clustered_flags;
    fbe_status_t                                                status;

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_provision_drive_get_health_check_info_t),
                                                    (fbe_payload_control_buffer_t *)&get_health_check_info_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(	(fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s get buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Get health_check information */
    get_health_check_info_p->originator = fbe_provision_drive_is_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_HC_ORIGINATOR);
    fbe_provision_drive_get_local_state(provision_drive_p, &local_state);
    fbe_provision_drive_get_clustered_flags(provision_drive_p, &clustered_flags);
    fbe_provision_drive_get_peer_clustered_flags(provision_drive_p, &peer_clustered_flags);
    get_health_check_info_p->local_state = local_state & (FBE_PVD_LOCAL_STATE_HEALTH_CHECK_MASK|FBE_PVD_LOCAL_STATE_HEALTH_CHECK_ABORT_REQ);
    get_health_check_info_p->clustered_flag = clustered_flags & FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_MASK;
    get_health_check_info_p->peer_clustered_flag = peer_clustered_flags & FBE_PROVISION_DRIVE_CLUSTERED_FLAG_HEALTH_CHECK_MASK;

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}

static fbe_status_t
fbe_provision_drive_control_metadata_paged_operation(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;
    fbe_payload_metadata_operation_t * metadata_operation_p = NULL;
    fbe_payload_control_operation_opcode_t opcode;
    fbe_payload_control_buffer_t * buffer_p = NULL;
    fbe_lba_t           metadata_start_lba;
    fbe_block_count_t   metadata_block_count;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    fbe_base_object_trace( (fbe_base_object_t *)provision_drive,
                           FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s entry\n", __FUNCTION__);

    fbe_payload_control_get_buffer(control_operation_p, &buffer_p);
    if (buffer_p == NULL) {
        fbe_base_object_trace( (fbe_base_object_t *)provision_drive,
                               FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate a metadata operation
     */
    metadata_operation_p =  fbe_payload_ex_allocate_metadata_operation(sep_payload_p);
    if (metadata_operation_p == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s unable to allocate metadata operation\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_opcode(control_operation_p, &opcode);

    if (opcode == FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_SET_BITS) {
        fbe_base_config_control_metadata_paged_change_bits_t * change_bits = NULL;
        fbe_payload_control_get_buffer(control_operation_p, &change_bits);
 
        /* Always trace a set bits request.
         */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "provision_drive: usurper paged metadata set bits packet: 0x%x offset: %lld data[0]: 0x%x size: %d \n",
                              (fbe_u32_t)packet_p, change_bits->metadata_offset, 
                              *((fbe_u32_t *)&change_bits->metadata_record_data[0]),
                              change_bits->metadata_record_data_size);

        status = fbe_payload_metadata_build_paged_set_bits(metadata_operation_p, 
                                                           &provision_drive->base_config.metadata_element, 
                                                           change_bits->metadata_offset,
                                                           change_bits->metadata_record_data,
                                                           change_bits->metadata_record_data_size,
                                                           change_bits->metadata_repeat_count);
    }
    else if (opcode == FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_CLEAR_BITS)
    {
        fbe_base_config_control_metadata_paged_change_bits_t * change_bits = NULL;
        fbe_payload_control_get_buffer(control_operation_p, &change_bits);

        status = fbe_payload_metadata_build_paged_clear_bits(metadata_operation_p, 
                                                             &provision_drive->base_config.metadata_element, 
                                                             change_bits->metadata_offset,
                                                             change_bits->metadata_record_data,
                                                             change_bits->metadata_record_data_size,
                                                             change_bits->metadata_repeat_count);
    }
    else if (opcode == FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_GET_BITS)
    {
        fbe_base_config_control_metadata_paged_get_bits_t * get_bits = NULL;
        fbe_payload_control_get_buffer(control_operation_p, &get_bits);
        status = fbe_payload_metadata_build_paged_get_bits(metadata_operation_p, 
                                                           &provision_drive->base_config.metadata_element, 
                                                           get_bits->metadata_offset,
                                                           get_bits->metadata_record_data,
                                                           get_bits->metadata_record_data_size,
                                                           sizeof(fbe_provision_drive_paged_metadata_t));

        /* If FUA is requested, set the proper flag in the metadata operation.
         */
        if ((get_bits->get_bits_flags & FBE_BASE_CONFIG_CONTROL_METADATA_PAGED_GET_BITS_FLAGS_FUA_READ) != 0)
        {
            metadata_operation_p->u.metadata.operation_flags |= FBE_PAYLOAD_METADATA_OPERATION_FLAGS_FUA_READ;
        }
    }
    else if (opcode == FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_WRITE)
    {
        fbe_base_config_control_metadata_paged_change_bits_t * write_bits = NULL;
        fbe_payload_control_get_buffer(control_operation_p, &write_bits);
        status = fbe_payload_metadata_build_paged_write(metadata_operation_p, 
                                                        &provision_drive->base_config.metadata_element, 
                                                        write_bits->metadata_offset,
                                                        write_bits->metadata_record_data,
                                                        write_bits->metadata_record_data_size,
                                                        write_bits->metadata_repeat_count);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive,
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
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s get lock range failed with status: 0x%x", 
                              __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We calculate the lock range using a logical address. */
    fbe_payload_metadata_set_metadata_stripe_offset(metadata_operation_p, metadata_start_lba);
    fbe_payload_metadata_set_metadata_stripe_count(metadata_operation_p, metadata_block_count);

    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_control_metadata_paged_operation_completion, provision_drive);

    fbe_payload_ex_increment_metadata_operation_level(sep_payload_p);

    status = fbe_metadata_operation_entry(packet_p);

    return status;
}


static fbe_status_t 
fbe_provision_drive_control_metadata_paged_operation_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * sep_payload_p = NULL;
    fbe_provision_drive_t *provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_payload_metadata_operation_t * metadata_operation_p = NULL;
    fbe_base_config_control_metadata_paged_get_bits_t * get_bits = NULL;    /* INPUT */
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_u8_t *  metadata_record_data = NULL;
    fbe_status_t    packet_status = FBE_STATUS_OK;
    fbe_u32_t       packet_qualifier = 0;
    fbe_payload_metadata_status_t metadata_status;

    packet_status = fbe_transport_get_status_code(packet_p);
    if (packet_status == FBE_STATUS_OK) {
        sep_payload_p = fbe_transport_get_payload_ex(packet_p);
        metadata_operation_p =  fbe_payload_ex_get_metadata_operation(sep_payload_p);
        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
        fbe_payload_metadata_get_status(metadata_operation_p, &metadata_status);

        if(metadata_status == FBE_PAYLOAD_METADATA_STATUS_OK) {
            control_operation = fbe_payload_ex_get_control_operation(sep_payload_p);  
            fbe_payload_control_get_buffer(control_operation, &get_bits);
            metadata_record_data = get_bits->metadata_record_data;
            fbe_copy_memory(metadata_record_data,
                            metadata_operation_p->u.metadata.record_data, 
                            metadata_operation_p->u.metadata.record_data_size);
        }
        else {
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                                  FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s error metadata status: 0x%x\n", __FUNCTION__, metadata_status);
            packet_status = FBE_STATUS_GENERIC_FAILURE;
            packet_qualifier = 0;
        }
    } else {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s error packet status: 0x%x\n", __FUNCTION__, packet_status);
        fbe_payload_ex_release_metadata_operation(sep_payload_p, metadata_operation_p);
    }

    /* Set the status of the metdata request.
     */
    fbe_transport_set_status(packet_p, packet_status, packet_qualifier);

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_provision_drive_get_opaque_data(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_provision_drive_get_opaque_data_t * get_opaque_data_p = NULL;
    fbe_status_t							status;
    fbe_object_id_t                         my_object_id;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  

    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_provision_drive_get_opaque_data_t),
                                                    (fbe_payload_control_buffer_t *)&get_opaque_data_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(	(fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s get buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }
    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &my_object_id);

    status = fbe_database_get_pvd_opaque_data(my_object_id, get_opaque_data_p->opaque_data, sizeof(get_opaque_data_p->opaque_data));
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed to get the opaque_data of pvd 0x%x from database.\n",
                                __FUNCTION__, my_object_id);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else{
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}


/*!****************************************************************************
 * fbe_provision_drive_get_pool_id()
 ******************************************************************************
 * @brief
 *  This function gets the the pool-id information from the database service
 *  user table for this PVD. 
 *
 * @param provision_drive_p - The provision drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/13/2011 - Created. Arun S
 * 
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_get_pool_id(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t                      *sep_payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;
    fbe_provision_drive_control_pool_id_t  *get_pool_id_p = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         my_object_id = FBE_OBJECT_ID_INVALID;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  

    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_provision_drive_control_pool_id_t),
                                                    (fbe_payload_control_buffer_t *)&get_pool_id_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(	(fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s get buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &my_object_id);

    status = fbe_database_get_pvd_pool_id(my_object_id, &get_pool_id_p->pool_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s: Failed to get the pool_id of pvd 0x%x from database.\n", 
                              __FUNCTION__, my_object_id);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }

    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}

/*!****************************************************************************
 * fbe_provision_drive_set_pool_id()
 ******************************************************************************
 * @brief
 *  This function sets the the pool-id information in the database service
 *  user table for this PVD. 
 *
 * @param provision_drive_p - The provision drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/13/2011 - Created. Arun S
 * 
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_set_pool_id(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t                      *sep_payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;
    fbe_provision_drive_control_pool_id_t  *set_pool_id_p = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_object_id_t                         my_object_id = FBE_OBJECT_ID_INVALID;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  

    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_provision_drive_control_pool_id_t),
                                                    (fbe_payload_control_buffer_t *)&set_pool_id_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(	(fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s get buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &my_object_id);

    status = fbe_database_set_pvd_pool_id(my_object_id, set_pool_id_p->pool_id);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 
                              "%s: Failed to set the pool_id of pvd 0x%x in database.\n", 
                              __FUNCTION__, my_object_id);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }

    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);

    return status;
}

/*!****************************************************************************
 * fbe_provision_drive_disable_verify
 ******************************************************************************
 *
 * @brief
 *    This function disables sniff verifies on the specified provision drive
 *    by:
 *
 *    1. Clearing the provision drive's sniff verify enable flag
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 *
 * @return  fbe_status_t          -  status of this operation
 *
 * @version
 *    06/17/2011 - Created
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_disable_verify(fbe_provision_drive_t* in_provision_drive_p)
{
    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( in_provision_drive_p );

    // clear sniff verify enable flag
    fbe_provision_drive_clear_verify_enable( in_provision_drive_p );

    // return success
    return FBE_STATUS_OK;

}   // end fbe_provision_drive_disable_verify()


/*!****************************************************************************
 * fbe_provision_drive_enable_verify
 ******************************************************************************
 *
 * @brief
 *    This function enables sniff verifies on the specified provision drive
 *    by:
 *
 *    1. Setting the provision drive's sniff verify enable flag
 *
 * @param   in_provision_drive_p  -  pointer to provision drive
 *
 * @return  fbe_status_t          -  status of this operation
 *
 * @version
 *    06/17/2011 - Created
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_enable_verify(fbe_provision_drive_t* in_provision_drive_p)
{
    // trace function entry
    FBE_PROVISION_DRIVE_TRACE_FUNC_ENTRY( in_provision_drive_p );

    // set sniff verify enable flag
    fbe_provision_drive_set_verify_enable( in_provision_drive_p );

    // return success
    return FBE_STATUS_OK;
}   // end fbe_provision_drive_enable_verify()


/*!****************************************************************************
 * fbe_provision_drive_clear_logical_errors()
 ******************************************************************************
 * @brief
 *  Clear out any logical errors we have on the upstream edge.
 * 
 * @param provision_drive_p - The virtual drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/28/2011 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_clear_logical_errors(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t                    status;
    fbe_payload_ex_t               *payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_block_edge_t                *edge_p = (fbe_block_edge_t *)fbe_transport_get_edge(packet_p);
    fbe_edge_index_t                edge_index;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Only clear our attribute on the edge this was sent down.
     */
    if (edge_p != NULL)
    {
        fbe_object_id_t client_id;
        fbe_block_transport_get_client_id(edge_p, &client_id);
        fbe_block_transport_get_server_index(edge_p, &edge_index);
        status = fbe_block_transport_server_clear_path_attr_all_servers(&provision_drive_p->base_config.block_transport_server,
                                                                        FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS);
        if (status == FBE_STATUS_OK) 
        { 
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "clear FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS from client 0x%x %d\n", 
                                  client_id, edge_index);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "error %d clearing FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS from client 0x%x %d\n", 
                                  status, client_id, edge_index);
        }
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s no edge in packet %p\n", __FUNCTION__, packet_p);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }

    status = fbe_block_transport_send_control_packet(((fbe_base_config_t *)provision_drive_p)->block_edge_p, packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_clear_logical_errors()
 ******************************************************************************/
/*!****************************************************************************
* fbe_provision_drive_get_removed_timestamp()
******************************************************************************
* @brief
*	This function is used to get the removed tiem stamp.it is only used for test
* 
* @param  provision_drive		- provision drive object.
* @param  packet				- pointer to the packet.
*
* @return status				- status of the operation.
*
* @author
*	03/28/2012 - Created. zhangy
*
******************************************************************************/
static fbe_status_t
fbe_provision_drive_get_removed_timestamp(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t *							payload = NULL;
    fbe_payload_control_operation_t *			control_operation = NULL;
    fbe_provision_drive_get_removed_timestamp_t * get_removed_timestamp = NULL;
    fbe_status_t								status;
    
    fbe_provision_drive_utils_trace(provision_drive,
                                        FBE_TRACE_LEVEL_DEBUG_HIGH,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "%s entry\n", __FUNCTION__);
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);	
    status = fbe_provision_drive_get_control_buffer(provision_drive, packet,
                                                    sizeof(fbe_provision_drive_get_removed_timestamp_t),
                                                    (fbe_payload_control_buffer_t *)&get_removed_timestamp);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(	(fbe_base_object_t *)provision_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    status = fbe_provision_drive_metadata_get_time_stamp(provision_drive, &get_removed_timestamp->removed_tiem_stamp);
    if (status != FBE_STATUS_OK) 
    {
            fbe_base_object_trace((fbe_base_object_t *)provision_drive,
                                    FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "pvd_get_zero_chkpt: Failed to get removed time stamp. Status: 0x%X\n", status);
    
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
* end fbe_provision_drive_get_removed_timestamp()
*******************************************************************************/
    
/*!****************************************************************************
* fbe_provision_drive_set_removed_timestamp()
******************************************************************************
* @brief
*	This function is used to set the removed timestamp. it is only for test use
* 
* @param  provision_drive		- provision drive object.
* @param  packet				- pointer to the packet.
*
* @return status				- status of the operation.
*
* @author
*	03/28/2012 - Created. zhangy
*
******************************************************************************/
static fbe_status_t
fbe_provision_drive_set_removed_timestamp(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet)
{   
    fbe_status_t                                status;
    fbe_payload_ex_t *                         payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_provision_drive_control_set_removed_timestamp_t * set_removed_timestamp = NULL;

    fbe_provision_drive_utils_trace(provision_drive,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    status = fbe_provision_drive_get_control_buffer(provision_drive, packet,
                                                    sizeof(fbe_provision_drive_control_set_removed_timestamp_t),
                                                    (fbe_payload_control_buffer_t *)&set_removed_timestamp);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(	(fbe_base_object_t *)provision_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s get buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    status = fbe_provision_drive_metadata_ex_set_time_stamp(provision_drive, packet, set_removed_timestamp->removed_tiem_stamp,set_removed_timestamp->is_persist_b);   

    return status;
}
/******************************************************************************
 * end fbe_provision_drive_set_removed_timestamp()
******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_get_checkpoint()
 ******************************************************************************
 * @brief
 *  This usurper command will just return FBE INVALID  as checkpoint. This is more
 *  of a stub than anything else. For first 4 drives there is no VD for system RG
 *  When system RG asks for checkpoint to downstream, it will come to PVD we will
 *  just return invalid, meaning the same drive came back. If it is a new drive in the
 *  system RG, system raid object will be notified to mark NR on the entire drive
 * 
 * @param provision_drive_p - The provision drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  4/19/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_get_checkpoint(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;
    fbe_raid_group_get_checkpoint_info_t   *rg_server_obj_info_p = NULL; 

    fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_raid_group_get_checkpoint_info_t),
                                                    (fbe_payload_control_buffer_t *)&rg_server_obj_info_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(	(fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s get buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    rg_server_obj_info_p->downstream_checkpoint = FBE_LBA_INVALID;

    /* If enabled trace some information
     */
    fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "provision_drive: get chkpt: 0x%llx\n",
                          (unsigned long long)rg_server_obj_info_p->downstream_checkpoint);
                            
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}// End fbe_provision_drive_get_checkpoint

/*!****************************************************************************
 * fbe_provision_drive_get_checkpoint()
 ******************************************************************************
 * @brief
 *  This usurpur command will just return FBE INVALID  as checkpoint. This is more
 *  of a stub than anything else. For first 4 drives there is no VD for system RG
 *  When system RG asks for checkpoint to downstream, it will come to PVD we will
 *  just return invalid, meaning the same drive came back. If it is a new drive in the
 *  system RG, system raid object will be notified to mark NR on the entire drive
 * 
 * @param provision_drive_p - The provision drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  4/19/2012 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_get_slf_state(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;
    fbe_provision_drive_get_slf_state_t    *get_slf_state_p = NULL; 

    fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_provision_drive_get_slf_state_t),
                                                    (fbe_payload_control_buffer_t *)&get_slf_state_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(	(fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s get buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    if (fbe_provision_drive_is_local_state_set(provision_drive_p, FBE_PVD_LOCAL_STATE_SLF))
    {
        get_slf_state_p->is_drive_slf = FBE_TRUE;
    }
    else
    {
        get_slf_state_p->is_drive_slf = FBE_FALSE;
    }

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}// End fbe_provision_drive_get_checkpoint

/*!***************************************************************************
 *          fbe_provision_drive_usurper_set_default_nonpage()
 *****************************************************************************
 *
 * @author
 *  05/15/2012 - Created. zhangy26   
 *
 ****************************************************************/
static fbe_status_t
fbe_provision_drive_usurper_set_nonpaged_metadata(fbe_provision_drive_t *provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                     sep_payload_p = NULL;
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_base_config_control_set_nonpaged_metadata_t*  control_set_np_p = NULL;
    fbe_status_t  status;

    /* Get the usurpur control buffer */
    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p, 
                                                    sizeof(fbe_base_config_control_set_nonpaged_metadata_t),
                                                    (fbe_payload_control_buffer_t*)&control_set_np_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }
    
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);  

    if(control_set_np_p->set_default_nonpaged_b)
    {
        
        status = fbe_provision_drive_metadata_set_default_nonpaged_metadata(provision_drive_p,packet_p);
    
    }else{
      
      /* Send the nonpaged metadata write request to metadata service. */
      status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) provision_drive_p,
                                    packet_p,
                                    0,
                                    control_set_np_p->np_record_data, /* non paged metadata memory. */
                                    sizeof(fbe_provision_drive_nonpaged_metadata_t)); /* size of the non paged metadata. */
    }
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed to set object default NP.\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_GENERIC_FAILURE;

    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        
}
/**************************************************
 * end fbe_provision_drive_usurper_set_debug_flags()
 **************************************************/

/*!****************************************************************************
 * fbe_provision_drive_test_slf()
 ******************************************************************************
 * @brief
 *  This function is for test only. It will allocate a block payload and send
 *  over FBE CMI.
 * 
 * @param provision_drive_p - The provision drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  05/26/2012 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_test_slf(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet_p)
{   
    fbe_status_t                                status;
    fbe_payload_ex_t * payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p;
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t  * sg_list = NULL;

    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_u32_t *op;
    fbe_payload_block_operation_opcode_t block_op;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);  

    status = fbe_provision_drive_get_control_buffer(provision_drive, packet_p,
                                                    sizeof(fbe_u32_t),
                                                    (fbe_payload_control_buffer_t *)&op);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(	(fbe_base_object_t *)provision_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s get buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    if (*op == 0)
        block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ;
    else
        block_op = FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE;

    //payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_allocate_block_operation(payload_p);

    /* First build the block operation.
     * We also need to manually increment the payload level 
     * since we will not be sending this on an edge. 
     */
    status = fbe_payload_block_build_operation(block_operation_p,
                                               block_op,
                                               0x100000,
                                               1,
                                               520,
                                               1,
                                               NULL);
    fbe_payload_ex_increment_block_operation_level(payload_p);

    /* Set the sg ptr into the packet.
     */
    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL){
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = 520;
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);
    sg_list[1].count = 0;
    sg_list[1].address = NULL;
    fbe_payload_ex_set_sg_list(payload_p, sg_list, 1);
    if (block_op == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)
        fbe_copy_memory(sg_list[0].address, "test_slf", 9);

    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_test_slf_completion, provision_drive);

    status = fbe_cmi_send_packet(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}// End fbe_provision_drive_test_slf

/*!****************************************************************************
 * fbe_provision_drive_test_slf_completion()
 ******************************************************************************
 * @brief
 *  This is the completion function of test slf.
 * 
 * @param packet_p - The packet requesting this operation.
 * @param context - The context of completion function.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  05/26/2012 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_test_slf_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t * provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_payload_ex_t * payload_p = NULL;
    fbe_payload_block_operation_t *block_operation_p;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_payload_block_operation_opcode_t    opcode;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_get_opcode(block_operation_p, &opcode);
    fbe_payload_ex_get_sg_list(payload_p, &sg_list, NULL);

    if (opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ)
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s READ complete %s.\n",
                                __FUNCTION__, sg_list[0].address);

    fbe_transport_release_buffer(sg_list);
    fbe_payload_ex_release_block_operation(payload_p, block_operation_p);

    return FBE_STATUS_OK;
}// End fbe_provision_drive_test_slf_completion



/*!**************************************************************
 * fbe_provision_drive_map_lba_to_chunk()
 ****************************************************************
 * @brief
 *  This function maps a client lba on the pvd to a paged
 *  metadata chunk and a paged metadata lba.
 *
 * @param provision_drive_p - Provision drive object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  6/4/2012 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t
fbe_provision_drive_map_lba_to_chunk(fbe_provision_drive_t * provision_drive_p,
                                     fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_payload_ex_t *                                 payload_p = NULL;
    fbe_payload_control_operation_t *                   control_operation_p = NULL;
    fbe_provision_drive_map_info_t *                    provision_drive_info_p = NULL;
    fbe_lba_t metadata_lba;
    fbe_chunk_index_t chunk_index;
    fbe_provision_drive_metadata_positions_t metadata_positions;

    /* Retrive the spare info request from the packet. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Get the usurpur control buffer */
    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p, 
                                                    sizeof(fbe_provision_drive_map_info_t),
                                                    (fbe_payload_control_buffer_t *)&provision_drive_info_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }
    if ((provision_drive_info_p->lba / FBE_PROVISION_DRIVE_CHUNK_SIZE) > FBE_U32_MAX)
    {
        fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s lba 0x%llx / chunk size 0x%x > FBE_U32_MAX\n", 
                               __FUNCTION__, (unsigned long long)provision_drive_info_p->lba, FBE_PROVISION_DRIVE_CHUNK_SIZE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
    chunk_index = (fbe_chunk_index_t)(provision_drive_info_p->lba / FBE_PROVISION_DRIVE_CHUNK_SIZE);
    metadata_lba = (chunk_index * sizeof(fbe_provision_drive_paged_metadata_t)) / FBE_METADATA_BLOCK_DATA_SIZE;

    fbe_provision_drive_get_metadata_positions(provision_drive_p, &metadata_positions);
    provision_drive_info_p->metadata_pba = metadata_lba + metadata_positions.paged_metadata_lba;
    provision_drive_info_p->chunk_index = chunk_index;
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_map_lba_to_chunk()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_usurper_clear_drive_fault()
 ******************************************************************************
 * @brief
 *  This function is used to clear drive fault attribute on block edge.
 * 
 * @param provision_drive_p - The provision drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  07/13/2012 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_usurper_clear_drive_fault(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{   
    fbe_payload_ex_t *                     payload_p = NULL;
    fbe_payload_control_operation_t *      control_operation_p = NULL;
    fbe_bool_t                             drive_fault_state = FBE_FALSE;
    fbe_lifecycle_state_t my_state;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Check whether the pvd is in drive fault state */
    fbe_base_config_metadata_get_lifecycle_state((fbe_base_config_t *)provision_drive_p, &my_state);
    fbe_provision_drive_metadata_get_drive_fault_state(provision_drive_p, &drive_fault_state);
    fbe_base_object_trace( (fbe_base_object_t *)provision_drive_p,
                           FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s DRIVE FAULT %d lifecycle %d\n", 
                           __FUNCTION__, drive_fault_state, my_state);
    if (drive_fault_state == FBE_TRUE)
    {
        fbe_provision_drive_set_clustered_flag(provision_drive_p, FBE_PROVISION_DRIVE_CLUSTERED_FLAG_CLEAR_DRIVE_FAULT);
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t*)provision_drive_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_CLEAR_DRIVE_FAULT);
    }
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_usurper_clear_drive_fault()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_usurper_get_spindown_qualified()
 ******************************************************************************
 * @brief
 *  This function is used to get spindown qualified flag.
 * 
 * @param provision_drive_p - The provision drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  09/24/2012 - Created. Vera Wang
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_usurper_get_spindown_qualified(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{   
    fbe_payload_ex_t *                          payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_base_config_power_save_capable_info_t * power_save_capable_p = NULL;
    fbe_status_t                                status;
    fbe_payload_control_buffer_length_t         length = 0;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Get the usurpur control buffer */
    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p, 
                                                    sizeof(fbe_base_config_power_save_capable_info_t),
                                                    (fbe_payload_control_buffer_t*)&power_save_capable_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* If length of the buffer does not match with set get provision drive 
     * information structure lenghth then return error.
     */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if(length != sizeof(fbe_base_config_power_save_capable_info_t)) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload buffer length mismatch.\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    power_save_capable_p->spindown_qualified = fbe_provision_drive_is_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_SPIN_DOWN_QUALIFIED);

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_usurper_get_spindown_qualified()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_update_logical_error_stats()
 ******************************************************************************
 * @brief
 *  This function handles the update logical errors block transport usurper.
 * 
 * @param provision_drive_p - The provision drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  09/27/2012 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_update_logical_error_stats(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{   
    fbe_payload_ex_t *                          payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_block_transport_control_logical_error_t *    logical_error_p = NULL;
    fbe_block_edge_t *                          block_edge_p = NULL;
    fbe_object_id_t                             my_object_id;
    fbe_status_t                                status;

    fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_block_transport_control_logical_error_t),
                                                    (fbe_payload_control_buffer_t *)&logical_error_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(	(fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s get buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &my_object_id);
    if (logical_error_p->pvd_object_id != my_object_id)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s object ID provided does not match. actual=0x%x, expected 0x%x.\n",
                              __FUNCTION__, logical_error_p->pvd_object_id, my_object_id);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* For CRC errors, send the usurper packet to peer if the PVD is in SLF */
    if (((logical_error_p->error_type == FBE_BLOCK_TRANSPORT_ERROR_UNEXPECTED_CRC_MULTI_BITS) ||
         (logical_error_p->error_type == FBE_BLOCK_TRANSPORT_ERROR_UNEXPECTED_CRC_SINGLE_BIT) ||
         (logical_error_p->error_type == FBE_BLOCK_TRANSPORT_ERROR_UNEXPECTED_CRC)) &&
        fbe_provision_drive_slf_need_redirect(provision_drive_p))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s redirect to peer SP.\n",
                              __FUNCTION__);
        fbe_cmi_send_packet(packet_p);
        return FBE_STATUS_OK;
    }

    /* Otherwise send it downstream */
    fbe_base_config_get_block_edge((fbe_base_config_t*)provision_drive_p, &block_edge_p, 0);
    fbe_block_transport_send_control_packet(block_edge_p, packet_p);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_update_logical_error_stats()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_provision_drive_usurper_set_percent_rebuilt()
 ***************************************************************************** 
 * 
 * @brief   This usurper updates the `percent rebuilt' in the provision drive
 *          object.  This field is an opaque value as far as the provision
 *          drive in concerned.
 * 
 * @param   provision_drive_p - The provision drive.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  03/20/2013  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_usurper_set_percent_rebuilt(fbe_provision_drive_t *provision_drive_p, 
                                                            fbe_packet_t *packet_p)
{   
    fbe_payload_ex_t                                   *payload_p = NULL;
    fbe_payload_control_operation_t                    *control_operation_p = NULL;
    fbe_provision_drive_control_set_percent_rebuilt_t  *set_percent_rebuilt_p = NULL;
    fbe_status_t                                        status;

    fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_provision_drive_control_set_percent_rebuilt_t),
                                                    (fbe_payload_control_buffer_t *)&set_percent_rebuilt_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(	(fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s get buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Validate a sane value.
     */
    if (((fbe_s32_t)set_percent_rebuilt_p->percent_rebuilt < 0) ||
        (set_percent_rebuilt_p->percent_rebuilt > 100)             )
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s illegal percent rebuilt: %d value\n",
                                __FUNCTION__, set_percent_rebuilt_p->percent_rebuilt);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Update the value in the provision drive.
     */
    fbe_provision_drive_set_percent_rebuilt(provision_drive_p, set_percent_rebuilt_p->percent_rebuilt);
    
    /* Complete packet and return success.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/*******************************************************
 * end fbe_provision_drive_usurper_set_percent_rebuilt()
 *******************************************************/

/*!****************************************************************************
 *          fbe_provision_drive_control_reconstruct_paged_completion()
 ******************************************************************************
 *
 * @brief   Completion function for start encryption usurper.
 *
 * @param   packet_p     - Packet requesting operation.
 * @param   context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  01/10/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_control_reconstruct_paged_completion(fbe_packet_t * packet_p,
                                         fbe_packet_completion_context_t context)
{
    fbe_status_t                        status;
    fbe_provision_drive_t              *provision_drive_p = NULL;
    fbe_payload_ex_t                   *sep_payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_scheduler_hook_status_t         hook_status;

    provision_drive_p = (fbe_provision_drive_t *)context;

    /* This method is called after release the NP lock.  Therefore we cannot use
     * the packet status to determine the status of the re-init request.
     */
    status = fbe_transport_get_status_code(packet_p);
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* If the needs paged reconstuct is set or the encryption is not set
     * it indicates that the re-initialization of the paged failed.
     */
    if ((status != FBE_STATUS_OK)                                                                        ||
        fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, 
                                                    FBE_PROVISION_DRIVE_NP_FLAG_PAGED_NEEDS_CONSUMED) ||
        ( fbe_base_config_is_rekey_mode((fbe_base_config_t *)provision_drive_p)                    &&
         !fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, 
                                                      FBE_PROVISION_DRIVE_NP_FLAG_PAGED_VALID)     )    ) {
        /* If status is not good then mark the control request as failed.
         */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: reinit paged failed stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
    } else {
        /* No hook status is handled since we can only log here.
         */
        fbe_provision_drive_check_hook(provision_drive_p,
                                       SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION, 
                                       FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_OF_PAGED_SUCCESS, 
                                       0,  &hook_status);
        fbe_lifecycle_force_clear_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t*)provision_drive_p,
                                       FBE_PROVISION_DRIVE_LIFECYCLE_COND_PAGED_METADATA_RECONSTRUCT);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    
    /* Return success
     */
    return FBE_STATUS_OK;
}
/************************************************************** 
* end fbe_provision_drive_control_reconstruct_paged_completion()
***************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_control_reconstruct_paged()
 ******************************************************************************
 * @brief
 *  Start Encryption by rewriting the paged.
 * 
 * @param  provision_drive      - provision drive object.
 * @param  packet               - pointer to the packet.
 *
 * @return status               - status of the operation.
 *
 * @author
 *  10/30/2013 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_control_reconstruct_paged(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t                        status;
    fbe_payload_ex_t                   *sep_payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_scheduler_hook_status_t         hook_status;
    fbe_bool_t                      b_is_system_drive = FBE_FALSE;
    fbe_object_id_t                 object_id;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
    b_is_system_drive = fbe_database_is_object_system_pvd(object_id);

    /* Only user drives need to encrypt paged.
     */
    if (b_is_system_drive){
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    /* No hook status is handled since we can only log here.
     */
    fbe_provision_drive_check_hook(provision_drive_p,
                                   SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_ENCRYPTION, 
                                   FBE_PROVISION_DRIVE_SUBSTATE_ENCRYPTION_OF_PAGED_REQUEST, 
                                   0,  &hook_status);

    /* Set the condition that will restart the encryption.
     */
    status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, (fbe_base_object_t *)provision_drive_p,
                                    FBE_PROVISION_DRIVE_LIFECYCLE_COND_PAGED_METADATA_RECONSTRUCT);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR,  
                              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                              "%s Couldn't set the condition. status:0x%x\n", 
                              __FUNCTION__,status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return status;
    }

    /* Set the completion.
     */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_control_reconstruct_paged_completion, provision_drive_p);

    /* Take the NP during the paged metadata encryption.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    status = fbe_provision_drive_get_NP_lock(provision_drive_p, packet_p, fbe_provision_drive_mark_paged_write_start);
    return status;
}
/******************************************************************************
 * end fbe_provision_drive_control_reconstruct_paged()
*******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_usurper_validate_block_encryption()
 ******************************************************************************
 * @brief
 *  The operation validates if the data at the LBA and block count specified
 * by the user is really encrypted or not
 * 
 * @param  provision_drive      - provision drive object.
 * @param  packet               - pointer to the packet.
 *
 * @return status               - status of the operation.
 *
 * @author
 *  11/04/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_usurper_validate_block_encryption(fbe_provision_drive_t *provision_drive_p, fbe_packet_t *packet_p)
{
    fbe_payload_ex_t *                          payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_provision_drive_validate_block_encryption_t * block_encryption_info_p = NULL;
    fbe_status_t                                status;
    fbe_u32_t                   number_of_chunks = 0;
    fbe_memory_chunk_size_t     chunk_size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
    fbe_memory_request_t*   				memory_request_p = NULL;
    fbe_memory_request_priority_t   		memory_request_priority = 0;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Get the usurpur control buffer */
    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p, 
                                                    sizeof(fbe_provision_drive_validate_block_encryption_t),
                                                    (fbe_payload_control_buffer_t*)&block_encryption_info_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    memory_request_p = fbe_transport_get_memory_request(packet_p);
    memory_request_priority = fbe_transport_get_resource_priority(packet_p);	

    /* Determine the amount of memory that is required for this operation */
    fbe_provision_drive_calculate_memory_chunk_size(block_encryption_info_p->num_blocks_to_check,
                                                    &number_of_chunks,
                                                    &chunk_size);

    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,    
                           "%s: Check Encryption:0x%llx, Size:%d, Num chunk:%d, \n",
                           __FUNCTION__, block_encryption_info_p->num_blocks_to_check,
                          chunk_size, number_of_chunks);

    fbe_memory_build_chunk_request(memory_request_p, 
                                   chunk_size,
                                   number_of_chunks, 
                                   0, /* Lowest resource priority */
                                   FBE_MEMORY_REQUEST_IO_STAMP_INVALID,
                                   (fbe_memory_completion_function_t)fbe_provision_drive_validate_block_encryption_memory_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                    provision_drive_p);

    status = fbe_memory_request_entry(memory_request_p);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 									   
                              "memory allocation failed, status:0x%x, line:%d\n", status, __LINE__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);   	 
        fbe_transport_complete_packet(packet_p);
    }
    return status;
}
/*!**************************************************************
 * fbe_provision_drive_validate_block_encryption_memory_completion()
 ****************************************************************
 * @brief
 *  This function is the completion function for the memory
 *  allocation. It issues the actual read
 *
 * @param memory_request_p  
 * @param in_context   
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/03/2013 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
static fbe_status_t 
fbe_provision_drive_validate_block_encryption_memory_completion(fbe_memory_request_t * memory_request_p, 
                                                                fbe_memory_completion_context_t in_context)
{
    fbe_packet_t*   							packet_p = NULL;
    fbe_status_t								status;
    fbe_payload_control_operation_t*			control_operation_p = NULL;
    fbe_payload_ex_t*  						    payload_p = NULL;
    fbe_provision_drive_t*  					provision_drive_p = NULL;    
    fbe_memory_header_t*						master_memory_header_p = NULL;
    fbe_memory_header_t*						next_memory_header_p = NULL;
    fbe_memory_header_t*						current_memory_header_p = NULL;
    fbe_sg_element_t *  						sg_list_p = NULL;
    fbe_provision_drive_validate_block_encryption_t * block_encryption_info_p = NULL;
    fbe_optimum_block_size_t                    optimum_block_size;
    fbe_payload_block_operation_t               *block_operation_p;
    fbe_u32_t                                   sg_list_size;
    fbe_u8_t *                                  buffer_p = NULL;
    fbe_memory_chunk_size_t                     current_size;
    fbe_sg_element_t                            *sg_p;
    fbe_u32_t                                   chunks_used;
    fbe_packet_t*       						new_packet_p = NULL;
    fbe_payload_ex_t*  						    new_payload_p = NULL;

    provision_drive_p = (fbe_provision_drive_t*)in_context;
    
    // Get the memory that was allocated
    master_memory_header_p = fbe_memory_get_first_memory_header(memory_request_p);
    current_memory_header_p = master_memory_header_p;

    // get the pointer to original packet.
    packet_p = (fbe_packet_t *)fbe_transport_memory_request_to_packet(memory_request_p);	
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);    

    /* Get the usurpur control buffer */
    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p, 
                                                    sizeof(fbe_provision_drive_validate_block_encryption_t),
                                                    (fbe_payload_control_buffer_t*)&block_encryption_info_p);

    /* We need one extra for termination */
    sg_list_size = (master_memory_header_p->number_of_chunks + 1) * sizeof(fbe_sg_element_t);
    
    /* find the last chunk */
    for (chunks_used = 0; chunks_used < master_memory_header_p->number_of_chunks - 1; chunks_used ++)
    {
        current_memory_header_p = current_memory_header_p->u.hptr.next_header;
    }

    /* carve memory for SG List from the end of the last chunk */
    buffer_p = current_memory_header_p->data + master_memory_header_p->memory_chunk_size - sg_list_size;
    sg_list_p = (fbe_sg_element_t *) buffer_p;
    sg_p = sg_list_p;
    fbe_memory_get_next_memory_header(master_memory_header_p, &next_memory_header_p);

    current_memory_header_p = master_memory_header_p;
    current_size = master_memory_header_p->memory_chunk_size;
    while (current_memory_header_p != NULL)
    {
        buffer_p = current_memory_header_p->data;
        fbe_sg_element_init(sg_list_p, current_size, buffer_p);
        fbe_sg_element_increment(&sg_list_p);

        /* next chunk */
        current_memory_header_p = current_memory_header_p->u.hptr.next_header;
    }
    fbe_sg_element_terminate(sg_list_p);

    new_packet_p = fbe_transport_allocate_packet();
    if (new_packet_p == NULL){
       fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,    
                               "%s: Unable to allocate packet\n",
                               __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_packet(new_packet_p);
    new_payload_p = fbe_transport_get_payload_ex(new_packet_p);
    
    block_operation_p = fbe_payload_ex_allocate_block_operation(new_payload_p);
    if (block_operation_p == NULL) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,    
                               "%s: Unable to allocate block operation\n",
                               __FUNCTION__);
           fbe_memory_free_request_entry(memory_request_p);
           fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
           fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);   	 
           fbe_transport_complete_packet(packet_p);
           return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_payload_ex_set_sg_list(new_payload_p, sg_p, 0);

    /* allocate and initialize the block operation. */
    fbe_block_transport_edge_get_optimum_block_size(&provision_drive_p->block_edge, &optimum_block_size);

    status = fbe_payload_block_build_operation(block_operation_p,
                                                   FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ,
                                                   block_encryption_info_p->encrypted_pba,
                                                   block_encryption_info_p->num_blocks_to_check,
                                                   FBE_BE_BYTES_PER_BLOCK,
                                                   optimum_block_size,
                                                   NULL/*HACK for now, we don't need it*/);

   fbe_transport_set_completion_function(new_packet_p, 
                                          fbe_provision_drive_usurper_validate_block_encryption_completion, 
                                          provision_drive_p);

    fbe_transport_add_subpacket(packet_p, new_packet_p);
    fbe_base_object_add_to_usurper_queue((fbe_base_object_t *)provision_drive_p, packet_p);
    status = fbe_base_config_send_functional_packet((fbe_base_config_t *)provision_drive_p, new_packet_p, 0);
    return status;

}

/*!****************************************************************************
* fbe_provision_drive_usurper_validate_block_encryption_completion()
******************************************************************************
* @brief
*  This is a completion function for where we have the data that was read
* to determine if the block(s) were encrypted or not
* 
 * @param   packet_p        - pointer to disk zeroing IO packet
* @param   context         - context, which is a pointer to the pvd object.
*
* @return fbe_status_t.    - status of the operation.
*
* @author
*  11/04/2013 - Created. Ashok Tamilarasan
*
******************************************************************************/
static fbe_status_t 
fbe_provision_drive_usurper_validate_block_encryption_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t*  					provision_drive_p = NULL;    
    fbe_sg_element_t *  						sg_list_p = NULL;
    fbe_provision_drive_validate_block_encryption_t * block_encryption_info_p = NULL;
    fbe_memory_request_t*   					memory_request_p = NULL;
    fbe_payload_block_operation_t               *block_operation_p;
    fbe_lba_t                                   start_lba;
    fbe_block_count_t                           block_count;
    fbe_payload_block_operation_status_t        block_status;
    fbe_payload_control_operation_t*			master_control_operation_p = NULL;
    fbe_payload_ex_t*  						    payload_p = NULL;
    fbe_payload_ex_t*      					    master_payload_p = NULL;
    fbe_status_t                                status;
    fbe_packet_t*                               master_packet_p;
    fbe_u32_t                                   index;

    provision_drive_p = (fbe_provision_drive_t*)context;

    master_packet_p = fbe_transport_get_master_packet(packet_p);

    /* Get the subpacket payload and control operation., input buffer and sglist */
    master_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    master_control_operation_p = fbe_payload_ex_get_control_operation(master_payload_p); 	 

    /* Get the usurpur control buffer */
    status = fbe_provision_drive_get_control_buffer(provision_drive_p, master_packet_p, 
                                                    sizeof(fbe_provision_drive_validate_block_encryption_t),
                                                    (fbe_payload_control_buffer_t*)&block_encryption_info_p);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_payload_block_get_status(block_operation_p, &block_status);
    block_encryption_info_p->blocks_encrypted = 0;
    if(block_status == FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) 
    {
        /* Get the start lba of the block operation. */
        fbe_payload_block_get_lba(block_operation_p, &start_lba);
        fbe_payload_block_get_block_count(block_operation_p, &block_count);
    
        /* get sg list with new packet. */
        fbe_payload_ex_get_sg_list(payload_p, &sg_list_p, NULL);

        for(index = 0; index < block_count; index++)
        {
            status = fbe_provision_drive_validate_checksum(provision_drive_p,
                                                           sg_list_p, 
                                                           start_lba, 
                                                           1,
                                                           index);
            /* If we got an error, so the data should be encrypted */
            block_encryption_info_p->blocks_encrypted += (status != FBE_STATUS_OK) ? 1: 0;
        }

        fbe_payload_control_set_status(master_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,    
                               "%s: Num Encrypted: %d \n",
                               __FUNCTION__, (fbe_u32_t)block_encryption_info_p->blocks_encrypted);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INFO,    
                               "%s: Error reading data. Status:%d, \n",
                               __FUNCTION__, block_status);
        fbe_payload_control_set_status(master_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }

    fbe_base_object_remove_from_usurper_queue((fbe_base_object_t *)provision_drive_p, master_packet_p);
    fbe_transport_remove_subpacket(packet_p);
    fbe_transport_release_packet(packet_p);
    memory_request_p = fbe_transport_get_memory_request(master_packet_p);
    fbe_memory_free_request_entry(memory_request_p);

    fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);   
    fbe_transport_complete_packet(master_packet_p); 
    return FBE_STATUS_OK;
}

/******************************************************************************
*            fbe_provision_drive_calculate_memory_chunk_size ()
*******************************************************************************
*
*  Description: This function calculates the total chunks and the chunk size 
* required for the operation including the memory for the data transfer and
* other associated tracking structure(SG)
*
* 
*  Inputs: blocks  - Total data blocks to be read or written
*          chunks - Number of chunks required to satisfy the request
*          chunk_size - Chunk size
*
*  Return Value: 
*   success or failure
*
*  History:
*   11/05/13 - Ashok Tamilarasan - Created     
******************************************************************************/
static fbe_u32_t fbe_provision_drive_calculate_memory_chunk_size(fbe_block_count_t blocks, 
                                                                 fbe_u32_t *chunks, 
                                                                 fbe_memory_chunk_size_t *chunk_size)
{
    fbe_u32_t                           number_of_chunks = 0;
    fbe_u32_t                           number_of_data_chunks = 0;
    fbe_memory_chunk_size_t             size;
    fbe_memory_chunk_size_block_count_t chunk_blocks;
    fbe_u32_t                           sg_list_size = 0;
    fbe_u64_t                           total_transfer_size_with_ts = 0;

    if(blocks <= FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_1)
    {
        size = FBE_MEMORY_CHUNK_SIZE_FOR_1_BLOCK_IO;
        chunk_blocks = FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_1;
    }
    else if (blocks > FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_PACKET)
    {
        size = FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO;
        chunk_blocks = FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_64;
    }
    else
    {
        size = FBE_MEMORY_CHUNK_SIZE_FOR_PACKET;
        chunk_blocks = FBE_MEMORY_CHUNK_SIZE_BLOCK_COUNT_PACKET;
    }

    // 2. size the sg_list, enough to hold one extra element
    sg_list_size = (sizeof(fbe_sg_element_t) * (number_of_data_chunks+1));
    
    /* we need to add tracking structure and sg element to the total size for memory needed*/
    total_transfer_size_with_ts = (blocks * FBE_BE_BYTES_PER_BLOCK) + sg_list_size;

    number_of_chunks = (fbe_u32_t) total_transfer_size_with_ts / size;

    if (total_transfer_size_with_ts % size)
    {
        number_of_chunks ++; //one more chunk for the leftover
    }

    *chunks = number_of_chunks;
    *chunk_size = size; 

    return (FBE_STATUS_OK);
}

/*!****************************************************************************
 * fbe_provision_drive_unregister_keys()
 ******************************************************************************
 * @brief
 *  This function is used to unregister the keys with miniport
 * 
 * @param  provision_drive      - provision drive object.
 * @param  packet               - pointer to the packet.
 *
 * @return status               - status of the operation.
 *
 * @author
 *  12/18/2013 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_unregister_keys(fbe_provision_drive_t * provision_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t *                          payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_edge_index_t                            server_index;
    fbe_status_t                                status;
    fbe_bool_t                      b_is_system_drive = FBE_FALSE;
    fbe_object_id_t                 object_id;

    fbe_provision_drive_utils_trace(provision_drive,
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    /* Get the edge this packet come from */
    fbe_transport_get_server_index(packet, &server_index);
    if(server_index >= FBE_PROVISION_DRIVE_KEY_INFO_TABLE_MAX)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Client Index %d greater than supported%d.\n",
                                __FUNCTION__, server_index, FBE_PROVISION_DRIVE_KEY_INFO_TABLE_MAX);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(provision_drive->key_info_table[server_index].key_handle == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s key_handle NULL. \n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    /* Set key handle to NULL */
    provision_drive->key_info_table[server_index].key_handle = NULL;

    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive, &object_id);
    b_is_system_drive = fbe_database_is_object_system_pvd(object_id);
    if (b_is_system_drive) {
        /* Get the monitor to unregister the keys.  User drives do this in activate.
         */
        status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, 
                                        (fbe_base_object_t *)provision_drive,
                                        FBE_BASE_CONFIG_LIFECYCLE_COND_ENCRYPTION);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)provision_drive, 
                                  FBE_TRACE_LEVEL_ERROR,  
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                  "%s Couldn't set the condition. status:0x%x\n", 
                                  __FUNCTION__,status);
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            return status;
        }
        fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const,
                                 (fbe_base_object_t*)provision_drive,
                                 (fbe_lifecycle_timer_msec_t)0);
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_provision_drive_unregister_keys()
*******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_port_unregister_keys()
 ******************************************************************************
 * @brief
 *  This function is used to unregister the keys with miniport
 * 
 * @param  provision_drive      - provision drive object.
 * @param  packet               - pointer to the packet.
 * @param  paserver_indexcket   - server index.
 *
 * @return status               - status of the operation.
 *
 * @author
 *  12/18/2013 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t
fbe_provision_drive_port_unregister_keys(fbe_provision_drive_t * provision_drive_p, 
                                         fbe_packet_t * packet_p,
                                         fbe_edge_index_t server_index)
{
    fbe_status_t                                status;
    fbe_payload_ex_t *                          payload = NULL;
    fbe_payload_control_operation_t *           control_operation = NULL;
    fbe_base_port_mgmt_unregister_keys_t *      port_unregister_keys;
    fbe_payload_control_operation_t *           new_control_operation_p = NULL;
    fbe_provision_drive_register_keys_context_t * context_p = NULL;

    fbe_provision_drive_utils_trace(provision_drive_p,
                                    FBE_TRACE_LEVEL_DEBUG_HIGH,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                    FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                    "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_any_control_operation(payload);  

    new_control_operation_p = fbe_payload_ex_allocate_control_operation(payload);
    if(new_control_operation_p == NULL)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_payload_ex_increment_control_operation_level(payload);

    /* Allocate the memory for the physical drive management information. */
    port_unregister_keys = (fbe_base_port_mgmt_unregister_keys_t *) fbe_transport_allocate_buffer();
    if (port_unregister_keys == NULL)
    {
        fbe_payload_ex_release_control_operation(payload, new_control_operation_p);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Setup context. */
    context_p = (fbe_provision_drive_register_keys_context_t*) (port_unregister_keys + 1);
    context_p->client_index = server_index;

    fbe_zero_memory(port_unregister_keys, sizeof(fbe_base_port_mgmt_unregister_keys_t));
    port_unregister_keys->num_of_keys = 0;
    if (provision_drive_p->key_info_table[server_index].mp_key_handle_1 != FBE_INVALID_KEY_HANDLE)
    {
        port_unregister_keys->mp_key_handle[port_unregister_keys->num_of_keys] = provision_drive_p->key_info_table[server_index].mp_key_handle_1;
        port_unregister_keys->num_of_keys++;
    }

    if (provision_drive_p->key_info_table[server_index].mp_key_handle_2 != FBE_INVALID_KEY_HANDLE)
    {
        port_unregister_keys->mp_key_handle[port_unregister_keys->num_of_keys] = provision_drive_p->key_info_table[server_index].mp_key_handle_2;
        port_unregister_keys->num_of_keys++;
    }

    if (port_unregister_keys->num_of_keys == 0){
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Nothing to unregister. \n",
                                __FUNCTION__);
        fbe_transport_release_buffer(port_unregister_keys);
        fbe_payload_ex_release_control_operation(payload, new_control_operation_p);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_register_release_memory, provision_drive_p);

    status = fbe_provision_drive_unregister_keys_send_packet(provision_drive_p, 
                                                             packet_p, 
                                                             port_unregister_keys, 
                                                             server_index, 
                                                             fbe_provision_drive_port_unregister_keys_completion);

    return status;
}
/******************************************************************************
 * end fbe_provision_drive_port_unregister_keys()
*******************************************************************************/

/*!****************************************************************************
 * fbe_provision_drive_port_unregister_keys_completion()
 ******************************************************************************
 * @brief
 *  This function is used as a completion function for the port number and it
 *  issues another operation to get the enclosure number info.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  12/18/2013 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_port_unregister_keys_completion(fbe_packet_t * packet_p,
                                                    fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *                                 provision_drive_p = NULL;
    fbe_payload_ex_t *                                      sep_payload_p = NULL;
    fbe_payload_control_operation_t *                       control_operation_p = NULL;
    fbe_payload_control_operation_t *                       prev_control_operation_p = NULL;
    fbe_base_port_mgmt_unregister_keys_t *                  port_unregister_keys = NULL;
    fbe_status_t                                            status;
    fbe_payload_control_status_t                            control_status;
    fbe_u8_t *                                              buffer_p = NULL;
    fbe_provision_drive_register_keys_context_t * context_p = NULL;

    provision_drive_p = (fbe_provision_drive_t *)context;

    /* get the current control operation for the port information. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* get the status of the control operation. */
    fbe_payload_control_get_status(control_operation_p, &control_status);
    status = fbe_transport_get_status_code(packet_p);

    /* get the previous control operation and associated buffer. */
    if (control_operation_p->operation_header.prev_header->payload_opcode != FBE_PAYLOAD_OPCODE_CONTROL_OPERATION) {
        /* This is the unregister following an unexpected error.
         */
        prev_control_operation_p = NULL;
    }else {
        prev_control_operation_p = fbe_payload_ex_get_prev_control_operation(sep_payload_p);
    }
    /* get the buffer of the current control operation. */
    fbe_payload_control_get_buffer(control_operation_p, &buffer_p);
    port_unregister_keys = (fbe_base_port_mgmt_unregister_keys_t *) buffer_p;
    context_p = (fbe_provision_drive_register_keys_context_t*) (port_unregister_keys + 1);

    /* If status is not good then complete the packet with error. */
    if((status != FBE_STATUS_OK) ||
       (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: get drive location failed, ctl_stat:0x%x, stat:0x%x, \n",
                               __FUNCTION__, control_status, status);
        status = (status != FBE_STATUS_OK) ? status : FBE_STATUS_GENERIC_FAILURE;
        control_status = (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK) ? control_status : FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
        fbe_transport_set_status(packet_p, status, 0);
        if (prev_control_operation_p != NULL) {
            fbe_payload_control_set_status(prev_control_operation_p, control_status);
        }
        return status;
    }

    provision_drive_p->key_info_table[context_p->client_index].mp_key_handle_1 =  FBE_INVALID_KEY_HANDLE;
    provision_drive_p->key_info_table[context_p->client_index].mp_key_handle_2 =  FBE_INVALID_KEY_HANDLE;
    /* The key_handle could still be valid */
    //provision_drive_p->key_info_table[context_p->client_index].key_handle = NULL;
    provision_drive_p->key_info_table[context_p->client_index].port_object_id = FBE_OBJECT_ID_INVALID;

    return FBE_STATUS_OK;
}
/********************************************************* 
* end fbe_provision_drive_port_unregister_keys_completion()
**********************************************************/

/*!****************************************************************************
 * fbe_provision_drive_usurper_set_encryption_mode
 ******************************************************************************
 *
 * @brief
 *    This function sets the encryption mode for the object. 
 *    
 *
 * @param   in_packet_p 		  -  pointer to control packet requesting this
 *  								 operation
 * @param   context 			  -  context, which is a pointer to the base config
 *  								 object
 *
 * @return  fbe_status_t		  -  status of this operation
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_usurper_set_encryption_mode(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_base_config_control_encryption_mode_t * set_encryption_mode = NULL;
    fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;
    fbe_provision_drive_config_type_t   current_config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_INVALID;
    fbe_object_id_t						object_id;
    fbe_bool_t							is_system_drive;
    fbe_lifecycle_state_t               lifecycle_state; 
    fbe_base_config_encryption_mode_t   encryption_mode;

     /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_base_config_control_encryption_mode_t),
                                                    (fbe_payload_control_buffer_t)&set_encryption_mode);

    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }
    fbe_base_config_get_encryption_mode((fbe_base_config_t*)provision_drive_p, &encryption_mode);

    status = fbe_base_config_set_encryption_mode((fbe_base_config_t*)provision_drive_p, set_encryption_mode->encryption_mode);

    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *) provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s can't get encryption state, status: %d\n", __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_provision_drive_get_config_type(provision_drive_p, &current_config_type);
    
    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
    is_system_drive = fbe_database_is_object_system_pvd(object_id);

    fbe_base_object_get_lifecycle_state((fbe_base_object_t*)provision_drive_p, &lifecycle_state);
    /* We do not need to process the validate bits in specialize since they will get setup as the object 
     * pushes its keys. 
     */
    if (is_system_drive &&
        (lifecycle_state != FBE_LIFECYCLE_STATE_SPECIALIZE) &&
        (encryption_mode != set_encryption_mode->encryption_mode) &&
        (set_encryption_mode->encryption_mode == FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED)) {

        /* We need to check if we need to change the bits in the nonpaged.
         */
        fbe_transport_set_completion_function(packet_p, fbe_provision_drive_check_scrub_needed, provision_drive_p);
        status = fbe_provision_drive_get_NP_lock(provision_drive_p, packet_p, fbe_provision_drive_encrypt_validate_update_all);
    } else {
        fbe_provision_drive_check_scrub_needed(packet_p, provision_drive_p);
    }
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/*!****************************************************************************
 * fbe_provision_drive_check_scrub_needed()
 ******************************************************************************
 * @brief
 *  Determine if we need to set the scrub needed bit.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  3/10/2014 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_check_scrub_needed(fbe_packet_t * packet_p,
                                              fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *provision_drive_p = NULL;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_base_config_control_encryption_mode_t * set_encryption_mode = NULL;
    fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;
    fbe_memory_request_t *              memory_request_p = NULL;
    fbe_memory_request_priority_t       memory_request_priority = 0;
    fbe_provision_drive_config_type_t   current_config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_INVALID;
    fbe_cmi_sp_state_t                  sp_state;
    fbe_lba_t                           background_zero_checkpoint;

    provision_drive_p = (fbe_provision_drive_t *)context;

     /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_base_config_control_encryption_mode_t),
                                                    (fbe_payload_control_buffer_t)&set_encryption_mode);

    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    status = fbe_base_config_set_encryption_mode((fbe_base_config_t*)provision_drive_p, set_encryption_mode->encryption_mode);

    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *) provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s can't get encryption state, status: %d\n", __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    fbe_provision_drive_get_config_type(provision_drive_p, &current_config_type);

    if ((set_encryption_mode->encryption_mode != FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED) ||
        (current_config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* we need to clear the scrub needed bit when consumed PVD changed to encrypted mode,
     * and set scrub complete.
     * only active side need to do this.
     */
    fbe_cmi_get_current_sp_state(&sp_state);
    if ((!(fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_NEEDED))) ||
        (sp_state != FBE_CMI_STATE_ACTIVE))
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    fbe_provision_drive_metadata_get_background_zero_checkpoint(provision_drive_p, &background_zero_checkpoint); 

    /* Only clear scrub needed if we are not still in the process of scrubbing.
     * It is possible that this consumed drive has extra space that is not consumed by any RAID Group and it 
     * is still in the process of scrubbing. 
     */
    if ((background_zero_checkpoint != FBE_LBA_INVALID) ||
        fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_INTENT)) {
        fbe_provision_drive_np_flags_t np_flags;
        fbe_provision_drive_metadata_get_np_flag(provision_drive_p, &np_flags);
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "provision drive -> ENCRYPTED scrubing in progress: bgchpt: 0x%llx flags: 0x%x \n",
                              background_zero_checkpoint, np_flags);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    /* We need a subpacket since we ran out of completion function stack in the packet.
     */
    memory_request_p = fbe_transport_get_memory_request(packet_p);
    memory_request_priority = fbe_transport_get_resource_priority(packet_p);
    memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;

    /* build the memory request for allocation.  We add (1) to the memory
     * request priority of the packet. */
    fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                   1,
                                   memory_request_priority,
                                   fbe_transport_get_io_stamp(packet_p),
                                   (fbe_memory_completion_function_t)fbe_provision_drive_set_scrub_complete_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   (fbe_memory_completion_context_t)provision_drive_p);

    fbe_transport_memory_request_set_io_master(memory_request_p,  packet_p);

    fbe_memory_request_entry(memory_request_p);
    /* Completion function is always called.
     */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/********************************************************* 
* end fbe_provision_drive_check_scrub_needed()
**********************************************************/
/*!****************************************************************************
 * @fn fbe_provision_drive_set_scrub_complete_allocation_completion()
 ******************************************************************************
 * @brief
 *  This is the completion for allocating memory for the packet we use for NP IO.
 *
 * @param memory_request - Pointer to the memory request.
 * @param context        - Pointer to the provision drive object.
 *
 * @return fbe_status_t.
 * 
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_set_scrub_complete_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                                             fbe_memory_completion_context_t context)
{
    fbe_provision_drive_t *provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_packet_t *master_packet_p = NULL;
    fbe_packet_t *sub_packet_p = NULL;
    fbe_payload_ex_t *master_payload_p = NULL;    
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_memory_header_t * master_memory_header = NULL; 

    master_packet_p = fbe_transport_memory_request_to_packet(memory_request_p);
    master_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(master_payload_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE) {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_provision_drive_utils_trace(provision_drive_p, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "pvd initiate disk zeroing memory request: 0x%p state: %d allocation failed \n",
                                        memory_request_p, memory_request_p->request_state);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        
        fbe_transport_complete_packet(master_packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    master_memory_header = memory_request_p->ptr;   
    sub_packet_p = (fbe_packet_t*)fbe_memory_header_to_data(master_memory_header);
    fbe_transport_initialize_packet(sub_packet_p);

    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "pvd set scrub complete in NP.\n");

    fbe_transport_add_subpacket(master_packet_p, sub_packet_p);
    fbe_transport_set_completion_function(sub_packet_p, fbe_provision_drive_set_scrub_bits_completion, provision_drive_p);

    fbe_transport_set_completion_function(sub_packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);
    fbe_provision_drive_get_NP_lock(provision_drive_p, sub_packet_p, fbe_provision_drive_set_scrub_complete_bits);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/***********************************************************************************
 * end fbe_provision_drive_set_scrub_complete_allocation_completion()
 ***********************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_set_scrub_complete_bits()
 ******************************************************************************
 * @brief
 *
 *  This is the callback function after the nonpaged lock. 
 * 
 * @param sg_list           - Pointer to sg list.
 * @param slot_offset       - Offset to the paged metadata.
 * @param context           - Pointer to context.
 *
 * @return fbe_status_t.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_set_scrub_complete_bits(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t * provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_status_t status;
    fbe_provision_drive_np_flags_t          np_flags;

    status = fbe_provision_drive_metadata_get_np_flag(provision_drive_p, &np_flags);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "UZ: failed to get non-paged flags, status %d\n",
                              status);
        return FBE_STATUS_OK;
    }

    np_flags &= ~FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_NEEDED;
    np_flags &= ~FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_INTENT;
    np_flags |= FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_COMPLETE;

    /* this function always returns FBE_STATUS_MORE_PROCESSING_REQUIRED */
    status = fbe_provision_drive_metadata_np_flag_set(provision_drive_p, packet_p, np_flags);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_set_scrub_complete_bits()
*******************************************************************************/


/*!****************************************************************************
 * fbe_provision_drive_initiate_consumed_area_zeroing()
 ******************************************************************************
 * @brief
 *  This function sets up the NP bit for background monitor to zero consumed area.
 *
 * @param provision_drive   - The provision drive.
 * @param in_packet_p        - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_setup_consumed_area_zeroing(fbe_provision_drive_t*   provision_drive_p,
                                                   fbe_packet_t*            packet_p)
{
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_memory_request_t *           memory_request_p = NULL;
    fbe_memory_request_priority_t    memory_request_priority = 0;
    fbe_cmi_sp_state_t               sp_state;

    /* no op from passive side */
    fbe_cmi_get_current_sp_state(&sp_state);
    if (sp_state != FBE_CMI_STATE_ACTIVE)
    {
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* We need a subpacket since we ran out of completion function stack in the packet.
     */
    memory_request_p = fbe_transport_get_memory_request(packet_p);
    memory_request_priority = fbe_transport_get_resource_priority(packet_p);
    memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;

    /* build the memory request for allocation.  We add (1) to the memory
     * request priority of the packet. */
    fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                   1,
                                   memory_request_priority,
                                   fbe_transport_get_io_stamp(packet_p),
                                   (fbe_memory_completion_function_t)fbe_provision_drive_set_scrub_intent_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   (fbe_memory_completion_context_t)provision_drive_p);

    fbe_transport_memory_request_set_io_master(memory_request_p,  packet_p);

    fbe_memory_request_entry(memory_request_p);
    /* Completion function is always called.
     */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_provision_drive_setup_consumed_area_zeroing()
*******************************************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_set_scrub_intent_allocation_completion()
 ******************************************************************************
 * @brief
 *  This is the completion for allocating memory for the packet we use for NP IO.
 *
 * @param memory_request - Pointer to the memory request.
 * @param context        - Pointer to the provision drive object.
 *
 * @return fbe_status_t.
 * 
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_set_scrub_intent_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                                           fbe_memory_completion_context_t context)
{
    fbe_provision_drive_t *provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_packet_t *master_packet_p = NULL;
    fbe_packet_t *sub_packet_p = NULL;
    fbe_payload_ex_t *master_payload_p = NULL;    
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_memory_header_t * master_memory_header = NULL; 

    master_packet_p = fbe_transport_memory_request_to_packet(memory_request_p);
    master_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(master_payload_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE) {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_provision_drive_utils_trace(provision_drive_p, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "pvd initiate disk zeroing memory request: 0x%p state: %d allocation failed \n",
                                        memory_request_p, memory_request_p->request_state);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        
        fbe_transport_complete_packet(master_packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    master_memory_header = memory_request_p->ptr;   
    sub_packet_p = (fbe_packet_t*)fbe_memory_header_to_data(master_memory_header);
    fbe_transport_initialize_packet(sub_packet_p);

    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "pvd set scrub intent in NP.\n");

    fbe_transport_add_subpacket(master_packet_p, sub_packet_p);
    fbe_transport_set_completion_function(sub_packet_p, fbe_provision_drive_set_scrub_bits_completion, provision_drive_p);

    fbe_transport_set_completion_function(sub_packet_p, fbe_provision_drive_release_NP_lock, provision_drive_p);
    fbe_provision_drive_get_NP_lock(provision_drive_p, sub_packet_p, fbe_provision_drive_set_scrub_intent_bits);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/***********************************************************************************
 * end fbe_provision_drive_set_scrub_intent_allocation_completion()
 ***********************************************************************************/
/*!**************************************************************
 * fbe_provision_drive_get_unconsumed_mark_range()
 ****************************************************************
 * @brief
 *  Determine the range of the PVD that needs to be marked
 *  for scrubbing.
 *
 * @param provision_drive_p
 * @param lba_p
 * @param blocks_p
 * 
 * @return fbe_status_t
 *
 * @author
 *  7/7/2014 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_provision_drive_get_unconsumed_mark_range(fbe_provision_drive_t *provision_drive_p,
                                                                  fbe_lba_t *lba_p,
                                                                  fbe_block_count_t *blocks_p)
{
    fbe_lba_t lba;
    fbe_lba_t default_offset;
    fbe_lba_t paged_metadata_start_lba;

    fbe_base_config_get_default_offset((fbe_base_config_t*)provision_drive_p, &default_offset);
    fbe_base_config_metadata_get_paged_record_start_lba((fbe_base_config_t *) provision_drive_p,
                                                        &paged_metadata_start_lba);
    fbe_block_transport_server_find_max_consumed_lba(&provision_drive_p->base_config.block_transport_server,
                                                     &lba);
    if (lba == FBE_LBA_INVALID) {
        /* If the lba is invalid it means three is nothing consumed.  Set the 
         * consumed area from the default offset to the provision drive paged.
         */
        *lba_p = default_offset;
        *blocks_p = paged_metadata_start_lba - default_offset;
    } else if ((lba + 1) >= paged_metadata_start_lba) {
        /* The entire capacity is consumed.  Set the lba to invalid and the 
         * block count to 0.
         */
        *lba_p = FBE_LBA_INVALID;
        *blocks_p = 0;
    } else {
        /* There is a portion of the extent that is unconsumed.
         */
        lba ++;  /* Get the start of the range to mark. */
        if (lba < default_offset) {
            /* Mark starting at the default offset and ending at the metadata.
             */
            lba = default_offset;
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "unconsumed mark range starts at default offset lba: 0x%llx.\n", default_offset);
        }
        /* Mark from this lba to start of metadata.
         */
        *blocks_p = paged_metadata_start_lba - lba;
        *lba_p = lba;
    }

    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "unconsumed mark range is lba: 0x%llx blocks: 0x%llx md_lba: 0x%llx.\n", 
                          *lba_p, *blocks_p, paged_metadata_start_lba);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_provision_drive_get_unconsumed_mark_range()
 ******************************************/
/*!****************************************************************************
 * @fn fbe_provision_drive_set_scrub_intent_bits()
 ******************************************************************************
 * @brief
 *
 *  This is the callback function after the nonpaged lock. 
 * 
 * @param sg_list           - Pointer to sg list.
 * @param slot_offset       - Offset to the paged metadata.
 * @param context           - Pointer to context.
 *
 * @return fbe_status_t.
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_set_scrub_intent_bits(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t * provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_status_t status;
    fbe_provision_drive_np_flags_t          np_flags;
    fbe_provision_drive_config_type_t           current_config_type = FBE_PROVISION_DRIVE_CONFIG_TYPE_INVALID;
    fbe_lba_t default_offset;
    fbe_block_count_t capacity; 

    fbe_provision_drive_get_config_type(provision_drive_p, &current_config_type);

    status = fbe_provision_drive_metadata_get_np_flag(provision_drive_p, &np_flags);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "UZ: failed to get non-paged flags, status %d\n",
                              status);
        return FBE_STATUS_OK;
    }

    /* for consumed PVD, set NEEDED bit.  once raid group completes encryption
     * this NEEDED bit will be replaced by COMPLETE bit.
     */
    if (current_config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID)
    {
        fbe_lba_t lba;
        fbe_block_count_t blocks;
        fbe_provision_drive_get_unconsumed_mark_range(provision_drive_p, &lba, &blocks);

        if (blocks >= FBE_PROVISION_DRIVE_CHUNK_SIZE) {
            /* set INTENT bit to let background monitor pick up the task */
            np_flags &= ~FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_NEEDED;
            np_flags |= FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_INTENT;
            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p,
                                  FBE_TRACE_LEVEL_INFO,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "Set SCRUB_INTENT on user area for lba: 0x%llx bl: 0x%llx flags: 0x%x\n",
                                  lba, blocks, np_flags);
        } else {
            np_flags &= ~FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_INTENT;
            np_flags |= FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_NEEDED;
        }
    }
    else
    {
        fbe_base_config_get_default_offset((fbe_base_config_t*)provision_drive_p, &default_offset);
        fbe_base_config_get_capacity((fbe_base_config_t*)provision_drive_p, &capacity);
        if ((capacity - default_offset) >= FBE_PROVISION_DRIVE_CHUNK_SIZE) {
            /* set INTENT bit to let background monitor pick up the task */
            np_flags &= ~FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_NEEDED;
            np_flags |= FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_INTENT;
        } else {
            np_flags &= ~FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_NEEDED;
            np_flags &= ~FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_INTENT;
            np_flags |= FBE_PROVISION_DRIVE_NP_FLAG_PAGED_SCRUB_COMPLETE;
        }
    }

    /* this function always returns FBE_STATUS_MORE_PROCESSING_REQUIRED */
    status = fbe_provision_drive_metadata_np_flag_set(provision_drive_p, packet_p, np_flags);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/***********************************************************************************
 * end fbe_provision_drive_set_scrub_intent_bits()
 ***********************************************************************************/
/*!**************************************************************
 * fbe_provision_drive_set_scrub_bits_completion()
 ****************************************************************
 * @brief
 *  Completion for when we have kicked off set scrub bits.
 *
 * @param paket_p -  current usurper op.
 * @param context - the provision drive.             
 *
 * @return -   
 *
 * @author
 *  02/07/2014  NCHIU  - Created.
 *
 ****************************************************************/

static fbe_status_t
fbe_provision_drive_set_scrub_bits_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_payload_ex_t *master_payload_p = NULL;
    fbe_status_t status;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_packet_t *master_packet_p = fbe_transport_get_master_packet(packet_p);
    fbe_memory_request_t *memory_request_p = NULL;

    status = fbe_transport_get_status_code(packet_p);

    master_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(master_payload_p);

    /* We need to remove packet from master queue */
    fbe_transport_remove_subpacket(packet_p);

    fbe_transport_destroy_packet(packet_p);

    memory_request_p = fbe_transport_get_memory_request(master_packet_p);
    //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
    //fbe_memory_free_entry(memory_ptr);
    fbe_memory_free_request_entry(memory_request_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "set_scrub_bits_completion, status:%d.\n",
                              status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(master_packet_p, status, 0);
        fbe_transport_complete_packet(master_packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);

    fbe_transport_complete_packet(master_packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
} 
/******************************************
 * end fbe_provision_drive_set_scrub_bits_completion()
 ******************************************/

/*!****************************************************************************
 * fbe_provision_drive_usurper_get_key_info
 ******************************************************************************
 *
 * @brief
 *    This function gets DEK information. 
 *    
 *
 * @param   in_packet_p 		  -  pointer to control packet requesting this
 *  								 operation
 * @param   context 			  -  context, which is a pointer to the base config
 *  								 object
 *
 * @return  fbe_status_t		  -  status of this operation
 *
 * @author
 *  02/20/2014  Lili Chen  - Created.
 *
 ******************************************************************************/
static fbe_status_t 
fbe_provision_drive_usurper_get_key_info(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_provision_drive_get_key_info_t * get_key_info = NULL;
    fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;
    fbe_edge_index_t                    server_index;

     /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_provision_drive_get_key_info_t),
                                                    (fbe_payload_control_buffer_t)&get_key_info);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    if(get_key_info->server_index >= FBE_PROVISION_DRIVE_KEY_INFO_TABLE_MAX)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Server Index %d greater than supported%d.\n",
                                __FUNCTION__, get_key_info->server_index, FBE_PROVISION_DRIVE_KEY_INFO_TABLE_MAX);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    server_index = get_key_info->server_index;
    get_key_info->key_mask = 0;
    if (provision_drive_p->key_info_table[server_index].key_handle) {
        get_key_info->key_mask = provision_drive_p->key_info_table[server_index].key_handle->key_mask;
    }
    get_key_info->key1_valid = (provision_drive_p->key_info_table[server_index].mp_key_handle_1 != FBE_INVALID_KEY_HANDLE)? FBE_TRUE : FBE_FALSE;
    get_key_info->key2_valid = (provision_drive_p->key_info_table[server_index].mp_key_handle_2 != FBE_INVALID_KEY_HANDLE)? FBE_TRUE : FBE_FALSE;
    get_key_info->port_object_id = provision_drive_p->key_info_table[server_index].port_object_id;

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}


/*!***************************************************************************
 *          fbe_provision_drive_write_default_paged_metadata_completion()
 *****************************************************************************
 *
 * @brief 
 *  This is the completion for writing default paged metadata.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  03/05/2014  Lili Chen  - Created.
 *
 ******************************************************************************/
fbe_status_t 
fbe_provision_drive_write_default_paged_metadata_completion (fbe_packet_t * packet_p,
                                                             fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *             provision_drive_p = NULL;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;

    provision_drive_p = (fbe_provision_drive_t*)context;

    /* get the packet status. */
    status = fbe_transport_get_status_code (packet_p);

    /* get control status */
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_status(control_operation, &control_status);

    /* verify the status and take appropriate action. */
    if((control_status == FBE_PAYLOAD_CONTROL_STATUS_OK) && (status == FBE_STATUS_OK))
    {
        status = fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const, 
                                        (fbe_base_object_t *)provision_drive_p,
                                        FBE_BASE_CONFIG_LIFECYCLE_COND_ENCRYPTION);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                                  FBE_TRACE_LEVEL_ERROR,  
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                  "%s Couldn't set the condition. status:0x%x\n", 
                                  __FUNCTION__,status);
        }
        fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const,
                                 (fbe_base_object_t*)provision_drive_p,
                                  (fbe_lifecycle_timer_msec_t)0);
    }

    return FBE_STATUS_OK;
}
/*******************************************************************
 * end fbe_provision_drive_write_default_paged_metadata_completion()
 *******************************************************************/

/*!**************************************************************
 * fbe_provision_drive_update_validation_all_completion()
 ****************************************************************
 * @brief
 *  Update the validation mode across all RGs.
 *  This is needed for when we destroy a raid group, we might
 *  need to clean up that our validation area is intact.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return fbe_status_t
 *
 * @author
 *  3/11/2014 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t 
fbe_provision_drive_update_validation_all_completion(fbe_packet_t * packet_p,
                                                     fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *provision_drive_p = (fbe_provision_drive_t*)context;
    fbe_status_t status;
    status = fbe_provision_drive_get_NP_lock(provision_drive_p, packet_p, fbe_provision_drive_encrypt_validate_update_all);
    return status;
}
/******************************************
 * end fbe_provision_drive_update_validation_all_completion()
 ******************************************/

/*!**************************************************************
 * fbe_provision_drive_usurper_quiesce()
 ****************************************************************
 * @brief
 *  This function quiesce the pvd.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  03/14/2014 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t 
fbe_provision_drive_usurper_quiesce(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;

    fbe_provision_drive_lock(provision_drive_p);

    /* Only set the quiesce condition if we are not already quiesced.
     */
    if (!fbe_base_config_is_any_clustered_flag_set((fbe_base_config_t *) provision_drive_p, 
                            (FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED|FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCING)))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s FBE_PROVISION_DRIVE_CONTROL_CODE_QUIESCE received. Quiescing...\n", __FUNCTION__);
        /* this prevents unquiesce from happening automatically */
        fbe_base_config_set_flag((fbe_base_config_t *) provision_drive_p, FBE_BASE_CONFIG_FLAG_USER_INIT_QUIESCE);
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t*)provision_drive_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_QUIESCE);
        fbe_provision_drive_unlock(provision_drive_p);
        /* Schedule monitor.
         */
        status = fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const,
                                          (fbe_base_object_t *) provision_drive_p,
                                          (fbe_lifecycle_timer_msec_t) 0);    /* Immediate reschedule */
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s FBE_PROVISION_DRIVE_CONTROL_CODE_QUIESCE received. Already quiesced or quiescing.  fl:0x%x.\n", 
                              __FUNCTION__, provision_drive_p->flags);
        fbe_provision_drive_unlock(provision_drive_p);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    
    return status;
}
/**************************************
 * end fbe_provision_drive_usurper_quiesce()
 **************************************/

/*!**************************************************************
 * fbe_provision_drive_usurper_unquiesce()
 ****************************************************************
 * @brief
 *  This function unquiesces the pvd.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  03/14/2014 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t
fbe_provision_drive_usurper_unquiesce(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_provision_drive_lock(provision_drive_p);

    /* Only set the unquiesce condition if we are already quiesced.
     */
    if (fbe_base_config_is_clustered_flag_set((fbe_base_config_t *)provision_drive_p, FBE_BASE_CONFIG_CLUSTERED_FLAG_QUIESCED))
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s FBE_PROVISION_DRIVE_CONTROL_CODE_UNQUIESCE received. Un-Quiescing...\n", __FUNCTION__);
        fbe_base_config_clear_flag((fbe_base_config_t *)provision_drive_p, FBE_BASE_CONFIG_FLAG_USER_INIT_QUIESCE);
        fbe_base_config_clear_clustered_flag((fbe_base_config_t *)provision_drive_p, FBE_BASE_CONFIG_METADATA_MEMORY_FLAGS_QUIESCE_HOLD);
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t*)provision_drive_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_UNQUIESCE);
        fbe_provision_drive_unlock(provision_drive_p);

        /* Schedule monitor.
         */
        status = fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const,
                                          (fbe_base_object_t *)provision_drive_p,
                                          (fbe_lifecycle_timer_msec_t) 0);    /* Immediate reschedule */
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s FBE_PROVISION_DRIVE_CONTROL_CODE_UNQUIESCE received. Not quiesced.\n", __FUNCTION__);
        fbe_provision_drive_unlock(provision_drive_p);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}
/**************************************
 * end fbe_provision_drive_usurper_unquiesce()
 **************************************/

/*!**************************************************************
 * fbe_provision_drive_usurper_get_clustered_flags()
 ****************************************************************
 * @brief
 *  This function gets base config clustered flags.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  03/14/2014 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t
fbe_provision_drive_usurper_get_clustered_flags(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_base_config_clustered_flags_t * base_config_clustered_flags;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;

     /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_base_config_clustered_flags_t),
                                                    (fbe_payload_control_buffer_t)&base_config_clustered_flags);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    fbe_base_config_get_clustered_flags((fbe_base_config_t *)provision_drive_p, base_config_clustered_flags);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}
/**************************************
 * end fbe_provision_drive_usurper_get_clustered_flags()
 **************************************/

/*!**************************************************************
 * fbe_provision_drive_usurper_disable_paged_cache()
 ****************************************************************
 * @brief
 *  This function invalidates paged metadata cache.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  05/15/2014 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t
fbe_provision_drive_usurper_disable_paged_cache(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    if (fbe_provision_drive_metadata_cache_is_enabled(provision_drive_p)) {
        fbe_metadata_element_clear_paged_object_cache_flag(&((fbe_base_config_t *)provision_drive_p)->metadata_element);
        fbe_metadata_element_set_paged_object_cache_function(&((fbe_base_config_t *)provision_drive_p)->metadata_element,
                                                             NULL);
        status = fbe_base_config_metadata_paged_clear_cache((fbe_base_config_t *)provision_drive_p);
    }

    if (status == FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    } else {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}
/**************************************
 * end fbe_provision_drive_usurper_disable_paged_cache()
 **************************************/

/*!**************************************************************
 * fbe_provision_drive_usurper_get_paged_cache_info()
 ****************************************************************
 * @brief
 *  This function gets paged metadata cache information.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  05/15/2014 - Created. Lili Chen
 *
 ****************************************************************/
static fbe_status_t
fbe_provision_drive_usurper_get_paged_cache_info(fbe_provision_drive_t * provision_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_provision_drive_get_paged_cache_info_t *get_info;

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_provision_drive_get_paged_cache_info_t),
                                                    (fbe_payload_control_buffer_t)&get_info);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    get_info->miss_count = provision_drive_p->paged_metadata_cache.miss_count;
    get_info->hit_count = provision_drive_p->paged_metadata_cache.hit_count;
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}
/**************************************
 * end fbe_provision_drive_usurper_get_paged_cache_info()
 **************************************/

/*!****************************************************************************
 *          fbe_provision_drive_set_swap_pending_completion()
 ******************************************************************************
 * @brief
 *  We just updated the NP to mark the provision drive logically offline. Now
 *  set the condition to take the provision drive offline (fail).
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/05/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_set_swap_pending_completion(fbe_packet_t *packet_p,
                                                                    fbe_packet_completion_context_t context)
{
    fbe_status_t                        status;
    fbe_provision_drive_t              *provision_drive_p = NULL;
    fbe_payload_ex_t                   *sep_payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_scheduler_hook_status_t         hook_status;

    provision_drive_p = (fbe_provision_drive_t *)context;

    /* This method is called after release the NP lock.
     */
    status = fbe_transport_get_status_code(packet_p);
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* If the request failed or the NP mark offline falg is not set, the request
     * failed.
     */
    if ((status != FBE_STATUS_OK)                                                                  ||
        !fbe_provision_drive_metadata_is_any_np_flag_set(provision_drive_p, 
                                                         FBE_PROVISION_DRIVE_NP_FLAG_SWAP_PENDING_BITS)    ) {
        /* If status is not good then mark the control request as failed.
         */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: mark NP swap pending failed stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
    } else {
        /* No hook status is handled since we can only log here.
         */
        fbe_provision_drive_check_hook(provision_drive_p,
                                       SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SWAP_PENDING, 
                                       FBE_PROVISION_DRIVE_SUBSTATE_SWAP_PENDING_SET, 
                                       0,  &hook_status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    
    /* Return success
     */
    return FBE_STATUS_OK;
}
/************************************************************** 
* end fbe_provision_drive_set_swap_pending_completion()
***************************************************************/

/*!****************************************************************************
 *          fbe_provision_drive_usurper_set_swap_pending()
 ****************************************************************************** 
 * 
 * @brief   Use the drive offline `reason' to determine which provision drive
 *          offline flag to set and then set it.
 *
 * @param   provision_drive_p - Pointer to provision drive object
 * @param   packet_p - Pointer to usurper packet
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  06/05/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_usurper_set_swap_pending(fbe_provision_drive_t *provision_drive_p, 
                                                                       fbe_packet_t *packet_p)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t            *control_operation = NULL;
    fbe_provision_drive_set_swap_pending_t *set_swap_pending_p = NULL;
    fbe_scheduler_hook_status_t                 hook_status;

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_provision_drive_set_swap_pending_t),
                                                    (fbe_payload_control_buffer_t)&set_swap_pending_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Validate the request.
     */
    if ((set_swap_pending_p->set_swap_pending_reason <= FBE_PROVISION_DRIVE_DRIVE_SWAP_PENDING_REASON_NONE) ||
        (set_swap_pending_p->set_swap_pending_reason >= FBE_PROVISION_DRIVE_DRIVE_SWAP_PENDING_REASON_LAST)    ) {
        /* Unsupported reason.
         */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s mark offline reason: %d not supported\n",
                                __FUNCTION__, set_swap_pending_p->set_swap_pending_reason);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* No hook status is handled since we can only log here.
     */
    fbe_provision_drive_check_hook(provision_drive_p,
                                   SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SWAP_PENDING, 
                                   FBE_PROVISION_DRIVE_SUBSTATE_SET_SWAP_PENDING, 
                                   0,  &hook_status);

    /* Set the completion.
     */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_set_swap_pending_completion, provision_drive_p);

    /* Take the NP lock when modifying the NP flags.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    status = fbe_provision_drive_get_NP_lock(provision_drive_p, packet_p, fbe_provision_drive_set_swap_pending);
    return status;
}
/********************************************************* 
* end fbe_provision_drive_usurper_set_swap_pending()
**********************************************************/

/*!****************************************************************************
 *          fbe_provision_drive_clear_swap_pending_completion()
 ******************************************************************************
 * @brief
 *  We just updated the NP to clear the `mark offline' NP flag.  Now set the
 *  condition to take the provision drive back online.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @note    Note no check of other reason the drive should be offline are done
 *          here.  Instead they are done in the verify mark offline clear
 *          condition.
 *
 * @author
 *  06/05/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_clear_swap_pending_completion(fbe_packet_t *packet_p,
                                                                      fbe_packet_completion_context_t context)
{
    fbe_status_t                        status;
    fbe_provision_drive_t              *provision_drive_p = NULL;
    fbe_payload_ex_t                   *sep_payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_scheduler_hook_status_t         hook_status;
    fbe_object_id_t                     object_id;
    fbe_bool_t                          is_system_drive;
    fbe_lifecycle_state_t               lifecycle_state;
    fbe_system_encryption_mode_t	    system_encryption_mode;

    /* Determine if encryption is enabled.
     */
    fbe_database_get_system_encryption_mode(&system_encryption_mode); 

    /* Different behavior for system drives that are Ready
     */
    provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_base_object_get_object_id((fbe_base_object_t *)provision_drive_p, &object_id);
    is_system_drive = fbe_database_is_object_system_pvd(object_id);
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)provision_drive_p, &lifecycle_state);

    /* This method is called after release the NP lock.
     */
    status = fbe_transport_get_status_code(packet_p);
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* If the request failed or the NP mark offline flag is set, the request
     * failed.
     */
    if ((status != FBE_STATUS_OK)                                                                ||
        fbe_provision_drive_metadata_is_any_np_flag_set(provision_drive_p, 
                                                        FBE_PROVISION_DRIVE_NP_FLAG_SWAP_PENDING_BITS)    ) {
        /* If status is not good then mark the control request as failed.
         */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: clear NP swap pending failed stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
    } else {
        /* No hook status is handled since we can only log here.
         */
        fbe_provision_drive_check_hook(provision_drive_p,
                                       SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SWAP_PENDING, 
                                       FBE_PROVISION_DRIVE_SUBSTATE_SWAP_PENDING_CLEARED, 
                                       0,  &hook_status);

        /* If `needs zero' is set then set the conditions.
         */
        if (fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, 
                                                        FBE_PROVISION_DRIVE_NP_FLAG_PAGED_NEEDS_ZERO)) {
            /* If `needs zero' is set (even for system drives) we need to
             * mark the paged as `needs zero'.
             */
            /* we have to force SPs on both side to ACTIVATE */
            fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                           (fbe_base_object_t *) provision_drive_p,
                           FBE_BASE_CONFIG_LIFECYCLE_COND_CLUSTERED_ACTIVATE);        

            fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                           (fbe_base_object_t *) provision_drive_p,
                           FBE_PROVISION_DRIVE_LIFECYCLE_COND_PAGED_METADATA_RECONSTRUCT);
        } else if ((system_encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED) &&
                   (is_system_drive == FBE_TRUE)                                    && 
                   (lifecycle_state == FBE_LIFECYCLE_STATE_READY)                      ) {
            /* Else if we need to reset the paged for this drive, do so now.
             */
            fbe_transport_set_completion_function(packet_p, fbe_provision_drive_write_default_paged_metadata_completion, provision_drive_p);
            fbe_transport_set_completion_function(packet_p, fbe_provision_drive_write_default_paged_metadata_set_np_flag, provision_drive_p);
            return fbe_provision_drive_write_default_system_paged(provision_drive_p, packet_p);
        }
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    
    /* Return success
     */
    return FBE_STATUS_OK;
}
/************************************************************** 
* end fbe_provision_drive_clear_swap_pending_completion()
***************************************************************/

/*!****************************************************************************
 *          fbe_provision_drive_usurper_clear_swap_pending()
 ****************************************************************************** 
 * 
 * @brief   Usurper to clear the `marked offline' NP flags and if allowed take
 *          the provision drive out of the failed state.
 *
 * @param   provision_drive_p - Pointer to provision drive object
 * @param   packet_p - Pointer to usurper packet
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  06/05/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_usurper_clear_swap_pending(fbe_provision_drive_t *provision_drive_p, 
                                                                        fbe_packet_t *packet_p)
{
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_scheduler_hook_status_t hook_status;

    /* No hook status is handled since we can only log here.
     */
    fbe_provision_drive_check_hook(provision_drive_p,
                                   SCHEDULER_MONITOR_STATE_PROVISION_DRIVE_SWAP_PENDING, 
                                   FBE_PROVISION_DRIVE_SUBSTATE_CLEAR_SWAP_PENDING, 
                                   0,  &hook_status);

    /*! @note We do NOT set the condition to put the drive online here since 
     *        we must wait for the NP flag to be set.
     */

    /* Set the completion.
     */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_clear_swap_pending_completion, provision_drive_p);

    /* Take the NP lock when modifying the NP flags.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    status = fbe_provision_drive_get_NP_lock(provision_drive_p, packet_p, fbe_provision_drive_clear_swap_pending);
    return status;
}
/********************************************************* 
* end fbe_provision_drive_usurper_clear_swap_pending()
**********************************************************/

/*!****************************************************************************
 *          fbe_provision_drive_usurper_get_swap_pending()
 ****************************************************************************** 
 * 
 * @brief   Simply set the reason code in the request based on the `mark drive
 *          offline' NP flags.
 *
 * @param   provision_drive_p - Pointer to provision drive object
 * @param   packet_p - Pointer to usurper packet
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  06/05/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_usurper_get_swap_pending(fbe_provision_drive_t *provision_drive_p, 
                                                                       fbe_packet_t *packet_p)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t                       *payload = NULL;
    fbe_payload_control_operation_t        *control_operation = NULL;
    fbe_provision_drive_get_swap_pending_t *get_swap_pending_p = NULL;
    fbe_provision_drive_np_flags_t          current_np_flags;
    fbe_provision_drive_np_flags_t          swap_pending_np_flags;

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_provision_drive_get_swap_pending_t),
                                                    (fbe_payload_control_buffer_t)&get_swap_pending_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Get the existing NP flags.
     */
    status = fbe_provision_drive_metadata_get_np_flag(provision_drive_p, &current_np_flags);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed to get NP flags status: 0x%x\n",
                              __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Based on the NP mark offline flags set the reason.
     */
   swap_pending_np_flags = current_np_flags & FBE_PROVISION_DRIVE_NP_FLAG_SWAP_PENDING_BITS;
    switch (swap_pending_np_flags) {
        case 0:
            get_swap_pending_p->get_swap_pending_reason = FBE_PROVISION_DRIVE_DRIVE_SWAP_PENDING_REASON_NONE;
            break;
        case FBE_PROVISION_DRIVE_NP_FLAG_SWAP_PENDING_PROACTIVE_COPY:
            get_swap_pending_p->get_swap_pending_reason = FBE_PROVISION_DRIVE_DRIVE_SWAP_PENDING_REASON_PROACTIVE_COPY;
            break;
        case FBE_PROVISION_DRIVE_NP_FLAG_SWAP_PENDING_USER_COPY:
            get_swap_pending_p->get_swap_pending_reason = FBE_PROVISION_DRIVE_DRIVE_SWAP_PENDING_REASON_USER_COPY;
            break;
        default:
            /* Unsupported NP flags
             */
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s mark offline NP flags: 0x%x not supported\n",
                                __FUNCTION__,swap_pending_np_flags);
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Complete request.
     */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}
/********************************************************* 
* end fbe_provision_drive_usurper_get_swap_pending()
**********************************************************/

/*!****************************************************************************
 *          fbe_provision_drive_set_eas_start_completion()
 ******************************************************************************
 * @brief
 *  We just updated the NP to set `EAS start' NP flag.  Now set the
 *  condition to put the provision drive in fail state.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @author
 *  07/22/2014  Lili Chen  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_set_eas_start_completion(fbe_packet_t *packet_p,
                                                                 fbe_packet_completion_context_t context)
{
    fbe_status_t                        status;
    fbe_provision_drive_t              *provision_drive_p = NULL;
    fbe_payload_ex_t                   *sep_payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;

    provision_drive_p = (fbe_provision_drive_t *)context;

    /* This method is called after release the NP lock.
     */
    status = fbe_transport_get_status_code(packet_p);
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* If the request failed or the NP mark offline falg is not set, the request
     * failed.
     */
    if ((status != FBE_STATUS_OK) ||
        !fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_NP_FLAG_EAS_STARTED)) {
        /* If status is not good then mark the control request as failed.
         */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: mark NP offline failed stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
    } else {
        /* Set the condition to take the provision drive offline.
         */
        fbe_lifecycle_set_cond(&fbe_provision_drive_lifecycle_const,
                               (fbe_base_object_t *)provision_drive_p,
                               FBE_BASE_CONFIG_LIFECYCLE_COND_DOWNSTREAM_HEALTH_BROKEN);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    
    /* Return success
     */
    return FBE_STATUS_OK;
}
/************************************************************** 
* end fbe_provision_drive_set_eas_start_completion()
***************************************************************/

/*!****************************************************************************
 *          fbe_provision_drive_usurper_set_eas_start()
 ****************************************************************************** 
 * 
 * @brief   Set the drive EAS start.
 *
 * @param   provision_drive_p - Pointer to provision drive object
 * @param   packet_p - Pointer to usurper packet
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  07/22/2014  Lili Chen  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_usurper_set_eas_start(fbe_provision_drive_t *provision_drive_p, 
                                                              fbe_packet_t *packet_p)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t            *control_operation = NULL;

    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,    
                           "%s: set EAS start\n",
                           __FUNCTION__);

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /*! @note We do NOT set the condition to take the drive offline here since 
     *        we must wait for the NP flag to be set.
     */

    /* Set the completion.
     */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_set_eas_start_completion, provision_drive_p);

    /* Take the NP lock when modifying the NP flags.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    status = fbe_provision_drive_get_NP_lock(provision_drive_p, packet_p, fbe_provision_drive_set_eas_start);
    return status;
}
/********************************************************* 
* end fbe_provision_drive_usurper_set_eas_start()
**********************************************************/

/*!****************************************************************************
 *          fbe_provision_drive_set_eas_complete_completion()
 ******************************************************************************
 * @brief
 *  We just updated the NP to set `EAS complete' NP flag.  Now set the
 *  condition to take the provision drive back online.
 *
 * @param packet_p     - Packet requesting operation.
 * @param context      - Packet completion context (provision drive object).
 *
 * @return status - The status of the operation.
 *
 * @note    Note no check of other reason the drive should be offline are done
 *          here.  Instead they are done in the verify mark offline clear
 *          condition.
 *
 * @author
 *  07/22/2014  Lili Chen  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_set_eas_complete_completion(fbe_packet_t *packet_p,
                                                                    fbe_packet_completion_context_t context)
{
    fbe_status_t                        status;
    fbe_provision_drive_t              *provision_drive_p = NULL;
    fbe_payload_ex_t                   *sep_payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;

    provision_drive_p = (fbe_provision_drive_t *)context;

    /* This method is called after release the NP lock.
     */
    status = fbe_transport_get_status_code(packet_p);
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* If the request failed or the NP mark offline flag is set, the request
     * failed.
     */
    if ((status != FBE_STATUS_OK) ||
        !fbe_provision_drive_metadata_is_np_flag_set(provision_drive_p, FBE_PROVISION_DRIVE_NP_FLAG_EAS_COMPLETE)) {
        /* If status is not good then mark the control request as failed.
         */
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,    
                               "%s: set EAS complete failed stat:0x%x, \n",
                               __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
    } else {
        /* Reschedule to take the provision drive out of failed.
         */
        fbe_lifecycle_reschedule(&fbe_provision_drive_lifecycle_const,
                                 (fbe_base_object_t *) provision_drive_p,
                                 (fbe_lifecycle_timer_msec_t) 0);    /* Immediate reschedule */
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    
    /* Return success
     */
    return FBE_STATUS_OK;
}
/************************************************************** 
* end fbe_provision_drive_set_eas_complete_completion()
***************************************************************/

/*!****************************************************************************
 *          fbe_provision_drive_usurper_set_eas_complete()
 ****************************************************************************** 
 * 
 * @brief   Set the drive EAS complete.
 *
 * @param   provision_drive_p - Pointer to provision drive object
 * @param   packet_p - Pointer to usurper packet
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  07/22/2014  Lili Chen  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_usurper_set_eas_complete(fbe_provision_drive_t *provision_drive_p, 
                                                              fbe_packet_t *packet_p)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t            *control_operation = NULL;

    fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,    
                           "%s: set EAS complete\n",
                           __FUNCTION__);

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /*! @note We do NOT set the condition to take the drive offline here since 
     *        we must wait for the NP flag to be set.
     */

    /* Set the completion.
     */
    fbe_transport_set_completion_function(packet_p, fbe_provision_drive_set_eas_complete_completion, provision_drive_p);

    /* Take the NP lock when modifying the NP flags.
     */
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    status = fbe_provision_drive_get_NP_lock(provision_drive_p, packet_p, fbe_provision_drive_set_eas_complete);
    return status;
}
/********************************************************* 
* end fbe_provision_drive_usurper_set_eas_complete()
**********************************************************/

/*!****************************************************************************
 *          fbe_provision_drive_usurper_get_eas_state()
 ****************************************************************************** 
 * 
 * @brief   Get the drive EAS state.
 *
 * @param   provision_drive_p - Pointer to provision drive object
 * @param   packet_p - Pointer to usurper packet
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  07/22/2014  Lili Chen  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_usurper_get_eas_state(fbe_provision_drive_t *provision_drive_p, 
                                                              fbe_packet_t *packet_p)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t            *control_operation = NULL;
    fbe_provision_drive_get_eas_state_t        *get_state_p = NULL;
    fbe_provision_drive_np_flags_t              current_np_flags;

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_provision_drive_get_eas_state_t),
                                                    (fbe_payload_control_buffer_t)&get_state_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Get the existing NP flags.
     */
    status = fbe_provision_drive_metadata_get_np_flag(provision_drive_p, &current_np_flags);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed to get NP flags status: 0x%x\n",
                              __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Based on the NP flags set the state.
     */
    if (current_np_flags & FBE_PROVISION_DRIVE_NP_FLAG_EAS_STARTED) {
        get_state_p->is_started = FBE_TRUE;
    } else {
        get_state_p->is_started = FBE_FALSE;
    }

    if (current_np_flags & FBE_PROVISION_DRIVE_NP_FLAG_EAS_COMPLETE) {
        get_state_p->is_complete = FBE_TRUE;
    } else {
        get_state_p->is_complete = FBE_FALSE;
    }

    /* Complete request.
     */
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}
/********************************************************* 
* end fbe_provision_drive_usurper_get_eas_state()
**********************************************************/

/*!****************************************************************************
 *          fbe_provision_drive_propagate_block_geometry()
 ****************************************************************************** 
 * 
 * @brief   update the edge block geometry.
 *
 * @param   provision_drive_p - Pointer to provision drive object
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  12/24/2014  Jibing Dong  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_propagate_block_geometry(fbe_provision_drive_t *provision_drive_p)
{
    fbe_block_edge_geometry_t ds_geometry;
    fbe_provision_drive_configured_physical_block_size_t configured_physical_block_size;

    /* update the block edge geometry. */
    fbe_provision_drive_get_configured_physical_block_size(provision_drive_p,
                                                           &configured_physical_block_size);

    if (configured_physical_block_size == FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_520)
    {
        ds_geometry = FBE_BLOCK_EDGE_GEOMETRY_520_520; 
    }
    else if (configured_physical_block_size == FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_4160)
    {
        ds_geometry = FBE_BLOCK_EDGE_GEOMETRY_4160_4160;
    }
    else if (configured_physical_block_size == FBE_PROVISION_DRIVE_CONFIGURED_PHYSICAL_BLOCK_SIZE_INVALID)
    {
        return FBE_STATUS_OK;
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *) provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: configured physical block size: %d not supported\n",
                              __FUNCTION__, configured_physical_block_size);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_block_transport_server_set_block_size(&provision_drive_p->base_config.block_transport_server,
                                              ds_geometry);
    return FBE_STATUS_OK;
}
/********************************************************* 
* end fbe_provision_drive_propagate_block_geometry()
**********************************************************/

/*!****************************************************************************
 *          fbe_provision_drive_usurper_get_ssd_statitics()
 ****************************************************************************** 
 * 
 * @brief   Get the SSD drive statistics.
 *
 * @param   provision_drive_p - Pointer to provision drive object
 * @param   packet_p - Pointer to usurper packet
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  10/03/2014  Lili Chen  - Created.
 *
 ******************************************************************************/
fbe_status_t fbe_provision_drive_usurper_get_ssd_statitics(fbe_provision_drive_t *provision_drive_p, 
                                                                  fbe_packet_t *packet_p)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t            *control_operation = NULL;
    fbe_provision_drive_get_ssd_statistics_t   *get_stats_p = NULL;
    fbe_memory_request_t *                      memory_request_p = NULL;
    fbe_memory_request_priority_t               memory_request_priority = 0;

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_provision_drive_get_ssd_statistics_t),
                                                    (fbe_payload_control_buffer_t)&get_stats_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* We need a buffer to hold the log page.
     */
    memory_request_p = fbe_transport_get_memory_request(packet_p);
    memory_request_priority = fbe_transport_get_resource_priority(packet_p);
    memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;

    /* build the memory request for allocation.  We add (1) to the memory
     * request priority of the packet. */
    fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO,
                                   1, /* one chunk should be enough */
                                   memory_request_priority,
                                   fbe_transport_get_io_stamp(packet_p),
                                   (fbe_memory_completion_function_t)fbe_provision_drive_usurper_get_ssd_statitics_allocation_completion,
                                   (fbe_memory_completion_context_t)provision_drive_p);

    fbe_transport_memory_request_set_io_master(memory_request_p,  packet_p);

    fbe_memory_request_entry(memory_request_p);
    /* Completion function is always called.
     */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/********************************************************* 
* end fbe_provision_drive_usurper_get_ssd_statitics()
**********************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_usurper_get_ssd_statitics_allocation_completion()
 ******************************************************************************
 * @brief
 *  This is the completion for allocating memory for the packet to get SSD stats.
 *
 * @param memory_request - Pointer to the memory request.
 * @param context        - Pointer to the provision drive object.
 *
 * @return fbe_status_t.
 * 
 * @author
 *  10/03/2014  Lili Chen  - Created.
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_usurper_get_ssd_statitics_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                                                    fbe_memory_completion_context_t context)
{
    fbe_status_t status;
    fbe_provision_drive_t *provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_packet_t *packet_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_payload_control_operation_t *new_control_operation_p = NULL;
    fbe_memory_header_t * master_memory_header = NULL; 
    fbe_physical_drive_mgmt_get_log_page_t * get_log_page = NULL;
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t  *sg_list = NULL, *sg_list_p;
    fbe_memory_chunk_size_t     chunk_size;

    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE) {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_provision_drive_utils_trace(provision_drive_p, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "pvd get ssd statistics request: 0x%p state: %d allocation failed \n",
                                        memory_request_p, memory_request_p->request_state);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Carve the memory for the log request */
    master_memory_header = fbe_memory_get_first_memory_header(memory_request_p);
    chunk_size = master_memory_header->memory_chunk_size;
    buffer = (fbe_u8_t *)fbe_memory_header_to_data(master_memory_header);
    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list_p = sg_list;
    fbe_sg_element_init(sg_list_p, chunk_size - 520, buffer + 520); /* leave 520 for 3 sg elements and get_log_page */

    get_log_page = (fbe_physical_drive_mgmt_get_log_page_t *)(buffer + 2*sizeof(fbe_sg_element_t));
    fbe_sg_element_increment(&sg_list_p);
    fbe_sg_element_terminate(sg_list_p);
    fbe_payload_ex_set_sg_list(payload_p, sg_list, 0);

#define LOG_PAGE_31   0x31         /* Erase Count Statistics Page */
    /* Fill the log sense request buffer */
    get_log_page->page_code = LOG_PAGE_31;
    get_log_page->transfer_count = chunk_size - 520;

    new_control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);
    if (new_control_operation_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: error alloc null control op\n", __FUNCTION__);

        fbe_memory_free_request_entry(memory_request_p);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_OK;
    }

    /* Build the control packet to get the physical drive inforamtion. */
    fbe_payload_control_build_operation(new_control_operation_p,
                                        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_LOG_PAGES,
                                        get_log_page,
                                        sizeof(fbe_physical_drive_mgmt_get_log_page_t));

    /* increment the control operation and call the direct function to get stats */
    fbe_payload_ex_increment_control_operation_level(payload_p);

    /* Set the completion function. */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_usurper_get_ssd_statitics_completion,
                                          provision_drive_p);

    status = fbe_block_transport_send_control_packet(((fbe_base_config_t *)provision_drive_p)->block_edge_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_provision_drive_usurper_get_ssd_statitics_allocation_completion()
 **************************************/

/*!**************************************************************
 * fbe_provision_drive_usurper_get_ssd_statitics_completion()
 ****************************************************************
 * @brief
 *  Completion for getting SSD log page.
 *
 * @param paket_p -  current usurper op.
 * @param context - the provision drive.             
 *
 * @return -   
 *
 * @author
 *  10/03/2014  Lili Chen  - Created.
 *
 ****************************************************************/
static fbe_status_t
fbe_provision_drive_usurper_get_ssd_statitics_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_status_t status;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_payload_control_operation_t *   prev_control_operation_p = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_memory_request_t *memory_request_p = NULL;
    fbe_provision_drive_get_ssd_statistics_t   *get_stats_p = NULL;
    fbe_physical_drive_mgmt_get_log_page_t * get_log_page = NULL;
    fbe_sg_element_t  *sg_list = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_buffer(control_operation_p, &get_log_page);
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_ex_get_sg_list(payload_p, &sg_list, NULL);

    /* get the previous control operation and associated buffer. */
    prev_control_operation_p = fbe_payload_ex_get_prev_control_operation(payload_p);
    fbe_payload_control_get_buffer(prev_control_operation_p, &get_stats_p);

    /* releaes the buffer and control operation. */
    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);

    memory_request_p = fbe_transport_get_memory_request(packet_p);

    if ((status != FBE_STATUS_OK) || (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "get SSD log page failed, status:%d cntl_status: 0x%x.\n",
                              status, control_status);

        fbe_memory_free_request_entry(memory_request_p);
        fbe_payload_control_set_status(prev_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Parse the log page here */
    fbe_provision_drive_usurper_parse_ssd_statistics(provision_drive_p, sg_list, get_stats_p, get_log_page->transfer_count);

    fbe_memory_free_request_entry(memory_request_p);
    fbe_payload_control_set_status(prev_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
} 
/******************************************
 * end fbe_provision_drive_usurper_get_ssd_statitics_completion()
 ******************************************/

/* Log page parameters header */
#pragma pack(1)
typedef struct fbe_log_param_header_s 
{
    fbe_u16_t   param_code;
    fbe_u8_t    flags;
    fbe_u8_t    length;
} fbe_log_param_header_t;
#pragma pack()

enum fbe_log_param_code_e
{
    FBE_LOG_PARAM_CODE_8000                       =    0x8000,
    FBE_LOG_PARAM_CODE_8001                       =    0x8001,
    FBE_LOG_PARAM_CODE_8002                       =    0x8002,
    FBE_LOG_PARAM_CODE_8003                       =    0x8003,
    FBE_LOG_PARAM_CODE_8FFE                       =    0x8FFE,
    FBE_LOG_PARAM_CODE_8FFF                       =    0x8FFF,
};

/*!**************************************************************
 * fbe_provision_drive_usurper_parse_log_page_31()
 ****************************************************************
 * @brief
 *  This funtion parses SSD log page 31.
 *
 * @param   provision_drive_p - Pointer to provision drive object
 *
 * @return -   
 *
 * @author
 *  10/03/2014  Lili Chen  - Created.
 *
 ****************************************************************/
static void fbe_provision_drive_usurper_parse_log_page_31(fbe_provision_drive_t * provision_drive_p, 
                                                          fbe_u8_t *byte_ptr, 
                                                          fbe_provision_drive_get_ssd_statistics_t * get_stats_p,
                                                          fbe_u16_t transfer_count)
{
    fbe_u16_t i = 0;
    fbe_log_param_header_t *log_header_ptr;
    fbe_u16_t header_size = sizeof(fbe_log_param_header_t);
    fbe_u16_t param_code, channel;
    fbe_u8_t param_length;
    fbe_u64_t avg_erase_count, num_channels = 0, total_avg_erase_count = 0;
    fbe_u64_t max_erase_count = 0, min_erase_count = 0;

    /* Skip 4-bytes header */
    i = 4; 
    byte_ptr += 4;
    while ((i + header_size) < transfer_count) 
    {
        log_header_ptr = (fbe_log_param_header_t *)byte_ptr;
        param_code = csx_ntohs(log_header_ptr->param_code);
        param_length = log_header_ptr->length;

        if (param_code == FBE_LOG_PARAM_CODE_8000) {
            get_stats_p->max_erase_count = csx_ntohll(*(fbe_u64_t *)(byte_ptr + header_size));
        } else if (param_code == FBE_LOG_PARAM_CODE_8FFF) {
            if (param_length == 4) {
                get_stats_p->eol_PE_cycles = csx_ntohl(*(fbe_u32_t *)(byte_ptr + header_size));
            } else if (param_length == 8) {
                get_stats_p->eol_PE_cycles = csx_ntohll(*(fbe_u64_t *)(byte_ptr + header_size));
            }

            if (get_stats_p->eol_PE_cycles == 0)
            {
                get_stats_p->eol_PE_cycles = FBE_PROVISION_DRIVE_DEFAULT_EOL_PE_CYCLES;
            }
            fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, 
                      FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                      "%s param_length 0x%x pe_cycle 0x%llx\n", 
                      __FUNCTION__, param_length, get_stats_p->eol_PE_cycles);
        }  else if (param_code == FBE_LOG_PARAM_CODE_8FFE) {
            if (param_length == 4) {
                get_stats_p->power_on_hours = csx_ntohl(*(fbe_u32_t *)(byte_ptr + header_size));
            } else if (param_length == 8) {
                get_stats_p->power_on_hours = csx_ntohll(*(fbe_u64_t *)(byte_ptr + header_size));
            }

            fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                      FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                      "%s param_length 0x%x power_on_hours 0x%llx\n", 
                      __FUNCTION__, param_length, get_stats_p->power_on_hours);

        } else {
            channel = param_code & 0x0FF0;
            param_code &= ~channel;
            switch (param_code) {
                case FBE_LOG_PARAM_CODE_8001:
                    max_erase_count = csx_ntohll(*(fbe_u64_t *)(byte_ptr + header_size));
                    break;
                case FBE_LOG_PARAM_CODE_8002:   
                    min_erase_count = csx_ntohll(*(fbe_u64_t *)(byte_ptr + header_size));
                    break;
                case FBE_LOG_PARAM_CODE_8003: 
                    avg_erase_count = csx_ntohll(*(fbe_u64_t *)(byte_ptr + header_size));
                    if (avg_erase_count) {
                        total_avg_erase_count += avg_erase_count;
                        num_channels++;
                    }
                    fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "PVD_LOG_P_31 ch 0x%x max 0x%llx min 0x%llx avg 0x%llx\n", 
                              (channel >> 4), max_erase_count, min_erase_count, avg_erase_count);
                    max_erase_count = min_erase_count = avg_erase_count = 0;
                    break;
                default:
                    fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_DEBUG_LOW, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s skipping param 0x%x \n", 
                              __FUNCTION__, (param_code|channel));
                    break;
            }
        }
        i += (header_size + param_length);
        byte_ptr += (header_size + param_length);
    }

    if (num_channels) {
        get_stats_p->average_erase_count = total_avg_erase_count/num_channels;
    }
    return;
}
/******************************************
 * end fbe_provision_drive_usurper_parse_log_page_31()
 ******************************************/

/*!**************************************************************
 * fbe_provision_drive_usurper_parse_ssd_statistics()
 ****************************************************************
 * @brief
 *  This funtion parses SSD log page.
 *
 * @param   provision_drive_p - Pointer to provision drive object
 *
 * @return -   
 *
 * @author
 *  10/03/2014  Lili Chen  - Created.
 *
 ****************************************************************/
static void fbe_provision_drive_usurper_parse_ssd_statistics(fbe_provision_drive_t * provision_drive_p, 
                                                             fbe_sg_element_t * sg_list, 
                                                             fbe_provision_drive_get_ssd_statistics_t * get_stats_p,
                                                             fbe_u16_t transfer_count)
{
    fbe_u8_t *byte_ptr = sg_list[0].address;

    if (transfer_count > sg_list[0].count) {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s transfer count 0x%x too large\n", 
                              __FUNCTION__, transfer_count);
        return;
    }

    if (transfer_count < 8) {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s transfer count 0x%x too small\n", 
                              __FUNCTION__, transfer_count);
        return;
    }

    if (((*byte_ptr) & 0x3F) == 0x31) {
        fbe_provision_drive_usurper_parse_log_page_31(provision_drive_p, byte_ptr, get_stats_p, transfer_count);
    }

    return;
}
/******************************************
 * end fbe_provision_drive_usurper_parse_ssd_statistics()
 ******************************************/

/*!****************************************************************************
 *          fbe_provision_drive_usurper_get_ssd_block_limits()
 ****************************************************************************** 
 * 
 * @brief   Get the SSD drive block limits.
 *
 * @param   provision_drive_p - Pointer to provision drive object
 * @param   packet_p - Pointer to usurper packet
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  10/03/2014  Lili Chen  - Created.
 *  4/20/2015   Deanna Heng - Modified. 
 *
 ******************************************************************************/
static fbe_status_t fbe_provision_drive_usurper_get_ssd_block_limits(fbe_provision_drive_t *provision_drive_p, 
                                                                  fbe_packet_t *packet_p)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t                           *payload = NULL;
    fbe_payload_control_operation_t            *control_operation = NULL;
    fbe_provision_drive_get_ssd_block_limits_t *get_block_limits_p = NULL;
    fbe_memory_request_t *                      memory_request_p = NULL;
    fbe_memory_request_priority_t               memory_request_priority = 0;

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p,
                                                    sizeof(fbe_provision_drive_get_ssd_block_limits_t),
                                                    (fbe_payload_control_buffer_t)&get_block_limits_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* We need a buffer to hold the vpd page.
     */
    memory_request_p = fbe_transport_get_memory_request(packet_p);
    memory_request_priority = fbe_transport_get_resource_priority(packet_p);
    memory_request_priority += FBE_TRANSPORT_RESOURCE_PRIORITY_ADJUSTMENT_FIRST;

    /* build the memory request for allocation.  We add (1) to the memory
     * request priority of the packet. */
    fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_64_BLOCKS_IO,
                                   1, /* one chunk should be enough */
                                   memory_request_priority,
                                   fbe_transport_get_io_stamp(packet_p),
                                   (fbe_memory_completion_function_t)fbe_provision_drive_usurper_get_ssd_block_limits_allocation_completion,
                                   (fbe_memory_completion_context_t)provision_drive_p);

    fbe_transport_memory_request_set_io_master(memory_request_p,  packet_p);

    fbe_memory_request_entry(memory_request_p);
    /* Completion function is always called.
     */
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/********************************************************* 
* end fbe_provision_drive_usurper_get_ssd_block_limits()
**********************************************************/

/*!****************************************************************************
 * @fn fbe_provision_drive_usurper_get_ssd_block_limits_allocation_completion()
 ******************************************************************************
 * @brief
 *  This is the completion for allocating memory for the packet to get SSD block limits.
 *
 * @param memory_request - Pointer to the memory request.
 * @param context        - Pointer to the provision drive object.
 *
 * @return fbe_status_t.
 * 
 * @author
 *  10/03/2014  Lili Chen  - Created.
 *  4/20/2015   Deanna Heng - Modified. 
 *
 ******************************************************************************/
static fbe_status_t
fbe_provision_drive_usurper_get_ssd_block_limits_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                                                    fbe_memory_completion_context_t context)
{
    fbe_status_t status;
    fbe_provision_drive_t *provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_packet_t *packet_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_payload_control_operation_t *new_control_operation_p = NULL;
    fbe_memory_header_t * master_memory_header = NULL; 
    fbe_physical_drive_mgmt_get_log_page_t * get_log_page = NULL;
    fbe_u8_t * buffer = NULL;
    fbe_sg_element_t  *sg_list = NULL, *sg_list_p;
    fbe_memory_chunk_size_t     chunk_size;

    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Check allocation status */
    if (fbe_memory_request_is_allocation_complete(memory_request_p) == FBE_FALSE) {
        /* Currently this callback doesn't expect any aborted requests */
        fbe_provision_drive_utils_trace(provision_drive_p, 
                                        FBE_TRACE_LEVEL_ERROR,
                                        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                        FBE_PROVISION_DRIVE_DEBUG_FLAG_GENERAL_TRACING,
                                        "pvd get ssd block limits request: 0x%p state: %d allocation failed \n",
                                        memory_request_p, memory_request_p->request_state);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Carve the memory for the log request */
    master_memory_header = fbe_memory_get_first_memory_header(memory_request_p);
    chunk_size = master_memory_header->memory_chunk_size;
    buffer = (fbe_u8_t *)fbe_memory_header_to_data(master_memory_header);
    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list_p = sg_list;
    fbe_sg_element_init(sg_list_p, chunk_size - 520, buffer + 520); /* leave 520 for 3 sg elements and get_vpd_page */

    get_log_page = (fbe_physical_drive_mgmt_get_log_page_t *)(buffer + 2*sizeof(fbe_sg_element_t));
    fbe_sg_element_increment(&sg_list_p);
    fbe_sg_element_terminate(sg_list_p);
    fbe_payload_ex_set_sg_list(payload_p, sg_list, 0);

#define VPD_PAGE_B0   0xB0         /* Block Limits Page */
    /* Fill the log sense request buffer */
    get_log_page->page_code = VPD_PAGE_B0;
    get_log_page->transfer_count = chunk_size - 520;

    new_control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);
    if (new_control_operation_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)provision_drive_p, FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: error alloc null control op\n", __FUNCTION__);

        fbe_memory_free_request_entry(memory_request_p);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_OK;
    }

    /* Build the control packet to get the physical drive inforamtion. */
    fbe_payload_control_build_operation(new_control_operation_p,
                                        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_VPD_PAGES,
                                        get_log_page,
                                        sizeof(fbe_physical_drive_mgmt_get_log_page_t));

    /* increment the control operation and call the direct function to get stats */
    fbe_payload_ex_increment_control_operation_level(payload_p);

    /* Set the completion function. */
    fbe_transport_set_completion_function(packet_p,
                                          fbe_provision_drive_usurper_get_ssd_block_limits_completion,
                                          provision_drive_p);

    status = fbe_block_transport_send_control_packet(((fbe_base_config_t *)provision_drive_p)->block_edge_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_provision_drive_usurper_get_ssd_block_limits_allocation_completion()
 **************************************/

/*!**************************************************************
 * fbe_provision_drive_usurper_get_ssd_block_limits_completion()
 ****************************************************************
 * @brief
 *  Completion for getting SSD vpd b0 page.
 *
 * @param paket_p -  current usurper op.
 * @param context - the provision drive.             
 *
 * @return -   
 *
 * @author
 *  10/03/2014  Lili Chen  - Created.
 *  4/20/2015   Deanna Heng - Modified. 
 *
 ****************************************************************/
static fbe_status_t
fbe_provision_drive_usurper_get_ssd_block_limits_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_provision_drive_t *provision_drive_p = (fbe_provision_drive_t *)context;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_status_t status;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_payload_control_operation_t *   prev_control_operation_p = NULL;
    fbe_payload_control_status_t        control_status;
    fbe_memory_request_t *memory_request_p = NULL;
    fbe_provision_drive_get_ssd_block_limits_t   *get_block_limits_p = NULL;
    fbe_physical_drive_mgmt_get_log_page_t * get_log_page = NULL;
    fbe_sg_element_t  *sg_list = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);
    fbe_payload_control_get_buffer(control_operation_p, &get_log_page);
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_ex_get_sg_list(payload_p, &sg_list, NULL);

    /* get the previous control operation and associated buffer. */
    prev_control_operation_p = fbe_payload_ex_get_prev_control_operation(payload_p);
    fbe_payload_control_get_buffer(prev_control_operation_p, &get_block_limits_p);

    /* releaes the buffer and control operation. */
    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);

    memory_request_p = fbe_transport_get_memory_request(packet_p);

    if ((status != FBE_STATUS_OK) || (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "get SSD log page failed, status:%d cntl_status: 0x%x.\n",
                              status, control_status);

        fbe_memory_free_request_entry(memory_request_p);
        fbe_payload_control_set_status(prev_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    /* Parse the log page here */
    fbe_provision_drive_usurper_parse_ssd_block_limits(provision_drive_p, sg_list, get_block_limits_p, get_log_page->transfer_count);

    fbe_memory_free_request_entry(memory_request_p);
    fbe_payload_control_set_status(prev_control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
} 
/******************************************
 * end fbe_provision_drive_usurper_get_ssd_block_limits_completion()
 ******************************************/

/*!**************************************************************
 * fbe_provision_drive_usurper_parse_log_page_31()
 ****************************************************************
 * @brief
 *  This funtion parses SSD vpd b0 page.
 *
 * @param   provision_drive_p - Pointer to provision drive object
 *
 * @return -   
 *
 * @author
 *  10/03/2014  Lili Chen  - Created.
 *  4/20/2015   Deanna Heng - Modified. 
 *
 ****************************************************************/
static void fbe_provision_drive_usurper_parse_vpd_page_b0(fbe_provision_drive_t * provision_drive_p, 
                                                          fbe_u8_t *byte_ptr, 
                                                          fbe_provision_drive_get_ssd_block_limits_t * get_block_limits_p,
                                                          fbe_u16_t transfer_count)
{
    get_block_limits_p->max_transfer_length = csx_ntohl(*(fbe_u32_t *)(byte_ptr + 6));
    get_block_limits_p->max_unmap_lba_count = csx_ntohl(*(fbe_u32_t *)(byte_ptr + 20));
    get_block_limits_p->max_unmap_block_descriptor_count = csx_ntohl(*(fbe_u32_t *)(byte_ptr + 24));
    get_block_limits_p->optimal_unmap_granularity = csx_ntohl(*(fbe_u32_t *)(byte_ptr + 28));
    get_block_limits_p->unmap_granularity_alignment = (csx_ntohl(*(fbe_u32_t *)(byte_ptr + 28))) & 0x7FFFFFFF;
    get_block_limits_p->max_write_same_length = csx_ntohll(*(fbe_u64_t *)(byte_ptr + 36));

    return;
}
/******************************************
 * end fbe_provision_drive_usurper_parse_vpd_page_b0()
 ******************************************/

/*!**************************************************************
 * fbe_provision_drive_usurper_parse_ssd_block_limits()
 ****************************************************************
 * @brief
 *  This funtion parses SSD vpd b0 page.
 *
 * @param   provision_drive_p - Pointer to provision drive object
 *
 * @return -   
 *
 * @author
 *  10/03/2014  Lili Chen  - Created.
 *  4/20/2015   Deanna Heng - Modified. 
 *
 ****************************************************************/
static void fbe_provision_drive_usurper_parse_ssd_block_limits(fbe_provision_drive_t * provision_drive_p, 
                                                             fbe_sg_element_t * sg_list, 
                                                             fbe_provision_drive_get_ssd_block_limits_t * get_block_limits_p,
                                                             fbe_u16_t transfer_count)
{
    fbe_u8_t *byte_ptr = sg_list[0].address;

    if (transfer_count > sg_list[0].count) {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s transfer count 0x%x too large\n", 
                              __FUNCTION__, transfer_count);
        return;
    }

    if (transfer_count < 8) {
        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s transfer count 0x%x too small\n", 
                              __FUNCTION__, transfer_count);
        return;
    }

    
    fbe_provision_drive_usurper_parse_vpd_page_b0(provision_drive_p, byte_ptr, get_block_limits_p, transfer_count);

    return;
}
/******************************************
 * end fbe_provision_drive_usurper_parse_ssd_block_limits()
 ******************************************/

/*!**************************************************************
 * fbe_provision_drive_usurper_set_unmap_enabled_disabled()
 ****************************************************************
 * @brief
 *  enable or disable unmap on the pvd
 *
 * @param   provision_drive_p - Pointer to provision drive object
 * @param   packet_p - Pointer to the packet
 *
 * @return -   
 *
 * @author
 *  4/20/2015   Deanna Heng - Modified. 
 *
 ****************************************************************/
static fbe_status_t fbe_provision_drive_usurper_set_unmap_enabled_disabled(fbe_provision_drive_t * provision_drive_p, 
                                                                           fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_provision_drive_control_set_unmap_enabled_disabled_t * set_unmap_enabled_disabled = NULL;

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    status = fbe_provision_drive_get_control_buffer(provision_drive_p, packet_p, 
                                                    sizeof(fbe_provision_drive_control_set_unmap_enabled_disabled_t),
                                                    (fbe_payload_control_buffer_t *)&set_unmap_enabled_disabled);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)provision_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    if (set_unmap_enabled_disabled->is_enabled == FBE_FALSE)
    {
        fbe_provision_drive_clear_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_UNMAP_ENABLED); 

        if (status == FBE_STATUS_OK) {
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        } else {
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        }

        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s UNMAP clearing unmap enabled\n", 
                                  __FUNCTION__);
    } 
    else if (set_unmap_enabled_disabled->is_enabled == FBE_TRUE)
    {
        fbe_provision_drive_set_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_UNMAP_ENABLED); 
        fbe_provision_drive_set_flag(provision_drive_p, FBE_PROVISION_DRIVE_FLAG_UNMAP_SUPPORTED);

        if (status == FBE_STATUS_OK) {
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        } else {
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        }

        fbe_base_object_trace((fbe_base_object_t*)provision_drive_p, 
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s UNMAP setting unmap enabled\n", 
                                  __FUNCTION__);
    }


    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return status;

}
/******************************************
 * end fbe_provision_drive_usurper_set_unmap_enabled_disabled()
 ******************************************/


/************************************
 * end fbe_provision_drive_usurper.c
 ************************************/
