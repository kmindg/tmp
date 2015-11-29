#ifndef FBE_API_TERMINATOR_INTERFACE_H
#define FBE_API_TERMINATOR_INTERFACE_H

#include "fbe/fbe_types.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_api_terminator_common.h"
#include "fbe/fbe_api_sim_transport.h"
#include "../../fbe/src/services/cluster_memory/interface/memory/fbe_cms_memory.h"

typedef fbe_terminator_api_device_class_handle_t fbe_api_terminator_device_class_handle_t;


fbe_status_t fbe_api_terminator_init(void);
fbe_status_t fbe_api_terminator_destroy(void);

/* Device */
fbe_status_t fbe_api_terminator_force_login_device (fbe_api_terminator_device_handle_t device_handle);

fbe_status_t fbe_api_terminator_force_logout_device (fbe_api_terminator_device_handle_t device_handle);

fbe_status_t fbe_api_terminator_find_device_class(const char * device_class_name, 
												fbe_api_terminator_device_class_handle_t *device_class_handle);

fbe_status_t fbe_api_terminator_create_device_class_instance(fbe_api_terminator_device_class_handle_t device_class_handle,
												 const char * device_type,
                                                 fbe_api_terminator_device_handle_t * device_handle);

fbe_status_t fbe_api_terminator_insert_device(fbe_api_terminator_device_handle_t parent_device,
                                              fbe_api_terminator_device_handle_t child_device);

fbe_status_t fbe_api_terminator_remove_device (fbe_api_terminator_device_handle_t device_handle);

fbe_status_t fbe_api_terminator_set_device_attribute(fbe_api_terminator_device_handle_t device_handle,
                                                    const char * attribute_name,
                                                    const char * attribute_value);

fbe_status_t fbe_api_terminator_get_device_attribute(fbe_api_terminator_device_handle_t device_handle,
                                                    const char * attribute_name,
                                                    char * attribute_value_buffer,
                                                    fbe_u32_t buffer_length);

fbe_status_t fbe_api_terminator_set_sas_enclosure_drive_slot_eses_status(fbe_api_terminator_device_handle_t enclosure_handle,
                                                                         fbe_u32_t slot_number,
                                                                         ses_stat_elem_array_dev_slot_struct drive_slot_stat);

fbe_status_t fbe_api_terminator_get_ps_eses_status(fbe_api_terminator_device_handle_t enclosure_handle,
                                                   terminator_eses_ps_id ps_id,
                                                   ses_stat_elem_ps_struct *ps_stat);
fbe_status_t fbe_api_terminator_set_ps_eses_status(fbe_api_terminator_device_handle_t enclosure_handle,
                                                   terminator_eses_ps_id ps_id,
                                                   ses_stat_elem_ps_struct ps_stat);
fbe_status_t fbe_api_terminator_get_sas_conn_eses_status(fbe_api_terminator_device_handle_t enclosure_handle,
                                                   terminator_eses_sas_conn_id sas_conn_id,
                                                   ses_stat_elem_sas_conn_struct *sas_conn_stat);
fbe_status_t fbe_api_terminator_set_sas_conn_eses_status(fbe_api_terminator_device_handle_t enclosure_handle,
                                                   terminator_eses_sas_conn_id sas_conn_id,
                                                   ses_stat_elem_sas_conn_struct sas_conn_stat);

fbe_status_t fbe_api_terminator_get_lcc_eses_status(fbe_api_terminator_device_handle_t enclosure_handle,
                                                   ses_stat_elem_encl_struct *lcc_stat);
fbe_status_t fbe_api_terminator_set_lcc_eses_status(fbe_api_terminator_device_handle_t enclosure_handle,
                                                   ses_stat_elem_encl_struct lcc_stat);

fbe_status_t fbe_api_terminator_get_cooling_eses_status(fbe_api_terminator_device_handle_t enclosure_handle,
                                                        terminator_eses_cooling_id cooling_id,
                                                        ses_stat_elem_cooling_struct *cooling_stat);
fbe_status_t fbe_api_terminator_set_cooling_eses_status(fbe_api_terminator_device_handle_t enclosure_handle,
                                                        terminator_eses_cooling_id cooling_id,
                                                        ses_stat_elem_cooling_struct cooling_stat);

fbe_status_t fbe_api_terminator_set_temp_sensor_eses_status(fbe_api_terminator_device_handle_t enclosure_handle,
                                                            terminator_eses_temp_sensor_id temp_sensor_id,
                                                            ses_stat_elem_temp_sensor_struct temp_sensor_stat);
fbe_status_t fbe_api_terminator_get_temp_sensor_eses_status(fbe_api_terminator_device_handle_t enclosure_handle,
                                                            terminator_eses_temp_sensor_id temp_sensor_id,
                                                            ses_stat_elem_temp_sensor_struct *temp_sensor_stat);

fbe_status_t fbe_api_terminator_activate_device(fbe_api_terminator_device_handle_t device_handle);

fbe_status_t fbe_api_terminator_set_resume_prom_info(fbe_api_terminator_device_handle_t enclosure_handle,
                                                     terminator_eses_resume_prom_id_t resume_prom_id,
                                                     fbe_u8_t *buf,
                                                     fbe_u32_t len);

fbe_status_t fbe_api_terminator_set_io_mode(fbe_terminator_io_mode_t io_mode);

fbe_status_t fbe_api_terminator_set_io_global_completion_delay(fbe_u32_t global_completion_delay);

fbe_status_t fbe_api_terminator_mark_eses_page_unsupported(fbe_u8_t cdb_opcode, fbe_u8_t diag_page_code);

fbe_status_t fbe_api_terminator_get_device_cpd_device_id(const fbe_api_terminator_device_handle_t device_handle, fbe_miniport_device_id_t *cpd_device_id);

fbe_status_t fbe_api_terminator_reserve_miniport_sas_device_table_index(const fbe_u32_t port_number, const fbe_miniport_device_id_t cpd_device_id);

fbe_status_t fbe_api_terminator_miniport_sas_device_table_force_add(const fbe_api_terminator_device_handle_t device_handle, const fbe_miniport_device_id_t cpd_device_id);

fbe_status_t fbe_api_terminator_miniport_sas_device_table_force_remove(const fbe_api_terminator_device_handle_t device_handle);

/* Board */
fbe_status_t fbe_api_terminator_insert_board(fbe_terminator_board_info_t *board_info);

fbe_status_t fbe_api_terminator_send_specl_sfi_mask_data(PSPECL_SFI_MASK_DATA sfi_mask_data);

/* Port */
fbe_status_t fbe_api_terminator_insert_sas_port(fbe_terminator_sas_port_info_t *port_info, 
											fbe_api_terminator_device_handle_t *port_handle);

fbe_status_t fbe_api_terminator_insert_fc_port(fbe_terminator_fc_port_info_t *port_info, 
											fbe_api_terminator_device_handle_t *port_handle);

fbe_status_t fbe_api_terminator_insert_iscsi_port(fbe_terminator_iscsi_port_info_t *port_info, 
											fbe_api_terminator_device_handle_t *port_handle);
fbe_status_t fbe_api_terminator_insert_fcoe_port(fbe_terminator_fcoe_port_info_t *port_info, 
											fbe_api_terminator_device_handle_t *port_handle);

fbe_status_t fbe_api_terminator_get_port_handle(fbe_u32_t backend_number, 
												fbe_api_terminator_device_handle_t *port_handle);

fbe_status_t fbe_api_terminator_start_port_reset (fbe_api_terminator_device_handle_t port_handle);

fbe_status_t fbe_api_terminator_complete_port_reset (fbe_api_terminator_device_handle_t port_handle);

fbe_status_t fbe_api_terminator_get_hardware_info(fbe_api_terminator_device_handle_t port_handle, fbe_cpd_shim_hardware_info_t *hdw_info);
fbe_status_t fbe_api_terminator_set_hardware_info(fbe_api_terminator_device_handle_t port_handle, fbe_cpd_shim_hardware_info_t *hdw_info);
fbe_status_t fbe_api_terminator_get_sfp_media_interface_info(fbe_api_terminator_device_handle_t port_handle, fbe_cpd_shim_sfp_media_interface_info_t *sfp_media_interface_info);
fbe_status_t fbe_api_terminator_set_sfp_media_interface_info(fbe_api_terminator_device_handle_t port_handle, fbe_cpd_shim_sfp_media_interface_info_t *sfp_media_interface_info);
fbe_status_t fbe_api_terminator_get_port_link_info(fbe_api_terminator_device_handle_t port_handle, fbe_cpd_shim_port_lane_info_t *port_link_info);
fbe_status_t fbe_api_terminator_set_port_link_info(fbe_api_terminator_device_handle_t port_handle, fbe_cpd_shim_port_lane_info_t *port_link_info);
fbe_status_t fbe_api_terminator_get_encryption_keys(fbe_api_terminator_device_handle_t port_handle, fbe_key_handle_t key_handle, fbe_u8_t *keys);
fbe_status_t fbe_api_terminator_get_port_address(fbe_api_terminator_device_handle_t port_handle, fbe_address_t *address);

/* Enclosure */
fbe_status_t fbe_api_terminator_get_enclosure_handle(fbe_u32_t port_number,
                                                     fbe_u32_t enclosure_number,
                                                     fbe_api_terminator_device_handle_t * enclosure_handle);

fbe_status_t fbe_api_terminator_insert_sas_enclosure(fbe_api_terminator_device_handle_t port_handle, 
												 fbe_terminator_sas_encl_info_t *encl_info, 
												 fbe_api_terminator_device_handle_t *enclosure_handle);

fbe_status_t fbe_api_terminator_remove_sas_enclosure(fbe_api_terminator_device_handle_t enclosure_handle);

fbe_status_t fbe_api_terminator_enclosure_bypass_drive_slot (fbe_api_terminator_device_handle_t enclosure_handle, fbe_u32_t slot_number);
fbe_status_t fbe_api_terminator_enclosure_unbypass_drive_slot (fbe_api_terminator_device_handle_t enclosure_handle, fbe_u32_t slot_number);

fbe_status_t fbe_api_terminator_get_emcEnclStatus(fbe_api_terminator_device_handle_t enclosure_handle,
                                                  ses_pg_emc_encl_stat_struct *emcEnclStatusPtr);
fbe_status_t fbe_api_terminator_set_emcEnclStatus(fbe_api_terminator_device_handle_t enclosure_handle,
                                                  ses_pg_emc_encl_stat_struct *emcEnclStatusPtr);
fbe_status_t fbe_api_terminator_get_emcPsInfoStatus(fbe_api_terminator_device_handle_t enclosure_handle,
                                                    terminator_eses_ps_id ps_id,
                                                    ses_ps_info_elem_struct *emcPsInfoStatusPtr);
fbe_status_t fbe_api_terminator_set_emcPsInfoStatus(fbe_api_terminator_device_handle_t enclosure_handle,
                                                    terminator_eses_ps_id ps_id,
                                                    ses_ps_info_elem_struct *emcPsInfoStatusPtr);

fbe_status_t fbe_api_terminator_eses_increment_config_page_gen_code(fbe_api_terminator_device_handle_t enclosure_handle);

fbe_status_t fbe_api_terminator_eses_set_unit_attention(fbe_api_terminator_device_handle_t enclosure_handle);
fbe_status_t fbe_api_terminator_eses_set_ver_desc(fbe_api_terminator_device_handle_t enclosure_handle,
                                                ses_subencl_type_enum subencl_type,
                                                fbe_u8_t  side,
                                                fbe_u8_t  comp_type,
                                                ses_ver_desc_struct ver_desc);

fbe_status_t fbe_api_terminator_eses_get_ver_desc(fbe_api_terminator_device_handle_t enclosure_handle,
                                                ses_subencl_type_enum subencl_type,
                                                fbe_u8_t side,
                                                fbe_u8_t  comp_type,
                                                ses_ver_desc_struct *ver_desc);

fbe_status_t fbe_api_terminator_eses_set_download_microcode_stat_page_stat_desc(fbe_api_terminator_device_handle_t enclosure_handle,
                                                fbe_download_status_desc_t download_stat_desc);

fbe_status_t fbe_api_terminator_set_buf_by_buf_id(fbe_api_terminator_device_handle_t enclosure_handle,
                                                fbe_u8_t buf_id,
                                                fbe_u8_t *buf,
                                                fbe_u32_t len);

fbe_status_t fbe_api_terminator_set_buf(fbe_api_terminator_device_handle_t enclosure_handle,
                                        ses_subencl_type_enum subencl_type,
                                        fbe_u8_t side,
                                        ses_buf_type_enum buf_type,
                                        fbe_u8_t *buf,
                                        fbe_u32_t len);
fbe_status_t fbe_api_terminator_reinsert_enclosure (fbe_api_terminator_device_handle_t port_handle,
                                                 fbe_api_terminator_device_handle_t enclosure_handle);
fbe_status_t fbe_api_terminator_pull_enclosure (fbe_api_terminator_device_handle_t enclosure_handle);

fbe_status_t fbe_api_terminator_set_need_update_enclosure_firmware_rev(fbe_bool_t  update_rev);
fbe_status_t fbe_api_terminator_get_need_update_enclosure_firmware_rev(fbe_bool_t  *update_rev);

fbe_status_t fbe_api_terminator_set_need_update_enclosure_resume_prom_checksum(fbe_bool_t need_update_checksum);
fbe_status_t fbe_api_terminator_get_need_update_enclosure_resume_prom_checksum(fbe_bool_t *need_update_checksum);

fbe_status_t fbe_api_terminator_set_enclosure_firmware_activate_time_interval(fbe_api_terminator_device_handle_t enclosure_handle, fbe_u32_t time_interval);
fbe_status_t fbe_api_terminator_get_enclosure_firmware_activate_time_interval(fbe_api_terminator_device_handle_t enclosure_handle, fbe_u32_t *time_interval);
fbe_status_t fbe_api_terminator_set_enclosure_firmware_reset_time_interval(fbe_api_terminator_device_handle_t enclosure_handle, fbe_u32_t time_interval);
fbe_status_t fbe_api_terminator_get_enclosure_firmware_reset_time_interval(fbe_api_terminator_device_handle_t enclosure_handle, fbe_u32_t *time_interval);
fbe_status_t fbe_api_terminator_count_terminator_devices (fbe_terminator_device_count_t *dev_count);

fbe_status_t fbe_api_terminator_get_connector_id_list_for_enclosure(fbe_api_terminator_device_handle_t enclosure_handle,
                                                   fbe_term_encl_connector_list_t * connector_ids);

fbe_status_t fbe_api_terminator_get_drive_slot_parent(fbe_api_terminator_device_handle_t * enclosure_handle, fbe_u32_t * slot_number);

fbe_status_t fbe_api_terminator_get_terminator_device_count(fbe_terminator_device_count_t * dev_counts);

fbe_status_t fbe_api_terminator_set_completion_dpc(fbe_bool_t b_should_terminator_completion_be_at_dpc);

/* Drive */
fbe_status_t fbe_api_terminator_get_drive_handle(fbe_u32_t port_number,
                                                 fbe_u32_t enclosure_number,
                                                 fbe_u32_t slot_number,
                                                 fbe_api_terminator_device_handle_t * drive_handle);

fbe_status_t fbe_api_terminator_insert_sas_drive(fbe_api_terminator_device_handle_t enclosure_handle,
												 fbe_u32_t slot_number,
												 fbe_terminator_sas_drive_info_t *drive_info, 
												 fbe_api_terminator_device_handle_t *drive_handle);
fbe_status_t fbe_api_terminator_insert_sata_drive(fbe_api_terminator_device_handle_t enclosure_handle,
												 fbe_u32_t slot_number,
												 fbe_terminator_sata_drive_info_t *drive_info, 
												 fbe_api_terminator_device_handle_t *drive_handle);
fbe_status_t fbe_api_terminator_reinsert_drive (fbe_api_terminator_device_handle_t enclosure_handle,
												 fbe_u32_t slot_number,
												 fbe_api_terminator_device_handle_t drive_handle);
fbe_status_t fbe_api_terminator_remove_sas_drive (fbe_api_terminator_device_handle_t drive_handle);
fbe_status_t fbe_api_terminator_remove_sata_drive (fbe_api_terminator_device_handle_t drive_handle);
fbe_status_t fbe_api_terminator_pull_drive (fbe_api_terminator_device_handle_t drive_handle);
fbe_status_t fbe_api_terminator_get_sata_drive_info(fbe_api_terminator_device_handle_t drive_handle, fbe_terminator_sata_drive_info_t *drive_info);
fbe_status_t fbe_api_terminator_get_sas_drive_info(fbe_api_terminator_device_handle_t drive_handle, fbe_terminator_sas_drive_info_t *drive_info);
fbe_status_t fbe_api_terminator_force_create_sas_drive (fbe_terminator_sas_drive_info_t *drive_info, fbe_api_terminator_device_handle_t *drive_handle);
fbe_status_t fbe_api_terminator_force_create_sata_drive (fbe_terminator_sata_drive_info_t *drive_info, fbe_api_terminator_device_handle_t *drive_handle);
fbe_status_t fbe_api_terminator_set_drive_product_id(fbe_api_terminator_device_handle_t drive_handle, const fbe_u8_t * product_id);
fbe_status_t fbe_api_terminator_get_drive_product_id(fbe_api_terminator_device_handle_t drive_handle, fbe_u8_t * product_id);

fbe_status_t fbe_api_terminator_get_drive_error_count(fbe_api_terminator_device_handle_t handle, fbe_u32_t  *const error_count_p);

fbe_status_t fbe_api_terminator_drive_error_injection_init(void);
fbe_status_t fbe_api_terminator_drive_error_injection_destroy(void);
fbe_status_t fbe_api_terminator_drive_error_injection_start(void);
fbe_status_t fbe_api_terminator_drive_error_injection_stop(void);
fbe_status_t fbe_api_terminator_drive_error_injection_add_error(fbe_terminator_neit_error_record_t record);
fbe_status_t fbe_api_terminator_drive_error_injection_init_error(fbe_terminator_neit_error_record_t* record);

fbe_status_t fbe_api_terminator_set_simulated_drive_type(terminator_simulated_drive_type_t simulated_drive_type);
fbe_status_t fbe_api_terminator_get_simulated_drive_type(terminator_simulated_drive_type_t *simulated_drive_type);
fbe_status_t fbe_api_terminator_set_simulated_drive_server_name(const char* server_name);
fbe_status_t fbe_api_terminator_set_simulated_drive_server_port(fbe_u16_t server_port);
fbe_status_t fbe_api_terminator_drive_set_state(fbe_terminator_api_device_handle_t device_handle,terminator_sas_drive_state_t  drive_state);
fbe_status_t fbe_api_terminator_sas_drive_get_default_page_info(fbe_sas_drive_type_t drive_type, fbe_terminator_sas_drive_type_default_info_t *default_info);
fbe_status_t fbe_api_terminator_sas_drive_set_default_page_info(fbe_sas_drive_type_t drive_type, const fbe_terminator_sas_drive_type_default_info_t *default_info);
fbe_status_t fbe_api_terminator_sas_drive_set_default_field(fbe_sas_drive_type_t drive_type, fbe_terminator_drive_default_field_t field, const fbe_u8_t *inq_data, fbe_u32_t inq_size);
fbe_status_t fbe_api_terminator_drive_set_log_page_31_data(fbe_api_terminator_device_handle_t drive_handle,
                                                           fbe_u8_t * log_page_31,
                                                           fbe_u32_t length);      
fbe_status_t fbe_api_terminator_drive_get_log_page_31_data(fbe_api_terminator_device_handle_t drive_handle,
                                                           fbe_u8_t *log_page_31,
                                                           fbe_u32_t *length);             

/* Other */
fbe_status_t fbe_api_terminator_get_devices_count_by_type_name(const fbe_u8_t * type_name, fbe_u32_t  *const device_count);

fbe_status_t fbe_api_terminator_set_cmi_port_base(fbe_u16_t port_base);
fbe_status_t fbe_api_terminator_get_connector_id_list_for_enclosure (fbe_terminator_api_device_handle_t enclosure_handle,
                                                        fbe_term_encl_connector_list_t *connector_ids);
fbe_status_t fbe_api_terminator_enclosure_find_slot_parent (fbe_terminator_api_device_handle_t *enclosure_handle, 
																  fbe_u32_t *slot_number);

fbe_status_t fbe_api_terminator_persistent_memory_get_persistence_request(fbe_bool_t * request);
fbe_status_t fbe_api_terminator_persistent_memory_set_persistence_request(fbe_bool_t request);
fbe_status_t fbe_api_terminator_persistent_memory_set_persistence_status(fbe_cms_memory_persist_status_t persistence_status);
fbe_status_t fbe_api_terminator_persistent_memory_get_persistence_status(fbe_cms_memory_persist_status_t *persistence_status);
fbe_status_t fbe_api_terminator_persistent_memory_zero_memory(void);

#endif /*FBE_API_TERMINATOR_INTERFACE_H*/


