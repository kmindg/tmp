/***************************************************************************
 * Copyright (C) EMC Corporation 2013
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_memory_persistence.c
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

static fbe_spinlock_t fbe_persist_memory_lock;
static fbe_persistent_memory_set_params_t fbe_persist_memory_params;

void fbe_persistent_memory_get_params(fbe_persistent_memory_set_params_t **set_params_p)
{
    *set_params_p = &fbe_persist_memory_params;
}

fbe_status_t 
fbe_memory_persistence_init(void)
{
    memory_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY, "%s: entry\n", __FUNCTION__);

    fbe_zero_memory(&fbe_persist_memory_params, sizeof(fbe_persistent_memory_set_params_t));
    fbe_spinlock_init(&fbe_persist_memory_lock);
    return fbe_memory_persistence_setup();
}

fbe_status_t 
fbe_memory_persistence_destroy(void)
{
    memory_service_trace(FBE_TRACE_LEVEL_DEBUG_HIGH, 
                         FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                         "%s: entry\n", __FUNCTION__);

    fbe_spinlock_destroy(&fbe_persist_memory_lock);
    return fbe_memory_persistence_cleanup();
}

fbe_status_t 
fbe_persistent_memory_operation_entry(fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_ex_t * sep_payload = NULL;
    fbe_payload_persistent_memory_operation_t * persistent_memory_operation = NULL;
    fbe_payload_persistent_memory_operation_opcode_t opcode;
    fbe_cpu_id_t cpu_id;
    
    /* validating the cpu_id */
    fbe_transport_get_cpu_id(packet, &cpu_id);
    if(cpu_id == FBE_CPU_ID_INVALID){
        memory_service_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                             "%s Invalid cpu_id in packet!\n", __FUNCTION__);
    }

    sep_payload = fbe_transport_get_payload_ex(packet);
    persistent_memory_operation =  fbe_payload_ex_get_persistent_memory_operation(sep_payload);

    fbe_payload_persistent_memory_get_opcode(persistent_memory_operation, &opcode);
    switch(opcode){
        case FBE_PAYLOAD_PERSISTENT_MEMORY_OPERATION_OPCODE_READ_AND_PIN:
            status = fbe_memory_persistence_read_and_pin(packet);  
            break;
        case FBE_PAYLOAD_PERSISTENT_MEMORY_OPERATION_OPCODE_PERSIST_UNPIN:
            status = fbe_memory_persistence_unpin(packet);  
            break;
        case FBE_PAYLOAD_PERSISTENT_MEMORY_OPERATION_OPCODE_CHECK_VAULT_BUSY:
            status = fbe_memory_persistence_check_vault_busy(packet);  
            break;
        default:
            memory_service_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s Invalid opcode %d\n", __FUNCTION__, opcode);

            fbe_payload_persistent_memory_set_status(persistent_memory_operation, FBE_PAYLOAD_PERSISTENT_MEMORY_STATUS_FAILURE);    
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}
static fbe_status_t fbe_persistent_memory_set_params(fbe_packet_t * packet);

fbe_status_t fbe_persistent_memory_control_entry(fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_control_operation_opcode_t control_code;

    control_code = fbe_transport_get_control_code(packet);

    switch(control_code) {

        case FBE_MEMORY_SERVICE_CONTROL_CODE_PERSIST_SET_PARAMS:
            status = fbe_persistent_memory_set_params(packet);
            break;
        default:
            memory_service_trace(FBE_TRACE_LEVEL_ERROR,
                                 FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                 "%s: Unknown control code 0x%X\n", __FUNCTION__, control_code);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_transport_complete_packet(packet);
            break;
    }
    return status;
}
static fbe_status_t fbe_persistent_memory_set_params(fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;
    fbe_persistent_memory_set_params_t *set_params_p = NULL;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        memory_service_trace(
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get packet payload\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        memory_service_trace(
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get control operation\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_payload_control_get_buffer(control_operation, (fbe_u32_t*)&set_params_p);
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    buffer_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length); 
    if (buffer_length != sizeof(fbe_persistent_memory_set_params_t)) {
        memory_service_trace(
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get control operation buffer size\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* Save the params.
     */
    fbe_persist_memory_params = *set_params_p;

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
/*************************
 * end file fbe_memory_persistence.c
 *************************/


