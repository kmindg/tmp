/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  terminator_fc_enclosure.h
 ***************************************************************************
 *
 *  Description
 *      prototypes for terminator fc enclosure class
 *
 *
 *  History:
 *      09/23/08    Dipak Patel    Created
 *
 ***************************************************************************/

#ifndef TERMINATOR_FC_ENCLOSURE_H
#define TERMINATOR_FC_ENCLOSURE_H

#include "terminator_base.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_enclosure.h"


typedef struct terminator_fc_enclosure_info_s{
    fbe_fc_enclosure_type_t      enclosure_type;
    fbe_u8_t                      uid[16];
    fbe_u32_t                     encl_number;
    fbe_u32_t                     backend;
    fbe_bool_t                    encl_state_change;
}terminator_fc_enclosure_info_t;

fbe_status_t fc_enclosure_set_enclosure_type(terminator_enclosure_t * self, fbe_fc_enclosure_type_t type);
fbe_fc_enclosure_type_t fc_enclosure_get_enclosure_type(terminator_enclosure_t * self);
terminator_fc_enclosure_info_t * fc_enclosure_info_new(fbe_terminator_fc_encl_info_t *fc_encl_info);
terminator_fc_enclosure_info_t * allocate_fc_enclosure_info(void);
fbe_status_t fc_enclosure_set_enclosure_uid(terminator_enclosure_t * self, fbe_u8_t *uid);
fbe_u8_t *fc_enclosure_get_enclosure_uid(terminator_enclosure_t * self);
terminator_enclosure_t * enclosure_get_last_enclosure(base_component_t * self);
fbe_status_t enclosure_enumerate_devices(terminator_enclosure_t * self, fbe_terminator_device_ptr_t device_list[], fbe_u32_t *total_devices);
fbe_diplex_address_t fc_enclosure_get_diplex_address(terminator_enclosure_t * self);
fbe_status_t fc_enclosure_insert_fc_drive(terminator_enclosure_t * self, 
                                            fbe_u32_t slot_number, 
                                            fbe_terminator_fc_drive_info_t *drive_info,
                                            fbe_terminator_device_ptr_t *drive_ptr);

fbe_status_t fc_enclosure_is_slot_available(terminator_enclosure_t * self, fbe_u32_t slot_number, fbe_bool_t * result);
fbe_status_t fc_enclosure_call_io_api(terminator_enclosure_t * self, fbe_terminator_io_t * terminator_io);

fbe_status_t fc_enclosure_get_device_ptr_by_slot(terminator_enclosure_t * self, fbe_u32_t slot_number, fbe_terminator_device_ptr_t *device_ptr);
fbe_status_t fc_enclosure_set_backend_number(terminator_enclosure_t * self, fbe_u32_t backend_number);
fbe_status_t fc_enclosure_set_enclosure_number(terminator_enclosure_t * self, fbe_u32_t enclosure_number);
fbe_u32_t fc_enclosure_get_enclosure_number(terminator_enclosure_t * self);
fbe_u32_t fc_enclosure_get_backend_number(terminator_enclosure_t * self);
fbe_status_t fc_enclosure_set_state_change(terminator_enclosure_t * self, fbe_bool_t encl_state_change);
fbe_status_t fc_enclosure_get_state_change(terminator_enclosure_t * self, fbe_bool_t *encl_state_change);

#endif /* TERMINATOR_FC_ENCLOSURE_H */
