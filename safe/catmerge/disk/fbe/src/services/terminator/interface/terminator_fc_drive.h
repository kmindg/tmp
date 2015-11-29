/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  terminator_fc_drive.h
 ***************************************************************************
 *
 *  Description
 *      prototypes for terminator fc drive class
 *
 *
 *  History:
 *      09/23/08    Dipak Patel    Created
 *
 ***************************************************************************/

#ifndef TERMINATOR_FC_DRIVE_H
#define TERMINATOR_FC_DRIVE_H

#include "terminator_base.h"
#include "fbe/fbe_physical_drive.h"


#define INVALID_SLOT_NUMBER 0x3C


typedef struct terminator_fc_drive_info_s{
    fbe_fc_drive_type_t drive_type;
    fbe_u8_t drive_serial_number[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE];
    fbe_u32_t backend_number;
    fbe_u32_t encl_number;
    fbe_u32_t slot_number;    
    fbe_u8_t product_id[FBE_SCSI_INQUIRY_PRODUCT_ID_SIZE + 1];
}terminator_fc_drive_info_t;


terminator_fc_drive_info_t * allocate_fc_drive_info(void);
terminator_fc_drive_info_t * fc_drive_info_new(fbe_terminator_fc_drive_info_t *fc_drive_info);
void fc_drive_set_drive_type(terminator_drive_t * self, fbe_fc_drive_type_t type);
fbe_fc_drive_type_t fc_drive_get_drive_type(terminator_drive_t * self);
void fc_drive_set_slot_number(terminator_drive_t * self, fbe_u32_t slot_number);
fbe_u32_t fc_drive_get_slot_number(terminator_drive_t * self);
fbe_status_t fc_drive_call_io_api(terminator_drive_t * self, fbe_terminator_io_t * terminator_io);
fbe_status_t fc_drive_info_get_serial_number(terminator_fc_drive_info_t * self, fbe_u8_t *serial_number);
fbe_status_t fc_drive_info_set_serial_number(terminator_fc_drive_info_t * self, fbe_u8_t *serial_number);
fbe_status_t fc_drive_info_get_product_id(terminator_fc_drive_info_t * self, fbe_u8_t *product_id);
fbe_status_t fc_drive_info_set_product_id(terminator_fc_drive_info_t * self, fbe_u8_t *product_id);


fbe_u8_t * fc_drive_get_serial_number(terminator_drive_t * self);
void fc_drive_set_serial_number(terminator_drive_t * self, const fbe_u8_t * serial_number);
fbe_u8_t * fc_drive_get_product_id(terminator_drive_t * self);
void fc_drive_set_product_id(terminator_drive_t * self, const fbe_u8_t * product_id);
void fc_drive_set_backend_number(terminator_drive_t * self, fbe_u32_t backend_number);
void fc_drive_set_enclosure_number(terminator_drive_t * self, fbe_u32_t encl_number);
fbe_u32_t fc_drive_get_backend_number(terminator_drive_t * self);
fbe_u32_t fc_drive_get_enclosure_number(terminator_drive_t * self);

#endif /* TERMINATOR_FC_DRIVE_H */
