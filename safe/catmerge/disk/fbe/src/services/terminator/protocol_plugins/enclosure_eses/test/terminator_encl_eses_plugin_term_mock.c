/***************************************************************************
 *  terminator_encl_eses_plugin_term_mock.c
 ***************************************************************************
 *
 *  Description
 *      As we dont run terminator for unit testing plugins, this has
 *      mocks all the terminator related functions used in 
 *      Enclosure eses plugin code (terminator_encl_eses_plugin_main.c
 *      & others).
 *      
 *  NOTE:
 *   IMPORTANT: All the terminator interface functions used by enclosure
 *   eses module SHOULD have a DUMMY defintion here or else the build for 
 *   terminator_encl_eses_plugin_test.exe will fail with the LINK ERROR
 *   for that particular undefined terminator  function.
 *
 *  History:
 *     Oct08   Created
 *    
 ***************************************************************************/



#include "fbe_terminator.h"
#include "terminator_sas_io_api.h"


fbe_status_t terminator_virtual_phy_get_enclosure_uid(fbe_terminator_device_ptr_t handle, fbe_u8_t ** uid)
{
    return(FBE_STATUS_OK);
}

fbe_bool_t terminator_is_eses_page_supported(fbe_u8_t cdb_opcode, fbe_u8_t diag_page_code)
{
    return(FBE_TRUE);
}

fbe_status_t terminator_max_conns_per_lcc(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_conns)
{
    return(FBE_STATUS_OK);
}

fbe_bool_t terminator_phy_corresponds_to_connector(
    fbe_u8_t phy_id, 
    fbe_u8_t *connector, 
    fbe_u8_t *connector_id,
    fbe_sas_enclosure_type_t encl_type)
{
    return(FBE_TRUE);
}

fbe_bool_t terminator_phy_corresponds_to_drive_slot(fbe_u8_t phy_id, fbe_u8_t *drive_slot, fbe_sas_enclosure_type_t encl_type)
{
    return(FBE_TRUE);
}


fbe_status_t terminator_max_phys(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_phys)
{
    return(FBE_STATUS_OK);
}
fbe_status_t terminator_max_drive_slots(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_drive_slots)
{
    return(FBE_STATUS_OK);
}
fbe_status_t terminator_max_single_lane_conns_per_port(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_conns)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_max_conns_per_port(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_conns)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_max_conn_id_count(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_conn_id_count)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_max_temp_sensors_per_lcc(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_temp_sensor_elems_count)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_get_encl_sas_address_by_virtual_phy_id(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u64_t *sas_address)
{
    return(FBE_STATUS_OK);
}
fbe_status_t terminator_get_sas_address_by_slot_number_and_virtual_phy_handle(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u8_t dev_slot_num, 
    fbe_u64_t *sas_address)
{
    return(FBE_STATUS_OK);
}
fbe_status_t terminator_get_phy_eses_status (
    fbe_terminator_device_ptr_t virtual_phy_handle, 
    fbe_u32_t phy_number, 
    ses_stat_elem_exp_phy_struct *exp_phy_stat)
{
    return(FBE_STATUS_OK);
}
fbe_status_t terminator_get_drive_eses_status (
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u32_t slot_number, 
    ses_stat_elem_array_dev_slot_struct *ses_stat_elem)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_set_drive_eses_status (
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u32_t slot_number, 
    ses_stat_elem_array_dev_slot_struct ses_stat_elem)
{
    return(FBE_STATUS_OK);
}


fbe_status_t terminator_get_ps_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle, 
    terminator_eses_ps_id ps_id, 
    ses_stat_elem_ps_struct *ps_stat)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_get_sas_conn_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle, 
    terminator_eses_sas_conn_id sas_conn_id, 
    ses_stat_elem_sas_conn_struct *sas_conn_stat)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_get_cooling_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle, 
    terminator_eses_cooling_id cooling_id, 
    ses_stat_elem_cooling_struct *cooling_stat)
{
    return(FBE_STATUS_OK);
}
fbe_status_t terminator_get_temp_sensor_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle, 
    terminator_eses_temp_sensor_id temp_sensor_id,
    ses_stat_elem_temp_sensor_struct *temp_sensor_stat)
{
    return(FBE_STATUS_OK);
}
fbe_status_t terminator_get_downstream_wideport_device_status(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_bool_t *inserted)
{
    return(FBE_STATUS_OK);
}
fbe_status_t terminator_get_upstream_wideport_device_status(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_bool_t *inserted)
{
    return(FBE_STATUS_OK);
}
fbe_status_t terminator_get_child_expander_wideport_device_status_by_connector_id(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u8_t conn_id,
    fbe_bool_t *inserted)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_get_downstream_wideport_sas_address(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u64_t *sas_address)
{
    return(FBE_STATUS_OK);
}
fbe_status_t terminator_map_position_max_conns_to_range_conn_id(
    fbe_sas_enclosure_type_t encl_type,
    fbe_u8_t position, 
    fbe_u8_t max_conns, 
    terminator_conn_map_range_t *return_range,
    fbe_u8_t *conn_id)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_get_upstream_wideport_sas_address(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u64_t *sas_address)
{
    return(FBE_STATUS_OK);
}
fbe_status_t terminator_get_child_expander_wideport_sas_address_by_connector_id(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u8_t conn_id,
    fbe_u64_t *sas_address)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_get_overall_temp_sensor_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle, 
    terminator_eses_temp_sensor_id temp_sensor_id,
    ses_stat_elem_temp_sensor_struct *temp_sensor_stat)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_check_sas_enclosure_type(fbe_sas_enclosure_type_t encl_type)
{
    return(FBE_STATUS_OK);
}

void terminator_trace(fbe_u32_t x,
                      fbe_u32_t y,
                      const char * fmt, ...)
{
    return;
}

fbe_status_t terminator_virtual_phy_is_drive_slot_available(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                            fbe_u8_t drive_slot, 
                                                            fbe_bool_t *available)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_virtual_phy_logout_drive_in_slot(fbe_terminator_device_ptr_t virtual_phy_handle, 
                                                         fbe_u8_t drive_slot)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_virtual_phy_login_drive_in_slot(fbe_terminator_device_ptr_t virtual_phy_handle, 
                                                        fbe_u8_t drive_slot)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_virtual_phy_is_drive_in_slot_logged_in(fbe_terminator_device_ptr_t virtual_phy_handle, 
                                                               fbe_u8_t drive_slot,
                                                               fbe_bool_t *drive_logged_in)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_set_phy_eses_status (fbe_terminator_device_ptr_t virtual_phy_handle, 
                                             fbe_u32_t phy_number,
                                             ses_stat_elem_exp_phy_struct exp_phy_stat)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_get_eses_page_info(fbe_terminator_device_ptr_t virtual_phy_handle,
                                           terminator_vp_eses_page_info_t **vp_eses_page_info)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_virtual_phy_get_unit_attention(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                       fbe_u8_t *unit_attention)
{
    return(FBE_STATUS_OK);
}
fbe_status_t terminator_virtual_phy_set_unit_attention(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                       fbe_u8_t unit_attention)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_eses_get_download_microcode_stat_page_stat_desc(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                         fbe_download_status_desc_t *download_stat_desc)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_get_encl_eses_status(fbe_terminator_device_ptr_t virtual_phy_handle,
                                             ses_stat_elem_encl_struct *encl_stat)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_set_encl_eses_status(fbe_terminator_device_ptr_t virtual_phy_handle,
                                             ses_stat_elem_encl_struct encl_stat)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_get_chassis_encl_eses_status(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                     ses_stat_elem_encl_struct *encl_stat)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_set_chassis_encl_eses_status(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                    ses_stat_elem_encl_struct encl_stat)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_get_drive_slot_to_phy_mapping(fbe_u8_t drive_slot, fbe_u8_t *phy_id, fbe_sas_enclosure_type_t encl_type)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_eses_get_buf_info_by_buf_id(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                    fbe_u8_t buf_id,
                                                    terminator_eses_buf_info_t **buf_info)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_virtual_phy_power_cycle_lcc(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                    fbe_u32_t power_cycle_time_in_ms)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_get_display_eses_status(fbe_terminator_device_ptr_t virtual_phy_handle, 
                                               terminator_eses_display_character_id display_character_id,
                                               ses_stat_elem_display_struct *display_stat_elem)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_set_display_eses_status(fbe_terminator_device_ptr_t virtual_phy_handle, 
                                               terminator_eses_display_character_id display_character_id,
                                               ses_stat_elem_display_struct display_stat_elem)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_virtual_phy_power_cycle_drive(fbe_terminator_device_ptr_t virtual_phy_handle,
                                         fbe_u32_t power_cycle_time_in_ms,
                                         fbe_u32_t  drive_slot_number)
{

    return(FBE_STATUS_OK);
}fbe_status_t terminator_set_drive_slot_insert_count(fbe_terminator_device_ptr_t virtual_phy_handle, 
                                                    fbe_u32_t slot_number, 
                                                    fbe_u8_t insert_count)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_get_drive_slot_insert_count(fbe_terminator_device_ptr_t virtual_phy_handle, 
                                                    fbe_u32_t slot_number, 
                                                    fbe_u8_t *insert_count)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_set_drive_power_down_count(fbe_terminator_device_ptr_t virtual_phy_handle, 
                                                   fbe_u32_t slot_number, 
                                                   fbe_u8_t power_down_count)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_get_drive_power_down_count(fbe_terminator_device_ptr_t virtual_phy_handle, 
                                                   fbe_u32_t slot_number, 
                                                   fbe_u8_t *power_down_count)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_clear_drive_power_down_counts(fbe_terminator_device_ptr_t virtual_phy_handle)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_clear_drive_slot_insert_counts(fbe_terminator_device_ptr_t virtual_phy_handle)
{
    return(FBE_STATUS_OK);
}

fbe_status_t terminator_update_enclosure_drive_slot_number(fbe_terminator_device_ptr_t device_ptr, fbe_u32_t *ret_slot_number)
{
    return(FBE_STATUS_OK);
}
