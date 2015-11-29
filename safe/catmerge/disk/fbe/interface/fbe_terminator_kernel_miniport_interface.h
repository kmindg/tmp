#ifndef FBE_TERMINATOR_KERNEL_MINIPORT_INTERFACE_H
#define FBE_TERMINATOR_KERNEL_MINIPORT_INTERFACE_H

/*APIs to get/set information in the miniport*/
#include "fbe_terminator_miniport_interface.h"

typedef struct fbe_terminator_kernel_miniport_api_s
{
    fbe_status_t (*init)(void);
    fbe_status_t (*destroy)(void);
    fbe_status_t (*enumerate_cpd_ports)(fbe_cpd_shim_enumerate_backend_ports_t * cpd_shim_enumerate_backend_ports);
    fbe_status_t (*port_init)(fbe_u32_t io_port_number, fbe_u32_t io_portal_number, fbe_u32_t *port_handle);
    fbe_status_t (*port_destroy)(fbe_u32_t port_handle);
    fbe_status_t (*register_callback)(fbe_u32_t port_handle, fbe_terminator_miniport_api_callback_function_t callback_function, void * callback_context);
    fbe_status_t (*unregister_callback)(fbe_u32_t port_index);
    fbe_status_t (*register_payload_completion)(fbe_u32_t port_index, fbe_payload_ex_completion_function_t completion_function, fbe_payload_ex_completion_context_t  completion_context);
    fbe_status_t (*unregister_payload_completion)(fbe_u32_t port_index);
    fbe_status_t (*get_port_type)(fbe_u32_t port_index,fbe_port_type_t *port_type);
    fbe_status_t (*remove_port)(fbe_u32_t port_index);
    fbe_status_t (*port_inserted)(fbe_u32_t port_index);
    fbe_status_t (*set_speed)(fbe_u32_t port_index, fbe_port_speed_t speed);
    fbe_status_t (*get_port_info)(fbe_u32_t port_index, fbe_port_info_t * port_info);
    fbe_status_t (*send_payload)(fbe_u32_t port_index, fbe_cpd_device_id_t cpd_device_id, fbe_payload_ex_t * payload);
    fbe_status_t (*send_fis)(fbe_u32_t port_index, fbe_cpd_device_id_t cpd_device_id, fbe_payload_ex_t * payload);
    fbe_status_t (*start_port_reset)(fbe_u32_t port_index);
    fbe_status_t (*complete_port_reset)(fbe_u32_t port_index);
    fbe_status_t (*auto_port_reset)(fbe_u32_t port_index);
    fbe_status_t (*device_state_change_notify)(fbe_u32_t port_index);
    fbe_status_t (*reset_device)(fbe_u32_t port_index, fbe_miniport_device_id_t cpd_device_id);
    fbe_status_t (*set_global_completion_delay)(fbe_u32_t global_completion_delay);
} fbe_terminator_kernel_miniport_api_t;


#endif /*FBE_TERMINATOR_KERNEL_MINIPORT_INTERFACE_H*/
