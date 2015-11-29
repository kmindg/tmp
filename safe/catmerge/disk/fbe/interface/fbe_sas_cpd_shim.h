#error "This file is deprecated"

#ifndef FBE_FIBRE_CPD_SHIM_H
#define FBE_FIBRE_CPD_SHIM_H

#include "fbe/fbe_types.h"

#define FBE_SAS_CPD_SHIM_MAX_PORTS 24
#define FBE_MAX_EXPANDERS 10

typedef struct fbe_sas_cpd_shim_enumerate_backend_ports_s {
	fbe_u32_t number_of_io_ports;
	fbe_u32_t io_port_array[FBE_SAS_CPD_SHIM_MAX_PORTS];
}fbe_sas_cpd_shim_enumerate_backend_ports_t;

typedef struct fbe_expander_info_s {
    fbe_sas_address_t sas_address;
    fbe_ses_address_t ses_address;
    fbe_u8_t		bus;
	fbe_u8_t		target_id;
	fbe_u8_t		enclosure_num;
	fbe_u8_t		cabling_position;
	fbe_bool_t		new_entry;
}fbe_expander_info_t;

typedef struct fbe_expander_list_s {
    fbe_u8_t			num_expanders;
	fbe_expander_info_t	expander_info[FBE_MAX_EXPANDERS];
}fbe_expander_list_t;

fbe_status_t fbe_sas_cpd_port_init(void);
fbe_status_t fbe_sas_cpd_shim_enumerate_backend_ports(fbe_sas_cpd_shim_enumerate_backend_ports_t * sas_cpd_shim_enumerate_backend_ports);
fbe_status_t fbe_sas_cpd_shim_get_expander_list(fbe_u32_t portNumber, fbe_expander_list_t *expander_list);
#endif /*FBE_FIBRE_CPD_SHIM_H */
