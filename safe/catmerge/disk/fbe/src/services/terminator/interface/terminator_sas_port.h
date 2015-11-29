#ifndef TERMINATOR_SAS_PORT_H
#define TERMINATOR_SAS_PORT_H

#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_types.h"

/* These are the attributes specific to sas_port */
typedef struct terminator_sas_port_info_s{
    /* These are the user data either load from xml or passed in as input from api.*/
    fbe_sas_address_t           sas_address;
    fbe_u32_t                   io_port_number;
    fbe_u32_t                   portal_number;
    fbe_u32_t                   backend_number;
    fbe_u32_t                   miniport_sas_device_table_index;
}terminator_sas_port_info_t;

fbe_status_t fbe_terminator_sas_port_info_get_io_port_number(fbe_terminator_sas_port_info_t *port_info, fbe_u32_t *io_port_number);
fbe_status_t fbe_terminator_sas_port_info_get_portal_number(fbe_terminator_sas_port_info_t *port_info, fbe_u32_t *portal_number);
fbe_status_t fbe_terminator_sas_port_info_get_backend_number(fbe_terminator_sas_port_info_t *port_info, fbe_u32_t *backend_number);
fbe_status_t fbe_terminator_sas_port_info_get_sas_address(fbe_terminator_sas_port_info_t *port_info, fbe_sas_address_t *sas_address);

#endif /* TERMINATOR_SAS_PORT_H */