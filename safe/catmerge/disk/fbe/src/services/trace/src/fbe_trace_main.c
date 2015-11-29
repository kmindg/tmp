#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_transport.h"
#include "fbe/fbe_service.h"
#include "fbe/fbe_package.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_library_interface.h"
#include "fbe_trace.h"
#include "fbe_base.h"
#include "fbe_base_service.h"
#include "fbe_service_manager.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_trace_private.h"
#include "fbe_panic_sp.h"
#include "fbe_notification.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe/fbe_trace_interface.h"

static fbe_status_t fbe_trace_control_reset_error_counters(fbe_packet_t * packet);
static fbe_status_t fbe_trace_control_enable_backtrace(fbe_packet_t * packet);
static fbe_status_t fbe_trace_control_disable_backtrace(fbe_packet_t * packet);
static fbe_status_t fbe_lifecycle_debug_trace_control_set_flags(fbe_packet_t * packet);
static fbe_status_t fbe_lifecycle_debug_set_trace_flag(fbe_lifecycle_state_debug_tracing_flags_t default_trace_flag);
static fbe_status_t fbe_trace_control_command_to_ktrace_buff(fbe_packet_t *packet);

fbe_status_t fbe_trace_control_entry(fbe_packet_t * packet);
fbe_service_methods_t fbe_trace_service_methods = {FBE_SERVICE_ID_TRACE, fbe_trace_control_entry};

typedef struct fbe_trace_service_s {
    fbe_base_service_t base_service;
} fbe_trace_service_t;

static fbe_trace_service_t trace_service;

static fbe_u32_t lifecycle_debug_flag = FBE_LIFECYCLE_DEBUG_FLAG_NONE;

static fbe_u32_t trace_critical_error_counter;
static fbe_u32_t trace_error_counter;
#ifdef ALAMOSA_WINDOWS_ENV
static fbe_bool_t fbe_trace_backtrace_enabled = FBE_TRUE;
#else
// In KH, we have cases where the system thread does this backtrace 8
// times in a row for successive trace statements and causes the watch
// dog to go off.  Disabling this for now.
static fbe_bool_t fbe_trace_backtrace_enabled = FBE_FALSE;
#endif
fbe_trace_level_t fbe_trace_default_trace_level = FBE_TRACE_LEVEL_INFO;

typedef struct fbe_trace_current_error_limits_s {
    fbe_trace_error_limit_action_t action;
    fbe_u32_t num_errors;
} fbe_trace_current_error_limits_t;

fbe_trace_current_error_limits_t fbe_trace_current_error_limits[FBE_TRACE_LEVEL_LAST] = 
{ 
    {FBE_TRACE_ERROR_LIMIT_ACTION_INVALID, 0}, 
    {FBE_TRACE_ERROR_LIMIT_ACTION_INVALID, 0},
    {FBE_TRACE_ERROR_LIMIT_ACTION_INVALID, 0},
    {FBE_TRACE_ERROR_LIMIT_ACTION_INVALID, 0},  
    {FBE_TRACE_ERROR_LIMIT_ACTION_INVALID, 0},  
    {FBE_TRACE_ERROR_LIMIT_ACTION_INVALID, 0},  
    {FBE_TRACE_ERROR_LIMIT_ACTION_INVALID, 0},  
    {FBE_TRACE_ERROR_LIMIT_ACTION_INVALID, 0},   
};  

typedef struct fbe_trace_lib_trace_level_s {
    fbe_library_id_t library_id;
    fbe_trace_level_t trace_level;
} fbe_trace_lib_trace_level_t;

static fbe_trace_lib_trace_level_t fbe_trace_default_lib_trace_level[] = {
    { FBE_LIBRARY_ID_LIFECYCLE,             FBE_TRACE_LEVEL_INFO },
    { FBE_LIBRARY_ID_FLARE_SHIM,            FBE_TRACE_LEVEL_INFO },
    { FBE_LIBRARY_ID_ENCL_DATA_ACCESS,      FBE_TRACE_LEVEL_INFO },
    { FBE_LIBRARY_ID_FBE_API,               FBE_TRACE_LEVEL_INFO },
    { FBE_LIBRARY_ID_FBE_NEIT_API,          FBE_TRACE_LEVEL_INFO },
    { FBE_LIBRARY_ID_PMC_SHIM,              FBE_TRACE_LEVEL_INFO },
    { FBE_LIBRARY_ID_DRIVE_STAT,            FBE_TRACE_LEVEL_INFO },
    { FBE_LIBRARY_ID_DRIVE_SPARING,         FBE_TRACE_LEVEL_INFO},
    { FBE_LIBRARY_ID_FIS,                   FBE_TRACE_LEVEL_INFO },
    { FBE_LIBRARY_ID_SYSTEM_LIMITS,         FBE_TRACE_LEVEL_INFO },
    { FBE_LIBRARY_ID_RAID,                  FBE_TRACE_LEVEL_INFO },
};
static const fbe_u32_t fbe_trace_default_lib_trace_level_max = 
    sizeof(fbe_trace_default_lib_trace_level) / sizeof(fbe_trace_lib_trace_level_t);

static const char trace_error_stamp[] = {"<error>"};
static fbe_trace_err_set_notify_level_t notify_level;

static fbe_status_t
fbe_trace_control_packet_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;
    fbe_payload_control_status_t payload_status;
    fbe_payload_control_status_qualifier_t payload_status_qualifier;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get current control operation\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_payload_control_get_status(control_operation, &payload_status);
    fbe_payload_control_get_status_qualifier(control_operation, &payload_status_qualifier);
    fbe_payload_ex_release_control_operation(payload, control_operation);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get previous control operation\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_payload_control_set_status(control_operation, payload_status);
    fbe_payload_control_set_status_qualifier(control_operation, payload_status_qualifier);
    return FBE_STATUS_OK;
}

static fbe_status_t
fbe_trace_control_get_trace_flags(fbe_packet_t * packet,
                                fbe_payload_ex_t * payload,
                                fbe_trace_level_control_t * trace_flag_control)
{
    fbe_payload_control_operation_t * control_operation;
    fbe_object_id_t object_id;

    object_id = (fbe_object_id_t)trace_flag_control->fbe_id;

    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    if (control_operation == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get control operation\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_build_operation(control_operation,
                                        FBE_BASE_OBJECT_CONTROL_CODE_GET_TRACE_FLAG,
                                        &trace_flag_control->trace_flag,
                                        sizeof(fbe_trace_flags_t));

    fbe_transport_set_completion_function (packet, fbe_trace_control_packet_completion, NULL);
    fbe_transport_set_address(packet,
                              FBE_PACKAGE_ID_PHYSICAL,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id);
    fbe_service_manager_send_control_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t
fbe_trace_control_get_obj_level(fbe_packet_t * packet,
                                fbe_payload_ex_t * payload,
                                fbe_trace_level_control_t * trace_level_control)
{
    fbe_payload_control_operation_t * control_operation;
    fbe_object_id_t object_id;

    object_id = (fbe_object_id_t)trace_level_control->fbe_id;
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    if (control_operation == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get control operation\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_payload_control_build_operation(control_operation,
                                        FBE_BASE_OBJECT_CONTROL_CODE_GET_TRACE_LEVEL,
                                        &trace_level_control->trace_level,
                                        sizeof(fbe_trace_level_t));
    fbe_transport_set_completion_function (packet, fbe_trace_control_packet_completion, NULL);
    fbe_transport_set_address(packet,
                              packet->package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id);
    fbe_service_manager_send_control_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t
fbe_trace_control_get_srv_level(fbe_packet_t * packet,
                                fbe_payload_ex_t * payload,
                                fbe_trace_level_control_t * trace_level_control)
{
    fbe_payload_control_operation_t * control_operation;
    fbe_service_id_t service_id;

    service_id = (fbe_service_id_t)trace_level_control->fbe_id;
    if ((service_id <= FBE_SERVICE_ID_BASE_SERVICE) ||
        (service_id >= FBE_SERVICE_ID_LAST) ||
        (service_id == FBE_SERVICE_ID_TRACE)) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get trace level on service, id: %x\n", __FUNCTION__, service_id);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    if (control_operation == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get control operation\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_build_operation(control_operation,
                                        FBE_BASE_SERVICE_CONTROL_CODE_GET_TRACE_LEVEL,
                                        &trace_level_control->trace_level,
                                        sizeof(fbe_trace_level_t));
    
    fbe_transport_set_completion_function (packet, fbe_trace_control_packet_completion, NULL);
    fbe_transport_set_address(packet,
                              packet->package_id,
                              (fbe_service_id_t)trace_level_control->fbe_id,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID);
    fbe_service_manager_send_control_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t
fbe_trace_control_get_lib_level(fbe_packet_t * packet,
                                fbe_payload_ex_t * payload,
                                fbe_trace_level_control_t * trace_level_control)
{
    fbe_library_id_t library_id;
    fbe_trace_level_t trace_level;
    fbe_u32_t ii;

    library_id = (fbe_library_id_t)trace_level_control->fbe_id;
    if ((library_id == FBE_LIBRARY_ID_INVALID) || (library_id >= FBE_LIBRARY_ID_LAST)) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, invalid library, id: %x\n", __FUNCTION__, library_id);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    trace_level = fbe_trace_default_trace_level;
    for (ii = 0; ii < fbe_trace_default_lib_trace_level_max; ii++) {
        if (library_id == fbe_trace_default_lib_trace_level[ii].library_id) {
            trace_level = fbe_trace_default_lib_trace_level[ii].trace_level;
            break;
        }
    }
    trace_level_control->trace_level = trace_level;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t
fbe_trace_control_get_level(fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;
    fbe_trace_level_control_t * trace_level_control;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get packet payload\n", __FUNCTION__);

        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get control operation\n", __FUNCTION__);
        
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    status = fbe_payload_control_get_buffer(control_operation, (fbe_payload_control_buffer_t*)&trace_level_control);
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    buffer_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length); 
    if (buffer_length != sizeof(fbe_trace_level_control_t)) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get control operation buffer size\n", __FUNCTION__);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch(trace_level_control->trace_type) {
        case FBE_TRACE_TYPE_DEFAULT:
            trace_level_control->trace_level = fbe_trace_default_trace_level;
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);
            status = FBE_STATUS_OK;
            break;
        case FBE_TRACE_TYPE_OBJECT:
            status = fbe_trace_control_get_obj_level(packet, payload, trace_level_control);
            break;
        case FBE_TRACE_TYPE_SERVICE:
            status = fbe_trace_control_get_srv_level(packet, payload, trace_level_control);
            break;
        case FBE_TRACE_TYPE_LIB:
            status = fbe_trace_control_get_lib_level(packet, payload, trace_level_control);
            break;
        default:
            fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s, unknown trace type: %d\n", __FUNCTION__, trace_level_control->trace_type);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}

static fbe_status_t
fbe_trace_control_get_flags(fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;
    fbe_trace_level_control_t * trace_flag_control;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get packet payload\n", __FUNCTION__);

        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get control operation\n", __FUNCTION__);

        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    status = fbe_payload_control_get_buffer(control_operation, (fbe_payload_control_buffer_t*)&trace_flag_control);
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    buffer_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length); 
    if (buffer_length != sizeof(fbe_trace_level_control_t)) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get control operation buffer size\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch(trace_flag_control->trace_type) {
        case FBE_TRACE_TYPE_MGMT_ATTRIBUTE:
            status = fbe_trace_control_get_trace_flags(packet, payload, trace_flag_control);
            break;
        default:
            fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s, unknown trace flag: %d\n", __FUNCTION__, trace_flag_control->trace_flag);

            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}

static fbe_status_t
fbe_trace_control_set_trace_flags(fbe_packet_t * packet,
                                fbe_payload_ex_t * payload,
                                fbe_trace_level_control_t * trace_flag_control)
{
    fbe_payload_control_operation_t * control_operation;
    fbe_object_id_t object_id;
    fbe_package_id_t package_id;

    fbe_transport_get_package_id(packet, &package_id);

    object_id = (fbe_object_id_t)trace_flag_control->fbe_id;

    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    if (control_operation == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get operation\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_build_operation(control_operation,
                                        FBE_BASE_OBJECT_CONTROL_CODE_SET_TRACE_FLAG,
                                        &trace_flag_control->trace_flag,
                                        sizeof(fbe_trace_flags_t));

    fbe_transport_set_completion_function (packet, fbe_trace_control_packet_completion, NULL);
    fbe_transport_set_address(packet,
                              package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id);

    fbe_service_manager_send_control_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t
fbe_trace_control_set_obj_level(fbe_packet_t * packet,
                                fbe_payload_ex_t * payload,
                                fbe_trace_level_control_t * trace_level_control)
{
    fbe_payload_control_operation_t * control_operation;
    fbe_object_id_t object_id;

    object_id = (fbe_object_id_t)trace_level_control->fbe_id;
    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    if (control_operation == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get operation\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_build_operation(control_operation,
                                        FBE_BASE_OBJECT_CONTROL_CODE_SET_TRACE_LEVEL,
                                        &trace_level_control->trace_level,
                                        sizeof(fbe_trace_level_t));
    fbe_transport_set_completion_function (packet, fbe_trace_control_packet_completion, NULL);
    fbe_transport_set_address(packet,
                              packet->package_id,
                              FBE_SERVICE_ID_TOPOLOGY,
                              FBE_CLASS_ID_INVALID,
                              object_id);
    fbe_service_manager_send_control_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t
fbe_trace_control_set_srv_level(fbe_packet_t * packet,
                                fbe_payload_ex_t * payload,
                                fbe_trace_level_control_t * trace_level_control)
{
    fbe_payload_control_operation_t * control_operation;
    fbe_service_id_t service_id;

    service_id = (fbe_service_id_t)trace_level_control->fbe_id;
    if ((service_id <= FBE_SERVICE_ID_BASE_SERVICE) ||
        (service_id >= FBE_SERVICE_ID_LAST) ||
        (service_id == FBE_SERVICE_ID_TRACE)) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't set trace level on service, id: %x\n", __FUNCTION__, service_id);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_allocate_control_operation(payload);
    if (control_operation == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get operation\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_payload_control_build_operation(control_operation,
                                        FBE_BASE_SERVICE_CONTROL_CODE_SET_TRACE_LEVEL,
                                        &trace_level_control->trace_level,
                                        sizeof(fbe_trace_level_t));
    fbe_transport_set_completion_function (packet, fbe_trace_control_packet_completion, NULL);
    fbe_transport_set_address(packet,
                              packet->package_id,
                              service_id,
                              FBE_CLASS_ID_INVALID,
                              FBE_OBJECT_ID_INVALID);
    fbe_service_manager_send_control_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t
fbe_trace_control_set_lib_level(fbe_packet_t * packet,
                                fbe_payload_ex_t * payload,
                                fbe_trace_level_control_t * trace_level_control)
{
    fbe_library_id_t library_id;
    fbe_u32_t ii;

    library_id = (fbe_library_id_t)trace_level_control->fbe_id;
    if ((library_id == FBE_LIBRARY_ID_INVALID) || (library_id >= FBE_LIBRARY_ID_LAST)) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, invalid library, id: %x\n", __FUNCTION__, library_id);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (ii = 0; ii < fbe_trace_default_lib_trace_level_max; ii++) {
        if (library_id == fbe_trace_default_lib_trace_level[ii].library_id) {
            fbe_trace_default_lib_trace_level[ii].trace_level = trace_level_control->trace_level;

            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);
            return FBE_STATUS_OK;
        }
    }

    fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                           FBE_TRACE_LEVEL_ERROR,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s, can't set trace level for library, library_id: %d\n", __FUNCTION__, library_id);

    fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_GENERIC_FAILURE;
}

static fbe_status_t
fbe_trace_control_set_level(fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;
    fbe_trace_level_control_t * trace_level_control;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t buffer_length;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get packet payload\n", __FUNCTION__);

        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get control operation\n", __FUNCTION__);

        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    status = fbe_payload_control_get_buffer(control_operation, (fbe_payload_control_buffer_t*)&trace_level_control);
    if (status != FBE_STATUS_OK) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get control operation buffer\n", __FUNCTION__);

        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length); 
    if (buffer_length != sizeof(fbe_trace_level_control_t)) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get control operation buffer size\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if ((trace_level_control->trace_level == FBE_TRACE_LEVEL_INVALID) ||
        (trace_level_control->trace_level >= FBE_TRACE_LEVEL_LAST)) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, invalid trace level: %d\n", __FUNCTION__, trace_level_control->trace_level);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch(trace_level_control->trace_type) {
        case FBE_TRACE_TYPE_DEFAULT:
            fbe_trace_default_trace_level = trace_level_control->trace_level;
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);
            status = FBE_STATUS_OK;
            break;
        case FBE_TRACE_TYPE_OBJECT:
            status = fbe_trace_control_set_obj_level(packet, payload, trace_level_control);
            break;
        case FBE_TRACE_TYPE_SERVICE:
            status = fbe_trace_control_set_srv_level(packet, payload, trace_level_control);
            break;
        case FBE_TRACE_TYPE_LIB:
            status = fbe_trace_control_set_lib_level(packet, payload, trace_level_control);
            break;
        default:
            fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s, unknown trace type: %d\n", __FUNCTION__, trace_level_control->trace_type);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}

static fbe_status_t
fbe_trace_control_set_flags(fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;
    fbe_trace_level_control_t * trace_flag_control;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t buffer_length;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get packet payload\n", __FUNCTION__);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get control operation\n", __FUNCTION__);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    status = fbe_payload_control_get_buffer(control_operation, (fbe_payload_control_buffer_t*)&trace_flag_control);
    if (status != FBE_STATUS_OK) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get control operation buffer\n", __FUNCTION__);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if (buffer_length != sizeof(fbe_trace_level_control_t)) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get control operation buffer size\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch(trace_flag_control->trace_type) {
        case FBE_TRACE_TYPE_MGMT_ATTRIBUTE:
            status = fbe_trace_control_set_trace_flags(packet, payload, trace_flag_control);
            break;
        default:
            fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s, unknown trace flag: %d\n", __FUNCTION__, trace_flag_control->trace_flag);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }

    return status;
}

static fbe_status_t
fbe_trace_control_get_error_counters(fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;
    fbe_trace_error_counters_t * error_counters;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get packet payload\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get control operation\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_payload_control_get_buffer(control_operation, (fbe_payload_control_buffer_t*)&error_counters);
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    buffer_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length); 
    if (buffer_length != sizeof(fbe_trace_error_counters_t)) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get control operation buffer size\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    error_counters->trace_critical_error_counter = fbe_trace_get_critical_error_counter();
    error_counters->trace_error_counter = fbe_trace_get_error_counter();
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}    

static fbe_status_t
fbe_trace_control_clear_error_counters(fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get packet payload\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get control operation\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

	trace_critical_error_counter = 0;
	trace_error_counter = 0;

    fbe_base_service_trace((fbe_base_service_t*)&trace_service,
            FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_INFO,
			"%s: Done.\n", __FUNCTION__);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}    

static fbe_status_t 
fbe_trace_init_error_limits(void)
{
    fbe_u32_t index;
    for ( index = 0; index < FBE_TRACE_LEVEL_LAST; index++)
    {
        fbe_trace_current_error_limits[index].action = FBE_TRACE_ERROR_LIMIT_ACTION_INVALID;
        fbe_trace_current_error_limits[index].num_errors = 0;
    }
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_trace_set_error_limit(fbe_trace_error_limit_t * error_limit_p)
{
    if (error_limit_p->trace_level >= FBE_TRACE_LEVEL_LAST)
    {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s, passed in trace level %d invalid\n", 
                               __FUNCTION__, error_limit_p->trace_level);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (error_limit_p->action >= FBE_TRACE_ERROR_LIMIT_ACTION_LAST)
    {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s, passed in action %d invalid\n", 
                               __FUNCTION__, error_limit_p->action);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                           FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s, level: %d add action: 0x%x num_errs: 0x%x\n", 
                           __FUNCTION__, error_limit_p->trace_level, error_limit_p->action, error_limit_p->num_errors);
    fbe_trace_current_error_limits[error_limit_p->trace_level].action = error_limit_p->action;
    fbe_trace_current_error_limits[error_limit_p->trace_level].num_errors = error_limit_p->num_errors;
    return FBE_STATUS_OK;
}

fbe_status_t 
fbe_trace_get_error_limit(fbe_trace_get_error_limit_t * error_limit_p)
{
    fbe_u32_t level;

    /* Just return the records for the trace levels.
     */
    for ( level = 0; level < FBE_TRACE_LEVEL_LAST; level++)
    {
        error_limit_p->records[level].action = fbe_trace_current_error_limits[level].action;
        error_limit_p->records[level].error_limit = fbe_trace_current_error_limits[level].num_errors;

        /* The number of errors for error/critical error is held in global variables local to this file.
         */
        switch (level)
        {
            case FBE_TRACE_LEVEL_ERROR:
                error_limit_p->records[level].num_errors = trace_error_counter;
                break;
            case FBE_TRACE_LEVEL_CRITICAL_ERROR:
                error_limit_p->records[level].num_errors = trace_critical_error_counter;
                break;

            /* Currently trace limits are only supported for error and critical error levels. 
             */
            default:
                error_limit_p->records[level].num_errors = 0;
                break;
        };
    }
    return FBE_STATUS_OK;
}

static fbe_status_t
fbe_trace_handle_error_limits(fbe_trace_level_t trace_level, fbe_u32_t error_count) 
{
    fbe_trace_current_error_limits_t *error_limit_p = &fbe_trace_current_error_limits[trace_level];
    /* Only perform an action if this level has a valid limit and if the current
     * error count is at or above the number specified by the user. 
     */
    if ((error_limit_p->action != FBE_TRACE_ERROR_LIMIT_ACTION_INVALID) &&
        (error_count >= error_limit_p->num_errors))
    {
        if (error_limit_p->action == FBE_TRACE_ERROR_LIMIT_ACTION_TRACE)
        {
            fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s encountered error limit of %d errors for trace level %d\n", 
                                   __FUNCTION__, error_limit_p->num_errors, trace_level);
        }
        else if (error_limit_p->action == FBE_TRACE_ERROR_LIMIT_ACTION_STOP_SYSTEM)
        {
            /* The user has selected to stop the system when we hit this limit.
             * This should be the only place in the system that we panic. 
             */
            fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                                   FBE_TRACE_LEVEL_WARNING,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s hit error limit of %d errors for trace level %d stopping system\n", 
                                   __FUNCTION__, error_limit_p->num_errors, trace_level);
            fbe_trace_stop_system();
        }
    }
    return FBE_STATUS_OK;
}

static fbe_status_t
fbe_trace_control_set_error_limit(fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;
    fbe_trace_error_limit_t * error_limit_p = NULL;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get packet payload\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get control operation\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_payload_control_get_buffer(control_operation, (fbe_payload_control_buffer_t*)&error_limit_p);
    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    buffer_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length); 
    if (buffer_length != sizeof(fbe_trace_error_limit_t)) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get control operation buffer size\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_trace_set_error_limit(error_limit_p);

    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}
static fbe_status_t
fbe_trace_control_get_error_limit(fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;
    fbe_trace_get_error_limit_t * error_limit_p = NULL;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_status_t status;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get packet payload\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                "%s, can't get control operation\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_payload_control_get_buffer(control_operation, (fbe_payload_control_buffer_t*)&error_limit_p);
    if (status != FBE_STATUS_OK) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s, can't get buffer status: 0x%x\n", __FUNCTION__, status);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    buffer_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length); 
    if (buffer_length != sizeof(fbe_trace_get_error_limit_t)) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s, control operation buffer size %d != %llu\n", __FUNCTION__,
                               buffer_length, (unsigned long long)sizeof(fbe_trace_get_error_limit_t));
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_trace_get_error_limit(error_limit_p);

    if (status != FBE_STATUS_OK) {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t
fbe_trace_control_trace_err_set_notify_level(fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;
    fbe_trace_err_set_notify_level_t * notify_level_control;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t notify_level_length;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get packet payload\n", __FUNCTION__);

        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get control operation\n", __FUNCTION__);

        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    status = fbe_payload_control_get_buffer(control_operation, (fbe_payload_control_buffer_t*)&notify_level_control);
    if (notify_level_control == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get control operation buffer\n", __FUNCTION__);

        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    fbe_payload_control_get_buffer_length(control_operation, &notify_level_length);
    if (notify_level_length != sizeof(fbe_trace_err_set_notify_level_t)) {
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, control operation buffer length mismatch\n", __FUNCTION__);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    notify_level.level = notify_level_control->level;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return status;
   
}

fbe_status_t 
fbe_trace_control_entry(fbe_packet_t * packet)
{    
    fbe_payload_control_operation_opcode_t control_code;
    fbe_status_t status;

    control_code = fbe_transport_get_control_code(packet);
    if (control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) {
        fbe_trace_init();
        fbe_base_service_init((fbe_base_service_t*)&trace_service);
        fbe_base_service_set_trace_level((fbe_base_service_t *) &trace_service, FBE_TRACE_LEVEL_INFO);
        fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_OK;
    }

    if (fbe_base_service_is_initialized((fbe_base_service_t*)&trace_service) == FALSE) {
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    switch(control_code) {
        case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
            fbe_trace_destroy();
            fbe_base_service_destroy((fbe_base_service_t*)&trace_service);
            fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
            fbe_transport_complete_packet(packet);
            status = FBE_STATUS_OK;
            break;
        case FBE_TRACE_CONTROL_CODE_GET_LEVEL:
            status = fbe_trace_control_get_level(packet);
            break;
        case FBE_TRACE_CONTROL_CODE_SET_LEVEL:
            status = fbe_trace_control_set_level(packet);
            break;
        case FBE_TRACE_CONTROL_CODE_GET_TRACE_FLAG:
            status = fbe_trace_control_get_flags(packet);
            break;
        case FBE_TRACE_CONTROL_CODE_SET_TRACE_FLAG:
            status = fbe_trace_control_set_flags(packet);
            break;
        case FBE_TRACE_CONTROL_CODE_GET_ERROR_COUNTERS:
            status = fbe_trace_control_get_error_counters(packet);
            break;
        case FBE_TRACE_CONTROL_CODE_CLEAR_ERROR_COUNTERS:
            status = fbe_trace_control_clear_error_counters(packet);
            break;
        case FBE_TRACE_CONTROL_CODE_SET_ERROR_LIMIT:
            status = fbe_trace_control_set_error_limit(packet);
            break;
        case FBE_TRACE_CONTROL_CODE_GET_ERROR_LIMIT:
            status = fbe_trace_control_get_error_limit(packet);
            break;
        case FBE_TRACE_CONTROL_CODE_RESET_ERROR_COUNTERS:
            status = fbe_trace_control_reset_error_counters(packet);
            break;
        case FBE_TRACE_CONTROL_CODE_TRACE_ERR_SET_NOTIFY_LEVEL:
            status = fbe_trace_control_trace_err_set_notify_level(packet);
            break;
    case FBE_TRACE_CONTROL_CODE_SET_LIFECYCLE_DEBUG_TRACE_FLAG:
            status = fbe_lifecycle_debug_trace_control_set_flags(packet);
            break;
        case FBE_TRACE_CONTROL_CODE_ENABLE_BACKTRACE:
            status = fbe_trace_control_enable_backtrace(packet);
            break;
        case FBE_TRACE_CONTROL_CODE_DISABLE_BACKTRACE:
            status = fbe_trace_control_disable_backtrace(packet);
            break;
        case FBE_TRACE_CONTROL_CODE_COMMAND_TO_KTRACE_BUFF:
            status = fbe_trace_control_command_to_ktrace_buff(packet);
            break;
        default:
            fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                                   FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s, unknown trace control code: %X\n", __FUNCTION__, control_code);

            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            fbe_transport_complete_packet(packet);
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
    }
    return status;
}

static fbe_const_class_info_t class_info[] = {
    { "invalid_class_id",           FBE_CLASS_ID_INVALID },
    { "vertex",                     FBE_CLASS_ID_VERTEX },

    { "base_object",                FBE_CLASS_ID_BASE_OBJECT },
    { "base_discovered",            FBE_CLASS_ID_BASE_DISCOVERED },
    { "base_discovering",           FBE_CLASS_ID_BASE_DISCOVERING },

    { "board_first",                FBE_CLASS_ID_BOARD_FIRST },
    { "base_board",                 FBE_CLASS_ID_BASE_BOARD },
    { "hammerhead_board",           FBE_CLASS_ID_HAMMERHEAD_BOARD },
    { "sledgehammer_board",         FBE_CLASS_ID_SLEDGEHAMMER_BOARD },
    { "jackhammer_board",           FBE_CLASS_ID_JACKHAMMER_BOARD },
    { "boomslang_board",            FBE_CLASS_ID_BOOMSLANG_BOARD },
    { "dell_board",                 FBE_CLASS_ID_DELL_BOARD },
    { "fleet_board",                FBE_CLASS_ID_FLEET_BOARD },
    { "magnum_board",               FBE_CLASS_ID_MAGNUM_BOARD },
    { "armada_board",               FBE_CLASS_ID_ARMADA_BOARD },
    { "board_last",                 FBE_CLASS_ID_BOARD_LAST },

    { "port_first",                 FBE_CLASS_ID_PORT_FIRST },
    { "base_port",                  FBE_CLASS_ID_BASE_PORT },
    { "fibre_port",                 FBE_CLASS_ID_FIBRE_PORT },
    { "sas_port",                   FBE_CLASS_ID_SAS_PORT },
    { "fc_port",                    FBE_CLASS_ID_FC_PORT },
    { "iscsi_port",                 FBE_CLASS_ID_ISCSI_PORT },
    { "sas_lsi_port",               FBE_CLASS_ID_SAS_LSI_PORT },
    { "sas_cpd_port",               FBE_CLASS_ID_SAS_CPD_PORT },
    { "sas_pmc_port",               FBE_CLASS_ID_SAS_PMC_PORT },
    { "fc_pmc_port",                FBE_CLASS_ID_FC_PMC_PORT },
    { "fibre_xpe_port",             FBE_CLASS_ID_FIBRE_XPE_PORT },
    { "fibre_dpe_port",             FBE_CLASS_ID_FIBRE_DPE_PORT },
    { "port_last",                  FBE_CLASS_ID_PORT_LAST },

    { "lcc_first",                  FBE_CLASS_ID_LCC_FIRST },
    { "base_lcc",                   FBE_CLASS_ID_BASE_LCC },
    { "fibre_lcc",                  FBE_CLASS_ID_FIBRE_LCC },
    { "fibre_koto_lcc",             FBE_CLASS_ID_FIBRE_KOTO_LCC },
    { "fibre_yukon_lcc",            FBE_CLASS_ID_FIBRE_YUKON_LCC },
    { "fibre_broadsword_2G_lcc",    FBE_CLASS_ID_FIBRE_BROADSWORD_2G_LCC },
    { "fibre_bonesword_4G_lcc",     FBE_CLASS_ID_FIBRE_BONESWORD_4G_LCC },
    { "sas_voyager_ee_lcc",         FBE_CLASS_ID_SAS_VOYAGER_EE_LCC },
    { "sas_viking_drvsxp_lcc",      FBE_CLASS_ID_SAS_VIKING_DRVSXP_LCC },
    { "sas_cayenne_drvsxp_lcc",     FBE_CLASS_ID_SAS_CAYENNE_DRVSXP_LCC },
    { "sas_naga_drvsxp_lcc",        FBE_CLASS_ID_SAS_NAGA_DRVSXP_LCC },
    { "sas_lcc",                    FBE_CLASS_ID_SAS_LCC },
    { "sas_bullet_lcc",             FBE_CLASS_ID_SAS_BULLET_LCC },
    { "lcc_last",                   FBE_CLASS_ID_LCC_LAST },

    { "logical_drive_first",        FBE_CLASS_ID_LOGICAL_DRIVE_FIRST },
    { "base_logical_drive",         FBE_CLASS_ID_BASE_LOGICAL_DRIVE },
    { "logical_drive",              FBE_CLASS_ID_LOGICAL_DRIVE },
    { "logical_drive_last",         FBE_CLASS_ID_LOGICAL_DRIVE_LAST },
    
    { "physical_drive_first",       FBE_CLASS_ID_PHYSICAL_DRIVE_FIRST },
    { "base_physical_drive",        FBE_CLASS_ID_BASE_PHYSICAL_DRIVE },
    { "fibre_physical_drive_first", FBE_CLASS_ID_FIBRE_PHYSICAL_DRIVE_FIRST },
    { "fibre_physical_drive",       FBE_CLASS_ID_FIBRE_PHYSICAL_DRIVE },
    { "fibre_physical_drive_last",  FBE_CLASS_ID_FIBRE_PHYSICAL_DRIVE_LAST },
    { "sas_physical_drive_first",   FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_FIRST },
    { "sas_physical_drive",         FBE_CLASS_ID_SAS_PHYSICAL_DRIVE },
    { "sas_flash_drive",            FBE_CLASS_ID_SAS_FLASH_DRIVE },
    { "sata_paddlecard_drive",      FBE_CLASS_ID_SATA_PADDLECARD_DRIVE },
    { "sas_physical_drive_last",    FBE_CLASS_ID_SAS_PHYSICAL_DRIVE_LAST },
    { "sata_physical_drive_first",  FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_FIRST },
    { "sata_physical_drive",        FBE_CLASS_ID_SATA_PHYSICAL_DRIVE },
    { "sata_flash_drive",           FBE_CLASS_ID_SATA_FLASH_DRIVE },
    { "sata_physical_drive_last",   FBE_CLASS_ID_SATA_PHYSICAL_DRIVE_LAST },
    { "physical_drive_last",        FBE_CLASS_ID_PHYSICAL_DRIVE_LAST },

    { "enclosure_first",            FBE_CLASS_ID_ENCLOSURE_FIRST },
    { "base_enclosure",             FBE_CLASS_ID_BASE_ENCLOSURE },
    { "sas_enclosure_first",        FBE_CLASS_ID_SAS_ENCLOSURE_FIRST },
    { "sas_enclosure",              FBE_CLASS_ID_SAS_ENCLOSURE },
    { "eses_enclosure",             FBE_CLASS_ID_ESES_ENCLOSURE },
    { "sas_bullet_enclosure",       FBE_CLASS_ID_SAS_BULLET_ENCLOSURE },
    { "sas_viper_enclosure",        FBE_CLASS_ID_SAS_VIPER_ENCLOSURE },
    { "sas_magnum_enclosure",       FBE_CLASS_ID_SAS_MAGNUM_ENCLOSURE },
    { "sas_citadel_enclosure",      FBE_CLASS_ID_SAS_CITADEL_ENCLOSURE },
    { "sas_bunker_enclosure",       FBE_CLASS_ID_SAS_BUNKER_ENCLOSURE },    
    { "sas_derringer_enclosure",    FBE_CLASS_ID_SAS_DERRINGER_ENCLOSURE },
    { "sas_voyager_icm_enclosure",  FBE_CLASS_ID_SAS_VOYAGER_ICM_ENCLOSURE },
    { "sas_viking_iosxp_enclosure", FBE_CLASS_ID_SAS_VIKING_IOSXP_ENCLOSURE },
    { "sas_cayenne_iosxp_enclosure", FBE_CLASS_ID_SAS_CAYENNE_IOSXP_ENCLOSURE },
    { "sas_naga_iosxp_enclosure",   FBE_CLASS_ID_SAS_NAGA_IOSXP_ENCLOSURE },
    { "sas_fallback_enclosure",     FBE_CLASS_ID_SAS_FALLBACK_ENCLOSURE },    
    { "sas_boxwood_enclosure",      FBE_CLASS_ID_SAS_BOXWOOD_ENCLOSURE },    
    { "sas_knot_enclosure",         FBE_CLASS_ID_SAS_KNOT_ENCLOSURE },     
    { "sas_ancho_enclosure",        FBE_CLASS_ID_SAS_ANCHO_ENCLOSURE,},
    { "sas_pinecone_enclosure",     FBE_CLASS_ID_SAS_PINECONE_ENCLOSURE },    
    { "sas_steeljaw_enclosure",     FBE_CLASS_ID_SAS_STEELJAW_ENCLOSURE },    
    { "sas_ramhorn_enclosure",      FBE_CLASS_ID_SAS_RAMHORN_ENCLOSURE },    
    { "sas_rhea_enclosure",         FBE_CLASS_ID_SAS_RHEA_ENCLOSURE },    
    { "sas_miranda_enclosure",      FBE_CLASS_ID_SAS_MIRANDA_ENCLOSURE },    
    { "sas_calypso_enclosure",      FBE_CLASS_ID_SAS_CALYPSO_ENCLOSURE },    
    { "sas_tabasco_enclosure",      FBE_CLASS_ID_SAS_TABASCO_ENCLOSURE },
    { "sas_enclosure_last",         FBE_CLASS_ID_SAS_ENCLOSURE_LAST },
    { "enclosure_last",             FBE_CLASS_ID_ENCLOSURE_LAST },
    { "base_config",                FBE_CLASS_ID_BASE_CONFIG },
    { "raid_group",                 FBE_CLASS_ID_RAID_GROUP },
    { "mirror",                     FBE_CLASS_ID_MIRROR },
    { "striper",                    FBE_CLASS_ID_STRIPER },
    { "parity",                     FBE_CLASS_ID_PARITY },
    { "virtual_drive",              FBE_CLASS_ID_VIRTUAL_DRIVE },
    { "provisioned_drive",          FBE_CLASS_ID_PROVISION_DRIVE },
    { "lun",                        FBE_CLASS_ID_LUN },
    { "bvd_interface",              FBE_CLASS_ID_BVD_INTERFACE },
    { "bvd_interface",              FBE_CLASS_ID_BVD_INTERFACE },
    { "extent_pool",                FBE_CLASS_ID_EXTENT_POOL },
    { "ext_pool_lun",               FBE_CLASS_ID_EXTENT_POOL_LUN },
    { "ext_pool_metadata_lun",      FBE_CLASS_ID_EXTENT_POOL_METADATA_LUN },
    { "base_environment",           FBE_CLASS_ID_BASE_ENVIRONMENT },
    { "encl_mgmt",                  FBE_CLASS_ID_ENCL_MGMT},
    { "sps_mgmt",                   FBE_CLASS_ID_SPS_MGMT},
    { "drive_mgmt",                 FBE_CLASS_ID_DRIVE_MGMT},
    { "module_mgmt",                FBE_CLASS_ID_MODULE_MGMT},
    { "ps_mgmt",                    FBE_CLASS_ID_PS_MGMT},
    { "board_mgmt",                 FBE_CLASS_ID_BOARD_MGMT},
    { "cooling_mgmt",               FBE_CLASS_ID_COOLING_MGMT},
    { "last_class_id",              FBE_CLASS_ID_LAST }
};

static const fbe_u32_t class_info_max = sizeof(class_info)/sizeof(fbe_const_class_info_t);

fbe_status_t
fbe_get_class_by_name(const char * class_name, fbe_const_class_info_t ** pp_class_info)
{
    const char *n1, *n2;
    fbe_u32_t ii;

    if (pp_class_info == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *pp_class_info = NULL;

    if (class_name == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (ii = 0; ii < class_info_max; ii++ ) {
        for (n1 = class_name, n2 = class_info[ii].class_name; ((*n1 == *n2) && (*n1 != 0)); n1++, n2++) ;
        if (*n1 == *n2) {
            *pp_class_info = &class_info[ii];
            return FBE_STATUS_OK;
        }
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t
fbe_get_class_by_id(fbe_class_id_t class_id, fbe_const_class_info_t ** pp_class_info)
{
    fbe_u32_t i;

    if (pp_class_info == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *pp_class_info = NULL;

    if (class_id >= FBE_CLASS_ID_LAST) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Loop through and get the pointer to the class info */
    for (i = 0; i < class_info_max; i++ ) {
        if (class_id == class_info[i].class_id) {
            *pp_class_info = &class_info[i];
            return FBE_STATUS_OK;
        }
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

static fbe_const_service_info_t service_info[] = {
    { "first_service",            FBE_SERVICE_ID_FIRST },
    { "base_service",             FBE_SERVICE_ID_BASE_SERVICE },
    { "memory",                   FBE_SERVICE_ID_MEMORY },
    { "topology",                 FBE_SERVICE_ID_TOPOLOGY },
    { "notification",             FBE_SERVICE_ID_NOTIFICATION },
    { "simulator",                FBE_SERVICE_ID_SIMULATOR },
    { "service_manager",          FBE_SERVICE_ID_SERVICE_MANAGER },
    { "scheduler",                FBE_SERVICE_ID_SCHEDULER },
    { "terminator",               FBE_SERVICE_ID_TERMINATOR },
    { "transport",                FBE_SERVICE_ID_TRANSPORT },
    { "event_log",                FBE_SERVICE_ID_EVENT_LOG },
    { "drive_config",             FBE_SERVICE_ID_DRIVE_CONFIGURATION },
    { "event",                    FBE_SERVICE_ID_EVENT },
    { "logical_error_service",    FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION },
    { "cmi",                      FBE_SERVICE_ID_CMI },
    { "job_service",              FBE_SERVICE_ID_JOB_SERVICE },
    { "environment_limit",        FBE_SERVICE_ID_ENVIRONMENT_LIMIT },
    { "database",    			  FBE_SERVICE_ID_DATABASE },
    { "clustered_memory",    	  FBE_SERVICE_ID_CMS },
    { "persist",    			  FBE_SERVICE_ID_PERSIST },
    { "last_service_id",          FBE_SERVICE_ID_LAST }
};

static const fbe_u32_t service_info_max = sizeof(service_info)/sizeof(fbe_const_service_info_t);

fbe_status_t
fbe_get_service_by_name(const char * service_name, fbe_const_service_info_t ** pp_service_info)
{
    const char *n1, *n2;
    fbe_u32_t ii;

    if (pp_service_info == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *pp_service_info = NULL;

    if (service_name == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (ii = 0; ii < service_info_max; ii++ ) {
        for (n1 = service_name, n2 = service_info[ii].service_name; ((*n1 == *n2) && (*n1 != 0)); n1++, n2++) ;
        if (*n1 == *n2) {
            *pp_service_info = &service_info[ii];
            return FBE_STATUS_OK;
        }
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t
fbe_get_service_by_id(fbe_service_id_t service_id, fbe_const_service_info_t ** pp_service_info)
{
    fbe_u32_t ii;

    if (pp_service_info == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *pp_service_info = NULL;

    if (service_id >= FBE_SERVICE_ID_LAST) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    ii = service_id & FBE_SERVICE_ID_MASK;
    if (service_id == service_info[ii].service_id) {
        *pp_service_info = &service_info[ii];
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_GENERIC_FAILURE;
}


static fbe_const_library_info_t library_info[] = {
    { "invalid_library_id",       FBE_LIBRARY_ID_INVALID },
    { "lifecycle",                FBE_LIBRARY_ID_LIFECYCLE },
    { "flare_shim",               FBE_LIBRARY_ID_FLARE_SHIM },
    { "enclosure_data_access",    FBE_LIBRARY_ID_ENCL_DATA_ACCESS },
    { "fbe_api",                  FBE_LIBRARY_ID_FBE_API },
    { "neit_api",                 FBE_LIBRARY_ID_FBE_NEIT_API },
    { "pmc_shim",                 FBE_LIBRARY_ID_PMC_SHIM} ,    
    { "drv_stats",                FBE_LIBRARY_ID_DRIVE_STAT } ,
    { "xor",				      FBE_LIBRARY_ID_XOR} ,		
    { "raid",				      FBE_LIBRARY_ID_RAID} ,	
    { "spare",                    FBE_LIBRARY_ID_DRIVE_SPARING},
    { "pmc_shim",                 FBE_LIBRARY_ID_PMC_SHIM} ,    
    { "drv_stats",                FBE_LIBRARY_ID_DRIVE_STAT} ,
    { "xor",				      FBE_LIBRARY_ID_XOR} ,		
    { "raid",				      FBE_LIBRARY_ID_RAID} ,	
    { "spare",                    FBE_LIBRARY_ID_DRIVE_SPARING},
    { "drv_stats",                FBE_LIBRARY_ID_DRIVE_STAT } ,
    { "fis",                      FBE_LIBRARY_ID_FIS },
    { "system_limits",            FBE_LIBRARY_ID_SYSTEM_LIMITS },
    { "packet_serialize",         FBE_LIBRARY_ID_PACKET_SERIALIZE },
    { "last_library_id",          FBE_LIBRARY_ID_LAST },
};

static const fbe_u32_t library_info_max = sizeof(library_info)/sizeof(fbe_const_library_info_t);

fbe_status_t
fbe_get_library_by_name(const char * library_name, fbe_const_library_info_t ** pp_library_info)
{
    const char *n1, *n2;
    fbe_u32_t ii;

    if (pp_library_info == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *pp_library_info = NULL;

    if (library_name == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for (ii = 0; ii < library_info_max; ii++ ) {
        for (n1 = library_name, n2 = library_info[ii].library_name; ((*n1 == *n2) && (*n1 != 0)); n1++, n2++) ;
        if (*n1 == *n2) {
            *pp_library_info = &library_info[ii];
            return FBE_STATUS_OK;
        }
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t
fbe_get_library_by_id(fbe_library_id_t library_id, fbe_const_library_info_t ** pp_library_info)
{
    if (pp_library_info == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *pp_library_info = NULL;

    if (library_id >= FBE_LIBRARY_ID_LAST) { 
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (library_id == library_info[library_id].library_id) {
        *pp_library_info = &library_info[library_id];
        return FBE_STATUS_OK;
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

fbe_status_t 
fbe_trace_init(void)
{    
    trace_critical_error_counter = 0;
    trace_error_counter = 0;

    /* Need to initialize fbe ktrace infrastructure as required
     */
    if (fbe_ktrace_is_initialized() == FBE_FALSE)
    {
        fbe_ktrace_initialize();
    }

    fbe_trace_init_error_limits();
    notify_level.level = FBE_TRACE_LEVEL_INVALID;

    return FBE_STATUS_OK;
}    

fbe_status_t 
fbe_trace_destroy(void)
{
    /* Must destroy the fbe ktrace also */
    fbe_ktrace_destroy();
    return FBE_STATUS_OK;
}

void 
fbe_trace_set_default_trace_level(fbe_trace_level_t default_trace_level)
{
    fbe_trace_default_trace_level = default_trace_level;
}

#if 0
fbe_trace_level_t 
fbe_trace_get_default_trace_level(void)
{
    return fbe_trace_default_trace_level;
}
#endif

fbe_u32_t 
fbe_trace_get_critical_error_counter(void)
{
    return trace_critical_error_counter;
}

fbe_u32_t 
fbe_trace_get_error_counter(void)
{
    return trace_error_counter;
}

const char *
fbe_trace_get_level_stamp(fbe_trace_level_t trace_level)
{
    const char * stamp;

    stamp = trace_error_stamp;
    switch(trace_level){
        case FBE_TRACE_LEVEL_CRITICAL_ERROR:
            stamp = "CERR";
            break;
        case FBE_TRACE_LEVEL_ERROR:
            stamp = "ERR ";
            break;
        case FBE_TRACE_LEVEL_WARNING:
            stamp = "WARN";
            break;
        case FBE_TRACE_LEVEL_INFO:
            stamp = "INFO";
            break;
        case FBE_TRACE_LEVEL_DEBUG_LOW:
            stamp = "DBG1";
            break;
        case FBE_TRACE_LEVEL_DEBUG_MEDIUM:
            stamp = "DBG2";
            break;
        case FBE_TRACE_LEVEL_DEBUG_HIGH:
            stamp = "DBG3";
            break;
    }
    return stamp;
}

const char *
fbe_trace_get_component_type_stamp(fbe_u32_t component_type)
{
    const char * stamp;

    stamp = trace_error_stamp;
    switch (component_type) {
        case FBE_COMPONENT_TYPE_PACKAGE:
            stamp = "PKGE";
            break;
        case FBE_COMPONENT_TYPE_SERVICE:
            stamp = "SERV";
            break;
        case FBE_COMPONENT_TYPE_CLASS:
            stamp = "CLAS";
            break;
        case FBE_COMPONENT_TYPE_OBJECT:
            stamp = "OBJ ";
            break;
        case FBE_COMPONENT_TYPE_LIBRARY:
            stamp = "LIB ";
            break;
    }
    return stamp;
}

const char *
fbe_trace_get_package_id_stamp(fbe_u32_t component_id)
{
    const char * stamp;

    stamp = trace_error_stamp;
    switch (component_id) {
        case FBE_PACKAGE_ID_PHYSICAL:
            stamp = "PHYS";
            break;
        case FBE_PACKAGE_ID_SEP_0:
            stamp = "SEP0";
            break;
        case FBE_PACKAGE_ID_ESP:
            stamp = "ESP";
            break;
        case FBE_PACKAGE_ID_NEIT:
            stamp = "NEIT";
            break;
        case FBE_PACKAGE_ID_KMS:
            stamp = "KMS";
            break;
    }
    return stamp;
}

const char *
fbe_trace_get_service_id_stamp(fbe_u32_t component_id)
{
    const char * stamp;

    stamp = trace_error_stamp;
    switch (component_id) {
        case FBE_SERVICE_ID_BASE_SERVICE:
            stamp = "BASE";
            break;
        case FBE_SERVICE_ID_MEMORY:
            stamp = "MEM ";
            break;
        case FBE_SERVICE_ID_TOPOLOGY:
            stamp = "TOPO";
            break;
        case FBE_SERVICE_ID_NOTIFICATION:
            stamp = "NTFY";
            break;
        case FBE_SERVICE_ID_SIMULATOR:
            stamp = "SIMU";
            break;
        case FBE_SERVICE_ID_SERVICE_MANAGER:
            stamp = "MNGR";
            break;
        case FBE_SERVICE_ID_SCHEDULER:
            stamp = "SCHD";
            break;
        case FBE_SERVICE_ID_TERMINATOR:
            stamp = "TERM";
            break;
        case FBE_SERVICE_ID_TRANSPORT:
            stamp = "TRAN";
            break;
        case FBE_SERVICE_ID_EVENT_LOG:
            stamp = "ELOG";
            break;
        case FBE_SERVICE_ID_ENVIRONMENT_LIMIT:
            stamp = "ENVLIM";
            break;
        case FBE_SERVICE_ID_DRIVE_CONFIGURATION:
            stamp = "DCFG";
            break;
        case FBE_SERVICE_ID_RDGEN:
            stamp = "RDGN";
            break;
        case FBE_SERVICE_ID_TRACE:
            stamp = "TRAC";
            break;
        case FBE_SERVICE_ID_TRAFFIC_TRACE:
            stamp = "TRAF";
            break;
        case FBE_SERVICE_ID_SECTOR_TRACE:
            stamp = "SECT";
            break;
        case FBE_SERVICE_ID_SIM_SERVER:
            stamp = "SSRV";
            break;
        case FBE_SERVICE_ID_METADATA:
            stamp = "META";
            break;
        case FBE_SERVICE_ID_JOB_SERVICE:
            stamp = "JOB ";
            break;
        case FBE_SERVICE_ID_EVENT:
            stamp = "EVNT";
            break;
        case FBE_SERVICE_ID_PERSIST:
            stamp = "PERSIT";	
            break;
        case FBE_SERVICE_ID_PROTOCOL_ERROR_INJECTION:
            stamp = "PERRIN";
            break;
        case FBE_SERVICE_ID_LOGICAL_ERROR_INJECTION:
            stamp = "LERRIN";
            break;
        case FBE_SERVICE_ID_CMI:
            stamp = "CMI ";	
            break;
        case FBE_SERVICE_ID_EIR:
            stamp = "EIR ";	
            break;
        case FBE_SERVICE_ID_USER_SERVER:
            stamp = "USRV";	
            break;
        case FBE_SERVICE_ID_DATABASE:
            stamp = "DBS ";	
            break;
        case FBE_SERVICE_ID_CMS:
            stamp = "CMS";	
            break;
        case FBE_SERVICE_ID_RAW_MIRROR:
            stamp = "RAWMIR";	
            break;
        case FBE_SERVICE_ID_CMS_EXERCISER:
            stamp = "CMSEX";   
            break;
        case FBE_SERVICE_ID_DEST:
            stamp = "DEST";   
            break;
        case FBE_SERVICE_ID_PERFSTATS:
		    stamp = "PSTATS";   
		    break;
        case FBE_SERVICE_ID_ENVSTATS:
		    stamp = "ESTATS";   
		    break;
        default:
            break;
        

    }
    return stamp;
}

const char *
fbe_trace_get_library_id_stamp(fbe_u32_t component_id)
{
    const char * stamp;

    stamp = trace_error_stamp;
    switch (component_id) {
        case FBE_LIBRARY_ID_LIFECYCLE:
            stamp = "LIFE";
            break;
        case FBE_LIBRARY_ID_FLARE_SHIM:
            stamp = "SHIM";
            break;
        case FBE_LIBRARY_ID_ENCL_DATA_ACCESS:
            stamp = "E_DA";
            break;
        case FBE_LIBRARY_ID_FBE_API:
            stamp = "FAPI";
            break;
        case FBE_LIBRARY_ID_FBE_NEIT_API:
            stamp = "FNEIT";
            break;
        case FBE_LIBRARY_ID_PMC_SHIM:
            stamp = "PMCS";
            break;
        case FBE_LIBRARY_ID_DRIVE_STAT:
            stamp = "DSTATS";
            break;
        case FBE_LIBRARY_ID_CPD_SHIM:
            stamp = "CPD";
            break;
        case FBE_LIBRARY_ID_XOR:
            stamp = "XOR";
            break;
        case FBE_LIBRARY_ID_RAID:
            stamp = "RAID";
            break;
        case FBE_LIBRARY_ID_DRIVE_SPARING:
            stamp = "SPARE";
            break;
        case FBE_LIBRARY_ID_FIS:
            stamp = "FIS";
            break;
        case FBE_LIBRARY_ID_SYSTEM_LIMITS:
            stamp = "SYSL";
            break;
        case FBE_LIBRARY_ID_PACKET_SERIALIZE:
            stamp = "PKTSER";
            break;
        case FBE_LIBRARY_ID_MULTICORE_QUEUE:
            stamp = "MCQ";
            break;
    }
    return stamp;
}

#define FBE_TRACE_U32_STAMP_SIZE 9
#define FBE_TRACE_STAMP_SIZE ((FBE_TRACE_U32_STAMP_SIZE * 4) + 32)
#define FBE_TRACE_MSG_SIZE 256

void fbe_trace_backtrace_handler(csx_rt_proc_backtrace_context_t backtrace_context, csx_cstring_t fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    fbe_KvTraceRingArgList(FBE_TRACE_RING_DEFAULT, fmt, args);
    va_end(args);
}
void fbe_trace_backtrace(void)
{
    csx_rt_proc_request_backtrace(fbe_trace_backtrace_handler, NULL);
}
static void 
trace_to_ring(fbe_trace_ring_t trace_ring,
              fbe_u32_t component_type,
              fbe_u32_t component_id,
              fbe_trace_level_t trace_level,
              fbe_u32_t message_id,
              const fbe_u8_t * fmt, 
              va_list argList)
{
    /* This is a big stack frame!
     * However, the alternative requires a lock on static buffers.
     * Pick your poison. */

    fbe_bool_t needs_panic = FBE_FALSE;
    fbe_u8_t stamp[FBE_TRACE_STAMP_SIZE];
    fbe_u8_t msg[FBE_TRACE_MSG_SIZE];

    fbe_u8_t trace_level_stamp[FBE_TRACE_U32_STAMP_SIZE];
    fbe_u8_t component_type_stamp[FBE_TRACE_U32_STAMP_SIZE];
    fbe_u8_t component_id_stamp[FBE_TRACE_U32_STAMP_SIZE];
    fbe_package_id_t package_id;
    const fbe_u8_t * p_component_type_stamp;
    const fbe_u8_t * p_component_id_stamp;
    const fbe_u8_t * p_trace_level_stamp;

    fbe_const_class_info_t * p_class_info;
    fbe_status_t status;

    fbe_u32_t indexS = 0;
    fbe_u32_t indexM = 0;
    fbe_notification_info_t notification_info;

    switch(trace_level){
        case FBE_TRACE_LEVEL_CRITICAL_ERROR:
            trace_critical_error_counter++;
            if ((fbe_trace_current_error_limits[trace_level].action == FBE_TRACE_ERROR_LIMIT_ACTION_INVALID) ||
                (fbe_trace_current_error_limits[trace_level].action == FBE_TRACE_ERROR_LIMIT_ACTION_STOP_SYSTEM))
            {
                needs_panic = FBE_TRUE;
            }
            fbe_get_package_id(&package_id);
            if (fbe_trace_backtrace_enabled == FBE_TRUE){
                csx_rt_proc_request_backtrace(fbe_trace_backtrace_handler, NULL);
            }
            fbe_trace_handle_error_limits(FBE_TRACE_LEVEL_CRITICAL_ERROR, trace_critical_error_counter);
            break;
        case FBE_TRACE_LEVEL_ERROR:
            trace_error_counter++;
            fbe_trace_handle_error_limits(FBE_TRACE_LEVEL_ERROR, trace_error_counter);
            if (fbe_trace_backtrace_enabled == FBE_TRUE) {
                csx_rt_proc_request_backtrace(fbe_trace_backtrace_handler, NULL);
            }
            break;
        default:
            break;
    }

    p_trace_level_stamp = p_component_type_stamp = p_component_id_stamp = trace_error_stamp;

    p_trace_level_stamp = fbe_trace_get_level_stamp(trace_level);
    p_component_type_stamp = fbe_trace_get_component_type_stamp(component_type);

    switch (component_type) {
        case FBE_COMPONENT_TYPE_PACKAGE:
            p_component_id_stamp = fbe_trace_get_package_id_stamp(component_id);
            break;
        case FBE_COMPONENT_TYPE_SERVICE:
            p_component_id_stamp = fbe_trace_get_service_id_stamp(component_id);
            break;
        case FBE_COMPONENT_TYPE_CLASS:
            status = fbe_get_class_by_id((fbe_class_id_t)component_id, &p_class_info);
            if (status == FBE_STATUS_OK) {
                p_component_id_stamp = p_class_info->class_name;
            } else {
                p_component_id_stamp = trace_error_stamp;
            }
            break;
        case FBE_COMPONENT_TYPE_OBJECT:
            break;
        case FBE_COMPONENT_TYPE_LIBRARY:
            p_component_id_stamp = fbe_trace_get_library_id_stamp(component_id);
            break;
    }

    if (p_trace_level_stamp == (fbe_u8_t *)trace_error_stamp) {
        fbe_zero_memory(trace_level_stamp, FBE_TRACE_U32_STAMP_SIZE);
        csx_p_snprintf(trace_level_stamp, FBE_TRACE_U32_STAMP_SIZE, "%4X", trace_level);
        p_trace_level_stamp = trace_level_stamp;
    }

    if (p_component_type_stamp == (fbe_u8_t *)trace_error_stamp) {
        fbe_zero_memory(component_type_stamp, FBE_TRACE_U32_STAMP_SIZE);
        csx_p_snprintf(component_type_stamp, FBE_TRACE_U32_STAMP_SIZE, "%4X", component_type);
        p_component_type_stamp = component_type_stamp;
    }

    if (p_component_id_stamp == (fbe_u8_t *)trace_error_stamp) {
        fbe_zero_memory(component_id_stamp, FBE_TRACE_U32_STAMP_SIZE);
        csx_p_snprintf(component_id_stamp, FBE_TRACE_U32_STAMP_SIZE, "%4X", component_id);
        p_component_id_stamp = component_id_stamp;
    }

    fbe_zero_memory(stamp, FBE_TRACE_STAMP_SIZE);
    csx_p_snprintf(stamp, FBE_TRACE_STAMP_SIZE, "%s %s %s %6X :",
                       p_trace_level_stamp, p_component_type_stamp, p_component_id_stamp, message_id);

    fbe_zero_memory(msg, FBE_TRACE_MSG_SIZE);
    csx_p_vsnprintf(msg, FBE_TRACE_MSG_SIZE, fmt, argList);

    //Create a notification if notify_level equal to trace_level
    if (trace_level <= notify_level.level)
    {
        notification_info.notification_type = FBE_NOTIFICATION_TYPE_FBE_ERROR_TRACE;
        notification_info.class_id = FBE_OBJECT_ID_INVALID;
        notification_info.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
        
        for (indexS = 0; indexS < FBE_TRACE_STAMP_SIZE && (stamp[indexS] != (fbe_char_t)0); indexS++)
        {
            notification_info.notification_data.error_trace_info.bytes[indexS] = stamp[indexS];
        }

        for (indexM = 0; indexM < FBE_TRACE_MSG_SIZE && (msg[indexM] != (fbe_char_t)0); indexM++)
        {
            notification_info.notification_data.error_trace_info.bytes[indexM+indexS] = msg[indexM];
        }
        notification_info.notification_data.error_trace_info.bytes[indexM+indexS] = '\0';
        fbe_notification_send(FBE_OBJECT_ID_INVALID, notification_info);
    }

    switch(trace_ring) {
        case FBE_TRACE_RING_DEFAULT:
            fbe_KvTrace("%s %s", stamp, msg);
            break;
        case FBE_TRACE_RING_STARTUP:
            fbe_KvTraceStart("%s %s", stamp, msg);
            break;
        case FBE_TRACE_RING_TRAFFIC:
            fbe_KvTraceRing(FBE_TRACE_RING_TRAFFIC, "%s %s", stamp, msg);
            break;
        default:
            break;
    }

    /* If needs_panic is SET, which means we have CRITICAL ERROR. Panic in that case. */
    if(needs_panic)
    {
        /* Panic on CRITICAL ERRORs */
        FBE_PANIC_SP(FBE_PANIC_SP_BASE, FBE_STATUS_OK);
    }
}

void 
fbe_trace_at_startup(fbe_u32_t component_type,
                     fbe_u32_t component_id,
                     fbe_trace_level_t trace_level,
                     fbe_u32_t message_id,
                     const fbe_u8_t * fmt, 
                     va_list argList)
{
    trace_to_ring(FBE_TRACE_RING_STARTUP,
                  component_type,
                  component_id,
                  trace_level,
                  message_id,
                  fmt,
                  argList);
}

void 
fbe_trace_report_to_traffic(fbe_u32_t component_type,
                 fbe_u32_t component_id,
                 fbe_trace_level_t trace_level,
                 fbe_u32_t message_id,
                 const fbe_u8_t * fmt, 
                 va_list argList)
{
    trace_to_ring(FBE_TRACE_RING_TRAFFIC,
                  component_type,
                  component_id,
                  trace_level,
                  message_id,
                  fmt,
                  argList);
}

void 
fbe_trace_report(fbe_u32_t component_type,
                 fbe_u32_t component_id,
                 fbe_trace_level_t trace_level,
                 fbe_u32_t message_id,
                 const fbe_u8_t * fmt, 
                 va_list argList)
{
    trace_to_ring(FBE_TRACE_RING_DEFAULT,
                  component_type,
                  component_id,
                  trace_level,
                  message_id,
                  fmt,
                  argList);
}
void 
fbe_trace_report_w_header(fbe_u32_t component_type,
                 fbe_u32_t component_id,
                 fbe_trace_level_t trace_level,
                 fbe_u8_t * header_string,
                 const fbe_u8_t * fmt, 
                 va_list argList)
{   

    fbe_u8_t stamp[FBE_TRACE_STAMP_SIZE];
    fbe_u8_t msg[FBE_TRACE_MSG_SIZE];
    fbe_u8_t trace_level_stamp[FBE_TRACE_U32_STAMP_SIZE];   
    const fbe_u8_t * p_trace_level_stamp; 
    fbe_u8_t component_type_stamp[FBE_TRACE_U32_STAMP_SIZE];
    fbe_u8_t component_id_stamp[FBE_TRACE_U32_STAMP_SIZE];
    const fbe_u8_t * p_component_type_stamp = NULL;
    const fbe_u8_t * p_component_id_stamp = NULL;
    fbe_const_class_info_t * p_class_info;
    fbe_status_t status;

    p_trace_level_stamp =  NULL;



    switch(trace_level)
    {
        case FBE_TRACE_LEVEL_CRITICAL_ERROR:
            p_trace_level_stamp = "CERR";
        break;
        case FBE_TRACE_LEVEL_ERROR:
            p_trace_level_stamp = "ERR ";
        break;
        case FBE_TRACE_LEVEL_WARNING:
            p_trace_level_stamp = "WARN";
        break;
        case FBE_TRACE_LEVEL_INFO:
            p_trace_level_stamp = "INFO";
        break;
        case FBE_TRACE_LEVEL_DEBUG_LOW:
            p_trace_level_stamp = "DBG1";
        break;
        case FBE_TRACE_LEVEL_DEBUG_MEDIUM:
            p_trace_level_stamp = "DBG2";
        break;
        case FBE_TRACE_LEVEL_DEBUG_HIGH:
            p_trace_level_stamp = "DBG3";
        break;
    }

    switch (component_type) {
        case FBE_COMPONENT_TYPE_PACKAGE:
            p_component_type_stamp = "PKGE";
            switch (component_id) {
                case FBE_PACKAGE_ID_PHYSICAL:
                    p_component_id_stamp = "PHYS";
                break;
                case FBE_PACKAGE_ID_SEP_0:
                    p_component_id_stamp = "SEP0";
                break;
                case FBE_PACKAGE_ID_ESP:
                    p_component_id_stamp = "ESP";
                break;
                case FBE_PACKAGE_ID_NEIT:
                    p_component_id_stamp = "NEIT";
                break;
                case FBE_PACKAGE_ID_KMS:
                    p_component_id_stamp = "KMS";
                break;
            }
        break;
        case FBE_COMPONENT_TYPE_SERVICE:
            p_component_type_stamp = "SERV";
            switch (component_id) {
                case FBE_SERVICE_ID_BASE_SERVICE:
                    p_component_id_stamp = "BASE";
                break;
                case FBE_SERVICE_ID_MEMORY:
                    p_component_id_stamp = "MEM ";
                break;
                case FBE_SERVICE_ID_TOPOLOGY:
                    p_component_id_stamp = "TOPO";
                break;
                case FBE_SERVICE_ID_NOTIFICATION:
                    p_component_id_stamp = "NTFY";
                break;
                case FBE_SERVICE_ID_SIMULATOR:
                    p_component_id_stamp = "SIMU";
                break;
                case FBE_SERVICE_ID_SERVICE_MANAGER:
                    p_component_id_stamp = "MNGR";
                break;
                case FBE_SERVICE_ID_SCHEDULER:
                    p_component_id_stamp = "SCHD";
                break;
                case FBE_SERVICE_ID_TERMINATOR:
                    p_component_id_stamp = "TERM";
                break;
                case FBE_SERVICE_ID_TRANSPORT:
                    p_component_id_stamp = "TRAN";
                break;
                case FBE_SERVICE_ID_DRIVE_CONFIGURATION:
                    p_component_id_stamp = "DCFG";
                break;
                case FBE_SERVICE_ID_CMI:
                    p_component_id_stamp = "CMI ";
                    break;
                case FBE_SERVICE_ID_JOB_SERVICE:
                    p_component_id_stamp = "JOB";
                    break;
                case FBE_SERVICE_ID_DATABASE:
                    p_component_id_stamp = "DBS ";
                    break;
                case FBE_SERVICE_ID_CMS:
                    p_component_id_stamp = "CMS";
                    break;
                case FBE_SERVICE_ID_CMS_EXERCISER:
                    p_component_id_stamp = "CMSE";
                    break;

            }
        break;
        case FBE_COMPONENT_TYPE_CLASS:
            p_component_type_stamp = "CLAS";
            status = fbe_get_class_by_id((fbe_class_id_t)component_id, &p_class_info);
            if (status == FBE_STATUS_OK) {
                p_component_id_stamp = p_class_info->class_name;
            }
        break;
        case FBE_COMPONENT_TYPE_OBJECT:
            p_component_type_stamp = "OBJ ";
        break;
        case FBE_COMPONENT_TYPE_LIBRARY:
            p_component_type_stamp = "LIB ";
            switch (component_id) {
                case FBE_LIBRARY_ID_LIFECYCLE:
                    p_component_id_stamp = "LIFE";
                break;
                case FBE_LIBRARY_ID_FLARE_SHIM:
                    p_component_id_stamp = "SHIM";
                break;
                case FBE_LIBRARY_ID_ENCL_DATA_ACCESS:
                    p_component_id_stamp = "E_DA";
                break;
                case FBE_LIBRARY_ID_FBE_API:
                    p_component_id_stamp = "FAPI";
                break;
                case FBE_LIBRARY_ID_FBE_NEIT_API:
                    p_component_id_stamp = "FNEIT";
                break;
                case FBE_LIBRARY_ID_PMC_SHIM:
                    p_component_id_stamp = "PMCS";
                    break;
                case FBE_LIBRARY_ID_DRIVE_SPARING:
                    p_component_id_stamp = "SPARE";
                    break;
                case FBE_LIBRARY_ID_DRIVE_STAT:
                    p_component_id_stamp = "DSTATS";
                    break;
                case FBE_LIBRARY_ID_FIS:
                    p_component_id_stamp = "FIS";
                    break;
                case FBE_LIBRARY_ID_SYSTEM_LIMITS:
                    p_component_id_stamp = "SYSL";
            }
        break;
    }

    if (p_trace_level_stamp == NULL)
    {
        fbe_zero_memory(trace_level_stamp, FBE_TRACE_U32_STAMP_SIZE);
        csx_p_snprintf(trace_level_stamp, FBE_TRACE_U32_STAMP_SIZE, "%4X", trace_level);
        p_trace_level_stamp = trace_level_stamp;
    }

    if (p_component_type_stamp == NULL) {
        fbe_zero_memory(component_type_stamp, FBE_TRACE_U32_STAMP_SIZE);
        csx_p_snprintf(component_type_stamp, FBE_TRACE_U32_STAMP_SIZE, "%4X", component_type);
        p_component_type_stamp = component_type_stamp;
    }

    if (p_component_id_stamp == NULL) {
        fbe_zero_memory(component_id_stamp, FBE_TRACE_U32_STAMP_SIZE);
        csx_p_snprintf(component_id_stamp, FBE_TRACE_U32_STAMP_SIZE, "%4X", component_id);
        p_component_id_stamp = component_id_stamp;
    }

    fbe_zero_memory(stamp, FBE_TRACE_STAMP_SIZE);
    csx_p_snprintf(stamp, FBE_TRACE_STAMP_SIZE, "%s %s %s %s:",
                       p_trace_level_stamp, p_component_type_stamp, p_component_id_stamp, header_string);

    fbe_zero_memory(msg, FBE_TRACE_MSG_SIZE);
    csx_p_vsnprintf(msg, FBE_TRACE_MSG_SIZE, fmt, argList);

    fbe_KvTrace("%s %s", stamp, msg);
}


void
fbe_base_package_trace(fbe_package_id_t package_id,
                       fbe_trace_level_t trace_level,
                       fbe_u32_t message_id,
                       const fbe_char_t * fmt, ...)
{
    va_list argList;

    if (trace_level > fbe_trace_default_trace_level) {
        return;
    }
    va_start(argList, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_PACKAGE,
                     package_id,
                     trace_level,
                     message_id,
                     fmt, 
                     argList);
    va_end(argList);
}
void
fbe_base_package_startup_trace(fbe_package_id_t package_id,
                               fbe_trace_level_t trace_level,
                               fbe_u32_t message_id,
                               const fbe_char_t * fmt, ...)
{
    va_list argList;

    if (trace_level > fbe_trace_default_trace_level) {
        return;
    }
    va_start(argList, fmt);
    fbe_trace_at_startup(FBE_COMPONENT_TYPE_PACKAGE,
                         package_id,
                         trace_level,
                         message_id,
                         fmt, 
                         argList);
    va_end(argList);
}

void
fbe_base_library_trace(fbe_library_id_t library_id,
                       fbe_trace_level_t trace_level,
                       fbe_u32_t message_id,
                       const fbe_char_t * fmt, ...)
{
    va_list argList;

    if (trace_level > fbe_trace_default_trace_level) {
        return;
    }
    va_start(argList, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_LIBRARY,
                     library_id,
                     trace_level,
                     message_id,
                     fmt, 
                     argList);
    va_end(argList);
}

fbe_trace_level_t fbe_trace_get_lib_default_trace_level(fbe_library_id_t library_id)
{
    fbe_u32_t ii;

    for (ii = 0; ii < fbe_trace_default_lib_trace_level_max; ii++) {
        if (fbe_trace_default_lib_trace_level[ii].library_id == library_id) {
            return fbe_trace_default_lib_trace_level[ii].trace_level;
        }
    }
    return fbe_trace_default_trace_level;
}

static fbe_status_t fbe_trace_control_reset_error_counters(fbe_packet_t * packet)
{
    fbe_package_id_t		package_id = FBE_PACKAGE_ID_INVALID;

    fbe_get_package_id(&package_id);

    fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "Trace error counters in package:%d set to 0 by user request\n", package_id);

    trace_critical_error_counter = 0;
    trace_error_counter = 0;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}


fbe_status_t fbe_lifecycle_get_debug_trace_flag(fbe_u32_t * p_flag)
{
    *p_flag = lifecycle_debug_flag;
    return FBE_STATUS_OK;
}

static fbe_status_t
fbe_lifecycle_debug_trace_control_set_flags(fbe_packet_t * packet)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;
    fbe_lifecycle_debug_trace_control_t * lifecycle_trace_flag_control;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_control_buffer_length_t buffer_length;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get packet payload\n", __FUNCTION__);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }
  
    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get control operation\n", __FUNCTION__);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    status = fbe_payload_control_get_buffer(control_operation, (fbe_payload_control_buffer_t*)&lifecycle_trace_flag_control);
    if (lifecycle_trace_flag_control == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get control operation buffer\n", __FUNCTION__);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    fbe_payload_control_get_buffer_length(control_operation, &buffer_length); 
    if (buffer_length != sizeof(fbe_lifecycle_debug_trace_control_t)) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get control operation buffer size\n", __FUNCTION__);
        
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_lifecycle_debug_set_trace_flag(lifecycle_trace_flag_control->trace_flag);
    if (status != FBE_STATUS_OK)
    {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't set lifecycle debug flag\n", __FUNCTION__);
    }
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_lifecycle_debug_set_trace_flag(fbe_lifecycle_state_debug_tracing_flags_t default_trace_flag)
{
    lifecycle_debug_flag = default_trace_flag;
    return FBE_STATUS_OK;
}

static fbe_status_t fbe_trace_control_disable_backtrace(fbe_packet_t * packet)
{
    fbe_package_id_t		package_id = FBE_PACKAGE_ID_INVALID;

    fbe_get_package_id(&package_id);

    fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "Disable backtrace for package:%d by user request\n", package_id);
    fbe_trace_backtrace_enabled = FBE_FALSE;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}

static fbe_status_t fbe_trace_control_enable_backtrace(fbe_packet_t * packet)
{
    fbe_package_id_t		package_id = FBE_PACKAGE_ID_INVALID;

    fbe_get_package_id(&package_id);

    fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "Enable backtrace for package:%d by user request\n", package_id);
    fbe_trace_backtrace_enabled = FBE_TRUE;

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}

static fbe_status_t fbe_trace_control_command_to_ktrace_buff(fbe_packet_t *packet)
{
    fbe_payload_ex_t * payload;
    fbe_payload_control_operation_t * control_operation;
    fbe_trace_command_to_ktrace_buff_t * cmd_ktracebuf;
    fbe_payload_control_buffer_length_t buffer_length;
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get packet payload\n", __FUNCTION__);

        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get control operation\n", __FUNCTION__);

        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    status = fbe_payload_control_get_buffer(control_operation, (fbe_payload_control_buffer_t*)&cmd_ktracebuf);
    if (status != FBE_STATUS_OK) {
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    buffer_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length); 
    if (buffer_length != sizeof(fbe_trace_command_to_ktrace_buff_t)) {
        fbe_base_service_trace((fbe_base_service_t*)&trace_service,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s, can't get control operation buffer size\n", __FUNCTION__);

        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_KvTraceStart("FBE_CLI> %s \n", cmd_ktracebuf->cmd);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}
