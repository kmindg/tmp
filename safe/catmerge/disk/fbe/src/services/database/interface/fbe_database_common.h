#ifndef FBE_DATABASE_COMMON_H
#define FBE_DATABASE_COMMON_H
 
#include "fbe/fbe_class.h"
#include "fbe/fbe_raid_group.h"
#include "fbe/fbe_provision_drive.h"

#include "fbe_service_manager.h"
#include "fbe_transport_memory.h"
#include "fbe_block_transport.h"
#include "fbe_database.h"
#include "fbe_database_private.h"
#include "fbe_notification.h"

typedef struct database_destroy_notification_context_s
{
    fbe_object_id_t object_id;    
    fbe_semaphore_t sem;
}database_destroy_notification_context_t;

static fbe_notification_element_t CSX_MAYBE_UNUSED database_destroy_notification_element;
static database_destroy_notification_context_t CSX_MAYBE_UNUSED database_destroy_notification_context;

/* common packet send functions */
fbe_status_t fbe_database_send_control_packet_to_class(fbe_payload_control_buffer_t buffer, 
                                                       fbe_payload_control_buffer_length_t buffer_length, 
                                                       fbe_payload_control_operation_opcode_t opcode, 
                                                       fbe_package_id_t package_id, 
                                                       fbe_service_id_t service_id, 
                                                       fbe_class_id_t class_id, 
                                                       fbe_sg_element_t *sg_list, 
                                                       fbe_u32_t sg_count, 
                                                       fbe_bool_t traverse);

fbe_status_t fbe_database_send_packet_to_service(fbe_payload_control_operation_opcode_t control_code,
                                                 fbe_payload_control_buffer_t buffer,
                                                 fbe_payload_control_buffer_length_t buffer_length,
                                                 fbe_service_id_t service_id,
                                                 fbe_sg_element_t *sg_list,
                                                 fbe_u32_t sg_list_count,
                                                 fbe_packet_attr_t attr,
                                                 fbe_package_id_t package_id);

fbe_status_t fbe_database_send_packet_to_service_asynch(fbe_packet_t *packet_p,
                                                        fbe_payload_control_operation_opcode_t control_code,
                                                        fbe_payload_control_buffer_t buffer,
                                                        fbe_payload_control_buffer_length_t buffer_length,
                                                        fbe_service_id_t service_id,
                                                        fbe_sg_element_t *sg_list,
                                                        fbe_u32_t sg_list_count,
                                                        fbe_packet_attr_t attr,
                                                        fbe_packet_completion_function_t completion_function,
                                                        fbe_packet_completion_context_t completion_context,
                                                        fbe_package_id_t package_id);

fbe_status_t fbe_database_send_packet_to_object(fbe_payload_control_operation_opcode_t control_code,
                                                fbe_payload_control_buffer_t buffer,
                                                fbe_payload_control_buffer_length_t buffer_length,
                                                fbe_object_id_t object_id,
                                                fbe_sg_element_t *sg_list,
                                                fbe_u32_t sg_list_count,
                                                fbe_packet_attr_t attr,
                                                fbe_package_id_t package_id);

fbe_status_t fbe_database_send_packet_to_object_async(fbe_packet_t *packet,
                                                        fbe_payload_control_operation_opcode_t control_code,
                                                        fbe_payload_control_buffer_t buffer,
                                                        fbe_payload_control_buffer_length_t buffer_length,
                                                        fbe_object_id_t object_id,
                                                        fbe_sg_element_t *sg_list,
                                                        fbe_u32_t sg_list_count,
                                                        fbe_packet_attr_t attr,
                                                        fbe_packet_completion_function_t completion_function,
                                                        fbe_packet_completion_context_t completion_context,
                                                        fbe_package_id_t package_id);

/* Helper functions to process the packet */
#define fbe_database_get_payload(_packet, _payload_struct, _expected_payload_len) \
    fbe_database_get_payload_impl(_packet, (void **)_payload_struct, _expected_payload_len)
fbe_status_t fbe_database_get_payload_impl(fbe_packet_t *packet, 
                                      void **payload_struct, 
                                      fbe_u32_t expected_payload_len);
fbe_status_t fbe_database_complete_packet(fbe_packet_t *packet, 
                                          fbe_status_t packet_status);

/* object functions */
fbe_status_t fbe_database_create_object(fbe_class_id_t class_id, fbe_object_id_t *object_id);
fbe_status_t fbe_database_destroy_object(fbe_object_id_t object_id);
fbe_status_t fbe_database_commit_object(fbe_object_id_t object_id, database_class_id_t class_id, fbe_bool_t is_initial_commit);
fbe_status_t fbe_database_validate_object(fbe_object_id_t src_object_id);
fbe_status_t fbe_database_clone_object(fbe_object_id_t src_object_id, fbe_object_id_t *des_object_id);

/* edge functions*/
fbe_status_t fbe_database_create_edge(fbe_object_id_t object_id, 
									  fbe_object_id_t   server_id,       
									  fbe_edge_index_t client_index,  
									  fbe_lba_t capacity,
									  fbe_lba_t offset,
									  fbe_edge_flags_t  edge_flags);// fbe_block_transport_control_create_edge_t *create_edge)

fbe_status_t fbe_database_destroy_edge(fbe_object_id_t client_id, fbe_edge_index_t  client_index);

fbe_status_t fbe_database_set_lun_config(fbe_object_id_t object_id, fbe_database_lun_configuration_t *config);
fbe_status_t fbe_database_set_vd_config(fbe_object_id_t object_id, fbe_database_control_vd_set_configuration_t *config);
fbe_status_t fbe_database_set_raid_config(fbe_object_id_t object_id, fbe_raid_group_configuration_t *config);
fbe_status_t fbe_database_set_pvd_config(fbe_object_id_t object_id,fbe_provision_drive_configuration_t *config);
fbe_status_t fbe_database_set_ext_pool_config(fbe_object_id_t object_id,fbe_extent_pool_configuration_t *config);
fbe_status_t fbe_database_set_ext_pool_lun_config(fbe_object_id_t object_id,fbe_ext_pool_lun_configuration_t *config);
fbe_status_t fbe_database_update_vd_config(fbe_object_id_t object_id, fbe_database_control_vd_set_configuration_t *config);
fbe_status_t fbe_database_update_raid_config(fbe_object_id_t object_id, fbe_raid_group_configuration_t *config);
fbe_status_t fbe_database_update_pvd_config(fbe_object_id_t object_id,database_object_entry_t *config);
fbe_status_t fbe_database_update_pvd_block_size(fbe_object_id_t object_id, database_object_entry_t *config);
fbe_status_t fbe_database_set_system_power_save_info(fbe_system_power_saving_info_t *system_power_save_info);
fbe_status_t fbe_database_set_system_spare_info(fbe_system_spare_config_info_t *system_spare_info);
fbe_status_t fbe_database_common_get_virtual_drive_exported_size(fbe_lba_t in_provisional_drive_exported_capacity, fbe_lba_t *out_virtual_drive_exported_capacity);
fbe_status_t fbe_database_common_get_provision_drive_exported_size(fbe_lba_t in_ldo_exported_capacity, 
                                                                   fbe_block_edge_geometry_t block_edge_geometry,
                                                                   fbe_lba_t *out_provision_drive_exported_capacity);
fbe_status_t database_common_clone_object_nonpaged_metadata_data(fbe_object_id_t soucre_object_id, 
                                                                 fbe_object_id_t destination_object_id,
                                                                 fbe_config_generation_t dest_obj_generation_number);
fbe_status_t database_destroy_notification_callback(fbe_object_id_t object_id, 
        	 								       fbe_notification_info_t notification_info,
        									       fbe_notification_context_t context);

fbe_status_t fbe_database_set_system_encryption_info(fbe_system_encryption_info_t *system_encryption_info);

fbe_status_t database_get_lun_zero_status(fbe_object_id_t object_id, fbe_lun_get_zero_status_t *zero_status, fbe_packet_t *packet);

void database_set_lun_rotational_rate(database_object_entry_t* rg_object, fbe_u16_t* rotational_rate);

fbe_status_t fbe_database_get_pvd_for_vd(fbe_object_id_t vd_object_id, fbe_object_id_t* pvd, fbe_packet_t *packet);
fbe_status_t fbe_database_get_pvd_list_inter_phase(fbe_object_id_t object_id, fbe_object_id_t* pvd_list, fbe_packet_t *packet);
fbe_status_t fbe_database_get_pvd_list(fbe_database_raid_group_info_t *rg_info, fbe_packet_t *packet);
fbe_status_t fbe_database_get_all_pvd_for_vd(fbe_object_id_t vd_object_id, fbe_object_id_t *source, fbe_object_id_t *dest, fbe_packet_t *packet);

fbe_status_t fbe_database_get_lun_list(fbe_database_raid_group_info_t *rg_info, fbe_packet_t *packet);

fbe_status_t fbe_database_get_rgs_on_top_of_pvd(fbe_object_id_t pvd_id,
												fbe_object_id_t *rg_list,
												fbe_raid_group_number_t *rg_number_list,
                                                fbe_u32_t max_rg,
                                                fbe_u32_t *returned_rg,
												fbe_packet_t *packet);

fbe_status_t fbe_database_get_rgs_on_top_of_vd(fbe_object_id_t vd_id, fbe_object_id_t **rg_list, fbe_raid_group_number_t **rg_number_list,fbe_u32_t *total_discovered, fbe_u32_t *max_rg, fbe_packet_t *packet);

static __forceinline fbe_bool_t database_is_entry_valid(database_table_header_t *object_header)
{
	return ((object_header->state == DATABASE_CONFIG_ENTRY_VALID) ? FBE_TRUE : FBE_FALSE);
}

fbe_status_t fbe_database_get_rg_power_save_capable(fbe_database_raid_group_info_t *rg_info, fbe_packet_t *packet);
fbe_status_t fbe_database_set_ps_stats(fbe_database_control_update_ps_stats_t *ps_stats);

fbe_status_t fbe_database_zero_lun(fbe_object_id_t lun_object_id, fbe_bool_t force_init_zero);
fbe_status_t fbe_database_get_lun_imported_capacity (fbe_lba_t exported_capacity,
                                                     fbe_lba_t lun_align_size,
                                                     fbe_lba_t *imported_capacity);
fbe_status_t fbe_database_get_lun_exported_capacity (fbe_lba_t imported_capacity,
                                                     fbe_lba_t lun_align_size,
                                                     fbe_lba_t *exported_capacity);

fbe_status_t    fbe_database_is_pvd_consumed_by_raid_group(fbe_object_id_t  pvd_id, fbe_bool_t *is_consumed);

fbe_status_t fbe_database_get_pdo_object_id_by_location(fbe_u32_t bus,
                                                       fbe_u32_t enclosure,
                                                       fbe_u32_t slot,
                                                       fbe_object_id_t*   pdo_obj_id);
fbe_status_t fbe_database_get_serial_number_from_pdo(fbe_object_id_t pdo, 
                                                     fbe_u8_t*       serial_num);
                                                     
fbe_status_t fbe_database_get_total_objects_of_class(fbe_class_id_t class_id, 
                                                     fbe_package_id_t package_id, 
                                                     fbe_u32_t *total_objects_p);
fbe_status_t fbe_database_enumerate_class(fbe_class_id_t class_id,
                                          fbe_package_id_t package_id,
                                          fbe_object_id_t ** obj_array_p, 
                                          fbe_u32_t *num_objects_p);
                                          
fbe_status_t fbe_database_get_esp_object_id(fbe_class_id_t esp_class_id,
                                            fbe_object_id_t *object_id);

fbe_status_t fbe_database_get_object_lifecycle_state(fbe_object_id_t object_id, 
                                                     fbe_lifecycle_state_t * lifecycle_state, fbe_packet_t *packet);

fbe_status_t fbe_database_forward_packet_to_object(fbe_packet_t *packet,
												   fbe_payload_control_operation_opcode_t control_code,
                                                fbe_payload_control_buffer_t buffer,
                                                fbe_payload_control_buffer_length_t buffer_length,
                                                fbe_object_id_t object_id,
                                                fbe_sg_element_t *sg_list,
                                                fbe_u32_t sg_list_count,
                                                fbe_packet_attr_t attr,
                                                fbe_package_id_t package_id);

fbe_bool_t fbe_database_is_table_commit_required(database_config_table_type_t table_type);
fbe_status_t fbe_database_update_table_prior_commit(database_config_table_type_t table_type);
fbe_status_t fbe_database_commit_table(database_config_table_type_t table_type);
fbe_status_t fbe_database_update_local_table_after_peer_commit(void);
fbe_status_t fbe_database_get_next_edge_index_for_extent_pool_lun(fbe_u32_t pool_id, fbe_u32_t *index, fbe_lba_t *offset);
fbe_bool_t fbe_database_is_flash_drive(fbe_drive_type_t drive_type);

#endif /*  FBE_DATABASE_COMMON_H */
