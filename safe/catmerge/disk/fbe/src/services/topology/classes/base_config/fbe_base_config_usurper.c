/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_base_config_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the base_config object code for the usurper.
 * 
 * @ingroup base_config_class_files
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
#include "fbe/fbe_base_config.h"
#include "fbe_base_config_private.h"
#include "fbe/fbe_winddk.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "fbe_block_transport.h"
#include "fbe_raid_library.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_notification.h"

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/

static fbe_status_t 
fbe_base_config_attach_downstream_block_edge(fbe_base_config_t *base_config_p,
                                             fbe_u32_t index,
                                             fbe_object_id_t object_id);

static fbe_status_t fbe_base_config_get_block_edge_info(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);
static fbe_status_t
fbe_base_config_validate_capacity(fbe_base_config_t * base_config_p, fbe_packet_t * packet);
static fbe_status_t fbe_base_config_get_max_unused_extent_size(fbe_base_config_t * base_config_p, fbe_packet_t * packet);

static fbe_status_t fbe_base_config_control_metadata_paged_change_bits(fbe_base_config_t * base_config, fbe_packet_t * packet);
static fbe_status_t fbe_base_config_control_metadata_paged_get_bits(fbe_base_config_t * base_config, fbe_packet_t * packet);
static fbe_status_t fbe_base_config_control_metadata_paged_clear_cache(fbe_base_config_t * base_config, fbe_packet_t * packet);
static fbe_status_t fbe_base_config_control_metadata_nonpaged_change(fbe_base_config_t * base_config, fbe_packet_t * packet);

static fbe_status_t fbe_base_config_set_edge_tap_hook(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_base_config_remove_edge_tap_hook(fbe_base_config_t *base_config_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_base_config_control_metadata_get_info(fbe_base_config_t * base_config, fbe_packet_t * packet);

static fbe_status_t fbe_base_config_control_metadata_nonpaged_write_persist_NP_lock(fbe_base_config_t * base_config, fbe_packet_t * packet);
static fbe_status_t fbe_base_config_control_metadata_nonpaged_write_persist(fbe_base_config_t *base_config,fbe_packet_t * packet);
static fbe_status_t fbe_base_config_control_metadata_memory_read(fbe_base_config_t * base_config, fbe_packet_t * packet);
static fbe_status_t fbe_base_config_control_metadata_memory_update(fbe_base_config_t * base_config, fbe_packet_t * packet);

static fbe_status_t fbe_base_config_get_upstream_object_list (fbe_base_config_t *base_config_p, fbe_packet_t *packet_p);
static fbe_status_t fbe_base_config_get_downstream_object_list (fbe_base_config_t *base_config_p, fbe_packet_t *packet_p);
static fbe_status_t fbe_base_config_get_upstream_edge_count (fbe_base_config_t *base_config_p, fbe_packet_t *packet_p);
static fbe_status_t fbe_base_config_get_downstream_edge_list (fbe_base_config_t *base_config_p, fbe_packet_t *packet_p);
static fbe_status_t fbe_base_config_can_object_be_removed_when_edge_is_removed(fbe_base_config_t *base_config_p, fbe_packet_t *packet_p);
static fbe_status_t fbe_base_config_control_get_capacity(fbe_base_config_t * base_config, fbe_packet_t * packet);
static fbe_status_t fbe_base_config_control_get_width(fbe_base_config_t * base_config, fbe_packet_t * packet);

static fbe_status_t fbe_base_config_io_control_operation(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_base_config_io_control_operation_completion(fbe_packet_t * packet_p,fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_config_client_is_hibernating(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_base_config_client_is_out_of_hibernation(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_base_config_commit_configuration(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_base_config_usurper_enable_object_power_save (fbe_base_config_t *base_config_p, fbe_packet_t *packet_p);
static fbe_status_t fbe_base_config_usurper_disable_object_power_save (fbe_base_config_t *base_config_p, fbe_packet_t *packet_p);

static fbe_status_t fbe_base_config_send_block_operation(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_base_config_create_sub_packet_and_send_block_operation(fbe_base_object_t*  base_object_p,
                                                                               fbe_packet_t * packet_p);
static fbe_status_t fbe_base_config_send_block_operation_completion(fbe_packet_t* packet_p,
                                                                    fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_config_usurper_memory_allocation_completion(fbe_memory_request_t * memory_request_p, 
        fbe_memory_completion_context_t in_context);

static fbe_status_t fbe_base_config_get_downstream_edge_geometry(fbe_base_config_t *base_config_p, fbe_packet_t *packet);
static fbe_status_t fbe_base_config_usurper_send_event(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_base_config_send_event_completion(fbe_event_t * event_p, fbe_event_completion_context_t context);
static fbe_status_t fbe_base_config_usurper_background_operation(fbe_base_config_t *base_config_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_base_config_usurper_background_operation_completion(fbe_packet_t * packet_p,  fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_config_usurper_background_op_enable_check(fbe_base_config_t *base_config_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_base_config_usurper_disable_cmi(fbe_base_config_t *base_config_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_base_config_usurper_control_system_bg_service(fbe_packet_t * packet_p);
static fbe_status_t fbe_base_config_usurper_get_system_bg_service_status(fbe_packet_t * packet_p);
static fbe_status_t fbe_base_config_control_set_nonpaged_metadata_size(fbe_base_config_t * base_config, fbe_packet_t * packet);
static fbe_status_t fbe_base_config_usurper_set_deny_operation_permission(fbe_base_config_t *base_config_p, fbe_packet_t *packet_p);
static fbe_status_t fbe_base_config_usurper_clear_deny_operation_permission(fbe_base_config_t *base_config_p, fbe_packet_t *packet_p);
static fbe_status_t fbe_base_config_usurper_set_permanent_destroy(fbe_base_config_t *base_config_p, fbe_packet_t *packet_p);

static fbe_status_t base_config_init_write_verify_context(void* base_config_write_verify_context,fbe_base_config_t* base_config);
static fbe_status_t base_config_destroy_write_verify_context(void* base_config_write_verify_context,fbe_base_config_t* base_config);
static fbe_status_t fbe_base_config_write_verify_get_NP_lock_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_config_write_verify_release_NP_lock(fbe_packet_t * packet_p,fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_config_write_verify_non_paged_metadata(fbe_base_config_t *base_config_p, fbe_packet_t * packet_p);
static fbe_status_t base_config_write_verify_non_paged_metadata_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

static fbe_status_t fbe_base_config_control_passive_request(fbe_base_config_t * base_config, fbe_packet_t * packet);
static fbe_status_t fbe_base_config_usurper_get_stripe_blob(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_base_config_usurper_get_peer_lifecycle_state(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_base_config_usurper_get_encryption_state(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_base_config_usurper_set_encryption_state(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_base_config_usurper_get_encryption_mode(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_base_config_usurper_set_encryption_mode(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_base_config_usurper_get_generation_number(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_base_config_usurper_disable_peer_object(fbe_base_config_t *base_config_p, fbe_packet_t * packet_p);

/*!***************************************************************
 * fbe_base_config_control_entry()
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
fbe_base_config_control_entry(fbe_object_handle_t object_handle, 
                              fbe_packet_t * packet_p)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_config_t * base_config_p = NULL;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;
    fbe_payload_control_operation_opcode_t opcode;

    base_config_p = (fbe_base_config_t *)fbe_base_handle_to_pointer(object_handle);

    payload_p = fbe_transport_get_payload_ex(packet_p);

    control_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_opcode(control_p, &opcode);

    switch (opcode)
    {
        /* Configuration service ask to create edge to downstream object */
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_CREATE_EDGE:
            status = fbe_base_config_create_edge(base_config_p, packet_p);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_DESTROY_EDGE:
            status = fbe_base_config_destroy_edge(base_config_p, packet_p);
            break;
            /* Attach an upstream edge to our block transport server. */
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_ATTACH_EDGE:
            status = fbe_base_config_attach_upstream_block_edge(base_config_p, packet_p);
            break;

            /* Detach an upstream edge from our block transport server. */
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_DETACH_EDGE:
            status = fbe_base_config_detach_upstream_block_edge(base_config_p, packet_p);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_EDGE_INFO:
            status = fbe_base_config_get_block_edge_info(base_config_p, packet_p);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_SET_EDGE_TAP_HOOK:
            status = fbe_base_config_set_edge_tap_hook(base_config_p, packet_p);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_REMOVE_EDGE_TAP_HOOK:
            status = fbe_base_config_remove_edge_tap_hook(base_config_p, packet_p);
            break;
            /* validate if given capacity can fit in the servcer. */
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_VALIDATE_CAPACITY:
            status = fbe_base_config_validate_capacity(base_config_p, packet_p);
            break;
            /* get the size of the max extent the server can give. */
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_MAX_UNUSED_EXTENT_SIZE:
            status = fbe_base_config_get_max_unused_extent_size(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_COMMIT_CONFIGURATION:
            /* configuration commited and so we need to allow objects to proceed with. */
            status = fbe_base_config_commit_configuration(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_SET_BITS:
        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_CLEAR_BITS:
            status = fbe_base_config_control_metadata_paged_change_bits(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_CLEAR_CACHE:
            status = fbe_base_config_control_metadata_paged_clear_cache(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_NONPAGED_SET_BITS:
        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_NONPAGED_CLEAR_BITS:
        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_NONPAGED_SET_CHECKPOINT:
        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_NONPAGED_INCR_CHECKPOINT:
            status = fbe_base_config_control_metadata_nonpaged_change(base_config_p, packet_p);
            break;

        case FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST:
            status = fbe_base_config_get_upstream_object_list (base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_OBJECT_LIST:
            status = fbe_base_config_get_downstream_object_list (base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_EDGE_COUNT:
            status = fbe_base_config_get_upstream_edge_count (base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_GET_WIDTH:
            status = fbe_base_config_control_get_width(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_EDGE_LIST:
            status = fbe_base_config_get_downstream_edge_list (base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_GET_CAPACITY:
            status = fbe_base_config_control_get_capacity(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_GET_BITS:
            status = fbe_base_config_control_metadata_paged_get_bits(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_GET_INFO:
            status = fbe_base_config_control_metadata_get_info(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_CAN_OBJECT_BE_REMOVED:
            status = fbe_base_config_can_object_be_removed_when_edge_is_removed(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_IO_CONTROL_OPERATION:
            status = fbe_base_config_io_control_operation(base_config_p, packet_p);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_CLIENT_HIBERNATING:
            status  = fbe_base_config_client_is_hibernating(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_SET_SYSTEM_POWER_SAVING_INFO:
            status  = fbe_base_config_usurper_set_system_power_saving_info(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_GET_SYSTEM_POWER_SAVING_INFO:
            status = fbe_base_config_usurper_get_system_power_saving_info(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_ENABLE_OBJECT_POWER_SAVE:
            status = fbe_base_config_usurper_enable_object_power_save(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_DISABLE_OBJECT_POWER_SAVE:
            status = fbe_base_config_usurper_disable_object_power_save(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_GET_OBJECT_POWER_SAVE_INFO:
            status = fbe_base_config_get_object_power_save_info(base_config_p, packet_p);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_EXIT_HIBERNATION:
            status = fbe_base_config_client_is_out_of_hibernation(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_SET_OBJECT_POWER_SAVE_IDLE_TIME:
            status  = fbe_base_config_set_object_power_save_idle_time(base_config_p, packet_p);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_BLOCK_COMMAND:
		case FBE_BLOCK_TRANSPORT_CONTROL_CODE_IO_COMMAND:
            fbe_transport_set_completion_function(packet_p, fbe_base_config_send_block_operation_completion, base_config_p);
            status = fbe_base_config_create_sub_packet_and_send_block_operation((fbe_base_object_t*)base_config_p, packet_p);
        
            /*status = fbe_base_config_send_block_operation(base_config_p, packet_p);*/
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_NONPAGED_PERSIST:
            status  = fbe_base_config_metadata_nonpaged_persist(base_config_p, packet_p);
            break;

        case FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_EDGE_GEOMETRY:
            status = fbe_base_config_get_downstream_edge_geometry(base_config_p, packet_p);
            break;

        case FBE_BASE_CONFIG_CONTROL_CODE_GET_METADATA_STATISTICS:
            status = fbe_base_config_metadata_get_statistics(base_config_p, packet_p);
            break;

        case FBE_BASE_CONFIG_CONTROL_CODE_SEND_EVENT:
            status = fbe_base_config_usurper_send_event(base_config_p, packet_p);
            break;

        case FBE_BASE_CONFIG_CONTROL_CODE_BACKGROUND_OPERATION:
            status = fbe_base_config_usurper_background_operation(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_BACKGROUND_OP_ENABLE_CHECK:
            status = fbe_base_config_usurper_background_op_enable_check(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_DISABLE_CMI:
            status = fbe_base_config_usurper_disable_cmi(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_CONTROL_SYSTEM_BG_SERVICE:
            status  = fbe_base_config_usurper_control_system_bg_service(packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_GET_SYSTEM_BG_SERVICE_STATUS:
            status  = fbe_base_config_usurper_get_system_bg_service_status(packet_p);
            break;

        case FBE_BASE_CONFIG_CONTROL_CODE_NONPAGED_WRITE_VERIFY:
            status	= fbe_base_config_write_verify_non_paged_metadata(base_config_p,packet_p);
            break;

        case FBE_BASE_CONFIG_CONTROL_CODE_PASSIVE_REQUEST:
            status = fbe_base_config_control_passive_request(base_config_p, packet_p);
            break;

        case FBE_BASE_CONFIG_CONTROL_CODE_SET_NONPAGED_METADATA_SIZE:
            status = fbe_base_config_control_set_nonpaged_metadata_size(base_config_p, packet_p);
            break;

        case FBE_BASE_CONFIG_CONTROL_CODE_SET_DENY_OPERATION_PERMISSION:
            status = fbe_base_config_usurper_set_deny_operation_permission(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_CLEAR_DENY_OPERATION_PERMISSION:
            status = fbe_base_config_usurper_clear_deny_operation_permission(base_config_p, packet_p);
            break;

        case FBE_BASE_CONFIG_CONTROL_CODE_SET_PERMANENT_DESTROY:
            status = fbe_base_config_usurper_set_permanent_destroy(base_config_p, packet_p); 
            break;

        case FBE_BASE_CONFIG_CONTROL_CODE_GET_STRIPE_BLOB:
            status = fbe_base_config_usurper_get_stripe_blob(base_config_p, packet_p); 
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_GET_PEER_LIFECYCLE_STATE:
            status = fbe_base_config_usurper_get_peer_lifecycle_state(base_config_p, packet_p); 
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_NONPAGED_WRITE_PERSIST:
            status = fbe_base_config_control_metadata_nonpaged_write_persist_NP_lock(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_MEMORY_READ:
            status = fbe_base_config_control_metadata_memory_read(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_MEMORY_UPDATE:
            status = fbe_base_config_control_metadata_memory_update(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_GET_ENCRYPTION_STATE:
            status = fbe_base_config_usurper_get_encryption_state (base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_SET_ENCRYPTION_STATE:
            status = fbe_base_config_usurper_set_encryption_state (base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_SET_ENCRYPTION_MODE:
            status = fbe_base_config_usurper_set_encryption_mode(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_GET_ENCRYPTION_MODE:
            status = fbe_base_config_usurper_get_encryption_mode(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_SET_SYSTEM_ENCRYPTION_INFO:
            status  = fbe_base_config_usurper_set_system_encryption_info(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_GET_SYSTEM_ENCRYPTION_INFO:
            status = fbe_base_config_usurper_get_system_encryption_info(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_GET_GENERATION_NUMBER:
            status = fbe_base_config_usurper_get_generation_number(base_config_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_DISABLE_PEER_OBJECT:
            status = fbe_base_config_usurper_disable_peer_object(base_config_p, packet_p);
            break;
        default:
            status = fbe_base_object_control_entry(object_handle, packet_p);
            break;
    }
    return status;
}
/* end fbe_base_config_control_entry() */


/*!****************************************************************************
 * fbe_base_config_write_verify_non_paged_metadata()
 ******************************************************************************
 * @brief
 *   This function performs nonpaged MD rebuild. it writes/verifies object's nonpaged MD to the raw mirror
 *
 *
 * @param base_object_p      - pointer to the base object
 * @param packet                    - control packet
 *
 * @return fbe_lifecycle_status_t   - 
 * @author
 *   04/20/12  - Created. zhangy26
 *
 ******************************************************************************/
static fbe_status_t fbe_base_config_write_verify_non_paged_metadata(fbe_base_config_t *base_config_p,fbe_packet_t * packet_p)
{
    fbe_base_config_write_verify_context_t *	        base_config_write_verify_context = NULL;
    fbe_base_config_nonpaged_metadata_t *               base_config_nonpaged_metadata_p = NULL;
    fbe_status_t										status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *									payload = NULL;
    fbe_payload_control_operation_t *					control_operation = NULL;
    fbe_bool_t is_nonpaged_initialized = FBE_FALSE;	
    fbe_base_config_write_verify_info_t *	            control_write_verify_info = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_lifecycle_state_t   						lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &control_write_verify_info);
    if (control_write_verify_info == NULL) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_base_config_write_verify_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X length invalid\n", len);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_config_get_nonpaged_metadata_ptr(base_config_p, (void **)&base_config_nonpaged_metadata_p);
        
    if(base_config_nonpaged_metadata_p == NULL){ 
        /* The nonpaged pointers are not initialized yet */
        control_write_verify_info->need_clear_rebuild_count = FBE_TRUE;
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
    fbe_base_config_metadata_nonpaged_is_initialized(base_config_p, &is_nonpaged_initialized);
        
    if(is_nonpaged_initialized == FBE_FALSE)
    { 
        /* The nonpaged metadata are not initialized yet */
        control_write_verify_info->need_clear_rebuild_count = FBE_TRUE;			
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
    /* perform write/verify only we are out of SPECIALIZED state.
        If we are in SPECIALIZED state, here is a race condition between
        stripe lock init and write/verify get stripe lock*/
    status = fbe_base_object_get_lifecycle_state((fbe_base_object_t *)base_config_p, &lifecycle_state);
    if(lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE ||
       status != FBE_STATUS_OK)
    {
       control_write_verify_info->write_verify_operation_retryable_b = FBE_TRUE;
       control_write_verify_info->write_verify_op_success_b = FBE_FALSE;
       fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
       fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
       fbe_transport_complete_packet(packet_p);
       return FBE_STATUS_OK;
    }
    /*check the stripe lock is started or not*/
    if(!fbe_base_config_stripe_lock_is_started(base_config_p)){
        control_write_verify_info->write_verify_operation_retryable_b = FBE_TRUE;
        control_write_verify_info->write_verify_op_success_b = FBE_FALSE;
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
    base_config_write_verify_context = (fbe_base_config_write_verify_context_t *)fbe_transport_allocate_buffer();
    if(base_config_write_verify_context == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: FAILED to allocated buffer\n", __FUNCTION__);
        
        /*buffer allocated failed. we should keep the write_verify_count and retry it*/
        control_write_verify_info->write_verify_operation_retryable_b = FBE_TRUE;
        control_write_verify_info->write_verify_op_success_b = FBE_FALSE;
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
    base_config_init_write_verify_context(base_config_write_verify_context,base_config_p);
        
    /*only active side can  do nonpaged MD write verify*/
    if (fbe_base_config_is_active(base_config_p) ){
        fbe_transport_set_completion_function(packet_p, base_config_write_verify_non_paged_metadata_completion, base_config_write_verify_context);
        fbe_transport_set_completion_function(packet_p, fbe_base_config_write_verify_get_NP_lock_completion, base_config_write_verify_context);
        /*get the NP lock first*/
        fbe_base_config_get_np_distributed_lock(base_config_p,packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }else{
         /*clear the weite verify count*/
        control_write_verify_info->need_clear_rebuild_count = FBE_TRUE;		 
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_release_buffer(base_config_write_verify_context);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }


}

static fbe_status_t 
base_config_write_verify_non_paged_metadata_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{
    fbe_base_config_write_verify_context_t *	base_config_write_verify_context = NULL;
    fbe_payload_ex_t *				  payload = NULL;
    fbe_payload_control_operation_t *					control_operation = NULL;
    fbe_base_config_t *base_config_p = NULL;
    fbe_base_config_write_verify_info_t *	control_write_verify_info = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    
    base_config_write_verify_context = (fbe_base_config_write_verify_context_t*)context;
    base_config_p = base_config_write_verify_context->base_config_p;

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);	
    fbe_payload_control_get_buffer(control_operation, &control_write_verify_info);
    if (control_write_verify_info == NULL) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_base_config_write_verify_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X length invalid\n", len);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*copy write verify infomation to control buffer*/
    control_write_verify_info->write_verify_operation_retryable_b = base_config_write_verify_context->write_verify_operation_retryable_b;
    control_write_verify_info->write_verify_op_success_b = base_config_write_verify_context->write_verify_op_success_b;

    base_config_destroy_write_verify_context(base_config_write_verify_context,base_config_write_verify_context->base_config_p);
    fbe_transport_release_buffer(base_config_write_verify_context);
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    return FBE_STATUS_OK;
}

static fbe_status_t 
base_config_init_write_verify_context(void* context,fbe_base_config_t* base_config)
{
    fbe_base_config_write_verify_context_t *	base_config_write_verify_context = NULL;

    if(context == NULL)
    {
     fbe_base_object_trace((fbe_base_object_t*)base_config,
                 FBE_TRACE_LEVEL_ERROR,
                 FBE_TRACE_MESSAGE_ID_INFO,
                 "%s passing context is null\n", __FUNCTION__);
     return FBE_STATUS_GENERIC_FAILURE;
    }
    base_config_write_verify_context = (fbe_base_config_write_verify_context_t*)context;
    base_config_write_verify_context->base_config_p = base_config;
    base_config_write_verify_context->write_verify_operation_retryable_b = FBE_FALSE;
    base_config_write_verify_context->write_verify_op_success_b = FBE_FALSE;
    return FBE_STATUS_OK;
}
static fbe_status_t 
base_config_destroy_write_verify_context(void* context,fbe_base_config_t* base_config)
{
    fbe_base_config_write_verify_context_t *	base_config_write_verify_context = NULL;

    if(context == NULL)
    {
     fbe_base_object_trace((fbe_base_object_t*)base_config,
                 FBE_TRACE_LEVEL_ERROR,
                 FBE_TRACE_MESSAGE_ID_INFO,
                 "%s passing context is null\n", __FUNCTION__);
     return FBE_STATUS_GENERIC_FAILURE;
    }
    base_config_write_verify_context = (fbe_base_config_write_verify_context_t*)context;
    base_config_write_verify_context->base_config_p = NULL;
    base_config_write_verify_context->write_verify_operation_retryable_b = FBE_FALSE;
    base_config_write_verify_context->write_verify_op_success_b = FBE_FALSE;

    return FBE_STATUS_OK;
}

static fbe_status_t 
fbe_base_config_write_verify_get_NP_lock_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_base_config_write_verify_context_t * write_verify_context = (fbe_base_config_write_verify_context_t*)context;
    fbe_status_t status;
   
   /* If the status is not ok, that means we didn't get the 
      lock. the write/verify op is failed and we need retry it
   */
   status = fbe_transport_get_status_code (packet);
   
   if(status != FBE_STATUS_OK)
   {
       fbe_base_object_trace((fbe_base_object_t*)write_verify_context->base_config_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s can't get the NP lock, status: 0x%X", __FUNCTION__, status); 
      write_verify_context->write_verify_op_success_b = FBE_FALSE;
      write_verify_context->write_verify_operation_retryable_b = FBE_TRUE;
      fbe_transport_set_status(packet, status, 0);
      return FBE_STATUS_OK;
   }
   
   fbe_transport_set_completion_function(packet, fbe_base_config_write_verify_release_NP_lock, write_verify_context->base_config_p);      
   fbe_base_config_ex_metadata_nonpaged_write_verify(write_verify_context, packet);
   
   return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
static fbe_status_t fbe_base_config_write_verify_release_NP_lock(fbe_packet_t * packet_p,fbe_packet_completion_context_t context)
{
        
    fbe_base_config_t * base_config_p = (fbe_base_config_t*)context;

    fbe_base_config_release_np_distributed_lock(base_config_p, packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED; 
    
}

/*!**************************************************************
 * fbe_base_config_attach_downstream_block_edge() 
 ****************************************************************
 * @brief
 *  This function attaches the block edge.
 *
 * @param  base_config_p - The base config object.
 * @param  packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  05/21/2009 - Created. RPF
 *
 ****************************************************************/
static fbe_status_t 
fbe_base_config_attach_downstream_block_edge(fbe_base_config_t *base_config_p, fbe_u32_t index, fbe_object_id_t object_id) 
{
    fbe_block_transport_control_attach_edge_t *block_transport_control_attach_edge_p; 
    fbe_packet_t * new_packet_p = NULL;
    fbe_object_id_t my_object_id;
    fbe_u8_t * buffer = NULL;
    fbe_status_t status;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;
    fbe_block_edge_t *edge_p = NULL;
    fbe_package_id_t my_package_id;

    fbe_base_config_get_block_edge_ptr(base_config_p, &edge_p);

    fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s entry\n", __FUNCTION__);

    fbe_block_transport_set_transport_id(edge_p);
    fbe_base_object_get_object_id((fbe_base_object_t *)base_config_p, &my_object_id);
    fbe_block_transport_set_server_id(edge_p, object_id);
    fbe_block_transport_set_client_id(edge_p, my_object_id);
    fbe_block_transport_set_client_index(edge_p, 0);	/* We have only one block edge */

    /* Allocate packet. */
    new_packet_p = fbe_transport_allocate_packet();
    if (new_packet_p == NULL) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "fbe_transport_allocate_packet failed\n");
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Allocate buffer.
     * If this fails, then we will return an error since we cannot continue 
     * without a packet. 
     */
    buffer = fbe_transport_allocate_buffer();
    if (buffer == NULL)  {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_transport_allocate_buffer fail", __FUNCTION__);

        fbe_transport_release_packet(new_packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Get the pointer to the structure we will send to the physical drive. 
     * Fill it in with the edge we are sending. 
     */
    block_transport_control_attach_edge_p = (fbe_block_transport_control_attach_edge_t *)buffer;
    block_transport_control_attach_edge_p->block_edge = edge_p;
    fbe_get_package_id(&my_package_id);
    fbe_block_transport_edge_set_client_package_id(edge_p, my_package_id);


    /* Build the attach edge control packet. */
    fbe_transport_initialize_sep_packet(new_packet_p);
    payload_p = fbe_transport_get_payload_ex(new_packet_p);
    control_p = fbe_payload_ex_allocate_control_operation(payload_p);

    fbe_payload_control_build_operation(control_p,
                                        FBE_BLOCK_TRANSPORT_CONTROL_CODE_ATTACH_EDGE,
                                        block_transport_control_attach_edge_p,
                                        sizeof(fbe_block_transport_control_attach_edge_t));



    /* We are sending control packet, so we have to form address manually. */
    fbe_transport_set_address(new_packet_p,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id);

    fbe_transport_set_sync_completion_type(new_packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* Control packets should be sent via service manager. */
    status = fbe_service_manager_send_control_packet(new_packet_p);

    fbe_transport_wait_completion(new_packet_p);

    status = fbe_transport_get_status_code(new_packet_p);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed status %X\n", 
                                __FUNCTION__, status);
    }

    /* Free the resources we allocated previously.
     */
    fbe_transport_release_buffer(buffer);
    fbe_transport_release_packet(new_packet_p);

    return status;
}
/**************************************
 * end fbe_base_config_attach_downstream_block_edge()
 **************************************/

/*!**************************************************************
 * fbe_base_config_attach_upstream_block_edge()
 ****************************************************************
 * @brief
 *  Attach an upstream edge to our block transport server.
 *
 * @param base_config_p - The base config.
 * @param packet_p - the packet.
 *
 * @return fbe_status_t   
 *
 * @author
 *  5/21/2009 - Created. rfoley
 *
 ****************************************************************/

fbe_status_t 
fbe_base_config_attach_upstream_block_edge(fbe_base_config_t * base_config_p, fbe_packet_t * packet)
{
    fbe_block_transport_control_attach_edge_t * block_transport_control_attach_edge = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_status_t status;
    fbe_block_edge_geometry_t block_edge_geometry;
    fbe_traffic_priority_t  traffic_priority = FBE_TRAFFIC_PRIORITY_INVALID;
    fbe_class_id_t  		class_id = FBE_CLASS_ID_INVALID;
    
    /*! @todo We need to determine how to set this in the future.
     */
    fbe_payload_ex_t * payload_p = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    fbe_base_object_trace(  (fbe_base_object_t*)base_config_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry .\n", __FUNCTION__);

    class_id = fbe_base_object_get_class_id(fbe_base_pointer_to_handle((fbe_base_t *) base_config_p));   

    if(class_id == FBE_CLASS_ID_MIRROR || class_id == FBE_CLASS_ID_STRIPER ||class_id == FBE_CLASS_ID_PARITY)
    {
        block_edge_geometry = FBE_BLOCK_EDGE_GEOMETRY_520_520;
    }
    else
    {
        fbe_base_config_get_downstream_geometry(base_config_p, &block_edge_geometry);
        if(block_edge_geometry == FBE_BLOCK_EDGE_GEOMETRY_INVALID)
        {
            fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s fbe_base_config_get_downstream_geometry fail\n", __FUNCTION__);
        }
    }

    payload_p = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation, &block_transport_control_attach_edge);
    if (block_transport_control_attach_edge == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_block_transport_control_attach_edge_t))
    {
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                                  FBE_TRACE_LEVEL_DEBUG_HIGH,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s attaching block edge to 0x%x\n",
                                  __FUNCTION__,
                                  block_transport_control_attach_edge->block_edge->base_edge.server_id);
    

    /* Right now we do not know all the path state and path attributes rules,
       so we will do something straight forward and may be not exactly correct.
     */
    fbe_block_transport_server_lock(&base_config_p->block_transport_server);

    /* Set the edge geometry. */
    fbe_block_transport_edge_set_geometry(block_transport_control_attach_edge->block_edge, block_edge_geometry);

    /*set the edge priority based on the highet priority below us*/
    traffic_priority = fbe_base_config_get_downstream_edge_traffic_priority(base_config_p);
    fbe_block_transport_edge_set_traffic_priority(block_transport_control_attach_edge->block_edge, traffic_priority);


    if (class_id != FBE_CLASS_ID_EXTENT_POOL) {
        status = fbe_block_transport_server_attach_edge(&base_config_p->block_transport_server, 
                                                        block_transport_control_attach_edge->block_edge,
                                                        &fbe_base_config_lifecycle_const,
                                                        (fbe_base_object_t *)base_config_p );
    } else {
        status = fbe_block_transport_server_attach_preserve_server(&base_config_p->block_transport_server, 
                                                                   block_transport_control_attach_edge->block_edge,
                                                                   &fbe_base_config_lifecycle_const,
                                                                   (fbe_base_object_t *)base_config_p );
    }

    if (status != FBE_STATUS_OK)
    {
        fbe_block_transport_server_unlock(&base_config_p->block_transport_server);
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s error attaching block edge. %x \n",
                              __FUNCTION__,
                              status);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (class_id == FBE_CLASS_ID_PROVISION_DRIVE) {
        /* Make sure the server indexes are correct. 
         * If the order of attach is not what we expect (in order of lba), then we would 
         * need to correct the indexes.  This function will trace if the order is unexpected. 
         */
        fbe_block_transport_correct_server_index(&base_config_p->block_transport_server);
    }

    fbe_block_transport_server_unlock(&base_config_p->block_transport_server);

    if (class_id == FBE_CLASS_ID_LUN) {
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Edge from BVD to LUN ID 0x%X Attached\n",
                              __FUNCTION__,
                              block_transport_control_attach_edge->block_edge->base_edge.server_id);
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_base_config_attach_upstream_block_edge()
 ******************************************/

/*!**************************************************************
 * fbe_base_config_detach_upstream_block_edge()
 ****************************************************************
 * @brief
 *  Detach an upstream edge from our block transport server.
 *
 * @param base_config_p - The base config.
 * @param packet_p - the packet.
 *
 * @return fbe_status_t   
 *
 * @author
 *  5/21/2009 - Created. rfoley
 *
 ****************************************************************/
fbe_status_t 
fbe_base_config_detach_upstream_block_edge(fbe_base_config_t * base_config_p, fbe_packet_t * packet)
{
    fbe_block_transport_control_detach_edge_t * block_transport_control_detach_edge = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_u32_t number_of_clients;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    fbe_base_object_trace(  (fbe_base_object_t*)base_config_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry .\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &block_transport_control_detach_edge); 
    if (block_transport_control_detach_edge == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_block_transport_control_detach_edge_t))
    {
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Right now we do not know all the path state and path attributes rules,
       so we will do something straight forward and may be not exactly correct.
     */
    fbe_block_transport_server_lock(&base_config_p->block_transport_server);
    fbe_block_transport_server_detach_edge(&base_config_p->block_transport_server, 
                                           block_transport_control_detach_edge->block_edge);
    fbe_block_transport_server_unlock(&base_config_p->block_transport_server);

    fbe_block_transport_server_get_number_of_clients(&base_config_p->block_transport_server, &number_of_clients);

    /* It is possible that we waiting for discovery server to become empty.
     * It whould be nice if we can reschedule monitor when we have no clients any more 
     */
    fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                            FBE_TRACE_LEVEL_DEBUG_LOW,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s number_of_clients = %d\n", __FUNCTION__, number_of_clients);

    if (number_of_clients == 0) {
        status = fbe_lifecycle_reschedule(&fbe_base_config_lifecycle_const,
                                          (fbe_base_object_t *) base_config_p,
                                          (fbe_lifecycle_timer_msec_t) 0);    /* Immediate reschedule */
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_base_config_detach_upstream_block_edge()
 ******************************************/

/*!**************************************************************
 * fbe_base_config_create_edge()
 ****************************************************************
 * @brief
 *  This function sets up the basic edge configuration info
 *  for this base config object.
 *  We also attempt to attach the edge.
 *  If the edge cannot be attached or the edge info is invalid
 *  then we will fail.
 *
 * @param base_config_p - The base config.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/10/2009 - Created. Peter Puhov
 *
 ****************************************************************/
fbe_status_t
fbe_base_config_create_edge(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_block_transport_control_create_edge_t * create_edge_p = NULL;    /* INPUT */
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;  
    fbe_u32_t width;	
    fbe_block_edge_t *edge_p = NULL;
    fbe_object_id_t my_object_id;
    fbe_block_transport_control_attach_edge_t attach_edge;
    fbe_package_id_t my_package_id;
    fbe_base_config_path_state_counters_t path_state_counters;
    fbe_u64_t	lowest_latency_time_in_seconds = 0;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);  

    fbe_payload_control_get_buffer(control_operation_p, &create_edge_p);
    if (create_edge_p == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Do some basic validation on the edge info received. */
    fbe_base_config_get_width(base_config_p, &width);

    if (create_edge_p->client_index >= width) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s edge index invalid %d\n", __FUNCTION__ , create_edge_p->client_index);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Now attach the edge. */
    fbe_base_object_get_object_id((fbe_base_object_t *)base_config_p, &my_object_id);

    fbe_base_config_get_block_edge(base_config_p, &edge_p, create_edge_p->client_index);
    fbe_block_transport_set_transport_id(edge_p);
    fbe_block_transport_set_server_id(edge_p, create_edge_p->server_id);
    fbe_block_transport_set_client_id(edge_p, my_object_id);
    fbe_block_transport_set_client_index(edge_p, create_edge_p->client_index);
    /*!@todo server index needs to set with appropriate values if different client uses differnt server lba range. */
    fbe_block_transport_set_server_index(edge_p, 0);
    fbe_block_transport_edge_set_capacity(edge_p, create_edge_p->capacity);
    fbe_block_transport_edge_set_offset(edge_p, create_edge_p->offset);

    if ((create_edge_p->edge_flags & FBE_EDGE_FLAG_IGNORE_OFFSET) == FBE_EDGE_FLAG_IGNORE_OFFSET) 
    {
        fbe_base_transport_set_path_attributes((fbe_base_edge_t*)edge_p, FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IGNORE_OFFSET);
    }
    
    /*first of all we need to check what is the lowest latency time on the edges above us, the lowest one wins*/
    fbe_block_transport_server_get_lowest_ready_latency_time(&base_config_p->block_transport_server,
                                                             &lowest_latency_time_in_seconds);
    if (lowest_latency_time_in_seconds == 0) {
        lowest_latency_time_in_seconds = FBE_LUN_MAX_LATENCY_TIME_DEFAULT;
    }
    fbe_block_transport_edge_set_time_to_become_ready(edge_p, lowest_latency_time_in_seconds);

    attach_edge.block_edge = edge_p;

    fbe_get_package_id(&my_package_id);
    fbe_block_transport_edge_set_client_package_id(edge_p, my_package_id);

    status = fbe_base_config_send_attach_edge(base_config_p, packet_p, &attach_edge) ;
    if(status != FBE_STATUS_OK){
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_base_config_attach_downstream_block_edge failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If all of our edges are attached, kick off the monitor. */
    fbe_base_config_get_path_state_counters(base_config_p, &path_state_counters);
    if(!path_state_counters.invalid_counter)
    {
        status = fbe_lifecycle_reschedule(&fbe_base_config_lifecycle_const,
                                            (fbe_base_object_t *) base_config_p,
                                            (fbe_lifecycle_timer_msec_t) 0);	/* Immediate reschedule */
    }

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_base_config_create_edge()
 **************************************/

/*!**************************************************************
 * fbe_base_config_send_attach_edge() 
 ****************************************************************
 * @brief
 *  This function attaches the block edge.
 *
 * @param  base_config_p - The base config object.
 * @param  packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/10/2009 - Created. Peter Puhov
 *
 ****************************************************************/
fbe_status_t 
fbe_base_config_send_attach_edge(fbe_base_config_t *base_config_p,
                                 fbe_packet_t * packet_p,
                                 fbe_block_transport_control_attach_edge_t * attach_edge) 
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t *payload_p = NULL;
    fbe_payload_control_operation_t *control_p = NULL;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /* Allocate control operation. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_allocate_control_operation(payload_p);

    fbe_payload_control_build_operation(control_p,
                                        FBE_BLOCK_TRANSPORT_CONTROL_CODE_ATTACH_EDGE,
                                        attach_edge,
                                        sizeof(fbe_block_transport_control_attach_edge_t));

    /* We are sending control packet, so we have to form address manually. */
    fbe_transport_set_address(packet_p,
                              FBE_PACKAGE_ID_SEP_0,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              attach_edge->block_edge->base_edge.server_id);

    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* Control packets should be sent via service manager. */
    status = fbe_service_manager_send_control_packet(packet_p);

    fbe_transport_wait_completion(packet_p);

    status = fbe_transport_get_status_code(packet_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed status %X\n", 
                                __FUNCTION__, status);

        fbe_payload_ex_release_control_operation(payload_p, control_p);
        return status;
    }

    /* Free the resources we allocated previously.*/
    fbe_payload_ex_release_control_operation(payload_p, control_p);

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_base_config_send_attach_edge()
 **************************************/

/*!**************************************************************
 * fbe_base_config_destroy_edge()
 ****************************************************************
 * @brief
 *  This function is used to destroy the configured edge.
 *
 * @param base_config_p - The base config.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/10/2009 - Created. Dhaval Patel
 *
 ****************************************************************/
fbe_status_t
fbe_base_config_destroy_edge(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_block_transport_control_destroy_edge_t * destroy_edge_p = NULL;    /* INPUT */
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;  
    fbe_u32_t width;	
    fbe_block_edge_t *edge_p = NULL;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);  

    fbe_payload_control_get_buffer(control_operation_p, &destroy_edge_p);
    if (destroy_edge_p == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Do some basic validation on the edge info received. */
    fbe_base_config_get_width(base_config_p, &width);

    if (destroy_edge_p->client_index >= width) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s edge index invalid %d\n", __FUNCTION__ , destroy_edge_p->client_index);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the block edge pointer for the edge which is asked to detach.
     */
    fbe_base_config_get_block_edge(base_config_p, &edge_p, destroy_edge_p->client_index);

    /* Now detach an specific edge which is asked by configuration
     * service.
     */
    status = fbe_base_config_detach_edge (base_config_p, packet_p, edge_p);
    if(status != FBE_STATUS_OK){
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_base_config_detach_downstream_block_edge failed\n", __FUNCTION__);
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
/**************************************
 * end fbe_base_config_destroy_edge()
 **************************************/

/*!**************************************************************
 * fbe_base_config_get_block_edge_info()
 ****************************************************************
 * @brief
 *  This function returns the edge info for block edge.
 *
 * @param  base_config_p - The base config object.
 * @param  packet_p - The packet requesting this operation.

 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/22/2008 - Created. Dhaval Patel.
 *
 ****************************************************************/
static fbe_status_t
fbe_base_config_get_block_edge_info(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_block_transport_control_get_edge_info_t * get_edge_info = NULL;    /* INPUT */
    fbe_status_t status;
    fbe_block_edge_t *block_edge_p;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_u32_t width;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &get_edge_info);
    if (get_edge_info == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If edge index is invalid then fail the usurper command.
     */
    if ((get_edge_info->base_edge_info.client_index == FBE_EDGE_INDEX_INVALID))
    {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid edge index passed in.\n",
                                __FUNCTION__);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;    
    }
    /* If edge index is invalid then fail the usurper command.
     */
    if ((get_edge_info->base_edge_info.client_index == FBE_EDGE_INDEX_INVALID))
    {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid edge index passed in.\n",
                                __FUNCTION__);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;    
    }

    /* Number of edges never exceed the width of the object
     * If edge index is beyond the width then fail the usurper command.
     */
    fbe_base_config_get_width(base_config_p, &width);
    if ((get_edge_info->base_edge_info.client_index >= width))
    {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s edge index %d >= the object width, should be < %d\n",
                                __FUNCTION__, 
                                get_edge_info->base_edge_info.client_index,
                                width);

        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;    
    }

    status = fbe_base_config_get_block_edge(base_config_p, &block_edge_p, get_edge_info->base_edge_info.client_index);
    if ((status != FBE_STATUS_OK))
    {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid block edge.\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;    
    }

    /* Copy required block edge information.
     */
    status = fbe_block_transport_get_client_id(block_edge_p, &get_edge_info->base_edge_info.client_id);
    status = fbe_block_transport_get_server_id(block_edge_p, &get_edge_info->base_edge_info.server_id);

    status = fbe_block_transport_get_client_index(block_edge_p, &get_edge_info->base_edge_info.client_index);
    status = fbe_block_transport_get_server_index(block_edge_p, &get_edge_info->base_edge_info.server_index);

    status = fbe_block_transport_get_path_state(block_edge_p, &get_edge_info->base_edge_info.path_state);
    status = fbe_block_transport_get_path_attributes(block_edge_p, &get_edge_info->base_edge_info.path_attr);
    
    fbe_block_transport_edge_get_exported_block_size(block_edge_p, &get_edge_info->exported_block_size);
    fbe_block_transport_get_physical_block_size(block_edge_p->block_edge_geometry, &get_edge_info->imported_block_size);
    fbe_block_transport_edge_get_capacity(block_edge_p, &get_edge_info->capacity);
    get_edge_info->offset = fbe_block_transport_edge_get_offset(block_edge_p);
    get_edge_info->base_edge_info.transport_id = FBE_TRANSPORT_ID_BLOCK;
    get_edge_info->edge_p = (fbe_u64_t) block_edge_p;

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_base_config_get_block_edge_info()
*********************************************/
/*!**************************************************************
 * fbe_base_config_set_edge_tap_hook()
 ****************************************************************
 * @brief
 *  This function sets the edge tap hook function for our block edge.
 *
 * @param base_config_p - Our object to set hook for.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @revision
 *  12/22/2009 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t
fbe_base_config_set_edge_tap_hook(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_transport_control_set_edge_tap_hook_t * hook_info_p = NULL;    /* INPUT */
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL; 
    fbe_block_edge_t *block_edge_p = NULL;
    fbe_u32_t width;
    fbe_u32_t index;
    fbe_u32_t start_index;
    fbe_u32_t max_index;

    fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &hook_info_p);
    if (hook_info_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s hook_info_p is NULL\n",__FUNCTION__);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_base_config_get_width(base_config_p, &width);
    
    if (hook_info_p->edge_index == FBE_U32_MAX)
    {
        /* This means set the edge hook based on the edge mask.
         */
        max_index = width;
        start_index = 0;
    }
    /* User specified a specific edge index to set. 
     * Validate the edge is valid. 
     */
    else if (hook_info_p->edge_index >= width)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s edge index: %d is greater or equal width: %d\n", 
                              __FUNCTION__, hook_info_p->edge_index, width);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        /* Just set the hook for this edge.
         */
        start_index = hook_info_p->edge_index;
        max_index = hook_info_p->edge_index + 1;
    }

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s start index: %d last plus 1: %d \n", 
                          __FUNCTION__, start_index, max_index);

    /* Loop over all the edge indexes we should set for.
     */
    for ( index = start_index; index < max_index; index++)
    {
        /* Skip edges that aren't enabled.
         */
        if ((hook_info_p->edge_index == FBE_U32_MAX)          &&
            (((1 << index) & hook_info_p->edge_bitmask) == 0)    )
        {
            /* This is atypical so trace some information.
             */
            fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s Skipping index: %d width: %d bitmask: 0x%x\n", 
                          __FUNCTION__, index, width, hook_info_p->edge_bitmask);
            continue;
        }

        /* Fetch the edge for the index the user requested.
         */
        status = fbe_base_config_get_block_edge(base_config_p, &block_edge_p, index);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "base_cfg_set_edge_tap_hook Invalid block edge.\n");
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;    
        }
        /* Try to set the hook into the edge.
         */
        status = fbe_block_transport_edge_set_hook(block_edge_p, hook_info_p->edge_tap_hook);
        if (status != FBE_STATUS_OK) 
        { 
            fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "base_cfg_set_edge_tap_hook unable to set block edge hook for index:%d\n",
                                    index);
            break; 
        }
    }
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_base_config_set_edge_tap_hook()
 **************************************/
/*!**************************************************************
 * fbe_base_config_remove_edge_tap_hook()
 ****************************************************************
 * @brief
 *  This function removes the block edge tap hoook
 *
 * @param base_config_p - Our object to set hook for.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @revision
 *  09/07/2012  Ron Proulx  - Created.
 *
 ****************************************************************/
static fbe_status_t
fbe_base_config_remove_edge_tap_hook(fbe_base_config_t *base_config_p, fbe_packet_t * packet_p)
{
    fbe_transport_control_remove_edge_tap_hook_t *remove_hook_info_p = NULL;    /* INPUT */
    fbe_status_t status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL; 
    fbe_block_edge_t *block_edge_p = NULL;
    fbe_u32_t width;
    fbe_u32_t index;
    fbe_u32_t start_index;
    fbe_u32_t max_index;

    fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &remove_hook_info_p);
    if (remove_hook_info_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s hook_info_p is NULL\n",__FUNCTION__);

        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_base_config_get_width(base_config_p, &width);

    /* This means set the edge hook based on the edge mask.
     */
    max_index = width;
    start_index = 0;

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s start index: %d last plus 1: %d \n", 
                          __FUNCTION__, start_index, max_index);

    /* Just remove the first index
     */
    index = start_index;

    /* Fetch the edge for the index the user requested.
     */
    status = fbe_base_config_get_block_edge(base_config_p, &block_edge_p, index);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "base_cfg_set_edge_tap_hook Invalid block edge.\n");
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;    
    }

    /* Try to remove the hook into the edge.
     */
    status = fbe_block_transport_edge_remove_hook(block_edge_p, packet_p);
    if (status != FBE_STATUS_OK) 
    { 
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "base_cfg_remove_edge_tap_hook unable to removeblock edge hook for index:%d\n",
                                index);
    }

    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/*******************************************
 * end fbe_base_config_remove_edge_tap_hook()
 ********************************************/
/*!**************************************************************
 * fbe_base_config_validate_capacity()
 ****************************************************************
 * @brief
 *  This function validates the required capacity can be supported by the object.
 *  If so, the payload status is set to ok and a valid client_index and offset is 
 *  returned. 
 *
 * @param base_config_p - Our server object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @revision
 *  02/08/2010 - Created. guov
 *
 ****************************************************************/
static fbe_status_t
fbe_base_config_validate_capacity(fbe_base_config_t * base_config_p, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_ex_t * payload_p = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    fbe_block_transport_control_validate_capacity_t * block_transport_control_validate_capacity = NULL;
    fbe_payload_control_buffer_length_t len = 0;

    fbe_base_object_trace(  (fbe_base_object_t*)base_config_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry .\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation, &block_transport_control_validate_capacity);
    if (block_transport_control_validate_capacity == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_block_transport_control_validate_capacity_t))
    {
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_block_transport_server_lock(&base_config_p->block_transport_server);

    status = fbe_block_transport_server_validate_capacity(&base_config_p->block_transport_server, 
                                                          &block_transport_control_validate_capacity->capacity,
                                                          block_transport_control_validate_capacity->placement,
                                                          block_transport_control_validate_capacity->b_ignore_offset,
                                                          &block_transport_control_validate_capacity->client_index,
                                                          &block_transport_control_validate_capacity->block_offset);
    if (status != FBE_STATUS_OK)
    {
        fbe_block_transport_server_unlock(&base_config_p->block_transport_server);
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                              "%s validate capacity failed. %x \n",
                              __FUNCTION__,
                              status);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }
    fbe_block_transport_server_unlock(&base_config_p->block_transport_server);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;

}

/*!**************************************************************
 * fbe_base_config_get_max_unused_extent_size()
 ****************************************************************
 * @brief
 *  This function returns the size of the max unused extent of the server.
 *  The server may have multiple unused extents.  This function returns the
 *  size of the biggest one.
 *
 * @param base_config_p - Our server object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @revision
 *  02/08/2010 - Created. guov
 *
 ****************************************************************/
static fbe_status_t
fbe_base_config_get_max_unused_extent_size(fbe_base_config_t * base_config_p, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_ex_t * payload_p = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    fbe_block_transport_control_get_max_unused_extent_size_t * block_transport_control_get_max_unused_extent_size = NULL;
    fbe_payload_control_buffer_length_t len = 0;

    fbe_base_object_trace(  (fbe_base_object_t*)base_config_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry .\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation, &block_transport_control_get_max_unused_extent_size);
    if (block_transport_control_get_max_unused_extent_size == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "fbe_payload_control_get_buffer fail\n");

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_block_transport_control_get_max_unused_extent_size_t))
    {
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X len invalid\n", len);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_block_transport_server_lock(&base_config_p->block_transport_server);

    status = fbe_block_transport_server_get_max_unused_extent_size(&base_config_p->block_transport_server, 
                                                                   &block_transport_control_get_max_unused_extent_size->extent_size);
    if (status != FBE_STATUS_OK)
    {
        fbe_block_transport_server_unlock(&base_config_p->block_transport_server);
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                              "%s get max unused extent size failed. %x \n",
                              __FUNCTION__,
                              status);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }
    fbe_block_transport_server_unlock(&base_config_p->block_transport_server);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;

}

/*!**************************************************************
 * fbe_base_config_can_object_be_removed_when_edge_is_removed()
 ****************************************************************
 * @brief
 * This function is used to determine if given object can be removed
 * if upstream edge is removed
 * 
 * @param base_config_p - The object pointer.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  03/22/2010 - Created. Jesus Medina
 *
 ****************************************************************/
static fbe_status_t 
fbe_base_config_can_object_be_removed_when_edge_is_removed(fbe_base_config_t *base_config_p, fbe_packet_t *packet_p)
{
    fbe_status_t								status = FBE_STATUS_OK;
    fbe_payload_ex_t * 						payload_p = NULL;
    fbe_payload_control_operation_t *   		control_operation_p = NULL;
    fbe_base_config_can_object_be_removed_t 	*can_object_be_removed_p = NULL;
    fbe_payload_control_buffer_length_t 		length = 0;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
            FBE_TRACE_LEVEL_DEBUG_HIGH,
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    if (payload_p == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s fbe_payload_ex_get_control_operation failed\n",
                __FUNCTION__);

        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    if (control_operation_p == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s fbe_payload_ex_get_control_operation failed\n",
                __FUNCTION__);

        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Retrieve the payload buffer.
    */
    fbe_payload_control_get_buffer(control_operation_p, &can_object_be_removed_p);
    if (can_object_be_removed_p == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* If length of the buffer does not match with base config upstream object
     * list then return error.
     */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if(length != sizeof(fbe_base_config_can_object_be_removed_t))
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s payload buffer length mismatch.\n",
                __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

 
    fbe_block_transport_server_lock(&base_config_p->block_transport_server);

    status = fbe_block_transport_server_can_object_be_removed_when_edge_is_removed(&base_config_p->block_transport_server,
                                                                   can_object_be_removed_p->object_id, 
                                                                   &can_object_be_removed_p->object_to_remove);
    if (status != FBE_STATUS_OK)
    {
        fbe_block_transport_server_unlock(&base_config_p->block_transport_server);
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                              "%s call to determine if edge can be removed failed, status %x \n",
                              __FUNCTION__,
                              status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
    fbe_block_transport_server_unlock(&base_config_p->block_transport_server);

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/****************************************************************
 * end fbe_base_config_can_object_be_removed_when_edge_is_removed
 ****************************************************************/


static fbe_status_t
fbe_base_config_control_get_capacity(fbe_base_config_t * base_config, fbe_packet_t * packet)
{
    fbe_base_config_control_get_capacity_t * get_exported_capacity_p = NULL;	/* INPUT */
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t len = 0;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &get_exported_capacity_p);
    if (get_exported_capacity_p == NULL) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_base_config_control_get_capacity_t))
    {
        fbe_base_object_trace((fbe_base_object_t *) base_config, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X length invalid\n", len);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_config_get_capacity(base_config, &(get_exported_capacity_p->capacity));
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t
fbe_base_config_control_metadata_paged_change_bits(fbe_base_config_t * base_config, fbe_packet_t * packet)
{
    fbe_base_config_control_metadata_paged_change_bits_t * nonpaged_change = NULL;    /* INPUT */
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_operation_opcode_t opcode;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &nonpaged_change);
    if (nonpaged_change == NULL) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_opcode(control_operation, &opcode);

    if(opcode == FBE_BASE_CONFIG_CONTROL_CODE_METADATA_PAGED_SET_BITS){
        status = fbe_base_config_metadata_paged_set_bits(base_config,
                                                        packet,
                                                        nonpaged_change->metadata_offset,
                                                        nonpaged_change->metadata_record_data,
                                                        nonpaged_change->metadata_record_data_size,
                                                        nonpaged_change->metadata_repeat_count);
    } else {
        status = fbe_base_config_metadata_paged_clear_bits(base_config,
                                                        packet,
                                                        nonpaged_change->metadata_offset,
                                                        nonpaged_change->metadata_record_data,
                                                        nonpaged_change->metadata_record_data_size,
                                                        nonpaged_change->metadata_repeat_count);

    }

    return status;
}

static fbe_status_t
fbe_base_config_control_metadata_paged_clear_cache(fbe_base_config_t * base_config, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    status = fbe_base_config_metadata_paged_clear_cache(base_config);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return status;
}

static fbe_status_t
fbe_base_config_control_metadata_nonpaged_change(fbe_base_config_t * base_config, fbe_packet_t * packet)
{
    fbe_base_config_control_metadata_nonpaged_change_t * nonpaged_change = NULL;	/* INPUT */
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_operation_opcode_t opcode;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &nonpaged_change);
    if (nonpaged_change == NULL) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_opcode(control_operation, &opcode);

    switch(opcode){
        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_NONPAGED_SET_BITS:
        status = fbe_base_config_metadata_nonpaged_set_bits(base_config,
                                                        packet,
                                                            nonpaged_change->metadata_offset,
                                                            nonpaged_change->metadata_record_data,
                                                            nonpaged_change->metadata_record_data_size,
                                                            nonpaged_change->metadata_repeat_count);
            break;

        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_NONPAGED_CLEAR_BITS:
        status = fbe_base_config_metadata_nonpaged_clear_bits(base_config,
                                                        packet,
                                                            nonpaged_change->metadata_offset,
                                                            nonpaged_change->metadata_record_data,
                                                            nonpaged_change->metadata_record_data_size,
                                                            nonpaged_change->metadata_repeat_count);
            break;

        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_NONPAGED_SET_CHECKPOINT:
            status = fbe_base_config_metadata_nonpaged_set_checkpoint(base_config,
                                                                      packet,
                                                                      nonpaged_change->metadata_offset,
                                                                      nonpaged_change->second_metadata_offset,  
                                                                      *(fbe_u64_t*)nonpaged_change->metadata_record_data);
            break;

        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_NONPAGED_INCR_CHECKPOINT:
            status = fbe_base_config_metadata_nonpaged_incr_checkpoint(base_config,
                                                                        packet,
                                                                        nonpaged_change->metadata_offset,
                                                                        nonpaged_change->second_metadata_offset,
                                                                        *(fbe_u64_t*)nonpaged_change->metadata_record_data,
                                                                        nonpaged_change->metadata_repeat_count);
            break;
    }

    return status;
}

static fbe_status_t
fbe_base_config_control_metadata_paged_get_bits(fbe_base_config_t * base_config, fbe_packet_t * packet)
{
    fbe_base_config_control_metadata_paged_get_bits_t * get_bits = NULL;	/* INPUT */
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_operation_opcode_t opcode;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &get_bits);
    if (get_bits == NULL) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_opcode(control_operation, &opcode);

    status = fbe_base_config_metadata_paged_get_bits(base_config,
                                                     packet,
                                                     get_bits->metadata_offset,
                                                     get_bits->metadata_record_data,
                                                     get_bits->metadata_record_data_size,
                                                     get_bits->get_bits_flags);

    return status;
}



/*!**************************************************************
 * fbe_base_config_control_metadata_get_info() 
 ****************************************************************
 * @brief
 *  Get the nonpaged metadata info of object
 *
 * @param  base_config_p - The base config object.
 * @param  packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  05/22/2012 - Modified. Jingcheng Zhang to check nonpaged metadata initialized
 *                         and return version info        
 *
 ****************************************************************/
static fbe_status_t fbe_base_config_control_metadata_get_info(fbe_base_config_t * base_config, fbe_packet_t * packet)
{
    fbe_base_config_control_metadata_get_info_t *   get_metadata_info = NULL;    /* INPUT */
    fbe_status_t									status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t * 								payload = NULL;
    fbe_payload_control_operation_t *   			control_operation = NULL;
    fbe_u32_t   									len = 0;
    fbe_base_config_nonpaged_metadata_t *   		base_config_nonpaged_metadata_p = NULL;
    fbe_u32_t   									nonpaged_metadata_size = 0;
    fbe_payload_control_status_t                    control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_bool_t                                      initialized = FBE_FALSE;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &get_metadata_info);
    if (get_metadata_info == NULL) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof (fbe_base_config_control_metadata_get_info_t)){
        fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer_length failed\n",
                                __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;

    }

    /*let's populate all the information we have about the metadata*/
    get_metadata_info->metadata_info.metadata_element_state = base_config->metadata_element.metadata_element_state;

    /* let's get the non-paged metadata of this object. */
    fbe_base_config_get_nonpaged_metadata_ptr(base_config, (void **)&base_config_nonpaged_metadata_p);
    if (base_config_nonpaged_metadata_p != NULL) {
        /*check initialized after non-paged metadata pointer to avoid NULL pointer error trace*/
        fbe_base_config_metadata_nonpaged_is_initialized(base_config, &initialized);

        fbe_base_config_get_nonpaged_metadata_size(base_config, &nonpaged_metadata_size);

        fbe_zero_memory(&get_metadata_info->nonpaged_data.data, nonpaged_metadata_size);
        fbe_copy_memory(&get_metadata_info->nonpaged_data.data, 
                            base_config_nonpaged_metadata_p, 
                            nonpaged_metadata_size);

        /*return the non-paged metadata size*/
        get_metadata_info->nonpaged_data_size = nonpaged_metadata_size;
        /*get the committed non-paged metadata size of this object*/
        status = fbe_database_get_committed_nonpaged_metadata_size(base_config->base_object.class_id, 
                                                              &get_metadata_info->committed_nonpaged_data_size);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed to get commit NP size\n",
                                __FUNCTION__);
            get_metadata_info->committed_nonpaged_data_size = 0;
        }
        get_metadata_info->nonpaged_initialized = initialized;
        control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    } else{
        get_metadata_info->nonpaged_initialized = FBE_FALSE;
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    }

    fbe_payload_control_set_status(control_operation, control_status);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;

}

/*!**************************************************************
 * fbe_base_config_control_metadata_nonpaged_write_persist()
 ****************************************************************
 * @brief
 *  Write nonpaged metadata to memory and persist it.
 *  This function won't get NP lock before updating.
 *  It should only be used by LUN objects.
 *
 * @param  base_config_p - The base config object.
 * @param  packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  04/22/2013 - Create. Jamin Kang.
 *
 ****************************************************************/
static fbe_status_t fbe_base_config_control_metadata_nonpaged_write_persist(fbe_base_config_t *base_config,fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;
    fbe_base_config_control_metadata_nonpaged_change_t * nonpaged_change = NULL;	/* INPUT */
    fbe_status_t status;

    fbe_base_object_trace((fbe_base_object_t*)base_config,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    /* We can't use fbe_payload_ex_get_control_operation here as this function
     * maybe in the complete stack of allocating NP lock and current operation
     * is stripe lock operation.
     */
    control_operation = fbe_payload_ex_get_present_control_operation(payload);

    if (control_operation == NULL) {
        fbe_base_object_trace((fbe_base_object_t *)base_config,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s fbe_payload_ex_get_present_control_operation failed\n",
                              __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation, &nonpaged_change);
    if (nonpaged_change == NULL) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s: write @%llu, len: %u\n",
                            __FUNCTION__, nonpaged_change->metadata_offset,
                            nonpaged_change->metadata_record_data_size);

    status = fbe_base_config_metadata_nonpaged_write_persist(base_config, packet,
                                                             nonpaged_change->metadata_offset,
                                                             nonpaged_change->metadata_record_data,
                                                             nonpaged_change->metadata_record_data_size);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)base_config,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: failed to write/persist nonpaged metadata\n",
                              __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

static fbe_status_t
fbe_base_config_control_metadata_nonpaged_write_persist_release_NP_lock(fbe_packet_t *packet, fbe_packet_completion_context_t context)
{
    fbe_base_config_t * base_config = (fbe_base_config_t*)context;

    fbe_base_object_trace((fbe_base_object_t*)base_config,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);
    fbe_base_config_release_np_distributed_lock(base_config, packet);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

static fbe_status_t
fbe_base_config_control_metadata_nonpaged_write_persist_get_NP_lock_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_base_config_t * base_config = (fbe_base_config_t*)context;
    fbe_status_t status;

    fbe_base_object_trace((fbe_base_object_t*)base_config,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* If the status is not ok, that means we didn't get the
       lock. Failed the operation.
    */
    status = fbe_transport_get_status_code (packet);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: can't get the NP lock, status: 0x%X",
                              __FUNCTION__, status);
        fbe_transport_set_status(packet, status, 0);
        return FBE_STATUS_OK;
    }

    fbe_transport_set_completion_function(packet, fbe_base_config_control_metadata_nonpaged_write_persist_release_NP_lock, base_config);
    fbe_base_config_control_metadata_nonpaged_write_persist(base_config, packet);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!**************************************************************
 * fbe_base_config_control_metadata_nonpaged_write_persist_NP_lock()
 ****************************************************************
 * @brief
 *  Write nonpaged metadata to memory and persist it.
 *  This function will get NP lock before updating.
 *
 * @param  base_config_p - The base config object.
 * @param  packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  04/22/2013 - Create. Jamin Kang.
 *
 ****************************************************************/
static fbe_status_t fbe_base_config_control_metadata_nonpaged_write_persist_NP_lock(fbe_base_config_t * base_config, fbe_packet_t * packet)
{
    fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);


    if (base_config->base_object.class_id == FBE_CLASS_ID_LUN) {
        /* We needn't get NP lock for LUN */
        return fbe_base_config_control_metadata_nonpaged_write_persist(base_config, packet);
    }

    fbe_transport_set_completion_function(packet,
                                          fbe_base_config_control_metadata_nonpaged_write_persist_get_NP_lock_completion,
                                          base_config);
    fbe_base_config_get_np_distributed_lock(base_config, packet);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*!**************************************************************
 * fbe_base_config_control_metadata_memory_read()
 ****************************************************************
 * @brief
 *  Get the metadata memory info of object
 *
 * @param  base_config_p - The base config object.
 * @param  packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/29/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static fbe_status_t fbe_base_config_control_metadata_memory_read(fbe_base_config_t * base_config, fbe_packet_t * packet)
{
    fbe_base_config_control_metadata_memory_read_t *mm_info = NULL;
    fbe_u32_t   									len = 0;
    fbe_status_t									status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t * 								payload = NULL;
    fbe_payload_control_operation_t *   			control_operation = NULL;
    fbe_payload_control_status_t                    control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_u32_t                                       copy_size = 0;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &mm_info);
    if (mm_info == NULL) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof (fbe_base_config_control_metadata_memory_read_t)){
        fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer_length failed\n",
                                __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_base_config_metadata_memory_read(base_config,
                                                  mm_info->is_peer,
                                                  &mm_info->memory_data[0],
                                                  mm_info->memory_size, &copy_size);
    mm_info->memory_size = copy_size;

    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)base_config,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: failed to read metadata memory\n",
                              __FUNCTION__);
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    }

    fbe_payload_control_set_status(control_operation, control_status);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;

}

/*!**************************************************************
 * fbe_base_config_control_metadata_memory_update()
 ****************************************************************
 * @brief
 *  Update the metadata memory info of object
 *
 * @param  base_config_p - The base config object.
 * @param  packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/29/2013 - Created. Jamin Kang
 *
 ****************************************************************/
static fbe_status_t fbe_base_config_control_metadata_memory_update(fbe_base_config_t * base_config, fbe_packet_t * packet)
{
    fbe_base_config_control_metadata_memory_update_t *update_info = NULL;
    fbe_u32_t                           len = 0;
    fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t * 					payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_payload_control_status_t        control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &update_info);
    if (update_info == NULL) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof (fbe_base_config_control_metadata_memory_update_t)){
        fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer_length failed\n",
                                __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_base_config_metadata_memory_update(base_config,
                                                    &update_info->memory_data[0],
                                                    &update_info->memory_mask[0],
                                                    update_info->memory_offset,
                                                    update_info->memory_size);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *)base_config,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s: failed to update metadata memory\n",
                              __FUNCTION__);
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    }

    fbe_payload_control_set_status(control_operation, control_status);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_base_config_get_upstream_object_list()
 ****************************************************************
 * @brief
 * This function is used to get the attached upstream object list.
 * 
 * @param base_config_p - The virtual drive object pointer.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/23/2009 - Created. Dhaval Patel
 *
 ****************************************************************/
static fbe_status_t 
fbe_base_config_get_upstream_object_list (fbe_base_config_t *base_config_p, fbe_packet_t *packet_p)
{
    fbe_status_t								status = FBE_STATUS_OK;
    fbe_payload_ex_t * 						payload_p = NULL;
    fbe_payload_control_operation_t *   		control_operation_p = NULL;
    fbe_base_config_upstream_object_list_t *	upstream_object_list_p = NULL;
    fbe_payload_control_buffer_length_t 		length = 0;
    fbe_base_transport_server_t *   			base_transport_server_p = NULL;
    fbe_base_edge_t *   						base_edge_p = NULL;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    if (control_operation_p == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_ex_get_control_operation failed\n",
                                __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Retrieve the payload buffer. */
    fbe_payload_control_get_buffer(control_operation_p, &upstream_object_list_p);
    if (upstream_object_list_p == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* If length of the buffer does not match with base config upstream object
     * list then then return error.
     */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if(length != sizeof(fbe_base_config_upstream_object_list_t))
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload buffer lenght mismatch.\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Initialize the number of upstream objects with zero.
     */
    upstream_object_list_p->number_of_upstream_objects = 0;

    /* Get the base transport server pointer from the virtual drive object.
     */
    base_transport_server_p = (fbe_base_transport_server_t *)
            &(((fbe_base_config_t *) base_config_p )->block_transport_server);

    fbe_base_transport_server_lock(base_transport_server_p);
    base_edge_p = base_transport_server_p->client_list;

    while ((base_edge_p != NULL) && 
           (upstream_object_list_p->number_of_upstream_objects < FBE_MAX_UPSTREAM_OBJECTS))
    {
        upstream_object_list_p->upstream_object_list [upstream_object_list_p->number_of_upstream_objects] = base_edge_p->client_id;
        upstream_object_list_p->number_of_upstream_objects++;
        upstream_object_list_p->current_upstream_index = base_edge_p->client_index;
        base_edge_p = base_edge_p->next_edge;
    }
    fbe_base_transport_server_unlock(base_transport_server_p);

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);		
    return status;
}
/*************************************************
 * end fbe_base_config_get_upstream_object_list
 *************************************************/

/*!**************************************************************
 * fbe_base_config_get_downstream_object_list()
 ****************************************************************
 * @brief
 * This function is used to get the list of the downstream objects.
 * 
 * 
 * @param base_config_p - RAID group object..
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  12/02/2009 - Created. Dhaval Patel
 *
 ****************************************************************/
static fbe_status_t 
fbe_base_config_get_downstream_object_list (fbe_base_config_t *base_config_p, fbe_packet_t *packet_p)
{
    fbe_status_t								status = FBE_STATUS_OK;
    fbe_payload_ex_t * 						payload_p = NULL;
    fbe_payload_control_operation_t *   		control_operation_p = NULL;
    fbe_base_config_downstream_object_list_t *  base_config_downstream_object_list_p = NULL;
    fbe_payload_control_buffer_length_t 		length = 0;
    fbe_block_edge_t *  						block_edge_p = NULL;
    fbe_path_state_t							path_state = FBE_PATH_STATE_INVALID;
    fbe_u32_t   								width = 0;
    fbe_u32_t   								edge_index = 0;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /* Retrieve the payload and control operation from the packet.
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    if (control_operation_p == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_ex_get_control_operation failed\n",
                                __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Retrieve the downstream object list pointer from the payload.
     */
    fbe_payload_control_get_buffer(control_operation_p, &base_config_downstream_object_list_p);
    if (base_config_downstream_object_list_p == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* If length of the buffer does not match with downstream object list
     * structure then then return error.
     */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if(length != sizeof(fbe_base_config_downstream_object_list_t))
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload buffer lenght mismatch.\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Initialize the number of upstream objects with zero.
     */
    base_config_downstream_object_list_p->number_of_downstream_objects = 0;
    edge_index = 0;

    /* Get the width of the RAID group object.
     */
    fbe_base_config_get_width((fbe_base_config_t*)base_config_p, &width);

    /* Loop through the each downstream edges before we find either edge
     * pointer is NULL or path state is invalid.
     */
    while ((edge_index < width) && 
           (base_config_downstream_object_list_p->number_of_downstream_objects < FBE_MAX_DOWNSTREAM_OBJECTS))
    {
        /* Fetch the block edge ptr and send control packet on block edge.
         */
        fbe_base_config_get_block_edge((fbe_base_config_t*)base_config_p, &block_edge_p, edge_index);
        if (block_edge_p == NULL)
        {
            /* Break the loop if we find downstream edge is NULL.
             */
            break;
        }

        fbe_block_transport_get_path_state (block_edge_p, &path_state);
        if (path_state == FBE_PATH_STATE_INVALID)
        {
            /* continue the loop for the next edge_index
             */
            edge_index++;
            continue;
        }

        /* Copy the object id of this edge server. */
        base_config_downstream_object_list_p->downstream_object_list[base_config_downstream_object_list_p->number_of_downstream_objects] =
            block_edge_p->base_edge.server_id;

        /* Increment the number of downstream edges.
         */
        base_config_downstream_object_list_p->number_of_downstream_objects++;
        edge_index++;
    }


    /* Complete the packet with good status.
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}
/*************************************************
 * end fbe_base_config_get_downstream_object_list
 *************************************************/

/*!**************************************************************
 * fbe_base_config_get_upstream_edge_count()
 ****************************************************************
 * @brief
 * This function is used to get upstream edge count.
 * 
 * @param base_config_p - The virtual drive object pointer.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/23/2009 - Created. Dhaval Patel
 *
 ****************************************************************/
static fbe_status_t 
fbe_base_config_get_upstream_edge_count (fbe_base_config_t *base_config_p, fbe_packet_t *packet_p)
{
    fbe_status_t								status = FBE_STATUS_OK;
    fbe_payload_ex_t * 						payload_p = NULL;
    fbe_payload_control_operation_t *   		control_operation_p = NULL;
    fbe_base_config_upstream_edge_count_t * 	upstream_edge_count_p = NULL;
    fbe_payload_control_buffer_length_t 		length = 0;
    fbe_base_transport_server_t *   			base_transport_server_p = NULL;
    fbe_base_edge_t *   						base_edge_p = NULL;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    if (control_operation_p == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_ex_get_control_operation failed\n",
                                __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Retrieve the payload buffer.
     */
    fbe_payload_control_get_buffer(control_operation_p, &upstream_edge_count_p);
    if (upstream_edge_count_p == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* If length of the buffer does not match with base config upstream object
     * list then then return error.
     */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if(length != sizeof(fbe_base_config_upstream_edge_count_t))
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload buffer lenght mismatch.\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Initialize the number of upstream objects with zero.
     */
    upstream_edge_count_p->number_of_upstream_edges = 0;

    /* Get the base transport server pointer from the virtual drive object.
     */
    base_transport_server_p = (fbe_base_transport_server_t *)
            &(((fbe_base_config_t *) base_config_p )->block_transport_server);

    fbe_base_transport_server_lock(base_transport_server_p);

    /* Get the edge list and traverse through to count the number of upstream
     * edge count.
     */
    base_edge_p = base_transport_server_p->client_list;
    while (base_edge_p != NULL)
    {
        upstream_edge_count_p->number_of_upstream_edges++;
        base_edge_p = base_edge_p->next_edge;
    }
    fbe_base_transport_server_unlock(base_transport_server_p);

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);		
    return status;
}
/*************************************************
 * end fbe_base_config_get_upstream_edge_count
 *************************************************/

/*!**************************************************************
 * fbe_base_config_get_downstream_edge_list()
 ****************************************************************
 * @brief
 * This function is used to get the list of the downstream edges.
 * 
 * 
 * @param base_config_p - RAID group object..
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  12/02/2009 - Created. Dhaval Patel
 *
 ****************************************************************/
static fbe_status_t 
fbe_base_config_get_downstream_edge_list (fbe_base_config_t *base_config_p, fbe_packet_t *packet_p)
{
    fbe_status_t								status = FBE_STATUS_OK;
    fbe_payload_ex_t * 						payload_p = NULL;
    fbe_payload_control_operation_t *   		control_operation_p = NULL;
    fbe_base_config_downstream_edge_list_t *	fbe_base_config_downstream_edge_list_p = NULL;
    fbe_payload_control_buffer_length_t 		length = 0;
    fbe_block_edge_t *  						block_edge_p = NULL;
    fbe_path_state_t							path_state = FBE_PATH_STATE_INVALID;
    fbe_u32_t   								width = 0;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /* Retrieve the payload and control operation from the packet.
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    if (control_operation_p == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_ex_get_control_operation failed\n",
                                __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Retrieve the downstream object list pointer from the payload.
     */
    fbe_payload_control_get_buffer(control_operation_p, &fbe_base_config_downstream_edge_list_p);
    if (fbe_base_config_downstream_edge_list_p == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* If length of the buffer does not match with downstream object list
     * structure then then return error.
     */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if(length != sizeof(fbe_base_config_downstream_edge_list_t))
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload buffer lenght mismatch.\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Initialize the number of upstream objects with zero.
     */
    fbe_base_config_downstream_edge_list_p->number_of_downstream_edges = 0;

    /* Get the width of the RAID group object.
     */
    fbe_base_config_get_width((fbe_base_config_t*)base_config_p, &width);

    /* Loop through the each downstream edges before we find either edge
     * pointer is NULL or path state is invalid.
     */
    while ((fbe_base_config_downstream_edge_list_p->number_of_downstream_edges < width) &&
           (fbe_base_config_downstream_edge_list_p->number_of_downstream_edges < FBE_MAX_DOWNSTREAM_OBJECTS))
    {
        /* Fetch the block edge ptr and send control packet on block edge.
         */
        fbe_base_config_get_block_edge((fbe_base_config_t*)base_config_p, &block_edge_p, fbe_base_config_downstream_edge_list_p->number_of_downstream_edges);
        if (block_edge_p == NULL)
        {   
            /* Break the loop if we find downstream edge is NULL.
             */
            break;
        }
        fbe_block_transport_get_path_state (block_edge_p, &path_state);
        if (path_state == FBE_PATH_STATE_INVALID)
        {
            /* Break the loop when we find the path state invalid.
             */
            break;
        }

        /* Clear the destination memory before copying edge data.
         */
        fbe_set_memory (&(fbe_base_config_downstream_edge_list_p->downstream_edge_list[fbe_base_config_downstream_edge_list_p->number_of_downstream_edges]), 0,
                        sizeof(fbe_base_edge_t));
        fbe_copy_memory (&(fbe_base_config_downstream_edge_list_p->downstream_edge_list[fbe_base_config_downstream_edge_list_p->number_of_downstream_edges]),
                         &(block_edge_p->base_edge), sizeof(fbe_base_edge_t));

        /* Increment the number of downstream edges.
         */
        fbe_base_config_downstream_edge_list_p->number_of_downstream_edges++;
    }

    /* Complete the packet with good status.
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}
/*************************************************
 * end fbe_base_config_get_downstream_edge_list
 *************************************************/

/*!**************************************************************
 * fbe_base_config_get_width()
 ****************************************************************
 * @brief
 * This function is used to get the width.
 * 
 * 
 * @param base_config_p - SEP objects.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  28/05/2010 - Created. Shanmugam.
 *
 ****************************************************************/
static fbe_status_t 
fbe_base_config_control_get_width(fbe_base_config_t *base_config_p, fbe_packet_t *packet_p)
{
    fbe_base_config_control_get_width_t * get_width_p = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t len = 0;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &get_width_p);
    if (get_width_p == NULL) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_base_config_control_get_width_t))
    {
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X length invalid\n", len);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_config_get_width(base_config_p, &(get_width_p->width));
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/*************************************************
 * end fbe_base_config_get_width
 *************************************************/

/***************************************************************
 * fbe_base_config_io_control_operation()
 ****************************************************************
 * @brief
 * This function is used to send an I/O request to the object.
 * 
 * 
 * @param base_config_p - base config object..
 * @param packet_p  	- The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  3/19/2010 - Created. Dhaval Patel
 *
 ****************************************************************/
fbe_status_t
fbe_base_config_io_control_operation(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t * 						sep_payload_p = NULL;
    fbe_payload_control_operation_t *   		control_operation_p = NULL; 					 
    fbe_payload_block_operation_t * 			block_operation_p = NULL;
    fbe_payload_control_buffer_length_t 		length = 0;
    fbe_sg_element_t *  						sg_list_p = NULL;
    fbe_sg_element_t *  						sg_element_p = NULL;
    fbe_status_t								status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_config_io_control_operation_t *	base_config_io_control_operation_p = NULL;
    fbe_block_size_t							optimum_block_size;
    fbe_block_edge_geometry_t   				block_edge_geometry;
    fbe_block_count_t   						sg_block_count;


    fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
            FBE_TRACE_LEVEL_DEBUG_HIGH,
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    if (control_operation_p == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Verify the input buffer. */
    fbe_payload_control_get_buffer(control_operation_p, &base_config_io_control_operation_p);
    if (base_config_io_control_operation_p == NULL)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Verify the length of the input buffer. */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if(length != sizeof(fbe_base_config_io_control_operation_t))
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* determine optimum block size of the base config object. */
    fbe_base_config_get_downstream_geometry(base_config_p, &block_edge_geometry);
    if (block_edge_geometry == FBE_BLOCK_EDGE_GEOMETRY_INVALID)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* derive the optimum block size. */
    fbe_block_transport_get_optimum_block_size(block_edge_geometry, &optimum_block_size);

    /* Allocate the block operation to send the I/O. */
    block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);

    /* Build the block operation. */
    fbe_payload_block_build_operation(block_operation_p,
                                      base_config_io_control_operation_p->block_operation_code,
                                      base_config_io_control_operation_p->lba,    /* LBA */
                                      base_config_io_control_operation_p->block_count,    /* blocks */
                                      FBE_BE_BYTES_PER_BLOCK,    /* block size */
                                      optimum_block_size,    /* optimum block size */
                                      NULL);	/* preread descriptor */

    /* We use data buffer memory provided in control buffer so caller has to
     * take into account of these two sg elements.
     */
    sg_list_p = (fbe_sg_element_t *)base_config_io_control_operation_p->data_buffer;
    sg_element_p = sg_list_p;

    sg_block_count = base_config_io_control_operation_p->block_count * FBE_BE_BYTES_PER_BLOCK;

    if( sg_block_count > FBE_U32_MAX)
    {
        /* we do not expect the count to exceed 32 limit, but we did it here!
         */
         status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                                                  FBE_TRACE_LEVEL_ERROR,
                                                  FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                                  "0x%llx Invalid block count for sg element\n", 
                                                  (unsigned long long)sg_block_count);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }
    fbe_sg_element_init(sg_element_p,(fbe_u32_t)(sg_block_count), 
                        (base_config_io_control_operation_p->data_buffer + (2 * sizeof(fbe_sg_element_t))));
    fbe_sg_element_increment(&sg_element_p);
    fbe_sg_element_terminate(sg_element_p);
    fbe_payload_ex_set_sg_list(sep_payload_p, sg_list_p, 1);

    /* Set the completion function before issuing I/O request. */
    status = fbe_transport_set_completion_function(packet_p, fbe_base_config_io_control_operation_completion, base_config_p);

    /* Increment the block operation before calling bouncer entry. */
    fbe_payload_ex_increment_block_operation_level(sep_payload_p);

    /* Send the block operation. */
    /* Temporary hack */
    status = fbe_base_config_bouncer_entry(base_config_p, packet_p);
    if(base_config_p->block_transport_server.attributes & FBE_BLOCK_TRANSPORT_ENABLE_STACK_LIMIT){
        if(status == FBE_STATUS_OK){
            base_config_p->block_transport_server.block_transport_const->block_transport_entry(base_config_p, packet_p);
        }
    }
    return status;
}
/****************************************************************
 * end fbe_base_config_io_control_operation
 ****************************************************************/

/*!**************************************************************
 * fbe_base_config_io_control_operation_completion()
 ****************************************************************
 * @brief
 *  This function is used as completion function for the I/O
 *  control operation.
 * 
 * @param packet_p - Pointer to the packet.
 * @param context  - Packet completion context.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  3/19/2010 - Created. Dhaval Patel
 *
 ****************************************************************/
static fbe_status_t 
fbe_base_config_io_control_operation_completion(fbe_packet_t * packet_p,
                                                fbe_packet_completion_context_t context)
{
    fbe_base_config_t * 						base_config_p = NULL;
    fbe_payload_ex_t * 						sep_payload_p = NULL;
    fbe_payload_control_operation_t *   		control_operation_p = NULL;
    fbe_payload_block_operation_t * 			block_operation_p = NULL;
    fbe_status_t								status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_block_operation_status_t		block_operation_status;
    fbe_payload_control_status_t				control_status;
    fbe_base_config_io_control_operation_t *	base_config_io_control_operation_p = NULL;

   /* Cast the context to base config object pointer. */
    base_config_p = (fbe_base_config_t *)context;

    /* Get the payload and control operation of the packet. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_prev_control_operation(sep_payload_p);
   

    /* Get the block operation of the packet. */
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);

    /* Get the signature status buffer from the master packet. */
    fbe_payload_control_get_buffer(control_operation_p, &base_config_io_control_operation_p);

    /* Get the status of the update signature. */
    status = fbe_transport_get_status_code(packet_p);
    if (status == FBE_STATUS_OK)
    {
        fbe_payload_block_get_status(block_operation_p, &block_operation_status);
    }

    if ((status != FBE_STATUS_OK) || (block_operation_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS))
    {
        /* On failure update the control status as failed. */
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;
    }
    else
    {
        /* On success update the control status as ok. */
        control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    }

    /* Release this block payload since we are done with it. */
    fbe_payload_ex_release_block_operation(sep_payload_p, block_operation_p);
 
    /* Complete the packet with status. */
    fbe_transport_set_status(packet_p, status, 0);
    fbe_payload_control_set_status (control_operation_p, control_status);
    return status;
}
/****************************************************************
 * end fbe_base_config_io_control_operation_completion
 ****************************************************************/

static fbe_status_t fbe_base_config_client_is_hibernating(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_block_client_hibernating_info_t *   hibernation_info = NULL;	/* INPUT */
    fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t * 						payload = NULL;
    fbe_payload_control_operation_t *   	control_operation = NULL;
    fbe_payload_control_buffer_length_t 	len = 0;
    fbe_base_edge_t *   					base_edge = NULL;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &hibernation_info);
    if (hibernation_info == NULL){
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);    
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_block_client_hibernating_info_t)){
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Invalid length:%d\n",
                                __FUNCTION__, len);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    base_edge = fbe_transport_get_edge(packet_p);/*we need it to know the client index*/

    /*let's put on the edge the fact it's in hibernation*/
    status = fbe_block_transport_server_set_path_attr_without_event_notification(&base_config_p->block_transport_server,
                                                      base_edge->client_id,
                                                      FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_HIBERNATING);

    if(status != FBE_STATUS_OK){
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed to set hibernation attribute on edge:%d\n",
                                __FUNCTION__, base_edge->client_index);
    }

    /*and what is the latency time it requires*/
    status = fbe_block_transport_server_set_time_to_become_ready(&base_config_p->block_transport_server,
                                                                 base_edge->client_id,
                                                                 hibernation_info->max_latency_time_is_sec);

     if(status != FBE_STATUS_OK){
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed to set expected latency time on edge:%d\n",
                                __FUNCTION__, base_edge->client_index);
    }

    
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);    
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}


static fbe_status_t fbe_base_config_client_is_out_of_hibernation(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t * 						payload = NULL;
    fbe_payload_control_operation_t *   	control_operation = NULL;
    fbe_base_edge_t *   					base_edge = NULL;
    fbe_power_save_state_t 					power_save_state = FBE_POWER_SAVE_STATE_INVALID;
    fbe_u32_t                               number_of_clients;
    fbe_time_t                              current_time = 0;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload); 
     
    /* Only update the upstream edge attributes if there is an upstream object
     */
    fbe_block_transport_server_get_number_of_clients(&base_config_p->block_transport_server, &number_of_clients);
    if (number_of_clients > 0)
    {
        /* Get and validate the upstream edge passed.*/
        base_edge = fbe_transport_get_edge(packet_p);/*we need it to know the client index*/
        if (base_edge == NULL)
        {
            fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s base edge NULL with: %d clients\n",
                                  __FUNCTION__, number_of_clients);

            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_OK;
        }

        base_edge = fbe_transport_get_edge(packet_p);/*we need it to know the client index*/

        /* we treat this as an IO */
        current_time = fbe_get_time();
        fbe_block_transport_server_set_last_io_time(&base_config_p->block_transport_server, current_time);
        fbe_base_config_metadata_set_last_io_time(base_config_p, current_time);
        /*and set the condition that will update the metadata memory area for the peer*/
        fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, (fbe_base_object_t*)base_config_p, FBE_BASE_CONFIG_LIFECYCLE_COND_UPDATE_METADATA_MEMORY);

        /*let's put on the edge the fact it's not in hibernation anymore*/
        status = fbe_block_transport_server_clear_path_attr(&base_config_p->block_transport_server,
                                                        base_edge->server_index,
                                                        (FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_HIBERNATING|FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_READY_TO_HIBERNATE));

        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s failed to clear hibernation attribute on edge:%d\n",
                                __FUNCTION__, base_edge->client_index);

            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_OK;
        }
    }

    /* Lock the configuration for updating */
    fbe_base_config_lock(base_config_p); 
    /*let's wake up(if we are not already woken up)*/
    fbe_base_config_metadata_get_power_save_state(base_config_p, &power_save_state);

    if (power_save_state == FBE_POWER_SAVE_STATE_SAVING_POWER) {

        fbe_base_config_metadata_set_power_save_state(base_config_p, FBE_POWER_SAVE_STATE_EXITING_POWER_SAVE);

        /*we are good to go, there is no one above us*/
        status = fbe_lifecycle_set_cond(&fbe_base_config_lifecycle_const, 
                                        (fbe_base_object_t *)base_config_p, 
                                        FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);

        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Starting to get out of hibernation...\n",
                                __FUNCTION__);

    }
    /* when LUN enters its 2 minutes quiet time prior to going hibernation,
     * load media ioctl needs to stop that by changing the power save state.
     */
    if (power_save_state == FBE_POWER_SAVE_STATE_ENTERING_POWER_SAVE) {
        fbe_base_config_metadata_set_power_save_state(base_config_p, FBE_POWER_SAVE_STATE_EXITING_POWER_SAVE);

        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: stop hibernation...\n",
                                __FUNCTION__);
    }
    /* Unlock the configuration since update completed. */
    fbe_base_config_unlock(base_config_p);

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);    
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_base_config_commit_configuration()
 ******************************************************************************
 * @brief
 *  This function is used to set the configuration commited flag for the base
 *  config object.
 *  It is used to hold all the objects which is derived from base config until
 *  configuration service commit the configuration.
 *
 * @param base_config_p 		- Pointer to the base config object.
 * @param packet_p  			- The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  04/23/2010 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_base_config_commit_configuration(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t * 							payload_p = NULL;
    fbe_payload_control_operation_t *   			control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 			len = 0;
    fbe_base_config_commit_configuration_t *		base_config_commit_configuration;
    fbe_class_id_t  								base_config_class_id = FBE_CLASS_ID_INVALID;
    fbe_object_id_t 								base_config_object_id = FBE_OBJECT_ID_INVALID;
    fbe_notification_info_t 						notification_info;
    fbe_status_t									status;
    fbe_lifecycle_state_t   						lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &base_config_commit_configuration);
    if (base_config_commit_configuration == NULL)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_base_config_commit_configuration_t))
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_config_lock(base_config_p);
    /* Set the configuration commited flag if configuration service has commited the object information. */
    fbe_base_config_set_configuration_committed(base_config_p);
    if(base_config_commit_configuration->is_initial_commit == FBE_TRUE)
    {
        fbe_base_config_set_flag(base_config_p, FBE_BASE_CONFIG_FLAG_INITIAL_CONFIGURATION);
    }
    fbe_base_config_unlock(base_config_p);

    /* The set_default_offset has been moved to object level as we felt that is the right place... */

    /* Get class ID */
    base_config_class_id = fbe_base_object_get_class_id(fbe_base_pointer_to_handle((fbe_base_t *) base_config_p));    

    /* Get object ID */
    fbe_base_object_get_object_id((fbe_base_object_t *)base_config_p, &base_config_object_id);

    /* We will send nitification if we are in READY or HIBERNATE state ONLY */
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)base_config_p, &lifecycle_state);

    if(lifecycle_state == FBE_LIFECYCLE_STATE_READY || lifecycle_state == FBE_LIFECYCLE_STATE_HIBERNATE){
        /* Send notification of a configuration change */
        notification_info.notification_type = FBE_NOTIFICATION_TYPE_CONFIGURATION_CHANGED;
        notification_info.class_id  		= base_config_class_id;

        fbe_topology_class_id_to_object_type(base_config_class_id, &notification_info.object_type);

        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                              FBE_TRACE_LEVEL_DEBUG_LOW,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "UPDATE: commit config change obj: 0x%x class: %d object_type: 0x%llx\n",
                              base_config_object_id, notification_info.class_id, (unsigned long long)notification_info.object_type);

        status = fbe_notification_send(base_config_object_id, notification_info);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s: config change status notification failed, status: 0x%x\n",
                                    __FUNCTION__, 
                                    status);
        }
    }
    else if (lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE){
        /* we need to kick the object after the commit */
        status = fbe_lifecycle_reschedule(&fbe_base_config_lifecycle_const,
                                          (fbe_base_object_t *) base_config_p,
                                          (fbe_lifecycle_timer_msec_t) 0);    /* Immediate reschedule */
    }

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}
/******************************************************************************
 * end fbe_base_config_commit_configuration
 ******************************************************************************/

static fbe_status_t fbe_base_config_usurper_enable_object_power_save (fbe_base_config_t *base_config_p, fbe_packet_t *packet_p)
{
    fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t * 						payload = NULL;
    fbe_payload_control_operation_t *   	control_operation = NULL;
    fbe_class_id_t  						my_class_id = FBE_CLASS_ID_INVALID;
    
    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    /*if this is a LUN this usuper command is TEMPORARY invalid (until layered drivers will start using the
    FLARE_POWER_SAVE_POLICY IOCTL, we allow this at RAID level and below,*/
    my_class_id = fbe_base_object_get_class_id(fbe_base_pointer_to_handle((fbe_base_t *) base_config_p)); 

    if (my_class_id == FBE_CLASS_ID_LUN) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Illegal command for LU, send it to RAID only\n",
                                __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;

    }

    status = fbe_base_config_enable_object_power_save(base_config_p);

    fbe_payload_control_set_status(control_operation, ((status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE));
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}

static fbe_status_t fbe_base_config_usurper_disable_object_power_save (fbe_base_config_t *base_config_p, fbe_packet_t *packet_p)
{
    fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t * 						payload = NULL;
    fbe_payload_control_operation_t *   	control_operation = NULL;
    fbe_class_id_t  						my_class_id = FBE_CLASS_ID_INVALID;
    
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    /*if this is a LUN this usuper command is TEMPORARY invalid (until layered drivers will start using the
    FLARE_POWER_SAVE_POLICY IOCTL, we allow this at RAID level and below,*/
    my_class_id = fbe_base_object_get_class_id(fbe_base_pointer_to_handle((fbe_base_t *) base_config_p)); 

    if (my_class_id == FBE_CLASS_ID_LUN) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Illegal command for LU, send it to RAID only\n",
                                __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;

    }

    status = fbe_base_config_disable_object_power_save(base_config_p);

    fbe_payload_control_set_status(control_operation, ((status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE));
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}

static fbe_status_t fbe_base_config_usurper_set_deny_operation_permission(fbe_base_config_t *base_config_p, fbe_packet_t *packet_p)
{
    fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t * 						payload = NULL;
    fbe_payload_control_operation_t *   	control_operation = NULL;
    
    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    status = fbe_base_config_set_deny_operation_permission(base_config_p);

    fbe_payload_control_set_status(control_operation, ((status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE));
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}

static fbe_status_t fbe_base_config_usurper_clear_deny_operation_permission(fbe_base_config_t *base_config_p, fbe_packet_t *packet_p)
{
    fbe_status_t							status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t * 						payload = NULL;
    fbe_payload_control_operation_t *   	control_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    status = fbe_base_config_clear_deny_operation_permission(base_config_p);

    fbe_payload_control_set_status(control_operation, ((status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE));
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}


/*!**************************************************************
 * fbe_base_config_usurper_set_permanent_destroy()
 ****************************************************************
 * @brief
 *  This function sets permanent destroy bit in base config flags.
 *
 * @param in_base_config_p  - Pointer to the raid group.
 * @param in_packet_p       - Pointer to the packet.
 *
 * @return status - The status of the operation. 
 *
 * @author
 *  09/07/2012 - Created. Prahlad Purohit
 *
 ****************************************************************/
static fbe_status_t 
fbe_base_config_usurper_set_permanent_destroy(fbe_base_config_t *base_config_p, 
                                              fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    status = fbe_base_config_set_permanent_destroy(base_config_p);

    fbe_payload_control_set_status(control_operation, ((status == FBE_STATUS_OK) ? FBE_PAYLOAD_CONTROL_STATUS_OK : FBE_PAYLOAD_CONTROL_STATUS_FAILURE));
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_base_config_usurper_get_control_buffer()
 ****************************************************************
 * @brief
 *  This function gets the buffer pointer out of the packet
 *  and validates it. It returns error status if any of the
 *  pointer validations fail or the buffer size doesn't match
 *  the specifed buffer length.
 *
 * @param in_base_config_p   - Pointer to the raid group.
 * @param in_packet_p   	- Pointer to the packet.
 * @param in_buffer_length  - Expected length of the buffer.
 * @param out_buffer_pp 	- Pointer to the buffer pointer. 
 *
 * @return status - The status of the operation. 
 *
 * @author
 *  03/07/2011 - Created. Swati Fursule
 *
 ****************************************************************/
fbe_status_t fbe_base_config_usurper_get_control_buffer(
                                        fbe_base_config_t*  				in_base_config_p,
                                        fbe_packet_t*   					in_packet_p,
                                        fbe_payload_control_buffer_length_t in_buffer_length,
                                        fbe_payload_control_buffer_t*   	out_buffer_pp)
{
    fbe_payload_ex_t*  				payload_p;
    fbe_payload_control_operation_t*	control_operation_p;  
    fbe_payload_control_buffer_length_t buffer_length;


    // Get the control op buffer data pointer from the packet payload.
    payload_p = fbe_transport_get_payload_ex(in_packet_p);
    if (payload_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)in_base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    if (control_operation_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)in_base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s control operation is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation_p, out_buffer_pp);
    if (*out_buffer_pp == NULL)
    {
        // The buffer pointer is NULL!
        fbe_base_object_trace((fbe_base_object_t *)in_base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s buffer pointer is NULL\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    };

    fbe_payload_control_get_buffer_length(control_operation_p, &buffer_length);
    if (buffer_length != in_buffer_length)
    {
        // The buffer length doesn't match!
        fbe_base_object_trace((fbe_base_object_t *)in_base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s buffer length mismatch\n", __FUNCTION__);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;

} 
/**************************************
 * end fbe_base_config_usurper_get_control_buffer()
 **************************************/

/*!****************************************************************************
 * fbe_base_config_send_block_operation_completion()
 ******************************************************************************
 * @brief
 *  This function looks at the subpacket queue and if it is empty
 *  just return ok allowing the packet to complete
 *
 * @param packet_p  
 * @param context   
 *
 * @return status 
 *
 * @author
 *  03/07/2011 - Created. Swati Fursule
 *
 ******************************************************************************/
static fbe_status_t fbe_base_config_send_block_operation_completion(fbe_packet_t* packet_p,
                                                       fbe_packet_completion_context_t context)
{   
    fbe_memory_request_t*   							memory_request_p = NULL;
     
    memory_request_p = fbe_transport_get_memory_request(packet_p);
    fbe_memory_free_request_entry(memory_request_p);

    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_base_config_send_block_operation_completion()
 **************************************/


/*!****************************************************************************
 * fbe_base_config_usurper_subpacket_completion()
 ******************************************************************************
 * @brief
 *  This function checks the subpacket status, copies the status to the master packet.
 *  It releases the control operation and frees the subpacket. It completes the
 *  master packet
 *
 * @param packet_p  
 * @param context   		  
 *
 * @return status 
 *
 * @author
 *  03/07/2011 - Created. Swati Fursule
 *
 ******************************************************************************/
fbe_status_t fbe_base_config_usurper_subpacket_completion(fbe_packet_t* packet_p,
                                                       fbe_packet_completion_context_t context)
{ 
    fbe_base_object_t*  								base_object_p = NULL;      
    fbe_packet_t *  									master_packet_p = NULL;    
    fbe_payload_ex_t * 								payload_p = NULL;
    fbe_payload_ex_t * 								master_payload_p = NULL;
    fbe_payload_control_operation_t *   				control_operation_p = NULL;    
    fbe_payload_control_operation_t *   				master_control_operation_p = NULL;    
    fbe_status_t										status;
    fbe_payload_control_status_t						control_status;
    fbe_block_transport_block_operation_info_t *		block_operation_info_p = NULL;
    fbe_block_transport_block_operation_info_t *		master_block_operation_info_p = NULL;
    fbe_payload_control_operation_opcode_t  			opcode;
    fbe_sg_element_t *  								sg_list_p = NULL;
    fbe_u32_t   										sg_list_count;
	fbe_payload_control_operation_opcode_t		        control_opcode;
	fbe_block_transport_io_operation_info_t * io_operation;

    base_object_p = (fbe_base_object_t*)context;
    
    master_packet_p = (fbe_packet_t *)fbe_transport_get_master_packet(packet_p);		

    /* Get the subpacket payload and control operation., input buffer and sglist */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p); 	   
    fbe_payload_control_get_buffer(control_operation_p, &block_operation_info_p);
    fbe_payload_control_get_opcode(control_operation_p, &opcode);
    status = fbe_payload_ex_get_sg_list(payload_p, &sg_list_p, &sg_list_count);

    /* Get the master packet payload and control operation. */
    /* Save the control operation with the result of block operation.info */
    master_payload_p = fbe_transport_get_payload_ex(master_packet_p);

    master_control_operation_p = fbe_payload_ex_get_control_operation(master_payload_p);

	status = fbe_payload_ex_set_sg_list(master_payload_p, sg_list_p, sg_list_count);

	/* Check what kind of control operation we are using */
    fbe_payload_control_get_opcode(master_control_operation_p, &control_opcode);

	if(control_opcode == FBE_BLOCK_TRANSPORT_CONTROL_CODE_IO_COMMAND){
		fbe_payload_control_get_buffer(master_control_operation_p, &io_operation);

		io_operation->status =  block_operation_info_p->block_operation.status;
		io_operation->status_qualifier =  block_operation_info_p->block_operation.status_qualifier;	

	} else {
		fbe_payload_control_get_buffer(master_control_operation_p, &master_block_operation_info_p);

		master_block_operation_info_p->block_operation.status =  block_operation_info_p->block_operation.status;
		master_block_operation_info_p->block_operation.status_qualifier =  block_operation_info_p->block_operation.status_qualifier;	
		master_block_operation_info_p->verify_error_counts =  block_operation_info_p->verify_error_counts;
	}

    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    fbe_transport_remove_subpacket(packet_p);
    fbe_transport_copy_packet_status(packet_p, master_packet_p);

    /* If status is not good then complete the master packet with error. */
    if((status != FBE_STATUS_OK) || (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_object_trace( base_object_p,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "Marking for block operation failed, status 0x%x\n", status);
    }    

    /* Free the resources we allocated previously. */    
    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
    fbe_transport_destroy_packet(packet_p);

    /* Remove master packet from the usurper queue. */
    fbe_base_object_remove_from_usurper_queue(base_object_p, master_packet_p);
    fbe_transport_complete_packet(master_packet_p); 
    return status;
}
/**************************************
 * end fbe_base_config_usurper_subpacket_completion()
 **************************************/


/*!****************************************************************************
 * fbe_base_config_create_sub_packet_and_send_block_operation()
 ******************************************************************************
 * @brief
 * This function allocates memory for the subpacket
 *  This function creates a subpacket and copies the control operation
 *  from master packet and initiates a block opration. A subpacket is created
 *  because if the master packet is used, the stack level inside the
 *  packet exceeds the maximum limit.
 *
 * @param packet_p  	 
 * @param base_object_p  
 *
 * @return status
 *
 * @author
 *  03/07/2011 - Created. Swati Fursule
 *
 ******************************************************************************/
static fbe_status_t 
fbe_base_config_create_sub_packet_and_send_block_operation(fbe_base_object_t*  base_object_p,
                                                           fbe_packet_t * packet_p) 																	  
{
    fbe_memory_request_t*   				memory_request_p = NULL;
    fbe_memory_request_priority_t   		memory_request_priority = 0;
    fbe_status_t							status;    

    memory_request_p = fbe_transport_get_memory_request(packet_p);
    memory_request_priority = fbe_transport_get_resource_priority(packet_p);	

    /* build the memory request for allocation. */
    status = fbe_memory_build_chunk_request(memory_request_p,
                                   FBE_MEMORY_CHUNK_SIZE_FOR_PACKET,
                                   2 + 1, /* +1 for the FBE_BLOCK_TRANSPORT_CONTROL_CODE_IO_COMMAND */
                                   memory_request_priority,
                                   fbe_transport_get_io_stamp(packet_p),
                                   (fbe_memory_completion_function_t)fbe_base_config_usurper_memory_allocation_completion, /* SAFEBUG - cast to supress warning but must fix because memory completion function shouldn't return status */
                                   base_object_p);
    if(status == FBE_STATUS_GENERIC_FAILURE)
    {
        fbe_base_object_trace(base_object_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 									   
                              "Setup for memory allocation failed, status:0x%x, line:%d\n", status, __LINE__);  	  
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);   	 
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    fbe_transport_memory_request_set_io_master(memory_request_p,  packet_p);

    /* Issue the memory request to memory service. */
    status = fbe_memory_request_entry(memory_request_p);
    if((status != FBE_STATUS_OK) && (status != FBE_STATUS_PENDING))
    {
        fbe_base_object_trace(base_object_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED, 									   
                              "memory allocation failed, status:0x%x, line:%d\n", status, __LINE__);		
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);   	 
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    return status;
}
/**************************************
 * end fbe_base_config_create_sub_packet_and_send_block_operation()
 **************************************/

/*!**************************************************************
 * fbe_base_config_get_downstream_edge_geometry()
 ****************************************************************
 * @brief
 *  Obtains the downstream block geometry of an object
 *
 * @param base_config_p - The base config.
 * @param packet_p - the packet.
 *
 * @return fbe_status_t   
 *
 * @author
 *  03/03/2011 - Created. JMedina
 *
 ****************************************************************/

fbe_status_t fbe_base_config_get_downstream_edge_geometry(
        fbe_base_config_t * base_config_p, 
        fbe_packet_t * packet_p)
{
    fbe_status_t								status = FBE_STATUS_OK;
    fbe_payload_ex_t * 						payload_p = NULL;
    fbe_payload_control_operation_t *   		control_operation_p = NULL;
    fbe_base_config_block_edge_geometry_t   	*edge_geometry_p;
    fbe_block_edge_geometry_t   				temp_block_edge_geometry;
    fbe_payload_control_buffer_length_t 		length = 0;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
            FBE_TRACE_LEVEL_DEBUG_HIGH,
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "%s entry\n", __FUNCTION__);

    /* Retrieve the payload and control operation from the packet.
    */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    if (control_operation_p == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s fbe_payload_ex_get_control_operation failed\n",
                __FUNCTION__);

        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Retrieve the block edge geometry pointer from the payload.
    */
    fbe_payload_control_get_buffer(control_operation_p, &edge_geometry_p);
    if (edge_geometry_p == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* If length of the buffer does not match with block edge geometry structure
     * then return error.
     */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if(length != sizeof(edge_geometry_p))
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s payload buffer length mismatch.\n",
                __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Initialize the block geometry */
    edge_geometry_p->block_geometry = FBE_BLOCK_EDGE_GEOMETRY_INVALID;
    temp_block_edge_geometry = FBE_BLOCK_EDGE_GEOMETRY_INVALID;

    /* Get the block geometry */
    fbe_base_config_get_downstream_geometry(base_config_p, &temp_block_edge_geometry);
    if (temp_block_edge_geometry == FBE_BLOCK_EDGE_GEOMETRY_INVALID)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    edge_geometry_p->block_geometry = temp_block_edge_geometry;

    /* Complete the packet with good status.
    */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return status;
}

/******************************************
 * end fbe_base_config_get_downstream_edge_geometry()
 ******************************************/
/*!**************************************************************
 * fbe_base_config_usurper_memory_allocation_completion()
 ****************************************************************
 * @brief
 *  This function is the completion function for the memory
 *  allocation. It sends the subpacket to initiate verify
 *
 * @param memory_request_p  
 * @param in_context   
 *
 * @return status - The status of the operation.
 *
 * @author
 *  03/07/2011 - Created. Swati Fursule
 *
 ****************************************************************/
static fbe_status_t fbe_base_config_usurper_memory_allocation_completion(fbe_memory_request_t * memory_request_p, 
                                                          fbe_memory_completion_context_t in_context)
{
    fbe_packet_t*   							sub_packet_p = NULL;
    fbe_packet_t*   							packet_p = NULL;
    fbe_status_t								status;
    fbe_block_transport_block_operation_info_t* new_block_op_info_p = NULL;
    fbe_block_transport_block_operation_info_t* block_op_info_p = NULL; 
    fbe_payload_control_operation_t*			new_control_operation_p = NULL;
    fbe_payload_control_operation_t*			control_operation_p = NULL;
    fbe_payload_ex_t*  						new_payload_p = NULL;
    fbe_payload_ex_t*  						payload_p = NULL;
    fbe_base_object_t*  						base_object_p = NULL;    
    fbe_memory_header_t*						memory_header_p = NULL;
    fbe_memory_header_t*						next_memory_header_p = NULL;
    fbe_sg_element_t *  						sg_list_p = NULL;
	fbe_xor_execute_stamps_t					execute_stamps; 
	fbe_payload_control_operation_opcode_t		control_opcode;
	fbe_block_transport_block_operation_info_t* fake_block_op_info_p = NULL;
	fbe_block_transport_io_operation_info_t * io_operation;

   
    base_object_p = (fbe_base_object_t*)in_context;
    
    // Get the memory that was allocated
    memory_header_p = fbe_memory_get_first_memory_header(memory_request_p);
    new_block_op_info_p = (fbe_block_transport_block_operation_info_t*)fbe_memory_header_to_data(memory_header_p);

    fbe_memory_get_next_memory_header(memory_header_p, &next_memory_header_p);
    sub_packet_p = (fbe_packet_t*)fbe_memory_header_to_data(next_memory_header_p);
    fbe_transport_initialize_sep_packet(sub_packet_p);    

    // get the pointer to original packet.
    packet_p = fbe_transport_memory_request_to_packet(memory_request_p);	
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);    

    // Allocate new payload    
    new_payload_p = fbe_transport_get_payload_ex(sub_packet_p);
    
    /* Set sg list */
    fbe_payload_ex_get_sg_list(payload_p, &sg_list_p, NULL);
    fbe_payload_ex_set_sg_list(new_payload_p, sg_list_p, 1);

	/* Check what kind of control operation we are using */
    fbe_payload_control_get_opcode(control_operation_p, &control_opcode);

	if(control_opcode == FBE_BLOCK_TRANSPORT_CONTROL_CODE_IO_COMMAND){
		status = fbe_base_config_usurper_get_control_buffer((fbe_base_config_t*)base_object_p, packet_p, 
												sizeof(fbe_block_transport_io_operation_info_t),
												(fbe_payload_control_buffer_t)&io_operation);

		fbe_memory_get_next_memory_header(next_memory_header_p, &next_memory_header_p);
		fake_block_op_info_p = (fbe_block_transport_block_operation_info_t*)fbe_memory_header_to_data(next_memory_header_p);

		fbe_payload_block_build_operation(&fake_block_op_info_p->block_operation,
										  io_operation->block_opcode,
										  io_operation->lba,
										  io_operation->block_count,
										  io_operation->block_size,
										  1,
										  NULL);

		fake_block_op_info_p->is_checksum_requiered = io_operation->is_checksum_requiered;

		block_op_info_p = fake_block_op_info_p;
	} else {
		status = fbe_base_config_usurper_get_control_buffer((fbe_base_config_t*)base_object_p, packet_p, 
												sizeof(fbe_block_transport_block_operation_info_t),
												(fbe_payload_control_buffer_t)&block_op_info_p);

	}


    // Get the control buffer pointer from the packet payload.
    if (status != FBE_STATUS_OK) { 
        fbe_transport_destroy_packet(sub_packet_p); 
        fbe_memory_free_request_entry(memory_request_p);

        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }


	/* In some cases we need to calculate checksum */
	if(block_op_info_p->is_checksum_requiered && (block_op_info_p->block_operation.block_opcode == FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE)){
        fbe_xor_lib_execute_stamps_init(&execute_stamps);
        execute_stamps.fru[0].sg = sg_list_p;
        execute_stamps.fru[0].seed = block_op_info_p->block_operation.lba;
        execute_stamps.fru[0].count = block_op_info_p->block_operation.block_count;
        execute_stamps.fru[0].offset = 0;
        execute_stamps.fru[0].position_mask = 0;

        execute_stamps.data_disks = 1;
        execute_stamps.array_width = 1;
        execute_stamps.eboard_p = NULL;
        execute_stamps.eregions_p = NULL;
        execute_stamps.option = FBE_XOR_OPTION_GEN_CRC;
        status = fbe_xor_lib_execute_stamps(&execute_stamps); 

		if (execute_stamps.status != FBE_XOR_STATUS_NO_ERROR){
			fbe_transport_destroy_packet(sub_packet_p); 
			fbe_memory_free_request_entry(memory_request_p);
			fbe_base_object_trace(base_object_p,
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s fbe_xor_lib_execute_stamps Failed status %d\n",
									__FUNCTION__, status);

			fbe_base_object_trace(base_object_p,
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"LBA %llX BS = %llX Data %llX\n",
									block_op_info_p->block_operation.lba, 
									block_op_info_p->block_operation.block_count,
									* (fbe_u64_t*)sg_list_p[0].address);


			fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
			fbe_transport_complete_packet(packet_p);
			return FBE_STATUS_GENERIC_FAILURE; 
		}
	} /* We done with checksums */
   
    new_control_operation_p = fbe_payload_ex_allocate_control_operation(new_payload_p);
    if(new_control_operation_p == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace(base_object_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_ex_allocate_control_operation failed\n",
                                __FUNCTION__);
        fbe_transport_destroy_packet(sub_packet_p); 
        //fbe_memory_request_get_entry_mark_free(memory_request_p, &memory_ptr);
        //fbe_memory_free_entry(memory_ptr);  		
        fbe_memory_free_request_entry(memory_request_p);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }
    
	/* Not the best way to copy structures */
    new_block_op_info_p->block_operation = block_op_info_p->block_operation;
    new_block_op_info_p->verify_error_counts = block_op_info_p->verify_error_counts;

    fbe_payload_control_build_operation(new_control_operation_p,
                                        FBE_BLOCK_TRANSPORT_CONTROL_CODE_BLOCK_COMMAND,
                                        new_block_op_info_p,
                                        sizeof(fbe_block_transport_block_operation_info_t));

    fbe_payload_ex_increment_control_operation_level(new_payload_p);

    /* Set our completion function. */
    fbe_transport_set_completion_function(sub_packet_p, 
                                          fbe_base_config_usurper_subpacket_completion,
                                          base_object_p);

    /* Add the newly created packet in subpacket queue and put the master packet to usurper queue. */
    fbe_transport_add_subpacket(packet_p, sub_packet_p);
    fbe_transport_set_cancel_function(packet_p, fbe_base_object_packet_cancel_function, base_object_p);
    fbe_base_object_add_to_usurper_queue(base_object_p, packet_p);

    status = fbe_base_config_send_block_operation((fbe_base_config_t*)base_object_p, sub_packet_p);
    /*status = fbe_base_config_control_entry(object_handle, sub_packet_p);*/
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_base_config_usurper_memory_allocation_completion()
 **************************************/

/*!**************************************************************
 * fbe_base_config_send_block_operation()
 ****************************************************************
 * @brief
 *  This function sends block operations for this raid group object.
 *
 * @param base_config_p - The Base config object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/01/2010  Swati Fursule  - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_base_config_send_block_operation(fbe_base_config_t * base_config_p, 
                                     fbe_packet_t * packet_p)
{
    fbe_status_t							status = FBE_STATUS_OK;
    
    status = fbe_block_transport_send_block_operation(
             (fbe_block_transport_server_t*)&base_config_p->block_transport_server,
             packet_p,
             (fbe_base_object_t *)base_config_p);
    return status;
}
/**************************************
 * end fbe_base_config_send_block_operation()
 **************************************/

/*!**************************************************************
 * fbe_base_config_usurper_send_event()
 ****************************************************************
 * @brief
 *  This function sends event to upstream object through event service. It is basically
 *  used to test event service and object destroy overlap functionality.
 *
 * @param base_config_p - The Base config object.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  11/07/2011  Amit Dhaduk  - Created.
 *
 ****************************************************************/
static fbe_status_t 
fbe_base_config_usurper_send_event(fbe_base_config_t * base_config_p, 
                                     fbe_packet_t * packet_p)
{

    fbe_base_config_control_send_event_t * event_data_p = NULL;    /* INPUT */
    fbe_event_t *   			event_p = NULL;
    fbe_event_stack_t * 		event_stack = NULL;
    fbe_status_t							status = FBE_STATUS_OK;
    fbe_payload_ex_t * payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;  
    fbe_event_permit_request_t  permit_request = {0};    
    fbe_u32_t   					len;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_base_object_trace( (fbe_base_object_t *)base_config_p,
                      FBE_TRACE_LEVEL_DEBUG_HIGH,
                      FBE_TRACE_MESSAGE_ID_INFO,
                      "%s >>>>\n",
                      __FUNCTION__);


    fbe_payload_control_get_buffer(control_operation_p, &event_data_p);
    if (event_data_p == NULL)
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation_p, &len);
    if (len != sizeof(fbe_base_config_control_send_event_t))
    {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    event_p = fbe_memory_ex_allocate(sizeof(fbe_event_t));
    if(event_p == NULL)
    { 
        /* Do not worry we will send it later */
        fbe_base_object_trace( (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s memory allocation failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet_p, FBE_STATUS_INSUFFICIENT_RESOURCES, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    /* initialize the event. */
    fbe_event_init(event_p);
    fbe_event_set_master_packet(event_p, packet_p);

    if((event_data_p->event_type !=FBE_EVENT_TYPE_PERMIT_REQUEST) || 
        (event_data_p->event_data.permit_request.permit_event_type != FBE_PERMIT_EVENT_TYPE_ZERO_REQUEST))
    {
        fbe_base_object_trace( (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s not supported - event type 0x%x, permit type 0x%x\n",
                                __FUNCTION__,event_data_p->event_type, event_data_p->event_data.permit_request.permit_event_type);


            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            /* Release the event.  */
            fbe_event_release_stack(event_p, event_stack);
            fbe_event_destroy(event_p);
            fbe_memory_ex_release(event_p);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* setup the zero permit request event. */
    permit_request.permit_event_type = event_data_p->event_data.permit_request.permit_event_type;
    fbe_event_set_permit_request_data(event_p, &permit_request);

    /* allocate the event stack. */
    event_stack = fbe_event_allocate_stack(event_p);

    /* fill the LBA range to request for the zero operation. */
    fbe_event_stack_set_extent(event_stack, event_data_p->lba, event_data_p->block_count);

    /* Set medic action priority. */
    fbe_event_set_medic_action_priority(event_p, event_data_p->medic_action_priority);

    /* set the event completion function. */
    fbe_event_stack_set_completion_function(event_stack,
                                            fbe_base_config_send_event_completion,
                                            (fbe_base_object_t *)base_config_p);

    /* send the zero permit request to upstream object. */
    status = fbe_base_config_send_event((fbe_base_config_t *)(fbe_base_object_t *)base_config_p, event_data_p->event_type, event_p);
    if(status != FBE_STATUS_OK)
    {
        fbe_base_object_trace( (fbe_base_object_t *)base_config_p,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s fail status 0x%x\n",
                          __FUNCTION__, status);

        return status;
    }
    
    
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**************************************
 * end fbe_base_config_usurper_send_event()
 **************************************/

/*!****************************************************************************
 * fbe_base_config_send_event_completion()
 ******************************************************************************
 * @brief
 *  This function handles the event completion.
 *  
 * 
 * @param event_p   - Event pointer.
 * @param context_p - Context which was passed with event.
 *
 * @return fbe_status_t
 *  The status of the operation typically FBE_STATUS_OK.
 * @author
 * 11/07/2011 - Created. Amit Dhaduk.
 * 
 ******************************************************************************/
static fbe_status_t 
fbe_base_config_send_event_completion(fbe_event_t * event_p,
                                                              fbe_event_completion_context_t context)
{
    fbe_event_stack_t * 		event_stack = NULL;
    fbe_packet_t *  			packet_p = NULL;
    fbe_base_object_t * 	object_p = NULL;
    fbe_u32_t   				event_flags = 0;
    fbe_event_status_t  		event_status = FBE_EVENT_STATUS_INVALID;
    fbe_payload_ex_t * payload_p = NULL;
    fbe_payload_control_operation_t * control_operation_p = NULL;  

    event_stack = fbe_event_get_current_stack(event_p);
    fbe_event_get_master_packet(event_p, &packet_p);

    object_p = (fbe_base_object_t *) context;

    /* Get flag and status of the event. */
    fbe_event_get_flags (event_p, &event_flags);
    fbe_event_get_status(event_p, &event_status);

    /* Release the event.  */
    fbe_event_release_stack(event_p, event_stack);
    fbe_event_destroy(event_p);
    fbe_memory_ex_release(event_p);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_base_object_trace((fbe_base_object_t*)object_p, 
              FBE_TRACE_LEVEL_DEBUG_HIGH, 
              FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
              "%s status  0x%x\n", __FUNCTION__, event_status );

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;

}
/**************************************
 * end fbe_base_config_send_event_completion()
 **************************************/

/*!****************************************************************************
 * fbe_base_config_usurper_background_op_enable_check()
 ******************************************************************************
 * @brief
 *  This function is used to check if background operation is enabled or not.
 * 
 * @param  base_config  	- pointer to base config object.
 * @param  packet   		- pointer to the packet.
 *
 * @return status   			- status of the operation.
 *
 * @author
 *  08/16/2011 - Created. Amit Dhaduk
 *
 ******************************************************************************/
static fbe_status_t
fbe_base_config_usurper_background_op_enable_check(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t * 							payload_p = NULL;
    fbe_payload_control_operation_t *   			control_operation_p = NULL;
    fbe_base_config_background_op_enable_check_t *  op_enable_check_p = NULL;
    fbe_payload_control_buffer_length_t 			length = 0;
    fbe_bool_t  									b_operation_enabled;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);  

    fbe_payload_control_get_buffer(control_operation_p, &op_enable_check_p);
    if (op_enable_check_p == NULL) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If length of the buffer does not match with operation enable check 
     * information structure lenght then return error.
     */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if(length != sizeof(fbe_base_config_background_op_enable_check_t)) {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload buffer lenght mismatch.\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* check if background operation is enabled or not */
    fbe_base_config_is_background_operation_enabled(base_config_p, op_enable_check_p->background_operation, &b_operation_enabled);

    op_enable_check_p->is_enabled = b_operation_enabled;

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_base_config_usurper_background_op_enable_check()
*******************************************************************************/

/*!****************************************************************************
 * fbe_base_config_usurper_background_operation()
 ******************************************************************************
 * @brief
 *  This function is used to enable/disable background operation.
 * 
 * @param  base_config  	- provision drive object.
 * @param  packet   		- pointer to the packet.
 *
 * @return status   		- status of the operation.
 *
 * @author
 *  08/16/2011 - Created. Amit Dhaduk
 *
 ******************************************************************************/
static fbe_status_t
fbe_base_config_usurper_background_operation(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{

    fbe_payload_ex_t * 						payload_p = NULL;
    fbe_base_config_control_background_op_t*	background_operation_p = NULL;
    fbe_payload_control_operation_t *   		control_operation_p = NULL;
    fbe_payload_control_buffer_length_t 		length = 0;
    fbe_status_t								status;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);  

    fbe_payload_control_get_buffer(control_operation_p, &background_operation_p);
    if (background_operation_p == NULL) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If length of the buffer does not match with control background operation 
     * information structure lenght then return error.
     */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if(length != sizeof(fbe_base_config_control_background_op_t)) {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload buffer lenght mismatch.\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_set_completion_function(packet_p, fbe_base_config_usurper_background_operation_completion, base_config_p);

    if(background_operation_p->enable)
    {
        /* send enable background operation request */    
        status = fbe_base_config_enable_background_operation(base_config_p, packet_p, background_operation_p->background_operation);
    }
    else
    {
        /* send disable background operation request */
        status = fbe_base_config_disable_background_operation(base_config_p, packet_p, background_operation_p->background_operation);
    }

    if ((status != FBE_STATUS_PENDING) && (status != FBE_STATUS_OK)) 
    {
         fbe_base_transport_trace(FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s can't update non paged metadata", __FUNCTION__);
        return status;
    }

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    
}
/******************************************************************************
 * end fbe_base_config_usurper_background_operation()
*******************************************************************************/


/*!****************************************************************************
 * fbe_base_config_usurper_background_operation_completion
 ******************************************************************************
 *
 * @brief
 *    This function handles enable/disable background operation completion. 
 *    It validates the status and complete the operation. 
 *    
 *
 * @param   in_packet_p 		  -  pointer to control packet requesting this
 *  								 operation
 * @param   context 			  -  context, which is a pointer to the base config
 *  								 object
 *
 * @return  fbe_status_t		  -  status of this operation
 *
 * @version
 *    08/16/2011 - Created. Amit Dhaduk
 *
 ******************************************************************************/

static fbe_status_t
fbe_base_config_usurper_background_operation_completion(fbe_packet_t * packet_p, fbe_packet_completion_context_t context)
{

    fbe_base_config_t * 				base_config_p = NULL;
    fbe_payload_ex_t * 				sep_payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    fbe_payload_control_status_t		control_status = FBE_PAYLOAD_CONTROL_STATUS_OK;
    fbe_status_t						status;  
    
    base_config_p = (fbe_base_config_t *)context;

     /* Get sep payload. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);

    /* Get control operation. */
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    /* Get the packet status */
    status = fbe_transport_get_status_code(packet_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_config_p, 
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s background operation enable/disable failed, status:%d.",
                                __FUNCTION__, status);
        control_status = FBE_PAYLOAD_CONTROL_STATUS_FAILURE;

        fbe_payload_control_set_status(control_operation_p, control_status);
        fbe_transport_set_status(packet_p, status, 0);
    }
    
    return status;

} 
/******************************************************************************
 * end fbe_base_config_usurper_background_operation_completion()
*******************************************************************************/


static fbe_status_t fbe_base_config_usurper_disable_cmi(fbe_base_config_t *base_config_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t * 				sep_payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    
    /* Get sep payload. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);

    /* Get control operation. */
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    fbe_metadata_element_disable_cmi(&base_config_p->metadata_element);

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_base_config_usurper_control_system_bg_service()
 ****************************************************************
 * @brief
 *  This function is used to control system background service flag.
 *  To handle FBE_BASE_CONFIG_CONTROL_CODE_CONTROL_SYSTEM_BG_SERVICE
 *
 * @param packet_p - the packet.
 *
 * @return fbe_status_t   
 *
 * @author
 *  02/29/2012 - Created. Vera Wang
 *
 ****************************************************************/
fbe_status_t fbe_base_config_usurper_control_system_bg_service(fbe_packet_t * packet_p)
{
    fbe_base_config_control_system_bg_service_t * 		system_bg_service_p = NULL;    /* INPUT */
    fbe_status_t 										status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t * 									payload = NULL;
    fbe_payload_control_operation_t * 					control_operation = NULL;
    fbe_payload_control_buffer_length_t 				len = 0;
    
    fbe_base_config_class_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s entry\n", __FUNCTION__);

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &system_bg_service_p);
    if (system_bg_service_p == NULL){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s fbe_payload_control_get_buffer failed\n",
                                    __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);    
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_base_config_control_system_bg_service_t)){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Invalid length:%d\n",
                                    __FUNCTION__, len);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
    status = fbe_base_config_control_system_bg_service(system_bg_service_p);
    
    if(status != FBE_STATUS_OK){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s failed to control system background services flags\n",
                                    __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}
/******************************************************************************
 * end fbe_base_config_usurper_control_system_bg_service()
*******************************************************************************/

/*!**************************************************************
 * fbe_base_config_usurper_get_system_bg_service_status()
 ****************************************************************
 * @brief
 *  This function is used to get system background service status.
 *  To handle FBE_BASE_CONFIG_CONTROL_CODE_GET_SYSTEM_BG_SERVICE_STATUS
 *
 * @param packet_p - the packet.
 *
 * @return fbe_status_t   
 *
 * @author
 *  02/29/2012 - Created. Vera Wang
 *
 ****************************************************************/
fbe_status_t fbe_base_config_usurper_get_system_bg_service_status(fbe_packet_t * packet_p)
{
    fbe_base_config_control_system_bg_service_t * 		system_bg_service_p = NULL;    /* INPUT */
    fbe_status_t 										status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t * 									payload = NULL;
    fbe_payload_control_operation_t * 					control_operation = NULL;
    fbe_payload_control_buffer_length_t 				len = 0;
    
    fbe_base_config_class_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s entry\n", __FUNCTION__);

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &system_bg_service_p);
    if (system_bg_service_p == NULL){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s fbe_payload_control_get_buffer failed\n",
                                    __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);    
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof(fbe_base_config_control_system_bg_service_t)){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Invalid length:%d\n",
                                    __FUNCTION__, len);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
    status = fbe_base_config_get_system_bg_service_status(system_bg_service_p);
    
    if(status != FBE_STATUS_OK){
        fbe_base_config_class_trace(FBE_TRACE_LEVEL_WARNING,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s failed to get system background services flags\n",
                                    __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }

    /*we set this to true so the user know that at least once, someone enabled all the background services,
    which is what we expect from the managament layers to do when they start up*/
    system_bg_service_p->enabled_externally = fbe_base_config_all_bgs_started_externally();

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}
/******************************************************************************
 * end fbe_base_config_usurper_get_system_bg_service()
*******************************************************************************/


/*!**************************************************************
 * fbe_base_config_control_set_nonpaged_metadata_size()
 ****************************************************************
 * @brief
 *  This function is used to set the non-paged metadta size stored in
 *  the metadata element of base_config object. It is only used for test
 *  and debuging. Never use it directly in any other scenarios !!!!!!!
 *
 * @param packet_p - the packet.
 *
 * @return fbe_status_t   
 *
 * @author
 *  05/02/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
static fbe_status_t fbe_base_config_control_set_nonpaged_metadata_size(fbe_base_config_t * base_config, fbe_packet_t * packet)
{
    fbe_base_config_control_metadata_set_size_t *   set_metadata_info = NULL;    /* INPUT */
    fbe_payload_ex_t * 								payload = NULL;
    fbe_payload_control_operation_t *   			control_operation = NULL;
    fbe_u32_t   									len = 0;
    fbe_u32_t                                       nonpaged_metadata_size = 0;
    fbe_base_config_nonpaged_metadata_t *   		base_config_nonpaged_metadata_p = NULL;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &set_metadata_info);
    if (set_metadata_info == NULL) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if(len != sizeof (fbe_base_config_control_metadata_set_size_t)){
        fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer_length failed\n",
                                __FUNCTION__);

        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;

    }

    /* let's get the non-paged metadata of this object. */
    fbe_base_config_get_nonpaged_metadata_ptr(base_config, (void **)&base_config_nonpaged_metadata_p);
    fbe_base_config_get_nonpaged_metadata_size(base_config, &nonpaged_metadata_size);

    /*update the non-paged metadata size in metadata element*/
    base_config->metadata_element.nonpaged_record.data_size = set_metadata_info->new_size;
    set_metadata_info->old_size = nonpaged_metadata_size;
    if (set_metadata_info->set_ver_header) {
        if (base_config_nonpaged_metadata_p != NULL) {
            base_config_nonpaged_metadata_p->version_header.size = set_metadata_info->new_size;
        }
    }

    /*call metadata service to change the size*/
    fbe_base_config_metadata_nonpaged_init(base_config, set_metadata_info->new_size, packet);

    return FBE_STATUS_OK;
}

static fbe_status_t
fbe_base_config_control_passive_request(fbe_base_config_t * base_config, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;

    fbe_base_object_trace(  (fbe_base_object_t *)base_config,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    fbe_base_config_passive_request(base_config);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);

    return status;
}
/*!****************************************************************************
 * fbe_base_config_write_verify_non_paged_metadata_without_NP_lock()
 ******************************************************************************
 * @brief
 *   This function performs nonpaged MD rebuild. 
 *   it writes/verifies object's nonpaged MD to the raw mirror without NP lock.
 *   Some object may not have NP lock when do non paged metadata write/verify,
 *   such as LUN object, so we perform the operation without NP lock.
 *
 * @param base_object_p      - pointer to the base object
 * @param packet                    - control packet
 *
 * @return fbe_lifecycle_status_t   - 
 * @author
 *   05/24/12  - Created. zhangy26
 *
 ******************************************************************************/
fbe_status_t fbe_base_config_write_verify_non_paged_metadata_without_NP_lock(fbe_base_config_t *base_config_p,fbe_packet_t * packet_p)
{
    fbe_base_config_write_verify_context_t *	        base_config_write_verify_context = NULL;
    fbe_base_config_nonpaged_metadata_t *               base_config_nonpaged_metadata_p = NULL;
    fbe_status_t										status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *									payload = NULL;
    fbe_payload_control_operation_t *					control_operation = NULL;
    fbe_bool_t is_nonpaged_initialized = FBE_FALSE;	
    fbe_base_config_write_verify_info_t *	            control_write_verify_info = NULL;
    fbe_payload_control_buffer_length_t len = 0;
    fbe_lifecycle_state_t   						lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);  

    fbe_payload_control_get_buffer(control_operation, &control_write_verify_info);
    if (control_write_verify_info == NULL) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_base_config_write_verify_info_t))
    {
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X length invalid\n", len);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_config_get_nonpaged_metadata_ptr(base_config_p, (void **)&base_config_nonpaged_metadata_p);
        
    if(base_config_nonpaged_metadata_p == NULL){ 
        /* The nonpaged pointers are not initialized yet */
        control_write_verify_info->need_clear_rebuild_count = FBE_TRUE;
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
    fbe_base_config_metadata_nonpaged_is_initialized(base_config_p, &is_nonpaged_initialized);
        
    if(is_nonpaged_initialized == FBE_FALSE)
    { 
        /* The nonpaged metadata are not initialized yet */
        control_write_verify_info->need_clear_rebuild_count = FBE_TRUE;			
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
    /* perform write/verify only we are out of SPECIALIZED state.
        If we are in SPECIALIZED state, here is a race condition between
        stripe lock init and write/verify get stripe lock*/
    status = fbe_base_object_get_lifecycle_state((fbe_base_object_t *)base_config_p, &lifecycle_state);
    if(lifecycle_state == FBE_LIFECYCLE_STATE_SPECIALIZE ||
       status != FBE_STATUS_OK)
    {
       control_write_verify_info->write_verify_operation_retryable_b = FBE_TRUE;
       control_write_verify_info->write_verify_op_success_b = FBE_FALSE;
       fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
       fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
       fbe_transport_complete_packet(packet_p);
       return FBE_STATUS_OK;
    }
    base_config_write_verify_context = (fbe_base_config_write_verify_context_t *)fbe_transport_allocate_buffer();
    if(base_config_write_verify_context == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)base_config_p, FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: FAILED to allocated buffer\n", __FUNCTION__);
        
        /*buffer allocated failed. we should keep the write_verify_count and retry it*/
        control_write_verify_info->write_verify_operation_retryable_b = FBE_TRUE;
        control_write_verify_info->write_verify_op_success_b = FBE_FALSE;
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }
    base_config_init_write_verify_context(base_config_write_verify_context,base_config_p);
        
    /*only active side can  do nonpaged MD write verify*/
    if (fbe_base_config_is_active(base_config_p) ){
        fbe_transport_set_completion_function(packet_p, base_config_write_verify_non_paged_metadata_completion, base_config_write_verify_context);
        fbe_base_config_ex_metadata_nonpaged_write_verify(base_config_write_verify_context, packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }else{
         /*clear the weite verify count*/
        control_write_verify_info->need_clear_rebuild_count = FBE_TRUE;		 
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_release_buffer(base_config_write_verify_context);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_OK;
    }


}


static fbe_status_t 
fbe_base_config_usurper_get_stripe_blob(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_base_config_control_get_stripe_blob_t * blob = NULL;
    fbe_u32_t len;

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &blob);
    if (blob == NULL) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_base_config_control_get_stripe_blob_t))
    {
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%X length invalid\n", len);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    blob->size = base_config_p->metadata_element.stripe_lock_blob->size; 
    blob->write_count = base_config_p->metadata_element.stripe_lock_blob->write_count;
    blob->user_slot_size = base_config_p->metadata_element.stripe_lock_blob->user_slot_size;
    blob->private_slot = base_config_p->metadata_element.stripe_lock_blob->private_slot;
    blob->nonpaged_slot = base_config_p->metadata_element.stripe_lock_blob->nonpaged_slot;
    blob->flags = base_config_p->metadata_element.stripe_lock_blob->flags;

    if(blob->size > FBE_MEMORY_CHUNK_SIZE){
        fbe_copy_memory(blob->slot, base_config_p->metadata_element.stripe_lock_blob->slot, FBE_MEMORY_CHUNK_SIZE);
    } else {
        fbe_copy_memory(blob->slot, base_config_p->metadata_element.stripe_lock_blob->slot, blob->size);
    }


    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}


static fbe_status_t fbe_base_config_usurper_get_peer_lifecycle_state(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_base_config_get_peer_lifecycle_state_t * get_state = NULL;
    fbe_u32_t len;
    fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &get_state);
    if (get_state == NULL) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &len);
    if (len != sizeof(fbe_base_config_get_peer_lifecycle_state_t)){
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s %X length invalid\n", __FUNCTION__, len);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_base_config_metadata_get_peer_lifecycle_state(base_config_p, &get_state->peer_state); 

    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s can't get peer state, status: %d\n", __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;


}

/*!****************************************************************************
 * fbe_base_config_usurper_get_encryption_state
 ******************************************************************************
 *
 * @brief
 *    This function returns the encryption state for the object. 
 *    
 *
 * @param   in_packet_p 		  -  pointer to control packet requesting this
 *  								 operation
 * @param   context 			  -  context, which is a pointer to the base config
 *  								 object
 *
 * @return  fbe_status_t		  -  status of this operation
 *
 * @version
 *    09/28/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t fbe_base_config_usurper_get_encryption_state(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_base_config_control_get_encryption_state_t * get_encryption_state = NULL;
    fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    status = fbe_base_config_usurper_get_control_buffer(base_config_p, packet_p,
                                                        sizeof(fbe_base_config_control_get_encryption_state_t),
                                                        (fbe_payload_control_buffer_t)&get_encryption_state);

    status = fbe_base_config_get_encryption_state(base_config_p, &get_encryption_state->encryption_state);

    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s can't get encryption state, status: %d\n", __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_base_config_usurper_set_encryption_state
 ******************************************************************************
 *
 * @brief
 *    This function sets the encryption state for the object. 
 *    
 *
 * @param   in_packet_p 		  -  pointer to control packet requesting this
 *  								 operation
 * @param   context 			  -  context, which is a pointer to the base config
 *  								 object
 *
 * @return  fbe_status_t		  -  status of this operation
 *
 * @version
 *    09/28/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t fbe_base_config_usurper_set_encryption_state(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_base_config_control_set_encryption_state_t * set_encryption_state = NULL;
    fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;

    /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    status = fbe_base_config_usurper_get_control_buffer(base_config_p, packet_p,
                                                        sizeof(fbe_base_config_control_set_encryption_state_t),
                                                        (fbe_payload_control_buffer_t)&set_encryption_state);

    status = fbe_base_config_set_encryption_state(base_config_p, set_encryption_state->encryption_state);

    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s can't set encryption state, status: %d\n", __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_base_config_usurper_set_encryption_mode
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
 * @version
 *    09/28/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t fbe_base_config_usurper_set_encryption_mode(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_base_config_control_encryption_mode_t * set_encryption_mode = NULL;
    fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;


     /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    status = fbe_base_config_usurper_get_control_buffer(base_config_p, packet_p,
                                                        sizeof(fbe_base_config_control_encryption_mode_t),
                                                        (fbe_payload_control_buffer_t)&set_encryption_mode);

    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s set encryption mode to %d\n", __FUNCTION__, set_encryption_mode->encryption_mode);
    status = fbe_base_config_set_encryption_mode(base_config_p, set_encryption_mode->encryption_mode);

    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s can't get encryption state, status: %d\n", __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_base_config_usurper_get_encryption_mode
 ******************************************************************************
 *
 * @brief
 *    This function gets the encryption mode for the object. 
 *    
 *
 * @param   in_packet_p 		  -  pointer to control packet requesting this
 *  								 operation
 * @param   context 			  -  context, which is a pointer to the base config
 *  								 object
 *
 * @return  fbe_status_t		  -  status of this operation
 *
 * @version
 *    09/28/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t fbe_base_config_usurper_get_encryption_mode(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_base_config_control_encryption_mode_t * get_encryption_mode = NULL;
    fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;


     /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    status = fbe_base_config_usurper_get_control_buffer(base_config_p, packet_p,
                                                        sizeof(fbe_base_config_control_encryption_mode_t),
                                                        (fbe_payload_control_buffer_t)&get_encryption_mode);

    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    status = fbe_base_config_get_encryption_mode(base_config_p, &get_encryption_mode->encryption_mode);

    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s can't get encryption state, status: %d\n", __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_base_config_usurper_get_generation_number
 ******************************************************************************
 *
 * @brief
 *    This function gets the generation number  for the object. 
 *    
 *
 * @param   in_packet_p 		  -  pointer to control packet requesting this
 *  								 operation
 * @param   context 			  -  context, which is a pointer to the base config
 *  								 object
 *
 * @return  fbe_status_t		  -  status of this operation
 *
 * @version
 *    09/28/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t fbe_base_config_usurper_get_generation_number(fbe_base_config_t * base_config_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                  payload = NULL;
    fbe_payload_control_operation_t *   control_operation = NULL;
    fbe_base_config_object_get_generation_number_t * generation_num_p = NULL;
    fbe_status_t						status = FBE_STATUS_GENERIC_FAILURE;


     /*some sanity checks*/
    payload = fbe_transport_get_payload_ex(packet_p);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    status = fbe_base_config_usurper_get_control_buffer(base_config_p, packet_p,
                                                        sizeof(fbe_base_config_object_get_generation_number_t),
                                                        (fbe_payload_control_buffer_t)&generation_num_p);

    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace(  (fbe_base_object_t *)base_config_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n",
                                __FUNCTION__);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    status = fbe_base_config_get_generation_num(base_config_p, &generation_num_p->generation_number);

    if (status != FBE_STATUS_OK){
        fbe_base_object_trace((fbe_base_object_t *) base_config_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                              "%s can't get generation number, status: %d\n", __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_base_config_usurper_disable_peer_object(fbe_base_config_t *base_config_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t * 				sep_payload_p = NULL;
    fbe_payload_control_operation_t *   control_operation_p = NULL;
    
    /* Get sep payload. */
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);

    /* Get control operation. */
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    fbe_base_config_set_flag(base_config_p, FBE_BASE_CONFIG_FLAG_PEER_NOT_ALIVE);
    fbe_metadata_element_disable_cmi(&base_config_p->metadata_element);

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);

    return FBE_STATUS_OK;
}


/******************************
 * end fbe_base_config_usurper.c
 ******************************/

