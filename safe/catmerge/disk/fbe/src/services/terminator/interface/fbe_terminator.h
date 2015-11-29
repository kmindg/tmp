#ifndef FBE_TERMINATOR_H
#define FBE_TERMINATOR_H

#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe_sas.h"
#include "fbe_cpd_shim.h"
#include "fbe_terminator_miniport_interface.h"
#include "terminator_sas_io_api.h"
#include "fbe/fbe_eses.h"
#include "fbe_terminator_common.h"
#include "specl_sfi_types.h"

#define MAX_DEVICES (960 + 64 + 8)
#define FBE_TERMINATOR_PORT_SHIM_MAX_PORTS FBE_PMC_SHIM_MAX_PORTS
#define DEVICE_HANDLE_INVALID NULL
#define CPD_DEVICE_ID_INVALID -1
#define ELEM_STAT_CODE_MASK 0xF

#define TERMINATOR_ARRAY_SIZE(array)    (sizeof(array) / sizeof(array[0]))

#define  RETURN_ON_ERROR_STATUS \
    if(status != FBE_STATUS_OK) \
    {                           \
        return(status);         \
    }                           \

#define  RETURN_EMA_NOT_READY_ON_ERROR_STATUS            \
    if(status != FBE_STATUS_OK)                          \
    {                                                    \
        ema_not_ready(sense_buffer);                     \
        terminator_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s:%d\n", __FUNCTION__, __LINE__); \
        return(status);                                  \
    }                                                    \

#define CHECK_AND_COPY_STRUCTURE(DES_P, SOU_P) \
    if(DES_P == NULL || SOU_P == NULL) \
    { \
        return FBE_STATUS_GENERIC_FAILURE; \
    } \
    memcpy(DES_P, SOU_P, sizeof(*(SOU_P)));

typedef struct fbe_terminator_port_shim_backend_port_info_s {
    union
    {
        fbe_terminator_sas_port_info_t sas_info;
        fbe_terminator_fc_port_info_t fc_info;
    }type_specific_info;
    fbe_u32_t       port_number;
    fbe_u32_t       portal_number;
    fbe_u32_t       backend_port_number;
    fbe_port_type_t port_type;
    fbe_cpd_shim_connect_class_t connect_class;
    fbe_cpd_shim_port_role_t     port_role;
    fbe_cpd_shim_hardware_info_t hdw_info;
}fbe_terminator_port_shim_backend_port_info_t;

typedef enum terminator_component_type_e {
    TERMINATOR_COMPONENT_TYPE_INVALID,
    TERMINATOR_COMPONENT_TYPE_BOARD,
    TERMINATOR_COMPONENT_TYPE_PORT,
    TERMINATOR_COMPONENT_TYPE_ENCLOSURE,
    TERMINATOR_COMPONENT_TYPE_DRIVE,
    TERMINATOR_COMPONENT_TYPE_VIRTUAL_PHY,
    TERMINATOR_COMPONENT_TYPE_LAST
}terminator_component_type_t;

typedef enum terminator_component_state_e {
    TERMINATOR_COMPONENT_STATE_INITIALIZED,
    TERMINATOR_COMPONENT_STATE_LOGIN_PENDING,       // Object is pending login
    TERMINATOR_COMPONENT_STATE_LOGIN_COMPLETE,      // Once login is generated transitions to this state 
    TERMINATOR_COMPONENT_STATE_LOGOUT_PENDING,      // Not much use, but set when object in logout queue
    TERMINATOR_COMPONENT_STATE_LOGOUT_COMPLETE,     // Once logout is generated transitions to this state
    TERMINATOR_COMPONENT_STATE_NOT_PRESENT          // To simulate the scenario where the Physical HW 
}terminator_component_state_t;


// This is used when mapping a connector into a range
// We really do care about the values of these functions.  In
// particular they are assigned to connector_ids. Don't change
// the order/values unless you are clear on the usage within
// the physical package.
typedef enum terminator_conn_map_range_e
{
    CONN_IS_ILLEGAL = -1,
    CONN_IS_DOWNSTREAM = 0,
    CONN_IS_UPSTREAM,
    CONN_IS_RANGE0,
    CONN_IS_INTERNAL_RANGE1,  
    CONN_IS_INTERNAL_RANGE2,  // Not really used currently.
} terminator_conn_map_range_t;



fbe_status_t terminator_init (void);
fbe_status_t terminator_destroy(void);
fbe_status_t terminator_insert_board (fbe_terminator_board_info_t *board_info);
fbe_status_t terminator_remove_board (void);
//fbe_status_t terminator_insert_sas_port (fbe_u32_t port_number, fbe_terminator_sas_port_info_t *port_info);
//fbe_status_t terminator_insert_sas_enclosure (fbe_u32_t port_number, fbe_terminator_sas_encl_info_t *encl_info, fbe_terminator_device_ptr_t *encl_ptr);
fbe_status_t terminator_get_board_type(fbe_board_type_t *board_type);
fbe_status_t terminator_get_board_info(fbe_terminator_board_info_t *board_info);
fbe_status_t terminator_pop_specl_sfi_mask_data(PSPECL_SFI_MASK_DATA specl_sfi_mask_data_ptr);
fbe_status_t terminator_push_specl_sfi_mask_data(PSPECL_SFI_MASK_DATA specl_sfi_mask_data_ptr);
fbe_status_t terminator_get_specl_sfi_mask_data_queue_count(fbe_u32_t *cnt);
fbe_status_t terminator_enumerate_ports (fbe_terminator_port_shim_backend_port_info_t port_list[], fbe_u32_t *total_ports);
fbe_status_t terminator_enumerate_devices (fbe_terminator_device_ptr_t handle, fbe_terminator_device_ptr_t device_list[], fbe_u32_t *total_devices);
fbe_status_t terminator_get_device_login_data (fbe_terminator_device_ptr_t device_ptr, fbe_cpd_shim_callback_login_t *login_data);
fbe_status_t terminator_get_port_type (fbe_terminator_device_ptr_t handle, fbe_port_type_t *port_type);
fbe_status_t terminator_get_port_address (fbe_terminator_device_ptr_t handle, fbe_address_t *port_address);
fbe_status_t terminator_is_in_logout_state (fbe_terminator_device_ptr_t device_ptr, fbe_bool_t *logout_state);
fbe_status_t terminator_is_login_pending (fbe_terminator_device_ptr_t device_ptr, fbe_bool_t *pending);
fbe_status_t terminator_clear_login_pending (fbe_terminator_device_ptr_t device_ptr);
//fbe_status_t terminator_set_login_pending (fbe_u32_t port_number, fbe_terminator_device_ptr_t device_ptr);
fbe_status_t terminator_set_device_logout_pending (fbe_terminator_device_ptr_t device_ptr);
fbe_status_t terminator_set_speed (fbe_terminator_device_ptr_t handle, fbe_port_speed_t speed);
fbe_status_t terminator_get_port_info (fbe_terminator_device_ptr_t handle, fbe_port_info_t * port_info);
fbe_status_t terminator_get_port_link_info (fbe_terminator_device_ptr_t handle, fbe_cpd_shim_port_lane_info_t *port_link_info);
fbe_status_t terminator_register_keys (fbe_terminator_device_ptr_t handle, fbe_base_port_mgmt_register_keys_t * port_register_keys_p);
fbe_status_t terminator_reestablish_key_handle(fbe_terminator_device_ptr_t handle, 
                                               fbe_base_port_mgmt_reestablish_key_handle_t * port_reestablish_key_handle_p);
fbe_status_t terminator_unregister_keys (fbe_terminator_device_ptr_t handle, fbe_base_port_mgmt_unregister_keys_t * port_unregister_keys_p);

//fbe_status_t terminator_logout_all_devices(fbe_terminator_device_ptr_t handle);
fbe_status_t terminator_set_device_login_pending (fbe_terminator_device_ptr_t component_handle);
fbe_status_t terminator_get_hardware_info (fbe_terminator_device_ptr_t handle, fbe_cpd_shim_hardware_info_t *hardware_info);
fbe_status_t terminator_get_sfp_media_interface_info (fbe_terminator_device_ptr_t handle, fbe_cpd_shim_sfp_media_interface_info_t *sfp_media_interface_info);
fbe_status_t terminator_get_port_configuration (fbe_terminator_device_ptr_t handle,  fbe_cpd_shim_port_configuration_t *port_config_info);

fbe_status_t terminator_insert_sas_drive(fbe_terminator_device_ptr_t encl_ptr,
                                         fbe_u32_t slot_number,
                                         fbe_terminator_sas_drive_info_t *drive_info,
                                         fbe_terminator_device_ptr_t *drive_ptr);
fbe_status_t terminator_create_sas_drive(fbe_terminator_device_ptr_t encl_ptr,
                                         fbe_u32_t slot_number,
                                         fbe_terminator_sas_drive_info_t *drive_info,
                                         fbe_terminator_device_ptr_t *drive_ptr);
fbe_status_t terminator_get_device_type (fbe_terminator_device_ptr_t device_ptr, terminator_component_type_t *device_type);
fbe_status_t terminator_send_io(fbe_terminator_device_ptr_t handle, fbe_terminator_io_t *terminator_io);
fbe_status_t terminator_get_front_device_from_logout_queue(fbe_terminator_device_ptr_t port_ptr, 
                                                           fbe_terminator_device_ptr_t *device_ptr);
fbe_status_t terminator_get_next_device_from_logout_queue(fbe_terminator_device_ptr_t handle,
                                                          fbe_terminator_device_ptr_t device_id,
                                                          fbe_terminator_device_ptr_t *next_device_id);
fbe_status_t terminator_pop_device_from_logout_queue(fbe_terminator_device_ptr_t handle,
                                                     fbe_terminator_device_ptr_t *device_id);
fbe_status_t terminator_set_device_logout_complete(fbe_terminator_device_ptr_t device_ptr, fbe_bool_t flag);
fbe_status_t terminator_get_device_logout_complete(fbe_terminator_device_ptr_t device_ptr, fbe_bool_t *flag);
fbe_status_t terminator_destroy_device(fbe_terminator_device_ptr_t device_ptr);
fbe_status_t terminator_unmount_device(fbe_terminator_device_ptr_t device_ptr);

fbe_status_t terminator_set_drive_eses_status (
    fbe_terminator_device_ptr_t virtual_phy_handle, 
    fbe_u32_t slot_number, 
    ses_stat_elem_array_dev_slot_struct ses_stat_elem);
fbe_status_t terminator_get_drive_eses_status (
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u32_t slot_number, 
    ses_stat_elem_array_dev_slot_struct *ses_stat_elem);

fbe_status_t terminator_destroy_all_devices(fbe_terminator_device_ptr_t handle);
fbe_status_t terminator_abort_all_io(fbe_terminator_device_ptr_t device_ptr);
fbe_status_t fbe_terminator_get_port_device_id(fbe_u32_t port_number,
                                         fbe_terminator_api_device_handle_t *device_id);
//fbe_status_t terminator_get_sas_enclosure_device_id(fbe_u32_t port_number,
//                                                    fbe_u8_t enclosure_chain_depth,
//                                                    fbe_terminator_api_device_handle_t *device_id);
fbe_status_t terminator_get_sas_drive_device_ptr(fbe_terminator_device_ptr_t encl_ptr, fbe_u32_t slot_number, fbe_terminator_device_ptr_t *drive_ptr);
fbe_status_t terminator_get_drive_slot_number(fbe_terminator_device_ptr_t device_ptr, fbe_u32_t *slot_number);
fbe_status_t terminator_get_drive_enclosure_number(fbe_terminator_device_ptr_t device_ptr, fbe_u32_t *enclosure_number);

//fbe_status_t terminator_get_virtual_phy_id_by_encl_id(fbe_u32_t port_number, fbe_terminator_api_device_handle_t enclosure_id, fbe_terminator_api_device_handle_t *virtual_phy_id);

fbe_status_t terminator_get_phy_eses_status (
    fbe_terminator_device_ptr_t virtual_phy_handle, 
    fbe_u32_t phy_number, 
    ses_stat_elem_exp_phy_struct *exp_phy_stat);
fbe_status_t terminator_set_phy_eses_status (
    fbe_terminator_device_ptr_t virtual_phy_handle, 
    fbe_u32_t phy_number, 
    ses_stat_elem_exp_phy_struct exp_phy_stat);

fbe_status_t terminator_get_sas_address_by_slot_number_and_virtual_phy_handle(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u8_t dev_slot_num, 
    fbe_u64_t *sas_address);
fbe_status_t terminator_get_sas_drive_info(fbe_terminator_device_ptr_t device_ptr,
                                           fbe_terminator_sas_drive_info_t  *drive_info);
fbe_status_t terminator_set_sas_drive_info(fbe_terminator_device_ptr_t device_ptr,
                                           fbe_terminator_sas_drive_info_t  *drive_info);
fbe_status_t terminator_get_fc_drive_info(fbe_terminator_device_ptr_t device_ptr,
                                           fbe_terminator_fc_drive_info_t *drive_info);
fbe_status_t terminator_get_sata_drive_info(fbe_terminator_device_ptr_t device_ptr,
                                            fbe_terminator_sata_drive_info_t  *drive_info);
fbe_status_t terminator_set_sata_drive_info(fbe_terminator_device_ptr_t device_ptr,
                                            fbe_terminator_sata_drive_info_t  *drive_info);
fbe_status_t terminator_set_drive_product_id(fbe_terminator_device_ptr_t device_ptr,
                                            const fbe_u8_t * product_id);
fbe_status_t terminator_get_drive_product_id(fbe_terminator_device_ptr_t device_ptr,
                                            fbe_u8_t * product_id);
fbe_status_t terminator_verify_drive_product_id(fbe_terminator_device_ptr_t device_ptr,
                                                fbe_u8_t * product_id);
fbe_status_t fbe_terminator_wait_on_port_logout_complete(fbe_terminator_device_ptr_t handle);
fbe_status_t terminator_set_reset_completed(fbe_terminator_device_ptr_t handle);
fbe_status_t terminator_set_reset_begin(fbe_terminator_device_ptr_t handle);
fbe_status_t terminator_clear_reset_completed(fbe_terminator_device_ptr_t handle);
fbe_status_t terminator_clear_reset_begin(fbe_terminator_device_ptr_t handle);
fbe_status_t terminator_get_reset_completed(fbe_terminator_device_ptr_t handle, fbe_bool_t *reset_completed);
fbe_status_t terminator_get_reset_begin(fbe_terminator_device_ptr_t handle, fbe_bool_t *reset_begin);
fbe_status_t fbe_terminator_wait_on_port_reset_clear(fbe_terminator_device_ptr_t handle,
    fbe_cpd_shim_driver_reset_event_type_t reset_event);
fbe_status_t fbe_terminator_logout_all_devices_on_port(fbe_terminator_device_ptr_t handle);
fbe_status_t fbe_terminator_login_all_devices_on_port(fbe_terminator_device_ptr_t handle);
//fbe_status_t fbe_terminator_get_sas_enclosure_info(fbe_u32_t port_number,
//                                               fbe_terminator_api_device_handle_t id,
//                                               fbe_terminator_sas_encl_info_t   *encl_info);
//fbe_status_t fbe_terminator_set_sas_enclosure_info(fbe_u32_t port_number,
//                                               fbe_terminator_api_device_handle_t id,
//                                               fbe_terminator_sas_encl_info_t   *encl_info);
fbe_status_t terminator_get_downstream_wideport_device_status(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_bool_t *inserted);
fbe_status_t terminator_get_upstream_wideport_device_status(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_bool_t *inserted);
fbe_status_t terminator_get_child_expander_wideport_device_status_by_connector_id(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u8_t conn_id,
    fbe_bool_t *inserted);
fbe_status_t terminator_get_upstream_wideport_sas_address(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u64_t *sas_address);
fbe_status_t terminator_get_downstream_wideport_sas_address(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u64_t *sas_address);
fbe_status_t terminator_map_position_max_conns_to_range_conn_id(
    fbe_sas_enclosure_type_t encl_type,
    fbe_u8_t position, 
    fbe_u8_t max_conns,
    terminator_conn_map_range_t *return_range, 
    fbe_u8_t *conn_id);
fbe_status_t terminator_get_child_expander_wideport_sas_address_by_connector_id(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u8_t conn_id,
    fbe_u64_t *sas_address);
fbe_status_t fbe_terminator_remove_device (fbe_terminator_device_ptr_t device_ptr);
fbe_status_t fbe_terminator_unmount_device (fbe_terminator_device_ptr_t device_ptr);
fbe_status_t fbe_terminator_remove_drive(fbe_terminator_device_ptr_t drive_ptr);
fbe_status_t fbe_terminator_pull_drive(fbe_terminator_device_ptr_t drive_ptr);
fbe_status_t fbe_terminator_reinsert_drive(fbe_terminator_api_device_handle_t enclosure_handle,
										  fbe_u32_t slot_number,
										  fbe_terminator_api_device_handle_t drive_handle);
fbe_bool_t terminator_phy_corresponds_to_drive_slot(fbe_u8_t phy_id, fbe_u8_t *drive_slot, fbe_sas_enclosure_type_t encl_type);
fbe_bool_t terminator_phy_corresponds_to_connector(
    fbe_u8_t phy_id, 
    fbe_u8_t *connector, 
    fbe_u8_t *connector_id,
    fbe_sas_enclosure_type_t encl_type);

fbe_status_t fbe_terminator_set_io_mode(fbe_terminator_io_mode_t io_mode);
fbe_status_t fbe_terminator_get_io_mode(fbe_terminator_io_mode_t *io_mode);

fbe_status_t fbe_terminator_set_io_completion_at_dpc(fbe_bool_t b_is_io_completion_at_dpc);
fbe_status_t fbe_terminator_get_io_completion_at_dpc(fbe_bool_t *b_is_io_completion_at_dpc_p);

fbe_status_t fbe_terminator_set_io_global_completion_delay(fbe_u32_t global_completion_delay);

fbe_status_t fbe_terminator_set_io_global_completion_delay(fbe_u32_t global_completion_delay);

//fbe_status_t termiantor_destroy_disk_files(fbe_u32_t port_number);
fbe_status_t terminator_get_encl_sas_address_by_virtual_phy_id(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    fbe_u64_t *sas_address);
fbe_status_t terminator_get_drive_slot_to_phy_mapping(fbe_u8_t drive_slot, fbe_u8_t *phy_id, fbe_sas_enclosure_type_t encl_type);

fbe_status_t terminator_set_ps_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle, 
    terminator_eses_ps_id ps_id, 
    ses_stat_elem_ps_struct ps_stat);
fbe_status_t terminator_get_ps_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle, 
    terminator_eses_ps_id ps_id, 
    ses_stat_elem_ps_struct *ps_stat);

fbe_status_t terminator_get_emcEnclStatus (fbe_terminator_device_ptr_t virtual_phy_handle,
                                           ses_pg_emc_encl_stat_struct *emcEnclStatusPtr);
fbe_status_t terminator_set_emcEnclStatus (fbe_terminator_device_ptr_t virtual_phy_handle,
                                           ses_pg_emc_encl_stat_struct *emcEnclStatusPtr);
fbe_status_t terminator_get_emcPsInfoStatus (fbe_terminator_device_ptr_t virtual_phy_handle,
                                             ses_ps_info_elem_struct *emcPsInfoStatusPtr);
fbe_status_t terminator_set_emcPsInfoStatus (fbe_terminator_device_ptr_t virtual_phy_handle,
                                             ses_ps_info_elem_struct *emcPsInfoStatusPtr);
fbe_status_t terminator_get_emcGeneralInfoDirveSlotStatus (fbe_terminator_device_ptr_t virtual_phy_handle,
                                             ses_general_info_elem_array_dev_slot_struct *emcGeneralInfoDirveSlotStatusPtr,
                                             fbe_u8_t drive_slot);
fbe_status_t terminator_set_emcGeneralInfoDirveSlotStatus (fbe_terminator_device_ptr_t virtual_phy_handle,
                                             ses_general_info_elem_array_dev_slot_struct *emcGeneralInfoDirveSlotStatusPtr,
                                             fbe_u8_t drive_slot);

fbe_status_t terminator_set_sas_conn_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    terminator_eses_sas_conn_id sas_conn_id,
    ses_stat_elem_sas_conn_struct sas_conn_stat);
fbe_status_t terminator_get_sas_conn_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle,
    terminator_eses_sas_conn_id sas_conn_id,
    ses_stat_elem_sas_conn_struct *sas_conn_stat);

fbe_status_t terminator_set_cooling_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle, 
    terminator_eses_cooling_id cooling_id, 
    ses_stat_elem_cooling_struct cooling_stat);
fbe_status_t terminator_get_cooling_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle, 
    terminator_eses_cooling_id cooling_id, 
    ses_stat_elem_cooling_struct *cooling_stat);

fbe_status_t terminator_get_temp_sensor_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle, 
    terminator_eses_temp_sensor_id temp_sensor_id,
    ses_stat_elem_temp_sensor_struct *temp_sensor_stat);
fbe_status_t terminator_set_temp_sensor_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle, 
    terminator_eses_temp_sensor_id temp_sensor_id,
    ses_stat_elem_temp_sensor_struct temp_sensor_stat);
fbe_status_t terminator_get_overall_temp_sensor_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle, 
    terminator_eses_temp_sensor_id temp_sensor_id,
    ses_stat_elem_temp_sensor_struct *temp_sensor_stat);
fbe_status_t terminator_set_overall_temp_sensor_eses_status(
    fbe_terminator_device_ptr_t virtual_phy_handle, 
    terminator_eses_temp_sensor_id temp_sensor_id,
    ses_stat_elem_temp_sensor_struct temp_sensor_stat);

fbe_status_t terminator_virtual_phy_get_enclosure_type(fbe_terminator_device_ptr_t handle, fbe_sas_enclosure_type_t *encl_type);
fbe_status_t terminator_virtual_phy_get_enclosure_uid(fbe_terminator_device_ptr_t handle, fbe_u8_t ** uid);

fbe_status_t terminator_get_port_index(fbe_terminator_device_ptr_t component_handle, fbe_u32_t * port_number);
fbe_status_t terminator_get_port_ptr(fbe_terminator_device_ptr_t component_handle, 
                                        fbe_terminator_device_ptr_t *matching_port_handle);
fbe_status_t terminator_get_enclosure_virtual_phy_ptr(fbe_terminator_device_ptr_t enclosure_ptr, fbe_terminator_device_ptr_t * virtual_phy_handle);
fbe_status_t terminator_max_drive_slots(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_drive_slots);
fbe_status_t terminator_max_phys(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_phys);
fbe_status_t terminator_max_lccs(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_lccs);
fbe_status_t terminator_max_ee_lccs(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_ee_lccs);
fbe_status_t terminator_max_ext_cooling_elems(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_ext_cooling_elems);
fbe_status_t terminator_max_bem_cooling_elems(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_bem_cooling_elems);
fbe_status_t terminator_max_conns_per_lcc(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_conns);
fbe_status_t terminator_max_conns_per_lcc(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_conns);
fbe_status_t terminator_max_single_lane_conns_per_port(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_conns);
fbe_status_t terminator_max_conns_per_port(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_conns);
fbe_status_t terminator_max_conn_id_count(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_conn_id_count);
fbe_status_t terminator_max_temp_sensors_per_lcc(fbe_sas_enclosure_type_t encl_type, fbe_u8_t *max_temp_sensor_elems_count);
void terminator_free_mem_on_not_null(void *ptr);

fbe_status_t terminator_drive_get_type(fbe_terminator_device_ptr_t handle, fbe_sas_drive_type_t *drive_type);
fbe_status_t terminator_drive_get_serial_number(fbe_terminator_device_ptr_t handle, fbe_u8_t **serial_number);

fbe_status_t terminator_drive_get_state(fbe_terminator_device_ptr_t handle, terminator_sas_drive_state_t * drive_state);
fbe_status_t terminator_drive_set_state(fbe_terminator_device_ptr_t handle, terminator_sas_drive_state_t  drive_state);
fbe_status_t terminator_sas_drive_get_default_page_info(fbe_sas_drive_type_t drive_type, fbe_terminator_sas_drive_type_default_info_t *default_info_p);
fbe_status_t terminator_sas_drive_set_default_page_info(fbe_sas_drive_type_t drive_type, const fbe_terminator_sas_drive_type_default_info_t *default_info_p);
fbe_status_t terminator_sas_drive_set_default_field(fbe_sas_drive_type_t drive_type, fbe_terminator_drive_default_field_t field, fbe_u8_t *data, fbe_u32_t size);

fbe_status_t terminator_drive_increment_error_count(fbe_terminator_device_ptr_t handle);
//fbe_status_t terminator_drive_get_error_count(fbe_u32_t port_number,
//                                              fbe_terminator_api_device_handle_t drive_id,
//                                              fbe_u32_t *const error_count_p);
//fbe_status_t terminator_drive_clear_error_count(fbe_u32_t port_number,
//                                                fbe_terminator_api_device_handle_t drive_id);

//fbe_status_t terminator_get_device_id(fbe_terminator_device_ptr_t component_handle,
//                                      fbe_terminator_api_device_handle_t *device_id);
fbe_bool_t terminator_is_eses_page_supported(fbe_u8_t cdb_opcode, fbe_u8_t diag_page_code);
fbe_status_t terminator_mark_eses_page_unsupported(fbe_u8_t cdb_opcode, fbe_u8_t diag_page_code);

fbe_status_t terminator_lock_port(fbe_terminator_device_ptr_t port_handle);
fbe_status_t terminator_unlock_port(fbe_terminator_device_ptr_t port_handle);

fbe_status_t terminator_get_port_ptr_by_port_portal(fbe_u32_t io_port_number, fbe_u32_t io_portal_number, fbe_terminator_device_ptr_t *handle);

fbe_status_t terminator_virtual_phy_is_drive_slot_available(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                            fbe_u8_t drive_slot,
                                                            fbe_bool_t *available);
fbe_status_t terminator_virtual_phy_logout_drive_in_slot(fbe_terminator_device_ptr_t virtual_phy_handle, 
                                                         fbe_u8_t drive_slot);
fbe_status_t terminator_virtual_phy_login_drive_in_slot(fbe_terminator_device_ptr_t virtual_phy_handle, 
                                                        fbe_u8_t drive_slot);
fbe_status_t terminator_check_sas_enclosure_type(fbe_sas_enclosure_type_t encl_type);
fbe_status_t terminator_virtual_phy_is_drive_in_slot_logged_in(fbe_terminator_device_ptr_t virtual_phy_handle, 
                                                               fbe_u8_t drive_slot,
                                                               fbe_bool_t *drive_logged_in);
//terminator_component_state_t terminator_get_device_state(fbe_u32_t port_number,
//                                                         fbe_terminator_api_device_handle_t device_id,
//                                                         terminator_component_state_t *state);
fbe_status_t terminator_eses_increment_config_page_gen_code(fbe_terminator_device_ptr_t virtual_phy_handle);
fbe_status_t terminator_initialize_eses_page_info(fbe_sas_enclosure_type_t encl_type, terminator_vp_eses_page_info_t *eses_page_info);
fbe_status_t terminator_get_eses_page_info(fbe_terminator_device_ptr_t virtual_phy_handle,
                                           terminator_vp_eses_page_info_t **vp_eses_page_info);
fbe_status_t terminator_eses_set_ver_desc(fbe_terminator_device_ptr_t virtual_phy_handle,
                                        ses_subencl_type_enum subencl_type,
                                        terminator_eses_subencl_side side,
                                        fbe_u8_t  comp_type,
                                        ses_ver_desc_struct ver_desc);
fbe_status_t terminator_eses_set_ver_num(fbe_terminator_device_ptr_t virtual_phy_handle,
                                        ses_subencl_type_enum subencl_type,
                                        terminator_eses_subencl_side side,
                                        fbe_u8_t  comp_type,
                                        CHAR *ver_num);
fbe_status_t terminator_eses_get_ver_num(fbe_terminator_device_ptr_t virtual_phy_handle,
                                        ses_subencl_type_enum subencl_type,
                                        terminator_eses_subencl_side side,
                                        fbe_u8_t  comp_type,
                                        CHAR *ver_num);

fbe_status_t terminator_set_enclosure_firmware_activate_time_interval(fbe_terminator_device_ptr_t virtual_phy_ptr, fbe_u32_t time_interval);
fbe_status_t terminator_get_enclosure_firmware_activate_time_interval(fbe_terminator_device_ptr_t virtual_phy_ptr, fbe_u32_t *time_interval);
fbe_status_t terminator_set_enclosure_firmware_reset_time_interval(fbe_terminator_device_ptr_t virtual_phy_ptr, fbe_u32_t time_interval);
fbe_status_t terminator_get_enclosure_firmware_reset_time_interval(fbe_terminator_device_ptr_t virtual_phy_ptr, fbe_u32_t *time_interval);

fbe_status_t terminator_eses_get_subencl_ver_desc_position(terminator_vp_eses_page_info_t *eses_page_info,
                                                        ses_subencl_type_enum subencl_type,
                                                        terminator_eses_subencl_side side,
                                                        fbe_u8_t *ver_desc_start_index,
                                                        fbe_u8_t *num_ver_descs);
fbe_status_t terminator_eses_set_download_microcode_stat_page_stat_desc(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                         fbe_download_status_desc_t download_stat_desc);
fbe_status_t terminator_eses_get_download_microcode_stat_page_stat_desc(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                         fbe_download_status_desc_t *download_stat_desc);

fbe_status_t terminator_eses_get_vp_download_microcode_ctrl_page_info(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                      vp_download_microcode_ctrl_diag_page_info_t **vp_download_ctrl_page_info);
fbe_status_t terminator_eses_set_download_status(fbe_terminator_device_ptr_t virtual_phy_handle, fbe_u8_t status);
fbe_status_t terminator_eses_get_download_status(fbe_terminator_device_ptr_t virtual_phy_handle, fbe_u8_t *status);

fbe_status_t terminator_eses_get_ver_desc(fbe_terminator_device_ptr_t virtual_phy_handle,
                                        ses_subencl_type_enum subencl_type,
                                        terminator_eses_subencl_side side,
                                        fbe_u8_t  comp_type,
                                        ses_ver_desc_struct *ver_desc);
fbe_status_t terminator_eses_get_subencl_id(fbe_terminator_device_ptr_t virtual_phy_handle,
                                            ses_subencl_type_enum subencl_type,
                                            terminator_eses_subencl_side side,
                                            fbe_u8_t *subencl_id);
fbe_status_t terminator_virtual_phy_get_unit_attention(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                       fbe_u8_t *unit_attention);
fbe_status_t terminator_virtual_phy_set_unit_attention(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                       fbe_u8_t unit_attention);
fbe_status_t terminator_get_encl_eses_status(fbe_terminator_device_ptr_t virtual_phy_handle,
                                             ses_stat_elem_encl_struct *encl_stat);
fbe_status_t terminator_set_encl_eses_status(fbe_terminator_device_ptr_t virtual_phy_handle,
                                             ses_stat_elem_encl_struct encl_stat);
fbe_status_t terminator_get_chassis_encl_eses_status(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                     ses_stat_elem_encl_struct *encl_stat);
fbe_status_t terminator_set_chassis_encl_eses_status(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                     ses_stat_elem_encl_struct encl_stat);
fbe_status_t terminator_eses_set_buf(fbe_terminator_device_ptr_t virtual_phy_handle, 
                                    ses_subencl_type_enum subencl_type,
                                    fbe_u8_t side,
                                    ses_buf_type_enum buf_type,
                                    fbe_bool_t consider_writable,
                                    fbe_u8_t writable,
                                    fbe_bool_t consider_buf_index,
                                    fbe_u8_t buf_index,
                                    fbe_bool_t consider_buf_spec_info,
                                    fbe_u8_t buf_spec_info,
                                    fbe_u8_t *buf,
                                    fbe_u32_t len);

fbe_status_t terminator_get_buf_id_by_subencl_info(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                   ses_subencl_type_enum subencl_type,
                                                   fbe_u8_t side,
                                                   ses_buf_type_enum buf_type,
                                                   fbe_bool_t consider_writable,
                                                   fbe_u8_t writable,
                                                   fbe_bool_t consider_buf_index,
                                                   fbe_u8_t buf_index,
                                                   fbe_bool_t consider_buf_spec_info,
                                                   fbe_u8_t buf_spec_info,
                                                   fbe_u8_t *buf_id);

fbe_status_t terminator_set_bufid2_buf_area_to_bufid1(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                      fbe_u8_t buf_id1,
                                                      fbe_u8_t buf_id2);

fbe_status_t terminator_eses_get_buf_info_by_buf_id(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                    fbe_u8_t buf_id,
                                                    terminator_eses_buf_info_t **buf_info);
fbe_status_t terminator_eses_set_buf_by_buf_id(fbe_terminator_device_ptr_t virtual_phy_handle, 
                                            fbe_u8_t buf_id,
                                            fbe_u8_t *buf,
                                            fbe_u32_t len);
fbe_status_t terminator_virtual_phy_power_cycle_lcc(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                    fbe_u32_t power_cycle_time_in_ms);

fbe_status_t terminator_virtual_phy_power_cycle_drive(fbe_terminator_device_ptr_t virtual_phy_handle,
                                         fbe_u32_t power_cycle_time_in_ms,
                                         fbe_u32_t  drive_slot_number);

fbe_status_t terminator_enclosure_firmware_download(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                    ses_subencl_type_enum subencl_type,
                                                                    terminator_sp_id_t side,
                                                                    ses_comp_type_enum  comp_type,
                                                                    fbe_u32_t slot_num,
                                                                    CHAR   *new_rev_number);
fbe_status_t terminator_enclosure_firmware_activate(fbe_terminator_device_ptr_t virtual_phy_handle,
                                                                    ses_subencl_type_enum subencl_type,
                                                                    terminator_sp_id_t side,
                                                                    fbe_u32_t slot_num);

fbe_status_t terminator_get_maximum_transfer_bytes_from_miniport(const fbe_terminator_device_ptr_t device_handle,
                                                                 fbe_u32_t *maximum_transfer_bytes_p);

fbe_status_t terminator_get_display_eses_status(fbe_terminator_device_ptr_t virtual_phy_handle, 
                                               terminator_eses_display_character_id display_character_id,
                                               ses_stat_elem_display_struct *display_stat_elem);

fbe_status_t terminator_set_display_eses_status(fbe_terminator_device_ptr_t virtual_phy_handle, 
                                               terminator_eses_display_character_id display_character_id,
                                               ses_stat_elem_display_struct display_stat_elem);

//fbe_status_t terminator_create_disk_file(char *drive_name,
//                                         fbe_u32_t file_size_in_bytes);

fbe_status_t terminator_set_drive_slot_insert_count(fbe_terminator_device_ptr_t virtual_phy_handle, 
                                                    fbe_u32_t slot_number, 
                                                    fbe_u8_t insert_count);
fbe_status_t terminator_get_drive_slot_insert_count(fbe_terminator_device_ptr_t virtual_phy_handle, 
                                                    fbe_u32_t slot_number, 
                                                    fbe_u8_t *insert_count);
fbe_status_t terminator_set_drive_power_down_count(fbe_terminator_device_ptr_t virtual_phy_handle, 
                                                   fbe_u32_t slot_number, 
                                                   fbe_u8_t power_down_count);
fbe_status_t terminator_get_drive_power_down_count(fbe_terminator_device_ptr_t virtual_phy_handle, 
                                                   fbe_u32_t slot_number, 
                                                   fbe_u8_t *power_down_count);
fbe_status_t terminator_clear_drive_power_down_counts(fbe_terminator_device_ptr_t virtual_phy_handle);
fbe_status_t terminator_clear_drive_slot_insert_counts(fbe_terminator_device_ptr_t virtual_phy_handle);

fbe_status_t terminator_set_need_update_enclosure_firmware_rev(fbe_bool_t need_update_rev);
fbe_status_t terminator_get_need_update_enclosure_firmware_rev(fbe_bool_t *need_update_rev);

fbe_status_t terminator_set_need_update_enclosure_resume_prom_checksum(fbe_bool_t need_update_checksum);
fbe_status_t terminator_get_need_update_enclosure_resume_prom_checksum(fbe_bool_t *need_update_checksum);

/*!**************************************************************************
 * @file fbe_terminator.h
 ***************************************************************************
 *
 * @brief
 *  This file contains interface for the Terminator.
 *
 * @date 11/07/2008
 * @author VG
 *
 ***************************************************************************/
/*The following are the new interfaces for uSimCtrl integration*/
fbe_status_t terminator_create_board(fbe_terminator_device_ptr_t *device_handle);
fbe_status_t terminator_create_port(const char * type,fbe_terminator_device_ptr_t *device_handle);
fbe_status_t terminator_create_enclosure(const char * type, fbe_terminator_device_ptr_t *device_handle);

fbe_status_t terminator_create_drive(fbe_terminator_device_ptr_t *device_handle);
fbe_status_t terminator_destroy_drive(fbe_terminator_device_ptr_t device_handle);

fbe_status_t terminator_set_device_attributes(fbe_terminator_device_ptr_t device_handle, 
                                              void * attributes);
fbe_status_t terminator_insert_device(fbe_terminator_device_ptr_t parent_device, 
                                      fbe_terminator_device_ptr_t child_device);
fbe_status_t terminator_insert_device_new(fbe_terminator_device_ptr_t parent_device, 
                                      fbe_terminator_device_ptr_t child_device);
fbe_status_t terminator_activate_device(fbe_terminator_device_ptr_t device_handle);


/* Sata Drive*/
fbe_status_t terminator_insert_sata_drive(fbe_terminator_device_ptr_t encl_ptr,
                                         fbe_u32_t slot_number,
                                         fbe_terminator_sata_drive_info_t *drive_info,
                                         fbe_terminator_device_ptr_t *drive_ptr);
fbe_status_t terminator_create_sata_drive(fbe_terminator_device_ptr_t encl_ptr,
                                         fbe_u32_t slot_number,
                                         fbe_terminator_sata_drive_info_t *drive_info,
                                         fbe_terminator_device_ptr_t *drive_ptr);

/* Device reset interface */
fbe_status_t terminator_register_device_reset_function(fbe_terminator_device_ptr_t device_handle, 
                                                       fbe_terminator_device_reset_function_t reset_function);
fbe_status_t terminator_get_device_reset_function(fbe_terminator_device_ptr_t device_handle,
                                                  fbe_terminator_device_reset_function_t *reset_function);

fbe_status_t terminator_set_device_reset_delay(fbe_terminator_device_ptr_t device_handle, fbe_u32_t delay_in_ms);
fbe_status_t terminator_get_device_reset_delay(fbe_terminator_device_ptr_t device_handle, fbe_u32_t *delay_in_ms);

fbe_status_t terminator_logout_device(fbe_terminator_device_ptr_t device_handle);
fbe_status_t terminator_login_device(fbe_terminator_device_ptr_t device_handle);
//fbe_status_t terminator_get_device_handle_by_device_id(fbe_u32_t port_number,
//                                                       fbe_terminator_api_device_handle_t device_id,
//                                                       fbe_terminator_device_ptr_t *handle);

/* miniport device table related interfaces */
fbe_status_t terminator_get_miniport_sas_device_table_index(const fbe_terminator_device_ptr_t device_ptr, 
                                                            fbe_u32_t *device_table_index);
fbe_status_t terminator_set_miniport_sas_device_table_index(const fbe_terminator_device_ptr_t device_ptr, 
                                                            const fbe_miniport_device_id_t cpd_device_id);
fbe_status_t terminator_set_miniport_port_index(const fbe_terminator_device_ptr_t port_handle, 
                                                const fbe_u32_t port_index);
fbe_status_t terminator_get_miniport_port_index(const fbe_terminator_device_ptr_t port_handle, 
                                                fbe_u32_t *port_index);
fbe_status_t terminator_get_cpd_device_id_from_miniport(const fbe_terminator_device_ptr_t deivce_handle, 
                                                        fbe_miniport_device_id_t *cpd_device_id);

fbe_status_t terminator_get_port_miniport_port_index(fbe_terminator_device_ptr_t component_handle, fbe_u32_t *port_index);
fbe_status_t terminator_get_drive_parent_virtual_phy_ptr(fbe_terminator_device_ptr_t device_ptr, fbe_terminator_device_ptr_t *virtual_phy_handle);
fbe_status_t terminator_add_device_to_miniport_sas_device_table(const fbe_terminator_device_ptr_t deivce_handle); 
fbe_status_t terminator_remove_device_from_miniport_api_device_table (fbe_u32_t port_number, fbe_terminator_device_ptr_t device_id);
fbe_status_t terminator_reserve_cpd_device_id(const fbe_u32_t       port_index, 
                                              const fbe_miniport_device_id_t cpd_device_id);
fbe_status_t terminator_update_device_cpd_device_id(const fbe_terminator_device_ptr_t device_handle);
fbe_status_t terminator_update_device_parent_cpd_device_id(const fbe_terminator_device_ptr_t device_handle, const fbe_miniport_device_id_t cpd_device_id);
fbe_status_t terminator_create_disk_file(char *drive_name, fbe_u64_t file_size_in_bytes);

fbe_status_t terminator_get_ee_component_connector_id(fbe_terminator_device_ptr_t device_ptr, fbe_u32_t *ret_connector_id);
fbe_status_t terminator_update_drive_parent_device(fbe_terminator_device_ptr_t *parent_ptr, fbe_u32_t *slot_number);
// this function converts local slot number to enclosure slot number
fbe_status_t terminator_update_enclosure_drive_slot_number(fbe_terminator_device_ptr_t device_ptr, fbe_u32_t *ret_slot_number);
// converts from enclosure slot_number to local slot number for EEs.
fbe_status_t terminator_update_enclosure_local_drive_slot_number(fbe_terminator_device_ptr_t device_ptr, fbe_u32_t *slot_number);

fbe_status_t terminator_get_connector_id_list_for_parent_device(fbe_terminator_device_ptr_t device_ptr,
    fbe_term_encl_connector_list_t *connector_ids);

fbe_status_t terminator_wait_for_enclosure_firmware_activate_thread(void);

#endif /*FBE_TERMINATOR_H*/
