#ifndef TERMINATOR_LOGIN_DATA_H
#define TERMINATOR_LOGIN_DATA_H
#include "fbe/fbe_winddk.h"
#include "fbe_cpd_shim.h"
#include "fbe_terminator.h"

#define INVALID_LOCATOR_DATA -1

/* Init callback_login data structure */
void cpd_shim_callback_login_init(fbe_cpd_shim_callback_login_t * login_data);
/* device_type setter */
void cpd_shim_callback_login_set_device_type(fbe_cpd_shim_callback_login_t * login_data, fbe_cpd_shim_discovery_device_type_t device_type);
/* device_type getter*/
fbe_cpd_shim_discovery_device_type_t cpd_shim_callback_login_get_device_type(fbe_cpd_shim_callback_login_t * login_data);
/* device_id setter */
void cpd_shim_callback_login_set_device_id(fbe_cpd_shim_callback_login_t * login_data, fbe_miniport_device_id_t device_id);
/* device_id getter*/
fbe_miniport_device_id_t cpd_shim_callback_login_get_device_id(fbe_cpd_shim_callback_login_t * login_data);
/* device_address setter*/
void cpd_shim_callback_login_set_device_address(fbe_cpd_shim_callback_login_t * login_data, fbe_sas_address_t device_address);
/* device_address getter*/
fbe_sas_address_t cpd_shim_callback_login_get_device_address(fbe_cpd_shim_callback_login_t * login_data);
/* parent_device_id setter*/
void cpd_shim_callback_login_set_parent_device_id(fbe_cpd_shim_callback_login_t * login_data, fbe_miniport_device_id_t device_id);
/* parent_device_id getter*/
fbe_miniport_device_id_t cpd_shim_callback_login_get_parent_device_id(fbe_cpd_shim_callback_login_t * login_data);
/* parent_device_address setter*/
void cpd_shim_callback_login_set_parent_device_address(fbe_cpd_shim_callback_login_t * login_data, fbe_sas_address_t device_address);
/* parent_device_address getter*/
fbe_sas_address_t cpd_shim_callback_login_get_parent_device_address(fbe_cpd_shim_callback_login_t * login_data);
/* Initialize the device_locator */
void cpd_shim_callback_login_init_device_locator(fbe_cpd_shim_callback_login_t * login_data);
/* device_locator setter*/
void cpd_shim_callback_login_set_device_locator(fbe_cpd_shim_callback_login_t * login_data, fbe_cpd_shim_device_locator_t * device_locator);
/* enclosure_chain_depth getter*/
fbe_cpd_shim_device_locator_t * cpd_shim_callback_login_get_device_locator(fbe_cpd_shim_callback_login_t * login_data);
/* enclosure_chain_depth setter*/
void cpd_shim_callback_login_set_enclosure_chain_depth(fbe_cpd_shim_callback_login_t * login_data, fbe_u8_t enclosure_chain_depth);
/* enclosure_chain_depth getter*/
fbe_u8_t cpd_shim_callback_login_get_enclosure_chain_depth(fbe_cpd_shim_callback_login_t * login_data);
/* enclosure_chain_width setter*/
void cpd_shim_callback_login_set_enclosure_chain_width(fbe_cpd_shim_callback_login_t * login_data, fbe_u8_t enclosure_chain_width);
/* enclosure_chain_width getter*/
fbe_u8_t cpd_shim_callback_login_get_enclosure_chain_width(fbe_cpd_shim_callback_login_t * login_data);
/* phy_number setter*/
void cpd_shim_callback_login_set_phy_number(fbe_cpd_shim_callback_login_t * login_data, fbe_u8_t phy_number);
/* phy_number getter*/
fbe_u8_t cpd_shim_callback_login_get_phy_number(fbe_cpd_shim_callback_login_t * login_data);

#endif
