/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_memory_persistence_user.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the persistent memory service.
 *
 * @version
 *   11/26/2013:  Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_queue.h"
#include "fbe/fbe_memory.h"
#include "fbe_base_service.h"
#include "fbe_memory_private.h"
#include "fbe/fbe_rdgen.h"

fbe_status_t fbe_memory_persistence_setup(void)
{
    return FBE_STATUS_OK;
}

fbe_status_t fbe_memory_persistence_cleanup(void)
{
    return FBE_STATUS_OK;
}
fbe_status_t 
fbe_memory_persistence_read_pin_completion(fbe_packet_t *packet_p, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * sep_payload_p = NULL;
    fbe_payload_persistent_memory_operation_t * persistent_memory_operation_p = NULL;
    fbe_status_t status;
    fbe_rdgen_control_start_io_t *io_spec_p = (fbe_rdgen_control_start_io_t *)context;
    fbe_persistent_memory_set_params_t *params_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);

    status = fbe_transport_get_status_code(packet_p);
    persistent_memory_operation_p =  fbe_payload_ex_get_persistent_memory_operation(sep_payload_p);

    if ((status != FBE_STATUS_OK) ||
        (io_spec_p->status.block_status != FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS) ||
        (io_spec_p->status.rdgen_status != FBE_RDGEN_OPERATION_STATUS_SUCCESS) ||
        (io_spec_p->status.status != FBE_STATUS_OK)) {
        fbe_payload_persistent_memory_set_status(persistent_memory_operation_p, FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_FAILURE);

        memory_service_trace(FBE_TRACE_LEVEL_INFO, persistent_memory_operation_p->object_id,
                             "start failed st: %d lba/bl: 0x%llx/0x%llx bst: %d rdgst: 0x%x st: 0x%x\n", 
                             status, persistent_memory_operation_p->lba, persistent_memory_operation_p->blocks,
                             io_spec_p->status.block_status, io_spec_p->status.rdgen_status, io_spec_p->status.status);
    } else {

        memory_service_trace(FBE_TRACE_LEVEL_INFO, persistent_memory_operation_p->object_id,
                         "start successful lba/bl: 0x%llx/0x%llx\n",
                         persistent_memory_operation_p->lba, persistent_memory_operation_p->blocks);
        fbe_payload_persistent_memory_set_status(persistent_memory_operation_p, FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_OK);
        persistent_memory_operation_p->opaque = io_spec_p->status.context;
        persistent_memory_operation_p->b_was_dirty = FBE_FALSE;
    }
    fbe_persistent_memory_get_params(&params_p);
    if (params_p->b_force_dirty) {
        persistent_memory_operation_p->b_was_dirty = FBE_TRUE;
    }
    if (params_p->force_status != FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_INVALID) {
        fbe_payload_persistent_memory_set_status(persistent_memory_operation_p, params_p->force_status);
    }
    return FBE_STATUS_OK;
}
fbe_status_t fbe_memory_persistence_setup_rdgen_read_and_buffer(fbe_rdgen_control_start_io_t *start_io_p,
                                                                fbe_lba_t lba,
                                                                fbe_block_count_t blocks,
                                                                fbe_object_id_t object_id,
                                                                fbe_sg_element_t *sg_p,
                                                                fbe_bool_t b_disable_capacity_check)
{
    fbe_rdgen_io_specification_t *io_spec_p = &start_io_p->specification;
    fbe_rdgen_filter_t *filter_p = &start_io_p->filter;
    fbe_zero_memory(start_io_p, sizeof(fbe_rdgen_control_start_io_t));

    filter_p->class_id = FBE_CLASS_ID_INVALID;
    filter_p->edge_index = 0;
    filter_p->filter_type = FBE_RDGEN_FILTER_TYPE_OBJECT;
    filter_p->object_id = object_id;
    filter_p->package_id = FBE_PACKAGE_ID_SEP_0;

    io_spec_p->operation = FBE_RDGEN_OPERATION_READ_AND_BUFFER;
    io_spec_p->block_spec = FBE_RDGEN_BLOCK_SPEC_CONSTANT;
    io_spec_p->io_interface = FBE_RDGEN_INTERFACE_TYPE_PACKET;
    io_spec_p->lba_spec = FBE_RDGEN_LBA_SPEC_FIXED;
    io_spec_p->max_blocks = io_spec_p->min_blocks = blocks;
    io_spec_p->min_lba = io_spec_p->start_lba = lba;
    io_spec_p->max_lba = lba + blocks;
    io_spec_p->max_passes = 1;
    io_spec_p->pattern = FBE_RDGEN_PATTERN_LBA_PASS;
    io_spec_p->threads = 1;
    io_spec_p->sg_p = sg_p;
    if (b_disable_capacity_check){
        io_spec_p->options = FBE_RDGEN_OPTIONS_DISABLE_CAPACITY_CHECK;
    }
    return FBE_STATUS_OK;
}
fbe_status_t 
fbe_memory_persistence_complete_master(fbe_packet_t * packet_p, 
                                       fbe_packet_completion_context_t context)
{
    fbe_bool_t is_empty;
    fbe_packet_t *master_packet_p = fbe_transport_get_master_packet(packet_p);

    if (master_packet_p != NULL) {
        fbe_transport_remove_subpacket_is_queue_empty(packet_p, &is_empty);
                   
        if (is_empty)
        {
            fbe_transport_destroy_subpackets(master_packet_p);
        }
        fbe_transport_complete_packet(master_packet_p);
    } else {
        memory_service_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                             "%s no master packet for subpacket %p\n", __FUNCTION__, packet_p);
    }
    /* Regardless, free the memory for this packet now.
     */
    fbe_memory_native_release(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
fbe_status_t fbe_memory_persistence_read_and_pin(fbe_packet_t * packet_p)
{
    fbe_payload_ex_t * sep_payload_p = NULL;
    fbe_payload_persistent_memory_operation_t * persistent_memory_operation_p = NULL;
    fbe_rdgen_control_start_io_t *io_spec_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_packet_t *new_packet_p = NULL;
    fbe_payload_ex_t * sub_payload_p = NULL;

    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    persistent_memory_operation_p =  fbe_payload_ex_get_persistent_memory_operation(sep_payload_p);
    
    memory_service_trace(FBE_TRACE_LEVEL_INFO, persistent_memory_operation_p->object_id,
                         "pin start lba/bl: 0x%llx/0x%llx\n",
                         persistent_memory_operation_p->lba, persistent_memory_operation_p->blocks);

    /* This memory needs to be around after the unlock in cases of error. 
     * The reason is that rdgen will use this for its packet, and that packet needs to be around 
     * at the time of the flush. 
     */
    persistent_memory_operation_p->chunk_p = fbe_memory_native_allocate(sizeof(fbe_rdgen_control_start_io_t) +
                                                                        FBE_MEMORY_CHUNK_SIZE);
    if (persistent_memory_operation_p->chunk_p == NULL) {
        memory_service_trace(FBE_TRACE_LEVEL_ERROR, persistent_memory_operation_p->object_id,
                             "unable to allocate pin memory\n");
        fbe_payload_persistent_memory_set_status(persistent_memory_operation_p, FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_FAILURE);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_MORE_PROCESSING_REQUIRED;
    }

    new_packet_p = (fbe_packet_t*)(persistent_memory_operation_p->chunk_p);
    io_spec_p = (fbe_rdgen_control_start_io_t *)(persistent_memory_operation_p->chunk_p + FBE_MEMORY_CHUNK_SIZE);

    fbe_transport_set_completion_function(packet_p, fbe_memory_persistence_read_pin_completion, io_spec_p);
    fbe_payload_persistent_memory_set_status(persistent_memory_operation_p, FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_OK);

    fbe_transport_initialize_packet(new_packet_p);
    sub_payload_p = fbe_transport_get_payload_ex(new_packet_p);
    fbe_transport_add_subpacket(packet_p, new_packet_p);
    control_operation_p = fbe_payload_ex_allocate_control_operation(sub_payload_p);

    fbe_memory_persistence_setup_rdgen_read_and_buffer(io_spec_p,
                                                       persistent_memory_operation_p->lba,
                                                       persistent_memory_operation_p->blocks,
                                                       (fbe_object_id_t)persistent_memory_operation_p->object_id,
                                                       persistent_memory_operation_p->sg_p,
                                                       persistent_memory_operation_p->b_beyond_capacity);

    fbe_payload_control_build_operation(control_operation_p, FBE_RDGEN_CONTROL_CODE_START_IO,
                                        io_spec_p, sizeof(fbe_rdgen_control_start_io_t));
    fbe_payload_ex_increment_control_operation_level(sub_payload_p);
    fbe_transport_set_completion_function(new_packet_p, fbe_memory_persistence_complete_master, persistent_memory_operation_p);

    fbe_transport_set_address(new_packet_p, FBE_PACKAGE_ID_NEIT, FBE_SERVICE_ID_RDGEN, FBE_CLASS_ID_INVALID, FBE_OBJECT_ID_INVALID);
    fbe_service_manager_send_control_packet(new_packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
fbe_status_t 
fbe_memory_persistence_unpin_completion(fbe_packet_t *packet_p, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * sep_payload_p = NULL;
    fbe_payload_persistent_memory_operation_t * persistent_memory_operation_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_status_t status;
    
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    control_operation_p = fbe_payload_ex_get_control_operation(sep_payload_p);
    fbe_payload_ex_release_control_operation(sep_payload_p, control_operation_p);
    
    status = fbe_transport_get_status_code(packet_p);
    persistent_memory_operation_p =  fbe_payload_ex_get_persistent_memory_operation(sep_payload_p);

    memory_service_trace(FBE_TRACE_LEVEL_INFO, persistent_memory_operation_p->object_id,
                         "unpin unlock complete lba/bl: 0x%llx/0x%llx\n",
                         persistent_memory_operation_p->lba, persistent_memory_operation_p->blocks);
    if ((control_operation_p->status != FBE_PAYLOAD_CONTROL_STATUS_OK) || (status != FBE_STATUS_OK)) {
        memory_service_trace(FBE_TRACE_LEVEL_INFO, persistent_memory_operation_p->object_id,
                             "unlock failed st: %d/%d lba/bl: 0x%llx/0x%llx\n", control_operation_p->status, status,
                             persistent_memory_operation_p->lba, persistent_memory_operation_p->blocks);
        fbe_payload_persistent_memory_set_status(persistent_memory_operation_p, FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_FAILURE);
    } else {
        memory_service_trace(FBE_TRACE_LEVEL_INFO, persistent_memory_operation_p->object_id,
                             "unlock success lba/bl: 0x%llx/0x%llx\n",
                             persistent_memory_operation_p->lba, persistent_memory_operation_p->blocks);
        fbe_payload_persistent_memory_set_status(persistent_memory_operation_p, FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_OK);
    }
    return FBE_STATUS_OK;
}
fbe_status_t fbe_memory_persistence_setup_rdgen_unlock(fbe_rdgen_control_start_io_t *unlock_p,
                                                       fbe_object_id_t object_id,
                                                       void *context,
                                                       fbe_payload_persistent_memory_unpin_mode_t mode)
{
    fbe_rdgen_filter_t *filter_p = &unlock_p->filter;
    fbe_zero_memory(unlock_p, sizeof(fbe_rdgen_control_start_io_t));

    filter_p->class_id = FBE_CLASS_ID_INVALID;
    filter_p->edge_index = 0;
    filter_p->filter_type = FBE_RDGEN_FILTER_TYPE_OBJECT;
    filter_p->object_id = object_id;
    filter_p->package_id = FBE_PACKAGE_ID_SEP_0;

    /* The status was given back to us on the pin.
     */
    unlock_p->status.context = context;

    /* If the mode is anything other than written then a write is needed.
     */
    if (mode != FBE_PAYLOAD_PERSISTENT_MEMORY_UNPIN_MODE_DATA_WRITTEN) {
        unlock_p->specification.operation = FBE_RDGEN_OPERATION_READ_AND_BUFFER_UNLOCK_FLUSH;
    } else {
        unlock_p->specification.operation = FBE_RDGEN_OPERATION_READ_AND_BUFFER_UNLOCK;
    }
    return FBE_STATUS_OK;
}

fbe_status_t fbe_memory_persistence_unpin(fbe_packet_t * packet_p)
{
    fbe_payload_ex_t * sep_payload_p = NULL;
    fbe_payload_persistent_memory_operation_t * persistent_memory_operation_p = NULL;
    fbe_rdgen_control_start_io_t *unlock_p = NULL;
    fbe_payload_control_operation_t *control_operation_p = NULL;
    fbe_persistent_memory_set_params_t *params_p = NULL;
    
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    persistent_memory_operation_p =  fbe_payload_ex_get_persistent_memory_operation(sep_payload_p);

    /* This memory is just for our use.
     */
    unlock_p = (fbe_rdgen_control_start_io_t*) (persistent_memory_operation_p->chunk_p);

    control_operation_p = fbe_payload_ex_allocate_control_operation(sep_payload_p);

    fbe_persistent_memory_get_params(&params_p);
    
    if (params_p->b_always_unlock) {
        persistent_memory_operation_p->mode = FBE_PAYLOAD_PERSISTENT_MEMORY_UNPIN_MODE_DATA_WRITTEN;
    }
    fbe_memory_persistence_setup_rdgen_unlock(unlock_p, 
                                              persistent_memory_operation_p->object_id, 
                                              persistent_memory_operation_p->opaque,
                                              persistent_memory_operation_p->mode);

    fbe_payload_control_build_operation(control_operation_p, FBE_RDGEN_CONTROL_CODE_UNLOCK,
                                        unlock_p, sizeof(fbe_rdgen_control_start_io_t));
    fbe_payload_ex_increment_control_operation_level(sep_payload_p);

    fbe_transport_set_completion_function(packet_p, fbe_memory_persistence_unpin_completion, persistent_memory_operation_p);
    fbe_transport_set_address(packet_p, FBE_PACKAGE_ID_NEIT, FBE_SERVICE_ID_RDGEN, FBE_CLASS_ID_INVALID, FBE_OBJECT_ID_INVALID);
    fbe_service_manager_send_control_packet(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}
fbe_status_t fbe_memory_persistence_check_vault_busy(fbe_packet_t * packet_p)
{
    fbe_payload_ex_t * sep_payload_p = NULL;
    fbe_payload_persistent_memory_operation_t * persistent_memory_operation_p = NULL;
    fbe_persistent_memory_set_params_t *params_p = NULL;
    
    sep_payload_p = fbe_transport_get_payload_ex(packet_p);
    persistent_memory_operation_p =  fbe_payload_ex_get_persistent_memory_operation(sep_payload_p);

    fbe_persistent_memory_get_params(&params_p);

    /* If we need to mock that the vault is busy, return a busy status.  Otherwise return good status.
     */
    if (params_p->b_vault_busy) {
        persistent_memory_operation_p->persistent_memory_status = FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_BUSY;
    } else {
        persistent_memory_operation_p->persistent_memory_status = FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_OK;
    }
    /* Memory needs to be provided for this operation.
     */
    if (persistent_memory_operation_p->chunk_p == NULL) {
        persistent_memory_operation_p->persistent_memory_status = FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_INSUFFICIENT_RESOURCES;
    }
    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

/*************************
 * end file fbe_memory_persistence_user.c
 *************************/
