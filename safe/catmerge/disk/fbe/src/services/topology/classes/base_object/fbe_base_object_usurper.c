#include "base_object_private.h"

/* Forward declaration */
static fbe_status_t base_object_get_class_id(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t base_object_get_lifecycle_state(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t base_object_log_lifecycle_trace(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_status_t base_object_set_cond(fbe_base_object_t * p_object, fbe_packet_t * p_packet, fbe_lifecycle_cond_id_t cond_id);
static fbe_status_t base_object_trace_level_handler(fbe_base_object_t * p_object, fbe_packet_t * p_packet);
static fbe_status_t base_object_trace_flag_handler(fbe_base_object_t * p_object, fbe_packet_t * p_packet);

fbe_status_t 
fbe_base_object_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    /*fbe_payload_control_operation_opcode_t control_code;*/
    fbe_base_object_t * base_object = NULL;
    fbe_packet_attr_t packet_attr;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_operation_opcode_t opcode;

    /* KvTrace("%s entry\n", __FUNCTION__); */
    base_object = (fbe_base_object_t *)fbe_base_handle_to_pointer(object_handle);

    fbe_base_object_trace(  (fbe_base_object_t*)base_object,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry \n", __FUNCTION__);

    /* control_code = fbe_transport_get_control_code(packet); */
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_get_opcode(control_operation, &opcode);
    switch(opcode) {
/*      case FBE_BASE_OBJECT_CONTROL_CODE_GET_LEVEL:
            status = base_object_get_level( base_object, packet);       
            break;
*/
        case FBE_BASE_OBJECT_CONTROL_CODE_GET_CLASS_ID:
            status = base_object_get_class_id( base_object, packet);        
            break;

        case FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE:
            status = base_object_get_lifecycle_state(base_object, packet);
            break;
        case FBE_BASE_OBJECT_CONTROL_CODE_LOG_LIFECYCLE_TRACE:
            status = base_object_log_lifecycle_trace(base_object, packet);
            break;
        case FBE_BASE_OBJECT_CONTROL_CODE_SET_ACTIVATE_COND:
            status = base_object_set_cond(base_object, packet, FBE_BASE_OBJECT_LIFECYCLE_COND_ACTIVATE);
            break;
        case FBE_BASE_OBJECT_CONTROL_CODE_SET_HIBERNATE_COND:
            status = base_object_set_cond(base_object, packet, FBE_BASE_OBJECT_LIFECYCLE_COND_HIBERNATE);
            break;
        case FBE_BASE_OBJECT_CONTROL_CODE_SET_OFFLINE_COND:
            status = base_object_set_cond(base_object, packet, FBE_BASE_OBJECT_LIFECYCLE_COND_OFFLINE);
            break;
        case FBE_BASE_OBJECT_CONTROL_CODE_SET_FAIL_COND:
            status = base_object_set_cond(base_object, packet, FBE_BASE_OBJECT_LIFECYCLE_COND_FAIL);
            break;
        case FBE_BASE_OBJECT_CONTROL_CODE_SET_DESTROY_COND:
            status = base_object_set_cond(base_object, packet, FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
            break;
        case FBE_BASE_OBJECT_CONTROL_CODE_GET_TRACE_LEVEL:
        case FBE_BASE_OBJECT_CONTROL_CODE_SET_TRACE_LEVEL:
            status = base_object_trace_level_handler(base_object, packet);
            break;
        case FBE_BASE_OBJECT_CONTROL_CODE_GET_TRACE_FLAG:
        case FBE_BASE_OBJECT_CONTROL_CODE_SET_TRACE_FLAG:
            status = base_object_trace_flag_handler(base_object, packet);
            break;
        case FBE_BASE_OBJECT_CONTROL_CODE_SET_DEBUG_HOOK:
            base_object->scheduler_element.hook = fbe_scheduler_debug_hook;
            base_object->scheduler_element.hook_counter ++;
            status = FBE_STATUS_OK;
            fbe_transport_set_status(packet, status, 0);
            fbe_transport_complete_packet(packet);
            break;
        case FBE_BASE_OBJECT_CONTROL_CODE_CLEAR_DEBUG_HOOK:
            base_object->scheduler_element.hook_counter --;
            if (base_object->scheduler_element.hook_counter == 0) {
                base_object->scheduler_element.hook = NULL;
            }
            status = FBE_STATUS_OK;
            fbe_transport_set_status(packet, status, 0);
            fbe_transport_complete_packet(packet);
            break;
        default:
            /* The control code is uknown, but may be it is traverse packet */
            /* fbe_transport_get_packet_type(packet, &packet_type); */
            fbe_transport_get_packet_attr(packet, &packet_attr);
            if(packet_attr & FBE_PACKET_FLAG_TRAVERSE)
            {
                /* This object is not responsible to handle this packet and traverse flag set so decrement
                 * the stack level and allow packet to traverse.
                 */
                fbe_transport_decrement_stack_level(packet);
                fbe_base_object_decrement_userper_counter(object_handle);
                return FBE_STATUS_TRAVERSE;
            }
            else
            {
                fbe_base_object_trace(base_object,
                                      FBE_TRACE_LEVEL_WARNING,
                                      FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                      "%s Unknown control code 0x%X\n",
                                      __FUNCTION__, opcode);
                fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
                status = fbe_transport_complete_packet(packet);
            }

            break;
    }

    return status;
}

static fbe_status_t 
base_object_get_class_id(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_object_mgmt_get_class_id_t * base_object_mgmt_get_class_id = NULL;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_payload_control_buffer_length_t buffer_length;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    status = fbe_payload_control_get_buffer(control_operation, &base_object_mgmt_get_class_id);
    if(status != FBE_STATUS_OK){
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    status = fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(status != FBE_STATUS_OK){
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    if(buffer_length != sizeof(fbe_base_object_mgmt_get_class_id_t)){
        fbe_base_object_trace(  (fbe_base_object_t*)base_object,
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s, invalid buffer_length %d\n",
                                __FUNCTION__, buffer_length);
        
        status = FBE_STATUS_GENERIC_FAILURE;

        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    /* Check object state MUST be "specialized" */

    base_object_mgmt_get_class_id->class_id = base_object->class_id;
    status = FBE_STATUS_OK;
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

fbe_status_t
fbe_base_object_get_lifecycle_state(fbe_base_object_t * base_object, fbe_lifecycle_state_t *lifecycle_state)
{
	fbe_status_t status;
	status = fbe_lifecycle_get_state((fbe_lifecycle_const_t*)&fbe_base_object_lifecycle_const, base_object, lifecycle_state);
	return status;
}

static fbe_status_t
base_object_get_lifecycle_state(fbe_base_object_t * p_base_object, fbe_packet_t * p_packet)
{
    fbe_base_object_mgmt_get_lifecycle_state_t * p_response;
    fbe_status_t status;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    payload = fbe_transport_get_payload_ex(p_packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);    

    fbe_payload_control_get_buffer(control_operation, &p_response); 

    if (p_response != NULL) {
        status = fbe_base_object_get_lifecycle_state(p_base_object, &p_response->lifecycle_state);
/*
        status = fbe_lifecycle_get_state((fbe_lifecycle_const_t*)&fbe_base_object_lifecycle_const,
                                         p_base_object,
                                         &p_response->lifecycle_state);
 */
    }
    else {
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_transport_set_status(p_packet, status, 0);
    fbe_transport_complete_packet(p_packet);
    return status;
}

static fbe_status_t
base_object_log_lifecycle_trace(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
    /* XXX FIXME XXX */
    return FBE_STATUS_OK;
}

static fbe_status_t
base_object_set_cond(fbe_base_object_t * p_object, fbe_packet_t * p_packet, fbe_lifecycle_cond_id_t cond_id)
{
    fbe_status_t status;

    status = fbe_lifecycle_set_cond((fbe_lifecycle_const_t*)&FBE_LIFECYCLE_CONST_DATA(base_object), p_object, cond_id);

	fbe_base_object_trace(	(fbe_base_object_t*)p_object,
							FBE_TRACE_LEVEL_DEBUG_LOW,
							FBE_TRACE_MESSAGE_ID_INFO,
							"%s, cond_id = %d\n",
							__FUNCTION__, cond_id);

    fbe_transport_set_status(p_packet, status, 0);
    fbe_transport_complete_packet(p_packet);

    return status;
}

static fbe_status_t
base_object_trace_level_handler(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_trace_level_t * control_trace_level;
    fbe_payload_control_operation_t * control_operation;
    fbe_payload_control_operation_opcode_t opcode;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(p_packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    control_trace_level = NULL;
    fbe_payload_control_get_buffer(control_operation, &control_trace_level);
    if (control_trace_level == NULL) {
        fbe_base_object_trace((fbe_base_object_t*)p_object,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, invalid control buffer\n", __FUNCTION__);
        fbe_transport_set_status(p_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(p_packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if (buffer_length != sizeof(fbe_trace_level_t)) {
        fbe_base_object_trace((fbe_base_object_t*)p_object, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                               "%s, invalid buffer_length, got: %d, wanted: %llu\n",
                               __FUNCTION__, buffer_length,
			       (unsigned long long)sizeof(fbe_trace_level_t));
        fbe_transport_set_status(p_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(p_packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_opcode(control_operation, &opcode);
    switch(opcode) {
        case FBE_BASE_OBJECT_CONTROL_CODE_GET_TRACE_LEVEL:
            *control_trace_level = p_object->trace_level;
            status = FBE_STATUS_OK;
            break;
        case FBE_BASE_OBJECT_CONTROL_CODE_SET_TRACE_LEVEL:
            if ((*control_trace_level > FBE_TRACE_LEVEL_INVALID) &&
                (*control_trace_level < FBE_TRACE_LEVEL_LAST)) {
                status = fbe_base_object_set_trace_level(p_object, *control_trace_level);
            }
            else {
                fbe_base_object_trace((fbe_base_object_t*)p_object,
                                      FBE_TRACE_LEVEL_ERROR,
                                      FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                                      "%s, trace_level %d invalid\n", 
                                      __FUNCTION__, *control_trace_level);
                status = FBE_STATUS_GENERIC_FAILURE;
            }
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    fbe_transport_set_status(p_packet, status, 0);
    fbe_transport_complete_packet(p_packet);
    return status;
}

static fbe_status_t
base_object_trace_flag_handler(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_trace_flags_t * control_trace_flag;
    fbe_payload_control_operation_t * control_operation;
    fbe_payload_control_operation_opcode_t opcode;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(p_packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    control_trace_flag = NULL;
    fbe_payload_control_get_buffer(control_operation, &control_trace_flag);
    if (control_trace_flag == NULL) 
    {
        fbe_base_object_trace((fbe_base_object_t*)p_object,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, invalid control buffer\n", __FUNCTION__);

        fbe_transport_set_status(p_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(p_packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if (buffer_length != sizeof(fbe_trace_flags_t)) 
    {
        fbe_base_object_trace((fbe_base_object_t*)p_object, 
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_INVALID_OUT_LEN,
                               "%s, invalid buffer_length, got: %d, wanted: %llu\n",
                               __FUNCTION__, buffer_length,
			       (unsigned long long)sizeof(fbe_trace_flags_t));

        fbe_transport_set_status(p_packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(p_packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_opcode(control_operation, &opcode);
    switch(opcode) 
    {
        case FBE_BASE_OBJECT_CONTROL_CODE_GET_TRACE_FLAG:
            *control_trace_flag = p_object->mgmt_attributes;
            status = FBE_STATUS_OK;
            break;
        case FBE_BASE_OBJECT_CONTROL_CODE_SET_TRACE_FLAG:
            status = fbe_base_object_set_mgmt_attributes(p_object, (fbe_object_mgmt_attributes_t) *control_trace_flag);
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    fbe_transport_set_status(p_packet, status, 0);
    fbe_transport_complete_packet(p_packet);
    return status;
}
