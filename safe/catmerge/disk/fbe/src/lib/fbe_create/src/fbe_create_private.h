#ifndef FBE_CREATE_PRIVATE_H
#define FBE_CREATE_PRIVATE_H

typedef struct fbe_create_lib_physical_drive_info_s
{
    fbe_object_id_t           drive_object_id;
    fbe_u32_t                 bus;
    fbe_u32_t                 enclosure;
    fbe_u32_t                 slot;
    fbe_block_edge_geometry_t block_geometry;
    fbe_lba_t                 exported_capacity;   // exclude fru signature space

    fbe_u8_t                  serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];
} fbe_create_lib_physical_drive_info_t;



fbe_status_t fbe_create_lib_send_control_packet (fbe_payload_control_operation_opcode_t control_code,
        fbe_payload_control_buffer_t buffer,
        fbe_payload_control_buffer_length_t buffer_length,
        fbe_package_id_t package_id,
        fbe_service_id_t service_id,
        fbe_class_id_t class_id,
        fbe_object_id_t object_id,
        fbe_packet_attr_t attr);
fbe_status_t fbe_create_lib_send_control_packet_with_sg(fbe_payload_control_operation_opcode_t control_code,
                                                        fbe_payload_control_buffer_t buffer,
                                                        fbe_payload_control_buffer_length_t buffer_length,
                                                        fbe_sg_element_t *sg_ptr,
                                                        fbe_service_id_t service_id,
                                                        fbe_class_id_t class_id,
                                                        fbe_object_id_t object_id,
                                                        fbe_packet_attr_t attr);
fbe_status_t fbe_create_lib_send_control_packet_with_cpu_id (fbe_payload_control_operation_opcode_t control_code,
        fbe_payload_control_buffer_t buffer,
        fbe_payload_control_buffer_length_t buffer_length,
        fbe_package_id_t package_id,
        fbe_service_id_t service_id,
        fbe_class_id_t class_id,
        fbe_object_id_t object_id,
        fbe_packet_attr_t attr);
fbe_status_t fbe_create_lib_start_database_transaction (fbe_database_transaction_id_t *transaction_id, fbe_u64_t job_number);
fbe_status_t fbe_create_lib_commit_database_transaction (fbe_database_transaction_id_t transaction_id);
fbe_status_t fbe_create_lib_abort_database_transaction (fbe_database_transaction_id_t transaction_id);

fbe_status_t fbe_create_lib_database_service_create_edge(fbe_database_transaction_id_t transaction_id, 
                                                         fbe_object_id_t server_object_id,
                                                         fbe_object_id_t client_object_id, 
                                                         fbe_u32_t client_index, 
                                                         fbe_lba_t client_imported_capacity, 
                                                         fbe_lba_t offset,
                                                         fbe_u8_t*  serial_number,
					       fbe_edge_flags_t      edge_flags);

fbe_status_t fbe_create_lib_database_service_update_pvd_config_type(fbe_database_transaction_id_t transaction_id, 
                                                                    fbe_object_id_t pvd_object_id, 
                                                                    fbe_provision_drive_config_type_t config_type);
fbe_status_t fbe_create_lib_database_service_update_pvd_pool_id(fbe_database_transaction_id_t transaction_id, 
                                                                fbe_object_id_t pvd_object_id, 
                                                                fbe_u32_t pool_id);
fbe_status_t fbe_create_lib_database_service_update_encryption_mode(
                                  fbe_database_transaction_id_t transaction_id, 
                                  fbe_object_id_t object_id,
                                  fbe_base_config_encryption_mode_t mode);

fbe_status_t fbe_create_lib_database_service_lookup_raid_object_id(
        fbe_raid_group_number_t raid_group_id, fbe_object_id_t *rg_object_id_p);

fbe_status_t fbe_create_lib_database_service_scrub_old_user_data(void);

fbe_status_t fbe_create_lib_base_config_validate_capacity (fbe_object_id_t server_object_id,
                                                          fbe_lba_t client_imported_capacity,
                                                          fbe_block_transport_placement_t placement,
                                                          fbe_bool_t b_ignore_offset,
                                                          fbe_lba_t *offset,
                                                          fbe_u32_t *client_index,
                                                          fbe_lba_t *available_capacity);

fbe_status_t fbe_cretae_lib_get_provision_drive_config_type(fbe_object_id_t pvd_object_id,
                                                            fbe_provision_drive_config_type_t * provision_drive_config_type_p,
                                                            fbe_bool_t * end_of_life_state_p);

fbe_status_t fbe_create_lib_get_upstream_edge_count(fbe_object_id_t object_id, 
                                                    fbe_u32_t *number_of_upstream_edges);

fbe_status_t fbe_create_lib_get_upstream_edge_list(fbe_object_id_t object_id, 
                                                   fbe_u32_t *number_of_upstream_objects,
                                                   fbe_object_id_t upstream_object_list[]);

fbe_status_t fbe_create_lib_get_pvd_upstream_edge_list(
                                   fbe_object_id_t object_id, fbe_u32_t *number_of_upstream_objects,
                                   fbe_object_id_t upstream_rg_object_list_array[],
                                   fbe_object_id_t upstream_vd_object_list_array[]);

fbe_status_t fbe_create_lib_get_raid_group_type(fbe_object_id_t object_id, 
                                                 fbe_raid_group_type_t *raid_group_type);

fbe_status_t fbe_create_lib_get_packet_contents (fbe_packet_t *packet_p,
                                                 fbe_job_queue_element_t **out_job_queue_element_p);

fbe_status_t fbe_create_lib_get_object_state(fbe_object_id_t object_id, fbe_lifecycle_state_t * lifecycle_state);


fbe_status_t fbe_pvd_event_log_sniff_verify_enable_disable(fbe_object_id_t pvd_object_id, 
                                                                  fbe_bool_t sniff_verify_state);
fbe_status_t fbe_create_lib_lookup_lun_object_id(fbe_lun_number_t lun_number, fbe_object_id_t *lun_object_id_p);

fbe_status_t fbe_create_lib_get_downstream_edge_geometry(fbe_object_id_t object_id, 
        fbe_block_edge_geometry_t *out_edge_block_geometry);

 fbe_status_t fbe_raid_group_create_check_if_sys_drive(fbe_object_id_t pvd_object_id,
                                                      fbe_bool_t    *is_sys_drive_p);

fbe_status_t fbe_create_lib_validate_system_triple_mirror_healthy(fbe_bool_t *double_degraded);

fbe_status_t fbe_create_lib_get_drive_info_by_serial_number(fbe_u8_t* serial_number, fbe_create_lib_physical_drive_info_t* drive_info);
fbe_status_t fbe_create_lib_get_raid_health(fbe_object_id_t object_id, fbe_bool_t *is_broken);

fbe_status_t fbe_create_lib_get_list_of_downstream_objects(fbe_object_id_t object_id,
                                                                          fbe_object_id_t downstream_object_list[],
                                                                          fbe_u32_t *number_of_downstream_objects);
fbe_status_t fbe_create_lib_database_service_lookup_ext_pool_object_id(fbe_u32_t pool_id, fbe_object_id_t * object_id);
fbe_status_t fbe_create_lib_database_service_lookup_ext_pool_lun_object_id(fbe_u32_t pool_id, fbe_u32_t lun_id, fbe_object_id_t * object_id);

#endif /* FBE_CREATE_PRIVATE_H */
