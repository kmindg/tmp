#ifndef TERMINATOR_TEST_H
#define TERMINATOR_TEST_H

#include "mut.h"

/* terminator api suite */
void terminator_api_init_destroy_test(void);
void terminator_api_d60_test(void);
void terminator_api_d120_test(void);
void terminator_api_get_device_id_test(void);
void terminator_api_set_get_drive_info_test(void);
void terminator_api_force_logout_login_test(void);
void terminator_api_set_get_enclosure_info_test(void);
void terminator_api_set_get_io_mode_test(void);
void terminator_api_device_login_state_test(void);
void terminator_api_reset_device_test(void);
void terminator_api_miniport_sas_device_table_test(void);
void terminator_api_test_setup(void);
void terminator_api_test_teardown(void);

/* terminator suite */
void terminator_insert_remove(void);
void terminator_pull_reinsert_drive(void);
void terminator_pull_reinsert_enclosure(void);
void terminator_enclosure_firmware_download_activate_test(void);

/* terminator base test suite */
void baseComponentTest_init(void);
void baseComponentTest_add_child(void);
void baseComponentTest_add_remove_child(void);
void baseComponentTest_get_child_count(void);
void baseComponentTest_chain_component(void);
void baseComponentTest_get_child_by_id(void);
void baseComponentTest_base_generate_device_id(void);
void baseComponentTest_base_component_get_first_child(void);


/* Terminator board test suite*/
void board_new_test(void);

/* port test suite */
void port_new_test(void);
void port_set_port_number_test(void);
void get_sas_port_address_test(void);
void port_destroy_test(void);

/* terminator_eses_test suite */
void terminator_set_enclosure_drive_phy_status_test(void);

void terminator_addl_status_page_test(void);
void terminator_set_enclosure_ps_cooling_tempSensor_status_test(void);
void terminator_eses_sense_data_test(void);
void terminator_control_page_drive_bypass_test(void);
void terminator_config_page_gen_code_test(void);
void terminator_download_mcode_page_test(void);
void terminator_config_page_version_descriptors_test(void);
void terminator_magnum_enclosure_test(void);
void terminator_led_patterns_test(void);
void terminator_drive_poweroff_test(void);
void terminator_read_buf_test(void);
void terminator_write_buf_test(void);
void terminator_mode_cmds_test(void);

/* terminator_device_registry_test suite */
void terminator_device_registry_test_setup(void);
void terminator_device_registry_test_teardown(void);

void terminator_device_registry_attribute_management_test(void);
void terminator_device_registry_should_not_do_it_before_init(void);
void terminator_device_registry_init_test(void);
void terminator_device_registry_add_test(void);
void terminator_device_registry_device_attributes_test(void);
void terminator_device_registry_remove_test(void);
void terminator_device_registry_enumerate_all_test(void);
void terminator_device_registry_enumerate_by_type_test(void);
void terminator_device_registry_counters_test(void);
void terminator_device_registry_get_type_test(void);
void terminator_device_registry_destroy_test(void);
void terminator_device_registry_should_not_do_it_after_destroy(void);

/* terminator creation API test suite */
void terminator_creation_api_create_device_test(void);
void terminator_creation_api_enumerate_devices_test(void);
void terminator_creation_api_get_device_type_test(void);
void terminator_creation_api_set_device_attr_test(void);
void terminator_creation_api_get_device_attr_test(void);
void terminator_creation_api_destroy_device_test(void);
void terminator_creation_api_insert_and_activate_device_test(void);

/* terminator class management test suite */
void terminator_class_management_test(void);

#endif /* TERMINATOR_TEST_H */