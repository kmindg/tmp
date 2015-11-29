/***************************************************************************
 * Copyright (C) EMC Corporation 2007-2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_virtual_drive_usurper.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the virtual_drive object code for the usurper.
 * 
 * @ingroup virtual_drive_class_files
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
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_transport.h"
#include "fbe_virtual_drive_private.h"
#include "fbe_raid_group_object.h"
#include "fbe_base_config_private.h"
#include "fbe_mirror_private.h"
#include "fbe_transport_memory.h"
#include "fbe_service_manager.h"
#include "fbe_spare.h"
#include "fbe/fbe_virtual_drive.h"
#include "fbe/fbe_physical_drive.h"
#include "fbe_raid_library.h"
#include "fbe_private_space_layout.h"

/*****************************************
 * LOCAL FUNCTION DEFINITIONS
 *****************************************/
static fbe_status_t fbe_virtual_drive_create_edge(fbe_virtual_drive_t * virtual_drive_p,
                                                  fbe_packet_t * packet_p);

static fbe_status_t fbe_virtual_drive_destroy_edge(fbe_virtual_drive_t * virtual_drive_p,
                                                  fbe_packet_t * packet_p);

static fbe_status_t fbe_virtual_drive_send_attach_edge(fbe_virtual_drive_t *virtual_drive_p,
                                                       fbe_block_transport_control_attach_edge_t * attach_edge) ;

static fbe_status_t fbe_virtual_drive_sem_completion(fbe_packet_t * packet,
                                                     fbe_packet_completion_context_t context);

static fbe_status_t fbe_virtual_drive_get_spare_info(fbe_virtual_drive_t * virtual_drive_p,
                                                     fbe_packet_t * packet_p);

static fbe_status_t fbe_virtual_drive_get_spare_provision_drive_info_completion(fbe_packet_t * packet_p, 
                                                                fbe_packet_completion_context_t context);

static fbe_status_t fbe_virtual_drive_get_configuration(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_virtual_drive_validate_swap_in_request(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_virtual_drive_validate_swap_out_request(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_virtual_drive_get_checkpoint(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_virtual_drive_update_logical_error_stats(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_virtual_drive_clear_logical_errors(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p);

static fbe_status_t
fbe_virtual_drive_get_unused_drive_as_spare_flag(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p);

static fbe_status_t
fbe_virtual_drive_set_unused_drive_as_spare_flag(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_virtual_drive_download_ack(fbe_virtual_drive_t *virtual_drive_p, fbe_packet_t *packet_p);

static fbe_status_t fbe_virtual_drive_set_perm_spare_bit(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_virtual_drive_initiate_user_copy(fbe_virtual_drive_t *virtual_drive_p, 
                                                         fbe_packet_t *packet_p);

static fbe_status_t fbe_virtual_drive_usurper_copy_complete_set_checkpoint_to_end_marker(fbe_virtual_drive_t *virtual_drive_p, 
                                                                                         fbe_packet_t *packet_p);

static fbe_status_t fbe_virtual_drive_usurper_copy_failed_set_checkpoint_to_end_marker(fbe_virtual_drive_t *virtual_drive_p, 
                                                                                       fbe_packet_t *packet_p);

static fbe_status_t fbe_virtual_drive_parent_mark_needs_rebuild_done(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p);

static fbe_status_t fbe_virtual_drive_send_request_to_mark_consumed_completion(fbe_packet_t * packet_p, 
                                                                fbe_packet_completion_context_t context);

static fbe_status_t fbe_virtual_drive_get_virtual_drive_debug_flags(fbe_virtual_drive_t * virtual_drive_p, 
                                                                    fbe_packet_t * packet_p);

static fbe_status_t fbe_virtual_drive_set_virtual_drive_debug_flags(fbe_virtual_drive_t * virtual_drive_p, 
                                                                    fbe_packet_t * packet_p);

static fbe_status_t fbe_virtual_drive_get_info(fbe_virtual_drive_t * virtual_drive_p, 
                                               fbe_packet_t * packet_p);

static fbe_status_t fbe_virtual_drive_usurper_send_swap_command_complete(fbe_virtual_drive_t *virtual_drive_p, 
                                                                         fbe_packet_t *packet_p);

static fbe_status_t fbe_virtual_drive_usurper_set_nonpaged_metadata(fbe_virtual_drive_t *virtual_drive_p, 
                                                                    fbe_packet_t *packet_p);

static fbe_status_t fbe_virtual_drive_usurper_update_drive_keys(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_virtual_drive_usurper_register_drive_keys(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_virtual_drive_swap_out_remove_keys(fbe_virtual_drive_t * virtual_drive_p, 
                                                           fbe_packet_t * packet_p, 
                                                           fbe_edge_index_t swap_out_edge_index);
static fbe_status_t fbe_virtual_drive_init_encryption_state(fbe_virtual_drive_t * virtual_drive_p, 
                                                            fbe_bool_t swap_out);
static fbe_status_t fbe_virtual_drive_usurper_start_encryption(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p);
static fbe_status_t fbe_virtual_drive_handle_swap_request_rollback(fbe_virtual_drive_t *virtual_drive_p, 
                                                                   fbe_packet_t *packet_p);
static fbe_status_t fbe_virtual_drive_propagate_block_geometry(fbe_virtual_drive_t *virtual_drive_p);
static fbe_status_t fbe_virtual_drive_usurper_get_dieh_media_threshold(fbe_virtual_drive_t *virtual_drive_p, fbe_packet_t *packet_p);

/*!****************************************************************************
 * fbe_virtual_drive_set_configuration()
 ******************************************************************************
 * @brief
 *  This function sets up the basic configuration info
 *  for this virtual drive object.
 *
 * @param virtual_drive_p - Pointer to virtual drive object
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/02/2009  Ron Proulx  - Created
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_set_configuration(fbe_virtual_drive_t *virtual_drive_p, fbe_packet_t *packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t  current_vd_configuration_mode;
    fbe_virtual_drive_configuration_t      *set_config_p = NULL;    /* INPUT */
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;


    fbe_base_object_trace(  (fbe_base_object_t *)virtual_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);  

    status = fbe_virtual_drive_usurper_get_control_buffer(virtual_drive_p, packet_p,
                                                          sizeof(fbe_virtual_drive_configuration_t),
                                                          (fbe_payload_control_buffer_t *)&set_config_p);
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* We cannot set the configuration mode as unknown. */
    if (set_config_p->configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s User/Configuration service must provide the valid configuration mode.\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Get the lock since we are modifying the configuration mode.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &current_vd_configuration_mode);
    fbe_virtual_drive_lock(virtual_drive_p);

    /* Configure virtual drive object with width as 2 always. */
    fbe_base_config_set_width((fbe_base_config_t*)virtual_drive_p, 2);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s: mode: %d new mode: %d\n",
                          __FUNCTION__, current_vd_configuration_mode, set_config_p->configuration_mode);

    /*! Virtual drive configuration mode gets set when it is get created and it
     * changes dynamically for the sparing requirement.
     */
    fbe_virtual_drive_set_configuration_mode(virtual_drive_p, set_config_p->configuration_mode); 
    fbe_virtual_drive_set_new_configuration_mode(virtual_drive_p, FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN);

    /*! Set the new default offset.
     */
    fbe_virtual_drive_set_default_offset(virtual_drive_p, set_config_p->vd_default_offset);

    fbe_virtual_drive_unlock(virtual_drive_p);

    /* Complete the packet with good status. */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_set_configuration()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_update_configuration()
 ******************************************************************************
 * @brief
 *  This function is used to update the virtual drive configuration.
 *
 * @param virtual_drive_p   - Pointer to virtual drive object
 * @param packet_p          - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  04/21/2009  Dhaval Patel
 ******************************************************************************/
static fbe_status_t
fbe_virtual_drive_update_configuration(fbe_virtual_drive_t *virtual_drive_p,
                                            fbe_packet_t * packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_t  *update_config_p = NULL;    /* INPUT */
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_virtual_drive_configuration_mode_t current_configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;

    fbe_base_object_trace(  (fbe_base_object_t *)virtual_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);  

    status = fbe_virtual_drive_usurper_get_control_buffer(virtual_drive_p, packet_p,
                                                          sizeof(fbe_virtual_drive_configuration_t),
                                                          (fbe_payload_control_buffer_t *)&update_config_p);
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* We cannot allow to update the configuration mode with unknown. */
    if (update_config_p->configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s User/Configuration service must provide the valid configuration mode.\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /*! @note This is how we prevent the copy from starting until the copy job
     *        is complete:
     *          1. When a job starts the `request in progress' flag is set
     *          2. The virtual drive background monitor condition cannot and
     *             should not run any background monitor operation (including 
     *             copy) while there is a `request in-progress' since things
     *             can be changing while there is a job in progress.
     *          3. When the job completes `request in-progress' is cleared
     *             and this allows the copy progress to proceed.  
     */
    fbe_virtual_drive_lock(virtual_drive_p);
    fbe_virtual_drive_set_new_configuration_mode(virtual_drive_p, update_config_p->configuration_mode); 
    fbe_virtual_drive_unlock(virtual_drive_p);

    /* Set the appropriate conditions etc.
     */
    status = fbe_virtual_drive_swap_change_configuration_mode(virtual_drive_p);
    if (status != FBE_STATUS_OK)
    {
        /* Unexpected failure when changing the configuration mode.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s change config failed - status: 0x%x\n",
                                __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Generate a trace that the `new' configuration mode has been set.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: usurper update config mode: %d updated new mode to: %d \n",
                          current_configuration_mode, update_config_p->configuration_mode);


    /* Complete the packet with good status. */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_update_configuration_mode()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_control_entry()
 ******************************************************************************
 * @brief
 *  This function is the management packet entry point for
 *  operations to the virtual_drive object.
 *
 * @param object_handle - The config handle.
 * @param packet_p - The packet that is arriving.
 *
 * @return fbe_status_t
 *
 ******************************************************************************/
fbe_status_t 
fbe_virtual_drive_control_entry(fbe_object_handle_t object_handle, 
                                fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_virtual_drive_t                    *virtual_drive_p = NULL;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_p = NULL;
    fbe_payload_control_operation_opcode_t  opcode;
    fbe_edge_index_t                        edge_index;
    fbe_block_edge_t                       *block_edge_p = NULL;
    fbe_edge_index_t                        upstream_edge_index;
    fbe_object_id_t                         upstream_object_id;
    fbe_block_edge_t                       *upstream_edge_p = NULL;

    /* If object handle is NULL then use the virtual drive class control
     * entry.
     */
    if(object_handle == NULL){
        return  fbe_virtual_drive_class_control_entry(packet_p);
    }

    virtual_drive_p = (fbe_virtual_drive_t *)fbe_base_handle_to_pointer(object_handle);
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_opcode(control_p, &opcode);

    switch (opcode)
    {
        /* virtual_drive specific handling here. */
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_SET_CONFIGURATION:
            status = fbe_virtual_drive_set_configuration(virtual_drive_p, packet_p);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_INITIATE_USER_COPY:
            status = fbe_virtual_drive_initiate_user_copy(virtual_drive_p, packet_p);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_UPDATE_CONFIGURATION:
            status = fbe_virtual_drive_update_configuration(virtual_drive_p, packet_p);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_SPARE_INFO:
            status = fbe_virtual_drive_get_spare_info(virtual_drive_p, packet_p);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_COPY_COMPLETE_SET_CHECKPOINT_TO_END_MARKER:
            status = fbe_virtual_drive_usurper_copy_complete_set_checkpoint_to_end_marker(virtual_drive_p, packet_p);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_COPY_FAILED_SET_CHECKPOINT_TO_END_MARKER:
            status = fbe_virtual_drive_usurper_copy_failed_set_checkpoint_to_end_marker(virtual_drive_p, packet_p);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_SWAP_REQUEST_COMPLETE:
            status = fbe_virtual_drive_handle_swap_request_completion(virtual_drive_p, packet_p);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_CONFIGURATION:
            status = fbe_virtual_drive_get_configuration(virtual_drive_p, packet_p);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_CHECKPOINT:
            status = fbe_virtual_drive_get_checkpoint(virtual_drive_p, packet_p);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_SWAP_IN_VALIDATION:
            status = fbe_virtual_drive_validate_swap_in_request(virtual_drive_p, packet_p);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_SWAP_OUT_VALIDATION:
            status = fbe_virtual_drive_validate_swap_out_request(virtual_drive_p, packet_p);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_UNUSED_DRIVE_AS_SPARE_FLAG:
            status = fbe_virtual_drive_get_unused_drive_as_spare_flag(virtual_drive_p, packet_p);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_SET_UNUSED_DRIVE_AS_SPARE_FLAG:
            status = fbe_virtual_drive_set_unused_drive_as_spare_flag(virtual_drive_p, packet_p);
            break;    
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_CREATE_EDGE:
            status = fbe_virtual_drive_create_edge(virtual_drive_p, packet_p);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_DESTROY_EDGE:
            status = fbe_virtual_drive_destroy_edge(virtual_drive_p, packet_p);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_SET_NONPAGED_PERM_SPARE_BIT:
            status = fbe_virtual_drive_set_perm_spare_bit(virtual_drive_p, packet_p);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_MARK_NEEDS_REBUILD_DONE:
            status = fbe_virtual_drive_parent_mark_needs_rebuild_done(virtual_drive_p, packet_p);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_UPDATE_LOGICAL_ERROR_STATS:
            status = fbe_virtual_drive_update_logical_error_stats(virtual_drive_p, packet_p);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_CLEAR_LOGICAL_ERRORS:
            status = fbe_virtual_drive_clear_logical_errors(virtual_drive_p, packet_p);
            break;
        case FBE_PROVISION_DRIVE_CONTROL_CODE_DOWNLOAD_ACK:
            status = fbe_virtual_drive_download_ack(virtual_drive_p, packet_p);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_DEBUG_FLAGS:
            status = fbe_virtual_drive_get_virtual_drive_debug_flags(virtual_drive_p, packet_p);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_SET_DEBUG_FLAGS:
            status = fbe_virtual_drive_set_virtual_drive_debug_flags(virtual_drive_p, packet_p);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_GET_INFO:
            status = fbe_virtual_drive_get_info(virtual_drive_p, packet_p);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_SEND_SWAP_COMMAND_COMPLETE:
            status = fbe_virtual_drive_usurper_send_swap_command_complete(virtual_drive_p, packet_p);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_SWAP_REQUEST_ROLLBACK:
            status = fbe_virtual_drive_handle_swap_request_rollback(virtual_drive_p, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_METADATA_SET_NONPAGED:
            status = fbe_virtual_drive_usurper_set_nonpaged_metadata(virtual_drive_p, packet_p);
            break;
        case FBE_BLOCK_TRANSPORT_CONTROL_CODE_EXIT_HIBERNATION:
            /* Get the upstream edge since the usurper will clear the upstream
             * hibernation attribute.
             */
            fbe_block_transport_find_first_upstream_edge_index_and_obj_id(&virtual_drive_p->spare.raid_group.base_config.block_transport_server,
                                                                  &upstream_edge_index, &upstream_object_id);
            if (upstream_object_id != FBE_OBJECT_ID_INVALID)
            {
                upstream_edge_p = (fbe_block_edge_t *)fbe_base_transport_server_get_client_edge_by_client_id((fbe_base_transport_server_t *)&virtual_drive_p->spare.raid_group.base_config.block_transport_server,
                                                                             upstream_object_id);
                packet_p->base_edge = (fbe_base_edge_t *)upstream_edge_p;
            }
            status = fbe_mirror_control_entry(object_handle, packet_p);
            break;
        case FBE_BASE_CONFIG_CONTROL_CODE_UPDATE_DRIVE_KEYS:
            status = fbe_virtual_drive_usurper_update_drive_keys(virtual_drive_p, packet_p);
            break;
        case FBE_PROVISION_DRIVE_CONTROL_CODE_REGISTER_KEYS:
            status = fbe_virtual_drive_usurper_register_drive_keys(virtual_drive_p, packet_p);
            break;
        case FBE_VIRTUAL_DRIVE_CONTROL_CODE_ENCRYPTION_START:
            status = fbe_virtual_drive_usurper_start_encryption(virtual_drive_p, packet_p);
            break;
        case FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DIEH_MEDIA_THRESHOLD:
            status = fbe_virtual_drive_usurper_get_dieh_media_threshold(virtual_drive_p, packet_p);
            break;

        default:
            /* Allow the raid group mirror object to handle all other ioctls. */
            status = fbe_mirror_control_entry(object_handle, packet_p);

            /* If traverse flag is set then send the control packet down to block edge. */
            if (status == FBE_STATUS_TRAVERSE)
            {
                /*!@todo If packet provides the details about the which edge to send the
                 * request then we will use the edge index from the packet else it will
                 * alway send the request to primary edge for the mirror.
                 */

                /* get the primary edge index, if configuration mode is mirror then it will
                * return the primary edge index (original drive) of the mirror.
                */
                fbe_virtual_drive_get_primary_edge_index(virtual_drive_p, &edge_index);

                /* get the block edge pointer for the corresponding edge to forward the packet. */
                fbe_base_config_get_block_edge((fbe_base_config_t *) virtual_drive_p, &block_edge_p, edge_index);
                status = fbe_block_transport_send_control_packet(block_edge_p, packet_p);
            }
            else
            {
                /* Else handle those control code where specific virtual drive 
                 * processing is required.
                 */
                switch(opcode)
                {
                    case FBE_BLOCK_TRANSPORT_CONTROL_CODE_ATTACH_EDGE:
                        /* Propagate any edge attributes upstream.
                         */
                        fbe_virtual_drive_attribute_changed(virtual_drive_p, NULL);

                        break;

                    default:
                        /* Nothing to do.
                         */
                        break;
                }
            }
            break;
    }

    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_control_entry()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_create_edge()
 ******************************************************************************
 * @brief
 * This function is used to create edge and attach the edge with downstream 
 * object, it uses base config functionality for creating edge and attaching 
 * edge with downstream object.
 *
 * Once edge gets created and attached it will set the different condition based
 * on the number of edges attached with downstream object to start certain 
 * operation.
 *
 * @param virtual_drive_p - The virtual drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/10/2009 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_create_edge(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t *packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
	fbe_payload_ex_t                           *payload_p = NULL;
	fbe_payload_control_operation_t            *control_operation_p = NULL;  
	fbe_block_transport_control_create_edge_t  *create_edge_p = NULL;    /* INPUT */
    fbe_base_config_path_state_counters_t       path_state_counters;
    fbe_path_state_t                            first_edge_path_state = FBE_PATH_STATE_INVALID;
    fbe_path_state_t                            second_edge_path_state = FBE_PATH_STATE_INVALID;
    fbe_virtual_drive_configuration_mode_t      configuration_mode;
    fbe_block_edge_t                           *block_edge_p = NULL;
    fbe_edge_index_t                            swap_in_edge_index = FBE_EDGE_INDEX_INVALID;

    /* Populate the `swap_edge_index' from the packet
     */
	payload_p = fbe_transport_get_payload_ex(packet_p);
	control_operation_p = fbe_payload_ex_get_control_operation(payload_p);  
    status = fbe_virtual_drive_usurper_get_control_buffer(virtual_drive_p, packet_p,
                                                          sizeof(fbe_block_transport_control_create_edge_t),
                                                          (fbe_payload_control_buffer_t *)&create_edge_p);
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }


    /* Set encryption mode if needed
     */
    fbe_virtual_drive_init_encryption_state(virtual_drive_p, FBE_FALSE);

    /* Forward the request to base config.
     */
    swap_in_edge_index = create_edge_p->client_index;
    status = fbe_base_config_create_edge((fbe_base_config_t *) virtual_drive_p, packet_p);
    if (status != FBE_STATUS_OK)
    {
        /* Trace the error and return the status
         */
		fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s create edge failed - status: 0x%x\n", 
                              __FUNCTION__, status);
        return status;
    }

    /*! @todo Eventually the edge geometry will be persisted, so this will not be needed.
     */
    fbe_virtual_drive_propagate_block_geometry(virtual_drive_p);

    /* Validate the swap in edge index
     */
    if ((swap_in_edge_index != FIRST_EDGE_INDEX)  &&
        (swap_in_edge_index != SECOND_EDGE_INDEX)    )
    {
        /* Trace the error and return the status
         */
		fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s invalid swap-in edge index: %d\n", 
                              __FUNCTION__, swap_in_edge_index);
        return status;
    }

    /* Set the `swap_in_edge_index'
     */
    fbe_virtual_drive_set_swap_in_edge_index(virtual_drive_p, swap_in_edge_index);

    /* Get some debug information
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_base_config_get_block_edge((fbe_base_config_t *) virtual_drive_p, &block_edge_p, FIRST_EDGE_INDEX);
    fbe_block_transport_get_path_state(block_edge_p, &first_edge_path_state);
    fbe_base_config_get_block_edge((fbe_base_config_t *) virtual_drive_p, &block_edge_p, SECOND_EDGE_INDEX);
    fbe_block_transport_get_path_state(block_edge_p, &second_edge_path_state);

    /* If enabled trace some information
     */
    fbe_virtual_drive_trace(virtual_drive_p, 
                            FBE_TRACE_LEVEL_INFO, 
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING,
                            "virtual_drive: create edge mode: %d index: %d first state: %d second state: %d \n", 
                            configuration_mode, swap_in_edge_index, first_edge_path_state, second_edge_path_state);

    /* There are three times that an edge can be created:
     *  1. Initial creation when the object is instantiated
     *  2. Permanent spare (job service transaction)
     *  3. Copy request swaps-in the destination edge (job service transaction)
     *
     * Only when the request was initiated by the job service (i.e. request
     * in-progress set) do we need to immediately reschedule the monitor.
     */

    /* Set the `swap-in' condition.
     */
    status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                    (fbe_base_object_t *) virtual_drive_p,
                                    FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_SWAP_IN_EDGE);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Failed to set swap-in condition\n",
                              __FUNCTION__);
        return status;
    }

    /* Reschedule immediately
     */
    if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS))
    {
        status = fbe_lifecycle_reschedule(&fbe_base_config_lifecycle_const,
                                          (fbe_base_object_t *)virtual_drive_p,
                                          (fbe_lifecycle_timer_msec_t) 0);    /* Immediate reschedule */
        return status;
    }

    /* Get the path state counters for the downstream edges. 
     */
    fbe_base_config_get_path_state_counters((fbe_base_config_t *) virtual_drive_p, &path_state_counters);

    /* Virtual drive object is a special case and so it it verifies the downstream
     * edge state based on its current configuration mode and decide whether it has
     * enough downstream edges before it issue lifecycle reschedule.
     */
    if(((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) &&
        (first_edge_path_state != FBE_PATH_STATE_INVALID)) ||
       ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE) &&
        (second_edge_path_state != FBE_PATH_STATE_INVALID)) ||
       ((fbe_virtual_drive_is_mirror_configuration_mode_set(virtual_drive_p)) &&       
        (!path_state_counters.invalid_counter)))
    {
        status = fbe_lifecycle_reschedule(&fbe_base_config_lifecycle_const,
                                          (fbe_base_object_t *)virtual_drive_p,
                                          (fbe_lifecycle_timer_msec_t) 0);    /* Immediate reschedule */
    }
    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_create_edge()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_destroy_edge()
 ******************************************************************************
 * @brief
 * This function is used to destroy edge and attach the edge with downstream 
 * object, it uses base config functionality for creating edge and attaching 
 * edge with downstream object.
 *
 * Once edge gets created and attached it will set the different condition based
 * on the number of edges attached with downstream object to start certain 
 * operation.
 *
 * @param virtual_drive_p - The virtual drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/10/2009 - Created. Dhaval Patel
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_destroy_edge(fbe_virtual_drive_t *virtual_drive_p, fbe_packet_t *packet_p)
{
    fbe_status_t                                status = FBE_STATUS_OK;
	fbe_payload_ex_t                           *payload_p = NULL;
	fbe_payload_control_operation_t            *control_operation_p = NULL;  
	fbe_block_transport_control_destroy_edge_t *destroy_edge_p = NULL;    /* INPUT */
    fbe_base_config_path_state_counters_t       path_state_counters;
    fbe_path_state_t                            first_edge_path_state = FBE_PATH_STATE_INVALID;
    fbe_path_state_t                            second_edge_path_state = FBE_PATH_STATE_INVALID;
    fbe_virtual_drive_configuration_mode_t      configuration_mode;
    fbe_block_edge_t                           *block_edge_p = NULL;
    fbe_edge_index_t                            swap_out_edge_index = FBE_EDGE_INDEX_INVALID;

    /* Populate the `swap_edge_index' from the packet
     */
	payload_p = fbe_transport_get_payload_ex(packet_p);
	control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    status = fbe_virtual_drive_usurper_get_control_buffer(virtual_drive_p, packet_p,
                                                          sizeof(fbe_block_transport_control_destroy_edge_t),
                                                          (fbe_payload_control_buffer_t *)&destroy_edge_p);
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    swap_out_edge_index = destroy_edge_p->client_index;
    if ((swap_out_edge_index != FIRST_EDGE_INDEX)  &&
        (swap_out_edge_index != SECOND_EDGE_INDEX)    )
    {
        /* Trace the error and return the status
         */
		fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s invalid swap-out edge index: %d\n", 
                              __FUNCTION__, swap_out_edge_index);

		fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
		fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
		fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* We may need to remove keys before destroying edge.
     */
    fbe_virtual_drive_swap_out_remove_keys(virtual_drive_p, packet_p, swap_out_edge_index);

    /* Forward the request to base config.
     */
    status = fbe_base_config_destroy_edge((fbe_base_config_t *) virtual_drive_p, packet_p);
    if (status != FBE_STATUS_OK)
    {
        /* Trace the error and return the status
         */
		fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s destroy edge failed - status: 0x%x\n", 
                              __FUNCTION__, status);
        return status;
    }
    fbe_virtual_drive_propagate_block_geometry(virtual_drive_p);

    /* Set the `swap_out_edge_index'
     */
    fbe_virtual_drive_set_swap_out_edge_index(virtual_drive_p, swap_out_edge_index);

    /* Get some information for debug purposes
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_base_config_get_block_edge((fbe_base_config_t *) virtual_drive_p, &block_edge_p, FIRST_EDGE_INDEX);
    fbe_block_transport_get_path_state(block_edge_p, &first_edge_path_state);
    fbe_base_config_get_block_edge((fbe_base_config_t *) virtual_drive_p, &block_edge_p, SECOND_EDGE_INDEX);
    fbe_block_transport_get_path_state(block_edge_p, &second_edge_path_state);

    /* If enabled trace some information
     */
    fbe_virtual_drive_trace(virtual_drive_p, 
                            FBE_TRACE_LEVEL_INFO, 
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_SWAP_TRACING,
                            "virtual_drive: destroy edge mode: %d index: %d first state: %d second state: %d \n", 
                            configuration_mode, swap_out_edge_index, first_edge_path_state, second_edge_path_state);

    /* Set encryption mode if needed
     */
    fbe_virtual_drive_init_encryption_state(virtual_drive_p, FBE_TRUE);

    /* There are three times that an edge can be destroyed:
     *  1. The object is being destroyed
     *  2. Permanent spare (job service transaction)
     *  3. Copy request swaps-out the source edge (job service transaction)
     */

    /* Set the `swap-out' condition.
     */
    status = fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                                    (fbe_base_object_t *) virtual_drive_p,
                                    FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_SWAP_OUT_EDGE);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Failed to set swap-out condition\n",
                              __FUNCTION__);
        return status;
    }

    /* Get the path state counters for the downstream edges. 
     */
    fbe_base_config_get_path_state_counters((fbe_base_config_t *) virtual_drive_p, &path_state_counters);

    /* Virtual drive object is a special case and so it it verifies the downstream
     * edge state based on its current configuration mode and decide whether it has
     * enough downstream edges before it issue lifecycle reschedule.
     */
    if(((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE) &&
        (first_edge_path_state != FBE_PATH_STATE_INVALID)) ||
       ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE) &&
        (second_edge_path_state != FBE_PATH_STATE_INVALID)) ||
       ((fbe_virtual_drive_is_mirror_configuration_mode_set(virtual_drive_p)) &&       
        (path_state_counters.invalid_counter == 0)))
    {
        status = fbe_lifecycle_reschedule(&fbe_base_config_lifecycle_const,
                                          (fbe_base_object_t *)virtual_drive_p,
                                          (fbe_lifecycle_timer_msec_t) 0);
    }
    return status;
}
/***************************************
 * end fbe_virtual_drive_destroy_edge()
 ***************************************/

/*!****************************************************************************
 * fbe_virtual_drive_get_spare_info()
 ******************************************************************************
 * @brief
 *  This function is use to get the spare drive information, it is mainly used 
 *  by drive spare library to collect the drive information to find the best 
 *  suitable spare.
 *
 *  Provision drive object does not store this information and so it always 
 *  send the request to the downstream object with TRAVERSE option set.
 *
 * @param virtual_drive_p - The provision drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/12/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_virtual_drive_get_spare_info(fbe_virtual_drive_t * virtual_drive_p,
                                 fbe_packet_t * packet_p)
{
    fbe_payload_ex_t *                     payload_p = NULL;
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_spare_drive_info_t *                spare_drive_info_p = NULL;
    fbe_spare_drive_info_t *                provision_drive_spare_drive_info_p = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_block_edge_t *                      block_edge_p = NULL;

    fbe_base_object_trace(  (fbe_base_object_t *)virtual_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /* Retrive the spare info request from the packet. */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    status = fbe_virtual_drive_usurper_get_control_buffer(virtual_drive_p, packet_p,
                                                          sizeof(fbe_spare_drive_info_t),
                                                          (fbe_payload_control_buffer_t *)&spare_drive_info_p);
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    fbe_virtual_drive_lock(virtual_drive_p);
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_virtual_drive_unlock(virtual_drive_p);

    if(fbe_virtual_drive_is_mirror_configuration_mode_set(virtual_drive_p))
    {
       status = FBE_STATUS_GENERIC_FAILURE;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
       
    }
    else if(configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)
    { 
        fbe_base_config_get_block_edge((fbe_base_config_t *) virtual_drive_p, &block_edge_p, FIRST_EDGE_INDEX);
            
    }
    else if(configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)
    { 
        fbe_base_config_get_block_edge((fbe_base_config_t *) virtual_drive_p, &block_edge_p, SECOND_EDGE_INDEX);
            
    }

    /* allocate the buffer to send the get spare inforamtion usurpur command to provision drive object. */
    provision_drive_spare_drive_info_p = (fbe_spare_drive_info_t *) fbe_transport_allocate_buffer();
    if (provision_drive_spare_drive_info_p == NULL)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* initialize the spare drive information. */
    fbe_spare_initialize_spare_drive_info(provision_drive_spare_drive_info_p);

    /* allocate the new control operation to get the spare information. */
    control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);
    fbe_payload_control_build_operation(control_operation_p,
                                        FBE_PROVISION_DRIVE_CONTROL_CODE_GET_SPARE_INFO,
                                        provision_drive_spare_drive_info_p,
                                        sizeof(fbe_spare_drive_info_t));

    fbe_payload_ex_increment_control_operation_level(payload_p);

    fbe_transport_set_completion_function(packet_p, fbe_virtual_drive_get_spare_provision_drive_info_completion, virtual_drive_p);
    fbe_block_transport_send_control_packet(block_edge_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_virtual_drive_get_spare_info()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_get_spare_provision_drive_info_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion fuction for the get spare inforamation.
 *
 * @param virtual_drive_p - The provision drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/12/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_get_spare_provision_drive_info_completion(fbe_packet_t * packet_p, 
                                                                                fbe_packet_completion_context_t context)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_t *                   virtual_drive_p = NULL;
    fbe_payload_ex_t *                     payload_p = NULL;
    fbe_payload_control_operation_t *       control_operation_p = NULL;
    fbe_payload_control_operation_t *       prev_control_operation_p = NULL;
    fbe_spare_drive_info_t *                spare_drive_info_p = NULL;
    fbe_spare_drive_info_t *                pvd_spare_drive_info_p = NULL;
    fbe_payload_control_status_t            control_status;
    fbe_lba_t                               exported_offset = FBE_LBA_INVALID;
    fbe_lba_t                               raid_group_capacity = FBE_LBA_INVALID;
    fbe_virtual_drive_metadata_positions_t  virtual_drive_metadata_positions;
    
    virtual_drive_p = (fbe_virtual_drive_t *)context;
    
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_buffer(control_operation_p, &pvd_spare_drive_info_p);

    prev_control_operation_p = fbe_payload_ex_get_prev_control_operation(payload_p);
    fbe_payload_control_get_buffer(prev_control_operation_p, &spare_drive_info_p);
    
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    /* If status is not good then complete the packet with error. */
    if((status != FBE_STATUS_OK) || (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK))
    {
        fbe_base_object_trace(  (fbe_base_object_t *)virtual_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "virtual_drive: get spare info failed, status 0x%x\n", status);
        status = (status != FBE_STATUS_OK) ? status : FBE_STATUS_GENERIC_FAILURE;

    }
    else
    {   
        /* copy pvd spare drive information to virtual drive spare information. */                   
        fbe_copy_memory(spare_drive_info_p, pvd_spare_drive_info_p, sizeof(fbe_spare_drive_info_t));

        /* Get the virtual drive metadata capacity. */
        status = fbe_virtual_drive_get_metadata_positions(virtual_drive_p,  &virtual_drive_metadata_positions);
        if (status != FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Failed to get metadata positions. status: 0x%x \n",
                                  __FUNCTION__, status);
        }
        else
        {
            /* Get the minimum capacity required for a replacement drive.
             */
            status = fbe_block_transport_server_get_minimum_capacity_required(&virtual_drive_p->spare.raid_group.base_config.block_transport_server,
                                                                              &exported_offset,
                                                                              &raid_group_capacity);
            if ((status != FBE_STATUS_OK)                ||
                (raid_group_capacity == FBE_LBA_INVALID)    )
            {
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Failed to get minimum capacity required. status: 0x%x \n",
                                      __FUNCTION__, status);
                status = FBE_STATUS_GENERIC_FAILURE;
            }
            else
            {
                /*! @note Else populate the `minimum capacity required' and the 
                 *        `exported offset'. The minimum required capacity is 
                 *        simply the virtual drive metadata start lba plus the 
                 *        capacity.  The `exported offset' is the maximum of 
                 *        the virtual drive offset and the provision drive
                 *        offset.
                 */
                spare_drive_info_p->capacity_required = virtual_drive_metadata_positions.paged_metadata_lba + virtual_drive_metadata_positions.paged_metadata_capacity;
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "virtual_drive: get spare info rg cap: 0x%llx vd meta start: 0x%llx cap: 0x%llx\n",
                                      raid_group_capacity, 
                                      virtual_drive_metadata_positions.paged_metadata_lba,
                                      virtual_drive_metadata_positions.paged_metadata_capacity);
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_INFO,
                                      FBE_TRACE_MESSAGE_ID_INFO,
                                      "virtual_drive: get spare info vd exported offset: 0x%llx cap: 0x%llx config: 0x%llx\n",
                                      (unsigned long long)exported_offset, (unsigned long long)spare_drive_info_p->capacity_required,
                                      (unsigned long long)spare_drive_info_p->configured_capacity);

                /* Validate that the provision drive offset is at least as large
                 * as the virtual drive offset.
                 */
                if (spare_drive_info_p->exported_offset < exported_offset)
                {
                    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                          FBE_TRACE_LEVEL_ERROR,
                                          FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                          "%s Downstream offset: 0x%llx is less than vd offset: 0x%llx\n",
                                          __FUNCTION__, (unsigned long long)spare_drive_info_p->exported_offset, (unsigned long long)exported_offset);
                    status = FBE_STATUS_GENERIC_FAILURE;
                }
                else
                {
                    /* Else use the virtual drive offset.
                     */
                    spare_drive_info_p->exported_offset = exported_offset;
                }
            }
        }
    }

    fbe_transport_release_buffer(pvd_spare_drive_info_p);
    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_get_spare_provision_drive_info_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_get_configuration()
 ******************************************************************************
 * @brief
 *  This usurpur command is used to get the current configuration details of
 *  the virtual drive object.
 * 
 * @param virtual_drive_p - The virtual drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/10/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_virtual_drive_get_configuration(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p)
{

    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_payload_ex_t *                         payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_virtual_drive_configuration_t *     virtual_drive_get_configuration_p = NULL; 

    fbe_base_object_trace(  (fbe_base_object_t *)virtual_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    status = fbe_virtual_drive_usurper_get_control_buffer(virtual_drive_p, packet_p,
                                                          sizeof(fbe_virtual_drive_configuration_t),
                                                          (fbe_payload_control_buffer_t *)&virtual_drive_get_configuration_p);
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Get the configuration details of the virtual drive object. */
    fbe_virtual_drive_lock(virtual_drive_p);
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, 
                                             &virtual_drive_get_configuration_p->configuration_mode);
    /* Look at the clustered flag.
     */
    virtual_drive_get_configuration_p->b_job_in_progress = fbe_raid_group_is_clustered_flag_set((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS);
    fbe_virtual_drive_unlock(virtual_drive_p);

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_get_configuration()
 ******************************************************************************/

/*!**************************************************************
 * fbe_virtual_drive_usurper_get_control_buffer()
 ****************************************************************
 * @brief
 *  This function gets the buffer pointer out of the packet
 *  and validates it. It returns error status if any of the
 *  pointer validations fail or the buffer size doesn't match
 *  the specifed buffer length.
 *
 * @param virtual_drive_p   - Pointer to the virtual drive
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
fbe_status_t fbe_virtual_drive_usurper_get_control_buffer(fbe_virtual_drive_t *virtual_drive_p,
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
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    if (control_operation_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s control operation is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer(control_operation_p, out_buffer_pp);
    if (*out_buffer_pp == NULL)
    {
        // The buffer pointer is NULL!
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
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
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s buffer length mismatch\n", __FUNCTION__);
        *out_buffer_pp = NULL;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;

}
/****************************************************
 * end fbe_virtual_drive_usurper_get_control_buffer()
 ****************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_get_virtual_drive_debug_flags()
 *****************************************************************************
 *
 * @brief   This method retrieves the value of the virtual drive debug flags
 *          for the virtual drive object specified.
 *
 * @param   virtual_drive_p - The raid group.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  
 *
 ****************************************************************/
static fbe_status_t
fbe_virtual_drive_get_virtual_drive_debug_flags(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_virtual_drive_debug_payload_t *get_debug_flags_p = NULL;
    
    /* Get a pointer to the debug flags to populate
     */
    status = fbe_virtual_drive_usurper_get_control_buffer(virtual_drive_p,
                                                          packet_p,
                                                          sizeof(fbe_virtual_drive_debug_payload_t),
                                                          (fbe_payload_control_buffer_t)&get_debug_flags_p);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s invalid buffer.\n", __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        /* Lock the raid group object to syncrounize udpate.
         */
        fbe_virtual_drive_lock(virtual_drive_p);
        fbe_virtual_drive_get_debug_flags(virtual_drive_p,
                                          &get_debug_flags_p->virtual_drive_debug_flags);
        fbe_virtual_drive_unlock(virtual_drive_p);

        /* Base object trace will trace the object id etc.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s received.\n",__FUNCTION__);
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "\t virtual drive debug flags: 0x%08x \n", 
                            get_debug_flags_p->virtual_drive_debug_flags);

        status = FBE_STATUS_OK;
    }

    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************
 * end fbe_virtual_drive_get_virtual_drive_debug_flags()
 ******************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_set_virtual_drive_debug_flags()
 *****************************************************************************
 *
 * @brief   This method sets the virtual drive debug
 *          flags to the value passed for the associated virtual drive.
 *
 * @param   virtual_drive_p - The raid group.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  
 *
 ****************************************************************/
static fbe_status_t
fbe_virtual_drive_set_virtual_drive_debug_flags(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_virtual_drive_debug_payload_t *set_debug_flags_p = NULL;
    fbe_virtual_drive_debug_flags_t  old_debug_flags;
    
    /* Get old debug flags.
     */
    fbe_virtual_drive_get_debug_flags(virtual_drive_p, &old_debug_flags);

    /* Now retrieve the value of the debug flags to set from the buffer.
     */
    fbe_virtual_drive_usurper_get_control_buffer(virtual_drive_p,
                                              packet_p, 
                                              sizeof(fbe_virtual_drive_debug_payload_t),
                                              (fbe_payload_control_buffer_t)&set_debug_flags_p);

    if (set_debug_flags_p == NULL)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "set_virtual_drive_debug_flags null buffer.\n");
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    else
    {
        /* Base object trace will trace the object id etc.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "set_virtual_drive_debug_flags: FBE_RAID_GROUP_CONTROL_CODE_SET_RAID_GROUP_DEBUG_FLAGS receive\n");
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                        FBE_TRACE_LEVEL_INFO,
                         FBE_TRACE_MESSAGE_ID_INFO,
                        "\t Old virtual drive debug flags: 0x%08x new: 0x%08x \n", 
                        old_debug_flags, set_debug_flags_p->virtual_drive_debug_flags);

        /* Lock the raid group object to syncrounize udpate.
         */
        fbe_virtual_drive_lock(virtual_drive_p);
        fbe_virtual_drive_set_debug_flags(virtual_drive_p,
                                       set_debug_flags_p->virtual_drive_debug_flags);
        fbe_virtual_drive_unlock(virtual_drive_p);
        status = FBE_STATUS_OK;
    }

    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/*******************************************************
 * end fbe_virtual_drive_set_virtual_drive_debug_flags()
 *******************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_validate_swap_in_request()
 ******************************************************************************
 * @brief
 *  This usurpur command is used to determine whether we need swap-in any
 *  more, job service sends this request to validate before it starts swap-in
 *  operation.
 * 
 * @param virtual_drive_p - The virtual drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/10/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_validate_swap_in_request(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;
    fbe_virtual_drive_swap_in_validation_t *virtual_drive_swap_in_validation_p = NULL;
    fbe_virtual_drive_flags_t               flags;

    fbe_base_object_trace(  (fbe_base_object_t *)virtual_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    status = fbe_virtual_drive_usurper_get_control_buffer(virtual_drive_p, packet_p,
                                                          sizeof(fbe_virtual_drive_swap_in_validation_t),
                                                          (fbe_payload_control_buffer_t *)&virtual_drive_swap_in_validation_p);
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* By default the status is not valid.
     */
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);
    virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_INVALID;

    /* By default confirmation should be enabled.  If not mark the request
     * so that we know confirmation is disabled.
     */
    if (virtual_drive_swap_in_validation_p->b_confirmation_enabled == FBE_FALSE)
    {
        fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_CONFIRMATION_DISABLED);
    }

    /* Update the last `copy request type'.
     */
    fbe_virtual_drive_set_copy_request_type(virtual_drive_p, virtual_drive_swap_in_validation_p->command_type);

    /* Validate the command is supported.
     */
    if (virtual_drive_swap_in_validation_p->command_type == FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND)
    {
        /* verify whether we need to swap-in permanent spare. */
        status = fbe_virtual_drive_swap_validate_permanent_spare_request(virtual_drive_p,
                                                                         virtual_drive_swap_in_validation_p);
        if (status == FBE_STATUS_OK)
        {
            virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_OK;
        }
        else
        {
            /* Else fbe_virtual_drive_swap_validate_permanent_spare_request()
             * should have populated the status.
             */
        }
    }
    else if(virtual_drive_swap_in_validation_p->command_type == FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND) 
    {
        /* verify whether we need to swap-in proactive copy . */
        status = fbe_virtual_drive_swap_validate_proactive_copy_request(virtual_drive_p,
                                                                        virtual_drive_swap_in_validation_p);
        if (status == FBE_STATUS_OK)
        {
            virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_OK;
        }
        else
        {
            /* Else fbe_virtual_drive_swap_validate_proactive_copy_request()
             * should have populated the status.
             */
        }
    }
    else if (virtual_drive_swap_in_validation_p->command_type == FBE_SPARE_INITIATE_USER_COPY_COMMAND)                
    {
        /* verify whether we can swap-in user proactive spare. */
        status = fbe_virtual_drive_swap_check_if_user_copy_is_allowed(virtual_drive_p,
                                                                      virtual_drive_swap_in_validation_p);
        if (status == FBE_STATUS_OK)
        {
            // ask permission from upstream
            fbe_virtual_drive_swap_ask_user_copy_permission(virtual_drive_p,packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }
    else if(virtual_drive_swap_in_validation_p->command_type == FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND)
    {
        /* verify whether we can start user copy */
        status = fbe_virtual_drive_swap_check_if_user_copy_is_allowed(virtual_drive_p,
                                                                      virtual_drive_swap_in_validation_p);
        if (status == FBE_STATUS_OK)
        {
            // ask permission from upstream
            fbe_virtual_drive_swap_ask_user_copy_permission(virtual_drive_p, packet_p);
            return FBE_STATUS_MORE_PROCESSING_REQUIRED;
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s unknown swap-in command type:0x%x.\n",
                              __FUNCTION__, virtual_drive_swap_in_validation_p->command_type);
        virtual_drive_swap_in_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_UNSUPPORTED_COMMAND;
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
 * end fbe_virtual_drive_validate_swap_in_request()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_validate_swap_out_request()
 ******************************************************************************
 * @brief
 *  This usurpur command is used to determine whether we need swap-out any
 *  more, job service sends this request to validate before it starts swap-in
 *  operation.
 * 
 * @param virtual_drive_p - The virtual drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/10/2009 - Created. Dhaval Patel
 *
 ******************************************************************************/
static fbe_status_t
fbe_virtual_drive_validate_swap_out_request(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;
    fbe_virtual_drive_swap_out_validation_t *virtual_drive_swap_out_validation_p = NULL;

    fbe_base_object_trace(  (fbe_base_object_t *)virtual_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    status = fbe_virtual_drive_usurper_get_control_buffer(virtual_drive_p, packet_p,
                                                          sizeof(fbe_virtual_drive_swap_out_validation_t),
                                                          (fbe_payload_control_buffer_t *)&virtual_drive_swap_out_validation_p);
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* By default the swap status is not valid.
     */
    virtual_drive_swap_out_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_INVALID;

    /* By default confirmation should be enabled.  If not mark the request
     * so that we know confirmation is disabled.
     */
    if (virtual_drive_swap_out_validation_p->b_confirmation_enabled == FBE_FALSE)
    {
        fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_CONFIRMATION_DISABLED);
    }

    /* Update the last `copy request type'.
     */
    fbe_virtual_drive_set_copy_request_type(virtual_drive_p, virtual_drive_swap_out_validation_p->command_type);

    /* Validate the swap-out command is supported.
     */
    if ((virtual_drive_swap_out_validation_p->command_type == FBE_SPARE_COMPLETE_COPY_COMMAND) ||
        (virtual_drive_swap_out_validation_p->command_type == FBE_SPARE_ABORT_COPY_COMMAND)       )
    {
        /* get the appropriate edge index which needs a swap-out. */
        status = fbe_virtual_drive_swap_validate_swap_out_request(virtual_drive_p, virtual_drive_swap_out_validation_p);
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s swap out get edge index: %d status :0x%x\n", 
                              __FUNCTION__, virtual_drive_swap_out_validation_p->swap_edge_index, status);
        if (status == FBE_STATUS_OK)
        {
            virtual_drive_swap_out_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_OK;
        }
        else
        {
            virtual_drive_swap_out_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_CONFIG_MODE_DOESNT_SUPPORT;
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s unknown swap-out command type:0x%x.\n",
                              __FUNCTION__, virtual_drive_swap_out_validation_p->command_type);
        virtual_drive_swap_out_validation_p->swap_status = FBE_VIRTUAL_DRIVE_SWAP_STATUS_UNSUPPORTED_COMMAND;
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
 * end fbe_virtual_drive_validate_swap_out_request()
 ******************************************************************************/

/*!**************************************************************
 * fbe_virtual_drive_update_logical_error_stats_completion()
 ****************************************************************
 * @brief
 *  Completion of our usurper to the lower level.
 *  We completed the first edge usurper.  
 *  Check if we need to send to the second edge.
 *
 * @param packet_p - Packet completing.
 * @param context - Virtual drive.
 *
 * @return FBE_STATUS_OK - if we are done.
 *         FBE_STATUS_MORE_PROCESSING_REQUIRED - If we sent to second edge.
 *
 * @author
 *  2/7/2011 - Created. Rob Foley
 *
 ****************************************************************/

fbe_status_t fbe_virtual_drive_update_logical_error_stats_completion(fbe_packet_t * packet_p,
                                                                     fbe_packet_completion_context_t context)
{
    fbe_status_t packet_status;
    fbe_virtual_drive_t *virtual_drive_p = (fbe_virtual_drive_t *)context;

    fbe_base_object_trace(  (fbe_base_object_t *)virtual_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    packet_status = fbe_transport_get_status_code(packet_p);

    /* We know if we need to send to the second edge based on the status in the packet.
     */
    if (packet_status != FBE_STATUS_OK)
    {
        fbe_block_edge_t *block_edge_p = NULL;
        /* We did not find the targeted drive on the first edge, send down the second edge.
         */
        fbe_base_config_get_block_edge((fbe_base_config_t*)virtual_drive_p, &block_edge_p, 1 /* second edge. */);
        fbe_block_transport_send_control_packet(block_edge_p, packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }
    else
    {
        /* The request found the targeted drive, just return the packet.
         */
        return FBE_STATUS_OK;
    }
}
/******************************************
 * end fbe_virtual_drive_update_logical_error_stats_completion()
 ******************************************/

/*!****************************************************************************
 * fbe_virtual_drive_update_logical_error_stats()
 ******************************************************************************
 * @brief
 *  Handle the update logical errors block transport usurper.
 *  Currently the only error type supported by the VD is the TIMEOUT_ERRORS.
 * 
 *  Handling is to mark the client's edge, which this packet came down.
 *  We add the FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS attribute to the edge
 *  so the upper level has something to indicate errors have occurred.
 *  Regardless of the state of the edge, the upper level will have something
 *  to indicate this drive is bad.
 * 
 *  After marking the edge we forward the packet down the edge.
 * 
 * @param virtual_drive_p - The virtual drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  2/8/2011 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t
fbe_virtual_drive_update_logical_error_stats(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t status;
    fbe_payload_ex_t *                         payload_p = NULL;
    fbe_payload_control_operation_t *           control_operation_p = NULL;
    fbe_block_transport_control_logical_error_t *    logical_error_p = NULL;
    fbe_virtual_drive_configuration_mode_t configuration_mode;
    fbe_block_edge_t *block_edge_p = NULL;
    fbe_block_edge_t *edge_p = (fbe_block_edge_t *)fbe_transport_get_edge(packet_p);
    fbe_edge_index_t edge_index;

    fbe_base_object_trace(  (fbe_base_object_t *)virtual_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    status = fbe_virtual_drive_usurper_get_control_buffer(virtual_drive_p, packet_p,
                                                          sizeof(fbe_block_transport_control_logical_error_t),
                                                          (fbe_payload_control_buffer_t *)&logical_error_p);
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* The VD only knows how to handle this one error type.
     * Other error types are not expected at this time.
     */
    if (logical_error_p->error_type != FBE_BLOCK_TRANSPORT_ERROR_TYPE_TIMEOUT)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s logical error type %d not expected\n", 
                              __FUNCTION__, logical_error_p->error_type);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* We only set the attribute on the client edge for this packet.
     */
    if (edge_p != NULL)
    {
        fbe_object_id_t client_id;
        fbe_block_transport_get_client_id(edge_p, &client_id);
        fbe_block_transport_get_server_index(edge_p, &edge_index);
        status = fbe_block_transport_server_set_path_attr(&virtual_drive_p->spare.raid_group.base_config.block_transport_server,
                                                          edge_index, 
                                                          FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS);
        if (status == FBE_STATUS_OK)
        {
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "set FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS for client 0x%x index %d\n", 
                                  client_id, edge_index);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "error %d on set FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS for client 0x%x index %d\n", 
                                  status, client_id, edge_index);
        }
    }
    else
    {
        /* We always expect the edge to be given.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s edge is null type: %d\n", 
                              __FUNCTION__, logical_error_p->error_type);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Determine which edges to update with our attribute.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    switch (configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
            /* Pass this through to the edge.
             * We do not need a callback since this is the only edge. 
             */
            fbe_base_config_get_block_edge((fbe_base_config_t*)virtual_drive_p, &block_edge_p, 0 /* First edge */ );
            fbe_block_transport_send_control_packet(block_edge_p, packet_p);
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            /* Pass this through to the edge.
             * We do not need a callback since this is the only edge. 
             */
            fbe_base_config_get_block_edge((fbe_base_config_t*)virtual_drive_p, &block_edge_p, 1 /* Second edge */);
            fbe_block_transport_send_control_packet(block_edge_p, packet_p);
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            /* Send to the first edge.  The callback will submit this to the second edge.
             */
            fbe_transport_set_completion_function(packet_p, fbe_virtual_drive_update_logical_error_stats_completion, virtual_drive_p);

            fbe_base_config_get_block_edge((fbe_base_config_t*)virtual_drive_p, &block_edge_p, 0);
            fbe_block_transport_send_control_packet(block_edge_p, packet_p);
            break;

        default:
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s unknown config mode: %d\n", __FUNCTION__, configuration_mode);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            break;
    };
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_update_logical_error_stats()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_clear_logical_errors()
 ******************************************************************************
 * @brief
 *  Clear out any logical errors we have on the upstream edge.
 * 
 * @param virtual_drive_p - The virtual drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  2/8/2011 - Created. Rob Foley
 *
 ******************************************************************************/
static fbe_status_t
fbe_virtual_drive_clear_logical_errors(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t                    status;
    fbe_payload_ex_t               *payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_block_edge_t                *edge_p = (fbe_block_edge_t *)fbe_transport_get_edge(packet_p);
    fbe_edge_index_t                edge_index;
    fbe_block_edge_t                *downstream_edge_p = NULL;
    fbe_virtual_drive_configuration_mode_t configuration_mode;
    fbe_path_attr_t                 path_attr;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Only clear our attribute on the edge this was sent down.
     */
    if (edge_p != NULL)
    {
        fbe_object_id_t client_id;
        fbe_block_transport_get_client_id(edge_p, &client_id);
        fbe_block_transport_get_server_index(edge_p, &edge_index);
        status = fbe_block_transport_server_clear_path_attr(&virtual_drive_p->spare.raid_group.base_config.block_transport_server,
                                                            edge_index, 
                                                            FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS);
        if (status == FBE_STATUS_OK) 
        { 
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "clear FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS from client 0x%x %d\n", 
                                  client_id, edge_index);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                  "error %d clearing FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS from client 0x%x %d\n", 
                                  status, client_id, edge_index);
        }

        fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
        /* In the case of a mirror mode.  We only send this to the edge that is completely 
         * healthy. 
         * The edge that is not completely healthy is handled by the VD if it gets timeout errors. 
         */
        switch (configuration_mode)
        {
            case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
                edge_index = FIRST_EDGE_INDEX;
                break;
            case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
                edge_index = SECOND_EDGE_INDEX;
                break;
            default:
                fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                      FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                      "%s unknown config mode: %d\n", __FUNCTION__, configuration_mode);
                fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
                fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
                fbe_transport_complete_packet(packet_p);
                return FBE_STATUS_OK;
        };

        fbe_base_config_get_block_edge((fbe_base_config_t*)virtual_drive_p, &downstream_edge_p, edge_index);
        fbe_block_transport_get_path_attributes(downstream_edge_p, &path_attr);

        /* Don't bother sending the packet if there are no timeout errors on the downstream edge.
         */
        if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS)
        {
            status = fbe_block_transport_send_control_packet(downstream_edge_p, packet_p);
        }
        else
        {
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s path_attr: 0x%x t/o errors not set.\n", __FUNCTION__, path_attr);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
        }
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s no edge in packet %p\n", __FUNCTION__, packet_p);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);

        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
    }
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_clear_logical_errors()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_get_checkpoint()
 ******************************************************************************
 * @brief
 *  This usurpur command will return the VD checkpoint according to the
 *  configuration mode
 * 
 * @param virtual_drive_p - The virtual drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/02/2012  Ron Proulx  - Updated.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_get_checkpoint(fbe_virtual_drive_t *virtual_drive_p, fbe_packet_t *packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;
    fbe_raid_group_get_checkpoint_info_t   *rg_server_obj_info_p = NULL; 
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_bool_t                              b_is_edge_swapped;
    fbe_bool_t                              b_is_copy_complete = FBE_FALSE;

    fbe_base_object_trace(  (fbe_base_object_t *)virtual_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    status = fbe_virtual_drive_usurper_get_control_buffer(virtual_drive_p, packet_p,
                                                          sizeof(fbe_raid_group_get_checkpoint_info_t),
                                                          (fbe_payload_control_buffer_t *)&rg_server_obj_info_p);
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }


    /* The request type must be supported.
     */
    switch(rg_server_obj_info_p->request_type)
    {
        case FBE_RAID_GROUP_GET_CHECKPOINT_REQUEST_TYPE_EVAL_MARK_NR:
        case FBE_RAID_GROUP_GET_CHECKPOINT_REQUEST_TYPE_CLEAR_RL:
            /* All the above types are supported.
             */
            break;

        case FBE_RAID_GROUP_GET_CHECKPOINT_REQUEST_TYPE_INVALID:
        default:
            /* Unsupported request type.
             */
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s request type: %d not supported\n",
                                __FUNCTION__, rg_server_obj_info_p->request_type);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, status, 0);
            fbe_transport_complete_packet(packet_p);
            return status;
    }

    /* Get the configuration details of the virtual drive object. */
    fbe_virtual_drive_lock(virtual_drive_p);
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_virtual_drive_metadata_is_edge_swapped(virtual_drive_p, &b_is_edge_swapped);
    fbe_virtual_drive_unlock(virtual_drive_p);
    b_is_copy_complete = fbe_virtual_drive_is_copy_complete(virtual_drive_p);

    /* If the virtual drive is in mirror mode always trace the information.
     * Otherwise only trace the information if enabled.
     */
    if ((configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE)  ||
        (configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE)    )
    {
        /* Always trace the information when the virtual drive is in mirror mode.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: get chkpt req class: %d type: %d idx: %d mode: %d swapped: %d copy complete: %d \n",
                              rg_server_obj_info_p->requestor_class_id, rg_server_obj_info_p->request_type,
                              rg_server_obj_info_p->edge_index, configuration_mode, 
                              b_is_edge_swapped, b_is_copy_complete);
    }
    else
    {
        /* Else only trace if enabled.
         */
        fbe_virtual_drive_trace(virtual_drive_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_VIRTUAL_DRIVE_DEBUG_FLAG_RL_TRACING,
                                "virtual_drive: get chkpt req class: %d type: %d idx: %d mode: %d swapped: %d copy complete: %d \n",
                                rg_server_obj_info_p->requestor_class_id, rg_server_obj_info_p->request_type, 
                                rg_server_obj_info_p->edge_index, configuration_mode, 
                                b_is_edge_swapped, b_is_copy_complete);
    }

    /*! @note Due to the difference in how we handle a request from a parent
     *        raid group and a request form the virutal drive itself, there
     *        are differnt handling routines for each.
     */
    if (rg_server_obj_info_p->requestor_class_id == FBE_CLASS_ID_VIRTUAL_DRIVE)
    {
        /* Invoke the method to populate the checkpoint when the calling class
         * is a virtual drive.
         */
        status = fbe_virtual_drive_get_checkpoint_for_virtual_drive(virtual_drive_p,
                                                                    rg_server_obj_info_p,
                                                                    configuration_mode,
                                                                    b_is_edge_swapped,
                                                                    b_is_copy_complete);
    }
    else
    {
        /* Else invoke the method when the calling class is a parent raid group.
         */
        status = fbe_virtual_drive_get_checkpoint_for_parent_raid_group(virtual_drive_p,
                                                                        rg_server_obj_info_p,
                                                                        configuration_mode,
                                                                        b_is_edge_swapped,
                                                                        b_is_copy_complete);
    }

    /* Trace an error if the we could not determine the checkpoint.
     */
    if (status != FBE_STATUS_OK)
    {
        /* Trace and error status.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s for req class: %d failed - status: 0x%x \n",
                              __FUNCTION__, rg_server_obj_info_p->requestor_class_id, status);
    }

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/***************************************** 
 * end fbe_virtual_drive_get_checkpoint()
 *****************************************/

/*!****************************************************************************
 * fbe_virtual_drive_get_unused_drive_as_spare_flag()
 ******************************************************************************
 * @brief
 *  This usurpur command will return the virtual drive flags.
 * 
 * @param virtual_drive_p - The virtual drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  05/05/2011 - Created. Vishnu Sharma
 *
 ******************************************************************************/
static fbe_status_t
fbe_virtual_drive_get_unused_drive_as_spare_flag(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t                                      status = FBE_STATUS_OK;
    fbe_payload_ex_t *                               payload_p = NULL;
    fbe_payload_control_operation_t *                 control_operation_p = NULL;
    fbe_payload_control_buffer_length_t               length = 0;
    fbe_virtual_drive_unused_drive_as_spare_flag_t *  flags_p =NULL;

    fbe_base_object_trace(  (fbe_base_object_t *)virtual_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &flags_p);
    if (flags_p == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)virtual_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* If length of the buffer does not match with set virtual drive 
     * sparing type then return error.
     */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if(length != sizeof(fbe_virtual_drive_unused_drive_as_spare_flag_t))
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload buffer lenght mismatch.\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Get the flags of the virtual drive object. */
    fbe_virtual_drive_lock(virtual_drive_p);
    fbe_virtual_drive_get_unused_drive_as_spare(virtual_drive_p,flags_p);
    fbe_virtual_drive_unlock(virtual_drive_p);

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}// End fbe_virtual_drive_get_unused_drive_as_spare_flag

/*!****************************************************************************
 * fbe_virtual_drive_set_unused_drive_as_spare_flag()
 ******************************************************************************
 * @brief
 *  This usurpur command will set the virtual drive flags.
 * 
 * @param virtual_drive_p - The virtual drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  05/05/2011 - Created. Vishnu Sharma
 *
 ******************************************************************************/
static fbe_status_t
fbe_virtual_drive_set_unused_drive_as_spare_flag(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t                                      status = FBE_STATUS_OK;
    fbe_payload_ex_t *                               payload_p = NULL;
    fbe_payload_control_operation_t *                 control_operation_p = NULL;
    fbe_payload_control_buffer_length_t               length = 0;
    fbe_virtual_drive_unused_drive_as_spare_flag_t *  flags_p =NULL;

    fbe_base_object_trace(  (fbe_base_object_t *)virtual_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_payload_control_get_buffer(control_operation_p, &flags_p);
    if (flags_p == NULL)
    {
        fbe_base_object_trace(  (fbe_base_object_t *)virtual_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* If length of the buffer does not match with set virtual drive 
     * sparing type then return error.
     */
    fbe_payload_control_get_buffer_length (control_operation_p, &length);
    if(length != sizeof(fbe_virtual_drive_unused_drive_as_spare_flag_t))
    {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s payload buffer lenght mismatch.\n",
                                __FUNCTION__);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Set the flags of the virtual drive object. */
    fbe_virtual_drive_lock(virtual_drive_p);
    fbe_virtual_drive_set_unused_drive_as_spare(virtual_drive_p,*flags_p);
    fbe_virtual_drive_unlock(virtual_drive_p);

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}// End fbe_virtual_drive_set_unused_drive_as_spare_flag

/*!****************************************************************************
 * fbe_virtual_drive_download_ack()
 ******************************************************************************
 * @brief
 *  This function would check all the upstream edges and if all the edges has
 *  acked the download request, send it down to PVD.
 * 
 * @param virtual_drive_p - The virtual drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  06/06/2011 - Created. Ruomu Gao
 *
 ******************************************************************************/
static fbe_status_t
fbe_virtual_drive_download_ack(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p)
{
    fbe_u32_t                       i = 0;
    fbe_base_edge_t                 *base_edge = NULL;
    fbe_block_edge_t                *block_edge = NULL;
    fbe_block_transport_server_t    *block_transport_server;
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_edge_index_t                edge_index;
    fbe_payload_ex_t               *payload_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_block_edge_t                *edge_p = (fbe_block_edge_t *)fbe_transport_get_edge(packet_p);

    block_transport_server = &((fbe_base_config_t *)virtual_drive_p)->block_transport_server;
    /* Set GRANT bit on the edge.
     */
    if (edge_p != NULL)
    {
        /* fbe_block_transport_get_server_index(edge_p, &edge_index);
        fbe_block_transport_server_set_path_attr(block_transport_server,
                                                 edge_index, 
                                                 FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_GRANT); */
        fbe_block_transport_set_path_attributes(edge_p, FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_GRANT);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s no edge in packet %p\n", __FUNCTION__, packet_p);
    }


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
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
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

    if (status == FBE_STATUS_OK)
    {
        /* We have all the edges acked and now pass the packet to PVD
         * get the primary edge index, if configuration mode is mirror then it will
         * return the primary edge index (original drive) of the mirror.
         */
        fbe_virtual_drive_get_primary_edge_index(virtual_drive_p, &edge_index);

        /* get the block edge pointer for the corresponding edge to forward the packet. */
        fbe_base_config_get_block_edge((fbe_base_config_t *) virtual_drive_p, &block_edge, edge_index);
        status = fbe_block_transport_send_control_packet(block_edge, packet_p);
    }
    else
    {
        /* Not all edges are ready, return the packet */
        payload_p = fbe_transport_get_payload_ex(packet_p);
        control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);

        status = FBE_STATUS_OK;
    }

    return status;
}

/*!****************************************************************************
 * fbe_virtual_drive_set_perm_spare_bit()
 ******************************************************************************
 * @brief
 *  This usurpur command is used to set the nonpaged metadata.  It is set when the
 *  permanent spare swaps in.  If the swap-in edge rebuild checkpoint is not at
 *  the end marker we must set it.
 * 
 * @param virtual_drive_p - The virtual drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  05/01/2013  Ron Proulx  - Updated.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_set_perm_spare_bit(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t  configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;
    fbe_edge_index_t                        swap_in_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_virtual_drive_flags_t               flags;
    fbe_spare_swap_command_t                swap_command;
    fbe_lba_t                               swap_in_edge_checkpoint = 0;         
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;

    /* Get the control operation
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Determine which edge is be swapped into.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);
    fbe_virtual_drive_get_copy_request_type(virtual_drive_p, &swap_command);

    /* Get the swap-in edge index and get the rebuild checkpoint
     */
    swap_in_edge_index = fbe_virtual_drive_get_swap_in_edge_index(virtual_drive_p);
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, swap_in_edge_index, &swap_in_edge_checkpoint);

    /* Trace some information
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: usurper set perm spare bit mode: %d cmd: %d flags: 0x%x index: %d chkpt: 0x%llx \n",
                          configuration_mode, swap_command, flags, swap_in_edge_index, 
                          (unsigned long long)swap_in_edge_checkpoint);

    /* For the most common case (permanent spare request) the swapped-in edge
     * checkpoint will be at the end marker.  But in the case where both the
     * source and destination drive fail, we must reset the checkpoint to the
     * end marker before we proceed with the permanent spare request.
     */
    if ((swap_command == FBE_SPARE_SWAP_IN_PERMANENT_SPARE_COMMAND) &&
        (swap_in_edge_checkpoint != FBE_LBA_INVALID)                   )
    {
        /* Trace this fact and first set the rebuild checkpoint to the end
         * marker.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: usurper set perm spare bit mode: %d cmd: %d flags: 0x%x index: %d set checkpoint to end\n",
                              configuration_mode, swap_command, flags, swap_in_edge_index);

        /* Set the completion to cleanup.
         */
        fbe_transport_set_completion_function(packet_p, 
                                              fbe_virtual_drive_set_swapped_bit_completion, 
                                              virtual_drive_p);  

        /* Set the end marker
         */
        status = fbe_virtual_drive_set_swapped_bit_set_checkpoint_to_end_marker(virtual_drive_p, packet_p);
        if (status == FBE_STATUS_MORE_PROCESSING_REQUIRED)
        {
            return status;
        }

        /* Request failed
         */
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Set the required non-paged flags when we start either a permanent spare
     * or copy request.
     */
    fbe_virtual_drive_set_swap_operation_start_nonpaged_flags(virtual_drive_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_virtual_drive_set_perm_spare_bit()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_start_user_copy()
 *****************************************************************************
 *
 * @brief   This method is called to setup the virtual drive to start a user
 *          copy request.  It can be called from either thru a job service
 *          usurper or the virtual drive monitor condition.
 * 
 * @param   virtual_drive_p - The virtual drive.
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  04/26/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
fbe_status_t fbe_virtual_drive_start_user_copy(fbe_virtual_drive_t *virtual_drive_p)
{

    fbe_virtual_drive_flags_t               flags;

    /* Get some debug information.
     */
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);

    /* If any of the operation complete flags is set trace an error and 
     * clear them. But continue.
     */
    if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_OPERATION_COMPLETE_MASK))
    {
        /* Trace an error, clear the flags and continue.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s operation complete flags: 0x%x set: 0x%x\n",
                              __FUNCTION__, 
                              FBE_VIRTUAL_DRIVE_FLAG_OPERATION_COMPLETE_MASK,
                              flags);

        /* Clear the flags and continue
         */
        fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_OPERATION_COMPLETE_MASK);
    }

    /* Set the flag to indicate that there is a swap request in progress
     */
    fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS);

    /* Else set the flag that indicates there is a change request in
     * progress.
     */
    fbe_raid_group_set_clustered_flag((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS);

    /* Set the flag to indicate that a user copy is in progress
     */
    if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_CONFIRMATION_DISABLED))
    {
        fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_USER_COPY_STARTED);
    }

    /* Return success
     */
    return FBE_STATUS_OK;
}
/*****************************************
 * end fbe_virtual_drive_start_user_copy()
 *****************************************/

/*!***************************************************************************
 * fbe_virtual_drive_initiate_user_copy()
 *****************************************************************************
 *
 * @brief   This usurper is sent to the virtual drive when a user initiated
 *          copy is requested.  The virtual drive needs to `setup' for the
 *          user copy.
 * 
 * @param   virtual_drive_p - The virtual drive.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  04/23/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_virtual_drive_initiate_user_copy(fbe_virtual_drive_t *virtual_drive_p, 
                                                         fbe_packet_t *packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_initiate_user_copy_t *initiate_user_copy_p = NULL;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_virtual_drive_flags_t               flags;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;

    /* Get the control operation
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Validate the usurper buffer
     */
    status = fbe_virtual_drive_usurper_get_control_buffer(virtual_drive_p, packet_p,
                                                          sizeof(fbe_virtual_drive_initiate_user_copy_t),
                                                          (fbe_payload_control_buffer_t *)&initiate_user_copy_p);
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Get some debug information.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: usurper initiate user copy mode: %d cmd: %d flags: 0x%x swap edge: %d \n",
                          configuration_mode, initiate_user_copy_p->command_type,
                          flags, initiate_user_copy_p->swap_edge_index);


    /* If `request in progress' is set something is wrong
     */
    if (fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS))
    {
        /* Trace an error and fail the request.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s unexpected flags: 0x%x (request in progress set)\n",
                                __FUNCTION__, flags);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /*! @note For testing purposes we allow `asynchronous' completion.
     *        Normally all operations should be completed in the monitor
     *        context but when we hit a debug hook we don't want to block
     *        the job service so we allow the job to complete without waiting
     *        for the virtual drive monitor.
     */
    if (initiate_user_copy_p->b_operation_confirmation == FBE_FALSE)
    {
        /* Generate a warning message and start user copy
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: usurper initiate user copy mode: %d cmd: %d confirmation disabled \n",
                              configuration_mode, initiate_user_copy_p->command_type);

        /* Flag the fact that confirmation is disabled.
         */
        fbe_virtual_drive_set_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_CONFIRMATION_DISABLED);

        /* Ignore the return status.
         */
        fbe_virtual_drive_start_user_copy(virtual_drive_p);
    }
    else
    {
        /* Else set the virtual drive monitor condition that will initiate
         * the user copy request.
         */
        fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const,
                               (fbe_base_object_t *)virtual_drive_p,
                               FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_START_USER_COPY);

        /* Reschedule immediately
         */
        fbe_lifecycle_reschedule(&fbe_base_config_lifecycle_const,
                                 (fbe_base_object_t *)virtual_drive_p,
                                 (fbe_lifecycle_timer_msec_t) 0);    /* Immediate reschedule */
    }

    /* Return the execution status.
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_initiate_user_copy()
 ******************************************************************************/

/*!***************************************************************************
 * fbe_virtual_drive_usurper_copy_complete_set_checkpoint_to_end_marker()
 *****************************************************************************
 *
 * @brief   This usurper is sent to the virtual drive when the rebuild
 *          is done and the job service has:
 *              o Changed the configuration mode to pass-thru
 *              o Swapped out the original source drive
 *          Now the job service is requesting that the virtual drive update the
 *          rebuild checkpoints (in-fact update any non-paged metadata) as
 *          required.
 * 
 * @param   virtual_drive_p - The virtual drive.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  04/22/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_virtual_drive_usurper_copy_complete_set_checkpoint_to_end_marker(fbe_virtual_drive_t *virtual_drive_p, 
                                                                                         fbe_packet_t *packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_virtual_drive_flags_t               flags;
    fbe_edge_index_t                        swap_out_edge_index;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;

    /* Get the control operation
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Currently there is no usurper payload associated with this request.
     * Get some debug information.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);
    swap_out_edge_index = fbe_virtual_drive_get_swap_out_edge_index(virtual_drive_p);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: usurper copy complete set chkpt to end mode: %d flags: 0x%x swap edge: %d \n",
                          configuration_mode, flags, swap_out_edge_index);

    /* If `request in progress' is not set or `copy complete' or `abort copy'
     * in progress is already set something is wrong.
     */
    if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS)     ||
         fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_COPY_COMPLETE_SET_CHECKPOINT) ||
         fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_COPY_FAILED_SET_CHECKPOINT)      )
    {
        /* Trace an error and fail the request.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s unexpected flags: 0x%x (request not set etc)\n",
                                __FUNCTION__, flags);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Set the condition that will set the destination drive checkpoint
     * to the end marker.
     */
    fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t *)virtual_drive_p, 
                           FBE_RAID_GROUP_LIFECYCLE_COND_SET_REBUILD_CHECKPOINT_TO_END_MARKER);

    /* Return the execution status.
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_usurper_copy_complete_set_checkpoint_to_end_marker()
 ******************************************************************************/

/*!***************************************************************************
 * fbe_virtual_drive_usurper_copy_failed_set_checkpoint_to_end_marker()
 *****************************************************************************
 *
 * @brief   This usurper is sent to the virtual drive after a copy request
 *          failed (a.k.a. aborted).  The job service has taken the following
 *          steps:
 *              o Changed the configuration mode to pass-thru
 *              o Swapped out the original source drive
 *          Now the job service is requesting that the virtual drive update the
 *          rebuild checkpoints (in-fact update any non-paged metadata) as
 *          required.
 * 
 * @param   virtual_drive_p - The virtual drive.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  04/22/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_virtual_drive_usurper_copy_failed_set_checkpoint_to_end_marker(fbe_virtual_drive_t *virtual_drive_p, 
                                                                                       fbe_packet_t *packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_virtual_drive_flags_t               flags;
    fbe_edge_index_t                        swap_out_edge_index;
    fbe_lifecycle_state_t                   my_state = FBE_LIFECYCLE_STATE_INVALID;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;

    /* Get the control operation
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Currently there is no usurper payload associated with this request.
     * Get some debug information.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_base_object_get_lifecycle_state((fbe_base_object_t *)virtual_drive_p, &my_state);
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);
    swap_out_edge_index = fbe_virtual_drive_get_swap_out_edge_index(virtual_drive_p);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: usurper copy failed set chkpt to end mode: %d life: %d flags: 0x%x swap edge: %d \n",
                          configuration_mode, my_state, flags, swap_out_edge_index);

    /* If `request in progress' is not set or `copy complete' or `abort copy'
     * in progress is already set something is wrong.
     */
    if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS)     ||
         fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_COPY_COMPLETE_SET_CHECKPOINT) ||
         fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_COPY_FAILED_SET_CHECKPOINT)      )
    {
        /* Trace an error and fail the request.
         */
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s unexpected flags: 0x%x (request not set etc)\n",
                                __FUNCTION__, flags);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* Set the condition that will properly set the rebuild logging
     * bitmask, checkpoints and non-paged flags as required.  If we
     * in the Ready state it means the destination drive failed.
     */
    if (my_state == FBE_LIFECYCLE_STATE_READY)
    {
        /* Use the raid group class condition to quiesce I/O etc.
         */
        fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t *)virtual_drive_p, 
                               FBE_RAID_GROUP_LIFECYCLE_COND_DEST_DRIVE_FAILED_SET_REBUILD_CHECKPOINT_TO_END_MARKER);
    }
    else
    {
        /* Else use the condition that simply updates the checkpoints.
         */
        fbe_lifecycle_set_cond(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t *)virtual_drive_p,
                               FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_COPY_FAILED_SET_REBUILD_CHECKPOINT_TO_END_MARKER);
    }

    /* Return the execution status.
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_usurper_copy_failed_set_checkpoint_to_end_marker()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_parent_mark_needs_rebuild_done()
 ******************************************************************************
 *
 * @brief   This usurper performs (2) actions:
 *              o Clear the swapped edge metadata flag
 *              o Set the rebuild checkpoint to the end marker
 *          This command is used by the parent raid group(s) to `reset' the
 *          virtual drive back into a `pass-thru' mode.  This usurper is sent
 *          after the parent raid group has marked needs rebuild.  Since we are
 *          about to publish that the virtual drive is fully rebuilt ALL parent
 *          raid groups MUST populate NR for any chunks that were not rebuilt
 *          by the virtual drive. 
 * 
 * @param   virtual_drive_p - The virtual drive.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return  status - The status of the operation.
 *
 * @todo    Currently this interface only supports a SINGLE parent raid group!!!
 *
 * @author
 *  09/01/2011 - Created. Ashwin Tamilarasan
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_parent_mark_needs_rebuild_done(fbe_virtual_drive_t *virtual_drive_p, 
                                                                     fbe_packet_t *packet_p)
{
    fbe_status_t                        status = FBE_STATUS_OK;
    fbe_bool_t                          b_is_edge_swapped = FBE_TRUE;
    fbe_bool_t                          b_is_degraded_needs_rebuild = FBE_FALSE;
    fbe_payload_ex_t                   *payload_p = NULL;
    fbe_payload_control_operation_t    *control_operation_p = NULL;
    fbe_edge_index_t                    edge_index_to_set = FBE_EDGE_INDEX_INVALID;

    /* Get the payload nad control operation.
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Use the virtual drive specific method to locate the disk that needs it
     * rebuild checkpoint set.
     */
    fbe_virtual_drive_parent_mark_needs_rebuild_done_get_edge_to_set_to_end_marker(virtual_drive_p, &edge_index_to_set);

    /* Determine if the edge is still swapped or not.
     */
    fbe_virtual_drive_metadata_is_edge_swapped(virtual_drive_p, &b_is_edge_swapped);

    /* Determine if degraded needs rebuild is set or noy
     */
    fbe_virtual_drive_metadata_is_degraded_needs_rebuild(virtual_drive_p, &b_is_degraded_needs_rebuild);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s swapped: %d degraded: %d index to set: %d \n", 
                          __FUNCTION__, b_is_edge_swapped, b_is_degraded_needs_rebuild, edge_index_to_set);


    /*! @note Only if the swapped bit is set do we need to take any action.
     *        We take the following actions in a specific order:
     *          1) Set the rebuild checkpoint to the end marker
     *          2) Clear the swapped bit (if required)
     *
     *  @note This currently only supports a single parent raid group.
     */
    if ((edge_index_to_set != FBE_EDGE_INDEX_INVALID) ||
        (b_is_edge_swapped == FBE_TRUE)               ||
        (b_is_degraded_needs_rebuild == FBE_TRUE)        )
    {
        /* Set the completion to cleanup.
         */
        fbe_transport_set_completion_function(packet_p, 
                                              fbe_virtual_drive_parent_mark_needs_rebuild_done_completion, 
                                              virtual_drive_p);  

        /* Set the end marker
         */
        status = fbe_virtual_drive_parent_mark_needs_rebuild_done_set_checkpoint_to_end_marker(virtual_drive_p, packet_p);
        if (status == FBE_STATUS_MORE_PROCESSING_REQUIRED)
        {
            return status;
        }
    }

    /* Return the execution status.
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_parent_mark_needs_rebuild_done()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_send_request_to_mark_consumed()
 ******************************************************************************
 * @brief
 * This function sends a request to provision drive to mark entire drive as consumed.
 * A new drive is going to be part of raid group so pvd has to correct its map to mark entire
 * virtual drive's imported capacity as consumed. This function expect that virtual drive is in
 * either passthrough first mode or passthrough second mode while sending mark_consume 
 * request to pvd.
 *
 *
 * @param virtual_drive_p - The virtual drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/20/2011 - Created. Amit Dhaduk
 *
 ******************************************************************************/
fbe_status_t
fbe_virtual_drive_send_request_to_mark_consumed(fbe_virtual_drive_t * virtual_drive_p,
                                                fbe_packet_t * packet_p)
{
    fbe_payload_control_operation_t         *control_operation_p = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_block_edge_t                       *block_edge_p = NULL;
    fbe_lba_t                               virtual_drive_exported_capacity;
    fbe_virtual_drive_metadata_positions_t  virtual_drive_metadata_position;
    fbe_lba_t                               start_lba;
    fbe_block_count_t                       block_count;
    fbe_packet_t                           *new_packet_p = NULL;
    fbe_optimum_block_size_t                optimum_block_size;
    fbe_payload_block_operation_t          *block_operation_p;  // block operation
    fbe_payload_ex_t                       *sep_payload_p = NULL;

    //  Get the sep payload and control operation
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);

    fbe_virtual_drive_lock(virtual_drive_p);
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_virtual_drive_unlock(virtual_drive_p);

    /* find out the block edge */
    if(fbe_virtual_drive_is_mirror_configuration_mode_set(virtual_drive_p))
    {

        fbe_base_object_trace(  (fbe_base_object_t *)virtual_drive_p,
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                                "%s entry fail as it is mirror configuration \n", __FUNCTION__);

        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
       
    }
    else if(configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)
    { 
        fbe_base_config_get_block_edge((fbe_base_config_t *) virtual_drive_p, &block_edge_p, FIRST_EDGE_INDEX);
            
    }
    else if(configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)
    { 
        fbe_base_config_get_block_edge((fbe_base_config_t *) virtual_drive_p, &block_edge_p, SECOND_EDGE_INDEX);
            
    }

    /* Initialize the vd's consumed space which is imported capacity (exported capacity + metadata) */
    start_lba = 0;
    fbe_base_config_get_capacity((fbe_base_config_t *) virtual_drive_p, &virtual_drive_exported_capacity);
    fbe_virtual_drive_get_metadata_positions(virtual_drive_p,  &virtual_drive_metadata_position);
    block_count = virtual_drive_exported_capacity  + virtual_drive_metadata_position.paged_metadata_capacity;

    /* get optimum block size for this i/o request */
    fbe_block_transport_edge_get_optimum_block_size(block_edge_p, &optimum_block_size);

    /* allocate a new block IO packet to send mark consume block request to pvd 
     * we have need to allocate a new packet to reduce packet stack
     */
    new_packet_p = fbe_transport_allocate_packet();
    fbe_transport_initialize_sep_packet(new_packet_p);

    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "vd: Send mark consumed start_lba:0x%llx, b_cnt:0x%llx, block packet 0x%p\n",
                          (unsigned long long)start_lba,
			  (unsigned long long)block_count, new_packet_p);

    /* get the sep payload */
    sep_payload_p = fbe_transport_get_payload_ex(new_packet_p);

    /* set-up block operation in sep payload */
    block_operation_p = fbe_payload_ex_allocate_block_operation(sep_payload_p);

    /* next, build block operation in sep payload */
    fbe_payload_block_build_operation( block_operation_p,       // pointer to block operation in sep payload
                                       FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_MARK_CONSUMED,               // block i/o operation code
                                       start_lba,            // block i/o starting lba
                                       block_count,          // block i/o count
                                       FBE_BE_BYTES_PER_BLOCK,  // block size
                                       optimum_block_size,      // optimum block size
                                       NULL);

    fbe_transport_set_completion_function(new_packet_p, 
                                          fbe_virtual_drive_send_request_to_mark_consumed_completion,
                                          virtual_drive_p);

    /* Propagate the current expiration time */
    fbe_transport_propagate_expiration_time(new_packet_p, packet_p);
    fbe_transport_propagate_physical_drive_service_time(new_packet_p, packet_p);

    /* Propagate the io_stamp */
    fbe_transport_set_io_stamp_from_master(packet_p, new_packet_p);

    /* Add the control packet to the usurper queue */
    fbe_transport_add_subpacket(packet_p, new_packet_p);
    fbe_transport_set_cancel_function(packet_p, fbe_base_object_packet_cancel_function, (fbe_base_object_t *)virtual_drive_p);
    fbe_base_object_add_to_usurper_queue((fbe_base_object_t*)virtual_drive_p, packet_p);

    /* now, activate this block operation  */
    fbe_payload_ex_increment_block_operation_level( sep_payload_p );

    /* find out the downstream edge connected with pvd to send block IO packet */
    if(configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE)
    {
        /* Forward the packet to the first downstream edge connected with pvd. */
        status = fbe_base_config_send_functional_packet((fbe_base_config_t *) virtual_drive_p, new_packet_p, 0);
    }
    else if(configuration_mode == FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE)
    {
        /* Forward the packet to the second downstream edge connected with pvd. */
        status = fbe_base_config_send_functional_packet((fbe_base_config_t *) virtual_drive_p, new_packet_p, 1);
    }
    else
    {
        /* It does not expect configuration mode to setup mirror when it is here. */
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s invalid configuration mode, config_mode:0x%x\n",
                              __FUNCTION__, configuration_mode);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;

}
/******************************************************************************
 * end fbe_virtual_drive_send_request_to_mark_consumed()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_send_request_to_mark_consumed_completion()
 ******************************************************************************
 * @brief
 *  This function is used as completion function for the mark consumed request.
 *
 * @param virtual_drive_p - The provision drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  10/20/2011 - Created. Amit Dhaduk
 *
 ******************************************************************************/
static fbe_status_t 
fbe_virtual_drive_send_request_to_mark_consumed_completion(fbe_packet_t * packet_p, 
                                                           fbe_packet_completion_context_t context)
{
    fbe_virtual_drive_t                    *virtual_drive_p = NULL;
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                       *sep_payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;
    fbe_payload_block_operation_t          *block_operation_p;  // block operation
    fbe_payload_block_operation_status_t    block_status;
    fbe_packet_t                           *master_packet_p = NULL;

    virtual_drive_p = (fbe_virtual_drive_t *)context;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    block_operation_p = fbe_payload_ex_get_block_operation(sep_payload_p);  

    master_packet_p = (fbe_packet_t *)fbe_transport_get_master_packet(packet_p);
    fbe_transport_remove_subpacket(packet_p);

    /* Remove master packet from the usurper queue. */
    fbe_base_object_remove_from_usurper_queue((fbe_base_object_t *)virtual_drive_p, master_packet_p);

    if (block_operation_p == NULL) {
        fbe_transport_release_packet(packet_p);
        sep_payload_p = fbe_transport_get_payload_ex(master_packet_p);
        control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p); 
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(master_packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_block_get_status(block_operation_p, &block_status);
    status = fbe_transport_get_status_code(packet_p);
    fbe_payload_ex_release_block_operation(sep_payload_p, block_operation_p); 


    /*we don't need the sub packet anymore*/
    fbe_transport_release_packet(packet_p);

    sep_payload_p = fbe_transport_get_payload_ex(master_packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p); 


    if ((FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS != block_status) || (status != FBE_STATUS_OK)) {

        fbe_base_object_trace(  (fbe_base_object_t *)virtual_drive_p,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "vd: Mark consumed failed, packet 0x%p, packet status 0x%x, block status 0x%x\n", packet_p, status, block_status);

        status = (status != FBE_STATUS_OK) ? status : FBE_STATUS_GENERIC_FAILURE;

        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(master_packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(master_packet_p);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "vd: Mark consumed master: 0x%p packet 0x%p packet status 0x%x, block status 0x%x\n", 
                          master_packet_p, packet_p, status, block_status);

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(master_packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(master_packet_p);


    return FBE_STATUS_OK;

    
}
/******************************************************************************
 * end fbe_virtual_drive_send_request_to_mark_consumed_completion()
 ******************************************************************************/

/*!****************************************************************************
 * fbe_virtual_drive_get_info()
 ******************************************************************************
 * @brief
 *  Get the following 
 *
 * @param virtual_drive_p - The provision drive.
 * @param packet_p - The packet requesting this operation.
 *
 * @return status - The status of the operation.
 *
 * @author
 *  04/16/2012 - Created. Shay Harel
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_get_info(fbe_virtual_drive_t * virtual_drive_p, 
                                                                    fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;
    fbe_virtual_drive_get_info_t           *virtual_drive_info_p = NULL;
    fbe_block_edge_t                       *first_edge_p = NULL;
    fbe_block_edge_t                       *second_edge_p = NULL;
    fbe_path_state_t                        first_edge_path_state = FBE_PATH_STATE_INVALID;
    fbe_path_state_t                        second_edge_path_state = FBE_PATH_STATE_INVALID;
    fbe_lba_t                               first_edge_checkpoint = 0;
    fbe_lba_t                               second_edge_checkpoint = 0;
    fbe_virtual_drive_configuration_mode_t  configuration_mode = FBE_VIRTUAL_DRIVE_CONFIG_MODE_UNKNOWN;
    fbe_object_id_t                         orig_pvd_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                         first_edge_object_id = FBE_OBJECT_ID_INVALID;
    fbe_object_id_t                         second_edge_object_id = FBE_OBJECT_ID_INVALID;
    fbe_virtual_drive_flags_t               flags;
    fbe_edge_index_t                        swap_out_edge_index = FBE_EDGE_INDEX_INVALID;

    fbe_base_object_trace(  (fbe_base_object_t *)virtual_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    status = fbe_virtual_drive_usurper_get_control_buffer(virtual_drive_p, packet_p,
                                                          sizeof(fbe_virtual_drive_get_info_t),
                                                          (fbe_payload_control_buffer_t *)&virtual_drive_info_p);
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    /* If we cannot get the swap-out edge index fail the request. Use the same 
     * method as virtual drive monitor to determine which edge will be swapped 
     * out.
     */
    status = fbe_virtual_drive_swap_get_swap_out_edge_index(virtual_drive_p, &swap_out_edge_index); 
    if (status != FBE_STATUS_OK)
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s failed to get swap out index - status: 0x%x\n",
                              __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }
    virtual_drive_info_p->swap_edge_index = swap_out_edge_index;

    /* Get the information for both edges
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_virtual_drive_get_orig_pvd_object_id(virtual_drive_p, &orig_pvd_object_id);
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, FIRST_EDGE_INDEX, &first_edge_checkpoint);        
    fbe_raid_group_get_rebuild_checkpoint((fbe_raid_group_t *)virtual_drive_p, SECOND_EDGE_INDEX, &second_edge_checkpoint);
    fbe_base_config_get_block_edge((fbe_base_config_t*)virtual_drive_p, &first_edge_p, FIRST_EDGE_INDEX);
    fbe_block_transport_get_path_state(first_edge_p, &first_edge_path_state);
    fbe_base_config_get_block_edge((fbe_base_config_t*)virtual_drive_p, &second_edge_p, SECOND_EDGE_INDEX);
    fbe_block_transport_get_path_state(second_edge_p, &second_edge_path_state);
    virtual_drive_info_p->b_user_copy_started = fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_USER_COPY_STARTED);

    /*! @note Notice we use the clustered flag.  Once this flag is cleared
     *        as far as the job service is concerned the request is complete.
     *        But the virtual drive will not consider the job complete until
     *        the FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS is cleared.
     *        This local flag is cleared at the very end of the job.
     */
    virtual_drive_info_p->b_request_in_progress = fbe_raid_group_is_clustered_flag_set((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS);

    virtual_drive_info_p->b_is_swap_in_complete = fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_IN_EDGE_COMPLETE);
    virtual_drive_info_p->b_is_change_mode_complete = fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_CHANGE_CONFIG_MODE_COMPLETE);
    virtual_drive_info_p->b_is_copy_complete = fbe_virtual_drive_is_copy_complete(virtual_drive_p);
    virtual_drive_info_p->b_is_swap_out_complete = fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_OUT_COMPLETE);
    virtual_drive_info_p->b_copy_complete_in_progress = fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_COPY_COMPLETE_SET_CHECKPOINT);
    virtual_drive_info_p->b_copy_failed_in_progress = fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_COPY_FAILED_SET_CHECKPOINT);
    
    /* Get the first and second edge object ids based on the edge state.
     */
    if ((first_edge_path_state != FBE_PATH_STATE_INVALID) &&
        (first_edge_path_state != FBE_PATH_STATE_GONE)       )
    {
        first_edge_object_id = first_edge_p->base_edge.server_id;
    }
    if ((second_edge_path_state != FBE_PATH_STATE_INVALID) &&
        (second_edge_path_state != FBE_PATH_STATE_GONE)       )
    {
        second_edge_object_id = second_edge_p->base_edge.server_id;
    }

    /* Populate the information based on configuration mode.
     */
    virtual_drive_info_p->configuration_mode = configuration_mode;
    switch(configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
            /* We are in pass-thru first edge:
             *  o Rebuild checkpoint is checkpoint for first edge
             *  o original object id is object id for first edge
             *  o spare object id is invalid
             */
            virtual_drive_info_p->vd_checkpoint = first_edge_checkpoint;
            virtual_drive_info_p->original_pvd_object_id = first_edge_p->base_edge.server_id;
            if (first_edge_object_id == FBE_OBJECT_ID_INVALID)
            {
                /* Report the original object id */
                virtual_drive_info_p->original_pvd_object_id = orig_pvd_object_id;
            }
            virtual_drive_info_p->spare_pvd_object_id = second_edge_object_id;
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            /* We are in pass-thru second edge:
             *  o Rebuild checkpoint is checkpoint for second edge
             *  o original object id is object id for second edge
             *  o spare object id is invalid
             */
            virtual_drive_info_p->vd_checkpoint = second_edge_checkpoint;
            virtual_drive_info_p->original_pvd_object_id = second_edge_p->base_edge.server_id;
            if (second_edge_object_id == FBE_OBJECT_ID_INVALID)
            {
                /* Report the original object id */
                virtual_drive_info_p->original_pvd_object_id = orig_pvd_object_id;
            }
            virtual_drive_info_p->spare_pvd_object_id = first_edge_object_id;
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            /* We are in mirror mode first edge:
             *  o Rebuild checkpoint is checkpoint for second edge
             *  o original object id is object id for first edge
             *  o spare object id is object id for second edge
             */
            virtual_drive_info_p->vd_checkpoint = second_edge_checkpoint;
            virtual_drive_info_p->original_pvd_object_id = first_edge_object_id;
            virtual_drive_info_p->spare_pvd_object_id = second_edge_object_id;
            break;

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:  
            /* We are in mirror mode second edge:
             *  o Rebuild checkpoint is checkpoint for first edge
             *  o original object id is object id for second edge
             *  o spare object id is object id of first edge
             */
            virtual_drive_info_p->vd_checkpoint = first_edge_checkpoint;
            virtual_drive_info_p->original_pvd_object_id = second_edge_object_id;
            virtual_drive_info_p->spare_pvd_object_id = first_edge_object_id;
            break;

        default:
            /* unsupported configuration mode */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR, 
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "%s unknown config mode: %d\n", 
                                  __FUNCTION__, configuration_mode);
            fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_OK;
    }

    /* This usurper is invoked many times.  Only trace if enabled.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_CONFIG_TRACING,
                            "virtual_drive: get info mode: %d flags: 0x%x swap: %d orig: 0x%x spare: 0x%x chkpt: 0x%llx \n",
                            virtual_drive_info_p->configuration_mode,
                            flags,
                            virtual_drive_info_p->swap_edge_index, 
                            virtual_drive_info_p->original_pvd_object_id,
                            virtual_drive_info_p->spare_pvd_object_id,
                            virtual_drive_info_p->vd_checkpoint);

    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;

}
/******************************************************************************
 * end fbe_virtual_drive_get_info()
 ******************************************************************************/

/*!**************************************************************
 * fbe_virtual_drive_usurper_set_nonpaged_metadata()
 ****************************************************************
 * @brief
 *  This function sets the nonpaged MD for VD.
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
fbe_virtual_drive_usurper_set_nonpaged_metadata(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_base_config_control_set_nonpaged_metadata_t* control_set_nonpaged_p = NULL;

    fbe_payload_ex_t *                      payload_p = NULL;
    fbe_payload_control_operation_t *       control_operation_p = NULL; 

    // Get the control buffer pointer from the packet payload.
    status = fbe_virtual_drive_usurper_get_control_buffer(virtual_drive_p, packet_p, 
                                            sizeof(fbe_base_config_control_set_nonpaged_metadata_t),
                                            (fbe_payload_control_buffer_t)&control_set_nonpaged_p);
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

        status = fbe_raid_group_metadata_set_default_nonpaged_metadata((fbe_raid_group_t *)virtual_drive_p,packet_p);
    }else{
        //  Send the nonpaged metadata write request to metadata service. 
        status = fbe_base_config_metadata_nonpaged_write_persist((fbe_base_config_t *) virtual_drive_p,
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
 *          fbe_virtual_drive_usurper_send_swap_command_complete()
 *****************************************************************************
 *
 * @brief   This usurper is sent to the virtual drive from the job service to
 *          tell the virtual drive that the swap job is now complete.  This
 *          routine simply clear the FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS
 *          flag.
 * 
 * @param   virtual_drive_p - The virtual drive.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  10/11/2013  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_virtual_drive_usurper_send_swap_command_complete(fbe_virtual_drive_t *virtual_drive_p, 
                                                                         fbe_packet_t *packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_virtual_drive_flags_t               flags;
    fbe_spare_swap_command_t                swap_command;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;

    /* Get the control operation
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Currently there is no usurper payload associated with this request.
     * Get some debug information.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);
    fbe_virtual_drive_get_copy_request_type(virtual_drive_p, &swap_command);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: usurper swap command complete mode: %d cmd: %d flags: 0x%x \n",
                          configuration_mode, swap_command, flags);

    /* If `request in progress' is not set or `copy complete' or `abort copy'
     * in progress is already set something is wrong.  But this may be a rollback 
     * so just complete the request. 
     */
    if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS))
    {
        /*! @note Special case are user initiated commands.  If they fail 
         *        they fail during validation reduce trace to warning.
         */
        if ((swap_command == FBE_SPARE_INITIATE_USER_COPY_COMMAND)    ||
            (swap_command == FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND)    )
        {
            /* Trace a warning and continue.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_WARNING,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "virtual_drive: usurper swap command complete user copy flags: 0x%x request not set \n",
                                  flags);
        }
        else
        {
            /* Trace an error and continue.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s unexpected flags: 0x%x (request not set)\n",
                                  __FUNCTION__, flags);
        }
    }

    /* Clear the confirmation disabled flag.
     */
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_CONFIRMATION_DISABLED);

    /* Clear the operation complete flags.
     */
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_OPERATION_COMPLETE_MASK);

    /* Simply clear the flag here.
     */
    fbe_virtual_drive_clear_flag(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS);

    /* We need to clear the clustered flag job in progress in case we take over when the peer dies.
     */
    fbe_raid_group_clear_clustered_flag((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS);

    /* Return the execution status.
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_usurper_send_swap_command_complete()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_usurper_update_drive_keys_completion()
 *****************************************************************************
 *
 * @brief   This is the completion function for registering keys.
 * 
 * @param   packet_p - The packet requesting this operation.
 * @param   context - The virtual drive.
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  01/06/2014  Lili Chen  - Created.
 *
 *****************************************************************************/
static fbe_status_t 
fbe_virtual_drive_usurper_update_drive_keys_completion(fbe_packet_t * packet_p,
                                                   fbe_packet_completion_context_t context)
{
    fbe_virtual_drive_t                 *virtual_drive_p = NULL;
    fbe_base_config_encryption_state_t  state;

    virtual_drive_p = (fbe_virtual_drive_t *)context;

    fbe_virtual_drive_lock(virtual_drive_p);

    fbe_base_config_get_encryption_state((fbe_base_config_t *)virtual_drive_p, &state);
    if (state == FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_OUT_OLD_KEY_NEED_REMOVED)
    {
        fbe_base_config_set_encryption_state((fbe_base_config_t *)virtual_drive_p, 
                                             FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_OUT_OLD_KEY_REMOVED);
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption: SWAP_OUT_OLD_KEY_REMOVED, orig state %d\n", state);
    } 
    else if (state == FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_IN_NEW_KEY_REQUIRED) 
    {
        fbe_base_config_set_encryption_state((fbe_base_config_t *)virtual_drive_p, 
                                             FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_IN_NEW_KEY_PROVIDED);
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption: SWAP_IN_NEW_KEY_PROVIDED, orig state %d\n", state);
    }
    fbe_virtual_drive_unlock(virtual_drive_p);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_usurper_update_drive_keys_completion()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_usurper_update_drive_keys()
 *****************************************************************************
 *
 * @brief   This usurper is sent to the virtual drive from the raid group.
 *          The vitual drive should know how to send to PVD.
 * 
 * @param   virtual_drive_p - The virtual drive.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  01/06/2014  Lili Chen  - Created.
 *
 *****************************************************************************/
static fbe_status_t
fbe_virtual_drive_usurper_update_drive_keys(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_p = NULL;
    fbe_block_edge_t                       *block_edge_p = NULL;
    fbe_base_config_encryption_state_t      state;
    fbe_object_id_t                         orig_pvd_object_id;
    fbe_edge_index_t                        swap_in_edge_index = FBE_EDGE_INDEX_INVALID;
    fbe_provision_drive_control_register_keys_t *register_keys = NULL;
    fbe_edge_index_t                        primary_edge_index, secondary_edge_index;
    fbe_path_state_t                        primary_edge_path_state, secondary_edge_path_state;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_get_control_operation(payload_p);
    status = fbe_virtual_drive_usurper_get_control_buffer(virtual_drive_p, packet_p, 
                                        sizeof(fbe_provision_drive_control_register_keys_t),
                                        (fbe_payload_control_buffer_t)&register_keys);
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status;
    }

    fbe_virtual_drive_get_orig_pvd_object_id(virtual_drive_p, &orig_pvd_object_id);
    fbe_base_config_get_encryption_state((fbe_base_config_t *)virtual_drive_p, &state);

    fbe_virtual_drive_get_primary_edge_index(virtual_drive_p, &primary_edge_index);
    fbe_virtual_drive_get_secondary_edge_index(virtual_drive_p, &secondary_edge_index);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[primary_edge_index],
                                       &primary_edge_path_state);
    fbe_block_transport_get_path_state(&((fbe_base_config_t *) virtual_drive_p)->block_edge_p[secondary_edge_index],
                                       &secondary_edge_path_state);
    fbe_base_config_get_block_edge((fbe_base_config_t *) virtual_drive_p, &block_edge_p, primary_edge_index);

    /* It is possible that in passive SP the VD has no chance to set the state yet. 
     * We need to set the state here before forwarding the packet to PVD.
     */
    if (!fbe_base_config_is_active((fbe_base_config_t*)virtual_drive_p) && 
        (state != FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_OUT_OLD_KEY_NEED_REMOVED) &&
        (state != FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_IN_NEW_KEY_REQUIRED))
    {
        if (register_keys->key_handle != NULL) {
            if (register_keys->key_handle_paco) {
                fbe_base_config_set_encryption_state((fbe_base_config_t *)virtual_drive_p, 
                                                     FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_IN_NEW_KEY_REQUIRED);
            } else /*if (!fbe_encryption_key_mask_has_valid_key(&register_keys->key_handle->key_mask))*/ {
                fbe_base_config_set_encryption_state((fbe_base_config_t *)virtual_drive_p, 
                                                     FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_OUT_OLD_KEY_NEED_REMOVED);
            }
        } else {
            /* We could get NULL keys from RG. So we should get encryption state from edge state */
            if ((primary_edge_path_state != FBE_PATH_STATE_INVALID) && (secondary_edge_path_state != FBE_PATH_STATE_INVALID)) {
                fbe_base_config_set_encryption_state((fbe_base_config_t *)virtual_drive_p, 
                                                     FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_IN_NEW_KEY_REQUIRED);
            } else {
                fbe_base_config_set_encryption_state((fbe_base_config_t *)virtual_drive_p, 
                                                     FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_OUT_OLD_KEY_NEED_REMOVED);
            }
        }
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s passive side not ready yet, state %d\n", __FUNCTION__, state);
        fbe_raid_group_set_clustered_flag((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS);
        /* Get the state again */
        fbe_base_config_get_encryption_state((fbe_base_config_t *)virtual_drive_p, &state);
    }

    if (state == FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_OUT_OLD_KEY_NEED_REMOVED)
    {
        if ((register_keys->key_handle != NULL) && fbe_encryption_key_mask_has_valid_key(&register_keys->key_handle->key_mask) ||
            (register_keys->key_handle == NULL) && (primary_edge_path_state != FBE_PATH_STATE_INVALID)) {
            /* For copy, we forward the packet to primary edge to update key handle. */
            fbe_transport_set_completion_function(packet_p, fbe_virtual_drive_usurper_update_drive_keys_completion, virtual_drive_p);
            status = fbe_block_transport_send_control_packet(block_edge_p, packet_p);
        } else {
            /* For permanent spare. */
            /* We should already unregister keys when the edge is destroyed. */
            fbe_payload_control_set_status(control_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
            fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet_p);
            fbe_base_config_set_encryption_state((fbe_base_config_t *)virtual_drive_p, 
                                                 FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_OUT_OLD_KEY_REMOVED);
            fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                 "Encryption: SWAP_OUT_OLD_KEY_REMOVED, orig state %d\n", state);
        }
    } 
    else if (state == FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_IN_NEW_KEY_REQUIRED) 
    {
        /* We need to send the keys to swap-in edge only. Set the key_handle to be the paco keys */
        /* For permanent spare, the keys are stored in key_handle */
        if (register_keys->key_handle_paco) {
            register_keys->key_handle = register_keys->key_handle_paco;
        }

        fbe_transport_set_completion_function(packet_p, fbe_virtual_drive_usurper_update_drive_keys_completion, virtual_drive_p);
        swap_in_edge_index = fbe_virtual_drive_get_swap_in_edge_index(virtual_drive_p);
        fbe_base_config_get_block_edge((fbe_base_config_t *) virtual_drive_p, &block_edge_p, swap_in_edge_index);
        status = fbe_block_transport_send_control_packet(block_edge_p, packet_p);
    }
    else if (fbe_private_space_layout_object_id_is_system_pvd(orig_pvd_object_id) &&
             ((primary_edge_path_state == FBE_PATH_STATE_BROKEN) || (primary_edge_path_state == FBE_PATH_STATE_DISABLED) ||
              (primary_edge_path_state == FBE_PATH_STATE_ENABLED)))
    {
        /* This is a special case where a system drive has been replaced and there was no real spare and so
         * we need to allow the udpate to go through. We are ignoring the state checks because it is not a
         * sparing scenario. This is real ugly though */
        fbe_transport_set_completion_function(packet_p, fbe_virtual_drive_usurper_update_drive_keys_completion, virtual_drive_p);
        status = fbe_block_transport_send_control_packet(block_edge_p, packet_p);
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                 "Encryption: Force Update of keys for PVD ID:0x%08x, orig state %d\n", orig_pvd_object_id, state);
    }
    else
    {
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p, 
                              FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s bad encryption state %d\n", __FUNCTION__, state);
        fbe_payload_control_set_status(control_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet_p);
    }

    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_usurper_update_drive_keys()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_swap_out_remove_keys()
 *****************************************************************************
 *
 * @brief   This function is used to send usurper to PVD to remove keys when an.
 *          edge is swapped out.
 * 
 * @param   virtual_drive_p - The virtual drive.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  01/06/2014  Lili Chen  - Created.
 *
 *****************************************************************************/
static fbe_status_t 
fbe_virtual_drive_swap_out_remove_keys(fbe_virtual_drive_t * virtual_drive_p, 
									   fbe_packet_t * packet_p, 
									   fbe_edge_index_t swap_out_edge_index)
{

    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_block_edge_t *edge_p = NULL;
    fbe_system_encryption_mode_t system_encryption_mode;

    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    fbe_database_get_system_encryption_mode(&system_encryption_mode);
    if (system_encryption_mode != FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED)
    { 
        /* The system is NOT in encrypted mode */
        return FBE_STATUS_OK;
    }

    /* Get the payload pointer from the packet. */
    payload = fbe_transport_get_payload_ex(packet_p);
    if (payload == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Failed to get the payload from packet\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_allocate_control_operation(payload); 
    if(control_operation == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Failed to allocate control operation\n",__FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Create the payload control operation to detach edge the edge. */
    fbe_payload_control_build_operation(control_operation,
                                        FBE_PROVISION_DRIVE_CONTROL_CODE_UNREGISTER_KEYS,
                                        NULL,
                                        0);

    fbe_transport_set_sync_completion_type(packet_p, FBE_TRANSPORT_COMPLETION_TYPE_MORE_PROCESSING_REQUIRED);

    /* Send control packet on edges, it will be trasported to server object-id of edge. */
    fbe_base_config_get_block_edge((fbe_base_config_t *)virtual_drive_p, &edge_p, swap_out_edge_index);
    fbe_block_transport_send_control_packet(edge_p, packet_p);

    fbe_transport_wait_completion(packet_p);

    fbe_payload_ex_release_control_operation(payload, control_operation);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_swap_out_remove_keys()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_init_encryption_state()
 *****************************************************************************
 *
 * @brief   This Function is used to set encryption state during edge swap-out
 *          or swap-in.
 * 
 * @param   virtual_drive_p - The virtual drive.
 * @param   swap_out - during swap_out or swap_in job.
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  01/22/2014  Lili Chen  - Created.
 *
 *****************************************************************************/
static fbe_status_t 
fbe_virtual_drive_init_encryption_state(fbe_virtual_drive_t * virtual_drive_p, 
                                        fbe_bool_t swap_out)
{
    fbe_system_encryption_mode_t system_encryption_mode;
    fbe_base_config_encryption_state_t state;

    fbe_database_get_system_encryption_mode(&system_encryption_mode);
    if (system_encryption_mode != FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED)
    { 
        /* The system is NOT in encrypted mode */
        return FBE_STATUS_OK;
    }

    if (!fbe_virtual_drive_is_flag_set(virtual_drive_p, FBE_VIRTUAL_DRIVE_FLAG_SWAP_REQUEST_IN_PROGRESS) &&
        !fbe_raid_group_is_clustered_flag_set((fbe_raid_group_t *)virtual_drive_p, FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS))
    {
        /* The VD is not in swap */
        return FBE_STATUS_OK;
    }

    fbe_virtual_drive_lock(virtual_drive_p);
    fbe_base_config_get_encryption_state((fbe_base_config_t *)virtual_drive_p, &state);
    if (swap_out)
    {
        fbe_base_config_set_encryption_state((fbe_base_config_t *)virtual_drive_p, 
                                             FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_OUT_OLD_KEY_NEED_REMOVED);
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption: SWAP_OUT_OLD_KEY_NEED_REMOVED, orig state %d\n", state);
    }
	else
    {
        fbe_base_config_set_encryption_state((fbe_base_config_t *)virtual_drive_p, 
                                             FBE_BASE_CONFIG_ENCRYPTION_STATE_SWAP_IN_NEW_KEY_REQUIRED);
        fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                              "Encryption: SWAP_IN_NEW_KEY_REQUIRED, orig state %d\n", state);
    }
    fbe_virtual_drive_unlock(virtual_drive_p);

    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_init_encryption_state()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_usurper_register_source_drive_keys_completion()
 *****************************************************************************
 *
 * @brief   This is the completion function for registering source drive keys.
 * 
 * @param   packet_p - The packet requesting this operation.
 * @param   context - The virtual drive.
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  01/24/2014  Lili Chen  - Created.
 *
 *****************************************************************************/
static fbe_status_t 
fbe_virtual_drive_usurper_register_source_drive_keys_completion(fbe_packet_t * packet_p,
                                                                fbe_packet_completion_context_t context)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_virtual_drive_t                    *virtual_drive_p = NULL;
    fbe_edge_index_t                        dest_edge_index;
    fbe_block_edge_t                       *block_edge_p = NULL;
    fbe_provision_drive_control_register_keys_t *register_keys = NULL;

    virtual_drive_p = (fbe_virtual_drive_t *)context;

    status = fbe_virtual_drive_usurper_get_control_buffer(virtual_drive_p, packet_p, 
                                            sizeof(fbe_provision_drive_control_register_keys_t),
                                            (fbe_payload_control_buffer_t)&register_keys);

    /* Set the key_handle to be the paco keys */
    register_keys->key_handle = register_keys->key_handle_paco;

    /* Send the packet to secondary edge */
    fbe_virtual_drive_get_secondary_edge_index(virtual_drive_p, &dest_edge_index);
    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s sending to secondary edge %d\n", __FUNCTION__, dest_edge_index);

    fbe_base_config_get_block_edge((fbe_base_config_t *) virtual_drive_p, &block_edge_p, dest_edge_index);
    status = fbe_block_transport_send_control_packet(block_edge_p, packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_virtual_drive_usurper_register_source_drive_keys_completion()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_usurper_register_drive_keys()
 *****************************************************************************
 *
 * @brief   This usurper is sent to the virtual drive from the raid group.
 *          The vitual drive should know how to send to PVD.
 * 
 * @param   virtual_drive_p - The virtual drive.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  01/24/2014  Lili Chen  - Created.
 *
 *****************************************************************************/
static fbe_status_t
fbe_virtual_drive_usurper_register_drive_keys(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    fbe_block_edge_t                       *block_edge_p = NULL;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_edge_index_t                        edge_index, source_edge_index;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_p = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    switch (configuration_mode)
    {
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_FIRST_EDGE:
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_PASS_THRU_SECOND_EDGE:
            /* For pass through mode, we forward the packet to primary edge */
            fbe_virtual_drive_get_primary_edge_index(virtual_drive_p, &edge_index);
            fbe_base_config_get_block_edge((fbe_base_config_t *) virtual_drive_p, &block_edge_p, edge_index);
            return (fbe_block_transport_send_control_packet(block_edge_p, packet_p));

        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_FIRST_EDGE:
            /* Set the source and destination edge indexes.
             */
            source_edge_index = FIRST_EDGE_INDEX;
            break;
        case FBE_VIRTUAL_DRIVE_CONFIG_MODE_MIRROR_SECOND_EDGE:
            /* Set the source and destination edge indexes.
             */
            source_edge_index = SECOND_EDGE_INDEX;
            break;
        default:
            /* Unexpected configuration mode.
             */
            fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_INFO,
                                  "virtual_drive: mode: %d unexpected mode\n",
                                  configuration_mode);

            fbe_payload_control_set_status(control_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
            fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet_p);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s sending to primary edge %d\n", __FUNCTION__, source_edge_index);

    /* Now we are in mirror mode */
    /* Send the packet to primary edge first */
    fbe_transport_set_completion_function(packet_p, fbe_virtual_drive_usurper_register_source_drive_keys_completion, virtual_drive_p);
    fbe_base_config_get_block_edge((fbe_base_config_t *) virtual_drive_p, &block_edge_p, source_edge_index);
    status = fbe_block_transport_send_control_packet(block_edge_p, packet_p);

    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_usurper_register_drive_keys()
 ******************************************************************************/


/*!***************************************************************************
 *          fbe_virtual_drive_encrypt_paged()
 *****************************************************************************
 *
 * @brief   This is a copy of fbe_raid_group_encrypt_paged().
 *          But for VD we skip WRITE_ZERO operation.
 * 
 * @param   virtual_drive_p - The virtual drive.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  02/03/2014  Lili Chen  - Created.
 *
 *****************************************************************************/
fbe_status_t 
fbe_virtual_drive_encrypt_paged(fbe_virtual_drive_t * virtual_drive_p,
                                fbe_packet_t *packet_p)
{
    fbe_raid_group_t *raid_group_p = (fbe_raid_group_t *)virtual_drive_p;
    extern fbe_status_t fbe_raid_group_write_paged_metadata(fbe_packet_t *packet_p, fbe_packet_completion_context_t context);
    extern fbe_status_t fbe_raid_group_encryption_mark_paged_nr(fbe_packet_t * packet_p, fbe_packet_completion_context_t context);

    fbe_transport_set_completion_function(packet_p, fbe_raid_group_encrypt_paged_completion, raid_group_p);
    fbe_transport_set_completion_function(packet_p, fbe_raid_group_write_paged_metadata, raid_group_p);
    fbe_raid_group_get_NP_lock(raid_group_p, packet_p, fbe_raid_group_encryption_mark_paged_nr);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_virtual_drive_encrypt_paged()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_usurper_start_encryption()
 *****************************************************************************
 *
 * @brief   This usurper is sent to the virtual drive from the raid group.
 *          The vitual drive should start paged metadata encryption.
 * 
 * @param   virtual_drive_p - The virtual drive.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  02/03/2014  Lili Chen  - Created.
 *
 *****************************************************************************/
static fbe_status_t
fbe_virtual_drive_usurper_start_encryption(fbe_virtual_drive_t * virtual_drive_p, fbe_packet_t * packet_p)
{
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_p = NULL;

    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_p = fbe_payload_ex_get_control_operation(payload_p);

    fbe_base_object_trace((fbe_base_object_t*)virtual_drive_p, FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                          "%s start\n", __FUNCTION__);

    /* Start paged encryption now */
    fbe_virtual_drive_encrypt_paged(virtual_drive_p, packet_p);

    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/******************************************************************************
 * end fbe_virtual_drive_usurper_start_encryption()
 ******************************************************************************/

/*!***************************************************************************
 *          fbe_virtual_drive_handle_swap_request_rollback()
 *****************************************************************************
 *
 * @brief   This usurper is sent to the virtual drive from the job service to
 *          tell the virtual drive that the swap request has failed.  The
 *          virtual drive needs to perform any necessary cleanup so that the
 *          job can be rolled back successfully.
 * 
 * @param   virtual_drive_p - The virtual drive.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return  status - The status of the operation.
 * 
 * @note    Currently we simply clear any conditions that could be pending.
 *
 * @author
 *  05/21/2014  Ron Proulx  - Created.
 *
 *****************************************************************************/
static fbe_status_t fbe_virtual_drive_handle_swap_request_rollback(fbe_virtual_drive_t *virtual_drive_p, 
                                                                   fbe_packet_t *packet_p)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_virtual_drive_configuration_mode_t  configuration_mode;
    fbe_virtual_drive_flags_t               flags;
    fbe_spare_swap_command_t                swap_command;
    fbe_payload_ex_t                       *payload_p = NULL;
    fbe_payload_control_operation_t        *control_operation_p = NULL;
    fbe_virtual_drive_control_swap_request_rollback_t *rollback_p = NULL;

    /* Get the control operation
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Get the control buffer pointer from the packet payload. The routine
     * will populate the control status if there is a failure.
     */
    status = fbe_virtual_drive_usurper_get_control_buffer(virtual_drive_p, packet_p, 
                                                          sizeof(fbe_virtual_drive_control_swap_request_rollback_t),
                                                          (fbe_payload_control_buffer_t)&rollback_p);
    if (status != FBE_STATUS_OK)  { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    /* Get some debug information.
     */
    fbe_virtual_drive_get_configuration_mode(virtual_drive_p, &configuration_mode);
    fbe_virtual_drive_get_flags(virtual_drive_p, &flags);
    fbe_virtual_drive_get_copy_request_type(virtual_drive_p, &swap_command);

    /* Trace some information.
     */
    fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                          FBE_TRACE_LEVEL_INFO,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "virtual_drive: handle swap rollback mode: %d cmd: %d flags: 0x%x orig: 0x%x\n",
                          configuration_mode, swap_command, flags, rollback_p->orig_pvd_object_id);

    /* Now update the `original' pvd object id using the rollback information.
     */
    fbe_virtual_drive_set_orig_pvd_object_id(virtual_drive_p, rollback_p->orig_pvd_object_id);

    /* Clear any outstanding conditions
     */
    fbe_lifecycle_force_clear_cond(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t*)virtual_drive_p,
                                   FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_SWAP_IN_EDGE);
    fbe_lifecycle_force_clear_cond(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t*)virtual_drive_p,
                                   FBE_VIRTUAL_DRIVE_LIFECYCLE_COND_SWAP_OUT_EDGE);
    fbe_lifecycle_force_clear_cond(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t*)virtual_drive_p,
                                   FBE_RAID_GROUP_LIFECYCLE_COND_CONFIGURATION_CHANGE);
    fbe_lifecycle_force_clear_cond(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t*)virtual_drive_p,
                                   FBE_RAID_GROUP_LIFECYCLE_COND_SET_REBUILD_CHECKPOINT_TO_END_MARKER);
    fbe_lifecycle_force_clear_cond(&fbe_virtual_drive_lifecycle_const, (fbe_base_object_t*)virtual_drive_p,
                                   FBE_RAID_GROUP_LIFECYCLE_COND_DEST_DRIVE_FAILED_SET_REBUILD_CHECKPOINT_TO_END_MARKER);

    /* Reschedule immediately
     */
    fbe_lifecycle_reschedule(&fbe_base_config_lifecycle_const,
                             (fbe_base_object_t *)virtual_drive_p,
                             (fbe_lifecycle_timer_msec_t) 0);    /* Immediate reschedule */

    /* Return the execution status.
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}
/******************************************************************************
 * end fbe_virtual_drive_handle_swap_request_rollback()
 ******************************************************************************/


static fbe_status_t fbe_virtual_drive_propagate_block_geometry(fbe_virtual_drive_t *virtual_drive_p)
{
    fbe_block_edge_geometry_t ds_geometry;
    fbe_base_config_get_downstream_geometry((fbe_base_config_t*)virtual_drive_p, &ds_geometry);

    fbe_block_transport_server_set_block_size(&virtual_drive_p->spare.raid_group.base_config.block_transport_server,
                                              ds_geometry);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 *          fbe_virtual_drive_usurper_get_dieh_media_threshold_completion()
 ******************************************************************************
 *
 * @brief   This is the completion after getting the DIEH media threshold
 *          information for the downstream PDO.  We simply complete the packet
 *          back to the original caller.
 *
 * @param   virtual_drive_p - The provision drive.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  05/22/2015  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_usurper_get_dieh_media_threshold_completion(fbe_packet_t * packet_p, 
                                                                                  fbe_packet_completion_context_t context)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_virtual_drive_t                            *virtual_drive_p = NULL;
    fbe_payload_ex_t                               *payload_p = NULL;
    fbe_payload_control_operation_t                *control_operation_p = NULL;
    fbe_payload_control_status_t                    control_status;
    fbe_physical_drive_get_dieh_media_threshold_t  *get_dieh_info_p = NULL;
    
    /* Get the virtual drive object.
     */
    virtual_drive_p = (fbe_virtual_drive_t *)context;

    /* Get the control operation
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);
    fbe_payload_control_get_status(control_operation_p, &control_status);

    /* Get the control buffer pointer from the packet payload. The routine
     * will populate the control status if there is a failure.
     */
    status = fbe_virtual_drive_usurper_get_control_buffer(virtual_drive_p, packet_p, 
                                                          sizeof(fbe_physical_drive_get_dieh_media_threshold_t),
                                                          (fbe_payload_control_buffer_t)&get_dieh_info_p);

    /* Release the current control operation and get the original.
     */
    fbe_payload_ex_release_control_operation(payload_p, control_operation_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* If the request failed update the status.
     */
    if ((status != FBE_STATUS_OK)                         ||
        (control_status != FBE_PAYLOAD_CONTROL_STATUS_OK)    )
    { 
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: get dieh completion failed - status: %d control status: %d \n",
                              status, control_status);
        fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet_p, status, 0);
        return status; 
    }

    /* If enabled trace the DIEH information.
     */
    fbe_virtual_drive_trace(virtual_drive_p,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_VIRTUAL_DRIVE_DEBUG_FLAG_EMEH_TRACING,
                            "virtual_drive: get dieh completion mode: %d rel: %d weight: %d paco pos: %d \n",
                            get_dieh_info_p->dieh_mode, get_dieh_info_p->dieh_reliability,
                            get_dieh_info_p->media_weight_adjust, get_dieh_info_p->vd_paco_position);

    /* Set the control status and complete the packet.
     */
    fbe_payload_control_set_status(control_operation_p, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet_p, status, 0);
    return status;
}
/******************************************************************************
 * end fbe_virtual_drive_usurper_get_dieh_media_threshold_completion()
 ******************************************************************************/

/*!****************************************************************************
 *          fbe_virtual_drive_usurper_get_dieh_media_threshold()
 ****************************************************************************** 
 * 
 * @brief   This function simply sets the completion for the get DIEH request
 *          to a virtual drive function.  This function populates the
 *          `paco_position' to either `FBE_EDGE_INDEX_INVALID' if there is no
 *          proactive copy in progress OR it sets it the upstream index for
 *          this virtual drive if there is a proactive copy in progress.
 * 
 * @param   virtual_drive_p - The virtual drive.
 * @param   packet_p - The packet requesting this operation.
 *
 * @return  status - The status of the operation.
 *
 * @author
 *  05/22/2015  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_virtual_drive_usurper_get_dieh_media_threshold(fbe_virtual_drive_t *virtual_drive_p, fbe_packet_t *packet_p)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_payload_ex_t                               *payload_p = NULL;
    fbe_payload_control_operation_t                *control_operation_p = NULL;
    fbe_payload_control_operation_t                *new_control_operation_p = NULL;
    fbe_physical_drive_get_dieh_media_threshold_t  *get_dieh_info_p = NULL;
    fbe_block_edge_t                               *block_edge_p = NULL;
    fbe_edge_index_t                                primary_edge_index = FBE_EDGE_INDEX_INVALID;

    /* Get the control operation
     */
    payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(payload_p);

    /* Get the control buffer pointer from the packet payload. The routine
     * will populate the control status if there is a failure.
     */
    status = fbe_virtual_drive_usurper_get_control_buffer(virtual_drive_p, packet_p, 
                                                          sizeof(fbe_physical_drive_get_dieh_media_threshold_t),
                                                          (fbe_payload_control_buffer_t)&get_dieh_info_p);
    if (status != FBE_STATUS_OK)  
    { 
        fbe_transport_set_status(packet_p, status, 0);
        fbe_transport_complete_packet(packet_p);
        return status; 
    }

    /* Validate that we can get the edge.
     */
    block_edge_p = (fbe_block_edge_t*)fbe_transport_get_edge(packet_p);
    if (block_edge_p == NULL)
    {
        /* We need the edge so that we can get the position in this raid group.
         */
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s block edge is NULL \n",
                              __FUNCTION__);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    /* Check if there is a proactive copy in progress.
     */
    if (!fbe_virtual_drive_swap_is_primary_edge_healthy(virtual_drive_p)                               && 
         (fbe_raid_group_is_clustered_flag_set((fbe_raid_group_t *)virtual_drive_p, 
                                               FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS) ||
          fbe_raid_group_is_peer_clustered_flag_set((fbe_raid_group_t *)virtual_drive_p, 
                                               FBE_RAID_GROUP_CLUSTERED_FLAG_SWAP_JOB_IN_PROGRESS) ||
          fbe_virtual_drive_is_mirror_configuration_mode_set(virtual_drive_p)                         )    )
    {
        /* Trace some info and set the `paco_position'.
         */
        get_dieh_info_p->vd_paco_position = block_edge_p->base_edge.client_index;
        fbe_base_object_trace((fbe_base_object_t *)virtual_drive_p,
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "virtual_drive: get dieh set paco position to: %d \n",
                              get_dieh_info_p->vd_paco_position);
    }
    else
    {
        /* Else there is no proactive copy in progress set set the `paco_position'
         * to `FBE_EDGE_INDEX_INVALID'
         */
        get_dieh_info_p->vd_paco_position = FBE_EDGE_INDEX_INVALID;
    }

    /* Now allocate another control buffer so that we can forward the request
     * downstream.
     */
    fbe_virtual_drive_get_primary_edge_index(virtual_drive_p, &primary_edge_index);
    fbe_base_config_get_block_edge((fbe_base_config_t *)virtual_drive_p, &block_edge_p, primary_edge_index);
    new_control_operation_p = fbe_payload_ex_allocate_control_operation(payload_p);
    fbe_payload_control_build_operation(new_control_operation_p,
                                        FBE_PHYSICAL_DRIVE_CONTROL_CODE_GET_DIEH_MEDIA_THRESHOLD,
                                        get_dieh_info_p,
                                        sizeof(fbe_physical_drive_get_dieh_media_threshold_t));
    fbe_payload_ex_increment_control_operation_level(payload_p);

    fbe_transport_set_completion_function(packet_p, fbe_virtual_drive_usurper_get_dieh_media_threshold_completion, virtual_drive_p);
    fbe_block_transport_send_control_packet(block_edge_p, packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
/**********************************************************
 * end fbe_virtual_drive_usurper_get_dieh_media_threshold()
 **********************************************************/

/**********************************
 * end fbe_virtual_drive_usurper.c
 **********************************/
