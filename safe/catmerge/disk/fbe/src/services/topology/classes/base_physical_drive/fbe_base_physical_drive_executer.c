#include "fbe/fbe_types.h"
#include "base_physical_drive_private.h"
#include "fbe_transport_memory.h"


/* Forward declaration */
static fbe_status_t fbe_base_physical_drive_get_protocol_address_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t base_physical_drive_discovery_transport_entry(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t base_physical_drive_discovery_transport_get_port_object_id(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet);
static fbe_status_t fbe_base_physical_drive_get_drive_location_info_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

static fbe_status_t fbe_base_physical_drive_send_power_cycle_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_physical_drive_get_permission_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_base_physical_drive_reset_slot_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);

/*
static fbe_status_t base_physical_drive_edge_state_change_event_entry(fbe_base_physical_drive_t * base_physical_drive, fbe_event_context_t event_context);
static fbe_status_t base_physical_drive_check_attributes(fbe_base_physical_drive_t * base_physical_drive);
*/

fbe_status_t 
fbe_base_physical_drive_event_entry(fbe_object_handle_t object_handle, fbe_event_type_t event_type, fbe_event_context_t event_context)
{
    fbe_base_physical_drive_t * base_physical_drive = NULL;
    fbe_status_t status;

    base_physical_drive = (fbe_base_physical_drive_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace((fbe_base_object_t*)base_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    /* Handle the event we have received. */
    switch (event_type)
    {
        case FBE_EVENT_TYPE_EDGE_STATE_CHANGE:
        /* There is no edges in base_physical_drive */
            /* status = base_physical_drive_edge_state_change_event_entry(base_physical_drive, event_context); */
            /* Forwarded it to base discovering object. */
            status = fbe_base_discovering_event_entry(object_handle, event_type, event_context);
            break;
        default:
        /* Forwarded it to base discovering object. */
            status = fbe_base_discovering_event_entry(object_handle, event_type, event_context);
            break;
    }

    return status;
}

#if 0
static fbe_status_t 
base_physical_drive_edge_state_change_event_entry(fbe_base_physical_drive_t * base_physical_drive, fbe_event_context_t event_context)
{
    fbe_path_state_t path_state;
    fbe_status_t status;
    fbe_transport_id_t transport_id;

    /* Get the transport type from the edge pointer. */
    fbe_base_transport_get_transport_id((fbe_base_edge_t *) event_context, &transport_id);

    /* Process discovery edge state change only */
    if (transport_id != FBE_TRANSPORT_ID_DISCOVERY) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get discovery edge state */
    status = fbe_discovery_transport_get_path_state(&((fbe_base_discovered_t *)base_physical_drive)->discovery_edge, &path_state);

    switch(path_state){
    case FBE_PATH_STATE_ENABLED:
            status = base_physical_drive_check_attributes(base_physical_drive);
            break;
        case FBE_PATH_STATE_GONE:
            status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const,
                                            (fbe_base_object_t*)base_physical_drive,
                                            FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
            break;
        case FBE_PATH_STATE_INVALID:
        case FBE_PATH_STATE_DISABLED:
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;

    }
    return status;
}

static fbe_status_t 
base_physical_drive_check_attributes(fbe_base_physical_drive_t * base_physical_drive)
{
    fbe_status_t status;
    fbe_path_attr_t path_attr;

    status = fbe_discovery_transport_get_path_attributes(
        &((fbe_base_discovered_t *)base_physical_drive)->discovery_edge, &path_attr);

    if(path_attr & FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT) {
        status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const,
                                        (fbe_base_object_t*)base_physical_drive,
                                        FBE_BASE_PHYSICAL_DRIVE_LIFECYCLE_COND_DISCOVERY_EDGE_NOT_PRESENT);
    }

    return FBE_STATUS_OK;
}
#endif

fbe_status_t 
fbe_base_physical_drive_io_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_base_physical_drive_t * base_physical_drive = NULL;
    fbe_transport_id_t transport_id;
    fbe_status_t status;

    base_physical_drive = (fbe_base_physical_drive_t *)fbe_base_handle_to_pointer(object_handle);
    fbe_base_object_trace(  (fbe_base_object_t*)base_physical_drive,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);

    /* Fisrt we need to figure out to what transport this packet belong */
    fbe_transport_get_transport_id(packet, &transport_id);
    switch(transport_id) {
        case FBE_TRANSPORT_ID_DISCOVERY:
            /* The server part of fbe_discovery transport is a member of discovering class.
                Even more than that, we do not expect to receive discovery protocol packets
                for "non discovering" objects 
            */
            status = fbe_base_discovering_discovery_bouncer_entry((fbe_base_discovering_t *) base_physical_drive,
                                                                        (fbe_transport_entry_t)base_physical_drive_discovery_transport_entry,
                                                                        packet);
            break;
    }

    return FBE_STATUS_GENERIC_FAILURE;
}


fbe_status_t 
fbe_base_physical_drive_get_protocol_address(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_u8_t * buffer = NULL;
    fbe_object_id_t my_object_id;
    fbe_sg_element_t  * sg_list = NULL; 
    fbe_path_state_t path_state = FBE_PATH_STATE_INVALID;

    fbe_base_object_trace((fbe_base_object_t*)base_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    // Check path state. If the enclosure is failed or gone, we should fail/destroy drive object too
    status = fbe_base_discovered_get_path_state((fbe_base_discovered_t *) base_physical_drive, &path_state);
    if(path_state == FBE_PATH_STATE_GONE)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s path state gone", __FUNCTION__);

        // set lifecycle condition to destroy - discovery edge gone
        status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                        (fbe_base_object_t*)base_physical_drive, 
                                        FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
                
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                                       FBE_TRACE_LEVEL_ERROR,
                                                       "%s can't set DESTROY condition, status: 0x%X\n",
                                                       __FUNCTION__, status);
        }        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }else if(status != FBE_STATUS_OK || path_state == FBE_PATH_STATE_BROKEN)
    {
        fbe_base_object_trace((fbe_base_object_t*)base_physical_drive, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s path state broken", __FUNCTION__);
        
        /* We need to set the BO:FAIL lifecycle condition */
        status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                        (fbe_base_object_t*)base_physical_drive, 
                                        FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);
        if (status != FBE_STATUS_OK) {
            fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                                       FBE_TRACE_LEVEL_ERROR,
                                                       "%s can't set FAIL condition, status: 0x%X\n",
                                                       __FUNCTION__, status);
        }
        
        fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)base_physical_drive, 
                                             FBE_BASE_PHYSICAL_DRIVE_DEATH_REASON_ENCLOSURE_OBJECT_FAILED);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    buffer = fbe_transport_allocate_buffer();
    if(buffer == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_transport_allocate_buffer fail", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_object_get_object_id((fbe_base_object_t *)base_physical_drive, &my_object_id);

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_allocate_discovery_operation(payload);

    fbe_payload_discovery_build_get_protocol_address(payload_discovery_operation, my_object_id);

    /* Provide memory for the responce */
    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = sizeof(fbe_payload_discovery_get_protocol_address_data_t);
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);


    status = fbe_transport_set_completion_function(packet, fbe_base_physical_drive_get_protocol_address_completion, base_physical_drive);

    status = fbe_base_discovered_send_functional_packet((fbe_base_discovered_t *) base_physical_drive, packet);

    return status;
}

static fbe_status_t 
fbe_base_physical_drive_get_protocol_address_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_base_physical_drive_t * base_physical_drive = NULL;
    fbe_sg_element_t  * sg_list = NULL; 
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_payload_discovery_get_protocol_address_data_t * payload_discovery_get_protocol_address_data = NULL;
    fbe_payload_discovery_status_t discovery_status;
    fbe_payload_control_operation_t * control_operation = NULL;

    base_physical_drive = (fbe_base_physical_drive_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
    fbe_payload_discovery_get_status(payload_discovery_operation, &discovery_status);
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    payload_discovery_get_protocol_address_data = (fbe_payload_discovery_get_protocol_address_data_t *)sg_list[0].address;

    base_physical_drive->address.sas_address = payload_discovery_get_protocol_address_data->address.sas_address;

    base_physical_drive->generation_code = payload_discovery_get_protocol_address_data->generation_code;
    base_physical_drive->element_type = payload_discovery_get_protocol_address_data->element_type;

    fbe_base_physical_drive_customizable_trace(base_physical_drive,
                                FBE_TRACE_LEVEL_INFO,
                                "base pdo get protocol. Addr.:0x%llx GenCode:0x%llX ElmT:%d\n",
                                (unsigned long long)base_physical_drive->address.sas_address, 
                                (unsigned long long)base_physical_drive->generation_code, base_physical_drive->element_type);
                                
    fbe_transport_release_buffer(sg_list);
    fbe_payload_ex_release_discovery_operation(payload, payload_discovery_operation);

    if((discovery_status != FBE_PAYLOAD_DISCOVERY_STATUS_OK) ||
       (base_physical_drive->element_type == FBE_PAYLOAD_DISCOVERY_ELEMENT_TYPE_INVALID)) { 
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    } else {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }

    return FBE_STATUS_OK;
}

static fbe_status_t 
base_physical_drive_discovery_transport_entry(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_base_physical_drive_t * base_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_payload_discovery_opcode_t discovery_opcode;
    fbe_status_t status;

    base_physical_drive = (fbe_base_physical_drive_t *)base_object;

    fbe_base_object_trace((fbe_base_object_t*)base_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
    fbe_payload_discovery_get_opcode(payload_discovery_operation, &discovery_opcode);
    
    switch(discovery_opcode){
        case FBE_PAYLOAD_DISCOVERY_OPCODE_GET_PORT_OBJECT_ID:
                status = base_physical_drive_discovery_transport_get_port_object_id(base_physical_drive, packet);
            break;
        default:
            fbe_base_object_trace((fbe_base_object_t *) base_physical_drive, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s Uknown discovery_opcode %X\n", __FUNCTION__, discovery_opcode);

            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}

static fbe_status_t 
base_physical_drive_discovery_transport_get_port_object_id(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * discovery_operation = NULL;
    fbe_sg_element_t  * sg_list = NULL;
    fbe_payload_discovery_get_port_object_id_data_t * payload_discovery_get_port_object_id_data = NULL;
    fbe_object_id_t my_object_id;

    fbe_base_object_trace((fbe_base_object_t*)base_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    payload = fbe_transport_get_payload_ex(packet);
    discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    payload_discovery_get_port_object_id_data = (fbe_payload_discovery_get_port_object_id_data_t *)sg_list[0].address;

    /* Physical drive object is a block_transport_server, so we need to "report" our object_id */
    fbe_base_object_get_object_id((fbe_base_object_t *)base_physical_drive, &my_object_id);

    payload_discovery_get_port_object_id_data->port_object_id = my_object_id;

    fbe_payload_discovery_set_status(discovery_operation, FBE_PAYLOAD_DISCOVERY_STATUS_OK);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_get_drive_location_info(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_u8_t * buffer = NULL;
    fbe_object_id_t my_object_id;
    fbe_sg_element_t  * sg_list = NULL; 

    fbe_base_object_trace((fbe_base_object_t*)base_physical_drive,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    buffer = fbe_transport_allocate_buffer();
    if(buffer == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s fbe_transport_allocate_buffer fail", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_object_get_object_id((fbe_base_object_t *)base_physical_drive, &my_object_id);

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_allocate_discovery_operation(payload);

    fbe_payload_discovery_build_get_drive_location_info(payload_discovery_operation, my_object_id);

    /* Provide memory for the responce */
    sg_list = (fbe_sg_element_t  *)buffer;
    sg_list[0].count = sizeof(fbe_payload_discovery_get_drive_location_info_t);
    sg_list[0].address = buffer + 2 * sizeof(fbe_sg_element_t);

    sg_list[1].count = 0;
    sg_list[1].address = NULL;

    fbe_payload_ex_set_sg_list(payload, sg_list, 1);

    status = fbe_transport_set_completion_function(packet, fbe_base_physical_drive_get_drive_location_info_completion, base_physical_drive);

    status = fbe_base_discovered_send_functional_packet((fbe_base_discovered_t *) base_physical_drive, packet);

    return status;
}

static fbe_status_t 
fbe_base_physical_drive_get_drive_location_info_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_base_physical_drive_t * base_physical_drive = NULL;
    fbe_sg_element_t  * sg_list = NULL; 
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_payload_discovery_get_drive_location_info_t * get_disk_position_p = NULL;
    fbe_payload_discovery_status_t discovery_status;
    fbe_payload_control_operation_t * control_operation = NULL;

    base_physical_drive = (fbe_base_physical_drive_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
    fbe_payload_discovery_get_status(payload_discovery_operation, &discovery_status);
    fbe_payload_ex_get_sg_list(payload, &sg_list, NULL);

    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    get_disk_position_p = (fbe_payload_discovery_get_drive_location_info_t *)sg_list[0].address;
    
    fbe_base_physical_drive_set_sp_id(base_physical_drive, get_disk_position_p->sp_id);
    fbe_base_physical_drive_set_port_number(base_physical_drive, get_disk_position_p->port_number);
    fbe_base_physical_drive_set_enclosure_number(base_physical_drive, get_disk_position_p->enclosure_number);
    fbe_base_physical_drive_set_slot_number(base_physical_drive, get_disk_position_p->slot_number);
    fbe_base_physical_drive_set_bank_width(base_physical_drive, get_disk_position_p->bank_width);

    fbe_transport_release_buffer(sg_list);
    fbe_payload_ex_release_discovery_operation(payload, payload_discovery_operation);

    if (discovery_status != FBE_PAYLOAD_DISCOVERY_STATUS_OK) { 
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    } else {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }

    return FBE_STATUS_OK;
}


fbe_status_t 
fbe_base_physical_drive_send_power_cycle(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet, fbe_bool_t completed)
{
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_object_id_t my_object_id;
    fbe_u32_t duration;

    fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                               FBE_TRACE_LEVEL_INFO,
                                               "%s entry\n", __FUNCTION__);


    fbe_base_object_get_object_id((fbe_base_object_t *)base_physical_drive, &my_object_id);

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_allocate_discovery_operation(payload);
    fbe_base_physical_drive_get_powercycle_duration(base_physical_drive, &duration);

    fbe_payload_discovery_build_power_cycle(payload_discovery_operation, my_object_id, completed, duration);

    status = fbe_transport_set_completion_function(packet, fbe_base_physical_drive_send_power_cycle_completion, base_physical_drive);

    status = fbe_base_discovered_send_functional_packet((fbe_base_discovered_t *) base_physical_drive, packet);

    return status;
}

static fbe_status_t 
fbe_base_physical_drive_send_power_cycle_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_base_physical_drive_t * base_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_payload_discovery_status_t discovery_status;
    fbe_payload_control_operation_t * control_operation = NULL;

    base_physical_drive = (fbe_base_physical_drive_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
    fbe_payload_discovery_get_status(payload_discovery_operation, &discovery_status);

    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    if(discovery_status != FBE_PAYLOAD_DISCOVERY_STATUS_OK) { 
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    } else {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }

    fbe_payload_ex_release_discovery_operation(payload, payload_discovery_operation);

    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_get_permission(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet, 
                                      fbe_bool_t get)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_status_t status;
    fbe_object_id_t my_object_id;

    payload = fbe_transport_get_payload_ex(packet);

    fbe_base_object_get_object_id((fbe_base_object_t *)base_physical_drive, &my_object_id);
    
    payload_discovery_operation = fbe_payload_ex_allocate_discovery_operation(payload);

    if (get) {
        fbe_payload_discovery_build_common_command(payload_discovery_operation, 
                                                   FBE_PAYLOAD_DISCOVERY_OPCODE_GET_SPINUP_CREDIT, my_object_id);
    } else {
        fbe_payload_discovery_build_common_command(payload_discovery_operation, 
                                                   FBE_PAYLOAD_DISCOVERY_OPCODE_RELEASE_SPINUP_CREDIT, my_object_id);
    }

    fbe_transport_set_completion_function(packet, fbe_base_physical_drive_get_permission_completion, base_physical_drive);
  
    /* We are sending discovery packet via discovery edge */
    status = fbe_base_discovered_send_functional_packet((fbe_base_discovered_t *) base_physical_drive, packet);

    return status;
}

/*
 * This is the completion function for the get_permission packet.
 */
static fbe_status_t 
fbe_base_physical_drive_get_permission_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_base_physical_drive_t * base_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_payload_discovery_status_t discovery_status;
    fbe_status_t status;
    fbe_payload_control_operation_t * control_operation = NULL;

    base_physical_drive = (fbe_base_physical_drive_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
    fbe_payload_discovery_get_status(payload_discovery_operation, &discovery_status);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else
    {
        if(discovery_status != FBE_PAYLOAD_DISCOVERY_STATUS_OK) { 
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        } else {
            fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
        }
    }

    fbe_payload_ex_release_discovery_operation(payload, payload_discovery_operation);
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_base_physical_drive_set_retry_msecs(fbe_payload_ex_t * payload, fbe_scsi_error_code_t error_code)
{
    fbe_time_t msecs_to_retry;

    fbe_base_physical_drive_get_retry_msecs(error_code, &msecs_to_retry);
    fbe_payload_ex_set_retry_wait_msecs(payload, msecs_to_retry);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_physical_drive_get_retry_msecs(fbe_scsi_error_code_t error_code,
                                                     fbe_time_t *msecs_to_retry)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch (error_code)
    {
        case FBE_SCSI_SLOT_BUSY:
        case FBE_SCSI_DEVICE_BUSY:
            *msecs_to_retry = FBE_BASE_PHYSICAL_DRIVE_RETRY_MSECS_DEV_BUSY;
            break;

        case FBE_SCSI_SELECTIONTIMEOUT:
        case FBE_SCSI_DEVUNITIALIZED:
        case FBE_SCSI_AUTO_SENSE_FAILED:
        case FBE_SCSI_BADBUSPHASE:
        case FBE_SCSI_BADSTATUS:
        case FBE_SCSI_XFERCOUNTNOTZERO:
        case FBE_SCSI_INVALIDSCB:
        case FBE_SCSI_TOOMUCHDATA:
        case FBE_SCSI_CHANNEL_INACCESSIBLE:
        case FBE_SCSI_IO_LINKDOWN_ABORT:
        case FBE_SCSI_FCP_INVALIDRSP:
            *msecs_to_retry = FBE_BASE_PHYSICAL_DRIVE_RETRY_MSECS_SUSPENDED;
            break;

        case FBE_SCSI_DEVICE_RESERVED:
            *msecs_to_retry = FBE_BASE_PHYSICAL_DRIVE_RETRY_MSECS_DEV_RESERVED;
            break;

        case FBE_SCSI_CC_NOT_READY:
        case FBE_SCSI_CC_FORMAT_IN_PROGRESS:
        case FBE_SCSI_DEVICE_NOT_READY:
            *msecs_to_retry = FBE_BASE_PHYSICAL_DRIVE_RETRY_MSECS_NOT_READY;
            break;

        case FBE_SCSI_CC_BECOMING_READY:
            *msecs_to_retry = FBE_BASE_PHYSICAL_DRIVE_RETRY_MSECS_BECOMING_READY;
            break;

        case FBE_SCSI_CC_DEV_RESET:
        case FBE_SCSI_BUSRESET:
            *msecs_to_retry = FBE_BASE_PHYSICAL_DRIVE_RETRY_MSECS_BUS_RESET;
            break;

        case FBE_SCSI_IO_TIMEOUT_ABORT:
            *msecs_to_retry = FBE_BASE_PHYSICAL_DRIVE_RETRY_MSECS_CMD_TIMEOUT;
            break;

        case FBE_SCSI_INCIDENTAL_ABORT:
            *msecs_to_retry = FBE_BASE_PHYSICAL_DRIVE_RETRY_MSECS_INCIDENTAL_ABORT;
            break;


        case FBE_SCSI_UNKNOWNRESPONSE:
        case FBE_SCSI_CC_ILLEGAL_REQUEST:
        case FBE_SCSI_INVALIDREQUEST:
        case FBE_SCSI_CC_ABORTED_CMD_PARITY_ERROR:
        case FBE_SCSI_CC_ABORTED_CMD:
        case FBE_SCSI_CC_GENERAL_FIRMWARE_ERROR:
        case FBE_SCSI_CC_HARDWARE_ERROR_NO_SPARE:
        case FBE_SCSI_CC_HARDWARE_ERROR_PARITY:
        case FBE_SCSI_CC_HARDWARE_ERROR_SELF_TEST:
        case FBE_SCSI_CC_HARDWARE_ERROR:
        case FBE_SCSI_CC_MEDIA_ERR_DONT_REMAP:
        case FBE_SCSI_CC_MEDIA_ERR_CANT_REMAP:
        case FBE_SCSI_CC_MODE_SELECT_OCCURRED:
        case FBE_SCSI_CC_UNIT_ATTENTION:
        case FBE_SCSI_CC_UNEXPECTED_SENSE_KEY:
        case FBE_SCSI_CHECK_COND_OTHER_SLOT:
        case FBE_SCSI_CC_SEEK_POSITIONING_ERROR:
        case FBE_SCSI_CC_SEL_ID_ERROR:
        case FBE_SCSI_CC_HARDWARE_ERROR_WRITE_FAULT:
        case FBE_SCSI_CC_INTERNAL_TARGET_FAILURE:
        case FBE_SCSI_CC_NOT_SPINNING:        
        case FBE_SCSI_CC_DATA_OFFSET_ERROR:
        case FBE_SCSI_CC_MEDIA_ERR_WRITE_ERROR:
        case FBE_SCSI_CC_DEFERRED_ERROR:
        case FBE_SCSI_CC_SENSE_DATA_MISSING:
        case FBE_SCSI_CC_NOERR:
        default:
            *msecs_to_retry = 0;
            break;
    };

    return status;
}
fbe_status_t
fbe_base_physical_drive_reset_slot(fbe_base_physical_drive_t * base_physical_drive, fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_status_t status;
    fbe_object_id_t my_object_id;

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_allocate_discovery_operation(payload);
    fbe_base_object_get_object_id((fbe_base_object_t *)base_physical_drive, &my_object_id);

    fbe_payload_discovery_build_common_command(payload_discovery_operation, 
                                               FBE_PAYLOAD_DISCOVERY_OPCODE_RESET_SLOT, my_object_id);

    fbe_transport_set_completion_function(packet, fbe_base_physical_drive_reset_slot_completion, base_physical_drive);
  
    /* We are sending discovery packet via discovery edge */
    status = fbe_base_discovered_send_functional_packet((fbe_base_discovered_t *) base_physical_drive, packet);

    return status;
}

static fbe_status_t 
fbe_base_physical_drive_reset_slot_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_base_physical_drive_t * base_physical_drive = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_discovery_operation_t * payload_discovery_operation = NULL;
    fbe_payload_discovery_status_t discovery_status;
    fbe_status_t status;
    fbe_payload_control_operation_t * control_operation = NULL;

    base_physical_drive = (fbe_base_physical_drive_t *)context;

    payload = fbe_transport_get_payload_ex(packet);
    payload_discovery_operation = fbe_payload_ex_get_discovery_operation(payload);
    fbe_payload_discovery_get_status(payload_discovery_operation, &discovery_status);
    control_operation = fbe_payload_ex_get_prev_control_operation(payload);

    /* We HAVE to check status of packet before we continue */
    status = fbe_transport_get_status_code(packet);
    if(status != FBE_STATUS_OK){
        fbe_base_physical_drive_customizable_trace(base_physical_drive, 
                                FBE_TRACE_LEVEL_ERROR,
                                "%s Invalid packet status: 0x%X\n",
                                __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_payload_ex_release_discovery_operation(payload, payload_discovery_operation);
        return FBE_STATUS_OK;
    }

    if(discovery_status != FBE_PAYLOAD_DISCOVERY_STATUS_OK) { 
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    } else {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }

    fbe_payload_ex_release_discovery_operation(payload, payload_discovery_operation);
    return FBE_STATUS_OK;

}
fbe_status_t fbe_base_physical_drive_handle_negotiate_block_size(fbe_base_physical_drive_t *physical_drive_p,
                                                                 fbe_packet_t *packet_p)
{
    fbe_status_t    status;
    fbe_block_transport_negotiate_t* negotiate_block_size_p = NULL;
    fbe_payload_ex_t *payload_p = fbe_transport_get_payload_ex(packet_p);
    fbe_payload_block_operation_t *block_operation_p = fbe_payload_ex_get_block_operation(payload_p);
    fbe_sg_element_t *sg_p = NULL;

    fbe_payload_ex_get_sg_list(payload_p, &sg_p, NULL);

    if (sg_p == NULL)
    {
        /* We always expect to receive a buffer.
         */
        fbe_base_object_trace((fbe_base_object_t*)physical_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                              "physical drive: negotiate sg null. 0x%p %s\n", 
                              physical_drive_p, __FUNCTION__);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_payload_block_set_status(block_operation_p, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NULL_SG);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Next, get a ptr to the negotiate info. 
     */
    negotiate_block_size_p = (fbe_block_transport_negotiate_t*)fbe_sg_element_address(sg_p);

    if (negotiate_block_size_p == NULL)
    {
        /* We always expect to receive a buffer.
         */
        fbe_base_object_trace((fbe_base_object_t*)physical_drive_p, 
                              FBE_TRACE_LEVEL_ERROR, 
                              FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                              "%s drive obj: 0x%x (%d_%d_%d) NULL buffer\n", 
                              __FUNCTION__, physical_drive_p->base_discovering.base_discovered.base_object.object_id,
                              physical_drive_p->port_number, physical_drive_p->enclosure_number, physical_drive_p->slot_number);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_payload_block_set_status(block_operation_p, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_INVALID_SG);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* If the block size is not specified use the default.
     */
    if (negotiate_block_size_p->block_size == 0)
    {
        negotiate_block_size_p->block_size = FBE_BE_BYTES_PER_BLOCK;
    }

    /* Get the exported capacity (in terms of client block size)
     */
    status = fbe_base_physical_drive_get_exported_capacity(physical_drive_p, negotiate_block_size_p->block_size,
                                                           &negotiate_block_size_p->block_count);
    if (status != FBE_STATUS_OK)
    {
        /* The physical block size must be equal or greater and a multiple.
         */
        fbe_base_object_trace((fbe_base_object_t*)physical_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                              "%s get exported obj: 0x%x (%d_%d_%d) client block size: %d physical: %d failed\n",
                              __FUNCTION__, physical_drive_p->base_discovering.base_discovered.base_object.object_id,
                              physical_drive_p->port_number, physical_drive_p->enclosure_number, physical_drive_p->slot_number,
                              negotiate_block_size_p->block_size, physical_drive_p->block_size);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_payload_block_set_status(block_operation_p, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNALIGNED_REQUEST);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Get the optimal block alignment.
     */
    status = fbe_base_physical_drive_get_optimal_block_alignment(physical_drive_p, negotiate_block_size_p->block_size,
                                                                 &negotiate_block_size_p->optimum_block_alignment);
    if (status != FBE_STATUS_OK)
    {
        /* The physical block size must be equal or greater and a multiple.
         */
        fbe_base_object_trace((fbe_base_object_t*)physical_drive_p, 
                              FBE_TRACE_LEVEL_WARNING, 
                              FBE_TRACE_MESSAGE_ID_INVALID_PARAMETER,
                              "%s get alignment obj: 0x%x (%d_%d_%d) client block size: %d physical: %d failed\n",
                              __FUNCTION__, physical_drive_p->base_discovering.base_discovered.base_object.object_id,
                              physical_drive_p->port_number, physical_drive_p->enclosure_number, physical_drive_p->slot_number,
                              negotiate_block_size_p->block_size, physical_drive_p->block_size);
        fbe_transport_set_status(packet_p, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_payload_block_set_status(block_operation_p, 
                                     FBE_PAYLOAD_BLOCK_OPERATION_STATUS_INVALID_REQUEST,
                                     FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_UNALIGNED_REQUEST);
        fbe_transport_complete_packet(packet_p);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*! @todo Get rid of optimal block size 
     */
    negotiate_block_size_p->optimum_block_size = negotiate_block_size_p->optimum_block_alignment;

    negotiate_block_size_p->physical_block_size = physical_drive_p->block_size;
    negotiate_block_size_p->physical_optimum_block_size = 1;

    fbe_transport_set_status(packet_p, FBE_STATUS_OK, 0);
    fbe_payload_block_set_status(block_operation_p, 
                                 FBE_PAYLOAD_BLOCK_OPERATION_STATUS_SUCCESS,
                                 FBE_PAYLOAD_BLOCK_OPERATION_QUALIFIER_NONE);
    fbe_transport_complete_packet(packet_p);
    return FBE_STATUS_OK;
}


/*!**************************************************************** 
   fbe_base_physical_drive_executor_fail_drive()
 *******************************************************************  
 * @brief
 *  Fails a drive with the specified death reason
 *
 * @param base_physical_drive_p - pointer to base physical drive object
 * @param reason - death reason
 * 
 * @return fbe_status_t - status of operation
 * 
 * @author
 *  05/07/2012  Wayne Garrett - Created.
 * 
 ******************************************************************/ 
fbe_status_t 
fbe_base_physical_drive_executor_fail_drive(fbe_base_physical_drive_t * base_physical_drive_p, fbe_base_physical_drive_death_reason_t reason)
{
    fbe_object_id_t objectId;
    fbe_status_t    status;

    status = fbe_base_discovered_set_death_reason((fbe_base_discovered_t *)base_physical_drive_p, reason);

    if (status != FBE_STATUS_OK)
    {
        fbe_base_physical_drive_customizable_trace(base_physical_drive_p, 
                                                   FBE_TRACE_LEVEL_ERROR,
                                                   "%s Failed to set death reason: %d status: 0x%x\n",
                                                   __FUNCTION__, reason, status);
     }

    fbe_base_object_get_object_id((fbe_base_object_t *)base_physical_drive_p, &objectId);
    
    fbe_base_physical_drive_customizable_trace(base_physical_drive_p, FBE_TRACE_LEVEL_INFO,
            "%s Failing Drive. Object id 0x%x . death reason 0x%x\n",__FUNCTION__, objectId, reason);

    status = fbe_lifecycle_set_cond(&fbe_base_physical_drive_lifecycle_const, 
                                    (fbe_base_object_t*)base_physical_drive_p, 
                                     FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);
    if (status != FBE_STATUS_OK) {
        fbe_base_physical_drive_customizable_trace(base_physical_drive_p,  FBE_TRACE_LEVEL_ERROR,
                                                "%s Failed to set condition\n",__FUNCTION__);
    }

    return status;
}
