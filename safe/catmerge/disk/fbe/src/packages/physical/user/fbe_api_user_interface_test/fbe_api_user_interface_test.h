#include "mut.h"
#include "fbe/fbe_terminator_api.h"
#include "fbe/fbe_physical_package.h"
#include "fbe_trace.h"
#include "fbe/fbe_api_enclosure_interface.h"


/*fbe_api_device_handle_getter_suite*/
void fbe_api_get_board_object_handle_test(void);
void fbe_api_get_port_object_handle_test(void);
void fbe_api_object_map_interface_get_enclosure_obj_id_test(void);
void fbe_api_get_physical_drive_object_handle_test(void);
void fbe_api_get_logical_drive_object_handle_test(void);

/* fbe_api_query_command_suite */
void fbe_api_get_object_type_test(void);
void fbe_api_get_object_class_id_test(void);
void fbe_api_get_object_port_number_test(void);
void fbe_api_get_object_lifecycle_state_test(void);
void fbe_api_topology_enumerate_objects_test(void);
void fbe_api_get_object_discovery_edge_info_test(void);
void fbe_api_get_object_ssp_transport_edge_info_test(void);
void fbe_api_get_object_block_edge_info_test(void);
void fbe_api_get_all_objects_in_state_test(void);
void fbe_api_get_physical_state_object_from_topology_test(void);
void fbe_api_get_logical_drive_state_from_topology_test(void);
void fbe_api_get_enclosure_state_from_topology_test(void);
void fbe_api_get_port_state_from_topology_test(void);
void fbe_api_get_object_stp_transport_edge_info_test(void);
void fbe_api_get_object_smp_transport_edge_info_test(void);


/* fbe_api_control_commands_suite */
void fbe_api_drive_reset_test(void);
void fbe_api_physical_drive_power_cycle_test(void);

/* Utility suite */
void fbe_api_wait_for_object_lifecycle_state_test(void);
void fbe_api_wait_for_number_of_objects_test(void);
/*
 * Enclosure command set
 */
void fbe_api_enclosure_bypass_drive_test(void);
void fbe_api_enclosure_unbypass_drive_test(void);
void fbe_api_enclosure_get_info_test(void);
void fbe_api_enclosure_get_setup_info_test(void);
void fbe_api_enclosure_set_control_test(void);
void fbe_api_enclosure_set_leds_test(void);
void fbe_api_enclosure_firmware_download_status_test(void);
void fbe_api_enclosure_firmware_download_test(void);
void fbe_api_enclosure_firmware_update_test(void);
void fbe_api_enclosure_get_slot_to_phy_mapping_test(void);

/* physical_drive suite */
void fbe_api_physical_drive_get_error_counts_test(void);
void fbe_api_physical_drive_clear_error_counts_test(void);
void fbe_api_physical_drive_set_and_get_object_queue_depth_test(void);
void fbe_api_physical_drive_is_wr_cache_enabled_test(void);
void fbe_api_physical_drive_get_drive_information_test(void);
void fbe_api_physical_drive_set_get_qdepth_test(void);
void fbe_api_physical_drive_set_get_timeout_test(void);
void fbe_api_physical_drive_set_sata_write_uncorrectable_test(void);

/* logical_drive suite */
void fbe_api_ldo_test_send_set_attributes_test(void);
void fbe_api_ldo_test_send_get_attributes_test(void);

/*object map tests*/
void fbe_api_object_map_interface_register_notification_test(void);
void fbe_api_object_map_interface_unregister_notification_test(void);
void fbe_api_object_map_interface_init_test (void);
void fbe_api_object_map_interface_destroy_test (void);
void fbe_api_object_map_interface_object_exists_test (void);
void fbe_api_object_map_interface_get_board_obj_id_test(void);
void fbe_api_object_map_interface_get_total_logical_drives_test(void);
void fbe_api_object_map_interface_get_object_handle_test (void);
void fbe_api_object_map_interface_get_logical_drive_object_handle_test (void);
void fbe_api_object_map_interface_get_obj_info_by_handle_test (void);

/*drive config teable tests*/
void fbe_api_drive_configuration_interface_get_handles_test(void);
void fbe_api_drive_configuration_interface_add_table_entry_test(void);
void fbe_api_drive_configuration_interface_get_table_entry_test(void);
void fbe_api_drive_configuration_interface_change_thresholds_test(void);
void fbe_api_drive_configuration_interface_add_port_table_entry_test(void);
void fbe_api_drive_configuration_interface_change_port_thresholds_test(void);
void fbe_api_drive_configuration_interface_get_port_handles_test(void);
void fbe_api_drive_configuration_interface_get_port_table_entry_test(void);




