#ifndef FBE_FIBRE_CPD_SHIM_H
#define FBE_FIBRE_CPD_SHIM_H

#include "fbe/fbe_types.h"
#include "fbe_fibre.h"

#include "fbe/fbe_port.h"
#include "fbe_cpd_shim.h"

#define FBE_FIBRE_CPD_SHIM_MAX_PORTS 24

typedef struct fbe_fibre_cpd_shim_enumerate_backend_ports_s {
	fbe_u32_t number_of_io_ports;
	fbe_u32_t io_port_array[FBE_FIBRE_CPD_SHIM_MAX_PORTS];
}fbe_fibre_cpd_shim_enumerate_backend_ports_t;


fbe_status_t fbe_fibre_cpd_init(void);
fbe_status_t fbe_fibre_cpd_destroy(void);

fbe_status_t 
fbe_fibre_cpd_port_init(fbe_u32_t port_number, 
						fbe_u32_t io_port_number, 
						fbe_io_block_completion_function_t completion_function,
						fbe_io_block_completion_context_t  completion_context,
						fbe_cpd_shim_callback_function_t callback_function,
						fbe_cpd_shim_callback_context_t callback_context);

fbe_status_t fbe_fibre_cpd_port_destroy(fbe_u32_t port_number);

fbe_status_t fbe_fibre_cpd_shim_login_drive(fbe_u32_t port_number, fbe_cpd_device_id_t cpd_device_id);
fbe_status_t fbe_fibre_cpd_shim_logout_drive(fbe_u32_t port_number, fbe_cpd_device_id_t cpd_device_id);

fbe_status_t fbe_fibre_cpd_shim_enumerate_backend_ports(fbe_fibre_cpd_shim_enumerate_backend_ports_t * fibre_cpd_shim_enumerate_backend_ports);

fbe_status_t fbe_fibre_cpd_shim_send_io(fbe_u32_t port_number, fbe_io_block_t * io_block);

fbe_status_t fbe_fibre_cpd_shim_change_speed(fbe_u32_t port_number, fbe_port_speed_t speed);
fbe_status_t fbe_fibre_cpd_shim_get_port_info(fbe_u32_t port_number, fbe_port_info_t * port_info);

#endif /*FBE_FIBRE_CPD_SHIM_H */
