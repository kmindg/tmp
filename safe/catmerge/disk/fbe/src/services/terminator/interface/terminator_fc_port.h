/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  terminator_fc_port.h
 ***************************************************************************
 *
 *  Description
 *      prototypes for terminator fc port class
 *
 *
 *  History:
 *      09/23/08    Dipak Patel    Created
 *
 ***************************************************************************/

#ifndef TERMINATOR_FC_PORT_H
#define TERMINATOR_FC_PORT_H

#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_types.h"

/* These are the attributes specific to fc_port */
typedef struct terminator_fc_port_info_s{
    /* These are the user data either load from xml or passed in as input from api.*/
    fbe_diplex_address_t        diplex_address;
    fbe_u32_t                   io_port_number;
    fbe_u32_t                   portal_number;
    fbe_u32_t                   backend_number;
}terminator_fc_port_info_t;

fbe_status_t fbe_terminator_fc_port_info_get_io_port_number(fbe_terminator_fc_port_info_t *port_info, fbe_u32_t *io_port_number);
fbe_status_t fbe_terminator_fc_port_info_get_portal_number(fbe_terminator_fc_port_info_t *port_info, fbe_u32_t *portal_number);
fbe_status_t fbe_terminator_fc_port_info_get_backend_number(fbe_terminator_fc_port_info_t *port_info, fbe_u32_t *backend_number);
fbe_status_t fbe_terminator_fc_port_info_get_diplex_address(fbe_terminator_fc_port_info_t *port_info, fbe_diplex_address_t *diplex_address);

terminator_fc_port_info_t *    allocate_fc_port_info(void);

#endif /* TERMINATOR_FC_PORT_H */
