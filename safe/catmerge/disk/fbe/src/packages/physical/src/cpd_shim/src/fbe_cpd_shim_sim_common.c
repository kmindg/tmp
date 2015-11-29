#include "fbe/fbe_winddk.h"
#include "fbe_cpd_shim.h"
#include "fbe_cpd_shim_private.h"
#include "fbe/fbe_payload_ex.h"
#include "fbe/fbe_transport.h"
#include "fbe_scsi.h"
/* #include "fbe/fbe_file.h" */
#include "fbe_terminator_miniport_interface.h"
#include  "fbe/fbe_library_interface.h"

#define FBE_CPD_SHIM_MEMORY_TAG               'mihs'
#define FBE_CPD_SHIM_INVALID_PORT_NUMBER      0xFFFF
#define FBE_CPD_SHIM_INVALID_PORTAL_NUMBER    0xFFFF

#define CPD_LOCAL_PORT_ADDRESS_BASE 0x123456789
#define FBE_CPD_USER_SHIM_MAX_DEV_TABLE_ENTRY 275

static fbe_terminator_miniport_interface_port_shim_sim_pointers_t miniport_interface_pointers;
static fbe_status_t fbe_cpd_shim_unregister_keys(fbe_u32_t port_handle, fbe_base_port_mgmt_unregister_keys_t *unregister_info);
static fbe_status_t fbe_cpd_shim_register_keys(fbe_u32_t port_handle, fbe_base_port_mgmt_register_keys_t * port_register_keys_p);

typedef struct cpd_port_s {
    fbe_u32_t                           port_handle;
    cpd_shim_port_state_t               state;
    fbe_u32_t                           io_port_number;
    fbe_u32_t                           io_portal_number;

    fbe_cpd_shim_callback_function_t    callback_function; /* Async events from miniport */
    fbe_cpd_shim_callback_context_t     callback_context;
    /* Port specific Information.*/
    fbe_atomic_t                        current_generation_code;
    fbe_cpd_shim_port_info_t            port_info;
    /* Device specific information.*/
    fbe_cpd_shim_device_table_t        *device_table;
    fbe_u32_t                           device_table_size;

} cpd_port_t;

#define FBE_CPD_SHIM_MAX_CALLBACK_INFO_ENTRIES  10

static cpd_port_t cpd_port_array[FBE_CPD_SHIM_MAX_PORTS];

static fbe_status_t fbe_cpd_shim_port_callback (fbe_cpd_shim_callback_info_t * callback_info,
                                                fbe_cpd_shim_callback_context_t context);

static __forceinline fbe_u32_t cpd_port_index_to_port_handle(fbe_u32_t index)
{
    return index;
}

static __forceinline fbe_u32_t cpd_port_handle_to_port_index(fbe_u32_t handle)
{
    return (handle < FBE_CPD_SHIM_MAX_PORTS) ? handle : FBE_CPD_SHIM_INVALID_PORT_HANDLE;
}

static __forceinline fbe_u32_t cpd_terminator_port_handle_to_port_index(fbe_u32_t handle)
{
    return (handle < FBE_CPD_SHIM_MAX_PORTS) ? handle : FBE_CPD_SHIM_INVALID_PORT_HANDLE;
}

static __forceinline fbe_u32_t cpd_port_index_to_terminator_port_handle(fbe_u32_t index)
{
    return index;
}

fbe_status_t
fbe_cpd_shim_destroy(void)
{
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_cpd_shim_port_init(fbe_u32_t io_port_number, fbe_u32_t io_portal_number, fbe_u32_t *port_handle)
{
    fbe_status_t status;
    fbe_u32_t terminator_port_handle;
    fbe_u32_t port_ii;
    fbe_cpd_shim_device_table_t * device_table;
    cpd_port_t * cpd_port = NULL;

    *port_handle = FBE_CPD_SHIM_INVALID_PORT_HANDLE;

    status = miniport_interface_pointers.terminator_miniport_api_port_init_function(io_port_number,
                                                                                    io_portal_number,
                                                                                    &terminator_port_handle);
    if (status != FBE_STATUS_OK) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s terminator init failed, status: 0x%X, io_port_number: 0x%X, io_portal_number: 0x%X\n",
                               __FUNCTION__, status, io_port_number, io_portal_number);
        return status;
    }
    port_ii = cpd_terminator_port_handle_to_port_index(terminator_port_handle);
    if (port_ii == FBE_CPD_SHIM_INVALID_PORT_HANDLE) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid port handle: 0x%X, io_port_number: 0x%X, io_portal_number: 0x%X\n",
                               __FUNCTION__, terminator_port_handle, io_port_number, io_portal_number);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    device_table = fbe_allocate_nonpaged_pool_with_tag(
                                         (sizeof(fbe_cpd_shim_device_table_t) +
                                         (FBE_CPD_USER_SHIM_MAX_DEV_TABLE_ENTRY * sizeof(fbe_cpd_shim_device_table_entry_t))),
                                         FBE_CPD_SHIM_MEMORY_TAG);
    if (device_table == NULL){
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s Failed allocation of memory for device table. io_port_number = %X, io_portal_number = %X \n",
                               __FUNCTION__, io_port_number, io_portal_number);
        miniport_interface_pointers.terminator_miniport_api_port_destroy_function(terminator_port_handle);
        return FBE_STATUS_INSUFFICIENT_RESOURCES;
    }

    *port_handle = cpd_port_index_to_port_handle(port_ii);
    fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: initializing port, handle: 0x%X, io_port_number: 0x%X, io_portal_number: 0x%X\n",
                           __FUNCTION__, *port_handle, io_port_number, io_portal_number);
    cpd_port = &cpd_port_array[port_ii];
    cpd_port->state = CPD_SHIM_PORT_STATE_INITIALIZED;
    cpd_port->port_handle = *port_handle;
    cpd_port->io_port_number = io_port_number;
    cpd_port->io_portal_number = io_portal_number;
    cpd_port->device_table = device_table;
    cpd_port->device_table->device_table_size = FBE_CPD_USER_SHIM_MAX_DEV_TABLE_ENTRY;
    fbe_zero_memory(cpd_port->device_table->device_entry,
                    (FBE_CPD_USER_SHIM_MAX_DEV_TABLE_ENTRY * sizeof(fbe_cpd_shim_device_table_entry_t)));
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_cpd_shim_port_destroy(fbe_u32_t port_handle)
{
    fbe_u32_t port_ii;
    cpd_port_t * cpd_port = NULL;
    fbe_u32_t terminator_port_handle;

    port_ii = cpd_port_handle_to_port_index(port_handle);
    if (port_ii == FBE_CPD_SHIM_INVALID_PORT_HANDLE) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid port handle: 0x%X\n",
                               __FUNCTION__, port_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                        FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s port handle: 0x%X \n", __FUNCTION__, port_handle);

    terminator_port_handle = cpd_port_index_to_terminator_port_handle(port_ii);
    miniport_interface_pointers.terminator_miniport_api_port_destroy_function(terminator_port_handle);

    cpd_port = &cpd_port_array[port_ii];
    if (cpd_port->device_table != NULL) {
        fbe_release_nonpaged_pool(cpd_port->device_table);
    }
    fbe_zero_memory(cpd_port,sizeof(cpd_port_t));
    cpd_port->port_handle = FBE_CPD_SHIM_INVALID_PORT_HANDLE;
    cpd_port->io_port_number = FBE_CPD_SHIM_INVALID_PORT_NUMBER;
    cpd_port->io_portal_number = FBE_CPD_SHIM_INVALID_PORTAL_NUMBER;
    cpd_port->state = CPD_SHIM_PORT_STATE_UNINITIALIZED;

    return FBE_STATUS_OK;
}

fbe_status_t
fbe_cpd_shim_enumerate_backend_io_modules(fbe_cpd_shim_enumerate_io_modules_t * cpd_shim_enumerate_io_modules)
{
    /* TODO: Implement in terminator.*/
    cpd_shim_enumerate_io_modules->rescan_required = FBE_TRUE;
    cpd_shim_enumerate_io_modules->number_of_io_modules = 0;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_cpd_shim_get_port_name_info(fbe_u32_t port_handle, fbe_cpd_shim_port_name_info_t *port_name_info)
{
    fbe_status_t status;
    fbe_address_t address;
    fbe_u32_t port_ii;
    fbe_u32_t terminator_port_handle;

     port_ii = cpd_port_handle_to_port_index(port_handle);
    if (port_ii == FBE_CPD_SHIM_INVALID_PORT_HANDLE) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid port handle: 0x%X\n",
                               __FUNCTION__, port_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s port_handle = %X\n",
                           __FUNCTION__, port_handle);

    terminator_port_handle = cpd_port_index_to_terminator_port_handle(port_ii);
    status  = miniport_interface_pointers.terminator_miniport_api_get_port_address_function(terminator_port_handle, &address);
    port_name_info->sas_address = address.sas_address;
    return status;
}
fbe_status_t
fbe_cpd_shim_enumerate_backend_ports(fbe_cpd_shim_enumerate_backend_ports_t * cpd_shim_enumerate_backend_ports)
{
    fbe_status_t status;

    status  = miniport_interface_pointers.terminator_miniport_api_enumerate_cpd_ports_function(cpd_shim_enumerate_backend_ports);
    return status;
}


fbe_status_t
fbe_cpd_shim_change_speed(fbe_u32_t port_handle, fbe_port_speed_t speed)
{
    fbe_u32_t port_ii;
    fbe_u32_t terminator_port_handle;
    fbe_status_t status;

    port_ii = cpd_port_handle_to_port_index(port_handle);
    if (port_ii == FBE_CPD_SHIM_INVALID_PORT_HANDLE) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid port handle: 0x%X\n",
                               __FUNCTION__, port_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s port_handle = %X, speed = %X\n",
                           __FUNCTION__, port_handle, speed);

    terminator_port_handle = cpd_port_index_to_terminator_port_handle(port_ii);
    status = miniport_interface_pointers.terminator_miniport_api_set_speed_function (terminator_port_handle, speed);
    return status;
}

fbe_status_t
fbe_cpd_shim_register_data_encryption_keys(fbe_u32_t port_handle, 
                                           fbe_base_port_mgmt_register_keys_t * port_register_keys_p)
{
    return fbe_cpd_shim_register_keys(port_handle, port_register_keys_p);
}

static fbe_status_t
fbe_cpd_shim_register_keys(fbe_u32_t port_handle, fbe_base_port_mgmt_register_keys_t * port_register_keys_p)

{
    fbe_u32_t port_ii;
    fbe_u32_t terminator_port_handle;
    fbe_status_t status;

    port_ii = cpd_port_handle_to_port_index(port_handle);
    if (port_ii == FBE_CPD_SHIM_INVALID_PORT_HANDLE) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid port handle: 0x%X\n",
                               __FUNCTION__, port_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s port_handle = %X\n",
                           __FUNCTION__, port_handle);

    terminator_port_handle = cpd_port_index_to_terminator_port_handle(port_ii);
    status = miniport_interface_pointers.terminator_miniport_api_register_keys_function (terminator_port_handle, port_register_keys_p);
    return status;
}

fbe_status_t
fbe_cpd_shim_unregister_data_encryption_keys(fbe_u32_t port_handle, fbe_base_port_mgmt_unregister_keys_t *unregister_info)
{
    return fbe_cpd_shim_unregister_keys(port_handle, unregister_info);

}
static fbe_status_t
fbe_cpd_shim_unregister_keys(fbe_u32_t port_handle, fbe_base_port_mgmt_unregister_keys_t *unregister_info)
{
    fbe_u32_t port_ii;
    fbe_u32_t terminator_port_handle;
    fbe_status_t status;

    port_ii = cpd_port_handle_to_port_index(port_handle);
    if (port_ii == FBE_CPD_SHIM_INVALID_PORT_HANDLE) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid port handle: 0x%X\n",
                               __FUNCTION__, port_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s port_handle = %X\n",
                           __FUNCTION__, port_handle);

    terminator_port_handle = cpd_port_index_to_terminator_port_handle(port_ii);
    status = miniport_interface_pointers.terminator_miniport_api_unregister_keys_function (terminator_port_handle, unregister_info);
    return status;
}

fbe_status_t fbe_cpd_shim_set_encryption_mode(fbe_u32_t port_handle, 
                                              fbe_port_encryption_mode_t mode)
{
     return FBE_STATUS_OK;
}

fbe_status_t
fbe_cpd_shim_register_kek(fbe_u32_t port_handle, 
                          fbe_base_port_mgmt_register_keys_t * port_register_keys_p,
                          void *context)
{
    fbe_status_t status;
    fbe_packet_t *packet;

    packet = (fbe_packet_t *)context;
    status = fbe_cpd_shim_register_keys(port_handle, port_register_keys_p);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;

}

fbe_status_t
fbe_cpd_shim_unregister_kek(fbe_u32_t port_handle, fbe_base_port_mgmt_unregister_keys_t *unregister_info)
{
    return fbe_cpd_shim_unregister_keys(port_handle, unregister_info);
}

fbe_status_t fbe_cpd_shim_register_kek_kek(fbe_u32_t port_handle, 
                                           fbe_base_port_mgmt_register_keys_t * port_register_keys_p,
                                           void *context)
{
    fbe_status_t status;
    fbe_packet_t *packet;

    packet = (fbe_packet_t *)context;
    status = fbe_cpd_shim_register_keys(port_handle, port_register_keys_p);
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_cpd_shim_rebase_all_keys(fbe_u32_t port_handle, 
                                          fbe_key_handle_t key_handle,
                                          void *port_kek_context)
{
    return FBE_STATUS_OK;
}
fbe_status_t fbe_cpd_shim_unregister_kek_kek(fbe_u32_t port_handle, fbe_base_port_mgmt_unregister_keys_t *unregister_info)
{
    return fbe_cpd_shim_unregister_keys(port_handle, unregister_info);
}

fbe_status_t fbe_cpd_shim_reestablish_key_handle(fbe_u32_t port_handle, fbe_base_port_mgmt_reestablish_key_handle_t * key_handle_info)
{
    fbe_u32_t port_ii;
    fbe_u32_t terminator_port_handle;
    fbe_status_t status;

    port_ii = cpd_port_handle_to_port_index(port_handle);
    if (port_ii == FBE_CPD_SHIM_INVALID_PORT_HANDLE) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid port handle: 0x%X\n",
                               __FUNCTION__, port_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s port_handle = %X\n",
                           __FUNCTION__, port_handle);

    terminator_port_handle = cpd_port_index_to_terminator_port_handle(port_ii);
    status = miniport_interface_pointers.terminator_miniport_api_reestablish_key_handle_function (terminator_port_handle, key_handle_info);
    return status;
}

fbe_status_t
fbe_cpd_shim_get_port_info(fbe_u32_t port_handle, fbe_port_info_t * port_info)
{
    fbe_status_t status;
    fbe_u32_t port_ii;
    fbe_u32_t terminator_port_handle;

    port_ii = cpd_port_handle_to_port_index(port_handle);
    if (port_ii == FBE_CPD_SHIM_INVALID_PORT_HANDLE) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid port handle: 0x%X\n",
                               __FUNCTION__, port_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                           FBE_TRACE_LEVEL_INFO,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s port_handle = %X, speed = %X\n",
                           __FUNCTION__, port_handle, port_info->link_speed);

    terminator_port_handle = cpd_port_index_to_terminator_port_handle(port_ii);
    status = miniport_interface_pointers.terminator_miniport_api_get_port_info_function (terminator_port_handle, port_info);
    return status;
}

fbe_status_t
fbe_cpd_shim_port_register_callback(fbe_u32_t port_handle,
                                    fbe_u32_t registration_flags,
                                    fbe_cpd_shim_callback_function_t callback_function,
                                    fbe_cpd_shim_callback_context_t callback_context)
{
    fbe_status_t status;
    fbe_u32_t port_ii;
    fbe_u32_t terminator_port_handle;

    port_ii = cpd_port_handle_to_port_index(port_handle);
    if (port_ii == FBE_CPD_SHIM_INVALID_PORT_HANDLE) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid port handle: 0x%X\n",
                               __FUNCTION__, port_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    cpd_port_array[port_ii].callback_function = callback_function;
    cpd_port_array[port_ii].callback_context = callback_context;
    terminator_port_handle = cpd_port_index_to_terminator_port_handle(port_ii);
    status = miniport_interface_pointers.terminator_miniport_api_register_callback_function(terminator_port_handle,
                                                                                            (fbe_terminator_miniport_api_callback_function_t)fbe_cpd_shim_port_callback,
                                                                                            (void **)&cpd_port_array[port_ii]);
    if (registration_flags & FBE_CPD_SHIM_REGISTER_CALLBACKS_SFP_EVENTS){
        status = miniport_interface_pointers.terminator_miniport_api_register_sfp_event_callback_function(terminator_port_handle,
                                                                                               (fbe_terminator_miniport_api_callback_function_t)fbe_cpd_shim_port_callback,
                                                                                                (void **)&cpd_port_array[port_ii]);
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                            FBE_TRACE_LEVEL_INFO,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s registered SFP callback. Status 0x%x. Port handle : 0x%x\n",
                            __FUNCTION__, status,port_handle);
    }

    return status;
}

fbe_status_t
fbe_cpd_shim_port_unregister_callback(fbe_u32_t port_handle)
{
    fbe_status_t status;
    fbe_u32_t port_ii;
    fbe_u32_t terminator_port_handle;

    port_ii = cpd_port_handle_to_port_index(port_handle);
    if (port_ii == FBE_CPD_SHIM_INVALID_PORT_HANDLE) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid port handle: 0x%X\n",
                               __FUNCTION__, port_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    terminator_port_handle = cpd_port_index_to_terminator_port_handle(port_ii);
    status = miniport_interface_pointers.terminator_miniport_api_unregister_callback_function(terminator_port_handle);
    status = miniport_interface_pointers.terminator_miniport_api_unregister_sfp_event_callback_function(terminator_port_handle);

    cpd_port_array[port_ii].callback_function = NULL;
    cpd_port_array[port_ii].callback_context = NULL;
    return status;
}

/* Step 1: Sync call to port object.
   Step 2: Seperate thread and async call to port object.
 */
fbe_status_t fbe_cpd_shim_port_callback (fbe_cpd_shim_callback_info_t * callback_info, fbe_cpd_shim_callback_context_t context)
{
    fbe_cpd_shim_callback_info_t local_callback_info;
    cpd_port_t * cpd_port = (cpd_port_t *)context;
    fbe_u32_t current_device_index = 0;
    fbe_cpd_shim_device_table_entry_t *current_device_entry = NULL;

    if ((context == NULL) || (callback_info == NULL)) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                            FBE_TRACE_LEVEL_CRITICAL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s Critical error NULL input pointer \n",__FUNCTION__);

        return FALSE;
    }

    fbe_atomic_increment(&(cpd_port->current_generation_code));
    fbe_zero_memory(&local_callback_info,sizeof(fbe_cpd_shim_callback_info_t));
    /* examine the CPD_EVENT_INFO to determine if the event is relavant */
    switch(callback_info->callback_type)
    {
    case FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN:
        current_device_index = FBE_CPD_GET_INDEX_FROM_CONTEXT(callback_info->info.login.device_id);
        if (current_device_index >= cpd_port->device_table->device_table_size) {
            fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                                   FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s Critical error device_index larger than table size. 0x%x 0x%x\n",
                                   __FUNCTION__, current_device_index,
                                   cpd_port->device_table->device_table_size);
            return FALSE;
        }

        current_device_entry = &(cpd_port->device_table->device_entry[current_device_index]);

        current_device_entry->log_out_received = FALSE;
        current_device_entry->device_type = callback_info->info.login.device_type;
        current_device_entry->device_id =  callback_info->info.login.device_id;
        current_device_entry->device_address = callback_info->info.login.device_address;
        current_device_entry->device_locator = callback_info->info.login.device_locator;
        current_device_entry->parent_device_id = callback_info->info.login.parent_device_id;
        fbe_atomic_increment(&(current_device_entry->current_gen_number));

        local_callback_info.callback_type = FBE_CPD_SHIM_CALLBACK_TYPE_DEVICE_TABLE_UPDATE;
        if(cpd_port->callback_function != NULL){
            cpd_port->callback_function(&local_callback_info, cpd_port->callback_context);
        }
        break;
    case FBE_CPD_SHIM_CALLBACK_TYPE_LOGIN_FAILED: /* for backend recovery */
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_INFO,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s CPD_EVENT_LOGIN_FAILED port_handle: 0x%X, cpd_device_id = 0x%llX \n",
                               __FUNCTION__, cpd_port->port_handle,
			       (unsigned long long)callback_info->info.login.device_id);
        break;
    case FBE_CPD_SHIM_CALLBACK_TYPE_LOGOUT:
    case FBE_CPD_SHIM_CALLBACK_TYPE_DEVICE_FAILED:
        current_device_index = FBE_CPD_GET_INDEX_FROM_CONTEXT(callback_info->info.login.device_id);
        if (current_device_index >= cpd_port->device_table->device_table_size){
            fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                                   FBE_TRACE_LEVEL_CRITICAL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s Critical error device_index larger than table size. 0x%x 0x%x\n",
                                   __FUNCTION__, current_device_index,
                                   cpd_port->device_table->device_table_size);
            return FALSE;
        }
        current_device_entry = &(cpd_port->device_table->device_entry[current_device_index]);
        current_device_entry->log_out_received = TRUE;
        fbe_atomic_increment(&(current_device_entry->current_gen_number));
        local_callback_info.callback_type = FBE_CPD_SHIM_CALLBACK_TYPE_DEVICE_TABLE_UPDATE;
        if(cpd_port->callback_function != NULL){
            cpd_port->callback_function(&local_callback_info, cpd_port->callback_context);
        }
        break;

    case FBE_CPD_SHIM_CALLBACK_TYPE_DISCOVERY:
        switch(callback_info->info.discovery.discovery_event_type) {
            case FBE_CPD_SHIM_DISCOVERY_EVENT_TYPE_START:
                fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                                       FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s port_handle: 0x%X Discovery Start event received.\n",
                                       __FUNCTION__, cpd_port->port_handle);
                break;
            case FBE_CPD_SHIM_DISCOVERY_EVENT_TYPE_COMPLETE:
                fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                                       FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s port_handle: 0x%X  Discovery complete event received.\n",
                                       __FUNCTION__, cpd_port->port_handle);
                break;
        }
        if(cpd_port->callback_function != NULL){
            cpd_port->callback_function(callback_info, cpd_port->callback_context);
        }
        break;

    case FBE_CPD_SHIM_CALLBACK_TYPE_DRIVER_RESET:
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s port_handle 0x%X  Driver reset event received.\n",
                               __FUNCTION__, cpd_port->port_handle);
        if(cpd_port->callback_function != NULL){
            cpd_port->callback_function(callback_info, cpd_port->callback_context);
        }
        break;

    case FBE_CPD_SHIM_CALLBACK_TYPE_SFP:
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s port_handle 0x%X  SFP even received. Event 0x%x.\n",
                               __FUNCTION__, cpd_port->port_handle,
                               callback_info->info.sfp_info.condition_type);

        if(cpd_port->callback_function != NULL){
            cpd_port->callback_function(callback_info, cpd_port->callback_context);
        }

        break;

    case FBE_CPD_SHIM_CALLBACK_TYPE_LINK_UP:
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s port_handle 0x%X  Link UP event received\n",
                               __FUNCTION__, cpd_port->port_handle);

        if(cpd_port->callback_function != NULL){
            cpd_port->callback_function(callback_info, cpd_port->callback_context);
        }
        break;

    case FBE_CPD_SHIM_CALLBACK_TYPE_LINK_DOWN:
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s port_handle 0x%X  Link DOWN event received\n",
                               __FUNCTION__, cpd_port->port_handle);

        if(cpd_port->callback_function != NULL){
            cpd_port->callback_function(callback_info, cpd_port->callback_context);
        }
        break;

    case FBE_CPD_SHIM_CALLBACK_TYPE_LANE_STATUS:
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s port_handle 0x%X  Link STATUS CHANGE event received\n",
                               __FUNCTION__, cpd_port->port_handle);

        if(cpd_port->callback_function != NULL){
            cpd_port->callback_function(callback_info, cpd_port->callback_context);
        }
        break;

    default: /* ignore the rest */
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s port_handle: 0x%X,  pInfo->type: 0x%X  \n",
                               __FUNCTION__, cpd_port->port_handle, callback_info->callback_type);
        break;
    } /* end of switch(Info->Type) */

    return (FALSE);
}

static csx_size_t cpd_shim_sim_max_stack_size = 0;

fbe_status_t
fbe_cpd_shim_send_payload(fbe_u32_t port_handle, fbe_cpd_device_id_t cpd_device_id, fbe_payload_ex_t * payload)
{
    fbe_status_t status;
    fbe_u32_t port_ii;
    fbe_u32_t terminator_port_handle;
	csx_size_t   	used_stack_size_rv;
	fbe_trace_level_t trace_level = FBE_TRACE_LEVEL_INFO;

	csx_p_get_used_stack_size(&used_stack_size_rv); 

	if(used_stack_size_rv > cpd_shim_sim_max_stack_size){
		cpd_shim_sim_max_stack_size = used_stack_size_rv;
		if(cpd_shim_sim_max_stack_size >= 15000){
			//trace_level = FBE_TRACE_LEVEL_CRITICAL_ERROR;
		}
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               trace_level,
                               FBE_TRACE_MESSAGE_ID_INFO,
							   "cpd_shim_sim_max_stack_size: %d\n",
                               (int)cpd_shim_sim_max_stack_size);

	}


    port_ii = cpd_port_handle_to_port_index(port_handle);
    if (port_ii == FBE_CPD_SHIM_INVALID_PORT_HANDLE) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid port handle: 0x%X\n",
                               __FUNCTION__, port_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    terminator_port_handle = cpd_port_index_to_terminator_port_handle(port_ii);
    status  = miniport_interface_pointers.terminator_miniport_api_send_payload_function(terminator_port_handle, cpd_device_id, payload);
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_cpd_shim_send_fis(fbe_u32_t port_handle, fbe_cpd_device_id_t cpd_device_id, fbe_payload_ex_t * payload)
{
    fbe_status_t status;
    fbe_u32_t port_ii;
    fbe_u32_t terminator_port_handle;

    port_ii = cpd_port_handle_to_port_index(port_handle);
    if (port_ii == FBE_CPD_SHIM_INVALID_PORT_HANDLE) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid port handle: 0x%X\n",
                               __FUNCTION__, port_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    terminator_port_handle = cpd_port_index_to_terminator_port_handle(port_ii);
    status  = miniport_interface_pointers.terminator_miniport_api_send_fis_function(terminator_port_handle, cpd_device_id, payload);
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_cpd_shim_port_register_payload_completion(fbe_u32_t port_handle,
                                              fbe_payload_ex_completion_function_t completion_function,
                                              fbe_payload_ex_completion_context_t  completion_context)
{
    fbe_status_t status;
    fbe_u32_t port_ii;
    fbe_u32_t terminator_port_handle;

    port_ii = cpd_port_handle_to_port_index(port_handle);
    if (port_ii == FBE_CPD_SHIM_INVALID_PORT_HANDLE) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid port handle: 0x%X\n",
                               __FUNCTION__, port_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    terminator_port_handle = cpd_port_index_to_terminator_port_handle(port_ii);
    status = miniport_interface_pointers.terminator_miniport_api_register_payload_completion_function (terminator_port_handle,
                                                                                                       completion_function,
                                                                                                       completion_context);
    return status;
}

fbe_status_t
fbe_cpd_shim_port_unregister_payload_completion(fbe_u32_t port_handle)
{
    fbe_status_t status;
    fbe_u32_t port_ii;
    fbe_u32_t terminator_port_handle;

    port_ii = cpd_port_handle_to_port_index(port_handle);
    if (port_ii == FBE_CPD_SHIM_INVALID_PORT_HANDLE) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid port handle: 0x%X\n",
                               __FUNCTION__, port_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    terminator_port_handle = cpd_port_index_to_terminator_port_handle(port_ii);
    status = miniport_interface_pointers.terminator_miniport_api_unregister_payload_completion_function(terminator_port_handle);
    return status;
}


fbe_status_t
fbe_cpd_shim_reset_device(fbe_u32_t port_handle, fbe_cpd_device_id_t cpd_device_id)
{
    fbe_u32_t port_ii;
    fbe_u32_t terminator_port_handle;

    port_ii = cpd_port_handle_to_port_index(port_handle);
    if (port_ii == FBE_CPD_SHIM_INVALID_PORT_HANDLE) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid port handle: 0x%X\n",
                               __FUNCTION__, port_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    terminator_port_handle = cpd_port_index_to_terminator_port_handle(port_ii);
    return miniport_interface_pointers.terminator_miniport_api_reset_device_function(terminator_port_handle, cpd_device_id);
}

fbe_status_t
fbe_cpd_shim_port_get_device_table_ptr(fbe_u32_t port_handle, fbe_cpd_shim_device_table_t **device_table_ptr)
{
    cpd_port_t * cpd_port;
    fbe_u32_t port_ii;
    fbe_status_t status;

    if (device_table_ptr == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *device_table_ptr = NULL;

    port_ii = cpd_port_handle_to_port_index(port_handle);
    if (port_ii == FBE_CPD_SHIM_INVALID_PORT_HANDLE) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid port handle: 0x%X\n",
                               __FUNCTION__, port_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = FBE_STATUS_GENERIC_FAILURE;
    cpd_port = &cpd_port_array[port_ii];
    if (cpd_port->device_table != NULL) {
        *device_table_ptr = cpd_port->device_table;
        status = FBE_STATUS_OK;
    }
    return status;
}

fbe_status_t
fbe_cpd_shim_port_get_port_state_info_ptr(fbe_u32_t port_handle, fbe_cpd_shim_port_info_t **port_state_info_ptr)
{
    cpd_port_t * cpd_port;
    fbe_u32_t port_ii;

    if (port_state_info_ptr == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *port_state_info_ptr = NULL;

    port_ii = cpd_port_handle_to_port_index(port_handle);
    if (port_ii == FBE_CPD_SHIM_INVALID_PORT_HANDLE) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid port handle: 0x%X\n",
                               __FUNCTION__, port_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    cpd_port = &cpd_port_array[port_ii];
    *port_state_info_ptr = &(cpd_port->port_info);
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_cpd_shim_port_get_device_table_max_index(fbe_u32_t port_handle,fbe_u32_t *device_table_max_index)
{
    cpd_port_t * cpd_port;
    fbe_status_t  status;
    fbe_u32_t port_ii;

    if (device_table_max_index == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *device_table_max_index = 0;

    port_ii = cpd_port_handle_to_port_index(port_handle);
    if (port_ii == FBE_CPD_SHIM_INVALID_PORT_HANDLE) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid port handle: 0x%X\n",
                               __FUNCTION__, port_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = FBE_STATUS_GENERIC_FAILURE;
    cpd_port = &cpd_port_array[port_handle];
    if (cpd_port->device_table != NULL) {
        *device_table_max_index = cpd_port->device_table->device_table_size;
        status = FBE_STATUS_OK;
    }
    return status;
}

fbe_status_t
fbe_cpd_shim_get_port_name(fbe_u32_t port_handle, fbe_port_name_t * port_name)
{
    fbe_status_t  status;
    fbe_u32_t port_ii;
    fbe_u32_t terminator_port_handle;

    if (port_name == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    port_ii = cpd_port_handle_to_port_index(port_handle);
    if (port_ii == FBE_CPD_SHIM_INVALID_PORT_HANDLE) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid port handle: 0x%X\n",
                               __FUNCTION__, port_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    terminator_port_handle = cpd_port_index_to_terminator_port_handle(port_ii);
    status = miniport_interface_pointers.terminator_miniport_api_get_port_address_function(terminator_port_handle,
                                                                                           &(port_name->port_name));

    return status;
}

fbe_status_t
fbe_cpd_shim_sim_set_terminator_miniport_pointers(fbe_terminator_miniport_interface_port_shim_sim_pointers_t * miniport_pointers)
{
    fbe_copy_memory(&miniport_interface_pointers, miniport_pointers, sizeof(fbe_terminator_miniport_interface_port_shim_sim_pointers_t));
    return FBE_STATUS_OK;
}


fbe_status_t
fbe_cpd_shim_get_media_inteface_info(fbe_u32_t  port_handle, fbe_cpd_shim_media_interface_information_type_t mii_type,
                                 fbe_cpd_shim_sfp_media_interface_info_t *media_interface_info)
{
    fbe_status_t status;
    fbe_u32_t port_ii;
    fbe_u32_t terminator_port_handle;

    port_ii = cpd_port_handle_to_port_index(port_handle);
    if (port_ii == FBE_CPD_SHIM_INVALID_PORT_HANDLE) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid port handle: 0x%X\n",
                               __FUNCTION__, port_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    terminator_port_handle = cpd_port_index_to_terminator_port_handle(port_ii);
    status = miniport_interface_pointers.terminator_miniport_api_get_sfp_media_interface_info_function (terminator_port_handle, media_interface_info);
    return status;
}

fbe_status_t
fbe_cpd_shim_get_hardware_info(fbe_u32_t port_handle, fbe_cpd_shim_hardware_info_t *hdw_info)
{
    fbe_status_t status;
    fbe_u32_t port_ii;
    fbe_u32_t terminator_port_handle;

    port_ii = cpd_port_handle_to_port_index(port_handle);
    if (port_ii == FBE_CPD_SHIM_INVALID_PORT_HANDLE) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid port handle: 0x%X\n",
                               __FUNCTION__, port_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    terminator_port_handle = cpd_port_index_to_terminator_port_handle(port_ii);
    status = miniport_interface_pointers.terminator_miniport_api_get_hardware_info_function (terminator_port_handle, hdw_info);
    return status;
}

fbe_status_t
fbe_cpd_shim_get_port_capabilities(fbe_u32_t port_handle,
                                  fbe_cpd_shim_port_capabilities_t *port_capabilities)
{
    fbe_zero_memory(port_capabilities, sizeof(*port_capabilities));
    /*! @todo Need to allow these values to change for test purposes.
     */
    port_capabilities->link_speed = 0;
    port_capabilities->maximum_transfer_length = FBE_CPD_USER_SHIM_MAX_TRANSFER_LENGTH;
    port_capabilities->maximum_sg_entries = FBE_CPD_USER_SHIM_MAX_SG_ENTRIES;

    return FBE_STATUS_OK;

}

fbe_status_t
fbe_cpd_shim_get_port_config_info(fbe_u32_t port_handle,
                                  fbe_cpd_shim_port_configuration_t *fbe_config_info)
{
    fbe_status_t status;
    fbe_u32_t port_ii;
    fbe_u32_t terminator_port_handle;

    port_ii = cpd_port_handle_to_port_index(port_handle);
    if (port_ii == FBE_CPD_SHIM_INVALID_PORT_HANDLE) {
        fbe_base_library_trace(FBE_LIBRARY_ID_CPD_SHIM,
                               FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s invalid port handle: 0x%X\n",
                               __FUNCTION__, port_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    terminator_port_handle = cpd_port_index_to_terminator_port_handle(port_ii);
    status = miniport_interface_pointers.terminator_miniport_api_port_configuration_function (terminator_port_handle, fbe_config_info);
    return status;
}

fbe_status_t
fbe_cpd_shim_reset_expander_phy(fbe_u32_t port_handle, fbe_cpd_device_id_t smp_port_device_id,fbe_u8_t phy_id)
{
    return FBE_STATUS_OK;
}

fbe_status_t
fbe_cpd_shim_get_port_lane_info(fbe_u32_t port_handle,
                                fbe_cpd_shim_connect_class_t   connect_class,
                                fbe_cpd_shim_port_lane_info_t *port_lane_info)
{
    fbe_status_t status;
    fbe_u32_t    port_ii;
    fbe_u32_t    terminator_port_handle;

    port_ii = cpd_port_handle_to_port_index(port_handle);
    if (port_ii == FBE_CPD_SHIM_INVALID_PORT_HANDLE)
    {
        KvPrint("%s invalid port handle: 0x%X\n", __FUNCTION__, port_handle);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    terminator_port_handle = cpd_port_index_to_terminator_port_handle(port_ii);
    status = miniport_interface_pointers.terminator_miniport_api_get_port_link_info_function(terminator_port_handle, port_lane_info);
    if (status == FBE_STATUS_OK)
    {
        KvPrint("%s Success. Speed: 0x%X State:0x%X\n", __FUNCTION__, port_lane_info->link_speed, port_lane_info->link_state);
    }

    return status;
}

fbe_status_t fbe_cpd_shim_set_async_io(fbe_bool_t sync_io)
{
	return FBE_STATUS_OK;
}

fbe_status_t fbe_cpd_shim_set_dmrb_zeroing(fbe_bool_t dmrb_zeroing)
{
	return FBE_STATUS_OK;
}

fbe_status_t fbe_cpd_shim_get_async_io(fbe_bool_t * async_io)
{
	* async_io = FBE_FALSE;
	return FBE_STATUS_OK;
}
