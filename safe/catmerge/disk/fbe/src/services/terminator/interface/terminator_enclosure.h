/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

#ifndef TERMINATOR_ENCLOSURE_H
#define TERMINATOR_ENCLOSURE_H

#include "terminator_base.h"
////#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_enclosure.h"
#include "fbe_sas.h"

#define VIPER_VIRTUAL_PHY_SAS_ADDR_MASK 0xFFFFFFFFFFFFFFC0
#define VIPER_VIRTUAL_PHY_SAS_ADDR_LAST_6_BITS 0x3E
#define VIPER_MAX_PHYSICAL_PHY_ID 23
#define SAS_MAX_PHYSICAL_PHY_ID 35
#define BULLET_MAX_PHYSICAL_PHY_ID 23


typedef struct terminator_sas_enclosure_info_s{
    fbe_sas_enclosure_type_t      enclosure_type;
    fbe_cpd_shim_callback_login_t login_data;
    fbe_u8_t					  uid[FBE_SCSI_INQUIRY_ENCLOSURE_SERIAL_NUMBER_SIZE];
    fbe_u64_t                       encl_number;
    fbe_u32_t                       backend;
    fbe_u32_t                   miniport_sas_device_table_index;
    terminator_component_type_t logical_parent_type; /* type of the parent in logical, not the parent it connects to physically */
    fbe_u32_t 	                    connector_id;
}terminator_sas_enclosure_info_t;

typedef struct terminator_enclosure_s{
    base_component_t base;
    fbe_enclosure_type_t encl_protocol;
}terminator_enclosure_t;

terminator_enclosure_t * enclosure_new(void);
fbe_status_t enclosure_destroy(terminator_enclosure_t * self);

terminator_sas_enclosure_info_t * sas_enclosure_info_new(fbe_terminator_sas_encl_info_t *sas_encl_info);
terminator_sas_enclosure_info_t * allocate_sas_enclosure_info(void);


fbe_status_t sas_enclosure_set_enclosure_type(terminator_enclosure_t * self, fbe_sas_enclosure_type_t type);
fbe_sas_enclosure_type_t sas_enclosure_get_enclosure_type(terminator_enclosure_t * self);

fbe_status_t sas_enclosure_set_logical_parent_type(terminator_enclosure_t * self, terminator_component_type_t type);
terminator_component_type_t sas_enclosure_get_logical_parent_type(terminator_enclosure_t * self);

fbe_status_t sas_enclosure_set_enclosure_uid(terminator_enclosure_t * self, fbe_u8_t *uid);
fbe_u8_t *sas_enclosure_get_enclosure_uid(terminator_enclosure_t * self);

void sas_enclosure_info_set_login_data(terminator_sas_enclosure_info_t * self, fbe_cpd_shim_callback_login_t *login_data);
fbe_cpd_shim_callback_login_t * sas_enclosure_info_get_login_data(terminator_sas_enclosure_info_t * self);

terminator_enclosure_t * enclosure_get_last_enclosure(base_component_t * self);
terminator_enclosure_t * enclosure_get_last_sibling_enclosure(base_component_t * self);
fbe_status_t sas_enclosure_get_connector_id_by_enclosure(terminator_enclosure_t * self, terminator_enclosure_t * connected_encl, fbe_u32_t *connector_id);
fbe_status_t enclosure_enumerate_devices(terminator_enclosure_t * self, fbe_terminator_device_ptr_t device_list[], fbe_u32_t *total_devices);
fbe_status_t sas_enclosure_generate_virtual_phy(terminator_enclosure_t * self);
fbe_sas_address_t sas_enclosure_calculate_virtual_phy_sas_address(terminator_enclosure_t * self);
fbe_sas_address_t sas_enclosure_get_sas_address(terminator_enclosure_t * self);
fbe_status_t sas_enclosure_insert_sas_drive(terminator_enclosure_t * self, 
                                            fbe_u32_t slot_number, 
                                            fbe_terminator_sas_drive_info_t *drive_info,
                                            fbe_terminator_device_ptr_t *drive_ptr);
fbe_status_t sas_enclosure_generate_sas_virtual_phy_phy_number(fbe_u8_t *phy_number, terminator_enclosure_t * self);
fbe_status_t sas_enclosure_calculate_sas_device_phy_number(fbe_u8_t *phy_number, terminator_enclosure_t * self, fbe_u32_t slot_number);
fbe_status_t sas_enclosure_is_slot_available(terminator_enclosure_t * self, fbe_u32_t slot_number, fbe_bool_t * result);
fbe_status_t sas_enclosure_is_connector_available(terminator_enclosure_t * self, fbe_u32_t connector_id, fbe_bool_t * result);
fbe_status_t sas_enclosure_call_io_api(terminator_enclosure_t * self, fbe_terminator_io_t * terminator_io);

fbe_status_t sas_enclosure_get_device_ptr_by_slot(terminator_enclosure_t * self, fbe_u32_t slot_number, fbe_terminator_device_ptr_t *device_ptr);
//fbe_status_t sas_enclosure_max_drive_slots(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_drive_slots);
fbe_status_t sas_enclosure_set_sas_address(terminator_enclosure_t * self, fbe_sas_address_t sas_address);
fbe_status_t sas_enclosure_update_virtual_phy_sas_address(terminator_enclosure_t * self);

fbe_status_t sas_enclosure_insert_sata_drive(terminator_enclosure_t * self, 
                                            fbe_u32_t slot_number, 
                                            fbe_terminator_sata_drive_info_t *drive_info,
                                            fbe_terminator_device_ptr_t *drive_ptr);
fbe_status_t enclosure_activate(terminator_enclosure_t * self);
fbe_status_t enclosure_update_drive_and_phy_status(terminator_enclosure_t * self);
fbe_status_t sas_enclosure_set_backend_number(terminator_enclosure_t * self, fbe_u32_t backend_number);
fbe_status_t sas_enclosure_set_enclosure_number(terminator_enclosure_t * self, fbe_u32_t enclosure_number);
fbe_u32_t sas_enclosure_get_enclosure_number(terminator_enclosure_t * self);
fbe_u32_t sas_enclosure_get_backend_number(terminator_enclosure_t * self);
fbe_status_t sas_enclosure_set_connector_id(terminator_enclosure_t * self, fbe_u32_t slot_id);
fbe_u32_t sas_enclosure_get_connector_id(terminator_enclosure_t * self);
fbe_status_t enclosure_add_sas_enclosure(terminator_enclosure_t * self, terminator_enclosure_t * new_enclosure, fbe_bool_t new_virtual_phy);

fbe_status_t enclosure_set_protocol(terminator_enclosure_t * self, fbe_enclosure_type_t protocol);
fbe_enclosure_type_t enclosure_get_protocol(terminator_enclosure_t * self);
fbe_status_t terminator_enclosure_initialize_protocol_specific_data(terminator_enclosure_t * encl_handle);
fbe_status_t sas_enclosure_update_child_enclosure_chain_depth(terminator_enclosure_t * self);
fbe_u8_t sas_enclosure_get_enclosure_chain_depth(terminator_enclosure_t * self);
fbe_status_t sas_enclosure_set_enclosure_chain_depth(terminator_enclosure_t * self, fbe_u8_t depth);
fbe_u8_t sas_enclosure_handle_get_enclosure_chain_depth(fbe_terminator_api_device_handle_t enclosure_handle);
fbe_status_t sas_enclosure_handle_set_enclosure_chain_depth(fbe_terminator_api_device_handle_t enclosure_handle, fbe_u8_t depth);
#endif /* TERMINATOR_ENCLOSURE_H */
