#ifndef FBE_API_DATABASE_INTERFACE_H
#define FBE_API_DATABASE_INTERFACE_H

/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_api_database_interface.h
 ***************************************************************************
 *
 * @brief
 *  This file contains definitions of functions that are exported by the 
 *  fbe_api_database_interface.
 * 
 * @ingroup fbe_api_storage_extent_package_interface_class_files
 * 
 * @version
 *   10/10/08    sharel - Created
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_api_common.h"
#include "fbe_database.h"

//----------------------------------------------------------------
// Define the top level group for the FBE Storage Extent Package APIs
//----------------------------------------------------------------
/*! @defgroup fbe_api_storage_extent_package_class FBE Storage Extent Package APIs
 *  @brief 
 *    This is the set of definitions for FBE Storage Extent Package APIs.
 *
 *  @ingroup fbe_api_storage_extent_package_interface
 */ 
//----------------------------------------------------------------

//----------------------------------------------------------------
// Define the FBE API Database Interface for the Usurper Interface. 
// This is where all the data structures defined. 
//----------------------------------------------------------------
/*! @defgroup fbe_api_database_interface_usurper_interface FBE API database Usurper Interface
 *  @brief 
 *    This is the set of definitions that comprise the FBE API database class
 *    usurper interface.
 *
 *  @ingroup fbe_api_classes_usurper_interface
 *  @{
 */ 
//----------------------------------------------------------------

FBE_API_CPP_EXPORT_START

/*! @} */ /* end of group fbe_api_database_interface_usurper_interface */

//----------------------------------------------------------------
// Define the group for the FBE API database Interface.  
// This is where all the function prototypes for the FBE API database.
//----------------------------------------------------------------
/*! @defgroup fbe_api_database_interface FBE API database Interface
 *  @brief 
 *    This is the set of FBE API database. 
 *
 *  @details 
 *    In order to access this library, please include fbe_api_database_interface.h.
 *
 *  @ingroup fbe_api_storage_extent_package_class
 *  @{
 */
//----------------------------------------------------------------

fbe_status_t FBE_API_CALL fbe_api_database_lookup_raid_group_by_number(fbe_raid_group_number_t raid_group_id, fbe_object_id_t *object_id);
fbe_status_t FBE_API_CALL fbe_api_database_lookup_lun_by_object_id(fbe_object_id_t object_id, fbe_lun_number_t *lun_number);
fbe_status_t FBE_API_CALL fbe_api_database_lookup_lun_by_number(fbe_lun_number_t lun_number, fbe_object_id_t *object_id);
fbe_status_t FBE_API_CALL fbe_api_database_lookup_raid_group_by_object_id(fbe_object_id_t object_id, fbe_raid_group_number_t *raid_group_id);
fbe_status_t FBE_API_CALL fbe_api_database_get_state(fbe_database_state_t *state);
fbe_status_t FBE_API_CALL fbe_api_database_get_peer_state(fbe_database_state_t *state);

fbe_status_t FBE_API_CALL fbe_api_database_get_service_mode_reason(fbe_database_control_get_service_mode_reason_t *ctrl_reason);
fbe_status_t FBE_API_CALL fbe_api_database_get_tables(fbe_object_id_t object_id, fbe_database_get_tables_t * tables);
fbe_status_t FBE_API_CALL fbe_api_database_get_lun_info(fbe_database_lun_info_t *lun_info);
fbe_status_t FBE_API_CALL fbe_api_database_get_system_object_id(fbe_object_id_t *object_id);
fbe_status_t FBE_API_CALL fbe_api_database_get_system_objects(fbe_class_id_t class_id,
                                                              fbe_object_id_t *system_object_list_p,
                                                              fbe_u32_t total_objects,
                                                              fbe_u32_t *actual_num_objects_p);
fbe_status_t FBE_API_CALL fbe_api_database_is_system_object(fbe_object_id_t object_id_to_check, fbe_bool_t *b_found);
fbe_status_t FBE_API_CALL fbe_api_database_get_stats( fbe_database_control_get_stats_t *get_stats);
fbe_status_t FBE_API_CALL fbe_api_database_start_transaction(fbe_database_transaction_info_t *transaction_info);
fbe_status_t FBE_API_CALL fbe_api_database_abort_transaction(fbe_database_transaction_id_t transaction_id);
fbe_status_t FBE_API_CALL fbe_api_database_create_lun(fbe_database_control_lun_t create_lun);
fbe_status_t FBE_API_CALL fbe_api_database_destroy_raid(fbe_database_control_destroy_object_t destroy_raid);
fbe_status_t FBE_API_CALL fbe_api_database_destroy_lun(fbe_database_control_destroy_object_t lun);
fbe_status_t FBE_API_CALL fbe_api_database_destroy_vd(fbe_database_control_destroy_object_t vd);
fbe_status_t FBE_API_CALL fbe_api_database_update_raid_group(fbe_database_control_update_raid_t raid_grp);
fbe_status_t FBE_API_CALL fbe_api_database_commit_transaction(fbe_database_transaction_id_t transaction_id);
fbe_status_t FBE_API_CALL fbe_api_database_set_power_save(fbe_database_power_save_t power_save_info);  
fbe_status_t FBE_API_CALL fbe_api_database_get_power_save(fbe_database_power_save_t *power_save_info); 
fbe_status_t FBE_API_CALL fbe_api_database_set_encryption(fbe_database_encryption_t encryption_info);  
fbe_status_t FBE_API_CALL fbe_api_database_get_encryption(fbe_database_encryption_t *encryption_info); 


fbe_status_t FBE_API_CALL fbe_api_database_get_transaction_info(database_transaction_t *ptr_info);

fbe_status_t FBE_API_CALL fbe_api_database_system_db_send_cmd(fbe_database_control_system_db_op_t *cmd_buffer);

fbe_status_t FBE_API_CALL fbe_api_database_set_chassis_replacement_flag(fbe_bool_t whether_set);


fbe_status_t FBE_API_CALL fbe_api_database_get_fru_descriptor_info(fbe_homewrecker_fru_descriptor_t* out_descriptor, 
                                                                                  fbe_homewrecker_get_fru_disk_mask_t disk_mask,
                                                                                  fbe_bool_t* warning);

fbe_status_t FBE_API_CALL fbe_api_database_set_fru_descriptor_info(fbe_database_control_set_fru_descriptor_t* in_fru_descriptor,
                                                                                  fbe_database_control_set_fru_descriptor_mask_t mask);

fbe_status_t FBE_API_CALL fbe_api_database_get_disk_signature_info(fbe_fru_signature_t* signature);
fbe_status_t FBE_API_CALL fbe_api_database_set_disk_signature_info(fbe_fru_signature_t* signature);
fbe_status_t FBE_API_CALL fbe_api_database_clear_disk_signature_info(fbe_fru_signature_t* signature);

fbe_status_t FBE_API_CALL fbe_api_database_system_send_clone_cmd(fbe_database_control_clone_object_t *cmd_buffer);
fbe_status_t FBE_API_CALL fbe_api_database_system_persist_prom_wwnseed_cmd(fbe_u32_t *cmd_buffer);
fbe_status_t FBE_API_CALL fbe_api_database_system_obtain_prom_wwnseed_cmd(fbe_u32_t *cmd_buffer);
fbe_status_t FBE_API_CALL fbe_api_database_set_time_threshold(fbe_database_time_threshold_t time_threshold_info);  
fbe_status_t FBE_API_CALL fbe_api_database_get_time_threshold(fbe_database_time_threshold_t *time_threshold_info); 
fbe_status_t FBE_API_CALL fbe_api_database_lookup_lun_by_wwid(fbe_assigned_wwid_t wwid, fbe_object_id_t *object_id);

fbe_status_t FBE_API_CALL fbe_api_database_get_compat_mode(fbe_compatibility_mode_t *commpat_mode);
fbe_status_t FBE_API_CALL fbe_api_database_commit(void);
fbe_status_t FBE_API_CALL fbe_api_set_ndu_state(fbe_set_ndu_state_t ndu_state);
fbe_status_t FBE_API_CALL fbe_api_database_get_all_luns(fbe_database_lun_info_t *lun_list, fbe_u32_t expected_count, fbe_u32_t *actual_count);
fbe_status_t FBE_API_CALL fbe_api_database_set_emv_params(fbe_database_emv_t *emv);
fbe_status_t FBE_API_CALL fbe_api_database_get_emv_params(fbe_database_emv_t *emv);
fbe_status_t FBE_API_CALL fbe_api_database_get_peer_sep_version(fbe_u64_t *version);
fbe_status_t FBE_API_CALL fbe_api_database_set_peer_sep_version(fbe_u64_t *version);
fbe_status_t FBE_API_CALL fbe_api_database_get_all_raid_groups(fbe_database_raid_group_info_t *rg_list, fbe_u32_t expected_count, fbe_u32_t *actual_count);
fbe_status_t FBE_API_CALL fbe_api_database_get_all_pvds(fbe_database_pvd_info_t *pvd_list, fbe_u32_t expected_count, fbe_u32_t *actual_count);
fbe_status_t FBE_API_CALL fbe_api_database_get_raid_group(fbe_object_id_t rg_id, fbe_database_raid_group_info_t *rg);
fbe_status_t FBE_API_CALL fbe_api_database_get_pvd(fbe_object_id_t pvd_id, fbe_database_pvd_info_t *pvd); 
fbe_status_t FBE_API_CALL fbe_api_database_stop_all_background_service(fbe_bool_t update_bgs_on_both_sp);
fbe_status_t FBE_API_CALL fbe_api_database_restart_all_background_service(void);
fbe_status_t FBE_API_CALL fbe_api_database_set_load_balance(fbe_bool_t is_enable);
fbe_status_t FBE_API_CALL fbe_api_database_get_system_db_header(database_system_db_header_t *out_db_header);
fbe_status_t FBE_API_CALL fbe_api_database_set_system_db_header(database_system_db_header_t *in_db_header);
fbe_status_t FBE_API_CALL fbe_api_database_init_system_db_header(database_system_db_header_t *out_db_header);
fbe_status_t FBE_API_CALL fbe_api_database_persist_system_db_header(database_system_db_header_t *in_db_header);
fbe_status_t FBE_API_CALL fbe_api_database_get_system_object_recreate_flags(fbe_database_system_object_recreate_flags_t *system_object_op_flags);
fbe_status_t FBE_API_CALL fbe_api_database_persist_system_object_recreate_flags(fbe_database_system_object_recreate_flags_t *system_object_op_flags);
fbe_status_t FBE_API_CALL fbe_api_database_set_system_object_recreation(fbe_object_id_t object_id, fbe_u8_t recreate_flags);
fbe_status_t FBE_API_CALL fbe_api_database_reset_system_object_recreation(void);
fbe_status_t FBE_API_CALL fbe_api_database_generate_configuration_for_system_parity_rg_and_lun(fbe_bool_t ndb);
fbe_status_t FBE_API_CALL fbe_api_database_generate_system_object_config_and_persist(fbe_object_id_t object_id);
fbe_status_t FBE_API_CALL fbe_api_database_generate_config_for_all_system_rg_and_lun_and_persist(void);
    
fbe_status_t FBE_API_CALL fbe_api_database_online_planned_drive(fbe_database_control_online_planned_drive_t* online_drive);


                              
fbe_status_t FBE_API_CALL fbe_api_database_cleanup_reinit_persit_service(void);
fbe_status_t FBE_API_CALL fbe_api_database_restore_user_configuration(fbe_database_config_entries_restore_t *restore_op_p); 

fbe_status_t FBE_API_CALL fbe_api_database_get_object_tables_in_range(void * in_buffer,fbe_u32_t in_buffer_size,fbe_object_id_t start_object_id,
                                                               fbe_object_id_t end_object_id,fbe_u32_t *actual_count);
fbe_status_t FBE_API_CALL fbe_api_database_get_user_tables_in_range(void * in_buffer,fbe_u32_t in_buffer_size,fbe_object_id_t start_object_id,
                                                               fbe_object_id_t end_object_id,fbe_u32_t *actual_count);
fbe_status_t FBE_API_CALL fbe_api_database_get_edge_tables_in_range(void * in_buffer,fbe_u32_t in_buffer_size,fbe_object_id_t start_object_id,
                                                               fbe_object_id_t end_object_id,fbe_u32_t *actual_count);

fbe_status_t FBE_API_CALL fbe_api_database_persist_system_db(void * in_buffer,
                                                                          fbe_u32_t in_buffer_size,
                                                                          fbe_u32_t system_object_count,
                                                                          fbe_u32_t *actual_count);
fbe_status_t FBE_API_CALL fbe_api_database_get_max_configurable_objects(fbe_u32_t *max_configurable_objects);
fbe_status_t FBE_API_CALL fbe_api_database_set_invalidate_config_flag(void);
fbe_status_t FBE_API_CALL fbe_api_database_clear_invalidate_config_flag(void);
fbe_status_t FBE_API_CALL fbe_api_database_get_disk_ddmi_data(fbe_u8_t *out_ddmi_data, fbe_u32_t in_bus, fbe_u32_t in_enclosure,
                                                                            fbe_u32_t in_slot, fbe_u32_t expected_count, fbe_u32_t *actual_count);
fbe_status_t FBE_API_CALL fbe_api_database_versioning_operation(fbe_database_control_versioning_op_code_t  *versioning_op_code_p);

fbe_status_t FBE_API_CALL fbe_api_database_add_hook(fbe_database_hook_type_t hook_type);
fbe_status_t FBE_API_CALL fbe_api_database_remove_hook(fbe_database_hook_type_t hook_type);
fbe_status_t FBE_API_CALL fbe_api_database_wait_hook(fbe_database_hook_type_t hook_type, fbe_u32_t timeout_ms);
fbe_status_t FBE_API_CALL fbe_api_database_lun_operation_ktrace_warning(fbe_object_id_t lun_id, 
                                              fbe_database_lun_operation_code_t operation,
                                              fbe_status_t result);
fbe_status_t FBE_API_CALL fbe_api_database_get_lun_raid_info(fbe_object_id_t lun_object_id,
                                                             fbe_database_get_sep_shim_raid_info_t *raid_info_p);
fbe_status_t FBE_API_CALL fbe_api_database_setup_encryption_key(fbe_database_control_setup_encryption_key_t *encryption_info);
fbe_status_t FBE_API_CALL fbe_api_database_setup_kek(fbe_database_control_setup_kek_t *kek_encryption_info);
fbe_status_t FBE_API_CALL fbe_api_database_destroy_kek(fbe_database_control_destroy_kek_t *kek_encryption_info);
fbe_status_t FBE_API_CALL fbe_api_database_setup_kek_kek(fbe_database_control_setup_kek_kek_t *kek_kek_encryption_info);
fbe_status_t FBE_API_CALL fbe_api_database_destroy_kek_kek(fbe_database_control_destroy_kek_kek_t *kek_kek_destroy_info);
fbe_status_t FBE_API_CALL fbe_api_database_reestablish_kek_kek(fbe_database_control_reestablish_kek_kek_t *kek_kek_handle_info);
fbe_status_t FBE_API_CALL fbe_api_database_set_port_encryption_mode(fbe_database_control_port_encryption_mode_t *mode_info);
fbe_status_t FBE_API_CALL fbe_api_database_get_port_encryption_mode(fbe_database_control_port_encryption_mode_t *mode_info);
fbe_status_t FBE_API_CALL fbe_api_database_get_total_locked_objects_of_class(database_class_id_t db_class_id, fbe_u32_t *total_objects );
fbe_status_t FBE_API_CALL fbe_api_database_get_locked_object_info_of_class(database_class_id_t db_class_id,
                                                                            fbe_database_control_get_locked_object_info_t * info_buffer, 
                                                                            fbe_u32_t total_objects, 
                                                                            fbe_u32_t *actual_objects);
fbe_status_t FBE_API_CALL fbe_api_database_get_encryption_info_of_class(database_class_id_t db_class_id,
                                                                           fbe_database_control_get_locked_object_info_t * info_buffer, 
                                                                           fbe_u32_t total_objects, 
                                                                           fbe_u32_t *actual_objects);

fbe_status_t FBE_API_CALL fbe_api_database_setup_encryption_rekey(fbe_database_control_setup_encryption_key_t *encryption_info);
fbe_status_t FBE_API_CALL fbe_api_database_set_capacity_limit(fbe_database_capacity_limit_t *capacity_limit);                                            
fbe_status_t FBE_API_CALL fbe_api_database_get_capacity_limit(fbe_database_capacity_limit_t *capacity_limit);                                            
fbe_status_t FBE_API_CALL fbe_api_database_remove_encryption_keys(fbe_database_control_remove_encryption_keys_t *encryption_info);
fbe_status_t FBE_API_CALL fbe_api_database_get_system_encryption_info(fbe_database_system_encryption_info_t *sys_encryption_info);
fbe_status_t FBE_API_CALL fbe_api_database_get_system_encryption_progress(fbe_database_system_encryption_progress_t *encryption_progress);
fbe_status_t FBE_API_CALL fbe_api_database_update_drive_keys(fbe_database_control_update_drive_key_t *encryption_info);
fbe_status_t FBE_API_CALL fbe_api_database_get_be_port_info(fbe_database_control_get_be_port_info_t *be_port_info);
fbe_status_t FBE_API_CALL fbe_api_database_get_drive_sn_for_raid(fbe_database_control_get_drive_sn_t *drive_sn);
fbe_status_t FBE_API_CALL fbe_api_database_get_encryption_paused(fbe_bool_t * paused);

fbe_status_t FBE_API_CALL fbe_api_database_get_backup_info(fbe_database_backup_info_t * backup_info);
fbe_status_t FBE_API_CALL fbe_api_database_set_backup_info(fbe_database_backup_info_t * backup_info);

fbe_status_t FBE_API_CALL fbe_api_database_set_update_db_on_peer_op(fbe_database_control_db_update_on_peer_op_t update_op);

fbe_status_t FBE_API_CALL fbe_api_database_get_system_scrub_progress(fbe_database_system_scrub_progress_t *scrub_progress);
fbe_status_t FBE_API_CALL fbe_api_database_set_poll_interval(fbe_u32_t poll_interval);
fbe_status_t FBE_API_CALL fbe_api_database_set_drive_tier_enabled(fbe_bool_t enabled);

fbe_status_t FBE_API_CALL fbe_api_database_set_debug_flags(fbe_database_debug_flags_t database_debug_flags);
fbe_status_t FBE_API_CALL fbe_api_database_get_debug_flags(fbe_database_debug_flags_t *database_debug_flags_p);

fbe_status_t FBE_API_CALL fbe_api_database_validate_database(fbe_database_validate_request_type_t caller,
                                                             fbe_database_validate_failure_action_t failure_action);

fbe_status_t FBE_API_CALL fbe_api_database_set_garbage_collection_debouncer(fbe_u32_t timeout_ms);
fbe_status_t FBE_API_CALL fbe_api_database_get_additional_supported_drive_types(fbe_database_additional_drive_types_supported_t *types_p);
fbe_status_t FBE_API_CALL fbe_api_database_set_additional_supported_drive_types(fbe_database_control_set_supported_drive_type_t *set_drive_type_p);

fbe_status_t FBE_API_CALL fbe_api_database_lookup_ext_pool_by_number(fbe_u32_t pool_id, fbe_object_id_t *object_id);
fbe_status_t FBE_API_CALL fbe_api_database_lookup_ext_pool_lun_by_number(fbe_u32_t lun_id, fbe_object_id_t *object_id);
fbe_status_t FBE_API_CALL fbe_api_database_check_bootflash_mode(fbe_bool_t *bootflash_mode_flag);

fbe_status_t FBE_API_CALL fbe_api_database_update_mirror_pvd_map(fbe_object_id_t rg_obj_id, fbe_u16_t edge_index, fbe_u8_t *sn, fbe_u32_t size);
fbe_status_t FBE_API_CALL fbe_api_database_get_mirror_pvd_map(fbe_object_id_t rg_obj_id, fbe_u16_t edge_index, fbe_u8_t *sn, fbe_u32_t size);



/*! @} */ /* end of group fbe_api_database_interface */

//----------------------------------------------------------------
// Define the group for all FBE Storage Extent Package APIs Interface class files.  
// This is where all the class files that belong to the FBE API Storage Extent
// Package define. In addition, this group is displayed in the FBE Classes
// module.
//----------------------------------------------------------------
/*! @defgroup fbe_api_storage_extent_package_interface_class_files FBE Storage Extent Package APIs Interface Class Files 
 *  @brief 
 *    This is the set of files for the FBE Storage Extent Package APIs Interface.
 * 
 *  @ingroup fbe_api_classes
 */
//----------------------------------------------------------------
FBE_API_CPP_EXPORT_END

#endif /*FBE_API_SEP_DATABASE_INTERFACE_H*/


