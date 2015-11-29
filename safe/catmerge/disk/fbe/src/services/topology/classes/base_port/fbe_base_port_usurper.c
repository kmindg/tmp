#include "fbe_base_discovered.h"
#include "base_port_private.h"
#include "fbe/fbe_board.h"
#include "fbe_transport_memory.h"

/* Forward declarations */
static fbe_status_t base_port_mgmt_get_port_number(fbe_base_port_t * base_port, fbe_packet_t * packet);
static fbe_status_t base_port_mgmt_get_port_role(fbe_base_port_t * base_port, fbe_packet_t * packet);
static fbe_status_t base_port_mgmt_stat_changed(fbe_base_port_t * base_port, fbe_packet_t * packet);
static fbe_status_t base_port_mgmt_get_port_info(fbe_base_port_t * base_port, fbe_packet_t * packet);
static fbe_status_t base_port_mgmt_get_hardware_info(fbe_base_port_t * base_port, fbe_packet_t * packet);
static fbe_status_t base_port_mgmt_get_sfp_info(fbe_base_port_t * base_port, fbe_packet_t * packet);
static fbe_status_t base_port_mgmt_set_debug_control(fbe_base_port_t * base_port, fbe_packet_t * packet);
static fbe_status_t base_port_mgmt_register_keys(fbe_base_port_t * base_port, fbe_packet_t * packet);
static fbe_status_t base_port_mgmt_unregister_keys(fbe_base_port_t * base_port, fbe_packet_t * packet);
static fbe_status_t base_port_mgmt_register_kek(fbe_base_port_t * base_port, fbe_packet_t * packet);
static fbe_status_t base_port_mgmt_unregister_kek(fbe_base_port_t * base_port, fbe_packet_t * packet);
static fbe_status_t base_port_mgmt_get_kek_handle(fbe_base_port_t * base_port, fbe_packet_t * packet);
static fbe_status_t base_port_mgmt_register_kek_kek(fbe_base_port_t * base_port, fbe_packet_t * packet);
static fbe_status_t base_port_mgmt_unregister_kek_kek(fbe_base_port_t * base_port, fbe_packet_t * packet);
static fbe_status_t base_port_mgmt_reestablish_key_handle(fbe_base_port_t *base_port, fbe_packet_t * packet);
static fbe_status_t base_port_mgmt_set_encryption_mode(fbe_base_port_t *base_port, fbe_packet_t * packet);
static fbe_status_t base_port_mgmt_debug_register_dek(fbe_base_port_t *base_port, fbe_packet_t * packet);

fbe_status_t 
fbe_base_port_control_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_payload_control_operation_opcode_t control_code;
    fbe_base_port_t * base_port = NULL;

    /*KvTrace("fbe_base_port_main: %s entry\n", __FUNCTION__);*/
    base_port = (fbe_base_port_t *)fbe_base_handle_to_pointer(object_handle);

    control_code = fbe_transport_get_control_code(packet);
    switch(control_code) {
        case FBE_BASE_PORT_CONTROL_CODE_GET_PORT_NUMBER:
            status = base_port_mgmt_get_port_number(base_port, packet);
            break;
        case FBE_BASE_PORT_CONTROL_CODE_GET_PORT_ROLE:
            status = base_port_mgmt_get_port_role(base_port, packet);
            break;
        case FBE_BASE_PORT_CONTROL_CODE_GET_PORT_INFO:
            status = base_port_mgmt_get_port_info (base_port, packet);
            break;
        case FBE_BASE_PORT_CONTROL_CODE_GET_HARDWARE_INFORMATION:
            status = base_port_mgmt_get_hardware_info(base_port, packet);
            break;
        case FBE_BASE_PORT_CONTROL_CODE_GET_SFP_INFORMATION:
            status = base_port_mgmt_get_sfp_info(base_port, packet);
            break;
        case FBE_BASE_PORT_CONTROL_CODE_SET_DEBUG_CONTROL:
            status = base_port_mgmt_set_debug_control(base_port, packet);
            break;
        case FBE_BASE_PORT_CONTROL_CODE_REGISTER_KEYS:
            status = base_port_mgmt_register_keys(base_port, packet);
            break;
        case FBE_BASE_PORT_CONTROL_CODE_UNREGISTER_KEYS:
            status = base_port_mgmt_unregister_keys(base_port, packet);
            break;
        case FBE_BASE_PORT_CONTROL_CODE_REGISTER_KEK:
            status = base_port_mgmt_register_kek(base_port, packet);
            break;
        case FBE_BASE_PORT_CONTROL_CODE_UNREGISTER_KEK:
            status = base_port_mgmt_unregister_kek(base_port, packet);
            break;
        case FBE_BASE_PORT_CONTROL_CODE_REGISTER_KEK_KEK:
            status = base_port_mgmt_register_kek_kek(base_port, packet);
            break;
        case FBE_BASE_PORT_CONTROL_CODE_UNREGISTER_KEK_KEK:
            status = base_port_mgmt_unregister_kek_kek(base_port, packet);
            break;
        case FBE_BASE_PORT_CONTROL_CODE_GET_KEK_HANDLE:
            status = base_port_mgmt_get_kek_handle(base_port, packet);
            break;
        case FBE_BASE_PORT_CONTROL_CODE_REESTABLISH_KEY_HANDLE:
            status = base_port_mgmt_reestablish_key_handle(base_port, packet);
            break;
        case FBE_BASE_PORT_CONTROL_CODE_SET_ENCRYPTION_MODE:
            status = base_port_mgmt_set_encryption_mode(base_port, packet);
            break;
        case FBE_BASE_PORT_CONTROL_CODE_DBEUG_REGISTER_DEK:
            status = base_port_mgmt_debug_register_dek(base_port, packet);
            break;
        default:
            status =  fbe_base_discovering_control_entry(object_handle, packet);
            break;
    }

    return status;
}


static fbe_status_t 
base_port_mgmt_get_port_number(fbe_base_port_t * base_port, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_port_mgmt_get_port_number_t * base_port_mgmt_get_port_number = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &base_port_mgmt_get_port_number); 
    if(base_port_mgmt_get_port_number == NULL){
        KvTrace("fbe_base_port_main: %s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_base_port_mgmt_get_port_number_t)){
        KvTrace("fbe_base_port_main: %s Invalid out_buffer_length %X \n", __FUNCTION__, out_len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*status = fbe_base_port_get_port_number(base_port, &base_port_mgmt_get_port_number->port_number);*/
    status = fbe_base_port_get_assigned_bus_number(base_port, &base_port_mgmt_get_port_number->port_number);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

static fbe_status_t 
base_port_mgmt_get_port_role(fbe_base_port_t * base_port, fbe_packet_t * packet)
{
    fbe_status_t status;
    fbe_base_port_mgmt_get_port_role_t * base_port_mgmt_get_port_role = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &base_port_mgmt_get_port_role); 
    if(base_port_mgmt_get_port_role == NULL){
        KvTrace("fbe_base_port_main: %s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_base_port_mgmt_get_port_role_t)){
        KvTrace("fbe_base_port_main: %s Invalid out_buffer_length %X \n", __FUNCTION__, out_len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*status = fbe_base_port_get_port_number(base_port, &base_port_mgmt_get_port_number->port_number);*/
    status = fbe_base_port_get_port_role(base_port, &base_port_mgmt_get_port_role->port_role);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

static fbe_status_t 
base_port_mgmt_get_port_info (fbe_base_port_t * base_port, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_port_info_t * base_port_info_p = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_port_number_t   port_number;
    fbe_port_info_t port_info;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);


    fbe_payload_control_get_buffer(control_operation, &base_port_info_p); 
    if(base_port_info_p == NULL){
        KvTrace("fbe_base_port_main: %s fbe_payload_control_get_buffer fail\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_port_info_t)){
        KvTrace("fbe_base_port_main: %s Invalid out_buffer_length %X \n", __FUNCTION__, out_len);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*status = fbe_base_port_get_port_number(base_port, &base_port_mgmt_get_port_number->port_number);*/
    /*fbe_base_port_get_port_type (base_port, &(base_port_info_p->type));*/
    fbe_base_port_get_port_role (base_port, &(base_port_info_p->port_role));
    fbe_base_port_get_port_number (base_port, &(base_port_info_p->io_port_number));
    fbe_base_port_get_io_portal_number (base_port, &(base_port_info_p->io_portal_number));
    fbe_base_port_get_assigned_bus_number(base_port, &(base_port_info_p->assigned_bus_number));
    fbe_base_port_get_maximum_transfer_length(base_port,&(base_port_info_p->maximum_transfer_bytes));
    fbe_base_port_get_maximum_sg_entries(base_port,&(base_port_info_p->maximum_sg_entries));
    fbe_base_port_get_link_info(base_port,&(base_port_info_p->port_link_info));
    fbe_base_port_get_core_affinity_info(base_port,&(base_port_info_p->multi_core_affinity_enabled),
                                                    &(base_port_info_p->active_proc_mask));
    base_port_info_p->link_speed = FBE_PORT_SPEED_INVALID;
    
    (void)fbe_base_port_get_port_number(base_port, &port_number);

    fbe_zero_memory(&port_info, sizeof(fbe_port_info_t));
    fbe_cpd_shim_get_port_info(port_number, &port_info);
    base_port_info_p->enc_mode = port_info.enc_mode;

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}


static fbe_status_t 
base_port_mgmt_get_hardware_info(fbe_base_port_t * base_port, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_port_hardware_info_t * base_hardware_info_p = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &base_hardware_info_p); 
    if(base_hardware_info_p == NULL){        
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s fbe_payload_control_get_buffer fail\n",
								__FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_port_hardware_info_t)){        
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Invalid out_buffer_length %X \n",
								__FUNCTION__, out_len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = fbe_base_port_get_hardware_info(base_port, base_hardware_info_p);
    if (status != FBE_STATUS_OK){
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s status: 0x%X returned from fbe_base_port_get_hardware_info",
								__FUNCTION__, status);
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

static fbe_status_t 
base_port_mgmt_register_keys(fbe_base_port_t * base_port, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_port_mgmt_register_keys_t * port_register_keys_p = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_port_number_t               port_number;
    fbe_key_handle_t    kek_handle;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &port_register_keys_p); 
    if(port_register_keys_p == NULL){        
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s fbe_payload_control_get_buffer fail\n",
								__FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_base_port_mgmt_register_keys_t)){        
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Invalid out_buffer_length %X \n",
								__FUNCTION__, out_len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
     
    fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s Received Key registration\n",
								__FUNCTION__);
    (void)fbe_base_port_get_port_number(base_port, &port_number);

    status = fbe_base_port_get_kek_handle(base_port, &kek_handle);

    /* Add the KEK handle so that Miniport can use it to unwrap the DEK(Data Encryption Keys) */
    port_register_keys_p->kek_handle = kek_handle;

    status = fbe_cpd_shim_register_data_encryption_keys(port_number, port_register_keys_p);

    if (status != FBE_STATUS_OK){
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s status: 0x%X returned from registration of keys",
								__FUNCTION__, status);
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
    return status;
}

static fbe_status_t 
base_port_mgmt_unregister_keys(fbe_base_port_t * base_port, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_port_mgmt_unregister_keys_t * port_unregister_keys_p = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_port_number_t               port_number;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &port_unregister_keys_p); 
    if(port_unregister_keys_p == NULL){        
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s fbe_payload_control_get_buffer fail\n",
								__FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_base_port_mgmt_unregister_keys_t)){        
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Invalid out_buffer_length %X \n",
								__FUNCTION__, out_len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s Received Key Unregistration\n",
								__FUNCTION__);
    (void)fbe_base_port_get_port_number(base_port, &port_number);

    status = fbe_cpd_shim_unregister_data_encryption_keys(port_number, port_unregister_keys_p);

    if (status != FBE_STATUS_OK){
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s status: 0x%X returned from registration of keys",
								__FUNCTION__, status);
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
    return status;
}

static fbe_status_t 
base_port_mgmt_register_kek(fbe_base_port_t * base_port, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_port_mgmt_register_keys_t port_register_keys;
    fbe_base_port_mgmt_register_kek_t * port_register_kek_p = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_port_number_t               port_number;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &port_register_kek_p); 
    if(port_register_kek_p == NULL){        
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s fbe_payload_control_get_buffer fail\n",
								__FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_base_port_mgmt_register_kek_t)){        
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Invalid out_buffer_length %X \n",
								__FUNCTION__, out_len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(base_port->kek_handle != FBE_INVALID_KEY_HANDLE) {
        /* We already have a handle and so just return that */
        fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s Kek handle already setup. Kek Handle:0x%llx \n",
								__FUNCTION__, (unsigned long long) base_port->kek_handle);
        port_register_kek_p->kek_handle = base_port->kek_handle;
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return FBE_STATUS_OK;
    }

    fbe_zero_memory(&port_register_keys, sizeof(fbe_base_port_mgmt_register_keys_t));

    port_register_keys.key_size = port_register_kek_p->key_size;
    port_register_keys.key_ptr = port_register_kek_p->kek_ptr;
    port_register_keys.num_of_keys = 1;

    if(port_register_kek_p->kek_kek_handle == FBE_INVALID_KEY_HANDLE) {
        port_register_keys.kek_handle = base_port->kek_kek_handle;
    }
    else
    {
        if(port_register_kek_p->kek_kek_handle != base_port->kek_kek_handle) {
            fbe_base_object_trace((fbe_base_object_t*)base_port, 
                                  FBE_TRACE_LEVEL_ERROR,
                                  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                  "%s Kek kek handle incorrect \n",
                                  __FUNCTION__);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        port_register_keys.kek_handle = port_register_kek_p->kek_kek_handle;
    }

    (void)fbe_base_port_get_port_number(base_port, &port_number);
    
    status = fbe_cpd_shim_register_kek(port_number, &port_register_keys, packet);

    if (status != FBE_STATUS_OK){
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s status: 0x%X returned from registration of keys",
								__FUNCTION__, status);
         fbe_transport_set_status(packet, status, 0);
         fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
    }
    else {
        base_port->kek_handle = port_register_keys.mp_key_handle[0];
        port_register_kek_p->kek_handle = base_port->kek_handle;

        /* Save the Kek as we might need it for controller reset processing */
        fbe_copy_memory(base_port->kek_key, 
                        port_register_kek_p->kek_ptr,
                        port_register_kek_p->key_size);

        fbe_base_object_trace((fbe_base_object_t*)base_port, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s Kek Reg. KEKKEK Handle:0x%llx, KEK Handle:0x%llx\n",
                                    __FUNCTION__,
                                    (unsigned long long) port_register_keys.kek_handle,
                                    (unsigned long long) port_register_keys.mp_key_handle[0]);
        /* This packet will be completed by the callback from the miniport */
    }

   
    return status;
}

static fbe_status_t 
base_port_mgmt_debug_register_dek(fbe_base_port_t * base_port, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_port_mgmt_register_keys_t port_register_keys;
    fbe_base_port_mgmt_debug_register_dek_t * port_register_dek_p = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_port_number_t               port_number;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &port_register_dek_p); 
    if(port_register_dek_p == NULL){        
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s fbe_payload_control_get_buffer fail\n",
								__FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_base_port_mgmt_debug_register_dek_t)){        
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Invalid out_buffer_length %X \n",
								__FUNCTION__, out_len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(base_port->kek_handle == FBE_INVALID_KEY_HANDLE) {
        fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Kek handle Not setup \n",
								__FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_zero_memory(&port_register_keys, sizeof(fbe_base_port_mgmt_register_keys_t));

    port_register_keys.key_size = port_register_dek_p->key_size;
    port_register_keys.key_ptr = (fbe_key_handle_t *)&port_register_dek_p->key[0];
    port_register_keys.num_of_keys = 1;
    port_register_keys.kek_handle = base_port->kek_handle;
    
    (void)fbe_base_port_get_port_number(base_port, &port_number);
    
    status = fbe_cpd_shim_register_data_encryption_keys(port_number, &port_register_keys);

    if (status != FBE_STATUS_OK){
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s status: 0x%X returned from registration of keys",
								__FUNCTION__, status);
    }
    else {
        fbe_base_object_trace((fbe_base_object_t*)base_port, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s Kek Reg. Handle:0x%llx, DEK Handle:0x%llx\n",
                                    __FUNCTION__,
                                    (unsigned long long) port_register_keys.kek_handle,
                                    (unsigned long long) port_register_keys.mp_key_handle[0]);
         port_register_dek_p->key_handle = port_register_keys.mp_key_handle[0];
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
    return status;
}

static fbe_status_t 
base_port_mgmt_unregister_kek(fbe_base_port_t * base_port, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_port_number_t               port_number;
    fbe_base_port_mgmt_unregister_keys_t unregister_info;

    (void)fbe_base_port_get_port_number(base_port, &port_number);

    if(base_port->kek_handle == FBE_INVALID_KEY_HANDLE) {
        fbe_base_object_trace((fbe_base_object_t*)base_port, 
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "%s KEK Already Unregistered \n",
                                    __FUNCTION__);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return status;

    }

    unregister_info.num_of_keys = 1;
    unregister_info.mp_key_handle[0] = base_port->kek_handle;

    fbe_base_object_trace((fbe_base_object_t*)base_port, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Unregister KEK 0x%llx \n",
                                __FUNCTION__, (unsigned long long) base_port->kek_handle );

    status = fbe_cpd_shim_unregister_kek(port_number, &unregister_info);

    if (status != FBE_STATUS_OK){
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s status: 0x%X returned from registration of keys",
								__FUNCTION__, status);
    }

    base_port->kek_handle = FBE_INVALID_KEY_HANDLE;
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
    return status;
}

static fbe_status_t 
base_port_mgmt_register_kek_kek(fbe_base_port_t * base_port, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_port_mgmt_register_keys_t port_register_keys;
    fbe_base_port_mgmt_register_kek_kek_t * port_register_kek_p = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_port_number_t               port_number;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &port_register_kek_p); 
    if(port_register_kek_p == NULL){        
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s fbe_payload_control_get_buffer fail\n",
								__FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_base_port_mgmt_register_kek_kek_t)){        
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Invalid out_buffer_length %X \n",
								__FUNCTION__, out_len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(base_port->kek_kek_handle != FBE_INVALID_KEY_HANDLE) {
        fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Kek KEK handle already setup \n",
								__FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    
    fbe_zero_memory(&port_register_keys, sizeof(fbe_base_port_mgmt_register_keys_t));

    port_register_keys.key_size = port_register_kek_p->key_size;
    port_register_keys.key_ptr = port_register_kek_p->kek_ptr;
    port_register_keys.num_of_keys = 1;
    
    (void)fbe_base_port_get_port_number(base_port, &port_number);
    
    status = fbe_cpd_shim_register_kek_kek(port_number, &port_register_keys, packet);

    fbe_base_object_trace((fbe_base_object_t*)base_port, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Received KEK of Kek registration. Handle:0x%08x\n",
                                __FUNCTION__,(unsigned int) port_register_keys.mp_key_handle);

    if (status != FBE_STATUS_OK){
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s status: 0x%X returned from registration of keys",
								__FUNCTION__, status);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
    }
    else {
        /* The callback that gets called from miniport will complete the packet */
        fbe_base_object_trace((fbe_base_object_t*)base_port, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Received KEK of Kek registration. Kek Kek Handle:0x%llx\n",
                                __FUNCTION__,(unsigned long long) port_register_keys.mp_key_handle[0]);
         port_register_kek_p->mp_key_handle = port_register_keys.mp_key_handle[0];
         base_port->kek_kek_handle = port_register_keys.mp_key_handle[0];
    }
    return status;
}

static fbe_status_t 
base_port_mgmt_unregister_kek_kek(fbe_base_port_t * base_port, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_port_number_t               port_number;
    fbe_base_port_mgmt_unregister_keys_t *unregister_info;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &unregister_info); 
    if(unregister_info == NULL){        
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s fbe_payload_control_get_buffer fail\n",
								__FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_base_port_mgmt_unregister_keys_t)){        
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Invalid out_buffer_length %X \n",
								__FUNCTION__, out_len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    (void)fbe_base_port_get_port_number(base_port, &port_number);

    fbe_base_object_trace((fbe_base_object_t*)base_port, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Unregister KEK of KEK \n",
                                __FUNCTION__);

    if(unregister_info->mp_key_handle[0] == FBE_INVALID_KEY_HANDLE) {
        unregister_info->mp_key_handle[0] = base_port->kek_kek_handle;
    }
    status = fbe_cpd_shim_unregister_kek_kek(port_number, unregister_info);

    if (status != FBE_STATUS_OK){
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s status: 0x%X returned from registration of keys",
								__FUNCTION__, status);
    }

    base_port->kek_kek_handle = FBE_INVALID_KEY_HANDLE;

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
    return status;
}

static fbe_status_t 
base_port_mgmt_reestablish_key_handle(fbe_base_port_t * base_port, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_port_number_t               port_number;
    fbe_base_port_mgmt_reestablish_key_handle_t *reestablish_key_handle;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &reestablish_key_handle); 
    if(reestablish_key_handle == NULL){        
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s fbe_payload_control_get_buffer fail\n",
								__FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_base_port_mgmt_reestablish_key_handle_t)){        
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Invalid out_buffer_length %X \n",
								__FUNCTION__, out_len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if(reestablish_key_handle->mp_key_handle == base_port->kek_kek_handle) {
        fbe_base_object_trace((fbe_base_object_t*)base_port, 
                              FBE_TRACE_LEVEL_INFO,
                              FBE_TRACE_MESSAGE_ID_INFO,
                              "%s Already Establish KEK of KEK handle 0x%llx \n",
                              __FUNCTION__, 
                              (unsigned long long) reestablish_key_handle->mp_key_handle );
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return FBE_STATUS_OK;
    }

    (void)fbe_base_port_get_port_number(base_port, &port_number);

    fbe_base_object_trace((fbe_base_object_t*)base_port, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Reestablish KEK of KEK handle 0x%llx \n",
                                __FUNCTION__, (unsigned long long) reestablish_key_handle->mp_key_handle );

    status = fbe_cpd_shim_reestablish_key_handle(port_number, reestablish_key_handle);

    if (status != FBE_STATUS_OK){
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s status: 0x%X returned from registration of keys",
								__FUNCTION__, status);
    }

    base_port->kek_kek_handle = reestablish_key_handle->mp_key_handle;
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
    return status;
}
static fbe_status_t 
base_port_mgmt_get_kek_handle(fbe_base_port_t * base_port, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_port_mgmt_get_kek_handle_t * kek_handle_p = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &kek_handle_p); 
    if(kek_handle_p == NULL){        
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s fbe_payload_control_get_buffer fail\n",
								__FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_base_port_mgmt_get_kek_handle_t)){        
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Invalid out_buffer_length %X \n",
								__FUNCTION__, out_len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_object_trace((fbe_base_object_t*)base_port, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Return the Kek handle back\n",
                                __FUNCTION__);

    if(base_port->kek_handle == FBE_INVALID_KEY_HANDLE) {
        fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Kek handle not setup \n",
								__FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    kek_handle_p->kek_ptr =  base_port->kek_handle;

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

static fbe_status_t 
base_port_mgmt_set_encryption_mode(fbe_base_port_t * base_port, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_port_mgmt_set_encryption_mode_t * encryption_mode_p = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;
    fbe_port_number_t               port_number;
    fbe_port_info_t port_info;
    
    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &encryption_mode_p); 
    if(encryption_mode_p == NULL){        
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s fbe_payload_control_get_buffer fail\n",
								__FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_base_port_mgmt_set_encryption_mode_t)){        
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Invalid out_buffer_length %X \n",
								__FUNCTION__, out_len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    (void)fbe_base_port_get_port_number(base_port, &port_number);

    fbe_zero_memory(&port_info, sizeof(fbe_port_info_t));
    fbe_cpd_shim_get_port_info(port_number, &port_info);

    /* we need to set it only if the port is not already in that mode */
    if(port_info.enc_mode == encryption_mode_p->encryption_mode) {
        fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
								"%s Mode %d already set \n",
								__FUNCTION__, port_info.enc_mode);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
        return status;
    }

    fbe_base_object_trace((fbe_base_object_t*)base_port, 
                                FBE_TRACE_LEVEL_INFO,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Set Encryption Mode to:%d \n",
                                __FUNCTION__, encryption_mode_p->encryption_mode);

    status = fbe_cpd_shim_set_encryption_mode(port_number, encryption_mode_p->encryption_mode);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_run_queue_push_packet(packet, FBE_TRANSPORT_RQ_METHOD_SAME_CORE);
    return status;
}

static fbe_status_t 
base_port_mgmt_get_sfp_info(fbe_base_port_t * base_port, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_port_sfp_info_t *port_sfp_info_p = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &port_sfp_info_p);
    if(port_sfp_info_p == NULL){        
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s fbe_payload_control_get_buffer fail\n",
								__FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_port_sfp_info_t)){        
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Invalid out_buffer_length %X \n",
								__FUNCTION__, out_len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    status = fbe_base_port_get_sfp_info(base_port, port_sfp_info_p);    
    if (status != FBE_STATUS_OK){
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s status: 0x%X returned from fbe_base_port_get_hardware_info",
								__FUNCTION__, status);
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}

static fbe_status_t
base_port_mgmt_set_debug_control(fbe_base_port_t * base_port, fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_port_dbg_ctrl_t *port_dbg_ctrl_p = NULL;
    fbe_payload_control_buffer_length_t out_len = 0;
    fbe_payload_ex_t * payload = NULL;
    fbe_payload_control_operation_t * control_operation = NULL;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    fbe_payload_control_get_buffer(control_operation, &port_dbg_ctrl_p);
    if(port_dbg_ctrl_p == NULL){
		fbe_base_object_trace((fbe_base_object_t*)base_port,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s fbe_payload_control_get_buffer fail\n",
								__FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_get_buffer_length(control_operation, &out_len);
    if(out_len != sizeof(fbe_port_dbg_ctrl_t)){
		fbe_base_object_trace((fbe_base_object_t*)base_port,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Invalid out_buffer_length %X \n",
								__FUNCTION__, out_len);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_base_port_set_debug_remove(base_port, port_dbg_ctrl_p->insert_masked);
    if (status != FBE_STATUS_OK){
		fbe_base_object_trace((fbe_base_object_t*)base_port,
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s status: 0x%X returned from fbe_base_port_get_hardware_info",
								__FUNCTION__, status);
    }

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}
