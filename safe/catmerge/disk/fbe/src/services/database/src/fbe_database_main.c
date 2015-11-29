/***************************************************************************
* Copyright (C) EMC Corporation 2010-2011
* All rights reserved.
* Licensed material -- property of EMC Corporation
***************************************************************************/

/*!**************************************************************************
* @file fbe_database_main.c
***************************************************************************
*
* @brief
*  This file contains database service functions including control entry.
*  
* @version
*  12/15/2010 - Created. 
*
***************************************************************************/

/*************************
*   INCLUDE FILES
*************************/
#include "fbe_database_private.h"
#include "fbe_database_config_tables.h"
#include "fbe_database_cmi_interface.h"
#include "fbe_private_space_layout.h"
#include "fbe_database_system_db_interface.h"
#include "fbe_database_homewrecker_db_interface.h"
#include "fbe_database_drive_connection.h"
#include "fbe_database_registry.h"
#include "fbe/fbe_base_config.h"
#include "fbe_database_hook.h"

#include "fbe_cmi.h"
#include "fbe/fbe_event_log_api.h"                  // for fbe_event_log_write
#include "fbe/fbe_event_log_utils.h"                // for message codes
#include "VolumeAttributes.h"
#include "fbe_raid_library.h"

/* Declare our service methods */
fbe_status_t fbe_database_control_entry(fbe_packet_t * packet);
fbe_service_methods_t fbe_database_service_methods = {FBE_SERVICE_ID_DATABASE, fbe_database_control_entry};

/* Globals */
fbe_database_service_t fbe_database_service;

/* Locals*/
static fbe_bool_t fbe_database_vault_drive_tier_enabled = FBE_DATABASE_EXTENDED_CACHE_ENABLED; /* set default */ 
static fbe_database_debug_flags_t fbe_database_default_debug_flags = 0;


/* private functions */
static fbe_status_t database_init(fbe_packet_t * packet);
static fbe_status_t database_destroy(fbe_packet_t * packet);

static fbe_status_t database_control_transaction_start(fbe_packet_t * packet);
static fbe_status_t database_control_transaction_commit(fbe_packet_t * packet);
static fbe_status_t database_control_transaction_abort(fbe_packet_t * packet);
static fbe_status_t database_control_create_vd(fbe_packet_t * packet);
static fbe_status_t database_control_create_pvd(fbe_packet_t * packet);
static fbe_status_t database_control_create_lun(fbe_packet_t * packet);
static fbe_status_t database_control_create_raid(fbe_packet_t * packet);
static fbe_status_t database_control_clone_object(fbe_packet_t * packet);
static fbe_status_t database_persist_prom_wwnseed(fbe_packet_t * packet);
static fbe_status_t database_obtain_prom_wwnseed(fbe_packet_t * packet);

static fbe_status_t database_control_create_edge(fbe_packet_t * packet);
static fbe_status_t database_control_destroy_pvd(fbe_packet_t * packet);
static fbe_status_t database_control_destroy_vd(fbe_packet_t * packet);
static fbe_status_t database_control_destroy_raid(fbe_packet_t * packet);
static fbe_status_t database_control_destroy_lun(fbe_packet_t * packet);
static fbe_status_t database_control_destroy_edge(fbe_packet_t * packet);
static fbe_status_t database_control_lookup_raid_by_number(fbe_packet_t * packet);
static fbe_status_t database_control_lookup_lun_by_number(fbe_packet_t * packet);
static fbe_status_t database_control_lookup_raid_by_object_id(fbe_packet_t * packet);
static fbe_status_t database_control_lookup_lun_by_object_id(fbe_packet_t * packet);
static fbe_status_t database_control_get_lun_info(fbe_packet_t *packet);
static fbe_status_t database_control_get_raid_user_private(fbe_packet_t *packet);
static fbe_status_t database_control_update_pvd(fbe_packet_t * packet);
static fbe_status_t database_control_update_pvd_block_size(fbe_packet_t * packet);
static fbe_status_t database_control_check_bootflash_mode(fbe_packet_t * packet);
static fbe_status_t database_control_update_encryption_mode(fbe_packet_t *packet);
static fbe_status_t database_control_update_vd(fbe_packet_t * packet);
static fbe_status_t database_control_update_lun(fbe_packet_t * packet);
static fbe_status_t database_control_update_raid(fbe_packet_t * packet);
static fbe_status_t database_control_restore_user_configuration(fbe_packet_t * packet);
static fbe_status_t database_control_cleanup_reinit_persist_service(fbe_packet_t * packet);
static fbe_status_t database_control_get_state(fbe_packet_t * packet);
static fbe_status_t database_control_get_service_mode_reason(fbe_packet_t * packet);
static fbe_status_t database_control_get_tables(fbe_packet_t * packet);
static fbe_status_t database_control_enumerate_system_objects(fbe_packet_t *packet);
static fbe_status_t database_control_set_power_save(fbe_packet_t *packet);
static fbe_status_t database_control_get_power_save(fbe_packet_t *packet);
static fbe_status_t database_control_update_peer_config(fbe_packet_t *packet);
static fbe_status_t database_control_update_system_spare_config(fbe_packet_t *packet);
static fbe_status_t database_map_psl_lun_to_attributes(fbe_object_id_t lun_object_id, fbe_u32_t *attributes);
static fbe_status_t database_control_get_stats(fbe_packet_t *packet);
static fbe_status_t database_control_get_transaction_info(fbe_packet_t * packet);
static fbe_status_t database_control_destroy_all_objects(void);
static fbe_status_t database_control_system_db_op(fbe_packet_t * packet);
static fbe_char_t *database_get_service_mode_reason_string(fbe_database_service_mode_reason_t reason);
static fbe_status_t database_control_get_fru_descriptor(fbe_packet_t * packet);
static fbe_status_t database_control_set_encryption(fbe_packet_t *packet);
static fbe_status_t database_control_get_encryption(fbe_packet_t *packet);
static fbe_status_t database_control_get_encryption_paused(fbe_packet_t *packet);
static fbe_status_t database_control_set_encryption_paused(fbe_packet_t *packet);
static fbe_status_t database_control_get_capacity_limit(fbe_packet_t *packet);
static fbe_status_t database_control_set_capacity_limit(fbe_packet_t *packet);

static fbe_status_t database_control_set_fru_descriptor(fbe_packet_t * packet);

static fbe_status_t database_control_get_disk_signature(fbe_packet_t * packet);
static fbe_status_t database_control_set_disk_signature(fbe_packet_t * packet);
static fbe_status_t database_control_clear_disk_signature(fbe_packet_t * packet);

static fbe_status_t database_control_set_time_threshold(fbe_packet_t *packet);
static fbe_status_t database_control_get_time_threshold(fbe_packet_t *packet);

static fbe_status_t database_control_generate_configuration_for_system_parity_RG_and_LUN(fbe_packet_t * packet);

static fbe_status_t database_get_next_available_lun_number(fbe_lun_number_t *lun_number);
static fbe_status_t database_get_next_available_rg_number(fbe_raid_group_number_t *rg_number, database_class_id_t db_class_id);

static fbe_status_t database_get_next_free_number(fbe_u32_t max_number,
                                                  fbe_u64_t *bitmap,
                                                  fbe_u32_t *out_num);

static fbe_status_t database_mark_number_used(fbe_u32_t max_number,
                                                  fbe_u64_t *bitmap,
                                                  fbe_u32_t in_number);

static fbe_status_t database_mark_number_unused(fbe_u32_t max_number,
                                                  fbe_u64_t *bitmap,
                                                  fbe_u32_t in_number);

static fbe_status_t database_fill_free_entry_bitmap(fbe_u32_t max_number,
                                                    database_table_t *table_ptr,
                                                    fbe_u64_t *bitmap,
                                                    database_class_id_t db_class_id);

static fbe_status_t database_mark_lun_number_used(fbe_lun_number_t lun_number, fbe_u64_t *bitmap);
static fbe_status_t database_mark_rg_number_used(fbe_raid_group_number_t rg_number, fbe_u64_t *bitmap);
static fbe_status_t database_control_lookup_lun_by_wwn(fbe_packet_t * packet);

static fbe_status_t database_control_update_peer_system_bg_service_flag(fbe_packet_t * packet);

static fbe_status_t database_control_clear_pending_transaction(fbe_packet_t* packet_p);
static fbe_status_t database_control_get_all_luns(fbe_packet_t *packet);
static fbe_status_t database_get_lun_info(fbe_database_lun_info_t *lun_info, fbe_packet_t *packet);
static fbe_status_t database_control_get_all_raid_groups(fbe_packet_t *packet);
static fbe_status_t database_get_raid_group_info(fbe_database_raid_group_info_t *rg_info, fbe_packet_t *packet);
static fbe_status_t database_control_get_all_pvds(fbe_packet_t *packet);
static fbe_status_t database_get_pvd_info(fbe_database_pvd_info_t *pvd_info, fbe_packet_t *packet);
static fbe_status_t database_control_get_raid_group(fbe_packet_t *packet);
static fbe_status_t database_control_get_pvd(fbe_packet_t *packet);

static fbe_status_t database_control_commit_database_tables(fbe_packet_t* packet_p);
static fbe_status_t database_check_entry_version_header(void);
static fbe_status_t database_ndu_commit_job_notification_callback(fbe_object_id_t object_id, 
                                                                        fbe_notification_info_t notification_info,
                                                                        fbe_notification_context_t context);
static fbe_status_t database_control_ndu_commit(fbe_packet_t * packet);
static fbe_status_t database_control_get_compat_mode(fbe_packet_t * packet);
static fbe_status_t database_control_versioning_operation(fbe_packet_t * packet);
static fbe_status_t database_control_get_disk_ddmi_data(fbe_packet_t * packet);



static fbe_status_t database_control_get_shared_emv_info(fbe_packet_t *packet);
static fbe_status_t database_control_set_shared_emv_info(fbe_packet_t *packet);

static fbe_status_t database_control_set_load_balance(fbe_packet_t *packet, fbe_bool_t is_enable);

static fbe_status_t database_control_get_system_db_header(fbe_packet_t * packet);
static fbe_status_t database_control_set_system_db_header(fbe_packet_t * packet);
static fbe_status_t database_control_init_system_db_header(fbe_packet_t * packet);
static fbe_status_t database_control_persist_system_db_header(fbe_packet_t * packet);

static fbe_status_t database_control_get_system_object_recreate_flags(fbe_packet_t *packet);
static fbe_status_t database_control_persist_system_object_recreate_flags(fbe_packet_t *packet);
static fbe_status_t database_control_generate_system_object_config_and_persist(fbe_packet_t *packet);
static fbe_status_t database_control_generate_all_system_rg_and_lun_config_and_persist(fbe_packet_t *packet);

static fbe_status_t database_control_make_planned_drive_online(fbe_packet_t * packet);
static fbe_status_t database_control_persist_system_db_header(fbe_packet_t * packet);
static fbe_status_t database_control_get_tables_in_range_with_type(fbe_packet_t *packet);

static fbe_status_t database_control_persist_system_db(fbe_packet_t *packet);
static fbe_status_t database_control_get_max_configurable_objects(fbe_packet_t * packet);
static fbe_status_t database_control_get_object_tables_in_range(fbe_packet_t *packet);
static fbe_status_t database_control_get_user_tables_in_range(fbe_packet_t *packet);
static fbe_status_t database_control_get_edge_tables_in_range(fbe_packet_t *packet);
static fbe_status_t fbe_database_get_rg_free_non_contigous_capacity(fbe_database_raid_group_info_t *rg_info, fbe_packet_t *packet);
static fbe_status_t fbe_database_get_rg_free_contigous_capacity(fbe_database_raid_group_info_t *rg_info, fbe_packet_t *packet);
static fbe_status_t database_control_connect_pvds(fbe_packet_t* packet);
static fbe_status_t database_control_set_invalidate_config_flag(fbe_packet_t *packet);
static fbe_status_t database_perform_hook(fbe_packet_t * packet);

static fbe_status_t database_trace_lun_user_info(database_user_entry_t *lun_user_entry);
static fbe_status_t database_trace_rg_user_info(database_user_entry_t *rg_user_entry);
static fbe_status_t database_control_lu_operation_from_cli(fbe_packet_t * packet);
static fbe_status_t fbe_database_get_lun_raid_info(fbe_packet_t * packet);
static fbe_status_t database_control_get_peer_state(fbe_packet_t * packet);
static fbe_status_t database_update_pvd_per_type(database_object_entry_t  *new_pvd_entry, fbe_update_pvd_type_t update_type, fbe_database_control_update_pvd_t  *update_pvd);
static fbe_status_t fbe_database_control_get_total_locked_objects_of_class(fbe_packet_t * packet);
static fbe_status_t fbe_database_control_get_locked_object_info_of_class(fbe_packet_t * packet);
static fbe_status_t database_commit_psl_changes(fbe_packet_t * packet);
static fbe_bool_t   database_new_system_lun_object_needed(void);
static fbe_status_t fbe_database_control_scrub_old_user_data_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context);
static fbe_status_t fbe_database_control_get_be_port_info(fbe_packet_t * packet);
static fbe_status_t database_control_get_backup_info(fbe_packet_t *packet);
static fbe_status_t database_control_set_backup_info(fbe_packet_t *packet);
static fbe_status_t database_control_set_update_peer_db(fbe_packet_t *packet);
static fbe_status_t fbe_database_control_get_drive_sn_for_raid(fbe_packet_t * packet);
static fbe_status_t fbe_database_control_set_extended_cache_enabled(fbe_packet_t *packet);
static fbe_status_t fbe_database_control_set_garbage_collection_debouncer(fbe_packet_t *packet);

static fbe_status_t fbe_database_control_get_pvd_list_for_ext_pool(fbe_packet_t *packet);
static fbe_status_t fbe_database_control_create_extent_pool(fbe_packet_t * packet);
static fbe_status_t fbe_database_control_create_ext_pool_lun(fbe_packet_t * packet);
static fbe_status_t fbe_database_control_destroy_extent_pool(fbe_packet_t * packet);
static fbe_status_t fbe_database_control_destroy_ext_pool_lun(fbe_packet_t * packet);
static fbe_status_t database_control_lookup_ext_pool_by_number(fbe_packet_t * packet);
static fbe_status_t database_control_lookup_ext_pool_lun_by_number(fbe_packet_t * packet);

static fbe_status_t fbe_database_control_mark_pvd_swap_pending(fbe_packet_t *packet);
static fbe_status_t fbe_database_control_clear_pvd_swap_pending(fbe_packet_t *packet);
static fbe_status_t fbe_database_control_set_debug_flags(fbe_packet_t *packet);
static fbe_status_t fbe_database_control_get_debug_flags(fbe_packet_t *packet);
static fbe_status_t fbe_database_control_is_validation_allowed(fbe_packet_t *packet);
static fbe_status_t fbe_database_control_validate_database(fbe_packet_t *packet);
static fbe_status_t fbe_database_control_enter_degraded_mode(fbe_packet_t *packet);
static fbe_status_t fbe_database_control_get_peer_sep_version(fbe_packet_t *packet);
static fbe_status_t fbe_database_control_set_peer_sep_version(fbe_packet_t *packet);
static fbe_status_t fbe_database_control_get_supported_drive_types(fbe_packet_t *packet_p);
static fbe_status_t fbe_database_control_set_supported_drive_types(fbe_packet_t *packet_p);

static fbe_status_t fbe_database_control_update_mirror_pvd_map(fbe_packet_t *packet);
static fbe_status_t fbe_database_control_get_mirror_pvd_map(fbe_packet_t *packet);

static fbe_status_t fbe_database_get_sep_shim_ext_lun_raid_info(fbe_database_get_sep_shim_raid_info_t *sep_shim_info);
static fbe_status_t database_get_ext_lun_info(fbe_database_lun_info_t *lun_info, fbe_packet_t *packet);
static fbe_status_t fbe_database_initialize_supported_drive_types(fbe_database_additional_drive_types_supported_t *supported_drive_types);

/**********************************************************************************************************************************************/

fbe_database_service_t *fbe_database_get_database_service(void)
{
    return(&fbe_database_service); 
}


fbe_status_t fbe_database_control_entry(fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_INVALID;
    fbe_payload_control_operation_opcode_t control_code;

    control_code = fbe_transport_get_control_code(packet);
    if(control_code == FBE_BASE_SERVICE_CONTROL_CODE_INIT) {
        status = database_init(packet);
        return status;
    }

    if(!fbe_base_service_is_initialized((fbe_base_service_t *) &fbe_database_service)){
        fbe_transport_set_status(packet, FBE_STATUS_NOT_INITIALIZED, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_NOT_INITIALIZED;
    }

    switch(control_code) {
        case FBE_BASE_SERVICE_CONTROL_CODE_DESTROY:
            status = database_destroy(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_TRANSACTION_START:
            status = database_control_transaction_start(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_TRANSACTION_COMMIT:
            status = database_control_transaction_commit(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_TRANSACTION_ABORT:
            status = database_control_transaction_abort(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_UPDATE_PVD:
            status = database_control_update_pvd(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_UPDATE_PVD_BLOCK_SIZE:
            status = database_control_update_pvd_block_size(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_UPDATE_ENCRYPTION_MODE:
            status = database_control_update_encryption_mode(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_CHECK_BOOTFLASH_MODE:
            status = database_control_check_bootflash_mode(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_CREATE_PVD:
            status = database_control_create_pvd(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_DESTROY_PVD:
            status = database_control_destroy_pvd(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_CREATE_VD:
            status = database_control_create_vd(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_UPDATE_VD:
            status = database_control_update_vd(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_DESTROY_VD:
            status = database_control_destroy_vd(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_CREATE_RAID:
            status = database_control_create_raid( packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_DESTROY_RAID:
            status = database_control_destroy_raid( packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_LOOKUP_RAID_BY_NUMBER:            
            status = database_control_lookup_raid_by_number(packet);               
            break;
        case FBE_DATABASE_CONTROL_CODE_LOOKUP_RAID_BY_OBJECT_ID:
            status = database_control_lookup_raid_by_object_id(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_UPDATE_RAID:
             status = database_control_update_raid(packet);
             break;
        case FBE_DATABASE_CONTROL_CODE_GET_RAID_USER_PRIVATE:
             status = database_control_get_raid_user_private(packet);
             break;

        case FBE_DATABASE_CONTROL_CODE_CREATE_LUN:
            status = database_control_create_lun( packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_DESTROY_LUN:
            status = database_control_destroy_lun( packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_UPDATE_LUN:
            status = database_control_update_lun( packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_LOOKUP_LUN_BY_NUMBER:
            status = database_control_lookup_lun_by_number(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_LOOKUP_LUN_BY_OBJECT_ID:
            status = database_control_lookup_lun_by_object_id(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_GET_LUN_INFO:
            status = database_control_get_lun_info(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_CLONE_OBJECT:
            status = database_control_clone_object(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_PERSIST_PROM_WWNSEED:
            status = database_persist_prom_wwnseed(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_OBTAIN_PROM_WWNSEED:
            status = database_obtain_prom_wwnseed(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_CREATE_EDGE:
            status = database_control_create_edge(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_DESTROY_EDGE:
            status = database_control_destroy_edge (packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_GET_STATS:
             status = database_control_get_stats(packet);
             break;
        case FBE_DATABASE_CONTROL_CODE_GET_STATE:
            status = database_control_get_state(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_GET_SERVICE_MODE_REASON:
            status = database_control_get_service_mode_reason(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_ENUMERATE_SYSTEM_OBJECTS:
            status = database_control_enumerate_system_objects(packet);
            break;
        case FBE_DATABASE_CONTROL_GET_ENCRYPTION:
            status = database_control_get_encryption(packet);
            break;
        case FBE_DATABASE_CONTROL_SET_ENCRYPTION:
            status = database_control_set_encryption(packet);
            break;
        case FBE_DATABASE_CONTROL_GET_ENCRYPTION_PAUSED:
            status = database_control_get_encryption_paused(packet);
            break;
        case FBE_DATABASE_CONTROL_SET_ENCRYPTION_PAUSED:
            status = database_control_set_encryption_paused(packet);
            break;
        case FBE_DATABASE_CONTROL_GET_POWER_SAVE:
            status = database_control_get_power_save(packet);
            break;
        case FBE_DATABASE_CONTROL_SET_POWER_SAVE:
            status = database_control_set_power_save(packet);
            break;
            //      case FBE_DATABASE_CONTROL_CODE_LOOKUP_SYSTEM_OBJECT_ID:
            //          status = database_control_lookup_system_object_id(packet);
            //          break;
            //      case FBE_DATABASE_CONTROL_CODE_GET_INTERNALS:
            //          status = database_control_get_internals(packet);
            //          break;
        case FBE_DATABASE_CONTROL_CODE_UPDATE_SYSTEM_SPARE_CONFIG:
            status = database_control_update_system_spare_config(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_GET_CAPACITY_LIMIT:
            status = database_control_get_capacity_limit(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_SET_CAPACITY_LIMIT:
            status = database_control_set_capacity_limit(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_GET_EMV:
            status = database_control_get_shared_emv_info(packet);
             break;
        case FBE_DATABASE_CONTROL_CODE_SET_EMV:
            status = database_control_set_shared_emv_info(packet);
             break;
            //      case FBE_DATABASE_CONTROL_CODE_SET_MASTER_RECORD:
            //          status = database_control_set_master_record(packet);
            //          break;
        case FBE_DATABASE_CONTROL_CODE_UPDATE_PEER_CONFIG:
            status = database_control_update_peer_config(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_GET_TABLES:
            status = database_control_get_tables(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_GET_TRANSACTION_INFO:
            status = database_control_get_transaction_info(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_SYSTEM_DB_OP_CMD:
            status = database_control_system_db_op(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_GET_FRU_DESCRIPTOR:
                status = database_control_get_fru_descriptor(packet);
                break;
        case FBE_DATABASE_CONTROL_CODE_SET_FRU_DESCRIPTOR:
                status = database_control_set_fru_descriptor(packet);
                break;
                
        case FBE_DATABASE_CONTROL_CODE_GET_DISK_SIGNATURE:
                status = database_control_get_disk_signature(packet);
                break;
                
        case FBE_DATABASE_CONTROL_CODE_SET_DISK_SIGNATURE:
                status = database_control_set_disk_signature(packet);
                break;

        case FBE_DATABASE_CONTROL_CODE_CLEAR_DISK_SIGNATURE:
                status = database_control_clear_disk_signature(packet);
                break;

        case FBE_DATABASE_CONTROL_SET_PVD_DESTROY_TIMETHRESHOLD:
            status = database_control_set_time_threshold(packet);
            break;

        case FBE_DATABASE_CONTROL_GET_PVD_DESTROY_TIMETHRESHOLD:
            status = database_control_get_time_threshold(packet);
            break;
            
        case FBE_DATABASE_CONTROL_CODE_LOOKUP_LUN_BY_WWID:
            status = database_control_lookup_lun_by_wwn(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_UPDATE_PEER_SYSTEM_BG_SERVICE_FLAG:
            status = database_control_update_peer_system_bg_service_flag(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_CLEAR_PENDING_TRANSACTION:
            status = database_control_clear_pending_transaction(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_GET_COMPAT_MODE:
            status = database_control_get_compat_mode(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_COMMIT:
            status = database_control_ndu_commit(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_SET_NDU_STATE:
            /* TBD*/
            status = FBE_STATUS_OK;
            fbe_database_complete_packet(packet, status);
            break;

        case FBE_DATABASE_CONTROL_CODE_GET_ALL_LUNS:
            status = database_control_get_all_luns(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_GET_ALL_RAID_GROUPS:
            status = database_control_get_all_raid_groups(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_COMMIT_DATABASE_TABLES:
            status = database_control_commit_database_tables(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_VERSIONING:            
            status = database_control_versioning_operation(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_GET_DISK_DDMI_DATA:
            status = database_control_get_disk_ddmi_data(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_GET_ALL_PVDS:
            status = database_control_get_all_pvds(packet);
            break;  
        case FBE_DATABASE_CONTROL_CODE_GET_RAID_GROUP:
            status = database_control_get_raid_group(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_GET_PVD:
            status = database_control_get_pvd(packet);
            break; 
        case FBE_DATABASE_CONTROL_CODE_STOP_ALL_BACKGROUND_SERVICES:
            status = database_stop_all_background_services(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_RESTART_ALL_BACKGROUND_SERVICES:
            status = database_restart_all_background_services(packet);
            break;

        case FBE_DATABASE_CONTROL_ENABLE_LOAD_BALANCE:
            status = database_control_set_load_balance(packet, FBE_TRUE);
            break;
        case FBE_DATABASE_CONTROL_DISABLE_LOAD_BALANCE:
            status = database_control_set_load_balance(packet, FBE_FALSE);
            break;

        case FBE_DATABASE_CONTROL_MAKE_PLANNED_DRIVE_ONLINE:
            status = database_control_make_planned_drive_online(packet);
            break;

            
        case FBE_DATABASE_CONTROL_CODE_GET_SYSTEM_DB_HEADER:
            status = database_control_get_system_db_header(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_SET_SYSTEM_DB_HEADER:
            status = database_control_set_system_db_header(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_INIT_SYSTEM_DB_HEADER:
            status = database_control_init_system_db_header(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_CLEANUP_RE_INIT_PERSIST_SERVICE:
            status = database_control_cleanup_reinit_persist_service(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_PERSIST_SYSTEM_DB_HEADER:
            status = database_control_persist_system_db_header(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_GET_TABLES_IN_RANGE_WITH_TYPE:
            status = database_control_get_tables_in_range_with_type(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_PERSIST_SYSTEM_DB:
            status = database_control_persist_system_db(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_GET_MAX_CONFIGURABLE_OBJECTS:
            status = database_control_get_max_configurable_objects(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_RESTORE_USER_CONFIGURATION:
            status = database_control_restore_user_configuration(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_CONNECT_DRIVES:
            status = database_control_connect_pvds(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_GET_SYSTEM_OBJECT_RECREATE_FLAGS:
            status = database_control_get_system_object_recreate_flags(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_PERSIST_SYSTEM_OBJECT_OP_FLAGS:
            status = database_control_persist_system_object_recreate_flags(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_SET_INVALIDATE_CONFIG_FLAG:
            status = database_control_set_invalidate_config_flag(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_GENERATE_SYSTEM_OBJECT_CONFIG_AND_PERSIST:
            status = database_control_generate_system_object_config_and_persist(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_GENERATE_ALL_SYSTEM_OBJECTS_CONFIG_AND_PERSIST:
            status = database_control_generate_all_system_rg_and_lun_config_and_persist(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_GENERATE_CONFIGURATION_FOR_SYSTEM_PARITY_RG_AND_LUN:
            status = database_control_generate_configuration_for_system_parity_RG_and_LUN(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_HOOK:
            status = database_perform_hook(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_MARK_LU_OPERATION_FROM_CLI:
            status = database_control_lu_operation_from_cli(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_GET_LU_RAID_INFO:
            status = fbe_database_get_lun_raid_info(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_SETUP_ENCRYPTION_KEY:
            status = fbe_database_control_setup_encryption_key(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_SETUP_ENCRYPTION_REKEY:
            status = fbe_database_control_setup_encryption_rekey(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_SETUP_KEK:
            status = fbe_database_control_setup_kek(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_DESTROY_KEK:
            status = fbe_database_control_destroy_kek(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_SETUP_KEK_KEK:
            status = fbe_database_control_setup_kek_kek(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_DESTROY_KEK_KEK:
            status = fbe_database_control_destroy_kek_kek(packet);
            break;

         case FBE_DATABASE_CONTROL_CODE_REESTABLISH_KEK_KEK:
            status = fbe_database_control_reestablish_kek_kek(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_SET_PORT_ENCRYPTION_MODE:
            status = fbe_database_control_set_port_encryption_mode(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_GET_PORT_ENCRYPTION_MODE:
            status = fbe_database_control_get_port_encryption_mode(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_GET_PEER_STATE:
            status = database_control_get_peer_state(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_GET_TOTAL_LOCKED_OBJECTS_OF_CLASS:
            status = fbe_database_control_get_total_locked_objects_of_class(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_GET_LOCKED_OBJECT_INFO_OF_CLASS:
            status = fbe_database_control_get_locked_object_info_of_class(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_REMOVE_ENCRYPTION_KEYS:
            status = fbe_database_control_remove_encryption_keys(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_GET_SYSTEM_ENCRYPTION_INFO:
            status = fbe_database_control_get_system_encryption_info(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_GET_SYSTEM_ENCRYPTION_PROGRESS:
            status = fbe_database_control_get_system_encryption_progress(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_UPDATE_DRIVE_KEYS:
            status = fbe_database_control_update_drive_keys(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_SCRUB_OLD_USER_DATA:
            status = fbe_database_control_scrub_old_user_data(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_GET_BACKUP_INFO:
            status = database_control_get_backup_info(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_SET_BACKUP_INFO:
            status = database_control_set_backup_info(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_GET_BE_PORT_INFO:
            status = fbe_database_control_get_be_port_info(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_SET_UPDATE_DB_ON_PEER:
            status = database_control_set_update_peer_db(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_GET_DRIVE_SN_FOR_RAID:
            status = fbe_database_control_get_drive_sn_for_raid(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_GET_SYSTEM_SCRUB_PROGRESS:
            status = fbe_database_control_get_system_scrub_progress(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_SET_POLL_INTERVAL:
            status = fbe_database_control_set_poll_interval(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_SET_EXTENDED_CACHE_ENABLED:
            status = fbe_database_control_set_extended_cache_enabled(packet);
            break;

        case FBE_DATABASE_CONTROL_SET_GARBAGE_COLLECTION_DEBOUNCER:
            status = fbe_database_control_set_garbage_collection_debouncer(packet);
            break;

        case FBE_DATABASE_CONTROL_GET_PVD_LIST_FOR_EXT_POOL:
            status = fbe_database_control_get_pvd_list_for_ext_pool(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_CREATE_EXT_POOL:
            status = fbe_database_control_create_extent_pool(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_DESTROY_EXT_POOL:
            status = fbe_database_control_destroy_extent_pool(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_CREATE_EXT_POOL_LUN:
            status = fbe_database_control_create_ext_pool_lun(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_DESTROY_EXT_POOL_LUN:
            status = fbe_database_control_destroy_ext_pool_lun(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_LOOKUP_EXT_POOL_LUN_BY_NUMBER:
            status = database_control_lookup_ext_pool_lun_by_number(packet);
            break;
        case FBE_DATABASE_CONTROL_CODE_LOOKUP_EXT_POOL_BY_NUMBER:
            status = database_control_lookup_ext_pool_by_number(packet);
            break;

        case FBE_DATABASE_CONTROL_MARK_PVD_SWAP_PENDING:
            status = fbe_database_control_mark_pvd_swap_pending(packet);
            break;

        case FBE_DATABASE_CONTROL_CLEAR_PVD_SWAP_PENDING:
            status = fbe_database_control_clear_pvd_swap_pending(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_SET_DEBUG_FLAGS:
            status = fbe_database_control_set_debug_flags(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_GET_DEBUG_FLAGS:
            status = fbe_database_control_get_debug_flags(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_IS_VALIDATION_ALLOWED:
            status = fbe_database_control_is_validation_allowed(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_VALIDATE_DATABASE:
            status = fbe_database_control_validate_database(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_ENTER_DEGRADED_MODE:
            status = fbe_database_control_enter_degraded_mode(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_GET_PEER_SEP_VERSION:
            status = fbe_database_control_get_peer_sep_version(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_SET_PEER_SEP_VERSION:
            status = fbe_database_control_set_peer_sep_version(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_UPDATE_MIRROR_PVD_MAP:
            status = fbe_database_control_update_mirror_pvd_map(packet);
            break;

        case FBE_DATABASE_CONTROL_CODE_GET_MIRROR_PVD_MAP:
            status = fbe_database_control_get_mirror_pvd_map(packet);
            break;
            
        case FBE_DATABASE_CONTROL_GET_ADDL_SUPPORTED_DRIVE_TYPES:
            status = fbe_database_control_get_supported_drive_types(packet);
            break;
    
        case FBE_DATABASE_CONTROL_SET_ADDL_SUPPORTED_DRIVE_TYPES:
            status = fbe_database_control_set_supported_drive_types(packet);
            break;

        default:
            status =  fbe_base_service_control_entry((fbe_base_service_t *) &fbe_database_service, packet);
            break;
    }
    return status;
}

void database_trace_at_startup(fbe_trace_level_t trace_level,
                               fbe_trace_message_id_t message_id,
                               const char * fmt, ...)
{
    va_list args;

    if (trace_level <= fbe_base_service_get_trace_level((fbe_base_service_t *)&fbe_database_service)) {
        va_start(args, fmt);
        fbe_trace_at_startup(FBE_COMPONENT_TYPE_SERVICE,
            FBE_SERVICE_ID_DATABASE,
            trace_level,
            message_id,
            fmt,
            args);
        va_end(args);
    }
}

void database_trace(fbe_trace_level_t trace_level,
                    fbe_trace_message_id_t message_id,
                    const char * fmt, ...)
{
    va_list args;
    fbe_trace_level_t default_trace_level;
    fbe_trace_level_t service_trace_level;

    service_trace_level = default_trace_level = fbe_trace_get_default_trace_level();
    if (fbe_base_service_is_initialized(&fbe_database_service.base_service)) {
        service_trace_level = fbe_base_service_get_trace_level(&fbe_database_service.base_service);
        if (default_trace_level > service_trace_level) {
            service_trace_level = default_trace_level;
        }
    }
    if (trace_level > service_trace_level) {
        return;
    }

    // drop the noise level down during destroy
    if (((fbe_database_service.state == FBE_DATABASE_STATE_DESTROYING)||
         (fbe_database_service.state == FBE_DATABASE_STATE_DESTROYING_SYSTEM))&&
        (trace_level == FBE_TRACE_LEVEL_ERROR)) {
        trace_level = FBE_TRACE_LEVEL_WARNING;
    }

    va_start(args, fmt);
    fbe_trace_report(FBE_COMPONENT_TYPE_SERVICE,
        FBE_SERVICE_ID_DATABASE,
        trace_level,
        message_id,
        fmt,
        args);
    va_end(args);
}

void set_database_service_state(fbe_database_service_t *database_service_ptr, fbe_database_state_t state)
{
    database_trace(FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                   "%s: state: 0x%x, prev cmi state: 0x%x\n",
                   __FUNCTION__, state,
                   database_service_ptr->prev_cmi_state);
    /* When set db state as ready, we need to mark if the current SP is passive side */
    if (state == FBE_DATABASE_STATE_READY) {
        fbe_bool_t is_active = database_common_cmi_is_active();

        if (!is_active || database_service_ptr->prev_cmi_state == FBE_DATABASE_CMI_STATE_PASSIVE) {
            database_service_ptr->db_become_ready_in_passiveSP = FBE_TRUE;
        } else {
            database_service_ptr->db_become_ready_in_passiveSP = FBE_FALSE;
        }
        if (is_active) {
            database_service_ptr->prev_cmi_state = FBE_DATABASE_CMI_STATE_ACTIVE;
        } else {
            database_service_ptr->prev_cmi_state = FBE_DATABASE_CMI_STATE_PASSIVE;
        }
    }
    database_service_ptr->state = state;

    if (database_service_ptr->state == FBE_DATABASE_STATE_READY) {
        fbe_cmi_mark_ready(FBE_CMI_CLIENT_ID_DATABASE);
    }

}

void set_database_service_peer_state(fbe_database_service_t *database_service_ptr, fbe_database_state_t peer_state)
{
    database_service_ptr->peer_state = peer_state;
    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                   "%s: peer state: 0x%x\n", 
                   __FUNCTION__, peer_state);
}

void get_database_service_peer_state(fbe_database_service_t *database_service_ptr, fbe_database_state_t *peer_state)
{
    *peer_state = database_service_ptr->peer_state;
}

fbe_bool_t is_peer_config_synch_allowed(fbe_database_service_t *database_service_ptr)
{
    fbe_database_state_t state, peer_state;

    get_database_service_state(database_service_ptr, &state);
    get_database_service_peer_state(database_service_ptr, &peer_state);
    if (peer_state == FBE_DATABASE_STATE_WAITING_FOR_CONFIG) {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}
fbe_bool_t is_peer_update_allowed(fbe_database_service_t *database_service_ptr)
{
    fbe_database_state_t peer_state;

    get_database_service_peer_state(database_service_ptr, &peer_state);
    if ((peer_state == FBE_DATABASE_STATE_READY)
        && database_common_cmi_is_active() && database_common_peer_is_alive()) {
        return FBE_TRUE;
    }
    return FBE_FALSE;
}


/*
 * In the code, the term "service mode" is used. 
 * But user only can see the term "degraded mode" in all the logs.
 */
void fbe_database_enter_service_mode(fbe_database_service_mode_reason_t reason)
{
    fbe_status_t	status = FBE_STATUS_OK;

    /*
     * first abort the potential on going transaction 
     */
    fbe_database_transaction_abort(&fbe_database_service);
    fbe_database_service.service_mode_reason = reason;

    /*Set the degraded mode reason for SPID */
    fbe_database_set_degraded_mode_reason_in_SPID(reason);
    
    /* set current SP to service state */
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,"%s: entry\n", __FUNCTION__);
    fbe_cmi_set_current_sp_state(FBE_CMI_STATE_SERVICE_MODE);

    set_database_service_state(&fbe_database_service, FBE_DATABASE_STATE_SERVICE_MODE);
    /*set degraded mode in NvRam*/
        status = sep_set_degraded_mode_in_nvram();
        if(FBE_STATUS_OK != status)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: fail set degraded mode in nvram. state: 0x%x\n", 
                           __FUNCTION__, status);
        }
 /* send message to peer even if peer has been in service mode */
    if (!fbe_database_mini_mode())
    {
        /* Send message to peer by database cmi message */
        fbe_database_cmi_send_db_service_mode_to_peer();
    }
}

/*
 * A public function for SEP to get the database service state
 */ 
void fbe_database_get_state(fbe_database_state_t *state)
{
    if (fbe_base_service_is_initialized(&fbe_database_service.base_service))
        get_database_service_state(&fbe_database_service, state);
    else
        *state = FBE_DATABASE_STATE_INVALID;
}

static fbe_status_t database_init(fbe_packet_t * packet)
{
    fbe_status_t 	status = FBE_STATUS_OK;
    EMCPAL_STATUS		thread_start_status;
    
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,"%s: entry\n", __FUNCTION__);

    /* In function fbe_database_get_rgs_on_top_of_pvd, we allocate buffers for upstream_object_list.
       If this breaks, need to make change in fbe_database_get_rgs_on_top_of_pvd. */
    FBE_ASSERT_AT_COMPILE_TIME(FBE_MEMORY_CHUNK_SIZE >= sizeof(fbe_base_config_upstream_object_list_t));

    /* init base service */
    fbe_base_service_init((fbe_base_service_t *)&fbe_database_service);
    fbe_base_service_set_service_id((fbe_base_service_t *)&fbe_database_service, FBE_SERVICE_ID_DATABASE);
    fbe_base_service_set_trace_level((fbe_base_service_t *)&fbe_database_service, fbe_trace_get_default_trace_level());
    database_hook_init();
    fbe_database_poll_initialize_poll_ring_buffer(&fbe_database_service.poll_ring_buffer);

    /* init service specific stuff */
    /* set the state to initing*/
    set_database_service_state(&fbe_database_service,FBE_DATABASE_STATE_INITIALIZING);
    set_database_service_peer_state(&fbe_database_service, FBE_DATABASE_STATE_INVALID);
    fbe_database_service.peer_config_update_job_packet = NULL;
    fbe_database_service.db_become_ready_in_passiveSP = FBE_FALSE;

    fbe_database_service.encryption_backup_state = FBE_ENCRYPTION_BACKUP_INVALID;
    fbe_database_service.use_transient_commit_level = FBE_FALSE;
    fbe_database_service.transient_commit_level = 0;
    fbe_database_service.db_debug_flags = fbe_database_default_debug_flags;
    fbe_database_service.db_debug_flags |= FBE_DATABASE_IF_DEBUG_DEFAULT_FLAGS;    

    fbe_database_service.forced_garbage_collection_debounce_timeout = FBE_DATABASE_FORCED_GARBAGE_COLLECTION_DEBOUNCE_TIME_DEFAULT_MS;
    fbe_database_initialize_supported_drive_types(&fbe_database_service.supported_drive_types);

    /*init lu/rg generation related parameters*/
    status = fbe_database_init_lu_and_rg_number_generation(&fbe_database_service);
    if (status != FBE_STATUS_OK) {
        return status;/*the function itself will trace why we failed*/
    }

    /*and let the thread do the rest*/
    thread_start_status = fbe_thread_init (&fbe_database_service.config_thread_handle, "fbe_db_cfg", fbe_database_config_thread, (void *)&fbe_database_service);
    if (EMCPAL_STATUS_SUCCESS != thread_start_status) {
         database_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: failed to start config thread\n", 
                        __FUNCTION__);

        set_database_service_state(&fbe_database_service, FBE_DATABASE_STATE_FAILED);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    /* service init completed */
    database_trace(FBE_TRACE_LEVEL_INFO, 
                  FBE_TRACE_MESSAGE_ID_FUNCTION_EXIT,
                  "%s: init finished(thread do the rest)...\n", 
                   __FUNCTION__);

    /* complete the packet */
    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}


static fbe_status_t database_destroy(fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_config_control_system_bg_service_t system_bg_service;

    database_trace(FBE_TRACE_LEVEL_INFO, 
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "%s: entry\n", 
        __FUNCTION__);

    /* stop all background services */
    system_bg_service.enable = FBE_FALSE;
    system_bg_service.bgs_flag = FBE_BASE_CONFIG_CONTROL_SYSTEM_BG_SERVICE_ENABLE_ALL;
    system_bg_service.issued_by_ndu = FBE_FALSE;
    system_bg_service.issued_by_scheduler = FBE_FALSE;

    status = fbe_base_config_control_system_bg_service(&system_bg_service);
    if (status != FBE_STATUS_OK){
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_INFO, 
                "%s: failed to stop background services\n", __FUNCTION__);
    }

    /* destroy cmi, make sure no one bothers us while we destroy */
    fbe_database_cmi_destroy();

    fbe_db_thread_stop(&fbe_database_service);
    fbe_database_drive_connection_destroy(&fbe_database_service);

    /* destroy service specific stuff */
    database_control_destroy_all_objects();

    /*destroy the persistence service related things*/
    status = fbe_database_system_db_persist_world_teardown();
    if (status != FBE_STATUS_OK){
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO, 
                "%s: failed to teardown system db journal world\n", __FUNCTION__);
    }
/*
    fbe_db_thread_stop(&fbe_database_service);
    fbe_database_drive_connection_destroy(&fbe_database_service);
*/
    /* unregister system drive copy back notification. */
    /*status = fbe_database_unregister_system_drive_copy_back_notification();
    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_INFO, 
                        "%s: failed to unregister system drive copy back notification\n", __FUNCTION__);
        return status;
    }*/

    /* close persist service */
    /* Transaction cleanup */
    fbe_database_free_transaction(&fbe_database_service);

    /* Free the in-memory config tables */
    fbe_database_config_tables_destroy(&fbe_database_service);

    /* destroy the spinlock for full poll statistics */
    fbe_spinlock_destroy(&fbe_database_service.poll_ring_buffer.lock);

    fbe_spinlock_destroy(&fbe_database_service.system_db_header_lock);

    fbe_database_system_db_destroy();

    fbe_database_destroy_lu_and_rg_number_generation(&fbe_database_service);

    fbe_database_homewrecker_db_destroy();

    /* destroy base service */
    fbe_base_service_destroy((fbe_base_service_t *) &fbe_database_service);

    fbe_database_encryption_destroy();
    database_trace(FBE_TRACE_LEVEL_INFO, 
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "%s: end\n", 
        __FUNCTION__);

    fbe_transport_set_status(packet, status, 0);
    fbe_transport_complete_packet(packet);
    return status;
}


fbe_bool_t fbe_database_is_pvd_exists(fbe_u8_t *SN)
{
    database_object_entry_t     *object_table_entry = NULL;
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t                  found = FALSE;

    fbe_spinlock_lock(&fbe_database_service.object_table.table_lock);
    status = fbe_database_config_table_get_object_entry_by_pvd_SN(fbe_database_service.object_table.table_content.object_entry_ptr, 
                                                                  fbe_database_service.object_table.table_size,
                                                                  SN,
                                                                  &object_table_entry);
    // if we found an entry
    if ((status == FBE_STATUS_OK) &&
        (object_table_entry != NULL))
    {
        found = TRUE;
    }
    fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);
    return found;
}

fbe_bool_t fbe_database_is_user_pvd_exists(fbe_u8_t *SN, 
                                                                                                                                           fbe_object_id_t *p_object_id)
{
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t                  found = FBE_FALSE;
    database_object_entry_t * object_entry = NULL;

    if (p_object_id == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Invalid param, NULL pointer.\n",
                __FUNCTION__);
        return FBE_FALSE;
    }

    fbe_spinlock_lock(&fbe_database_service.object_table.table_lock);
    status = fbe_database_config_table_get_last_object_entry_by_pvd_SN(fbe_database_service.object_table.table_content.object_entry_ptr, 
                                                                  fbe_database_service.object_table.table_size,
                                                                  SN,
                                                                  &object_entry);
    // if we found an entry
    if ((status == FBE_STATUS_OK) &&
         (object_entry != NULL) &&
         !(fbe_database_is_object_system_pvd(object_entry->header.object_id)))
    {
        found = FBE_TRUE;
        *p_object_id = object_entry->header.object_id;
    }
    fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);
    return found;
}

/* control functions */
static fbe_status_t database_control_transaction_start(fbe_packet_t * packet)
{
    fbe_database_transaction_info_t   *transaction_info = NULL;
    fbe_status_t                      status;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&transaction_info, sizeof(*transaction_info));
    if ( status == FBE_STATUS_OK )
    {
        // if database has corrupt object, it's not allowed to make configuration change
        // except for the recovery job.
        if((fbe_database_service.state == FBE_DATABASE_STATE_CORRUPT) && (transaction_info->transaction_type == FBE_DATABASE_TRANSACTION_CREATE))
        {
            status = FBE_STATUS_GENERIC_FAILURE;
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: Database state is corrupt, it's not allowed to make configuration change except for hot spare.\n", __FUNCTION__);
            fbe_database_complete_packet(packet, status);
            return status;
        }
        else if(fbe_database_service.state != FBE_DATABASE_STATE_READY)
        {
            /*Zhipeng Hu: This would never happen, because:
             *(1) On system booting case, database service's initial state is INVALID or INITIALIZING, so
             *     job service never gets chance to run a job.
             *
             *(2) On active SP dead case, our database_cmi_disable_service_when_lost_peer logic gurantees
             *     database service's state transits out of READY before the job execute thread is waken up. So
             *     in this case job service also never gets chance to run a job.
             *
             *Note: In job service execute thread, it will always check database's state. If not READY, it will
             *wait and check again after 1s.
             *
             *Though this will never happen, we still check it and output error to trace potential risk in future.*/
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: Database is not READY 0x%x, not allow to make cfg change.\n",
                           __FUNCTION__,
                           fbe_database_service.state);
            fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        else
        {
            status = fbe_database_transaction_start(&fbe_database_service, transaction_info);
            if (status == FBE_STATUS_OK) {
                fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &transaction_info->transaction_id);
            }
            else {
                transaction_info->transaction_id = FBE_DATABASE_TRANSACTION_ID_INVALID;
            }
        }               
    }
    
    database_trace(FBE_TRACE_LEVEL_INFO, 
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Start transaction: 0x%x\n", __FUNCTION__, (unsigned int)transaction_info->transaction_id);
    
    fbe_database_complete_packet(packet, status);
    return status;
} 

static fbe_status_t database_control_transaction_commit(fbe_packet_t * packet)
{
    fbe_database_transaction_id_t     *transaction_id = NULL;
    fbe_database_transaction_id_t      current_id = FBE_DATABASE_TRANSACTION_ID_INVALID;

    fbe_status_t                     status;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&transaction_id, sizeof(*transaction_id));
    if ( status == FBE_STATUS_OK )
    {
        /* check if the transaction id is valid */
        fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &current_id);
        if (*transaction_id != current_id){
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Invalid id 0x%llx.  Current id is 0x%llx\n",
                __FUNCTION__, (unsigned long long)(*transaction_id),
        (unsigned long long)current_id);
            fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE );
            return FBE_STATUS_GENERIC_FAILURE;
        }
        database_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Committing transaction 0x%x with job no 0x%x.\n",
            __FUNCTION__, (unsigned int)*transaction_id, (unsigned int)fbe_database_service.transaction_ptr->job_number);
        status = fbe_database_transaction_commit(&fbe_database_service);
    }
    
    database_trace(FBE_TRACE_LEVEL_INFO, 
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Commit transaction 0x%x returned 0x%x.\n",
        __FUNCTION__, (unsigned int)*transaction_id, status);
    
    fbe_database_complete_packet(packet, status);
    return status;
}

static fbe_status_t database_control_transaction_abort(fbe_packet_t * packet)
{
    fbe_database_transaction_id_t     *transaction_id = NULL;
    fbe_database_transaction_id_t     current_id = FBE_DATABASE_TRANSACTION_ID_INVALID;
    fbe_status_t                      status;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&transaction_id, sizeof(*transaction_id));
    if ( status == FBE_STATUS_OK )
    {
        fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &current_id);
        /* check if the transaction id is valid */
        if (*transaction_id != current_id){
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Invalid id 0x%llx.  Current id is 0x%llx\n",
                __FUNCTION__, (unsigned long long)(*transaction_id),
        (unsigned long long)current_id);
            fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE );
            return FBE_STATUS_GENERIC_FAILURE;
        }
        database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Aborting transaction 0x%llx.\n",
            __FUNCTION__, (unsigned long long)(*transaction_id));
        status = fbe_database_transaction_abort(&fbe_database_service);
    }
    database_trace(FBE_TRACE_LEVEL_INFO, 
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Abort transaction 0x%x returned 0x%x.\n",
        __FUNCTION__, (unsigned int)*transaction_id, status);
    fbe_database_complete_packet(packet, status);
    return status;
}
/*!***************************************************************
* @fn database_control_create_pvd
*****************************************************************
* @brief
* create pvd object
*
* @param packet -
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/

static fbe_status_t database_control_create_pvd(fbe_packet_t * packet)
{
    fbe_database_transaction_id_t    	current_id = FBE_DATABASE_TRANSACTION_ID_INVALID;
    fbe_status_t                     	status;
    fbe_database_control_pvd_t * 		pvd = NULL;
    

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&pvd, sizeof(*pvd));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &current_id);
    /* check if the transaction id is valid */
    if (pvd->transaction_id != current_id){
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Invalid id 0x%llx.  Current id is 0x%llx\n",
            __FUNCTION__, (unsigned long long)pvd->transaction_id,
        (unsigned long long)current_id);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE );
        return FBE_STATUS_GENERIC_FAILURE;
    }

    database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Creating PVD - transaction 0x%llx.\n",
        __FUNCTION__, (unsigned long long)pvd->transaction_id);		

    status = database_create_pvd(pvd);
    if (status == FBE_STATUS_OK && is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)pvd,
                                                             FBE_DATABE_CMI_MSG_TYPE_CREATE_PVD);

    }

    fbe_database_complete_packet(packet, status);
    return status;
}


/*!***************************************************************
* @fn database_control_create_vd
*****************************************************************
* @brief
* create vd object
*
* @param packet -
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/

static fbe_status_t database_control_create_vd(fbe_packet_t * packet)
{
    fbe_database_control_vd_t        *vd = NULL;
    fbe_database_transaction_id_t    current_id = FBE_DATABASE_TRANSACTION_ID_INVALID;
    fbe_status_t                     status;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&vd, sizeof(*vd));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &current_id);
    /* check if the transaction id is valid */
    if (vd->transaction_id != current_id){
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Invalid id 0x%llx.  Current id is 0x%llx\n",
            __FUNCTION__, (unsigned long long)vd->transaction_id,
        (unsigned long long)current_id);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE );
        return FBE_STATUS_GENERIC_FAILURE;
    }
    database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Creating VD - transaction 0x%llx.\n",
        __FUNCTION__, (unsigned long long)vd->transaction_id);	
    
    status = database_create_vd(vd);
    if (status == FBE_STATUS_OK && is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)vd,
                                                             FBE_DATABE_CMI_MSG_TYPE_CREATE_VD);

    }

    fbe_database_complete_packet(packet, status);
    return status;
}

/*!***************************************************************
* @fn database_control_create_raid
*****************************************************************
* @brief
* create RG object
*
* @param packet -
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
static fbe_status_t database_control_create_raid(fbe_packet_t * packet)
{
    fbe_database_control_raid_t      *create_raid = NULL;
    fbe_database_transaction_id_t     current_id = FBE_DATABASE_TRANSACTION_ID_INVALID;
    fbe_status_t                      status;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&create_raid, sizeof(*create_raid));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &current_id);
    /* check if the transaction id is valid */
    if (create_raid->transaction_id != current_id){
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Invalid id 0x%llx.  Current id is 0x%llx\n",
            __FUNCTION__, (unsigned long long)create_raid->transaction_id,
        (unsigned long long)current_id);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE );
        return FBE_STATUS_GENERIC_FAILURE;
    }
    database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Creating RG - transaction 0x%llx.\n",
        __FUNCTION__, (unsigned long long)create_raid->transaction_id);	

    status = database_create_raid(create_raid);
    if (status == FBE_STATUS_OK && is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)create_raid,
                                                             FBE_DATABE_CMI_MSG_TYPE_CREATE_RAID);

    }

    fbe_database_complete_packet(packet, status);
    return status;
}

/*!***************************************************************
* @fn database_control_create_lun
*****************************************************************
* @brief
* create lun object
*
* @param packet -
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/

static fbe_status_t database_control_create_lun(fbe_packet_t * packet)
{
    fbe_database_control_lun_t       *create_lun = NULL;
    fbe_database_transaction_id_t     current_id = FBE_DATABASE_TRANSACTION_ID_INVALID;
    fbe_status_t                      status;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&create_lun, sizeof(*create_lun));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &current_id);
    /* check if the transaction id is valid */	

    if (create_lun->transaction_id != current_id){
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Invalid id 0x%llx.  Current id is 0x%llx\n",
            __FUNCTION__, (unsigned long long)create_lun->transaction_id,
       (unsigned long long)current_id);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE );
        return FBE_STATUS_GENERIC_FAILURE;
    }

    database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: Creating LUN - transaction 0x%llx.\n",
                   __FUNCTION__, (unsigned long long)create_lun->transaction_id);


    status = database_create_lun(create_lun);
    if (status == FBE_STATUS_OK && is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)create_lun,
                                                             FBE_DATABE_CMI_MSG_TYPE_CREATE_LUN);

    }


    fbe_database_complete_packet(packet, status);
    return status;
}

/*!***************************************************************
* @fn database_control_clone_object
*****************************************************************
* @brief
* clone object
*
* @param packet -
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
static fbe_status_t database_control_clone_object(fbe_packet_t * packet)
{
    fbe_database_control_clone_object_t     *clone_object = NULL;
    fbe_database_transaction_id_t            current_id = FBE_DATABASE_TRANSACTION_ID_INVALID;
    fbe_status_t                            status;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&clone_object, sizeof(*clone_object));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &current_id);
    /* check if the transaction id is valid */
    if (clone_object->transaction_id != current_id){
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Invalid id 0x%llx.  Current id is 0x%llx\n",
                       __FUNCTION__,
               (unsigned long long)clone_object->transaction_id,
               current_id);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE );
        return FBE_STATUS_GENERIC_FAILURE;
    }
    database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: Cloning object - transaction 0x%llx.\n",
                   __FUNCTION__,
           (unsigned long long)clone_object->transaction_id);

    status = database_clone_object(clone_object);
    if (status == FBE_STATUS_OK && is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)clone_object,
                                                             FBE_DATABE_CMI_MSG_TYPE_CLONE_OBJECT);
    }

    fbe_database_complete_packet(packet, status);
    return status;
}

/*!***************************************************************
* @fn database_persist_prom_wwnseed
*****************************************************************
* @brief
* persist prom wwn seed
*
* @param packet -
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
static fbe_status_t database_persist_prom_wwnseed(fbe_packet_t * packet)
{
    fbe_u32_t                           *wwn_seed = NULL;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;


    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&wwn_seed, sizeof(fbe_u32_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    status = fbe_database_set_board_resume_prom_wwnseed_by_pp(*wwn_seed);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: set resume prom fail, wwn_seed:%d\n",
                       __FUNCTION__, *wwn_seed);
        fbe_database_complete_packet(packet, status);
        return status;
    }

    fbe_database_complete_packet(packet, status);
    return status;
}

/*!***************************************************************
* @fn database_obtain_prom_wwnseed
*****************************************************************
* @brief
* get prom wwnseed info
*
* @param packet -
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
static fbe_status_t database_obtain_prom_wwnseed(fbe_packet_t * packet)
{
    fbe_u32_t                           *wwn_seed = NULL;
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;


    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&wwn_seed, sizeof(fbe_u32_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    status = fbe_database_get_board_resume_prom_wwnseed_by_pp(wwn_seed);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: read resume prom wwn seed fail, wwn_seed:%d\n",
                       __FUNCTION__, *wwn_seed);
        fbe_database_complete_packet(packet, status);
        return status;
    }

    fbe_database_complete_packet(packet, status);
    return status;
}


/*!***************************************************************
* @fn database_control_create_edge
*****************************************************************
* @brief
* create edge
*
* @param packet -
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
static fbe_status_t database_control_create_edge(fbe_packet_t * packet)
{
    fbe_database_control_create_edge_t      *create_edge = NULL;        
    fbe_database_transaction_id_t            current_id = FBE_DATABASE_TRANSACTION_ID_INVALID;
    fbe_status_t                             status;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&create_edge, sizeof(*create_edge));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &current_id);
    /* check if the transaction id is valid */
    if (create_edge->transaction_id != current_id){
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Invalid id 0x%llx.  Current id is 0x%llx\n",
            __FUNCTION__, (unsigned long long)create_edge->transaction_id,
        current_id);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE );
        return FBE_STATUS_GENERIC_FAILURE;
    }
    database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Creating edge - transaction 0x%llx.\n",
        __FUNCTION__, (unsigned long long)create_edge->transaction_id);

    status = database_create_edge(create_edge);
    if (status == FBE_STATUS_OK && is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)create_edge,
                                                             FBE_DATABE_CMI_MSG_TYPE_CREATE_EDGE);

    }

    fbe_database_complete_packet(packet, status);
    return status;
}

/*DESTROY FUNCTIONS*/

/*!***************************************************************
* @fn database_control_destroy_pvd
*****************************************************************
* @brief
* destroy pvd object
*
* @param packet -
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
static fbe_status_t database_control_destroy_pvd(fbe_packet_t * packet)
{
    fbe_database_control_destroy_object_t     *pvd = NULL;
    fbe_database_transaction_id_t              current_id = FBE_DATABASE_TRANSACTION_ID_INVALID;
    fbe_status_t                               status;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&pvd, sizeof(*pvd));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &current_id);
    /* check if the transaction id is valid */
    if (pvd->transaction_id != current_id){
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Invalid id 0x%llx.  Current id is 0x%llx\n",
            __FUNCTION__, (unsigned long long)pvd->transaction_id,
        current_id);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE );
        return FBE_STATUS_GENERIC_FAILURE;
    }
    database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Destroying pvd - transaction 0x%x.\n",
        __FUNCTION__, (unsigned int)pvd->transaction_id);
    
    /* If this is a forced garbage collection then find unconsumed/failed drives.  System drives not allowed. */
    if (pvd->object_id == FBE_DATABASE_TRANSACTION_DESTROY_UNCONSUMED_PVDS_ID){
        database_object_entry_t * current_entry_ptr;
        database_table_header_t * header_ptr;
        fbe_u32_t destroy_count = 0;
        fbe_u32_t index;
        fbe_base_object_mgmt_get_lifecycle_state_t get_lifecycle;

        /* Gathering list of PVDs on active side.   Then sending destroy to both SPs.  This will
           guarantee that same objects are destroyed on both sides. */

        /* NOTE:  No lock is taken for table access since read only.  Guaranteed to not be freed at this time. */
        current_entry_ptr = fbe_database_service.object_table.table_content.object_entry_ptr;
        for(index = 0; index < fbe_database_service.object_table.table_size; index++) {

            if(current_entry_ptr != NULL) 
            {
                header_ptr = (database_table_header_t *)current_entry_ptr;

                if((header_ptr->state == DATABASE_CONFIG_ENTRY_VALID) &&
                   (current_entry_ptr->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE &&
                    !fbe_private_space_layout_object_id_is_system_pvd(header_ptr->object_id) &&   /* system drives won't be destroyed */
                    current_entry_ptr->set_config_union.pvd_config.config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED ))
                {                
                    status  = fbe_database_send_packet_to_object(FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
                                                                 &get_lifecycle,
                                                                 sizeof(fbe_base_object_mgmt_get_lifecycle_state_t),
                                                                 header_ptr->object_id,
                                                                 NULL,
                                                                 0,
                                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                                 FBE_PACKAGE_ID_SEP_0);


                    if (status != FBE_STATUS_OK) {
                        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                       "create_pvd_destroy_request: get lifecycle failed. Obj: 0x%x, status=%d\n", 
                                       header_ptr->object_id, status);
                    }
                    else {
                        if (get_lifecycle.lifecycle_state == FBE_LIFECYCLE_STATE_FAIL)
                        {
                            /* qualifies for removal.  destroy it. */
                            pvd->object_id = header_ptr->object_id;

                            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                                           "%s: Garbage Collection for pvd:0x%x, count:%d\n", __FUNCTION__, pvd->object_id, destroy_count+1);

                            /*let's start with the peer first if it's there*/
                            if (is_peer_update_allowed(&fbe_database_service)) {
                                /*synch to the peer*/
                                status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                                                     (void *)pvd,
                                                                                     FBE_DATABE_CMI_MSG_TYPE_DESTROY_PVD);

                            }

                            /*and now kill ourselvs*/
                            if (status == FBE_STATUS_OK) {
                                    status = database_destroy_pvd(pvd, FALSE);
                            }

                           destroy_count++;
                            if (destroy_count >= FBE_DATABASE_MAX_GARBAGE_COLLECTION_PVDS)
                            {
                                /* we hit our limit. stop searching.*/
                                break;
                            }
                        }
                    }
                }            
            }
            else
            {
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
            }
            current_entry_ptr++;
        }

        if (destroy_count == 0) {
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: Garbage Collection no pvds found\n", __FUNCTION__);
        }

    }
    else  /* normal destroy case */
    {    
        /*let's start with the peer first if it's there*/
        if (is_peer_update_allowed(&fbe_database_service)) {
            /*synch to the peer*/
            status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                                 (void *)pvd,
                                                                 FBE_DATABE_CMI_MSG_TYPE_DESTROY_PVD);
    
        }
    
        /*and now kill ourselvs*/
        if (status == FBE_STATUS_OK) {
                status = database_destroy_pvd(pvd, FALSE);
        }
    }

    fbe_database_complete_packet(packet, status);
    return status;
}

/*!***************************************************************
* @fn database_control_destroy_vd
*****************************************************************
* @brief
* destroy vd object
*
* @param packet -
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
static fbe_status_t database_control_destroy_vd(fbe_packet_t * packet)
{
    fbe_database_control_destroy_object_t     *vd = NULL;
    fbe_database_transaction_id_t              current_id = FBE_DATABASE_TRANSACTION_ID_INVALID;
    fbe_status_t                               status;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&vd, sizeof(*vd));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &current_id);
    /* check if the transaction id is valid */
    if (vd->transaction_id != current_id){
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Invalid id 0x%llx.  Current id is 0x%llx\n",
            __FUNCTION__, (unsigned long long)vd->transaction_id,
        (unsigned long long)current_id);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE );
        return FBE_STATUS_GENERIC_FAILURE;
    }
    database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Destroying VD - transaction 0x%x.\n",
        __FUNCTION__, (unsigned int)vd->transaction_id);
    
    /*let's start with the peer first if it's there*/
    if (is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)vd,
                                                             FBE_DATABE_CMI_MSG_TYPE_DESTROY_VD);

    }

    if (status == FBE_STATUS_OK) {
        status = database_destroy_vd(vd, FALSE);
    }

    fbe_database_complete_packet(packet, status);
    return status;
}

/*!***************************************************************
* @fn database_control_destroy_raid
*****************************************************************
* @brief
* destroy raid grp object
*
* @param packet -
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
static fbe_status_t database_control_destroy_raid(fbe_packet_t * packet)
{
    fbe_database_control_destroy_object_t      *destroy_raid = NULL;
    fbe_database_transaction_id_t              current_id = FBE_DATABASE_TRANSACTION_ID_INVALID;
    fbe_status_t                               status;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&destroy_raid, sizeof(*destroy_raid));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &current_id);
    /* check if the transaction id is valid */
    if (destroy_raid->transaction_id != current_id){
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Invalid id 0x%llx.  Current id is 0x%llx\n",
            __FUNCTION__, (unsigned long long)destroy_raid->transaction_id,
        (unsigned long long)current_id);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE );
        return FBE_STATUS_GENERIC_FAILURE;
    }
    database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Destroying RG - transaction 0x%x.\n",
        __FUNCTION__, (unsigned int)destroy_raid->transaction_id);	

    /*let's start with the peer first if it's there*/
    if (is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)destroy_raid,
                                                             FBE_DATABE_CMI_MSG_TYPE_DESTROY_RAID);

    }
    
    if (status == FBE_STATUS_OK) {
        status = database_destroy_raid(destroy_raid, FALSE);
    }
    
    fbe_database_complete_packet(packet, status);
    return status;
}
/*!***************************************************************
* @fn database_control_destroy_lun
*****************************************************************
* @brief
* destroy lun object
*
* @param packet -
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
static fbe_status_t database_control_destroy_lun(fbe_packet_t * packet)
{
    fbe_database_control_destroy_object_t      *destroy_lun = NULL;		
    fbe_database_transaction_id_t              current_id = FBE_DATABASE_TRANSACTION_ID_INVALID;	
    fbe_status_t                               status;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&destroy_lun, sizeof(*destroy_lun));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &current_id);
    /* check if the transaction id is valid */
    if (destroy_lun->transaction_id != current_id){
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Invalid id 0x%llx.  Current id is 0x%llx\n",
            __FUNCTION__, (unsigned long long)destroy_lun->transaction_id,
        current_id);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE );
        return FBE_STATUS_GENERIC_FAILURE;
    }
    database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Destroying LUN - transaction 0x%x.\n",
        __FUNCTION__, (unsigned int)destroy_lun->transaction_id);	

    /*let's start with the peer first if it's there*/
    if (is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)destroy_lun,
                                                             FBE_DATABE_CMI_MSG_TYPE_DESTROY_LUN);

    }

    if (status == FBE_STATUS_OK) {
        status = database_destroy_lun(destroy_lun, FALSE);
    }

    fbe_database_complete_packet(packet, status);
    return status;
}

/*!***************************************************************
* @fn database_control_destroy_edge
*****************************************************************
* @brief
* destroy edge
*
* @param packet -
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/

static fbe_status_t database_control_destroy_edge(fbe_packet_t * packet)
{
    fbe_database_control_destroy_edge_t   *destroy_edge = NULL;       
    fbe_database_transaction_id_t          current_id = FBE_DATABASE_TRANSACTION_ID_INVALID;
    fbe_status_t                           status;
    

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&destroy_edge, sizeof(*destroy_edge));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &current_id);
    /* check if the transaction id is valid */
    if (destroy_edge->transaction_id != current_id){
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Invalid id 0x%llx.  Current id is 0x%llx\n",
            __FUNCTION__, (unsigned long long)destroy_edge->transaction_id,
        current_id);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE );
        return FBE_STATUS_GENERIC_FAILURE;
    }
    database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Destroying edge - transaction 0x%llx.\n",
        __FUNCTION__, (unsigned long long)destroy_edge->transaction_id);	

    status = database_destroy_edge(destroy_edge);
    if (status == FBE_STATUS_OK && is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)destroy_edge,
                                                             FBE_DATABE_CMI_MSG_TYPE_DESTROY_EDGE);

    }
    
    fbe_database_complete_packet(packet, status);
    return status;
}


/* Look-up */

static fbe_status_t database_control_lookup_raid_by_number(fbe_packet_t * packet)
{
    fbe_status_t                     status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_lookup_raid_t *lookup_raid = NULL;  
    database_user_entry_t           *out_entry_ptr = NULL;     

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&lookup_raid, sizeof(*lookup_raid));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    status = fbe_database_config_table_get_user_entry_by_rg_id(&fbe_database_service.user_table, 
                                                               lookup_raid->raid_id,
                                                               &out_entry_ptr);
    if ((status == FBE_STATUS_OK)&&(out_entry_ptr != NULL)) {
        /* get the entry ok */
        if (out_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) {
            /* only return the object when the entry is valid */
            lookup_raid->object_id = out_entry_ptr->header.object_id;
        }
    }else{
#ifndef NO_EXT_POOL_ALIAS
        status = fbe_database_config_table_get_user_entry_by_ext_pool_id(&fbe_database_service.user_table, 
                                                                   lookup_raid->raid_id,
                                                                   &out_entry_ptr);
        if ((status == FBE_STATUS_OK)&&(out_entry_ptr != NULL)) {
            /* get the entry ok */
            if (out_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) {
            /* only return the object when the entry is valid */
                lookup_raid->object_id = out_entry_ptr->header.object_id;
            }
        }
        else
#endif
        {
        lookup_raid->object_id = FBE_OBJECT_ID_INVALID;
        status = FBE_STATUS_NO_OBJECT;

        database_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to find RG %d; returned 0x%x.\n",
            __FUNCTION__, lookup_raid->raid_id, status);
        }
    }
    fbe_database_complete_packet(packet, status);
    return status;
}

/* Look-up raid by object_id */

static fbe_status_t database_control_lookup_raid_by_object_id(fbe_packet_t * packet)
{
    fbe_status_t                     status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_lookup_raid_t *lookup_raid = NULL;  
    database_user_entry_t           *out_entry_ptr = NULL;     

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&lookup_raid, sizeof(*lookup_raid));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,	
                                                                   lookup_raid->object_id,
                                                                   &out_entry_ptr);
    if ((status == FBE_STATUS_OK)&&(out_entry_ptr != NULL)) {
        /* get the entry ok */
        if (out_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) {
            /* only return the object when the entry is valid */
            lookup_raid->raid_id = out_entry_ptr->user_data_union.rg_user_data.raid_group_number;
        }
    }else{
    //	lookup_raid->raid_id = NULL;
        database_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to find RG %d; returned 0x%x.\n",
            __FUNCTION__, lookup_raid->object_id, status);
        status = FBE_STATUS_NO_OBJECT;
    }
    fbe_database_complete_packet(packet, status);
    return status;
}
/*Look up LUN by lun number*/
static fbe_status_t database_control_lookup_lun_by_number(fbe_packet_t * packet)
{
    fbe_status_t                     status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_lookup_lun_t *lookup_lun = NULL;  
    database_user_entry_t           *out_entry_ptr = NULL;     

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&lookup_lun, sizeof(*lookup_lun));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    status = fbe_database_config_table_get_user_entry_by_lun_id(&fbe_database_service.user_table, 
                                                                lookup_lun->lun_number,
                                                                &out_entry_ptr);
    if ((status == FBE_STATUS_OK)&&(out_entry_ptr != NULL)) {
        /* get the entry ok */
        if (out_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) {
            /* only return the object when the entry is valid */
            lookup_lun->object_id = out_entry_ptr->header.object_id;
        }
    }else{
        lookup_lun->object_id = FBE_OBJECT_ID_INVALID;
        /*trace to a low level since many times it is used just to look for the next free lun number and it will be confusing in traces*/
        database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to find LUN %d; returned 0x%x.\n",
            __FUNCTION__, lookup_lun->lun_number, status);
        status = FBE_STATUS_NO_OBJECT;
    }
    fbe_database_complete_packet(packet, status);
    return status;
}

/* Look-up LUN by object_id */

static fbe_status_t database_control_lookup_lun_by_object_id(fbe_packet_t * packet)
{
    fbe_status_t                     status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_lookup_lun_t *lookup_lun = NULL;  
    database_user_entry_t           *out_entry_ptr = NULL;     

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&lookup_lun, sizeof(*lookup_lun));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,	 
                                                                   lookup_lun->object_id,
                                                                   &out_entry_ptr);
    if ((status == FBE_STATUS_OK)&&(out_entry_ptr != NULL)) {
        /* get the entry ok */
        if (out_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID &&
            out_entry_ptr->db_class_id == DATABASE_CLASS_ID_LUN) {
            /* only return the object when the entry is valid */
            lookup_lun->lun_number = out_entry_ptr->user_data_union.lu_user_data.lun_number;
        } else if (out_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID &&
                   out_entry_ptr->db_class_id == DATABASE_CLASS_ID_EXTENT_POOL_LUN) {
            /* only return the object when the entry is valid */
            lookup_lun->lun_number = out_entry_ptr->user_data_union.ext_pool_lun_user_data.lun_id;
        }
    }else{
    //	lookup_lun->raid_id = NULL;
        database_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to find LUN %d; returned 0x%x.\n",
            __FUNCTION__, lookup_lun->object_id, status);
        status = FBE_STATUS_NO_OBJECT;
    }
    fbe_database_complete_packet(packet, status);
    return status;
}


/*!***************************************************************
* @fn database_control_get_raid_user_private
*****************************************************************
* @brief
* get the user private raid attribute from database
*
* @param packet -packet
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
static fbe_status_t database_control_get_raid_user_private(fbe_packet_t * packet)
{
    fbe_status_t                                status = FBE_STATUS_GENERIC_FAILURE;
    database_user_entry_t                       *entry_ptr = NULL;
    fbe_database_control_user_private_raid_t    *private_rg;

    status = fbe_database_get_payload(packet, (void **)&private_rg, sizeof(*private_rg));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table, 
                                                                   private_rg->object_id,
                                                                   &entry_ptr);
    if ((status == FBE_STATUS_OK)&&(entry_ptr != NULL)) 
            private_rg->user_private= entry_ptr->user_data_union.rg_user_data.user_private;

    fbe_database_complete_packet(packet, status);            
    return status;
}

static fbe_status_t database_control_get_lun_info(fbe_packet_t *packet)
{    
    fbe_database_lun_info_t*            lun_info = NULL;
    fbe_status_t                        status = FBE_STATUS_OK;
   
    /* Verify packet contents */    
    status = fbe_database_get_payload(packet, (void **)&lun_info, sizeof(*lun_info));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }    

    status = database_get_lun_info(lun_info, packet);

    fbe_database_complete_packet(packet, status);    
    return status;
}

static fbe_status_t database_control_update_pvd(fbe_packet_t * packet)
{    
    fbe_database_control_update_pvd_t  *update_pvd = NULL;
    fbe_status_t                       status;
    fbe_database_transaction_id_t      current_id = FBE_DATABASE_TRANSACTION_ID_INVALID;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&update_pvd, sizeof(*update_pvd));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }    

    fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &current_id);
    /* check if the transaction id is valid */
    if (update_pvd->transaction_id != current_id){
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Invalid id 0x%llx.  Current id is 0x%llx\n",
            __FUNCTION__, (unsigned long long)update_pvd->transaction_id, current_id);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE );
        return FBE_STATUS_GENERIC_FAILURE;
    }
    

    if ( status == FBE_STATUS_OK ) {
        status = database_update_pvd(update_pvd);
        if (status == FBE_STATUS_OK && is_peer_update_allowed(&fbe_database_service)) {
            /*synch to the peer*/
            status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                                 (void *)update_pvd,
                                                                 FBE_DATABE_CMI_MSG_TYPE_UPDATE_PVD);
        }
    }     

    fbe_database_complete_packet(packet, status);
    return status;
}


static fbe_status_t database_control_update_pvd_block_size(fbe_packet_t * packet)
{    
    fbe_database_control_update_pvd_block_size_t  *update_pvd_block_size = NULL;
    fbe_status_t                       status;
    fbe_database_transaction_id_t      current_id = FBE_DATABASE_TRANSACTION_ID_INVALID;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&update_pvd_block_size, sizeof(*update_pvd_block_size));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }    

    fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &current_id);
    /* check if the transaction id is valid */
    if (update_pvd_block_size->transaction_id != current_id){
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Invalid id 0x%llx.  Current id is 0x%llx\n",
            __FUNCTION__, (unsigned long long)update_pvd_block_size->transaction_id, current_id);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE );
        return FBE_STATUS_GENERIC_FAILURE;
    }
    

    if ( status == FBE_STATUS_OK ) {
        status = database_update_pvd_block_size(update_pvd_block_size);
        if (status == FBE_STATUS_OK && is_peer_update_allowed(&fbe_database_service)) {
            /*synch to the peer*/
            status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                                 (void *)update_pvd_block_size,
                                                                 FBE_DATABE_CMI_MSG_TYPE_UPDATE_PVD_BLOCK_SIZE);
        }
    }     

    fbe_database_complete_packet(packet, status);
    return status;
}

static fbe_status_t database_control_update_encryption_mode(fbe_packet_t * packet)
{    
    fbe_database_control_update_encryption_mode_t  *update_encryption_mode = NULL;
    fbe_status_t                       status;
    fbe_database_transaction_id_t      current_id = FBE_DATABASE_TRANSACTION_ID_INVALID;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&update_encryption_mode, sizeof(*update_encryption_mode));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }    
    else
    {
        fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &current_id);
    /* check if the transaction id is valid */
        if (update_encryption_mode->transaction_id != current_id){
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Invalid id 0x%llx.  Current id is 0x%llx\n",
                __FUNCTION__, (unsigned long long)update_encryption_mode->transaction_id,
        current_id);
            fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE );
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    if ( status == FBE_STATUS_OK )
    {
        status =  database_update_encryption_mode(update_encryption_mode);
        if (status == FBE_STATUS_OK && is_peer_update_allowed(&fbe_database_service)) {
            /*synch to the peer*/
            status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                                 (void *)update_encryption_mode,
                                                                 FBE_DATABASE_CMI_MSG_TYPE_UPDATE_ENCRYPTION_MODE);
        }
    }     

    fbe_database_complete_packet(packet, status);
    return status;
}

static fbe_status_t database_control_update_vd(fbe_packet_t * packet)
{
    fbe_database_control_update_vd_t		 *update_vd = NULL;
    fbe_status_t                              status;
    fbe_database_transaction_id_t      		current_id = FBE_DATABASE_TRANSACTION_ID_INVALID;

   /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&update_vd, sizeof(*update_vd));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }    
    else
    {
        fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &current_id);
    /* check if the transaction id is valid */
        if (update_vd->transaction_id != current_id){
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Invalid id 0x%llx.  Current id is 0x%llx\n",
                __FUNCTION__, (unsigned long long)update_vd->transaction_id,
        current_id);
            fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE );
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }


    if ( status == FBE_STATUS_OK )
    {
        status = database_update_vd(update_vd);
        if (status == FBE_STATUS_OK && is_peer_update_allowed(&fbe_database_service)) {
            /*synch to the peer*/
            status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                                 (void *)update_vd,
                                                                 FBE_DATABE_CMI_MSG_TYPE_UPDATE_VD);
        }
    }   

    fbe_database_complete_packet(packet, status);    
    return status;
}


static fbe_status_t database_control_update_raid(fbe_packet_t *packet)
{
    fbe_database_control_update_raid_t *	update_raid       = NULL;
    fbe_status_t                            status;
    fbe_database_transaction_id_t      		current_id = FBE_DATABASE_TRANSACTION_ID_INVALID;
    
    database_trace(FBE_TRACE_LEVEL_DEBUG_HIGH,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: entry\n", 
                   __FUNCTION__);

/* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&update_raid, sizeof(*update_raid));

    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }    
    else
    {
        fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &current_id);
    /* check if the transaction id is valid */
        if (update_raid->transaction_id != current_id){
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Invalid id 0x%llx.  Current id is 0x%llx\n",
                __FUNCTION__, (unsigned long long)update_raid->transaction_id,
            current_id);
            fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE );
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
     
    status = database_update_raid(update_raid);
    if (status == FBE_STATUS_OK && is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)update_raid,
                                                             FBE_DATABE_CMI_MSG_TYPE_UPDATE_RAID);
    }
        
    fbe_database_complete_packet(packet, status);    
    return status;
}    

static fbe_status_t database_control_update_lun(fbe_packet_t * packet)
{    
    fbe_database_control_update_lun_t *update_lun = NULL;
    fbe_status_t                       status;
    fbe_database_transaction_id_t      current_id = FBE_DATABASE_TRANSACTION_ID_INVALID;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&update_lun, sizeof(*update_lun));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }    
    else
    {
        fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &current_id);
    /* check if the transaction id is valid */
        if (update_lun->transaction_id != current_id){
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: Invalid id 0x%llx.  Current id is 0x%llx\n",
                __FUNCTION__, (unsigned long long)update_lun->transaction_id,
        current_id);
            fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE );
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }

    if ( status == FBE_STATUS_OK )
    {        
        status = database_update_lun(update_lun);
        if (status == FBE_STATUS_OK && is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)update_lun,
                                                             FBE_DATABE_CMI_MSG_TYPE_UPDATE_LUN);
    }

       
    }          
    fbe_database_complete_packet(packet, status);
    return status;
}

static fbe_status_t database_control_get_state(fbe_packet_t * packet)
{
    fbe_status_t                     status = FBE_STATUS_OK;
    fbe_database_control_get_state_t *get_state = NULL;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&get_state, sizeof(*get_state));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    get_database_service_state(&fbe_database_service, &get_state->state);

    fbe_database_complete_packet(packet, status);
    return status;
}

static fbe_status_t database_control_get_service_mode_reason(fbe_packet_t * packet)
{
    fbe_status_t                     status = FBE_STATUS_OK;
    fbe_database_control_get_service_mode_reason_t *get_reason = NULL;
    fbe_char_t *p;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&get_reason, sizeof(*get_reason));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    get_reason->reason = fbe_database_service.service_mode_reason;

    p = database_get_service_mode_reason_string(get_reason->reason);
    strncpy(get_reason->reason_str, p, MAX_DB_SERVICE_MODE_REASON_STR - 1);

    fbe_database_complete_packet(packet, status);
    return status;
}


static fbe_status_t database_control_get_tables(fbe_packet_t * packet)
{
    fbe_status_t                      status = FBE_STATUS_OK;
    fbe_database_control_get_tables_t *get_tables = NULL;
    database_object_entry_t           *object_entry = NULL;
    database_edge_entry_t             *edge_entry = NULL;
    database_user_entry_t             *user_entry = NULL;	
    fbe_u32_t edge_i = 0;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, &get_tables, sizeof(*get_tables));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    } 
    
    else
    {
        /* Get the object entry*/
        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                  get_tables->object_id,
                                                                  &object_entry);  
        if ( status != FBE_STATUS_OK )
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get object entry for object %u\n", 
                           __FUNCTION__, 
                           get_tables->object_id);
        }
        else
        {
            get_tables->tables.object_entry = *object_entry;
        }
        
        /* Get the edge entry */
        for (edge_i = 0; edge_i < DATABASE_MAX_EDGE_PER_OBJECT; edge_i++)
        {
            status = fbe_database_config_table_get_edge_entry(&fbe_database_service.edge_table,
                                                          get_tables->object_id,
                                                           edge_i,
                                                          &edge_entry);  
            if ( status != FBE_STATUS_OK )
            {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: failed to get edge entry for object %u\n", 
                               __FUNCTION__, 
                               get_tables->object_id);
            }
            else
            {
                get_tables->tables.edge_entry[edge_i] = *edge_entry;
            }
        }
        

        /* Get the user entry */     
        status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                                       get_tables->object_id,
                                                                       &user_entry);
        if ( status != FBE_STATUS_OK )
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: failed to user entry %u\n", 
                         __FUNCTION__,
                         get_tables->object_id);
        }
        else
        {
            get_tables->tables.user_entry = *user_entry;
        }

    } 
   
    fbe_database_complete_packet(packet, status);
    return status;
}


static fbe_status_t database_control_enumerate_system_objects(fbe_packet_t *packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_database_control_enumerate_system_objects_t *enumerate_system_objects = NULL;
    fbe_u32_t                               i  =0;
    fbe_payload_ex_t *                         payload = NULL;
    fbe_sg_element_t *                      sg_element = NULL;
    fbe_u32_t                               sg_elements = 0;
    fbe_object_id_t *                       obj_id_list = NULL;
    fbe_u32_t                               total_objects_copied = 0;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)
                                      &enumerate_system_objects,
                                      sizeof(*enumerate_system_objects));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /*! @todo Currently class specification isn't supported.
     */
    if (enumerate_system_objects->class_id != FBE_CLASS_ID_INVALID)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: Currently filter by class id: 0x%x isn't supported.\n", 
                       __FUNCTION__, enumerate_system_objects->class_id);
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /*we need the sg list in order to fill this memory with the object IDs*/
    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_elements);

    obj_id_list = (fbe_object_id_t *)sg_element->address;

    /*sanity check*/
    if ((sg_element->count / sizeof(fbe_object_id_t)) != enumerate_system_objects->number_of_objects)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s Byte count: 0x%x doesn't agree with num objects: %d.\n",
                       __FUNCTION__, sg_element->count, enumerate_system_objects->number_of_objects);
        status = FBE_STATUS_GENERIC_FAILURE;
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /* Clean up the array */
    for (i = 0 ; i < enumerate_system_objects->number_of_objects; i++, obj_id_list ++)
    {
        *obj_id_list = FBE_OBJECT_ID_INVALID;
    }

    /* Invoke method to enumerate system objects */
    status = database_system_objects_enumerate_objects((fbe_object_id_t *)sg_element->address, 
                                                        enumerate_system_objects->number_of_objects, 
                                                        &total_objects_copied);
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s Enumerate system objects failed with status: 0x%x\n",
                       __FUNCTION__, status);
        fbe_database_complete_packet(packet, status);
        return status;
    }
    enumerate_system_objects->number_of_objects_copied = total_objects_copied;
    
    fbe_database_complete_packet(packet, status);
    return status;
}


fbe_status_t fbe_database_get_drive_transfer_limits(fbe_u32_t *max_bytes_per_drive_request, fbe_u32_t *max_sg_entries)
{
    *max_bytes_per_drive_request = 0x104000;
    *max_sg_entries = 0x82;
    return FBE_STATUS_OK;
}

fbe_status_t fbe_database_get_lun_export_type(fbe_object_id_t object_id, fbe_bool_t * export_type)
{
    fbe_status_t status = FBE_STATUS_GENERIC_FAILURE;
    fbe_private_space_layout_lun_info_t lun;

    if (object_id > FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST) {
        /* Not a system object, don't need to look up in the private space layout lib */
        *export_type = FBE_TRUE;
        return FBE_STATUS_OK;
    }
    else{
        status = fbe_private_space_layout_get_lun_by_object_id(object_id, &lun);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_INFO, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: failed to get private space layout info of LUN %d; returned 0x%x.\n",
                __FUNCTION__, object_id, status);
            *export_type = FBE_FALSE;
            return status;
        }
        *export_type = lun.export_device_b;
    }
    return status;
}

fbe_status_t fbe_database_get_lun_number(fbe_object_id_t object_id, fbe_lun_number_t *lun_number)
{
    fbe_status_t 			status = FBE_STATUS_GENERIC_FAILURE;
    database_user_entry_t   *out_entry_ptr = NULL;     

    if(object_id == FBE_OBJECT_ID_INVALID || object_id > fbe_database_service.max_configurable_objects)
    {
        database_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Invalid object id %d; returned 0x%x.\n",
            __FUNCTION__, object_id, status);

        *lun_number = FBE_U32_MAX;/*jiust so we can have a clue something went wrong*/

        return status;
    }


    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,	 
                                                                   object_id,
                                                                   &out_entry_ptr);
    if ((status == FBE_STATUS_OK)&&(out_entry_ptr != NULL)) {
        /* get the entry ok */
        if (out_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) {
            /* only return the object when the entry is valid */
            *lun_number = out_entry_ptr->user_data_union.lu_user_data.lun_number;
        }
    }else{
        *lun_number = FBE_LUN_ID_INVALID;
        database_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to find LUN %d; returned 0x%x.\n",
            __FUNCTION__, object_id, status);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}

fbe_status_t fbe_database_get_rg_number(fbe_object_id_t object_id, fbe_raid_group_number_t *rg_number)
{
    fbe_status_t 			status = FBE_STATUS_GENERIC_FAILURE;
    database_user_entry_t   *out_entry_ptr = NULL;     
    
    if(object_id == FBE_OBJECT_ID_INVALID || object_id > fbe_database_service.max_configurable_objects )
    {
        database_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Invalid object id %d; returned 0x%x.\n",
            __FUNCTION__, object_id, status);

        *rg_number = FBE_U32_MAX;/*jiust so we can have a clue something went wrong*/

        return status;
    }
    
    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table, 
                                                                   object_id,
                                                                   &out_entry_ptr);
    if ((status == FBE_STATUS_OK)&&(out_entry_ptr != NULL)) {
        /* get the entry ok */
        if (out_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) {
            /* only return the object when the entry is valid */
            *rg_number = out_entry_ptr->user_data_union.rg_user_data.raid_group_number;
        }
    }else{
        *rg_number = FBE_RAID_GROUP_INVALID;
        database_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to find RAID %d; returned 0x%x.\n",
            __FUNCTION__, object_id, status);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}

fbe_status_t fbe_database_get_service_table_ptr(database_table_t ** table_ptr, database_config_table_type_t table_type)
{
    fbe_status_t 			status = FBE_STATUS_OK;

    switch (table_type) {
    case DATABASE_CONFIG_TABLE_EDGE:
        *table_ptr = &fbe_database_service.edge_table;
        break;
    case DATABASE_CONFIG_TABLE_OBJECT:
        *table_ptr = &fbe_database_service.object_table;
        break;
    case DATABASE_CONFIG_TABLE_USER:
        *table_ptr = &fbe_database_service.user_table;
        break;
    case DATABASE_CONFIG_TABLE_GLOBAL_INFO:
        *table_ptr = &fbe_database_service.global_info;
        break;
    case DATABASE_CONFIG_TABLE_SYSTEM_SPARE:
        *table_ptr = &fbe_database_service.system_spare_table;
        break;
    default:
        *table_ptr = NULL;
        database_trace(FBE_TRACE_LEVEL_WARNING,	
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Table Type %d is not supported.\n",
                       __FUNCTION__, table_type);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}

/********************************************************************************** 
 * @brief
 *   process a received CMI message
 *   if the length of the received cmi msg is not equal with the length of the CMI message
 *   in current SP, return a warning CMI message to the sender.
 *   1) If the size of received CMI message is bigger, cut it by smaller size.
 *   2) If the size of received CMI message is smaller,
 *      initialize the other elements by default value.
 *   3) If the CMI message type is unknown, return a warning message to the sender.
 *  
 * @param
 *   db_cmi_msg 
 *  
 * @version 
 *      5/14/2012 modified by Gaohp (add size information in db cmi msg) 
 **************************************************************************************/
fbe_status_t fbe_database_process_received_cmi_message(fbe_database_cmi_msg_t *db_cmi_msg)
{
    switch (db_cmi_msg->msg_type) {
    case FBE_DATABE_CMI_MSG_TYPE_GET_CONFIG:
        fbe_database_cmi_process_get_config(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_FRU_DESCRIPTOR:
        fbe_database_cmi_process_update_fru_descriptor(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_SYSTEM_DB_HEADER:
        fbe_database_cmi_process_update_system_db_header(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_CONFIG:
        fbe_database_cmi_process_update_config(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_CONFIG_DONE:
        fbe_database_cmi_process_update_config_done(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_CONFIG_PASSIVE_INIT_DONE:
        fbe_database_cmi_process_update_config_passive_init_done(&fbe_database_service);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_START_CONFIRM:
        fbe_database_transaction_start_confirm_from_peer(db_cmi_msg);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_START:
        fbe_database_transaction_start_request_from_peer(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_INVALIDATE:
        fbe_database_transaction_invalidate_request_from_peer(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_INVALIDATE_CONFIRM:
        fbe_database_transaction_invalidate_confirm_from_peer(db_cmi_msg);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_COMMIT_CONFIRM:
        fbe_database_transaction_commit_confirm_from_peer(db_cmi_msg);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_COMMIT:
        fbe_database_transaction_commit_request_from_peer(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_COMMIT_DMA:
        fbe_database_transaction_commit_dma_from_peer(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_ABORT_CONFIRM:
        fbe_database_transaction_abort_confirm_from_peer(db_cmi_msg);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_TRANSACTION_ABORT:
        fbe_database_transaction_abort_request_from_peer(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_PVD:
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_PVD_BLOCK_SIZE:
    case FBE_DATABE_CMI_MSG_TYPE_CREATE_PVD:
    case FBE_DATABE_CMI_MSG_TYPE_DESTROY_PVD:
    case FBE_DATABE_CMI_MSG_TYPE_CREATE_VD:
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_VD:
    case FBE_DATABE_CMI_MSG_TYPE_DESTROY_VD:
    case FBE_DATABE_CMI_MSG_TYPE_CREATE_RAID:
    case FBE_DATABE_CMI_MSG_TYPE_DESTROY_RAID:
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_RAID:
    case FBE_DATABE_CMI_MSG_TYPE_CREATE_LUN:
    case FBE_DATABE_CMI_MSG_TYPE_DESTROY_LUN:
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_LUN:
    case FBE_DATABE_CMI_MSG_TYPE_CLONE_OBJECT:
    case FBE_DATABE_CMI_MSG_TYPE_CREATE_EDGE:
    case FBE_DATABE_CMI_MSG_TYPE_DESTROY_EDGE:
    case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_POWER_SAVE_INFO:
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_SPARE_CONFIG:
    case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_TIME_THRESHOLD_INFO:
    case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_BG_SERVICE_FLAG:
    case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_ENCRYPTION_MODE:
    case FBE_DATABE_CMI_MSG_TYPE_DATABASE_COMMIT_UPDATE_TABLE:
    case FBE_DATABASE_CMI_MSG_TYPE_UPDATE_ENCRYPTION_MODE:
    case FBE_DATABASE_CMI_MSG_TYPE_SETUP_ENCRYPTION_KEYS:
    case FBE_DATABASE_CMI_MSG_TYPE_REKEY_ENCRYPTION_KEYS:
    case FBE_DATABASE_CMI_MSG_TYPE_UPDATE_DRIVE_KEYS:
    case FBE_DATABE_CMI_MSG_TYPE_SET_GLOBAL_PVD_CONFIG:
    case FBE_DATABASE_CMI_MSG_TYPE_SET_ENCRYPTION_PAUSE:
    case FBE_DATABASE_CMI_MSG_TYPE_CREATE_EXTENT_POOL:
    case FBE_DATABASE_CMI_MSG_TYPE_CREATE_EXTENT_POOL_LUN:
    case FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL:
    case FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL_LUN:
        fbe_database_config_change_request_from_peer(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_PVD_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_PVD_BLOCK_SIZE_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_CREATE_PVD_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_DESTROY_PVD_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_CREATE_VD_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_VD_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_DESTROY_VD_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_CREATE_RAID_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_DESTROY_RAID_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_RAID_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_CREATE_LUN_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_DESTROY_LUN_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_LUN_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_CLONE_OBJECT_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_CREATE_EDGE_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_DESTROY_EDGE_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_POWER_SAVE_INFO_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_SPARE_CONFIG_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_TIME_THRESHOLD_INFO_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_BG_SERVICE_FLAG_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_CONNECT_DRIVE_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_ENCRYPTION_MODE_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_DATABASE_COMMIT_UPDATE_TABLE_CONFIRM:
    case FBE_DATABASE_CMI_MSG_TYPE_UPDATE_ENCRYPTION_MODE_CONFIRM:
    case FBE_DATABASE_CMI_MSG_TYPE_SET_ENCRYPTION_PAUSE_CONFIRM:
    case FBE_DATABE_CMI_MSG_TYPE_SET_GLOBAL_PVD_CONFIG_CONFIRM:
    case FBE_DATABASE_CMI_MSG_TYPE_CREATE_EXTENT_POOL_CONFIRM:
    case FBE_DATABASE_CMI_MSG_TYPE_CREATE_EXTENT_POOL_LUN_CONFIRM:
    case FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL_CONFIRM:
    case FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL_LUN_CONFIRM:
        fbe_database_config_change_confirmed_from_peer(db_cmi_msg);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_DB_SERVICE_MODE:
        fbe_database_db_service_mode_state_from_peer(&fbe_database_service);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_FRU_DESCRIPTOR_CONFIRM:
        fbe_database_cmi_update_fru_descriptor_confirm_from_peer(db_cmi_msg);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_SYSTEM_DB_HEADER_CONFIRM:
        fbe_database_cmi_update_system_db_header_confirm_from_peer(db_cmi_msg);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UNKNOWN_MSG_TYPE:
        /**************************************************************************** 
         * If current SP is a new version, and its new type of CMI message can't be
         * recognized by peer SP, peer SP will send back a warning CMI message.
         * If needed, please add the process function here.
         *
         * For example, if the sender is waiting for the confirm message from peer,
         * but peer SP can' recognized the CMI message type and send back this warning
         * message, current SP can report warning and wake up the waiter here.
         ****************************************************************************/
        fbe_database_cmi_unknown_cmi_msg_by_peer(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UNKNOWN_MSG_SIZE:
        fbe_database_cmi_unknown_cmi_msg_size_by_peer(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_MAKE_DRIVE_ONLINE:
        fbe_database_cmi_process_online_drive_request(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_MAKE_DRIVE_ONLINE_CONFIRM:
        fbe_database_cmi_online_drive_confirm_from_peer(db_cmi_msg);
        break;
    case FBE_DATABE_CMI_MSG_TYPE_UPDATE_SHARED_EMV_INFO:
        fbe_database_cmi_process_update_shared_emv_info(&fbe_database_service, db_cmi_msg);
        break;
     case FBE_DATABE_CMI_MSG_TYPE_CLEAR_USER_MODIFIED_WWN_SEED:
        fbe_database_clear_user_modified_wwn_seed_flag();
        break;
    case FBE_DATABE_CMI_MSG_TYPE_MAILBOMB:
        database_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Received a mailbomb from peer.\n",
                        __FUNCTION__);
        break;

    case FBE_DATABE_CMI_MSG_TYPE_CONNECT_DRIVE:
        fbe_database_cmi_process_connect_drives(db_cmi_msg);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_UPDATE_CONFIG_TABLE:
        fbe_database_cmi_process_update_config_table(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_UPDATE_CONFIG_TABLE_CONFIRM:
        fbe_database_cmi_process_update_config_table_confirm(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_SETUP_ENCRYPTION_KEYS_CONFIRM:
    case FBE_DATABASE_CMI_MSG_TYPE_REKEY_ENCRYPTION_KEYS_CONFIRM:
        fbe_database_cmi_process_setup_encryption_keys_confirm(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_UPDATE_DRIVE_KEYS_CONFIRM:
        fbe_database_cmi_process_update_drive_keys_confirm(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_GET_BE_PORT_INFO:
        fbe_database_cmi_process_get_be_port_info(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_GET_BE_PORT_INFO_CONFIRM:
        fbe_database_cmi_process_get_be_port_info_confirm(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_SET_ENCRYPTION_BACKUP_STATE:
        fbe_database_cmi_process_set_encryption_backup_state(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_SET_ENCRYPTION_BACKUP_STATE_CONFIRM:
        fbe_database_cmi_process_set_encryption_backup_state_confirm(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_SETUP_KEK:
        fbe_database_cmi_process_setup_kek(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_SETUP_KEK_CONFIRM:
        fbe_database_cmi_process_setup_kek_confirm(&fbe_database_service, db_cmi_msg);
        break;
     case FBE_DATABASE_CMI_MSG_TYPE_DESTROY_KEK:
        fbe_database_cmi_process_destroy_kek(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_DESTROY_KEK_CONFIRM:
        fbe_database_cmi_process_destroy_kek_confirm(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_SETUP_KEK_KEK:
        fbe_database_cmi_process_setup_kek_kek(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_SETUP_KEK_KEK_CONFIRM:
        fbe_database_cmi_process_setup_kek_kek_confirm(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_DESTROY_KEK_KEK:
        fbe_database_cmi_process_destroy_kek_kek(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_DESTROY_KEK_KEK_CONFIRM:
        fbe_database_cmi_process_destroy_kek_kek_confirm(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_REESTABLISH_KEK_KEK:
        fbe_database_cmi_process_reestablish_kek_kek(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_REESTABLISH_KEK_KEK_CONFIRM:
        fbe_database_cmi_process_reestablish_kek_kek_confirm(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_PORT_ENCRYPTION_MODE:
        fbe_database_cmi_process_port_encryption_mode(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_PORT_ENCRYPTION_MODE_CONFIRM:
        fbe_database_cmi_process_port_encryption_mode_confirm(&fbe_database_service, db_cmi_msg);
        break;
    case FBE_DATABASE_CMI_MSG_TYPE_KILL_VAULT_DRIVE:
        fbe_database_cmi_process_invalid_vault_drive(&fbe_database_service,db_cmi_msg);
        break;
    default:
        /* If received unknown CMI message, return type and error status to peer*/
        fbe_database_handle_unknown_cmi_msg_type_from_peer(&fbe_database_service, db_cmi_msg);

        /****************************************************************************
         *  5/11/2012: The caller of this function doesn't check the return value,
         *  but the comments before tell us return FBE_STATUS_GENERIC_FAILURE.
         *  Leave it unchanged now.
         ****************************************************************************/
        return FBE_STATUS_GENERIC_FAILURE;/*this is important because this status will be sent back to the peer to tell him we don't understand*/
    }

    /*no need to free any memory, CMI portion of database just gave it to you to look at and will take care of that*/

    return FBE_STATUS_OK;/*any status here does not matter because we are on a processign thread context that has nothing to do with it*/

}

/*to be implemented by the database code to process a lost peer*/
fbe_status_t fbe_database_process_lost_peer(void)
{
    fbe_database_state_t	db_state;
    fbe_status_t			status;
    fbe_u32_t               valid_db_drives = 0;
    fbe_homewrecker_fru_descriptor_t out_fru_descriptor;
    fbe_homewrecker_get_raw_mirror_info_t   fru_descriptor_error_report;
    homewrecker_system_disk_table_t system_disk_table[FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER] = {0};
    homewrecker_system_disk_table_t    standard_disk = {0};
    fbe_u32_t disk_index = 0;
    fbe_homewrecker_get_fru_disk_mask_t disk_mask;
    fbe_u32_t               retry_count = 10;
    
    /* Get local DB state */
    get_database_service_state(&fbe_database_service, &db_state);

    /* The peer is dead so reset the peer state to Invalid. */
    set_database_service_peer_state(&fbe_database_service, FBE_DATABASE_STATE_INVALID);

    /* send notification */
    fbe_database_send_encryption_change_notification(DATABASE_ENCRYPTION_NOTIFICATION_REASON_PEER_LOST);

    /* The DB is in initializing state. if the SP was an active SP, and peer SP died. 
        *  The config thread is still running to initialize the database service. 
        *  It's safe if waking up the thread and return here.
        *  If the SP was a passive SP and peer SP died. The config thread is still running ,
        *  and turns db state to "FBE_DATABASE_STATE_WAITING_FOR_CONFIG".
        *  It's also safe if waking up the config thread and return here.
        */
    if (db_state == FBE_DATABASE_STATE_INITIALIZING) {
        database_trace(FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: database is still in initializing state.\n",
                        __FUNCTION__);
        fbe_db_thread_wakeup();
        return FBE_STATUS_OK;
    }
        

    /*let's see if we are in a state where we upadte the peer tables*/
    if ((db_state == FBE_DATABASE_STATE_UPDATING_PEER) &&
        (fbe_database_service.peer_config_update_job_packet != NULL)) {

        database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Peer dies while we update its tables; stopping job. State=0x%X. \n",
                       __FUNCTION__, db_state);

        set_database_service_state(&fbe_database_service, FBE_DATABASE_STATE_READY);

        database_control_complete_update_peer_packet(FBE_STATUS_GENERIC_FAILURE);

        return FBE_STATUS_OK;
    }

    /*are we the passive peer and are we in the middle of updating out tables?*/
    if (db_state == FBE_DATABASE_STATE_WAITING_FOR_CONFIG) {

        database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Active Peer died while it sent us tables, erasing and loading from disks.\n",
                       __FUNCTION__);

        fbe_database_reload_tables_in_mid_update();

        return FBE_STATUS_OK;
        
    }

    /*no point of doing anything if we destroy*/
    if ((db_state == FBE_DATABASE_STATE_DESTROYING)||
        (db_state == FBE_DATABASE_STATE_DESTROYING_SYSTEM)){
         return FBE_STATUS_OK;
    }

    if (db_state == FBE_DATABASE_STATE_SERVICE_MODE) {
        database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Peer died; current SP is in degraded mode\n",
                       __FUNCTION__);
        return FBE_STATUS_OK;
    }

    /* Before promoted to active, this SP is still be the active side. Just return.*/
    if (fbe_database_service.db_become_ready_in_passiveSP == FBE_FALSE) {
        database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Peer died, but the SP is active before peer died. do nothing\n",
                       __FUNCTION__);
        return FBE_STATUS_OK;
    }
                       
    /*if this is a live system which is not in the middle of configuration and then the active side dies
    we need to take over the drive discovery so we want to refresh the tables again in case the active
    side was in the middle of finding a new drive. This call will not overwrite or hurst existing drives
    we also want to make sure the persist sevice updates it's tables so we run the "set_lun" command*/

    set_database_service_state(&fbe_database_service, FBE_DATABASE_STATE_INITIALIZING);


    status = fbe_database_raw_mirror_get_valid_drives(&valid_db_drives);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                FBE_TRACE_MESSAGE_ID_INFO, 
                "Database service failed to get valid raw-mirror drives, instead load from all drives \n");
            valid_db_drives = DATABASE_SYSTEM_RAW_MIRROR_ALL_DB_DRIVE_MASK;
    }

    /*Read fru descriptor region from disk*/
    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Read FRU descriptor from raw mirror\n",
                    __FUNCTION__);
    
    while(retry_count--)
    {
         status = fbe_database_get_homewrecker_fru_descriptor(&out_fru_descriptor,
                                                                                    &fru_descriptor_error_report,
                                                                                    FBE_FRU_DISK_ALL);

        if (FBE_STATUS_OK != status)
        {
            database_trace (FBE_TRACE_LEVEL_WARNING,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Failed to read fru descriptor, delay 3 second then retry it again\n",
                            __FUNCTION__);       
        } else if (fru_descriptor_error_report.status == FBE_GET_HW_STATUS_OK)
        {
            /* dp_report.status == OK shows every drive in raw_mirror is OK */

            /* Then we can set standard_disk->dp directly, we choose to trust in raw_mirror 
              * when it told Homewrecker all disks are OK.
              */
            fbe_database_display_fru_descriptor(&out_fru_descriptor);
            fbe_copy_memory(&(standard_disk.dp), &out_fru_descriptor, sizeof(out_fru_descriptor));

            /* After we set standard_disk->dp, we should set global_fru_descriptor_sequence_number.
              * In order to let Homewrecker get correct seq_num in possible following FRU_Descriptor update
              */
            status = database_set_fru_descriptor_sequence_number(standard_disk.dp.sequence_number);
            if (status != FBE_STATUS_OK)
            {
                database_trace(FBE_TRACE_LEVEL_ERROR, 
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "Homewrecker: Set fru descriptor sequence number failed. delay 3 second then try it again\n");                
            }
            else         
                /* Get standard FRU descriptor and set FRU descriptor seq num successfully. */
                break;
        }else
        {
            /* Has ERROR when read fru descriptor from raw mirror */

            /* Read all the fru descriptor seperatly from disks */
            database_trace(FBE_TRACE_LEVEL_INFO, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Homewrecker: Read fru descriptor seperatly from each disk in raw_mirror.\n");

            for (disk_index = 0; disk_index < FBE_FRU_DESCRIPTOR_DB_DRIVE_NUMBER; disk_index++)
            {
                disk_mask = 1 << disk_index;
                status = fbe_database_get_homewrecker_fru_descriptor(&system_disk_table[disk_index].dp, 
                                                                        &system_disk_table[disk_index].dp_report, disk_mask);

                if (FBE_STATUS_OK != status)
                {
                    database_trace(FBE_TRACE_LEVEL_INFO, 
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "Homewrecker: Read fru descriptor from disk 0_0_%d failed.\n",
                                    disk_index);
                    continue;
                }

                /* output the FRU Descriptor got from this disk into trace */
                database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                               "Homewrecker: on-disk FRU_Descriptor from disk 0_0_%d:\n",
                                               disk_index);
                fbe_database_display_fru_descriptor(&(system_disk_table[disk_index].dp));
            }
            
            /* all 3 copies of fru_descriptor on each DB drive are got
              * it is time to determine the standard fru_descriptor
              */
            status = fbe_database_homewrecker_determine_standard_fru_descriptor(system_disk_table, &standard_disk);    
            if (FBE_STATUS_OK != status)
            {
                database_trace (FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Failed to get standard fru descriptor, delay 3 second then retry it.\n",
                                __FUNCTION__);         
                fbe_thread_delay(3000); 
                continue;
            }

            /* After we set standard_disk->dp, we should set global_fru_descriptor_sequence_number.
              * In order to let Homewrecker get correct seq_num in possible following FRU_Descriptor update
              */
            status = database_set_fru_descriptor_sequence_number(standard_disk.dp.sequence_number);
            if (status != FBE_STATUS_OK)
            {
                database_trace(FBE_TRACE_LEVEL_ERROR, 
                                                FBE_TRACE_MESSAGE_ID_INFO,
                                                "Homewrecker: Set fru descriptor sequence number failed, delay 3 second then retry it.\n");
            }
            else
                /* Get standard FRU descriptor and set FRU descriptor seq num successfully. */
                break;
        }
  
         
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                        FBE_TRACE_MESSAGE_ID_INFO, 
                        "%s: failed to read fru descriptor from disk, retry again\n", __FUNCTION__);

        fbe_thread_delay(3000);        
    }

    if(status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "%s: failed to read fru descriptor from disk, retry already done\n", __FUNCTION__);
        return status;
    }

     fbe_spinlock_lock(&fbe_database_service.pvd_operation.system_fru_descriptor_lock);
     fbe_copy_memory(&fbe_database_service.pvd_operation.system_fru_descriptor, &(standard_disk.dp), sizeof(standard_disk.dp));
     fbe_spinlock_unlock(&fbe_database_service.pvd_operation.system_fru_descriptor_lock);

     /* init system db seq number */
     fbe_database_raw_mirror_init_block_seq_number(valid_db_drives);

    /* When the passive side takes over the services, it should replay the system db journal */
    status = database_system_db_persist_init(valid_db_drives);
    if (status != FBE_STATUS_OK){
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "%s: failed to init and replay system db journal\n", __FUNCTION__);
        return status;
    }
    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO, 
                    "check and replay system db journal done\n");

    /*we can do that again, if we were the active side and the passive dies it will complete quickly and nothing is changed*/
    status = fbe_database_init_persist();
    if (status != FBE_STATUS_OK) {
        /* after peer goes away, this code could be executed while local is unloading,
         * it may be expected to get an error in this case.  Change to Warning level.
         */
        database_trace (FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to init persist service\n", __FUNCTION__);
        return status;
    }
    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO, 
                    "fbe_database_init_persist done\n");

    status = fbe_database_process_incomplete_transaction_after_peer_died(&fbe_database_service);
    if (status != FBE_STATUS_OK){
        database_trace (FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to process incomplete transaction\n", __FUNCTION__);
        return status;
    }
    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO, 
                    "Check or process incomplete transaction done\n");

    /* if our global info table is out of sync with media, let's reload it */
    if (fbe_database_config_is_global_info_out_of_sync(&fbe_database_service.global_info))
    {
        database_trace(FBE_TRACE_LEVEL_INFO,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Peer died, reloading global info table\n",__FUNCTION__);
        status = fbe_database_load_global_info_table();
        if (status != FBE_STATUS_OK) {
            set_database_service_state(&fbe_database_service, FBE_DATABASE_STATE_DEGRADED);

            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to reinit global info table, status %d\n", __FUNCTION__, status);
            return status;
        }
    }

    database_trace(FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: Peer died, refreshing drive list\n",__FUNCTION__);

    status =  fbe_database_drive_discover(&fbe_database_service.pvd_operation);
    if (status != FBE_STATUS_OK) {
        set_database_service_state(&fbe_database_service, FBE_DATABASE_STATE_DEGRADED);

        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to init drive connection\n", __FUNCTION__);
        return status;
    }
    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO, 
                   "refresh drive list done\n");

    set_database_service_state(&fbe_database_service, FBE_DATABASE_STATE_READY);

    /* If enabled validate the in-memory database.
     */
    if (fbe_database_is_debug_flag_set(&fbe_database_service, FBE_DATABASE_DEBUG_FLAG_VALIDATE_DATABASE_ON_PEER_LOSS)) {
        fbe_database_start_validate_database_job(FBE_DATABASE_VALIDATE_REQUEST_TYPE_PEER_LOST,
                                                 FBE_DATABASE_VALIDATE_FAILURE_ACTION_ERROR_TRACE /* Error Trace and log*/);
    }

    /*start the drive process thread right after database becomes READY*/
    fbe_database_drive_connection_start_drive_process(&fbe_database_service);

    return FBE_STATUS_OK;
}

/*returns the default offset for a given object ID based on it's type and location*/
fbe_status_t fbe_database_get_default_offset(fbe_class_id_t obj_class_id, fbe_object_id_t object_id, fbe_lba_t *default_lba)
{
    database_class_id_t db_class_id = DATABASE_CLASS_ID_INVALID;

    db_class_id = database_common_map_class_id_fbe_to_database(obj_class_id);
    
    /*let's see what we got*/
    if ((db_class_id > DATABASE_CLASS_ID_RAID_START && db_class_id < DATABASE_CLASS_ID_RAID_END) ||
        (db_class_id == DATABASE_CLASS_ID_LUN) || (db_class_id == DATABASE_CLASS_ID_BVD_INTERFACE)) {
    
        *default_lba = 0;
    }else if (db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE || db_class_id == DATABASE_CLASS_ID_VIRTUAL_DRIVE) {
    
        /*let's see if these are system drives or not*/
        if (object_id > FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST) {
            fbe_private_space_layout_get_start_of_user_space(FBE_PRIVATE_SPACE_LAYOUT_ALL_FRUS, default_lba);
        }else{
            /*because system vd was removed, there is no VDs in system objects.
                    This if statment is for unexpect system vd passing in. 
                    If system VD is enabled, please change the codes here too*/
            if (db_class_id == DATABASE_CLASS_ID_VIRTUAL_DRIVE)
            {
                database_trace(FBE_TRACE_LEVEL_ERROR,
                               FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                               "%s: VD should not come to here\n", __FUNCTION__);
                return FBE_STATUS_GENERIC_FAILURE;
            }
            /*PVD object id is corresponding to slot number plus 1*/
            FBE_ASSERT_AT_COMPILE_TIME(FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_PVD_0 == 1);
            fbe_private_space_layout_get_start_of_user_space(object_id - 1, default_lba);
        }
    } else if (db_class_id == DATABASE_CLASS_ID_EXTENT_POOL || 
               db_class_id == DATABASE_CLASS_ID_EXTENT_POOL_LUN ||
               db_class_id == DATABASE_CLASS_ID_EXTENT_POOL_METADATA_LUN) {
        *default_lba = 0;
    } else
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s failed to get object(%d) entry. Obj Class id: %d, DB class id: %d\n",
                       __FUNCTION__, object_id, obj_class_id, db_class_id);
    
        return FBE_STATUS_GENERIC_FAILURE;
    }

    database_trace(FBE_TRACE_LEVEL_INFO,
                   FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                   "%s: Obj Class id: %d, DB class id: %d, Offset: 0x%llx\n",
                   __FUNCTION__, obj_class_id, db_class_id,
           (unsigned long long)(*default_lba));

    return FBE_STATUS_OK;
}


fbe_status_t database_get_pvd_operation(database_pvd_operation_t **pvd_operation)
{
    *pvd_operation = &fbe_database_service.pvd_operation;

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn database_control_get_encryption
 *****************************************************************
 * @brief
 *   Control function to get the encryption information. This futher
 *   sends the control code to base_config to get the global
 *   system encryption information.
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *
 ****************************************************************/
static fbe_status_t database_control_get_encryption(fbe_packet_t *packet)
{
    fbe_status_t                       status = FBE_STATUS_OK;
    fbe_database_encryption_t          *encryption = NULL;
    database_global_info_entry_t       *global_info_entry = NULL;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&encryption, sizeof(*encryption));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /* Get the Global Info entry from the config table */		
    status = fbe_database_config_table_get_global_info_entry(&fbe_database_service.global_info, 
                                                             DATABASE_GLOBAL_INFO_TYPE_SYSTEM_ENCRYPTION, 
                                                             &global_info_entry);

    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get Global Info entry from the DB service.\n", 
                       __FUNCTION__);
    }

    status = fbe_database_send_control_packet_to_class(&global_info_entry->info_union.encryption_info, 
                                                       sizeof(fbe_system_encryption_info_t), 
                                                       FBE_BASE_CONFIG_CONTROL_CODE_GET_SYSTEM_ENCRYPTION_INFO, 
                                                       FBE_PACKAGE_ID_SEP_0,
                                                       FBE_SERVICE_ID_TOPOLOGY,
                                                       FBE_CLASS_ID_BASE_CONFIG,
                                                       NULL,
                                                       0,
                                                       FBE_FALSE);

    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to send GET Global Info for encryption.\n", 
                       __FUNCTION__);
    }

    /* Copy the contents from the database table */
    fbe_copy_memory(&encryption->system_encryption_info,
                    &global_info_entry->info_union.encryption_info, 
                    sizeof(fbe_system_encryption_info_t));

    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn database_control_set_encryption
 *****************************************************************
 * @brief
 *   Control function to set the encryption information.
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    5/06/2011: Arun S - created
 *
 ****************************************************************/
static fbe_status_t database_control_set_encryption(fbe_packet_t *packet)
{
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_database_encryption_t              *encryption = NULL;
    fbe_database_transaction_id_t          db_transaction_id = 0;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&encryption, sizeof(*encryption));
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Get Payload failed.\n", 
                       __FUNCTION__);

        fbe_database_complete_packet(packet, status);
        return status;
    }

    fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &db_transaction_id);

    status = fbe_database_transaction_is_valid_id(encryption->transaction_id, db_transaction_id);

    if (encryption->system_encryption_info.encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED) {
        /* Tell the library that we are now encrypted.
         */
        fbe_raid_library_set_encrypted();
    }

    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = database_set_system_encryption_mode(encryption->system_encryption_info);
    if (status == FBE_STATUS_OK && is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)encryption,
                                                             FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_ENCRYPTION_MODE);
    }

    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn database_control_get_power_save
 *****************************************************************
 * @brief
 *   Control function to get the power save information. This futher
 *   sends the control code to base_config to get the global
 *   system power save information.
 *
 * @param packet - packet
 * @param packet_status - status to set for the packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    5/06/2011: Arun S - created
 *
 ****************************************************************/
static fbe_status_t database_control_get_power_save(fbe_packet_t *packet)
{
    fbe_status_t                       status = FBE_STATUS_OK;
    fbe_database_power_save_t          *power_save = NULL;
    database_global_info_entry_t       *global_info_entry = NULL;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&power_save, sizeof(*power_save));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /* Get the Global Info entry from the config table */		
    status = fbe_database_config_table_get_global_info_entry(&fbe_database_service.global_info, 
                                                             DATABASE_GLOBAL_INFO_TYPE_SYSTEM_POWER_SAVE, 
                                                             &global_info_entry);

    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get Global Info entry from the DB service.\n", 
                       __FUNCTION__);
    }

    status = fbe_database_send_control_packet_to_class(&global_info_entry->info_union.power_saving_info, 
                                                       sizeof(fbe_system_power_saving_info_t), 
                                                       FBE_BASE_CONFIG_CONTROL_CODE_GET_SYSTEM_POWER_SAVING_INFO, 
                                                       FBE_PACKAGE_ID_SEP_0,
                                                       FBE_SERVICE_ID_TOPOLOGY,
                                                       FBE_CLASS_ID_BASE_CONFIG,
                                                       NULL,
                                                       0,
                                                       FBE_FALSE);

    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to send GET Global Info for Power Save.\n", 
                       __FUNCTION__);
    }

    /* Copy the contents from the database table */
    fbe_copy_memory(&power_save->system_power_save_info,
                    &global_info_entry->info_union.power_saving_info, 
                    sizeof(fbe_system_power_saving_info_t));

    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn database_control_set_power_save
 *****************************************************************
 * @brief
 *   Control function to set the power save information.
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    5/06/2011: Arun S - created
 *
 ****************************************************************/
static fbe_status_t database_control_set_power_save(fbe_packet_t *packet)
{
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_database_power_save_t              *power_save = NULL;
    fbe_database_transaction_id_t          db_transaction_id = 0;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&power_save, sizeof(*power_save));
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Get Payload failed.\n", 
                       __FUNCTION__);

        fbe_database_complete_packet(packet, status);
        return status;
    }

    fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &db_transaction_id);

    status = fbe_database_transaction_is_valid_id(power_save->transaction_id, db_transaction_id);

    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = database_set_power_save(power_save);
    if (status == FBE_STATUS_OK && is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)power_save,
                                                             FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_POWER_SAVE_INFO);
    }

    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;
}

/*job service is asking us to send the configuration to the peer*/
static fbe_status_t database_control_update_peer_config(fbe_packet_t *packet)
{
    fbe_status_t                    status = FBE_STATUS_OK;
    fbe_database_state_t 			state;
    fbe_u32_t						wait_count = 0;
    database_system_db_header_t*     update_system_db_header = NULL;
    fbe_homewrecker_fru_descriptor_t* fru_descriptor_sent_to_peer = NULL; 
    fbe_object_id_t pdo_id = FBE_OBJECT_ID_INVALID;
    fbe_system_encryption_mode_t encryption_mode;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)NULL, 0);
    if ( status != FBE_STATUS_OK ){
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /*before we do anything we need to make sure we are initialized. We'll spin here on the expense of the job service context
    but this is not a big deal because until we are ready there is nothing much happening in the system anyways*/
    
    do {
        get_database_service_state(&fbe_database_service, &state);

        if (wait_count != 0) {
            fbe_thread_delay(100);
        }

        /*10 minutes is way too long*/
        if (wait_count == 6000) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: timed out waiting for DB to become ready.\n",__FUNCTION__ );

            set_database_service_state(&fbe_database_service, FBE_DATABASE_STATE_DEGRADED);
            fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        wait_count++;

    } while (state != FBE_DATABASE_STATE_READY);
    

    set_database_service_peer_state(&fbe_database_service, FBE_DATABASE_STATE_WAITING_FOR_CONFIG);
    fbe_database_get_system_encryption_mode(&encryption_mode);
    if(encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED)
    {
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: Waiting on KMS to give us go ahead ... \n",__FUNCTION__ );
        set_database_service_state(&fbe_database_service, FBE_DATABASE_STATE_WAITING_ON_KMS);

        /* send notification */
        fbe_database_send_encryption_change_notification(DATABASE_ENCRYPTION_NOTIFICATION_REASON_PEER_REQUEST_TABLE);

        /* Wait here for the KMS to give a go ahead */
        do {
            get_database_service_state(&fbe_database_service, &state);
            fbe_thread_delay(100);
            /* Fix AR 698996: when peer SP is not alive, we should just the loop and return error */
            if (!fbe_cmi_is_peer_alive())
            {
                database_trace(FBE_TRACE_LEVEL_WARNING, 
                               FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s: Peer is not alive.\n",__FUNCTION__ );

                set_database_service_state(&fbe_database_service, FBE_DATABASE_STATE_READY);
                set_database_service_peer_state(&fbe_database_service, FBE_DATABASE_STATE_INVALID);
                fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
                return FBE_STATUS_GENERIC_FAILURE;
            }
                
        } while (state != FBE_DATABASE_STATE_KMS_APPROVED);
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: KMS gave go ahead ... \n",__FUNCTION__ );
    }

    /*all we need to do is to start an "empty" job. This job will guarantee that while we update the 
      peer's configuration on it's context, no other job can run and we save ourselvs all the locking issues*/
    set_database_service_state(&fbe_database_service, FBE_DATABASE_STATE_UPDATING_PEER);
       

    /*first update peer system db header*/
    update_system_db_header = (database_system_db_header_t*)fbe_transport_allocate_buffer(); /*8 blks are enough to hold db header*/
    if(NULL == update_system_db_header)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: DB System Header is NULL. \n",__FUNCTION__ );
        set_database_service_state(&fbe_database_service, FBE_DATABASE_STATE_READY);
        set_database_service_peer_state(&fbe_database_service, FBE_DATABASE_STATE_INVALID);
        fbe_database_complete_packet(packet, status);
        return status;
    }
    fbe_spinlock_lock(&fbe_database_service.system_db_header_lock);
    fbe_copy_memory(update_system_db_header, &fbe_database_service.system_db_header, sizeof(database_system_db_header_t));
    fbe_spinlock_unlock(&fbe_database_service.system_db_header_lock);
    status = fbe_database_cmi_update_peer_system_db_header(update_system_db_header, &fbe_database_service.peer_sep_version);
    if ( status != FBE_STATUS_OK ){
        /* failed to update the peer, we will go back to ready state and mark peer state invalid */
        database_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to update the system db header to peer.", __FUNCTION__);
        fbe_transport_release_buffer((void*)update_system_db_header);
        set_database_service_state(&fbe_database_service, FBE_DATABASE_STATE_READY);
        set_database_service_peer_state(&fbe_database_service, FBE_DATABASE_STATE_INVALID);
        fbe_database_complete_packet(packet, status);
        return status;
    }
    
    /* then update peer in-memory fru descriptor */
    /* resue the memory allocated for update_system_db_header cmi message payload */
    fbe_zero_memory((void*)update_system_db_header, sizeof(fbe_homewrecker_fru_descriptor_t));
    fru_descriptor_sent_to_peer = (fbe_homewrecker_fru_descriptor_t*)update_system_db_header;

    fbe_spinlock_lock(&fbe_database_service.pvd_operation.system_fru_descriptor_lock);
    fbe_copy_memory(fru_descriptor_sent_to_peer, 
                                           &fbe_database_service.pvd_operation.system_fru_descriptor, 
                                           sizeof(fbe_homewrecker_fru_descriptor_t));
    fbe_spinlock_unlock(&fbe_database_service.pvd_operation.system_fru_descriptor_lock);
 
    status = fbe_database_cmi_update_peer_fru_descriptor(fru_descriptor_sent_to_peer);

    fbe_transport_release_buffer((void*)fru_descriptor_sent_to_peer);

    if ( status != FBE_STATUS_OK ){
        /* failed to update the peer, we will go back to ready state and mark peer state invalid */
        database_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to update the system db header to peer.", __FUNCTION__);
        set_database_service_state(&fbe_database_service, FBE_DATABASE_STATE_READY);
        set_database_service_peer_state(&fbe_database_service, FBE_DATABASE_STATE_INVALID);
        fbe_database_complete_packet(packet, status);
        return status;
    }        

    /*then update peer object/edge/global/spare tables*/
    if (fbe_database_ndu_is_committed()) {
        status = fbe_database_cmi_update_peer_tables_dma();
        if (status != FBE_STATUS_OK) {
            status = fbe_database_cmi_update_peer_tables();
        }
    } else {
        status = fbe_database_cmi_update_peer_tables();
    }
    if ( status != FBE_STATUS_OK ){
        /* failed to update the peer, we will go back to ready state and mark peer state invalid */
        set_database_service_state(&fbe_database_service, FBE_DATABASE_STATE_READY);
        set_database_service_peer_state(&fbe_database_service, FBE_DATABASE_STATE_INVALID);
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /*at this point we don't complete the packet but rather wait for either the peer dying or returnign to us
    the CMI opcode FBE_DATABE_CMI_MSG_TYPE_UPDATE_CONFIG_PASSIVE_INIT_DONE which means we can be ready on both 
    side for config changes. In the meantime, we'll have to keep a pointer of this packet in static memory
    but this is fine since we are under the protection of the job*/
    fbe_database_service.peer_config_update_job_packet = packet;


    if (database_common_peer_is_alive() && 
        fbe_database_process_killed_drive_needed(&pdo_id)) 
    {
        fbe_database_cmi_send_kill_drive_request_to_peer(pdo_id);
    }

    return FBE_STATUS_OK;

}

fbe_status_t database_control_complete_update_peer_packet(fbe_status_t status)
{
    fbe_database_complete_packet(fbe_database_service.peer_config_update_job_packet, status);
    fbe_database_service.peer_config_update_job_packet = NULL;

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn database_control_update_system_spare_config
 *****************************************************************
 * @brief
 *   Control function to set the global spare config.
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    6/01/2011: created
 *
 ****************************************************************/
static fbe_status_t database_control_update_system_spare_config(fbe_packet_t *packet)
{
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_database_control_update_system_spare_config_t   *spare_config = NULL;
    fbe_database_transaction_id_t          db_transaction_id = 0;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&spare_config, sizeof(*spare_config));
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Get Payload failed.\n", 
                       __FUNCTION__);

        fbe_database_complete_packet(packet, status);
        return status;
    }

    fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &db_transaction_id);

    status = fbe_database_transaction_is_valid_id(spare_config->transaction_id, db_transaction_id);

    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = database_update_system_spare_config(spare_config);
    if (status == FBE_STATUS_OK && is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)spare_config,
                                                             FBE_DATABE_CMI_MSG_TYPE_UPDATE_SPARE_CONFIG);
    }

    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;
}

fbe_status_t fbe_database_get_wwn_seed(fbe_u32_t * wwn_seed)
{
    *wwn_seed = 0xFBE35EED; /* Hard coded stub for now */
    return FBE_STATUS_OK;
}

fbe_status_t fbe_database_get_pvd_opaque_data(fbe_object_id_t object_id, fbe_u8_t *opaque_data_p, fbe_u32_t size)
{
    fbe_status_t 			status = FBE_STATUS_GENERIC_FAILURE;
    database_user_entry_t   *out_entry_ptr = NULL;  

    fbe_zero_memory(opaque_data_p, size);   

    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,	
                                                                   object_id,
                                                                   &out_entry_ptr);
    if ((status == FBE_STATUS_OK)&&(out_entry_ptr != NULL)) {
        /* get the entry ok */
        if (out_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) {
            /* copy the data */
            fbe_copy_memory(opaque_data_p, out_entry_ptr->user_data_union.pvd_user_data.opaque_data, size);   
        }
    }else{
        database_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to find PVD %d; returned 0x%x.\n",
            __FUNCTION__, object_id, status);
        status = FBE_STATUS_GENERIC_FAILURE;
    }
    return status;
}

/*!***************************************************************
 * @fn fbe_database_get_pvd_pool_id
 *****************************************************************
 * @brief
 *   Control function to get the pool-id of the PVD.
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    6/01/2011: created
 *
 ****************************************************************/
fbe_status_t fbe_database_get_pvd_pool_id(fbe_object_id_t object_id, fbe_u32_t *pool_id)
{
    fbe_status_t 			status = FBE_STATUS_GENERIC_FAILURE;
    database_user_entry_t   *out_entry_ptr = NULL;  
    database_object_entry_t *object_entry = NULL;

    /* Mask the pool id for ext pool */
    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              object_id,
                                                              &object_entry);  
    if (status == FBE_STATUS_OK && database_is_entry_valid(&object_entry->header) &&
        object_entry->set_config_union.pvd_config.config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_EXT_POOL){
        *pool_id = FBE_POOL_ID_INVALID;
        return FBE_STATUS_OK;
    }

    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,	
                                                                   object_id,
                                                                   &out_entry_ptr);
    if ((status == FBE_STATUS_OK)&&(out_entry_ptr != NULL))
    {
        /* get the entry ok */
        if (out_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID)
        {
            /* copy the data */
            *pool_id = out_entry_ptr->user_data_union.pvd_user_data.pool_id;
        }
    }
    else
    {
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: Failed to Get Pool id for the PVD %d.\n", 
                       __FUNCTION__, object_id);

        status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_database_set_pvd_pool_id
 *****************************************************************
 * @brief
 *   Control function to set the pool_id for the PVD.
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    6/01/2011: created
 *
 ****************************************************************/
fbe_status_t fbe_database_set_pvd_pool_id(fbe_object_id_t object_id, fbe_u32_t pool_id)
{
    fbe_status_t 			status = FBE_STATUS_GENERIC_FAILURE;
    database_user_entry_t   *out_entry_ptr = NULL;  

    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,	
                                                                   object_id,
                                                                   &out_entry_ptr);
    if ((status == FBE_STATUS_OK)&&(out_entry_ptr != NULL))
    {
        /* get the entry ok */
        if (out_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID)
        {
            /* copy the data */
            out_entry_ptr->user_data_union.pvd_user_data.pool_id = pool_id;
        }
    }
    else
    {
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: Failed to Get Pool id for the PVD %d.\n", 
                       __FUNCTION__, object_id);

        status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
static fbe_status_t database_update_pvd_per_type(database_object_entry_t  *new_pvd_entry, fbe_update_pvd_type_t update_type, fbe_database_control_update_pvd_t  *update_pvd)
{
    database_user_entry_t *			existing_pvd_user_entry = NULL;
    database_user_entry_t           new_pvd_user_entry;
    fbe_status_t					status = FBE_STATUS_OK;   
    fbe_system_encryption_mode_t	system_encryption_mode;

    switch (update_type)
    {
    case FBE_UPDATE_PVD_ENCRYPTION_IN_PROGRESS:
    case FBE_UPDATE_PVD_REKEY_IN_PROGRESS:
    case FBE_UPDATE_PVD_UNENCRYPTED:
    case FBE_UPDATE_PVD_TYPE:
        /* Let's handle case when PVD become a part of RG on encrypted array */
        status = fbe_database_get_system_encryption_mode(&system_encryption_mode);

        if ((system_encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED) && 
            //!fbe_database_is_object_system_pvd(update_pvd->object_id) &&
            (((new_pvd_entry->set_config_union.pvd_config.config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID) && 
              (update_pvd->config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID)) ||
             ((new_pvd_entry->set_config_union.pvd_config.config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID) && 
              (update_pvd->config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID)))) {

            /* depending on the update type, set our encryption mode. */
            if (update_type == FBE_UPDATE_PVD_ENCRYPTION_IN_PROGRESS) {
                 new_pvd_entry->base_config.encryption_mode = FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS;
            } else if (update_type == FBE_UPDATE_PVD_REKEY_IN_PROGRESS) {
                 new_pvd_entry->base_config.encryption_mode = FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS;
            } else if (update_type == FBE_UPDATE_PVD_UNENCRYPTED) {
                 new_pvd_entry->base_config.encryption_mode = FBE_BASE_CONFIG_ENCRYPTION_MODE_UNENCRYPTED;
            } else {
                /* PVD is about to become a part of RG, so we need to update encryption mode */
                new_pvd_entry->base_config.encryption_mode = FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED;
            }
        }

        /* Day one bug with destroy raid group Double updates are not supported */
        if((new_pvd_entry->set_config_union.pvd_config.config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_RAID) && 
            (update_pvd->config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED)){
            /* PVD is about to become UNCONSUMED, so we need to invalidate pool_id */
            fbe_spinlock_lock(&fbe_database_service.user_table.table_lock);
            status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                                            update_pvd->object_id,
                                                                            &existing_pvd_user_entry);
            if ( status != FBE_STATUS_OK ) {
                fbe_spinlock_unlock(&fbe_database_service.user_table.table_lock);
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Failed to find pvd %u in user table\n", __FUNCTION__, update_pvd->object_id); 
                return status;
            }
            fbe_copy_memory(&new_pvd_user_entry, existing_pvd_user_entry, sizeof(database_user_entry_t));
            fbe_spinlock_unlock(&fbe_database_service.user_table.table_lock);
            new_pvd_user_entry.header.state = DATABASE_CONFIG_ENTRY_MODIFY;
            /* even this is a modify, it can be first time a spare is configed, so let's fill it if entry id is 0 */
            if (new_pvd_user_entry.header.entry_id == 0){
                new_pvd_user_entry.header.object_id = update_pvd->object_id;
                new_pvd_user_entry.db_class_id = DATABASE_CLASS_ID_PROVISION_DRIVE;
            }
            new_pvd_user_entry.user_data_union.pvd_user_data.pool_id = FBE_POOL_ID_INVALID;

            status = fbe_database_transaction_add_user_entry(fbe_database_service.transaction_ptr, &new_pvd_user_entry);
            if ( status != FBE_STATUS_OK ){
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Failed to add user entry pvd 0x%x in user table\n", __FUNCTION__, update_pvd->object_id); 
            }
        } /* Day one bug with destroy raid group Double updates are not supported */ 
        

        new_pvd_entry->set_config_union.pvd_config.config_type = update_pvd->config_type;
        if (update_pvd->update_opaque_data == FBE_TRUE) {
            /* only update the user table when updating the pvd type */
            fbe_spinlock_lock(&fbe_database_service.user_table.table_lock);
            status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                                      update_pvd->object_id,
                                                                      &existing_pvd_user_entry);
            if ( status != FBE_STATUS_OK ) {
                fbe_spinlock_unlock(&fbe_database_service.user_table.table_lock);
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                             "%s Failed to find pvd %u in user table\n",
                             __FUNCTION__, 
                             update_pvd->object_id); 
                return status;
            }
            fbe_copy_memory(&new_pvd_user_entry, existing_pvd_user_entry, sizeof(database_user_entry_t));
            fbe_spinlock_unlock(&fbe_database_service.user_table.table_lock);
            new_pvd_user_entry.header.state = DATABASE_CONFIG_ENTRY_MODIFY;
            /* even this is a modify, it can be first time a spare is configed, so let's fill it if entry id is 0 */
            if (new_pvd_user_entry.header.entry_id == 0){
                new_pvd_user_entry.header.object_id = update_pvd->object_id;
                new_pvd_user_entry.db_class_id = DATABASE_CLASS_ID_PROVISION_DRIVE;
            }
            fbe_copy_memory(new_pvd_user_entry.user_data_union.pvd_user_data.opaque_data, 
                            update_pvd->opaque_data, 
                            sizeof(new_pvd_user_entry.user_data_union.pvd_user_data.opaque_data));

            status = fbe_database_transaction_add_user_entry(fbe_database_service.transaction_ptr,
                                                             &new_pvd_user_entry);
            if ( status != FBE_STATUS_OK ){
                set_database_service_state(&fbe_database_service,FBE_DATABASE_STATE_FAILED); /* TODO: Is this correct state? */
            }
        }
        break;
    case FBE_UPDATE_PVD_SNIFF_VERIFY_STATE:
        new_pvd_entry->set_config_union.pvd_config.sniff_verify_state=update_pvd->sniff_verify_state;
        break;
    case FBE_UPDATE_PVD_POOL_ID:
        fbe_spinlock_lock(&fbe_database_service.user_table.table_lock);
        status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                                  update_pvd->object_id,
                                                                  &existing_pvd_user_entry);
        if ( status != FBE_STATUS_OK )
        {
            fbe_spinlock_unlock(&fbe_database_service.user_table.table_lock);
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                         "%s Failed to find pvd %u in user table\n",
                         __FUNCTION__, 
                         update_pvd->object_id); 
            return status;
        }
        fbe_copy_memory(&new_pvd_user_entry, existing_pvd_user_entry, sizeof(database_user_entry_t));
        fbe_spinlock_unlock(&fbe_database_service.user_table.table_lock);
        new_pvd_user_entry.header.state = DATABASE_CONFIG_ENTRY_MODIFY;
        /* even this is a modify, it can be first time a spare is configed, so let's fill it if entry id is 0 */
        if (new_pvd_user_entry.header.entry_id == 0){
            new_pvd_user_entry.header.object_id = update_pvd->object_id;
            new_pvd_user_entry.db_class_id = DATABASE_CLASS_ID_PROVISION_DRIVE;
        }
        new_pvd_user_entry.user_data_union.pvd_user_data.pool_id = update_pvd->pool_id;

        status = fbe_database_transaction_add_user_entry(fbe_database_service.transaction_ptr,
                                                         &new_pvd_user_entry);
        if ( status != FBE_STATUS_OK ){
            set_database_service_state(&fbe_database_service,FBE_DATABASE_STATE_FAILED); /* TODO: Is this correct state? */
        }
        return FBE_STATUS_OK;  /* we don't need to update object table when we change only pool id */
        break;
    case FBE_UPDATE_PVD_SERIAL_NUMBER:
        fbe_copy_memory(new_pvd_entry->set_config_union.pvd_config.serial_num, 
                                               update_pvd->serial_num, 
                                               FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1);
        break; 
    case FBE_UPDATE_PVD_CONFIG:
        new_pvd_entry->set_config_union.pvd_config.configured_capacity = update_pvd->configured_capacity;
        new_pvd_entry->set_config_union.pvd_config.configured_physical_block_size = update_pvd->configured_physical_block_size;
        fbe_copy_memory(new_pvd_entry->set_config_union.pvd_config.serial_num, update_pvd->serial_num, 
                                               FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1);
        break;
    case FBE_UPDATE_BASE_CONFIG_ENCRYPTION_MODE:
        new_pvd_entry->base_config.encryption_mode = update_pvd->base_config_encryption_mode;
        status = database_common_update_encryption_mode_from_table_entry(new_pvd_entry);
        break;
               
    default:
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: Illegal pvd configuration update type: 0x%X\n",
                       __FUNCTION__, 
                       update_type);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

fbe_status_t database_update_pvd(fbe_database_control_update_pvd_t  *update_pvd)
{
    database_object_entry_t *		existing_pvd_entry = NULL;
    database_object_entry_t         new_pvd_entry;
    fbe_status_t					status;   
    fbe_update_pvd_type_t           individual_type;

    /* since only part of the data in table_entry is changed,
       copy the old data into the new entry, then modify the field we want. */
    /* first, let's get the existing pvd configuration from service object table*/
    fbe_spinlock_lock(&fbe_database_service.object_table.table_lock);
    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              update_pvd->object_id,
                                                              &existing_pvd_entry);
    if ( status != FBE_STATUS_OK ) {
        fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s Failed to find pvd %u in object table\n",
                     __FUNCTION__, 
                     update_pvd->object_id); 
        return status;
    }

    fbe_copy_memory(&new_pvd_entry, existing_pvd_entry,sizeof(database_object_entry_t));
    fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);
   
    if (update_pvd->update_type != FBE_UPDATE_PVD_USE_BITMASK) {
        status = database_update_pvd_per_type(&new_pvd_entry, update_pvd->update_type, update_pvd);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: failed to update config of object 0x%x\n",
                           __FUNCTION__, 
                           update_pvd->object_id);
            return status;
        }
        /* Fix AR 601890. accurev transaction 2227332 introduces this issue 
         * we don't need to update object table when we change only pool id, so just return */
        if (update_pvd->update_type == FBE_UPDATE_PVD_POOL_ID)
        {
            return status;
        }

    } else { /* Use bitmask */
        /* now we need to work on all update requests */
        individual_type = FBE_UPDATE_PVD_TYPE;
        do
        {
            /* if match, do update */
            if ((individual_type&update_pvd->update_type_bitmask)==individual_type)
            {
                status = database_update_pvd_per_type(&new_pvd_entry, individual_type, update_pvd);
                if (status != FBE_STATUS_OK) 
                {
                    database_trace(FBE_TRACE_LEVEL_WARNING, 
                                   FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                   "%s: failed to update config of object 0x%x, type 0x%x\n",
                                   __FUNCTION__, 
                                   update_pvd->object_id, individual_type);
                    return status;
                }
            }
            individual_type <<= 1;  /* next type */
        } while (individual_type<FBE_UPDATE_PVD_LAST);

        /*If we just mark the FBE_UPDATE_PVD_POOL_ID, just return here*/
        if (update_pvd->update_type_bitmask == FBE_UPDATE_PVD_POOL_ID)
        {
            return status;
        }
    }

    new_pvd_entry.header.state = DATABASE_CONFIG_ENTRY_MODIFY;
    /*update the update type*/
    new_pvd_entry.set_config_union.pvd_config.update_type = update_pvd->update_type;
    if (update_pvd->update_type == FBE_UPDATE_PVD_USE_BITMASK)
    {
        new_pvd_entry.set_config_union.pvd_config.update_type_bitmask = update_pvd->update_type_bitmask;
    }
    /* FBE_UPDATE_PVD_AFTER_ENCRYPTION_ENABLED should never been cleaned */
    if ((existing_pvd_entry->set_config_union.pvd_config.update_type_bitmask & FBE_UPDATE_PVD_AFTER_ENCRYPTION_ENABLED)
            == FBE_UPDATE_PVD_AFTER_ENCRYPTION_ENABLED)
    {
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: carry over ENCRYPTION bit, existing bits0x%x.\n",
                       __FUNCTION__, 
                       existing_pvd_entry->set_config_union.pvd_config.update_type_bitmask );
        new_pvd_entry.set_config_union.pvd_config.update_type_bitmask |= FBE_UPDATE_PVD_AFTER_ENCRYPTION_ENABLED;
    }
   
    status = database_common_update_config_from_table_entry(&new_pvd_entry);
    if (status != FBE_STATUS_OK) 
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to set config of object 0x%x\n",
                       __FUNCTION__, 
                       update_pvd->object_id);
        return status;
    }
    
    status = fbe_database_transaction_add_object_entry(fbe_database_service.transaction_ptr,
                                                       &new_pvd_entry);
    if ( status != FBE_STATUS_OK ){
        set_database_service_state(&fbe_database_service,FBE_DATABASE_STATE_FAILED); /* TODO: Is this correct state? */
    }

    return FBE_STATUS_OK;
}

fbe_status_t database_update_pvd_block_size(fbe_database_control_update_pvd_block_size_t  *update_pvd)
{
    database_object_entry_t *		existing_pvd_entry = NULL;
    database_object_entry_t         new_pvd_entry;
    fbe_status_t					status;   

    /* since only part of the data in table_entry is changed,
       copy the old data into the new entry, then modify the field we want. */
    /* first, let's get the existing pvd configuration from service object table*/
    fbe_spinlock_lock(&fbe_database_service.object_table.table_lock);
    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              update_pvd->object_id,
                                                              &existing_pvd_entry);
    if ( status != FBE_STATUS_OK ) {
        fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s Failed to find pvd %u in object table\n",
                     __FUNCTION__, 
                     update_pvd->object_id); 
        return status;
    }

    fbe_copy_memory(&new_pvd_entry, existing_pvd_entry,sizeof(database_object_entry_t));
    fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);
   

    new_pvd_entry.header.state = DATABASE_CONFIG_ENTRY_MODIFY;

    new_pvd_entry.set_config_union.pvd_config.configured_physical_block_size = update_pvd->configured_physical_block_size; 
    new_pvd_entry.set_config_union.pvd_config.update_type = FBE_UPDATE_PVD_CONFIG;


    status = fbe_database_update_pvd_block_size(new_pvd_entry.header.object_id, &new_pvd_entry);
    //status = database_common_update_config_from_table_entry(&new_pvd_entry);
    if (status != FBE_STATUS_OK) 
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to set config of object 0x%x\n",
                       __FUNCTION__, 
                       update_pvd->object_id);
        return status;
    }
    
    status = fbe_database_transaction_add_object_entry(fbe_database_service.transaction_ptr,
                                                       &new_pvd_entry);
    if ( status != FBE_STATUS_OK ){
        set_database_service_state(&fbe_database_service,FBE_DATABASE_STATE_FAILED); 
    }

    return FBE_STATUS_OK;
}

fbe_status_t database_update_encryption_mode(fbe_database_control_update_encryption_mode_t  *update_encryption_mode)
{
    database_object_entry_t *		existing_object_entry = NULL;
    database_object_entry_t         new_object_entry;
    fbe_status_t					status;   

    /* since only part of the data in table_entry is changed,
       copy the old data into the new entry, then modify the field we want. */
    /* first, let's get the existing pvd configuration from service object table*/
    fbe_spinlock_lock(&fbe_database_service.object_table.table_lock);
    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              update_encryption_mode->object_id,
                                                              &existing_object_entry);
    if ( status != FBE_STATUS_OK )
    {
        fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s Failed to find pvd %u in object table\n",
                     __FUNCTION__, 
                     update_encryption_mode->object_id); 
        return status;
    }
    fbe_copy_memory(&new_object_entry, existing_object_entry, sizeof(database_object_entry_t));
    fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);
   
    new_object_entry.base_config.encryption_mode = update_encryption_mode->base_config_encryption_mode;
       
    new_object_entry.header.state = DATABASE_CONFIG_ENTRY_MODIFY;

    status = database_common_update_encryption_mode_from_table_entry(&new_object_entry);
    if (status != FBE_STATUS_OK) 
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to set config of object 0x%x\n",
                       __FUNCTION__, 
                       update_encryption_mode->object_id);
        return status;
    }
    
    status = fbe_database_transaction_add_object_entry(fbe_database_service.transaction_ptr,
                                                       &new_object_entry);
    if ( status != FBE_STATUS_OK ){
        set_database_service_state(&fbe_database_service,FBE_DATABASE_STATE_FAILED); /* TODO: Is this correct state? */
    }

    return FBE_STATUS_OK;
}


fbe_status_t database_create_pvd(fbe_database_control_pvd_t * pvd)
{
    fbe_status_t                     status;
    database_object_entry_t          object_entry;
    database_user_entry_t            user_entry;
    fbe_u32_t                        size = 0;
    fbe_u32_t                        num_pvds_in_object_table = 0;
    fbe_u32_t                        num_pvds_in_transaction = 0;

    
    num_pvds_in_transaction = fbe_database_transaction_get_num_create_pvds(fbe_database_service.transaction_ptr);

    status = fbe_database_config_table_get_num_pvds(fbe_database_service.object_table.table_content.object_entry_ptr, 
                                                    fbe_database_service.object_table.table_size, 
                                                    &num_pvds_in_object_table);   
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get num pvds.  status:%d\n", __FUNCTION__, status);
        return status;
    }

    /* only add if there is room */
    if ((num_pvds_in_object_table + num_pvds_in_transaction) >= fbe_database_service.logical_objects_limits.platform_fru_count) {
        fbe_topology_control_get_physical_drive_by_sn_t  topology_search_sn;
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed. object table is full. max frus=%d \n",
                       __FUNCTION__, fbe_database_service.logical_objects_limits.platform_fru_count);

        /*first get the object id of pdo*/
        topology_search_sn.sn_size = sizeof(topology_search_sn.product_serial_num) - 1;
        fbe_copy_memory(topology_search_sn.product_serial_num, pvd->pvd_configurations.serial_num, topology_search_sn.sn_size);
        topology_search_sn.product_serial_num[topology_search_sn.sn_size] = '\0';
        topology_search_sn.object_id = FBE_OBJECT_ID_INVALID;

        status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_GET_PHYSICAL_DRIVE_BY_SERIAL_NUMBER, 
                                                     &topology_search_sn,
                                                     sizeof(topology_search_sn),
                                                     FBE_SERVICE_ID_TOPOLOGY,
                                                     NULL,  /* sg_list */
                                                     0,
                                                     FBE_PACKET_FLAG_EXTERNAL,
                                                     FBE_PACKAGE_ID_PHYSICAL);

        if(FBE_STATUS_OK != status || FBE_OBJECT_ID_INVALID == topology_search_sn.object_id)
        {
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO, 
                               "%s get pdo object id failed. sn:%s pvd:0x%x status:%d\n", 
                               __FUNCTION__, topology_search_sn.product_serial_num, pvd->object_id, status);
        }
        else
        {
            /* put request back on discovery queue. */
            fbe_database_add_drive_to_be_processed(&fbe_database_service.pvd_operation, topology_search_sn.object_id, FBE_LIFECYCLE_STATE_READY, DATABASE_PVD_DISCOVER_FLAG_NORMAL_PROCESS);        
        }
        
        return FBE_STATUS_OK;   /*returning OK so job service doesn't fail and roll back previously created pvds*/
    }

    /* add to transaction tables */
    database_common_init_object_entry(&object_entry);
    object_entry.header.object_id = pvd->object_id;
    object_entry.header.state = DATABASE_CONFIG_ENTRY_CREATE;
    object_entry.db_class_id = database_common_map_class_id_fbe_to_database(FBE_CLASS_ID_PROVISION_DRIVE);
    /* Initialize the size in version header*/
    status = database_common_get_committed_object_entry_size(object_entry.db_class_id, &size);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: get committed object entry size failed, size = %d \n",
                       __FUNCTION__, size);
        return status;
    }
    object_entry.header.version_header.size = size;
    if (database_common_cmi_is_active()){  /* set the generation number on the active side */
        pvd->pvd_configurations.generation_number = 
            fbe_database_transaction_get_next_generation_id(&fbe_database_service);
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Active side: Assigned generation number: 0x%llx for PVD.\n",
                       __FUNCTION__, 
                       (unsigned long long)pvd->pvd_configurations.generation_number);
    }

    fbe_copy_memory(&object_entry.set_config_union.pvd_config, &pvd->pvd_configurations, 
        sizeof(fbe_provision_drive_configuration_t));

    /* object id is assigned by the following function, so must call before add it to transaction table */
    status = database_common_create_object_from_table_entry(&object_entry);
    pvd->object_id = object_entry.header.object_id;  /* now set the object_id that is returned to caller */
    /* Initialize PVD configuration */
    status = database_common_set_config_from_table_entry(&object_entry);

    status = fbe_database_transaction_add_object_entry(fbe_database_service.transaction_ptr, &object_entry);

    /* Initiate and Initialize the user entries for PVD */
    database_common_init_user_entry(&user_entry);
    user_entry.header.object_id = pvd->object_id;
    user_entry.header.state = DATABASE_CONFIG_ENTRY_CREATE;
    user_entry.db_class_id = database_common_map_class_id_fbe_to_database(FBE_CLASS_ID_PROVISION_DRIVE);
    /* Initialize the size in version header*/
    status = database_common_get_committed_user_entry_size(user_entry.db_class_id, &size);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: get committed user entry size failed, size = %d \n",
                       __FUNCTION__, size);
        return status;
    }
    user_entry.header.version_header.size = size;
    user_entry.user_data_union.pvd_user_data.pool_id = FBE_POOL_ID_INVALID;
    fbe_zero_memory(user_entry.user_data_union.pvd_user_data.opaque_data, FBE_DATABASE_PROVISION_DRIVE_OPAQUE_DATA_MAX);   
    status = fbe_database_transaction_add_user_entry(fbe_database_service.transaction_ptr, &user_entry);

    return status;
}

fbe_status_t database_destroy_pvd(fbe_database_control_destroy_object_t * destroy_pvd,
                                  fbe_bool_t                            confirm_peer)
{
    database_object_entry_t                    object_entry;
    database_object_entry_t*                   existing_object_entry = NULL;
    database_user_entry_t                      user_entry;
    database_user_entry_t*                     existing_user_entry = NULL;
    fbe_status_t                               status;
    fbe_database_cmi_msg_t *                   msg_memory;

    fbe_spinlock_lock(&fbe_database_service.object_table.table_lock);
    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              destroy_pvd->object_id,
                                                              &existing_object_entry);
    if (status != FBE_STATUS_OK )
    {
        fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s Failed to find object %d in object table\n",
                     __FUNCTION__,destroy_pvd->object_id);
        return status;
    }
    fbe_copy_memory(&object_entry, existing_object_entry,sizeof(database_object_entry_t));
    fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);

    status = database_common_destroy_object_from_table_entry(&object_entry);

    if(confirm_peer)
    {
        msg_memory = fbe_database_cmi_get_msg_memory();
        if (msg_memory == NULL) 
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to get CMI msg memory\n", __FUNCTION__);
        }
        else
        {
            /*we'll return the status as a handshake to the master*/
            msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_DESTROY_PVD_CONFIRM;
            msg_memory->completion_callback = fbe_database_config_change_request_from_peer_completion;
            msg_memory->payload.transaction_confirm.status = status;

            fbe_database_cmi_send_message(msg_memory, NULL);
        }
    }

    status = database_common_wait_destroy_object_complete();
   
    object_entry.header.state = DATABASE_CONFIG_ENTRY_DESTROY;

    status = fbe_database_transaction_add_object_entry(fbe_database_service.transaction_ptr, 
                                                       &object_entry);

    fbe_spinlock_lock(&fbe_database_service.user_table.table_lock);
    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                              destroy_pvd->object_id,
                                                              &existing_user_entry);
    if (status != FBE_STATUS_OK )
    {
        fbe_spinlock_unlock(&fbe_database_service.user_table.table_lock);
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s Failed to find object %d in user table\n",
                     __FUNCTION__,destroy_pvd->object_id);
        return status;
    }
    fbe_copy_memory(&user_entry, existing_user_entry,sizeof(database_user_entry_t));
    fbe_spinlock_unlock(&fbe_database_service.user_table.table_lock);

    user_entry.header.state = DATABASE_CONFIG_ENTRY_DESTROY;
    status = fbe_database_transaction_add_user_entry(fbe_database_service.transaction_ptr, 
                                                     &user_entry);

    database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Destroy PVD transaction 0x%llx returned 0x%x.\n",
        __FUNCTION__, (unsigned long long)destroy_pvd->transaction_id, status);

    return status;
}

fbe_status_t database_create_vd(fbe_database_control_vd_t * vd)
{
    fbe_status_t						status;
    fbe_u32_t							size = 0;
    database_object_entry_t				object_entry;
    fbe_system_encryption_mode_t		system_encryption_mode;
    fbe_base_config_encryption_mode_t   base_config_encryption_mode;

    /* add to transaction tables */
    database_common_init_object_entry(&object_entry);
    object_entry.header.object_id = vd->object_id;
    object_entry.header.state = DATABASE_CONFIG_ENTRY_CREATE;
    object_entry.db_class_id = database_common_map_class_id_fbe_to_database(FBE_CLASS_ID_VIRTUAL_DRIVE);
    /* Initialize the size in version header*/
    status = database_common_get_committed_object_entry_size(object_entry.db_class_id, &size);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: get committed object entry size failed, size = %d \n",
                       __FUNCTION__, size);
        return status;
    }
    object_entry.header.version_header.size = size;
    if (database_common_cmi_is_active()){  /* set the generation number on the active side */
        vd->configurations.generation_number = 
            fbe_database_transaction_get_next_generation_id(&fbe_database_service);
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Active side: Assigned generation number: 0x%x for VD.\n",
                       __FUNCTION__, 
                       (unsigned int)vd->configurations.generation_number);
    }
    fbe_copy_memory(&object_entry.set_config_union.vd_config, &vd->configurations, 
        sizeof(fbe_database_control_vd_set_configuration_t));

    status = fbe_database_get_system_encryption_mode(&system_encryption_mode);
    /* Temporary hack */
    base_config_encryption_mode = system_encryption_mode;

    object_entry.base_config.encryption_mode = base_config_encryption_mode;

    /* object id is assigned by the following function, so must call before add it to transaction table */
    status = database_common_create_object_from_table_entry(&object_entry);
    status = database_common_update_encryption_mode_from_table_entry(&object_entry);
    vd->object_id = object_entry.header.object_id;  /* now set the object_id that is returned to caller */
    /* Initialize VD configuration */
    status = database_common_set_config_from_table_entry(&object_entry);

    status = fbe_database_transaction_add_object_entry(fbe_database_service.transaction_ptr, &object_entry);

    return status;
}

fbe_status_t database_update_vd(fbe_database_control_update_vd_t *update_vd)
{
    fbe_status_t                              status;
    database_object_entry_t                  *existing_vd_entry;
    database_object_entry_t                  new_vd_entry;

    /* first, let's get the existing vd configuration */
    fbe_spinlock_lock(&fbe_database_service.object_table.table_lock);
    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              update_vd->object_id,
                                                              &existing_vd_entry);
    fbe_copy_memory(&new_vd_entry, existing_vd_entry,sizeof(database_object_entry_t));
    // we may need to hold this lock longer
    fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);
          
    if ( status == FBE_STATUS_OK )
    {
        switch (update_vd->update_type)
        {
        case FBE_UPDATE_VD_MODE:
            new_vd_entry.set_config_union.vd_config.configuration_mode = update_vd->configuration_mode;
            break;
        default:
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: Illegal vd configuration update type: 0x%X\n",
                           __FUNCTION__, update_vd->update_type);
            status = FBE_STATUS_GENERIC_FAILURE;
        }
        new_vd_entry.header.state = DATABASE_CONFIG_ENTRY_MODIFY;
        /*copy update type to vd*/
        new_vd_entry.set_config_union.vd_config.update_vd_type = update_vd->update_type;

        status = database_common_update_config_from_table_entry(&new_vd_entry);
        if (status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: failed to update vd type: %d mode: %d obj: 0x%x - status: 0x%x\n",
                           __FUNCTION__, update_vd->update_type, update_vd->configuration_mode,
                           update_vd->object_id, status);
            return status;
        }

        status = fbe_database_transaction_add_object_entry(fbe_database_service.transaction_ptr, 
                                                           &new_vd_entry);
        if (status != FBE_STATUS_OK)
        {
            set_database_service_state(&fbe_database_service,FBE_DATABASE_STATE_FAILED); // Is this correct state?                
        } 
    }

    return status;
}

fbe_status_t database_destroy_vd(fbe_database_control_destroy_object_t *vd,
                                 fbe_bool_t                            confirm_peer)
{
    database_object_entry_t                    object_entry;
    database_object_entry_t*                   existing_object_entry = NULL;
    fbe_status_t                               status;
    fbe_database_cmi_msg_t *                   msg_memory;  

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                     "%s entry\n",__FUNCTION__);

    /*add to object entry transaction tables */
    fbe_spinlock_lock(&fbe_database_service.object_table.table_lock);
    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              vd->object_id,
                                                              &existing_object_entry);
    if (status != FBE_STATUS_OK )
    {
        fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s Failed to find object %d in object table\n",
                     __FUNCTION__,vd->object_id);
        return status;
    }
    fbe_copy_memory(&object_entry, existing_object_entry,sizeof(database_object_entry_t));
    fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);
     
    status = database_common_destroy_object_from_table_entry(&object_entry);

    if(confirm_peer)
    {
        msg_memory = fbe_database_cmi_get_msg_memory();
        if (msg_memory == NULL) 
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to get CMI msg memory\n", __FUNCTION__);
        }
        else
        {
            /*we'll return the status as a handshake to the master*/
            msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_DESTROY_VD_CONFIRM;
            msg_memory->completion_callback = fbe_database_config_change_request_from_peer_completion;
            msg_memory->payload.transaction_confirm.status = status;

            fbe_database_cmi_send_message(msg_memory, NULL);
        }
    }

    status = database_common_wait_destroy_object_complete();

    
    object_entry.header.state = DATABASE_CONFIG_ENTRY_DESTROY;
    
    status = fbe_database_transaction_add_object_entry(fbe_database_service.transaction_ptr, 
                                                       &object_entry);
    /*Add egde entries from service table to transaction table*/	
    status = fbe_database_config_table_copy_edge_entries(&fbe_database_service.edge_table,
                                                         vd->object_id,
                                                         fbe_database_service.transaction_ptr);
    if(status == FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Destroy VD transaction 0x%llx returned 0x%x.\n",
            __FUNCTION__, (unsigned long long)vd->transaction_id, status);
    }

    return status;

}

fbe_status_t database_create_raid(fbe_database_control_raid_t * create_raid)
{
    database_object_entry_t           object_entry;
    database_user_entry_t             user_entry;
    fbe_status_t                      status;
    fbe_u32_t                         size = 0;
    fbe_system_encryption_mode_t		system_encryption_mode;
    fbe_base_config_encryption_mode_t   base_config_encryption_mode;

    /*update transaction user and object tables */
    database_common_init_object_entry(&object_entry);
    object_entry.header.object_id = create_raid->object_id;
    object_entry.header.state = DATABASE_CONFIG_ENTRY_CREATE;
    object_entry.db_class_id = database_common_map_class_id_fbe_to_database(create_raid->class_id);
    /* Initialize the size in version header*/
    status = database_common_get_committed_object_entry_size(object_entry.db_class_id, &size);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: get committed object entry size failed, size = %d \n",
                       __FUNCTION__, size);
        return status;
    }
    object_entry.header.version_header.size = size;
    if (database_common_cmi_is_active()){  /* set the generation number on the active side */
        create_raid->raid_configuration.generation_number = 
            fbe_database_transaction_get_next_generation_id(&fbe_database_service);
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Active side: Assigned generation number: 0x%llx for RG.\n",
                       __FUNCTION__, 
                       (unsigned long long)create_raid->raid_configuration.generation_number);
    }

    fbe_copy_memory(&object_entry.set_config_union.rg_config, &create_raid->raid_configuration, 
        sizeof(fbe_raid_group_configuration_t));

    /*the upper layers has the option not to assigne the RG number so we have to do that*/
    if (database_common_cmi_is_active() && create_raid->raid_id == FBE_RAID_ID_INVALID && !create_raid->private_raid) {
        status = database_get_next_available_rg_number(&create_raid->raid_id, object_entry.db_class_id);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO, "%s: can't find a free rg number\n", __FUNCTION__);
            return status;

        }else{
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO, "%s: picked up RG number %d for user\n", __FUNCTION__, create_raid->raid_id);
        }
    }

    /* object id is assigned by the following function, so must call before add it to transaction table */
    status = database_common_create_object_from_table_entry(&object_entry);
    create_raid->object_id = object_entry.header.object_id;  /* now set the object_id that is returned to caller */
    /*Initialize RG configuration*/
    status = database_common_set_config_from_table_entry(&object_entry);

    status = fbe_database_get_system_encryption_mode(&system_encryption_mode);
    /* Temporary hack */
    base_config_encryption_mode = system_encryption_mode;

    object_entry.base_config.encryption_mode= base_config_encryption_mode;

    status = database_common_update_encryption_mode_from_table_entry(&object_entry);
    status = fbe_database_transaction_add_object_entry(fbe_database_service.transaction_ptr, 
        &object_entry);

    database_common_init_user_entry(&user_entry);
    user_entry.header.object_id = create_raid->object_id;
    user_entry.header.state = DATABASE_CONFIG_ENTRY_CREATE;
    user_entry.db_class_id = database_common_map_class_id_fbe_to_database(create_raid->class_id);
    /* Initialize the size in version header*/
    status = database_common_get_committed_user_entry_size(user_entry.db_class_id, &size);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: get committed user entry size failed, size = %d \n",
                       __FUNCTION__, size);
        return status;
    }
    user_entry.header.version_header.size = size;
    user_entry.user_data_union.rg_user_data.is_system = create_raid->private_raid;
    user_entry.user_data_union.rg_user_data.raid_group_number = create_raid->raid_id;
    user_entry.user_data_union.rg_user_data.user_private = create_raid->user_private;
    status = fbe_database_transaction_add_user_entry(fbe_database_service.transaction_ptr, 
        &user_entry);

    /* Log message to the event-log when DB has already added a RG to the Table, and not logging it at the RG
     * create commit time.  We cannot write to the Event Log in CMI thread.
     */
    status = fbe_event_log_write(SEP_INFO_RAID_GROUP_CREATED, NULL, 0, "%x %d",
                    create_raid->object_id, 
                    create_raid->raid_id); 

    database_trace_rg_user_info(&user_entry);

    return status;
}

fbe_status_t database_destroy_raid(fbe_database_control_destroy_object_t * destroy_raid,
                                   fbe_bool_t                            confirm_peer)
{
    database_object_entry_t                    object_entry;
    database_user_entry_t                      user_entry;	
    fbe_status_t                               status;
    database_user_entry_t *                    user_entry_ptr;
    fbe_database_cmi_msg_t *                   msg_memory;    
    database_object_entry_t*                   existing_object_entry = NULL;
    database_user_entry_t*                      existing_user_entry = NULL;
    fbe_database_control_remove_encryption_keys_t encryption_info;
    fbe_system_encryption_mode_t               encryption_mode;


     /* CBE We need to  clean up the keys here.
        It is temporary solution.
        In general we should not destroy the Raid group untill keys removed from key manager 
    */
    //fbe_database_invalidate_key(destroy_raid->object_id);

    /*let' makr the lun number is valid to be reused by other users if they wish to*/
    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table, destroy_raid->object_id, &user_entry_ptr);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /*update transaction user and object tables */	  
    fbe_spinlock_lock(&fbe_database_service.object_table.table_lock);
    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              destroy_raid->object_id,
                                                              &existing_object_entry);
    if (status != FBE_STATUS_OK )
    {
        fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s Failed to find object %d in object table\n",
                     __FUNCTION__,destroy_raid->object_id);
        return status;
    }
    fbe_copy_memory(&object_entry, existing_object_entry,sizeof(database_object_entry_t));
    fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);

    status = database_common_destroy_object_from_table_entry(&object_entry);

    if(confirm_peer)
    {
        msg_memory = fbe_database_cmi_get_msg_memory();

        if (msg_memory == NULL) 
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to get CMI msg memory\n", __FUNCTION__);
        }
        else
        {
            /*we'll return the status as a handshake to the master*/
            msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_DESTROY_RAID_CONFIRM;
            msg_memory->completion_callback = fbe_database_config_change_request_from_peer_completion;
            msg_memory->payload.transaction_confirm.status = status;

            fbe_database_cmi_send_message(msg_memory, NULL);
        }
    }

    /* Delete encryption key for passive side */
    fbe_database_get_system_encryption_mode(&encryption_mode);
    if (!database_common_cmi_is_active() && (encryption_mode == FBE_SYSTEM_ENCRYPTION_MODE_ENCRYPTED))
    {
        encryption_info.object_id = destroy_raid->object_id;
        database_remove_encryption_keys(&encryption_info);
    }

    status = database_common_wait_destroy_object_complete();
    
    object_entry.header.state = DATABASE_CONFIG_ENTRY_DESTROY;

    status = fbe_database_transaction_add_object_entry(fbe_database_service.transaction_ptr, 
                                                       &object_entry);


    fbe_spinlock_lock(&fbe_database_service.user_table.table_lock);
    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                              destroy_raid->object_id,
                                                              &existing_user_entry);
    if (status != FBE_STATUS_OK )
    {
        fbe_spinlock_unlock(&fbe_database_service.user_table.table_lock);
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s Failed to find object %d in user table\n",
                     __FUNCTION__,destroy_raid->object_id);
        return status;
    }
    fbe_copy_memory(&user_entry, existing_user_entry,sizeof(database_user_entry_t));
    fbe_spinlock_unlock(&fbe_database_service.user_table.table_lock);

    user_entry.header.state = DATABASE_CONFIG_ENTRY_DESTROY;
    status = fbe_database_transaction_add_user_entry(fbe_database_service.transaction_ptr, 
                                                     &user_entry);

    /*For this RG object id copy entires for all edges from Service table to Transaction table */
  
    status = fbe_database_config_table_copy_edge_entries(&fbe_database_service.edge_table,
                                                         destroy_raid->object_id,
                                                         fbe_database_service.transaction_ptr);
    
    database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Destroy RG transaction 0x%x returned 0x%x.\n",
        __FUNCTION__, (unsigned int)destroy_raid->transaction_id, status);

    /* Log message to the event-log when DB has already added a RG to the Table, and not logging it at the RG
     * destroy commit time.  We cannot write to the Event Log in CMI thread.
     */
    status = fbe_event_log_write(SEP_INFO_RAID_GROUP_DESTROYED, NULL, 0, "%x %d",
                    destroy_raid->object_id, 
                    user_entry.user_data_union.rg_user_data.raid_group_number); 

    return status;

}

fbe_status_t database_update_raid(fbe_database_control_update_raid_t *update_raid)
{
    fbe_status_t                        status;
    database_object_entry_t *			existing_raid_entry;
    database_object_entry_t             new_raid_entry;
    database_edge_entry_t *				existing_edge_entry;
    database_edge_entry_t *				new_edge_entry = NULL, *tmp_edge_entry = NULL;
    fbe_u32_t							edge_index, valid_edges = 0;

    /* we don't want to allocate memory under spinlock */
    if (update_raid->update_type == FBE_UPDATE_RAID_TYPE_EXPAND_RG) {

        new_edge_entry = fbe_allocate_nonpaged_pool_with_tag(sizeof(database_edge_entry_t) * DATABASE_MAX_EDGE_PER_OBJECT, 'ASBD');
        if (new_edge_entry == NULL) {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: can't allocate memory for edges\n", __FUNCTION__);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        fbe_zero_memory(new_edge_entry, sizeof(database_edge_entry_t) * DATABASE_MAX_EDGE_PER_OBJECT);
    }

    /* first, let's get the existing raid configuration */
    fbe_spinlock_lock(&fbe_database_service.object_table.table_lock);
    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              update_raid->object_id,
                                                              &existing_raid_entry);

    fbe_copy_memory(&new_raid_entry, existing_raid_entry,sizeof(database_object_entry_t));
    
    // we may need to hold this lock longer
    fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);

    if (update_raid->update_type == FBE_UPDATE_RAID_TYPE_EXPAND_RG) {

        /* FIXME 0: needed for expansion of non-system RGs 
           For expansion of system RGs, the update_raid request comes in
           with new_edge_capacity already filled out, because the PSL
           library tells us what it should be. If new_edge_capacity
           isn't filled out, we need to send a control packet down to
           the RAID group so that it can tell us how large its edges
           need to be for the new RG capacity.
        */

        tmp_edge_entry = new_edge_entry;

        fbe_spinlock_lock(&fbe_database_service.edge_table.table_lock);

        for (edge_index = 0; edge_index < DATABASE_MAX_EDGE_PER_OBJECT; edge_index++) {
        
            fbe_database_config_table_get_edge_entry(&fbe_database_service.edge_table,
                                                     update_raid->object_id,
                                                     edge_index,
                                                     &existing_edge_entry);

            if (existing_edge_entry->header.state == DATABASE_CONFIG_ENTRY_VALID) {
                existing_edge_entry->capacity = update_raid->new_edge_capacity; /*in the table now*/
                fbe_copy_memory(tmp_edge_entry, existing_edge_entry, sizeof(database_edge_entry_t));
            }else{
                break;
            }

            tmp_edge_entry++;
            valid_edges++;
        }
        fbe_spinlock_unlock(&fbe_database_service.edge_table.table_lock);
    }
        
    if ( status == FBE_STATUS_OK )
    {
        switch (update_raid->update_type)
        {
            case FBE_UPDATE_RAID_TYPE_DISABLE_POWER_SAVE:

                 new_raid_entry.set_config_union.rg_config.power_saving_enabled = FBE_FALSE;	                  
                 break;

            case FBE_UPDATE_RAID_TYPE_ENABLE_POWER_SAVE:

                 new_raid_entry.set_config_union.rg_config.power_saving_enabled = FBE_TRUE;	                  
                 break;                

            case FBE_UPDATE_RAID_TYPE_CHANGE_IDLE_TIME:

                 new_raid_entry.set_config_union.rg_config.power_saving_idle_time_in_seconds = update_raid->power_save_idle_time_in_sec;
                 
                 break;

            case FBE_UPDATE_RAID_TYPE_EXPAND_RG:

                 new_raid_entry.set_config_union.rg_config.capacity = update_raid->new_rg_size;	
                 
                 /*FIXME 1 - Needed for expansion of non-system RGs */
                 /*For R10 we need to work harder: find the mirros under us and update them in the table as well.
                 This means we need to create a new new_raid_entry here fgor the mirrors and add them to the updated table
                 The RG itself will also have to work harder if it's a striper since it will have to send the expansion down*/

                 /*FIXME 2 - Error path */
                 /*If we are not able to pesist and then we roll back, we need to undo this line from above:

                   existing_edge_entry->capacity = update_raid->new_edge_capacity;
                 */

                 /*FIXME 3 - Normal path */
                 /*need to send all the new edges over CMI to the peer as well since they are now only in memory and on the way to the disk but not on the peer.
                 similar to what we do in function database_control_create_edge when we create the edge locally and then send to the peer
                 */

                 break;

                default:
                    database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                 "%s: Illegal raid update type: 0x%X\n", 
                                 __FUNCTION__, 
                                 update_raid->update_type);
            
                    status = FBE_STATUS_GENERIC_FAILURE;
        }
        new_raid_entry.header.state = DATABASE_CONFIG_ENTRY_MODIFY;
        /*copy the update type*/
        new_raid_entry.set_config_union.rg_config.update_type = update_raid->update_type;
        status = database_common_update_config_from_table_entry(&new_raid_entry);
        if (status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_WARNING, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: failed to update raid type: %d pwr save idle time: 0x%lld obj: 0x%x - status: 0x%x\n",
                           __FUNCTION__, update_raid->update_type, 
                           (unsigned long long)update_raid->power_save_idle_time_in_sec,
                           update_raid->object_id, status);

            if (new_edge_entry != NULL) {
                fbe_release_nonpaged_pool_with_tag(new_edge_entry, 'ASBD');
            }

            return status;
        }

        status = fbe_database_transaction_add_object_entry(fbe_database_service.transaction_ptr, 
                 &new_raid_entry);
        if (status != FBE_STATUS_OK)
        {
            set_database_service_state(&fbe_database_service,FBE_DATABASE_STATE_FAILED); // Is this correct state?                
            if (new_edge_entry != NULL) {
                fbe_release_nonpaged_pool_with_tag(new_edge_entry, 'ASBD');
            }

            return FBE_STATUS_GENERIC_FAILURE;
        }

        /*for the edge change*/
        if (update_raid->update_type == FBE_UPDATE_RAID_TYPE_EXPAND_RG) {

            for (edge_index = 0; edge_index < valid_edges; edge_index++) {

                new_edge_entry->capacity = update_raid->new_edge_capacity;/*going to persistance soon*/
                status = fbe_database_transaction_add_edge_entry(fbe_database_service.transaction_ptr, 
                         new_edge_entry);

                if (status != FBE_STATUS_OK){
                    set_database_service_state(&fbe_database_service,FBE_DATABASE_STATE_FAILED); // Is this correct state?    
                    if (new_edge_entry != NULL) {
                        fbe_release_nonpaged_pool_with_tag(new_edge_entry, 'ASBD');
                    }

                    return FBE_STATUS_GENERIC_FAILURE;
                }
            }
        }
    }

    if (new_edge_entry != NULL) {
        fbe_release_nonpaged_pool_with_tag(new_edge_entry, 'ASBD');
    }

    return status;
}

fbe_status_t database_create_lun(fbe_database_control_lun_t* create_lun)
{
    database_object_entry_t           object_entry;
    database_user_entry_t             user_entry;
    fbe_status_t                      status;
    fbe_u32_t                         size = 0;
    
    /*update transaction user and object tables */
    database_common_init_object_entry(&object_entry);
    object_entry.header.object_id = create_lun->object_id;
    object_entry.header.state = DATABASE_CONFIG_ENTRY_CREATE;
    object_entry.db_class_id = database_common_map_class_id_fbe_to_database(FBE_CLASS_ID_LUN);
    /* Initialize the size in version header*/
    status = database_common_get_committed_object_entry_size(object_entry.db_class_id, &size);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: get committed object entry size failed, size = %d \n",
                       __FUNCTION__, size);
        return status;
    }
    object_entry.header.version_header.size = size;

    if (database_common_cmi_is_active()){  /* set the generation number on the active side */
        /* Because we allow recreate system LUNs, so we should use the default generation_number for system LUN */
        if (fbe_private_space_layout_object_id_is_system_lun(create_lun->object_id)) {
            create_lun->lun_set_configuration.generation_number = create_lun->object_id;
        } else {
            create_lun->lun_set_configuration.generation_number = 
                fbe_database_transaction_get_next_generation_id(&fbe_database_service);
        }
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Active side: Assigned generation number: 0x%llx for LUN.\n",
                       __FUNCTION__, 
                       (unsigned long long)create_lun->lun_set_configuration.generation_number);
    }

    fbe_copy_memory(&object_entry.set_config_union.lu_config, &create_lun->lun_set_configuration, 
        sizeof(fbe_database_lun_configuration_t));

    /*the upper layers has the option not to assigne the LUN number so we have to do that
    We do it on the active side only becase by the time it gets to the passive side, we would
    have already populated the number*/
    if (database_common_cmi_is_active() && create_lun->lun_number == FBE_LUN_ID_INVALID) {
        status = database_get_next_available_lun_number(&create_lun->lun_number);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: can't find a free lun number\n",__FUNCTION__);
            return status;

        }else{
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s: picked up lun number %d for user\n",__FUNCTION__, create_lun->lun_number);

        }
    }

    /* object id is assigned by the following function, so must call before add it to transaction table */
    status = database_common_create_object_from_table_entry(&object_entry);
    create_lun->object_id = object_entry.header.object_id;  /* now set the object_id that is returned to caller */
    status = database_common_set_config_from_table_entry(&object_entry);

    status = fbe_database_transaction_add_object_entry(fbe_database_service.transaction_ptr, 
        &object_entry);

    database_common_init_user_entry(&user_entry);
    user_entry.header.object_id = create_lun->object_id;
    user_entry.header.state = DATABASE_CONFIG_ENTRY_CREATE;
    user_entry.db_class_id = database_common_map_class_id_fbe_to_database(FBE_CLASS_ID_LUN);
    /* Initialize the size in version header*/
    status = database_common_get_committed_user_entry_size(user_entry.db_class_id, &size);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: get committed user entry size failed, size = %d \n",
                       __FUNCTION__, size);
        return status;
    }
    user_entry.header.version_header.size = size;

    user_entry.user_data_union.lu_user_data.export_device_b = FBE_TRUE;
    user_entry.user_data_union.lu_user_data.lun_number = create_lun->lun_number;
    user_entry.user_data_union.lu_user_data.world_wide_name = create_lun->world_wide_name;
    user_entry.user_data_union.lu_user_data.user_defined_name = create_lun->user_defined_name;
    user_entry.user_data_union.lu_user_data.bind_time = create_lun->bind_time;
    user_entry.user_data_union.lu_user_data.user_private = create_lun->user_private;
    status = fbe_database_transaction_add_user_entry(fbe_database_service.transaction_ptr, 
        &user_entry);

    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: failed to add LUN#:%d to user entry.\n",
                       __FUNCTION__, create_lun->lun_number);
        return status;        
    }

    /* Log message to the event-log when DB has already added a LUN to the Table, and not logging it at the LUN
     * create commit time.  We cannot write to the Event Log in CMI thread.
     */
    status = fbe_event_log_write(SEP_INFO_LUN_CREATED, NULL, 0, "%x %d",
                    create_lun->object_id, 
                    create_lun->lun_number); 

    database_trace_lun_user_info(&user_entry);

    return status;

}

fbe_status_t database_destroy_lun(fbe_database_control_destroy_object_t * destroy_lun,
                                  fbe_bool_t                            confirm_peer)
{
    database_object_entry_t                    object_entry;
    database_user_entry_t                      user_entry;
    fbe_status_t                               status;
    database_user_entry_t *                    user_entry_ptr;
    fbe_u32_t                                  lun_number;  
    fbe_database_cmi_msg_t *                   msg_memory;   
    database_object_entry_t*                   existing_object_entry = NULL;
    database_user_entry_t*                     existing_user_entry = NULL;

    /*let' makr the lun number is valid to be reused by other users if they wish to*/
    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table, destroy_lun->object_id, &user_entry_ptr);
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    /*update transaction user and object tables */	  
    fbe_spinlock_lock(&fbe_database_service.object_table.table_lock);
    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              destroy_lun->object_id,
                                                              &existing_object_entry);
    if (status != FBE_STATUS_OK )
    {
        fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s Failed to find object %d in object table\n",
                     __FUNCTION__,destroy_lun->object_id);
        return status;
    }
    fbe_copy_memory(&object_entry, existing_object_entry,sizeof(database_object_entry_t));
    fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);

    status = database_common_destroy_object_from_table_entry(&object_entry);

    if(confirm_peer)
    {
        msg_memory = fbe_database_cmi_get_msg_memory();
        if (msg_memory == NULL) 
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to get CMI msg memory\n", __FUNCTION__);
        }
        else
        {
            /*we'll return the status as a handshake to the master*/
            msg_memory->msg_type = FBE_DATABE_CMI_MSG_TYPE_DESTROY_LUN_CONFIRM;
            msg_memory->completion_callback = fbe_database_config_change_request_from_peer_completion;
            msg_memory->payload.transaction_confirm.status = status;

            fbe_database_cmi_send_message(msg_memory, NULL);
        }
    }

    status = database_common_wait_destroy_object_complete();
    
    object_entry.header.state = DATABASE_CONFIG_ENTRY_DESTROY;

    status = fbe_database_transaction_add_object_entry(fbe_database_service.transaction_ptr, 
                                                       &object_entry);

    fbe_spinlock_lock(&fbe_database_service.user_table.table_lock);
    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                              destroy_lun->object_id,
                                                              &existing_user_entry);
    if (status != FBE_STATUS_OK )
    {
        fbe_spinlock_unlock(&fbe_database_service.user_table.table_lock);
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s Failed to find object %d in user table\n",
                     __FUNCTION__,destroy_lun->object_id);
        return status;
    }
    fbe_copy_memory(&user_entry, existing_user_entry,sizeof(database_user_entry_t));
    fbe_spinlock_unlock(&fbe_database_service.user_table.table_lock);

    user_entry.header.state = DATABASE_CONFIG_ENTRY_DESTROY;
    status = fbe_database_transaction_add_user_entry(fbe_database_service.transaction_ptr, 
                                                     &user_entry);

    /*For this RG object id copy entires for all edges from Service table to Transaction table */
    status = fbe_database_config_table_copy_edge_entries(&fbe_database_service.edge_table,
                                                         destroy_lun->object_id,
                                                         fbe_database_service.transaction_ptr);

    /* get LUN number based on object ID */
    fbe_database_get_lun_number(destroy_lun->object_id, &lun_number);

    /* Log message to the event-log when DB has already updated a LUN to the Table, and not logging it at the LUN
     * destroy commit time.  We cannot write to the Event Log in CMI thread.
     */
    status = fbe_event_log_write(SEP_INFO_LUN_DESTROYED, NULL, 0, "%d",                                         
                                 lun_number);  

    database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Destroy LUN#:%d, obj: 0x%x, trans=0x%llx, sts=0x%x.\n",
        __FUNCTION__, lun_number, destroy_lun->object_id,
    (unsigned long long)destroy_lun->transaction_id, status);

    return status;
}


fbe_status_t database_update_lun(fbe_database_control_update_lun_t *update_lun)
{
    fbe_status_t                       status;
    database_user_entry_t             *existing_lun_entry;    
    database_user_entry_t              new_lun_entry;   

    fbe_spinlock_lock(&fbe_database_service.user_table.table_lock);
    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                                   update_lun->object_id,
                                                                   &existing_lun_entry);
    // we may need to hold this lock longer
    fbe_spinlock_unlock(&fbe_database_service.user_table.table_lock);

    fbe_copy_memory(&new_lun_entry, existing_lun_entry,sizeof(database_user_entry_t));

    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s Failed to find lun %u in tables\n",
                     __FUNCTION__, 
                     update_lun->object_id); 
        return status;
    }
    
    /*new_lun_entry.user_data_union.lu_user_data.world_wide_name = update_lun->world_wide_name;   */
    switch(update_lun->update_type)
    {
        case FBE_LUN_UPDATE_WWN:
            fbe_copy_memory(&new_lun_entry.user_data_union.lu_user_data.world_wide_name, 
                            &update_lun->world_wide_name, sizeof(update_lun->world_wide_name));
            break;
        case FBE_LUN_UPDATE_UDN:
            fbe_copy_memory(&new_lun_entry.user_data_union.lu_user_data.user_defined_name, 
                            &update_lun->user_defined_name, sizeof(update_lun->user_defined_name));
            break;
        case FBE_LUN_UPDATE_ATTRIBUTES:
            new_lun_entry.user_data_union.lu_user_data.attributes |= update_lun->attributes;
            break;
        default:
            break;
    }
    
    new_lun_entry.header.state = DATABASE_CONFIG_ENTRY_MODIFY;
     
    status = fbe_database_transaction_add_user_entry(fbe_database_service.transaction_ptr, 
             &new_lun_entry);

    if ( status != FBE_STATUS_OK )
    {
        set_database_service_state(&fbe_database_service,FBE_DATABASE_STATE_FAILED); // Is this correct state?                
    }

    database_trace_lun_user_info(&new_lun_entry);

    return status;
}

fbe_status_t database_clone_object(fbe_database_control_clone_object_t * clone_object)
{
    fbe_status_t status;
    status = fbe_database_clone_object(clone_object->src_object_id, &(clone_object->des_object_id));
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Clone object failed!! 0x%x.\n",
                       __FUNCTION__, status);
    }
    return status;
}

fbe_status_t database_create_edge(fbe_database_control_create_edge_t * create_edge)
{
    database_edge_entry_t                    edge_entry;
    fbe_status_t                             status;
    fbe_u32_t                                size = 0;
    database_object_entry_t                *object_entry = NULL;
    fbe_database_physical_drive_info_t              pdo_info;
    
    /*get the object entry from object table*/
    fbe_spinlock_lock(&fbe_database_service.object_table.table_lock);
    
    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table, create_edge->object_id, &object_entry);
    if (status != FBE_STATUS_OK)
    {
        fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);
        database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: get object entry failed!! 0x%x.\n",
            __FUNCTION__, status);
        return status;
    }
     fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);

    /*If this is a provision drive object*/
    if(object_entry->header.state !=  DATABASE_CONFIG_ENTRY_INVALID 
        && object_entry->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE)
    {                                
        /*connect by serial number rather than server_id in create_edge because
      * the object id in physical package may be not same between two sps*/
        status = fbe_database_connect_to_pdo_by_serial_number(create_edge->object_id,
                                                    create_edge->serial_num,
                                                    &pdo_info);
        if (status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                     FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: create edge for PVD 0x%x failed!! 0x%x.\n",
                     __FUNCTION__, create_edge->object_id,status);
        }

        return status;
    }

    /*update transaction edge table */
    database_common_init_edge_entry(&edge_entry);
    edge_entry.header.object_id = create_edge->object_id;
    edge_entry.header.state = DATABASE_CONFIG_ENTRY_CREATE;
    status = database_common_get_committed_edge_entry_size(&size);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: get committed edge entry size failed, size = %d \n",
                       __FUNCTION__, size);
        return status;
    }
    edge_entry.header.version_header.size = size;

    edge_entry.capacity = create_edge->capacity;
    edge_entry.client_index = create_edge->client_index;
    edge_entry.offset = create_edge->offset;
    edge_entry.server_id = create_edge->server_id;
    edge_entry.ignore_offset = create_edge->ignore_offset;

    /* todo: change create_edge to use a database struct */

    status = database_common_create_edge_from_table_entry(&edge_entry);

    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Create edge failed!! 0x%x.\n",
            __FUNCTION__, status);
        return status;
    }

    status = fbe_database_transaction_add_edge_entry(fbe_database_service.transaction_ptr, 
        &edge_entry);

    database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Create edge transaction 0x%llx returned 0x%x.\n",
        __FUNCTION__, (unsigned long long)create_edge->transaction_id, status);

    return status;
}

fbe_status_t database_destroy_edge(fbe_database_control_destroy_edge_t * destroy_edge)
{
    database_edge_entry_t                  edge_entry;
    database_edge_entry_t*                 existing_edge_entry = NULL;
    database_object_entry_t                *object_entry = NULL;
    fbe_status_t                           status;

    /*get the object entry from object table*/
    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table, destroy_edge->object_id, &object_entry);
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: get object entry failed!! 0x%x.\n",
            __FUNCTION__, status);
        return status;
    }

    /*If this is a provision drive object*/
    if(object_entry->header.state !=  DATABASE_CONFIG_ENTRY_INVALID 
        && object_entry->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE)
    {
        status = fbe_database_destroy_edge(destroy_edge->object_id, destroy_edge->block_transport_destroy_edge.client_index);
        if (status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: destroy edge for PVD failed!! 0x%x.\n",
                __FUNCTION__, status);
        }
        return status;
    }
    
    /*Map the destroy edge to transaction table edge entry*/
    fbe_spinlock_lock(&fbe_database_service.edge_table.table_lock);
    status = fbe_database_config_table_get_edge_entry(&fbe_database_service.edge_table,
                                                              destroy_edge->object_id,
                                                              destroy_edge->block_transport_destroy_edge.client_index,
                                                              &existing_edge_entry);
    if (status != FBE_STATUS_OK )
    {
        fbe_spinlock_unlock(&fbe_database_service.edge_table.table_lock);
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s Failed to find edge %d in object table\n",
                     __FUNCTION__,destroy_edge->object_id);
        return status;
    }
    fbe_copy_memory(&edge_entry, existing_edge_entry,sizeof(database_edge_entry_t));
    fbe_spinlock_unlock(&fbe_database_service.edge_table.table_lock);

    /* todo: change create_edge to use a database struct */
    status = database_common_destroy_edge_from_table_entry(&edge_entry);
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: destroy edge failed!! 0x%x.\n",
                       __FUNCTION__, status);
        return status;
    }
    
    edge_entry.header.state = DATABASE_CONFIG_ENTRY_DESTROY;

    status = fbe_database_transaction_add_edge_entry(fbe_database_service.transaction_ptr, 
                                                     &edge_entry);

    database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Destroy edge transaction 0x%llx returned 0x%x.\n",
        __FUNCTION__, (unsigned long long)destroy_edge->transaction_id, status);

    return status;
}

fbe_status_t database_set_power_save(fbe_database_power_save_t * power_save)
{
    fbe_status_t                           status = FBE_STATUS_OK;
    database_global_info_entry_t           new_global_info_entry;
    database_global_info_entry_t           *old_global_info_entry = NULL;
    fbe_object_id_t                        object_id;
    database_object_entry_t                *object_entry = NULL;
    fbe_database_control_update_ps_stats_t ps_stats;
    
    /* Get the Global Info entry from the config table */		
    status = fbe_database_config_table_get_global_info_entry(&fbe_database_service.global_info, 
                                                             DATABASE_GLOBAL_INFO_TYPE_SYSTEM_POWER_SAVE, 
                                                             &old_global_info_entry);

    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get Global Info entry from the DB service.\n", 
                       __FUNCTION__);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_copy_memory(&new_global_info_entry, old_global_info_entry, sizeof(database_global_info_entry_t));

    /* Fill up the power_save info to global info in database table */
    fbe_copy_memory(&new_global_info_entry.info_union.power_saving_info, &power_save->system_power_save_info, 
                    sizeof(fbe_system_power_saving_info_t));

    /* Create global info from table entry */
    status = database_common_update_global_info_from_table_entry(&new_global_info_entry);
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: Create global info failed!! 0x%x.\n", 
                       __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    new_global_info_entry.header.state = DATABASE_CONFIG_ENTRY_MODIFY;
    status = fbe_database_transaction_add_global_info_entry(fbe_database_service.transaction_ptr, &new_global_info_entry);

    if(status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: modify power saving transaction 0x%llx returned 0x%x.\n", 
                       __FUNCTION__,
               (unsigned long long)power_save->transaction_id, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* if statistic_enabled transits from false to true, we should reset all the statistic values for all PVDs.*/
    if (power_save->system_power_save_info.stats_enabled && (old_global_info_entry->info_union.power_saving_info.stats_enabled == FBE_FALSE)) 
    {
        fbe_zero_memory(&ps_stats.ps_stats, sizeof(fbe_physical_drive_power_saving_stats_t));

        /*go over the tables, get objects that are pvds and set power saving statistics as 0 to all the PVD*/
        for (object_id = 0; object_id < fbe_database_service.object_table.table_size; object_id++) {
            status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                      object_id,
                                                                      &object_entry);  

            if ( status != FBE_STATUS_OK ){
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: failed to get object entry for object 0x%X\n",__FUNCTION__, object_id);
                continue;
            }
            /* set 0 to power saving statistics for all the PVDs.*/
            if (database_is_entry_valid(&object_entry->header) &&
            object_entry->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE) {
                ps_stats.object_id = object_id;
                status = fbe_database_set_ps_stats(&ps_stats);
                if ( status != FBE_STATUS_OK )
                {
                    database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s Failed to set power saving statistics to pvd object\n",
                                   __FUNCTION__);          
                }   
            } 
        }
    }

    return status;
}

fbe_status_t database_set_system_encryption_mode(fbe_system_encryption_info_t encryption_info)
{
    fbe_status_t                           status = FBE_STATUS_OK;
    database_global_info_entry_t           new_global_info_entry;
    database_global_info_entry_t           *old_global_info_entry = NULL;
    
    /* Get the Global Info entry from the config table */		
    status = fbe_database_config_table_get_global_info_entry(&fbe_database_service.global_info, 
                                                             DATABASE_GLOBAL_INFO_TYPE_SYSTEM_ENCRYPTION, 
                                                             &old_global_info_entry);

    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get Global Info entry from the DB service.\n", 
                       __FUNCTION__);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_copy_memory(&new_global_info_entry, old_global_info_entry, sizeof(database_global_info_entry_t));

    /* Fill up the system encryption mode info to global info in database table */
    fbe_copy_memory(&new_global_info_entry.info_union.encryption_info,
                    &encryption_info, 
                    sizeof(fbe_system_encryption_info_t));

    /* Create global info from table entry */
    status = database_common_update_global_info_from_table_entry(&new_global_info_entry);
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: Create global info failed!! 0x%x.\n", 
                       __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    new_global_info_entry.header.state = DATABASE_CONFIG_ENTRY_MODIFY;
    status = fbe_database_transaction_add_global_info_entry(fbe_database_service.transaction_ptr, &new_global_info_entry);

    if(status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: set encryption mode transaction 0x%llx returned 0x%x.\n", 
                       __FUNCTION__, fbe_database_service.transaction_ptr->transaction_id, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

fbe_status_t fbe_database_get_system_encryption_mode(fbe_system_encryption_mode_t *encryption_mode)
{
    fbe_status_t                           status = FBE_STATUS_OK;
    database_global_info_entry_t           *global_info_entry;
    
    
    /* Get the Global Info entry from the config table */		
    status = fbe_database_config_table_get_global_info_entry(&fbe_database_service.global_info, 
                                                             DATABASE_GLOBAL_INFO_TYPE_SYSTEM_ENCRYPTION, 
                                                             &global_info_entry);

    if ( status != FBE_STATUS_OK )
    {
        if(global_info_entry == NULL)
        {
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: No Global Info entry from the DB service with type 0x%x.\n", 
                       __FUNCTION__, DATABASE_GLOBAL_INFO_TYPE_SYSTEM_ENCRYPTION);
            *encryption_mode = FBE_SYSTEM_ENCRYPTION_MODE_UNENCRYPTED;
            return FBE_STATUS_OK;
        }
        else
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get Global Info entry from the DB service.\n", 
                           __FUNCTION__);
            
            return FBE_STATUS_GENERIC_FAILURE;
        }
    }
    *encryption_mode = global_info_entry->info_union.encryption_info.encryption_mode;
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_set_encryption_paused
 *****************************************************************
 * @brief
 *
 * @param
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *
 ****************************************************************/
fbe_status_t fbe_database_set_encryption_paused(fbe_bool_t encryption_paused)
{
    fbe_status_t                           status = FBE_STATUS_OK;
    database_global_info_entry_t           new_global_info_entry;
    database_global_info_entry_t           *old_global_info_entry = NULL;
    
    /* Get the Global Info entry from the config table */		
    status = fbe_database_config_table_get_global_info_entry(&fbe_database_service.global_info, 
                                                             DATABASE_GLOBAL_INFO_TYPE_SYSTEM_ENCRYPTION, 
                                                             &old_global_info_entry);

    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get Global Info entry from the DB service.\n", 
                       __FUNCTION__);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_copy_memory(&new_global_info_entry, old_global_info_entry, sizeof(database_global_info_entry_t));

    /* Fill up the system encryption mode info to global info in database table */
    new_global_info_entry.info_union.encryption_info.encryption_paused = encryption_paused;

    /* Create global info from table entry */
    status = database_common_update_global_info_from_table_entry(&new_global_info_entry);
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: Create global info failed!! 0x%x.\n", 
                       __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    new_global_info_entry.header.state = DATABASE_CONFIG_ENTRY_MODIFY;
    status = fbe_database_transaction_add_global_info_entry(fbe_database_service.transaction_ptr, &new_global_info_entry);

    if(status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: set encryption mode transaction 0x%llx returned 0x%x.\n", 
                       __FUNCTION__, fbe_database_service.transaction_ptr->transaction_id, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}


/*!***************************************************************
 * @fn fbe_database_get_encryption_paused
 *****************************************************************
 * @brief
 *
 * @param
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *
 ****************************************************************/
fbe_status_t fbe_database_get_encryption_paused(fbe_bool_t *encryption_paused)
{
    fbe_status_t                           status = FBE_STATUS_OK;
    database_global_info_entry_t           *global_info_entry;
    
    
    /* Get the Global Info entry from the config table */		
    status = fbe_database_config_table_get_global_info_entry(&fbe_database_service.global_info, 
                                                             DATABASE_GLOBAL_INFO_TYPE_SYSTEM_ENCRYPTION, 
                                                             &global_info_entry);

    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get Global Info entry from the DB service.\n", 
                       __FUNCTION__);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *encryption_paused = global_info_entry->info_union.encryption_info.encryption_paused;
    return FBE_STATUS_OK;
}



/*!***************************************************************
 * @fn database_control_set_encryption_paused
 *****************************************************************
 * @brief
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 * 
 *
 ****************************************************************/
static fbe_status_t database_control_set_encryption_paused(fbe_packet_t *packet)
{
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_database_encryption_t              *encryption = NULL;
    fbe_database_transaction_id_t          db_transaction_id = 0;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&encryption, sizeof(*encryption));
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Get Payload failed.\n", 
                       __FUNCTION__);

        fbe_database_complete_packet(packet, status);
        return status;
    }

    fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &db_transaction_id);

    status = fbe_database_transaction_is_valid_id(encryption->transaction_id, db_transaction_id);

    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_database_set_encryption_paused(encryption->system_encryption_info.encryption_paused);

    if (status == FBE_STATUS_OK && is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)encryption,
                                                             FBE_DATABASE_CMI_MSG_TYPE_SET_ENCRYPTION_PAUSE);
    }

    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn database_control_get_encryption_paused
 *****************************************************************
 * @brief
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 * 
 *
 ****************************************************************/
static fbe_status_t database_control_get_encryption_paused(fbe_packet_t *packet)
{
    fbe_status_t                     status = FBE_STATUS_OK;
    fbe_bool_t                      *paused = NULL;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&paused, sizeof(*paused));
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Get Payload failed.\n", 
                       __FUNCTION__);

        fbe_database_complete_packet(packet, status);
        return status;
    }


    status = fbe_database_get_encryption_paused(paused);

    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;
}

fbe_status_t database_update_system_spare_config(fbe_database_control_update_system_spare_config_t * spare_config)
{
    fbe_status_t                           status = FBE_STATUS_OK;
    database_global_info_entry_t           new_global_info_entry;
    database_global_info_entry_t           *old_global_info_entry = NULL;
    
    /* Get the Global Info entry from the config table */		
    status = fbe_database_config_table_get_global_info_entry(&fbe_database_service.global_info, 
                                                             DATABASE_GLOBAL_INFO_TYPE_SYSTEM_SPARE, 
                                                             &old_global_info_entry);

    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get Global Info entry from the DB service.\n", 
                       __FUNCTION__);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_copy_memory(&new_global_info_entry, old_global_info_entry, sizeof(database_global_info_entry_t));

    /* Fill up the global info in database table */
    fbe_copy_memory(&new_global_info_entry.info_union.spare_info, &spare_config->system_spare_info, 
                    sizeof(fbe_system_spare_config_info_t));

    /* Create global info from table entry */
    status = database_common_update_global_info_from_table_entry(&new_global_info_entry);
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: Create global info failed!! 0x%x.\n", 
                       __FUNCTION__, status);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }
    new_global_info_entry.header.state = DATABASE_CONFIG_ENTRY_MODIFY;
    status = fbe_database_transaction_add_global_info_entry(fbe_database_service.transaction_ptr, &new_global_info_entry);

    if(status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: Create edge transaction 0x%llx returned 0x%x.\n", 
                       __FUNCTION__,
               (unsigned long long)spare_config->transaction_id, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

static fbe_status_t database_map_psl_lun_to_attributes(fbe_object_id_t lun_object_id, fbe_u32_t *attributes)
{
    switch (lun_object_id) {
    case FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_0:
        (*attributes) |= FBE_LU_ATTR_VCX_0;
        break;
    case FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_1:
        (*attributes) |= FBE_LU_ATTR_VCX_1;
        break;
    case FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_2:
        (*attributes) |= FBE_LU_ATTR_VCX_2;
        break;
    case FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_3:
        (*attributes) |= FBE_LU_ATTR_VCX_3;
        break;
    case FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_4:
        (*attributes) |= FBE_LU_ATTR_VCX_4;
        break;
    case FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_VCX_LUN_5:
        (*attributes) |= FBE_LU_ATTR_VCX_5;
        break;
    //HELGA HACK -- Allow c4admintool to see private LUN 8226
    case FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_UDI_SYSTEM_POOL:
        (*attributes) |= FBE_LU_ATTR_VCX_6;
        break;
    default:
        break;
        
    }

    return FBE_STATUS_OK;

}

static fbe_status_t database_control_get_stats(fbe_packet_t *packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_database_control_get_stats_t *		db_stats = NULL;
    fbe_u32_t								lun_count = 0;
    fbe_u32_t								rg_count = 0;
    fbe_database_poll_record_t              *poll_record = NULL;
    database_poll_ring_buffer_t             *poll_stats = &fbe_database_service.poll_ring_buffer;
#ifndef NO_EXT_POOL_ALIAS
    fbe_u32_t								pool_count = 0;
#endif

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&db_stats, sizeof(fbe_database_control_get_stats_t));
    if ( status != FBE_STATUS_OK ){
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Get Payload failed.\n", 
                       __FUNCTION__);

        fbe_database_complete_packet(packet, status);
        return status;
    }

    status = fbe_database_config_table_get_count_by_db_class(&fbe_database_service.user_table,
                                                             DATABASE_CLASS_ID_LUN,
                                                             &lun_count);

    if ( status != FBE_STATUS_OK ){
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: unable to get lun count\n", 
                       __FUNCTION__);

        fbe_database_complete_packet(packet, status);
        return status;
    }

    status = fbe_database_config_table_get_count_by_db_class(&fbe_database_service.user_table,
                                                             DATABASE_CLASS_ID_RAID_START,/*the function will understand we want all RG classed*/
                                                             &rg_count);

    if ( status != FBE_STATUS_OK ){
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: unable to get lun count\n", 
                       __FUNCTION__);

        fbe_database_complete_packet(packet, status);
        return status;
    }

#ifndef NO_EXT_POOL_ALIAS
    status = fbe_database_config_table_get_count_by_db_class(&fbe_database_service.user_table,
                                                             DATABASE_CLASS_ID_EXTENT_POOL,/*the function will understand we want all RG classed*/
                                                             &pool_count);
    if ( status != FBE_STATUS_OK ){
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: unable to get lun count\n", 
                       __FUNCTION__);

        fbe_database_complete_packet(packet, status);
        return status;
    }
    rg_count += pool_count;
#endif

    db_stats->num_system_luns           = fbe_private_space_layout_get_number_of_system_luns();
    db_stats->num_user_luns             = lun_count - db_stats->num_system_luns;
    db_stats->num_system_raids          = fbe_private_space_layout_get_number_of_system_raid_groups();
    db_stats->num_user_raids            = rg_count - db_stats->num_system_raids;
    db_stats->max_allowed_luns_per_rg   = fbe_database_service.logical_objects_limits.platform_max_lun_per_rg;
    db_stats->max_allowed_user_luns     = fbe_database_service.logical_objects_limits.platform_max_user_lun;
    db_stats->max_allowed_user_rgs      = fbe_database_service.logical_objects_limits.platform_max_rg;

    poll_record = &poll_stats->records[poll_stats->end];
    db_stats->last_poll_time            = poll_record->time_of_poll;
    db_stats->last_poll_bitmap          = poll_record->poll_request_bitmap;
    db_stats->threshold_count           = poll_stats->threshold_count;
    

    fbe_database_complete_packet(packet, status);
    return status;

}


static fbe_status_t database_control_get_transaction_info(fbe_packet_t * packet)
{
    fbe_status_t                     status = FBE_STATUS_OK;
    database_transaction_t *tran_info_ptr = NULL;
    database_transaction_t *transaction = NULL;


    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&tran_info_ptr, sizeof(database_transaction_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    } 
    else 
    {
        transaction = fbe_database_service.transaction_ptr;
        if (transaction != NULL)
        {
            fbe_copy_memory(tran_info_ptr, transaction, sizeof(database_transaction_t));
        }
        else
        {
            /*transaction pointer is NULL, it is not initialized yet, so can't get info*/
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                "%s: transaction of database service isn't initialized yet\n",
                __FUNCTION__);
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }
    

    fbe_database_complete_packet(packet, status);
    return status;
}

static fbe_status_t database_control_destroy_all_objects(void)
{
    fbe_u32_t		class_id = 0;
    fbe_status_t	status = FBE_STATUS_GENERIC_FAILURE;

    database_class_id_t		class_id_destruction_order [] =
    {
        DATABASE_CLASS_ID_EXTENT_POOL_LUN,
        DATABASE_CLASS_ID_EXTENT_POOL_METADATA_LUN,
        DATABASE_CLASS_ID_EXTENT_POOL,
        DATABASE_CLASS_ID_LUN,
        DATABASE_CLASS_ID_STRIPER,
        DATABASE_CLASS_ID_MIRROR,
        DATABASE_CLASS_ID_PARITY,
        DATABASE_CLASS_ID_VIRTUAL_DRIVE,
        DATABASE_CLASS_ID_PROVISION_DRIVE,
        DATABASE_CLASS_ID_BVD_INTERFACE,
        DATABASE_CLASS_ID_INVALID,
        
    };

#if 0 /* CMI is already destroyed */
    /*before we destroy the objects, we make sure all incoming CMi traffic is disabled since we don't want 
    anything coming while the object is being destroyed, in cause cause timing issues*/
    status = fbe_database_send_packet_to_service(FBE_CMI_CONTROL_CODE_DISABLE_TRAFFIC,
                                                 NULL,
                                                 0,
                                                 FBE_SERVICE_ID_CMI,
                                                 NULL,  /* no sg list*/
                                                 0,  /* no sg list*/
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s, failed to disable CMI incoming\n", __FUNCTION__);
    }
#endif

    /*make sure no one is doing any configuration changes.
    Technuically, there might still be a job in flight here but even with the previous approach of 
    simply killing the objects we were not protected against it. We can do a job in the future*/
    set_database_service_state(&fbe_database_service, FBE_DATABASE_STATE_DESTROYING);

    /* external system obj and user obj first */
    for (class_id = 0; class_id_destruction_order[class_id] != DATABASE_CLASS_ID_INVALID; class_id++) {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s destroy external db class id: %d\n", __FUNCTION__, class_id_destruction_order[class_id]);

        status = fbe_database_config_destroy_all_objects_of_class(class_id_destruction_order[class_id], &fbe_database_service, FBE_FALSE);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s, Destroy objects fail for class_id %d\n", __FUNCTION__, class_id_destruction_order[class_id]);
        }
    }

    /* internal system obj now */
    set_database_service_state(&fbe_database_service, FBE_DATABASE_STATE_DESTROYING_SYSTEM);
    for (class_id = 0; class_id_destruction_order[class_id] != DATABASE_CLASS_ID_INVALID; class_id++) {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s destroy internal db class id: %d\n", __FUNCTION__, class_id_destruction_order[class_id]);

        status = fbe_database_config_destroy_all_objects_of_class(class_id_destruction_order[class_id], &fbe_database_service, FBE_TRUE);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_CRITICAL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s, Destroy internal objects fail for class_id %d\n", __FUNCTION__, class_id_destruction_order[class_id]);
        }
    }
    
    return FBE_STATUS_OK;

}

static fbe_status_t database_control_enable_cmi_traffic(void)
{
    fbe_status_t status;

    status = fbe_database_send_packet_to_service(FBE_CMI_CONTROL_CODE_ENABLE_TRAFFIC,
                                                 NULL,
                                                 0,
                                                 FBE_SERVICE_ID_CMI,
                                                 NULL,  /* no sg list*/
                                                 0,  /* no sg list*/
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 FBE_PACKAGE_ID_SEP_0);

    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s, failed to enable CMI incoming\n", __FUNCTION__);
    }
    return status;
}


/*handle database system objects db related command*/
static fbe_status_t database_control_system_db_op(fbe_packet_t * packet)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_database_control_system_db_op_t *op = NULL;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&op, sizeof(fbe_database_control_system_db_op_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    if (op != NULL) {
        switch (op->cmd) {
        case FBE_DATABASE_CONTROL_SYSTEM_DB_READ_OBJECT:
            if ((! fbe_private_space_layout_object_id_is_system_object(op->object_id)) || 
                MAX_SYSTEM_DB_OP_CMD_BUFFER_SIZE < sizeof (database_object_entry_t))
            {
                status = FBE_STATUS_GENERIC_FAILURE;
            } 
            else
            {
                status = fbe_database_system_db_read_object_entry(op->object_id, 
                                                                  (database_object_entry_t *)op->cmd_data);
            }
            break;
        case FBE_DATABASE_CONTROL_SYSTEM_DB_READ_USER:
            if ((! fbe_private_space_layout_object_id_is_system_object(op->object_id)) ||
                MAX_SYSTEM_DB_OP_CMD_BUFFER_SIZE < sizeof (database_user_entry_t))
            {
                status = FBE_STATUS_GENERIC_FAILURE;
            } 
            else
            {
                status = fbe_database_system_db_read_user_entry(op->object_id, 
                                                                  (database_user_entry_t *)op->cmd_data);
            }
            break;
        case FBE_DATABASE_CONTROL_SYSTEM_DB_READ_EDGE:
            if ((! fbe_private_space_layout_object_id_is_system_object(op->object_id)) ||
                op->edge_index >= DATABASE_MAX_EDGE_PER_OBJECT ||
                MAX_SYSTEM_DB_OP_CMD_BUFFER_SIZE < sizeof (database_edge_entry_t))
            {
                status = FBE_STATUS_GENERIC_FAILURE;
            } 
            else
            {
                status = fbe_database_system_db_read_edge_entry(op->object_id, op->edge_index,
                                                                  (database_edge_entry_t *)op->cmd_data);
            }
            break;
        case FBE_DATABASE_CONTROL_SYSTEM_DB_WRITE_OBJECT:
            if ((! fbe_private_space_layout_object_id_is_system_object(op->object_id)) || 
                MAX_SYSTEM_DB_OP_CMD_BUFFER_SIZE < sizeof (database_object_entry_t))
            {
                status = FBE_STATUS_GENERIC_FAILURE;
            } 
            else
            {
                status = fbe_database_system_db_raw_persist_object_entry(op->persist_type, 
                                                                  (database_object_entry_t *)op->cmd_data);
            }
            break;
        case FBE_DATABASE_CONTROL_SYSTEM_DB_WRITE_USER:
            if ((! fbe_private_space_layout_object_id_is_system_object(op->object_id)) ||
                MAX_SYSTEM_DB_OP_CMD_BUFFER_SIZE < sizeof (database_user_entry_t))
            {
                status = FBE_STATUS_GENERIC_FAILURE;
            } 
            else
            {
                status = fbe_database_system_db_raw_persist_user_entry(op->persist_type, 
                                                                  (database_user_entry_t *)op->cmd_data);
            }
            break;
        case FBE_DATABASE_CONTROL_SYSTEM_DB_WRITE_EDGE:
            if ((! fbe_private_space_layout_object_id_is_system_object(op->object_id)) ||
                op->edge_index >= DATABASE_MAX_EDGE_PER_OBJECT ||
                MAX_SYSTEM_DB_OP_CMD_BUFFER_SIZE < sizeof (database_edge_entry_t))
            {
                status = FBE_STATUS_GENERIC_FAILURE;
            } 
            else
            {
                status = fbe_database_system_db_raw_persist_edge_entry(op->persist_type, 
                                                                      (database_edge_entry_t *)op->cmd_data);
            }
            break;
        case FBE_DATABASE_CONTROL_SYSTEM_DB_GET_RAW_MIRROR_ERR_REPORT:
            if (MAX_SYSTEM_DB_OP_CMD_BUFFER_SIZE < sizeof (fbe_database_control_raw_mirror_err_report_t))
            {
                status = FBE_STATUS_GENERIC_FAILURE;
            } 
            else
            {
                status = fbe_database_util_raw_mirror_get_error_report((fbe_database_control_raw_mirror_err_report_t *)op->cmd_data);
            }
            break;
        case FBE_DATABASE_CONTROL_SYSTEM_DB_VERIFY_BOOT:
            status = system_db_rw_verify();
            break;
        default:
            status = FBE_STATUS_GENERIC_FAILURE;
            break;
        }

    } else
        status = FBE_STATUS_GENERIC_FAILURE;


    fbe_database_complete_packet(packet, status);
    return status;
}


/*!**************************************************************
 * database_get_service_mode_reason_string()
 ****************************************************************
 * @brief
 *  This function converts database maintenanace reason to a text string.
 *
 * @param 
 *        database_get_maintenance_reason_string reason
 * 
 * @return
 *        fbe_char_t *  A string for db service mode reason
 ***************************************************************/
static fbe_char_t *
database_get_service_mode_reason_string(fbe_database_service_mode_reason_t reason)
{
    switch(reason)
    {
    case FBE_DATABASE_SERVICE_MODE_REASON_INVALID:
        return "Not in degraded mode.";
    case FBE_DATABASE_SERVICE_MODE_REASON_INTERNAL_OBJS:
        return "System config database read failure or data corrupted.";
    case FBE_DATABASE_SERVICE_MODE_REASON_HOMEWRECKER_CHASSIS_MISMATCHED:
        return "System disks do not match with chassis. System disks or chassis may be unplanned replaced";
    case FBE_DATABASE_SERVICE_MODE_REASON_HOMEWRECKER_SYSTEM_DISK_DISORDER:
        return "The System Drives are disordered";
    case FBE_DATABASE_SERVICE_MODE_REASON_HOMEWRECKER_INTEGRITY_BROKEN:
        return "System integrity is broken. More than two system disks are illegal";
    case FBE_DATABASE_SERVICE_MODE_REASON_HOMEWRECKER_DOUBLE_INVALID_DRIVE_WITH_DRIVE_IN_USER_SLOT:
        return "Two system drives are invalid and at least one of them are inserted in user slot"; 
    case FBE_DATABASE_SERVICE_MODE_REASON_HOMEWRECKER_OPERATION_ON_WWNSEED_CHAOS:
        return "Both usermodifiedWwnSeedFlag and chassis_replacement_movement were set";
    case FBE_DATABASE_SERVICE_MODE_REASON_SYSTEM_DB_HEADER_IO_ERROR:
        return "Load system db header IO error.";
    case FBE_DATABASE_SERVICE_MODE_REASON_SYSTEM_DB_HEADER_TOO_LARGE:
        return "Load too large system db header from disk.";
    case FBE_DATABASE_SERVICE_MODE_REASON_SYSTEM_DB_HEADER_DATA_CORRUPT:
        return "System db header is mismathed.";
    case FBE_DATABASE_SERVICE_MODE_REASON_INVALID_MEMORY_CONF_CHECK:
        return "Invalid system memory configuration found in database EMV check.";
    case FBE_DATABASE_SERVICE_MODE_REASON_PROBLEMATIC_DATABASE_VERSION:
        return "The version of database has a problem. The size in new version is smaller.\n";
    case FBE_DATABASE_SERVICE_MODE_REASON_SMALL_SYSTEM_DRIVE:
        return "The capacity of system drive smaller than 3G, not enough for system Raid Group. \n";
    case FBE_DATABASE_SERVICE_MODE_REASON_NOT_ALL_DRIVE_SET_ICA_FLAGS:
        return "When Set ICA flags, not all three drives are set with ICA flags\n";
    case FBE_DATABASE_SERVICE_MODE_REASON_DB_VALIDATION_FAILED:
        return "Database validation found one or more errors.\n";
    default:
        return "Unknown degraded mode reason.";
    }
}
/*!**************************************************************
 * fbe_database_is_pvd_exists_by_id()
 ****************************************************************
 * @brief
 *  This function check whether an pvd is exits by its id.
 *
 * @param 
 *        object_id
 * 
 * @return
 *        
 ***************************************************************/
fbe_bool_t fbe_database_is_pvd_exists_by_id(fbe_object_id_t object_id)
{
    database_object_entry_t     *object_table_entry = NULL;
    fbe_status_t                status = FBE_STATUS_GENERIC_FAILURE;
    fbe_bool_t                  found = FBE_FALSE;

    fbe_spinlock_lock(&fbe_database_service.object_table.table_lock);
    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              object_id,
                                                              &object_table_entry);
    // if we found an entry
    if ((status == FBE_STATUS_OK) &&
        (object_table_entry != NULL))
    {
        if(object_table_entry->header.object_id == object_id)
        {
            found = FBE_TRUE;
        }
    }
    fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);
    return found;
}

/*!***************************************************************
 * @fn database_control_get_time_threhold
 *****************************************************************
 * @brief
 *   Control function to get the time threshold information. 
 *
 * @param packet - packet
 * @param packet_status - status to set for the packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    1/06/2012: zhangy
 *
 ****************************************************************/
static fbe_status_t database_control_get_time_threshold(fbe_packet_t *packet)
{
    fbe_status_t                       status = FBE_STATUS_OK;
    fbe_database_time_threshold_t          *time_threshold = NULL;
    database_global_info_entry_t       *global_info_entry = NULL;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&time_threshold, sizeof(*time_threshold));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /* Get the Global Info entry from the config table */		
    status = fbe_database_config_table_get_global_info_entry(&fbe_database_service.global_info, 
                                                             DATABASE_GLOBAL_INFO_TYPE_SYSTEM_TIME_THRESHOLD, 
                                                             &global_info_entry);

    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get Global Info entry from the DB service.\n", 
                       __FUNCTION__);
    }

    /* Copy the contents from the database table */
    fbe_copy_memory(&time_threshold->system_time_threshold_info,
                    &global_info_entry->info_union.time_threshold_info, 
                    sizeof(fbe_system_time_threshold_info_t));

    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn database_control_set_time_threhold
 *****************************************************************
 * @brief
 *   Control function to set the time threshold information. 
 *
 * @param packet - packet
 * @param packet_status - status to set for the packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    1/06/2012: zhangy
 *
 ****************************************************************/
static fbe_status_t database_control_set_time_threshold(fbe_packet_t *packet)
{
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_database_time_threshold_t          *time_threshold = NULL;
    fbe_database_transaction_id_t          db_transaction_id = 0;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&time_threshold, sizeof(*time_threshold));
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Get Payload failed.\n", 
                       __FUNCTION__);

        fbe_database_complete_packet(packet, status);
        return status;
    }

    fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &db_transaction_id);

    status = fbe_database_transaction_is_valid_id(time_threshold->transaction_id, db_transaction_id);

    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    status = database_set_time_threshold(time_threshold);
    //need cmi to peer?
    if (status == FBE_STATUS_OK && is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
       status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)time_threshold,
                                                             FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_TIME_THRESHOLD_INFO);
    }

    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn database_set_time_threhold
 *****************************************************************
 * @brief
 *   function to set the time threshold information. 
 *
 * @param  time_threshold - 
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    1/06/2012: zhangy
 *
 ****************************************************************/
fbe_status_t database_set_time_threshold(fbe_database_time_threshold_t *time_threshold)
{
    fbe_status_t                           status = FBE_STATUS_OK;
    database_global_info_entry_t           new_global_info_entry;
    database_global_info_entry_t           *old_global_info_entry = NULL;
    
    /* Get the Global Info entry from the config table */		
    status = fbe_database_config_table_get_global_info_entry(&fbe_database_service.global_info, 
                                                             DATABASE_GLOBAL_INFO_TYPE_SYSTEM_TIME_THRESHOLD, 
                                                             &old_global_info_entry);

    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get Global Info entry from the DB service.\n", 
                       __FUNCTION__);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_copy_memory(&new_global_info_entry, old_global_info_entry, sizeof(database_global_info_entry_t));

    /* Fill up the pvd destroy threshold info to global info in database table */
    fbe_copy_memory(&new_global_info_entry.info_union.time_threshold_info, &time_threshold->system_time_threshold_info, 
                    sizeof(fbe_system_time_threshold_info_t));

    /* Create global info from table entry */
    status = database_common_update_global_info_from_table_entry(&new_global_info_entry);
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: Create global info failed!! 0x%x.\n", 
                       __FUNCTION__, status);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    new_global_info_entry.header.state = DATABASE_CONFIG_ENTRY_MODIFY;
    status = fbe_database_transaction_add_global_info_entry(fbe_database_service.transaction_ptr, &new_global_info_entry);

    if(status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: Create edge transaction 0x%llx returned 0x%x.\n", 
                       __FUNCTION__,
               (unsigned long long)time_threshold->transaction_id,
               status);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}
/*!***************************************************************
 * @fn database_get_time_threhold
 *****************************************************************
 * @brief
 *   function to get the time threshold information. 
 *
 * @param  time_threshold - 
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    1/06/2012: zhangy
 *
 ****************************************************************/
fbe_status_t database_get_time_threshold(fbe_system_time_threshold_info_t *time_threshold)
{
    fbe_status_t                           status = FBE_STATUS_OK;
    database_global_info_entry_t           *global_info_entry = NULL;
    
    /* Get the Global Info entry from the config table */		
    status = fbe_database_config_table_get_global_info_entry(&fbe_database_service.global_info, 
                                                             DATABASE_GLOBAL_INFO_TYPE_SYSTEM_TIME_THRESHOLD, 
                                                             &global_info_entry);

    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get Global Info entry from the DB service.\n", 
                       __FUNCTION__);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }
    fbe_copy_memory(time_threshold, &global_info_entry->info_union.time_threshold_info, sizeof(fbe_system_time_threshold_info_t));

    return status;
}



static fbe_status_t database_control_get_fru_descriptor(fbe_packet_t * packet)
{
    fbe_status_t                     status = FBE_STATUS_INVALID;
    fbe_database_control_get_fru_descriptor_t*    out_descriptor = NULL;
    fbe_homewrecker_fru_descriptor_t    copied_descriptor;
    fbe_homewrecker_get_raw_mirror_info_t   report;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&out_descriptor, sizeof(*out_descriptor));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    fbe_spinlock_lock(&fbe_database_service.pvd_operation.system_fru_descriptor_lock);
    fbe_copy_memory(&copied_descriptor, &fbe_database_service.pvd_operation.system_fru_descriptor, sizeof(copied_descriptor));
    fbe_spinlock_unlock(&fbe_database_service.pvd_operation.system_fru_descriptor_lock);

    if(0 == fbe_compare_string(copied_descriptor.magic_string, FBE_FRU_DESCRIPTOR_MAGIC_STRING_LENGTH, 
                    FBE_FRU_DESCRIPTOR_MAGIC_STRING, FBE_FRU_DESCRIPTOR_MAGIC_STRING_LENGTH, FBE_TRUE))
    {
        /*if the magic number is valid, just copy the in-memory copy*/
        fbe_copy_memory(&out_descriptor->descriptor, &copied_descriptor, sizeof(copied_descriptor));
        
        out_descriptor->access_status = FBE_GET_HW_STATUS_OK;
        
        fbe_database_complete_packet(packet, FBE_STATUS_OK);
        return FBE_STATUS_OK;
    }

    /*if the magic number is invalid, read from disk*/
    status = fbe_database_get_homewrecker_fru_descriptor(&copied_descriptor,
                                                                                                                          &report,
                                                                                                                          FBE_FRU_DISK_ALL);
    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get fru descriptor from disk.\n", 
                       __FUNCTION__);
        out_descriptor->access_status = FBE_GET_HW_STATUS_ERROR;        
        fbe_database_complete_packet(packet, status);
        return status;        
    }

    fbe_copy_memory(&out_descriptor->descriptor, &copied_descriptor, sizeof(copied_descriptor));

    out_descriptor->access_status = FBE_GET_HW_STATUS_OK;

    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;
    
}



static fbe_status_t database_control_set_fru_descriptor(fbe_packet_t * packet)
{
    fbe_status_t                     status = FBE_STATUS_INVALID;
    fbe_database_control_set_fru_descriptor_t*    in_descriptor = NULL;
    fbe_homewrecker_fru_descriptor_t    fru_descriptor;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&in_descriptor, sizeof(*in_descriptor));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    fru_descriptor.chassis_replacement_movement = in_descriptor->chassis_replacement_movement;
    fru_descriptor.structure_version = in_descriptor->structure_version;
    fru_descriptor.wwn_seed = in_descriptor->wwn_seed;
    fbe_copy_memory(&fru_descriptor.system_drive_serial_number, in_descriptor->system_drive_serial_number, FBE_FRU_DESCRIPTOR_SYSTEM_DRIVE_NUMBER * sizeof(serial_num_t));
    fbe_copy_memory(fru_descriptor.magic_string,
                    FBE_FRU_DESCRIPTOR_MAGIC_STRING, 
                    sizeof(FBE_FRU_DESCRIPTOR_MAGIC_STRING));

    if (is_peer_update_allowed(&fbe_database_service))
    {
        status = fbe_database_cmi_update_peer_fru_descriptor(&fru_descriptor);
        if ( status != FBE_STATUS_OK )
        {
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: Sync fru descriptor to passive side failed\n", __FUNCTION__);
            fbe_database_complete_packet(packet, status);
            return status;
        }
    }

    /*then change the fru descriptor in our side*/
    status = fbe_database_set_homewrecker_fru_descriptor(&fbe_database_service, &fru_descriptor);
    if ( status != FBE_STATUS_OK )
    {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "Wrote raw mirror fru descriptor failed\n");
    }

    fbe_database_complete_packet(packet, status);
    return status;

}

static fbe_status_t database_control_get_disk_signature(fbe_packet_t * packet)
{
    fbe_status_t                     status = FBE_STATUS_GENERIC_FAILURE;
    
    fbe_database_control_signature_t*    out_signature = NULL;
    fbe_database_fru_signature_IO_operation_status_t operation_status;
   
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&out_signature, sizeof(*out_signature));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    status = fbe_database_read_fru_signature_from_disk(out_signature->bus, out_signature->enclosure,
                                                        out_signature->slot, &out_signature->signature, 
                                                        &operation_status);
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }   

    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;

}
static fbe_status_t database_control_set_disk_signature(fbe_packet_t * packet)
{
    
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                           chassis_wwn_seed;
    fbe_database_control_signature_t*   in_signature = NULL;
    fbe_fru_signature_t                 signature = {0};
    fbe_database_fru_signature_IO_operation_status_t operation_status;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&in_signature, sizeof(*in_signature));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    
    /*Get wwn seed from midplane prom*/
    status = fbe_database_get_board_resume_prom_wwnseed_by_pp(&chassis_wwn_seed);
    if (FBE_STATUS_OK != status)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: Failed to read PROM wwn_seed\n",
                    __FUNCTION__);

        return status;
    }

    signature.bus = in_signature->bus;
    signature.enclosure = in_signature->enclosure;
    signature.slot = in_signature->slot;
    signature.system_wwn_seed = chassis_wwn_seed;
    signature.version = FBE_FRU_SIGNATURE_VERSION;
    fbe_copy_memory(signature.magic_string, 
                    FBE_FRU_SIGNATURE_MAGIC_STRING, 
                    FBE_FRU_SIGNATURE_MAGIC_STRING_SIZE);

    status = fbe_database_write_fru_signature_to_disk(signature.bus, 
                                                     signature.enclosure, 
                                                      signature.slot, 
                                                     &signature, 
                                                     &operation_status);
    if (FBE_STATUS_OK != status || 
        operation_status != FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_ACCESS_OK)
    {
    
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Failed to write signature to disk bus %d, enc %d, slot %d\n",
                        __FUNCTION__, signature.bus, signature.enclosure, signature.slot);
        fbe_database_complete_packet(packet, status);
        return status;
    }
    
    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;
}

static fbe_status_t database_control_clear_disk_signature(fbe_packet_t * packet)
{
    fbe_status_t                        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_signature_t*   in_signature = NULL;
    fbe_fru_signature_t                 signature = {0};
    fbe_database_fru_signature_IO_operation_status_t operation_status;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&in_signature, sizeof(*in_signature));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    status = fbe_database_write_fru_signature_to_disk(in_signature->bus, 
                                                     in_signature->enclosure, 
                                                      in_signature->slot, 
                                                     &signature, 
                                                     &operation_status);
    if (FBE_STATUS_OK != status || 
        operation_status != FBE_DATABASE_FRU_SIGNATURE_IO_OPERATION_STATUS_ACCESS_OK)
    {
    
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Failed to write signature to disk bus %d, enc %d, slot %d\n",
                        __FUNCTION__, in_signature->bus, in_signature->enclosure, in_signature->slot);
        fbe_database_complete_packet(packet, status);
        return status;
    }
    
    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;

}


/*!***************************************************************
 * @fn database_control_update_peer_system_bg_service_flag
 *****************************************************************
 * @brief
 *   Control function to update peer system background service flag.
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    2/28/2012: Vera created
 *
 ****************************************************************/
static fbe_status_t database_control_update_peer_system_bg_service_flag(fbe_packet_t *packet)
{
    fbe_status_t                                            status = FBE_STATUS_OK;
    fbe_database_control_update_peer_system_bg_service_t*  system_bg_service_p = NULL;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&system_bg_service_p, sizeof(*system_bg_service_p));
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Get Payload failed.\n", 
                       __FUNCTION__);

        fbe_database_complete_packet(packet, status);
        return status;
    }

    if (status == FBE_STATUS_OK && is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)system_bg_service_p,
                                                             FBE_DATABE_CMI_MSG_TYPE_SET_SYSTEM_BG_SERVICE_FLAG);
    }

    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;
}



static fbe_status_t database_get_next_available_lun_number(fbe_lun_number_t *lun_number)
{
    fbe_status_t	status;
    fbe_u64_t *		free_lun_number_bitmap = fbe_database_service.free_lun_number_bitmap;

    if (free_lun_number_bitmap == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }else{
        fbe_zero_memory(free_lun_number_bitmap, sizeof(fbe_u64_t) * (fbe_database_service.smallest_psl_lun_number / 64));
    }

    /*let's fill the bitmap*/
    status = database_fill_free_entry_bitmap(fbe_database_service.smallest_psl_lun_number,
                                             &fbe_database_service.user_table,
                                             free_lun_number_bitmap,
                                             DATABASE_CLASS_ID_LUN);
    if (status != FBE_STATUS_OK) {
        return status;
    }

#ifndef NO_EXT_POOL_ALIAS
    /*let's fill the bitmap for ext pool lun*/
    status = database_fill_free_entry_bitmap(fbe_database_service.smallest_psl_lun_number,
                                             &fbe_database_service.user_table,
                                             free_lun_number_bitmap,
                                             DATABASE_CLASS_ID_EXTENT_POOL_LUN);
    if (status != FBE_STATUS_OK) {
        return status;
    }
#endif

    /*and look for a free entry in it after the previous functiond marked the bits as used for existing lu numbers*/
    status = database_get_next_free_number(fbe_database_service.smallest_psl_lun_number,
                                           free_lun_number_bitmap,
                                           (fbe_u32_t *)lun_number);

   return status;
}

static fbe_status_t database_get_next_available_rg_number(fbe_raid_group_number_t *rg_number, database_class_id_t db_class_id)
{
    fbe_status_t	status;
    fbe_u64_t *		free_rg_number_bitmap = fbe_database_service.free_rg_number_bitmap;

    if (free_rg_number_bitmap == NULL) {
        return FBE_STATUS_GENERIC_FAILURE;
    }else{
        fbe_zero_memory(free_rg_number_bitmap, sizeof(fbe_u64_t) * (fbe_database_service.smallest_psl_rg_number/ 64));
    }

    /*let's fill the bitmap*/
    status = database_fill_free_entry_bitmap(fbe_database_service.smallest_psl_rg_number,
                                             &fbe_database_service.user_table,
                                             free_rg_number_bitmap,
                                             db_class_id);
    if (status != FBE_STATUS_OK) {
       return status;
    }

    /*and look for a free entry in it after the previous functiond marked the bits as used for existing lu numbers*/
    status = database_get_next_free_number(fbe_database_service.smallest_psl_rg_number,
                                           free_rg_number_bitmap,
                                           (fbe_u32_t *)rg_number);

    database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s:end\n",
                       __FUNCTION__);

    return status;
}


/*we get the next avaliable number based on the free bits.
We have an array of 64 bits so we need MAX_XXXX_NUMBER/64 entries in the array.
Each bit represent a lun number. This array is being updated when we create a lun/rg, delete a lun/rg or load one from the database*/
static fbe_status_t database_get_next_free_number(fbe_u32_t max_number,
                                                  fbe_u64_t *bitmap,
                                                  fbe_u32_t *out_num)

{

    fbe_u32_t 	max_slots = (max_number / 64);
    fbe_u32_t 	current_slot;
    fbe_u64_t	bit_search = 0x1;
    fbe_u32_t	bit_offset = 0;

    for (current_slot = 0; current_slot <= max_slots; current_slot++){

        /*is this entry filled ?*/
        if (*bitmap == 0xFFFFFFFFFFFFFFFF) {
            bitmap++;/*got to next one*/
            continue;
        }

        /*we have at least one free bit, let's fund it*/
        while((bit_search & (*bitmap)) != 0) {
            bit_search<<=1;
            bit_offset++;
        }

         *out_num = (current_slot * 64) + bit_offset;

         if ((*out_num) >= max_number) {
             *out_num = FBE_U32_MAX;
             return FBE_STATUS_GENERIC_FAILURE;
         }else{
            return FBE_STATUS_OK;
         }
    }

    *out_num = FBE_U32_MAX;
    return FBE_STATUS_GENERIC_FAILURE;
}



fbe_status_t database_mark_lun_number_used(fbe_lun_number_t lun_number,
                                           fbe_u64_t *bitmap)
{
    return database_mark_number_used(FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_PSM,
                                         bitmap,
                                         (fbe_u32_t )lun_number);

}

fbe_status_t database_mark_rg_number_used(fbe_raid_group_number_t rg_number,
                                          fbe_u64_t *bitmap)
{
    return database_mark_number_used(FBE_PRIVATE_SPACE_LAYOUT_RG_ID_TRIPLE_MIRROR,
                                         bitmap,
                                         (fbe_u32_t )rg_number);
}

static fbe_status_t database_mark_number_used(fbe_u32_t max_number,
                                                  fbe_u64_t *bitmap,
                                                  fbe_u32_t in_number)
{
    fbe_u32_t 					offset;
    fbe_u32_t					array_location;
    fbe_u64_t					bit_offset = 0x1;

    /*sanity check*/
    if (max_number <= in_number){
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s:attempt to set a number bigger than max: #:%d, Max:%d\n",
                       __FUNCTION__, in_number, max_number);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    offset = in_number;

    array_location = offset / 64;
    offset %= 64;
    bit_offset <<= offset;

    /* sanity check. */
    if (bitmap[array_location] & bit_offset) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s:location %d already set\n",
                       __FUNCTION__, in_number);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    bitmap[array_location] |= bit_offset;

    return FBE_STATUS_OK;
}

static fbe_status_t database_mark_number_unused(fbe_u32_t max_number,
                                                  fbe_u64_t *bitmap,
                                                  fbe_u32_t in_number)
{
    fbe_u32_t 					offset;
    fbe_u32_t					array_location;
    fbe_u64_t					bit_offset = 0x1;

    database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: #:%d\n",
                       __FUNCTION__, in_number);

    /*sanity check*/
    if (max_number <= in_number){
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s:attempt to clear a numer bigger than max: #:%d, Max:%d\n",
                       __FUNCTION__, in_number, max_number);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    offset = in_number;

    array_location = offset / 64;
    offset %= 64;
    bit_offset <<= offset;

    /*sanity check*/
    if (!(bitmap[array_location] & bit_offset)) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s:location %d already cleared\n",
                       __FUNCTION__, in_number);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    bitmap[array_location] &= ~bit_offset;

    return FBE_STATUS_OK;
}

/*while it seems stupid at first to do a brut force search every time we create a lun or a RG to search for a free number
it's very fast, checp and bug free comparing to keeping a cache of the used lun numbers. Taken less than 1ms to go through all obejects
If we keep a cache, it has to be up to date on boths sides with multiple code paths to cover.
We also have to upate the cache when removing objects or starting to load the system.
This create multiple code changes and more chances for bug, hence we taken the simplest approach*/
static fbe_status_t database_fill_free_entry_bitmap(fbe_u32_t max_number,
                                                    database_table_t *table_ptr,
                                                    fbe_u64_t *bitmap,
                                                    database_class_id_t db_class_id)
{
    fbe_u32_t table_index = FBE_RESERVED_OBJECT_IDS;
    fbe_status_t status;
    database_user_entry_t * user_entry;

    for (;table_index < table_ptr->table_size; table_index++) {
        status = fbe_database_config_table_get_user_entry_by_object_id(table_ptr, table_index, &user_entry);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s:can't get user entry for id %d\n",
                               __FUNCTION__, table_index);
            return status;
        }

        if (user_entry->header.state == DATABASE_CONFIG_ENTRY_INVALID ||
            user_entry->db_class_id == DATABASE_CLASS_ID_VIRTUAL_DRIVE ||
            user_entry->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE ||
            ((user_entry->db_class_id != DATABASE_CLASS_ID_LUN) && db_class_id == DATABASE_CLASS_ID_LUN) ||
            ((db_class_id> DATABASE_CLASS_ID_RAID_START && db_class_id < DATABASE_CLASS_ID_RAID_END)&&(user_entry->db_class_id < DATABASE_CLASS_ID_RAID_START ||
            user_entry->db_class_id > DATABASE_CLASS_ID_RAID_END))) {
            continue;/*nothing to do here*/
        }

        
        if (db_class_id == DATABASE_CLASS_ID_LUN && user_entry->user_data_union.lu_user_data.lun_number != FBE_LUN_ID_INVALID) {
            database_mark_lun_number_used(user_entry->user_data_union.lu_user_data.lun_number, bitmap);
        }else if (db_class_id> DATABASE_CLASS_ID_RAID_START && 
                  db_class_id < DATABASE_CLASS_ID_RAID_END &&
                  user_entry->user_data_union.rg_user_data.raid_group_number != FBE_RAID_ID_INVALID) {
            database_mark_rg_number_used(user_entry->user_data_union.rg_user_data.raid_group_number, bitmap);
        }
        else if ((db_class_id == DATABASE_CLASS_ID_EXTENT_POOL_LUN) && 
                 (user_entry->db_class_id == DATABASE_CLASS_ID_EXTENT_POOL_LUN || user_entry->db_class_id == DATABASE_CLASS_ID_EXTENT_POOL_METADATA_LUN) &&
                 (user_entry->user_data_union.ext_pool_lun_user_data.lun_id != FBE_LUN_ID_INVALID)) {
            database_mark_rg_number_used(user_entry->user_data_union.ext_pool_lun_user_data.lun_id, bitmap);
        }
        else if ((db_class_id == DATABASE_CLASS_ID_EXTENT_POOL) && 
                 (user_entry->db_class_id == DATABASE_CLASS_ID_EXTENT_POOL) &&
                 (user_entry->user_data_union.ext_pool_user_data.pool_id != FBE_POOL_ID_INVALID)) {
            database_mark_rg_number_used(user_entry->user_data_union.ext_pool_user_data.pool_id, bitmap);
        }

    }

    return FBE_STATUS_OK;

}

/*Look up LUN by lun WWID*/
static fbe_status_t database_control_lookup_lun_by_wwn(fbe_packet_t * packet)
{
    fbe_status_t                     status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_lookup_lun_by_wwn_t *lookup_lun = NULL;  
    database_user_entry_t           *out_entry_ptr = NULL;     

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&lookup_lun, sizeof(*lookup_lun));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    status = fbe_database_config_table_get_user_entry_by_lun_wwn(&fbe_database_service.user_table, 
                                                                lookup_lun->lun_wwid,
                                                                &out_entry_ptr);
    if ((status == FBE_STATUS_OK)&&(out_entry_ptr != NULL)) {
        /* get the entry ok */
        if (out_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) {
            /* only return the object when the entry is valid */
            lookup_lun->object_id = out_entry_ptr->header.object_id;
        }
    }else{
        lookup_lun->object_id = FBE_OBJECT_ID_INVALID;
        /*trace to a low level since many times it is used just to look for the next free lun number and it will be confusing in traces*/
        database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to find LUN  returned 0x%x.\n",__FUNCTION__,  status);
        status = FBE_STATUS_NO_OBJECT;
    }
    fbe_database_complete_packet(packet, status);
    return status;
}

/*!***************************************************************
 * @fn database_control_clear_pending_transaction
 *****************************************************************
 * @brief
 *   clear the current pending transaction.
 *
 * @param packet_p - control packet pointer
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    4/08/2012: created
 *
 ****************************************************************/
static fbe_status_t database_control_clear_pending_transaction(fbe_packet_t* packet_p)
{
    fbe_status_t status;
    status =  fbe_database_transaction_abort(&fbe_database_service);
    
    fbe_database_complete_packet(packet_p, status);
    return status;    
}

/* If upgrade commmit is done, return true, else return false*/
fbe_bool_t fbe_database_ndu_is_committed(void)
{
    fbe_bool_t is_committed = FBE_TRUE;

    fbe_spinlock_lock(&fbe_database_service.system_db_header_lock);
    if (SEP_PACKAGE_VERSION != fbe_database_service.system_db_header.persisted_sep_version)
    {
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: not committed, persisted version 0x%llx, current version 0x%x\n", 
                       __FUNCTION__, fbe_database_service.system_db_header.persisted_sep_version, SEP_PACKAGE_VERSION);
        is_committed = FBE_FALSE;
    }
    if ((INVALID_SEP_PACKAGE_VERSION != fbe_database_service.peer_sep_version) &&
        ((SEP_PACKAGE_VERSION != fbe_database_service.peer_sep_version) ||
         (fbe_database_service.peer_sep_version != fbe_database_service.system_db_header.persisted_sep_version)))
    {
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: not committed, persisted version 0x%llx, peer version 0x%llx\n", 
                       __FUNCTION__, fbe_database_service.system_db_header.persisted_sep_version, fbe_database_service.peer_sep_version);
        is_committed = FBE_FALSE;
    }
    fbe_spinlock_unlock(&fbe_database_service.system_db_header_lock);

    return is_committed;
}

/*** HACK to handle structure mismatch between snake and pre-snake */
fbe_bool_t fbe_database_is_peer_running_pre_snake_river_release(void)
{
    fbe_bool_t peer_alive;

    peer_alive = database_common_peer_is_alive();
    if((fbe_database_service.peer_sep_version < FLARE_COMMIT_LEVEL_ROCKIES_MR1_CBE) &&
       peer_alive)
    {
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: Peer SP running pre-snake release :%d \n", 
                       __FUNCTION__, (unsigned int)fbe_database_service.peer_sep_version);
        return FBE_TRUE;
    } else {
        return FBE_FALSE;
    }
}

fbe_status_t fbe_database_get_committed_nonpaged_metadata_size(fbe_class_id_t class_id, fbe_u32_t *size)
{
    fbe_status_t status = FBE_STATUS_OK;
    if (size == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: input argument is NULL\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_spinlock_lock(&fbe_database_service.system_db_header_lock);
    switch(class_id) {
    case FBE_CLASS_ID_PROVISION_DRIVE:
        *size = fbe_database_service.system_db_header.pvd_nonpaged_metadata_size;
        break;
    case FBE_CLASS_ID_LUN:
        *size = fbe_database_service.system_db_header.lun_nonpaged_metadata_size;
        break;
    case FBE_CLASS_ID_RAID_GROUP:
    case FBE_CLASS_ID_MIRROR:
    case FBE_CLASS_ID_STRIPER:
    case FBE_CLASS_ID_PARITY:
    case FBE_CLASS_ID_VIRTUAL_DRIVE:
        *size = fbe_database_service.system_db_header.raid_nonpaged_metadata_size;
        break;
    case FBE_CLASS_ID_EXTENT_POOL:
        *size = fbe_database_service.system_db_header.ext_pool_nonpaged_metadata_size;
        break;
    case FBE_CLASS_ID_EXTENT_POOL_LUN:
    case FBE_CLASS_ID_EXTENT_POOL_METADATA_LUN:
        *size = fbe_database_service.system_db_header.ext_pool_lun_nonpaged_metadata_size;
        break;
    default:
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: No size info for class_id = %d\n", 
                       __FUNCTION__, class_id);
        status = FBE_STATUS_GENERIC_FAILURE;
        break;
    }
    fbe_spinlock_unlock(&fbe_database_service.system_db_header_lock);

    return status;
}


static fbe_status_t database_control_get_all_luns(fbe_packet_t *packet)
{
    fbe_status_t                     		status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_get_all_luns_t *	get_all_luns = NULL;  
    fbe_object_id_t							object_id;
    fbe_database_lun_info_t *				lun_info = NULL;
    fbe_payload_ex_t *                  	payload = NULL;
    fbe_sg_element_t *                      sg_element = NULL;
    fbe_u32_t                               sg_elements = 0;
    database_object_entry_t             *	object_entry = NULL;
    fbe_u32_t 								number_of_luns_requested = 0;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&get_all_luns, sizeof(fbe_database_control_get_all_luns_t));
    if ( status != FBE_STATUS_OK ){
        fbe_database_complete_packet(packet, status);
        return status;
    }

    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_elements);

    /*point to start of the buffer*/
    lun_info = (fbe_database_lun_info_t *)sg_element->address;

    /*sanity check*/
    if ((sg_element->count / sizeof(fbe_database_lun_info_t)) != get_all_luns->number_of_luns_requested){
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s Byte count: 0x%x doesn't agree with num objects: %d.\n",
                       __FUNCTION__, sg_element->count, get_all_luns->number_of_luns_requested);

        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    database_trace(FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,"<<<< %s, START please don't call on high frequency>>>>>\n",__FUNCTION__);
    fbe_database_poll_record_poll(POLL_REQUEST_GET_ALL_LUNS); 

    number_of_luns_requested = get_all_luns->number_of_luns_requested;
    /*go over the tables, get objects that are luns and send them the database_get_lun_info*/
    for (object_id = 0; object_id < fbe_database_service.object_table.table_size && number_of_luns_requested > 0; object_id++) {
        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                  object_id,
                                                                  &object_entry); 
        
         
        if ( status == FBE_STATUS_OK ){
            if (database_is_entry_valid(&object_entry->header) &&
                ((object_entry->db_class_id == DATABASE_CLASS_ID_LUN) ||
                 object_entry->db_class_id == DATABASE_CLASS_ID_EXTENT_POOL_LUN)) {
                lun_info->lun_object_id = object_id;
                /*first get the basic stuff*/
                status = database_get_lun_info(lun_info, packet);
                if (status != FBE_STATUS_OK) {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: can't get lun info for object 0x%X.\n",__FUNCTION__, object_id);
                    continue;
                }else{
                    lun_info++;/*point to next memory address*/
                    get_all_luns->number_of_luns_returned++;
                    number_of_luns_requested--;
                }
                
            }
        }
        else{
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get object entry for object 0x%X which may be destroyed. \n",__FUNCTION__, object_id);
            continue;
        }
    }

    /*make sure it is not abused*/
    database_trace(FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,"<<<< %s, END please don't call on high frequency>>>>>\n",__FUNCTION__);
    
    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;

}

static fbe_status_t database_get_lun_info(fbe_database_lun_info_t *lun_info, fbe_packet_t *packet)
{
    database_edge_entry_t               *edge_to_raid = NULL;
    database_user_entry_t               *lun = NULL;	
    database_object_entry_t             *object_entry = NULL;
    fbe_lba_t                           lun_capacity = 0;
    fbe_block_count_t 					cache_zero_bit_map_size;
    fbe_status_t                     	status = FBE_STATUS_GENERIC_FAILURE;
    database_user_entry_t               *rg_user = NULL;	
    database_object_entry_t             *rg_object = NULL;	
    fbe_u32_t                           index;

    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              lun_info->lun_object_id,
                                                              &object_entry);  

    if ((status == FBE_STATUS_OK) && database_is_entry_valid(&object_entry->header)) {
        if (object_entry->db_class_id == DATABASE_CLASS_ID_EXTENT_POOL_LUN) {
            return database_get_ext_lun_info(lun_info, packet);
        }
    } else {
        /* This can be WARNING because there is a chance that the LUN has just removed but the database has not updated
         * at the time that Navicimom queries for the LUN info. 
         */
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get object entry for object %u\n", 
                       __FUNCTION__, 
                       lun_info->lun_object_id);
    }

    /* Get the edge to the raid object */		
    status = fbe_database_config_table_get_edge_entry(&fbe_database_service.edge_table,
                                                      lun_info->lun_object_id,
                                                      0,
                                                      &edge_to_raid);  
   
    if ( status == FBE_STATUS_OK )
    {
        /* Get the object table entry for lun capacity */		
        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                  lun_info->lun_object_id,
                                                                  &object_entry);  

        if ((status == FBE_STATUS_OK) && database_is_entry_valid(&object_entry->header))
        {
            lun_info->overall_capacity = object_entry->set_config_union.lu_config.capacity; 
            lun_capacity = object_entry->set_config_union.lu_config.capacity;
            /*we need to hide from the user the fact we created a bit more for sp cache to use*/
            fbe_lun_calculate_cache_zero_bit_map_size_to_remove(lun_capacity, &cache_zero_bit_map_size);
            lun_capacity -= cache_zero_bit_map_size;
        }
        else
        {
            /* This can be WARNING because there is a chance that the LUN has just removed but the database has not updated
             * at the time that Navicimom queries for the LUN info. 
             */
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get object entry for object %u\n", 
                           __FUNCTION__, 
                           lun_info->lun_object_id);
        }
    }
    else
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get lun edge for object %u\n", 
                       __FUNCTION__, 
                       lun_info->lun_object_id);
    }

    if ( status == FBE_STATUS_OK )
    {   
        status = fbe_database_forward_packet_to_object(packet, FBE_RAID_GROUP_CONTROL_CODE_GET_INFO,
                                                    &lun_info->raid_info,
                                                    sizeof(lun_info->raid_info),
                                                    edge_to_raid->server_id,
                                                    NULL,
                                                    0,
                                                    FBE_PACKET_FLAG_INTERNAL,
                                                    FBE_PACKAGE_ID_SEP_0);
        if ( status != FBE_STATUS_OK )
        {
            /* This can be WARNING because there is a chance that the LUN has just removed but the database has not updated
             * at the time that Navicimom queries for the LUN info. 
             */
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: failed to get raid_info for object %u\n", 
                         __FUNCTION__,
                         edge_to_raid->server_id);
        }
    }
    if ( status == FBE_STATUS_OK )
    {
        /* Lookup the lun object */     
        status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                                       lun_info->lun_object_id,
                                                                       &lun);

        if ( status != FBE_STATUS_OK || !database_is_entry_valid(&lun->header) )
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                         "%s: failed to lun object %u\n", 
                         __FUNCTION__,
                         lun_info->lun_object_id);
        }
    }
    if ( status == FBE_STATUS_OK )
    {

        /*  Fill in any LUN specific data here. RAID Specific
         *  information was already retrieved above. */       
        lun_info->capacity = lun_capacity;
        lun_info->offset = edge_to_raid->offset;        
        lun_info->bind_time = lun->user_data_union.lu_user_data.bind_time;
        lun_info->user_private = lun->user_data_union.lu_user_data.user_private;
        fbe_copy_memory(&lun_info->world_wide_name, &lun->user_data_union.lu_user_data.world_wide_name, 
                        sizeof(lun->user_data_union.lu_user_data.world_wide_name));
        fbe_copy_memory(&lun_info->user_defined_name, &lun->user_data_union.lu_user_data.user_defined_name, 
                        sizeof(lun->user_data_union.lu_user_data.user_defined_name));
        lun_info->raid_group_obj_id = edge_to_raid->server_id;
        lun_info->lun_characteristics = 0;

        /*make sure we mark LU attributes correctly*/
        lun_info->is_degraded = FBE_FALSE;
        lun_info->attributes = lun->user_data_union.lu_user_data.attributes;
        /*need to set the lu attribute if the lun is degraded. */
        for (index = 0; index < lun_info->raid_info.width; index++) {
            if(lun_info->raid_info.rebuild_checkpoint[index] != FBE_LBA_INVALID) {
                //lun_info->attributes |= VOL_ATTR_RAID_PROTECTION_DEGRADED;
                lun_info->is_degraded = FBE_TRUE;
                break;
            }
        }
        
        /*for VCX we need to add more bits*/
        if (fbe_private_space_layout_object_id_is_system_lun(lun_info->lun_object_id)) {

            lun_info->attributes = 0;/*for these we never change their attributes so we can start with 0*/
            lun_info->attributes |= (FBE_LU_SP_ONLY | FBE_LU_SYSTEM);
            status = database_map_psl_lun_to_attributes(lun_info->lun_object_id, &lun_info->attributes);

            /*if it's any of the VCX luns, we'll set it's characteristics as FLARE_LUN_CHARACTERISTIC_CELERRA*/
            if (lun_info->attributes & FBE_LU_ATTRIBUTES_VCX_MASK) {
                lun_info->lun_characteristics = FBE_LUN_CHARACTERISTIC_CELERRA; 
            }

        }else{
            lun_info->attributes |= FBE_LU_USER;
        }

        /*the rest is mostly for Admin*/
        lun_info->lun_number = lun->user_data_union.lu_user_data.lun_number;

        status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                                       edge_to_raid->server_id,
                                                                       &rg_user); 

        if (status != FBE_STATUS_OK || !database_is_entry_valid(&rg_user->header)) {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get RG user table entry for 0x%X\n",__FUNCTION__, lun_info->lun_object_id);
            return status;
        }

        lun_info->rg_number = rg_user->user_data_union.rg_user_data.raid_group_number;

        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                       edge_to_raid->server_id,
                                                                       &rg_object); 

        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get RG object table entry for 0x%X\n",__FUNCTION__, lun_info->lun_object_id);
            return status;
        }

        database_set_lun_rotational_rate(rg_object, &lun_info->rotational_rate);

        /*get the zero status for the LUN*/
        status = database_get_lun_zero_status(lun_info->lun_object_id, &lun_info->lun_zero_status, packet );
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_WARNING,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: failed to get zero status for 0x%X\n",__FUNCTION__, lun_info->lun_object_id);
            lun_info->lun_zero_status.zero_checkpoint = 0;
            lun_info->lun_zero_status.zero_percentage = 0;
        }

        /*lifecycle*/
        status = fbe_database_get_object_lifecycle_state(lun_info->lun_object_id, &lun_info->lifecycle_state, packet);
        if ( status != FBE_STATUS_OK ){
            database_trace(FBE_TRACE_LEVEL_WARNING,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: failed to get lifecycle for object 0x%X\n", 
                           __FUNCTION__,
                           lun_info->lun_object_id);
            return status;
        }

        /* rebuild_status */
        status = fbe_database_forward_packet_to_object(packet, FBE_LUN_CONTROL_CODE_GET_REBUILD_STATUS,
                                                    &lun_info->rebuild_status,
                                                    sizeof(fbe_lun_rebuild_status_t),
                                                    lun_info->lun_object_id,
                                                    NULL,
                                                    0,
                                                    FBE_PACKET_FLAG_NO_ATTRIB,
                                                    FBE_PACKAGE_ID_SEP_0);
        if ( status != FBE_STATUS_OK ){
            database_trace(FBE_TRACE_LEVEL_WARNING,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: failed to get rebuild status for object 0x%X\n", 
                           __FUNCTION__,
                           lun_info->lun_object_id);
            lun_info->rebuild_status.rebuild_checkpoint = 0;
            lun_info->rebuild_status.rebuild_percentage = 0;
        }

    }    

    return status;

}

static fbe_status_t database_get_ext_lun_info(fbe_database_lun_info_t *lun_info, fbe_packet_t *packet)
{
    database_object_entry_t             *object_entry = NULL;
    database_user_entry_t               *lun_user_entry = NULL;
    database_user_entry_t               *pool_user_entry = NULL;
    fbe_lba_t                           lun_capacity = 0;
    fbe_block_count_t 					cache_zero_bit_map_size;
    fbe_status_t                     	status = FBE_STATUS_GENERIC_FAILURE;

    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              lun_info->lun_object_id,
                                                              &object_entry);  

    if ((status == FBE_STATUS_OK) && database_is_entry_valid(&object_entry->header)) {
        lun_info->overall_capacity = object_entry->set_config_union.ext_pool_lun_config.capacity; 
        lun_capacity = object_entry->set_config_union.ext_pool_lun_config.capacity;
        /*we need to hide from the user the fact we created a bit more for sp cache to use*/
        fbe_lun_calculate_cache_zero_bit_map_size_to_remove(lun_capacity, &cache_zero_bit_map_size);
        lun_capacity -= cache_zero_bit_map_size;
    } else {
        /* This can be WARNING because there is a chance that the LUN has just removed but the database has not updated
         * at the time that Navicimom queries for the LUN info. 
         */
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get object entry for object %u\n", 
                       __FUNCTION__, 
                       lun_info->lun_object_id);
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                                   lun_info->lun_object_id,
                                                                   &lun_user_entry);  
    if ((status == FBE_STATUS_OK) && database_is_entry_valid(&lun_user_entry->header)) {
        status = fbe_database_config_table_get_user_entry_by_ext_pool_id(&fbe_database_service.user_table,
                                                                         lun_user_entry->user_data_union.ext_pool_lun_user_data.pool_id,
                                                                         &pool_user_entry);
        status = fbe_database_forward_packet_to_object(packet, FBE_RAID_GROUP_CONTROL_CODE_GET_INFO,
                                                       &lun_info->raid_info,
                                                       sizeof(lun_info->raid_info),
                                                       pool_user_entry->header.object_id,
                                                       NULL,
                                                       0,
                                                       FBE_PACKET_FLAG_INTERNAL,
                                                       FBE_PACKAGE_ID_SEP_0);
        if ( status != FBE_STATUS_OK ) {
            /* This can be WARNING because there is a chance that the LUN has just removed but the database has not updated
             * at the time that Navicimom queries for the LUN info. 
             */
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get raid_info for object %u\n", 
                           __FUNCTION__,
                           pool_user_entry->header.object_id);
        }
    }
    if ( status == FBE_STATUS_OK )
    {

        /*  Fill in any LUN specific data here. RAID Specific
         *  information was already retrieved above. */       
        lun_info->capacity = lun_capacity;
        lun_info->offset = object_entry->set_config_union.ext_pool_lun_config.offset;        
        lun_info->bind_time = 0;
        lun_info->user_private = FBE_FALSE;
        lun_info->world_wide_name = lun_user_entry->user_data_union.ext_pool_lun_user_data.world_wide_name;
        lun_info->user_defined_name = lun_user_entry->user_data_union.ext_pool_lun_user_data.user_defined_name;
        lun_info->raid_group_obj_id = pool_user_entry->header.object_id;
        lun_info->lun_characteristics = 0;

        /*make sure we mark LU attributes correctly*/
        lun_info->is_degraded = FBE_FALSE;
        lun_info->attributes = 0;//lun->user_data_union.lu_user_data.attributes;

        /*the rest is mostly for Admin*/
        lun_info->lun_number = lun_user_entry->user_data_union.ext_pool_lun_user_data.lun_id;

        lun_info->rg_number = pool_user_entry->user_data_union.ext_pool_user_data.pool_id;

        lun_info->rotational_rate = 0; // sas only

        /*get the zero status for the LUN*/
        lun_info->lun_zero_status.zero_checkpoint = FBE_LBA_INVALID;
        lun_info->lun_zero_status.zero_percentage = 100;

        /*lifecycle*/
        status = fbe_database_get_object_lifecycle_state(lun_info->lun_object_id, &lun_info->lifecycle_state, packet);
        if ( status != FBE_STATUS_OK ){
            database_trace(FBE_TRACE_LEVEL_WARNING,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                           "%s: failed to get lifecycle for object 0x%X\n", 
                           __FUNCTION__,
                           lun_info->lun_object_id);
            return status;
        }

       
        lun_info->rebuild_status.rebuild_checkpoint = FBE_LBA_INVALID;
        lun_info->rebuild_status.rebuild_percentage = 100;
    }
    return status;

}
/*!***************************************************************
 * @fn database_control_get_all_raid_groups()
 *****************************************************************
 * @brief
 *   Control function to get all raid groups in one shot.
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    04/30/2012: Vera created
 *
 ****************************************************************/
static fbe_status_t database_control_get_all_raid_groups(fbe_packet_t *packet)
{
    fbe_status_t                     		status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_get_all_rgs_t *	get_all_rgs = NULL;  
    fbe_database_raid_group_info_t *		rg_info = NULL;
    fbe_payload_ex_t *                  	payload = NULL;
    fbe_sg_element_t *                      sg_element = NULL;
    fbe_u32_t                               sg_elements = 0;
    database_object_entry_t *	            object_entry = NULL;
    fbe_object_id_t							object_id;
    fbe_u32_t 								number_of_rgs_requested = 0;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&get_all_rgs, sizeof(fbe_database_control_get_all_rgs_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_elements);

    /*point to start of the buffer*/
    rg_info = (fbe_database_raid_group_info_t *)sg_element->address;

    /*sanity check*/
    if ((sg_element->count / sizeof(fbe_database_raid_group_info_t)) != get_all_rgs->number_of_rgs_requested){
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s Byte count: 0x%x doesn't agree with num objects: %d.\n",
                       __FUNCTION__, sg_element->count, get_all_rgs->number_of_rgs_requested);

        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    database_trace(FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,"<<<< %s, START please don't call on high frequency>>>>>\n",__FUNCTION__);
    fbe_database_poll_record_poll(POLL_REQUEST_GET_ALL_RAID_GROUPS); 

    number_of_rgs_requested = get_all_rgs->number_of_rgs_requested;
    /*go over the tables, get objects that are rgs and send them the database_get_raid_group_info*/
    for (object_id = 0; object_id < fbe_database_service.object_table.table_size && number_of_rgs_requested > 0; object_id++) {
        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                  object_id,
                                                                  &object_entry);  

        if ( status != FBE_STATUS_OK){
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get object entry for object 0x%X\n",__FUNCTION__, object_id);
            continue;
        }
        if (database_is_entry_valid(&object_entry->header) &&
            object_entry->db_class_id > DATABASE_CLASS_ID_RAID_START && 
            object_entry->db_class_id < DATABASE_CLASS_ID_RAID_END) {                      
            /* first get the basic stuff. For RAID10, we won't return info for the internal rg.*/
            if (object_entry->set_config_union.rg_config.raid_type != FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)
            {
                rg_info->rg_object_id = object_id; 
                status = database_get_raid_group_info(rg_info, packet);
                if (status != FBE_STATUS_OK) {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: can't get raid group info for object 0x%X\n",__FUNCTION__, object_id);
                    continue;
                }
                else{
                    rg_info++;/*point to next memory address*/
                    get_all_rgs->number_of_rgs_returned++;
                    number_of_rgs_requested--;
                }
            }
        }
#ifndef NO_EXT_POOL_ALIAS
        else if (database_is_entry_valid(&object_entry->header) &&
            object_entry->db_class_id == DATABASE_CLASS_ID_EXTENT_POOL) {                      
            rg_info->rg_object_id = object_id; 
            status = database_get_raid_group_info(rg_info, packet);
            if (status != FBE_STATUS_OK) {
                database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: can't get raid group info for object 0x%X\n",__FUNCTION__, object_id);
                continue;
            }
            else{
                rg_info++;/*point to next memory address*/
                get_all_rgs->number_of_rgs_returned++;
                number_of_rgs_requested--;
            }
        }
#endif
    }

    /*make sure it is not abused*/
    database_trace(FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,"<<<< %s, END please don't call on high frequency>>>>>\n",__FUNCTION__);
    
    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;

}

static fbe_status_t 
database_get_raid_group_info_for_ext_pool(fbe_database_raid_group_info_t *rg_info, fbe_packet_t *packet)
{	
    fbe_status_t                     	     status = FBE_STATUS_GENERIC_FAILURE;
    database_user_entry_t                    *user_entry = NULL;

    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table, 
                                                                   rg_info->rg_object_id,
                                                                   &user_entry);
   
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get user_entry for object 0x%X\n", 
                       __FUNCTION__,
                       rg_info->rg_object_id);
        return status;
    }
    /* only fill user_private and rg_number when the user entry is valid and not empty. */
    if ((user_entry != NULL)&&(user_entry->header.state == DATABASE_CONFIG_ENTRY_VALID))
    {
        /* fill in user private*/
        rg_info->rg_info.user_private = FBE_FALSE;
        /* fill in rg number */     
        rg_info->rg_number = user_entry->user_data_union.ext_pool_user_data.pool_id;
    } 
    else 
    {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: user_entry for object 0x%X is invalid.\n", 
                       __FUNCTION__,
                       rg_info->rg_object_id);
    }

    /* fill in rg_info->rg_info. */
    status = fbe_database_forward_packet_to_object(packet, FBE_RAID_GROUP_CONTROL_CODE_GET_INFO,
                                                &rg_info->rg_info,
                                                sizeof(rg_info->rg_info),
                                                rg_info->rg_object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_INTERNAL,
                                                FBE_PACKAGE_ID_SEP_0);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get raid_info for object 0x%X\n", 
                       __FUNCTION__,
                       rg_info->rg_object_id);
        return status;
    }

    /* fill in rg_info->power_saving_policy */
    status = fbe_database_forward_packet_to_object(packet, 
                                                   FBE_RAID_GROUP_CONTROL_CODE_GET_POWER_SAVING_PARAMETERS,
                                                &rg_info->power_saving_policy,
                                                sizeof(rg_info->power_saving_policy),
                                                rg_info->rg_object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get power saving policy for object 0x%X\n", 
                       __FUNCTION__,
                       rg_info->rg_object_id);
        return status;
    }

    status = fbe_database_get_rg_free_contigous_capacity(rg_info, packet);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get raid group free contigous capacity for object 0x%X\n",
                       __FUNCTION__,
                       rg_info->rg_object_id);
        return status;
    }

    /* Get the pvd list based on RG Obj ID */
    status = fbe_database_get_pvd_list(rg_info, packet);
    if ( status != FBE_STATUS_OK )
    {
        /* This will call fbe_database_get_pvd_list_inter_phase() and ends up calling fbe_database_get_pvd_for_vd(). 
         * Set to WARNING since it may be in the middle of swapping out the edge.
         */
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get PVD list for RG Obj: 0x%X\n", 
                       __FUNCTION__,
                       rg_info->rg_object_id);
        return status;
    }
    rg_info->pvd_count = rg_info->rg_info.width; 

    /* fill in lun list */
    status = fbe_database_get_lun_list(rg_info, packet);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get lun list for object 0x%X\n", 
                       __FUNCTION__,
                       rg_info->rg_object_id);
        return status;
    }

    /* fill in the free non contigous capacity. */
#if 0
    status = fbe_database_get_rg_free_non_contigous_capacity(rg_info, packet);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get raid group free non contigous capacity for object 0x%X\n", 
                       __FUNCTION__,
                       rg_info->rg_object_id);
        return status;
    }
#endif
    rg_info->free_non_contigous_capacity = rg_info->extent_size.extent_size;

    /*lifecycle*/
    status = fbe_database_get_object_lifecycle_state(rg_info->rg_object_id, &rg_info->lifecycle_state, packet);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get lifecycle for object 0x%X\n", 
                       __FUNCTION__,
                       rg_info->rg_object_id);
        return status;
    }                                                              

    /* fill in raid group power save capable */
    rg_info->power_save_capable = FBE_FALSE;

    return status;
}

/*!***************************************************************
 * @fn database_get_raid_group_info()
 *****************************************************************
 * @brief
 *   Function to get_raid_group_info
 *
 * @param fbe_database_raid_group_info_t *rg_info
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    04/30/2012: Vera Wang created
 *
 ****************************************************************/
static fbe_status_t database_get_raid_group_info(fbe_database_raid_group_info_t *rg_info, fbe_packet_t *packet)
{	
    fbe_status_t                     	     status = FBE_STATUS_GENERIC_FAILURE;
    database_user_entry_t                    *user_entry = NULL;

    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table, 
                                                                   rg_info->rg_object_id,
                                                                   &user_entry);
   
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get user_entry for object 0x%X\n", 
                       __FUNCTION__,
                       rg_info->rg_object_id);
        return status;
    }
#ifndef NO_EXT_POOL_ALIAS
    if ((user_entry != NULL) && (user_entry->header.state == DATABASE_CONFIG_ENTRY_VALID) &&
        (user_entry->db_class_id == DATABASE_CLASS_ID_EXTENT_POOL))
    {
        return database_get_raid_group_info_for_ext_pool(rg_info, packet);
    }
#endif

    /* only fill user_private and rg_number when the user entry is valid and not empty. */
    if ((user_entry != NULL)&&(user_entry->header.state == DATABASE_CONFIG_ENTRY_VALID))
    {
        /* fill in user private*/
        rg_info->rg_info.user_private = user_entry->user_data_union.rg_user_data.user_private;
        /* fill in rg number */     
        rg_info->rg_number = user_entry->user_data_union.rg_user_data.raid_group_number;
    } 
    else 
    {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: user_entry for object 0x%X is invalid.\n", 
                       __FUNCTION__,
                       rg_info->rg_object_id);
    }

    /* fill in rg_info->rg_info. */
    status = fbe_database_forward_packet_to_object(packet, FBE_RAID_GROUP_CONTROL_CODE_GET_INFO,
                                                &rg_info->rg_info,
                                                sizeof(rg_info->rg_info),
                                                rg_info->rg_object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_INTERNAL,
                                                FBE_PACKAGE_ID_SEP_0);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get raid_info for object 0x%X\n", 
                       __FUNCTION__,
                       rg_info->rg_object_id);
        return status;
    }

    /* fill in rg_info->power_saving_policy */
    status = fbe_database_forward_packet_to_object(packet, 
                                                   FBE_RAID_GROUP_CONTROL_CODE_GET_POWER_SAVING_PARAMETERS,
                                                &rg_info->power_saving_policy,
                                                sizeof(rg_info->power_saving_policy),
                                                rg_info->rg_object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get power saving policy for object 0x%X\n", 
                       __FUNCTION__,
                       rg_info->rg_object_id);
        return status;
    }

    status = fbe_database_get_rg_free_contigous_capacity(rg_info, packet);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get raid group free contigous capacity for object 0x%X\n",
                       __FUNCTION__,
                       rg_info->rg_object_id);
        return status;
    }

    /* Get the pvd list based on RG Obj ID */
    status = fbe_database_get_pvd_list(rg_info, packet);
    if ( status != FBE_STATUS_OK )
    {
        /* This will call fbe_database_get_pvd_list_inter_phase() and ends up calling fbe_database_get_pvd_for_vd(). 
         * Set to WARNING since it may be in the middle of swapping out the edge.
         */
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get PVD list for RG Obj: 0x%X\n", 
                       __FUNCTION__,
                       rg_info->rg_object_id);
        return status;
    }

    /* fill in lun list */
    status = fbe_database_get_lun_list(rg_info, packet);

    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get lun list for object 0x%X\n", 
                       __FUNCTION__,
                       rg_info->rg_object_id);
        return status;
    }

    /* fill in the free non contigous capacity. */
    status = fbe_database_get_rg_free_non_contigous_capacity(rg_info, packet);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get raid group free non contigous capacity for object 0x%X\n", 
                       __FUNCTION__,
                       rg_info->rg_object_id);
        return status;
    }

    /*lifecycle*/
    status = fbe_database_get_object_lifecycle_state(rg_info->rg_object_id, &rg_info->lifecycle_state, packet);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get lifecycle for object 0x%X\n", 
                       __FUNCTION__,
                       rg_info->rg_object_id);
        return status;
    }                                                              

    /* fill in raid group power save capable */
    status = fbe_database_get_rg_power_save_capable(rg_info, packet);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s:can't send raid group power_save_capable for object 0x%X, by default, set to FALSE\n", 
                       __FUNCTION__,
                       rg_info->rg_object_id);

        rg_info->power_save_capable = FBE_FALSE;
        
    }

    return status;
}

/*!***************************************************************
 * @fn database_control_get_all_pvds
 *****************************************************************
 * @brief
 *   Control function to get all PVDS in one shot.
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    05/02/2012: Vera Wang created
 *
 ****************************************************************/
static fbe_status_t database_control_get_all_pvds(fbe_packet_t *packet)
{
    fbe_status_t                     		status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_get_all_pvds_t *	get_all_pvds = NULL;  
    fbe_database_pvd_info_t *		        pvd_info = NULL;
    fbe_payload_ex_t *                  	payload = NULL;
    fbe_sg_element_t *                      sg_element = NULL;
    fbe_u32_t                               sg_elements = 0;
    database_object_entry_t *	            object_entry = NULL;
    fbe_object_id_t							object_id;
    fbe_u32_t 								number_of_pvds_requested = 0;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&get_all_pvds, sizeof(fbe_database_control_get_all_pvds_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_elements);

    /*point to start of the buffer*/
    pvd_info = (fbe_database_pvd_info_t *)sg_element->address;

    /*sanity check*/
    if ((sg_element->count / sizeof(fbe_database_pvd_info_t)) != get_all_pvds->number_of_pvds_requested){
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s Byte count: 0x%x doesn't agree with num objects: %d.\n",
                       __FUNCTION__, sg_element->count, get_all_pvds->number_of_pvds_requested);

        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    database_trace(FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,"<<<< %s, START please don't call on high frequency>>>>>\n",__FUNCTION__);
    fbe_database_poll_record_poll(POLL_REQUEST_GET_ALL_PVDS); 

    number_of_pvds_requested = get_all_pvds->number_of_pvds_requested;
    /*go over the tables, get objects that are pvds and send them the database_get_pvd_info*/
    for (object_id = 0; object_id < fbe_database_service.object_table.table_size && number_of_pvds_requested > 0; object_id++) {
        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                  object_id,
                                                                  &object_entry);  

        if ( status != FBE_STATUS_OK ){
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get object entry for object 0x%X\n",__FUNCTION__, object_id);
            continue;
        }
        /* first get the basic stuff*/
        if (database_is_entry_valid(&object_entry->header) &&
            object_entry->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE) {
            pvd_info->pvd_object_id = object_id;
            /*first get the basic stuff*/
            status = database_get_pvd_info(pvd_info, packet);
            if (status != FBE_STATUS_OK) {
                    database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s:can't get pvd info,obj 0x%X\n",__FUNCTION__, object_id);
                    continue;
            }else{
                pvd_info++;/*point to next memory address*/
                get_all_pvds->number_of_pvds_returned++;
                number_of_pvds_requested--;
            }		
        }
    }

    /*make sure it is not abused*/
    database_trace(FBE_TRACE_LEVEL_INFO,FBE_TRACE_MESSAGE_ID_INFO,"<<<< %s, END please don't call on high frequency>>>>>\n",__FUNCTION__);
    
    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;

}

/*!***************************************************************
 * @fn database_get_pvd_info
 *****************************************************************
 * @brief
 *   Function to get pvd info.
 *
 * @param fbe_database_pvd_info_t *pvd_info
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    05/02/2012: Vera Wang created
 *
 ****************************************************************/
static fbe_status_t database_get_pvd_info(fbe_database_pvd_info_t *pvd_info, fbe_packet_t * packet) 
{	
    fbe_status_t                     	     	status = FBE_STATUS_GENERIC_FAILURE;
    fbe_u32_t                                   retry_count = 3;

    /* fill in pvd_info->pvd_info. */
    status = fbe_database_forward_packet_to_object(packet,
                                                   FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PROVISION_DRIVE_INFO,
                                                &pvd_info->pvd_info,
                                                sizeof(pvd_info->pvd_info),
                                                pvd_info->pvd_object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_INTERNAL,
                                                FBE_PACKAGE_ID_SEP_0);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get pvd_info. Obj:0x%X, status:0x%x\n", 
                       __FUNCTION__,
                       pvd_info->pvd_object_id, status);
        return status;
    }

    /* Check the location information from pvd_info->pvd_info*/
    if (pvd_info->pvd_info.port_number  == FBE_PORT_NUMBER_INVALID || 
        pvd_info->pvd_info.enclosure_number == FBE_ENCLOSURE_NUMBER_INVALID ||
        pvd_info->pvd_info.slot_number == FBE_SLOT_NUMBER_INVALID )
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: drive location is invalid(%d_%d_%d)\n",
                       __FUNCTION__,
                       pvd_info->pvd_info.port_number,
                       pvd_info->pvd_info.enclosure_number,
                       pvd_info->pvd_info.slot_number);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    /* fill in pvd_info->zeroing_percentage */
    if(pvd_info->pvd_info.zero_checkpoint >= pvd_info->pvd_info.capacity)
    {
        pvd_info->zeroing_percentage = 100;
    }
    else
    {
        /*Calculate zeroing percentage*/
        pvd_info->zeroing_percentage = (fbe_u16_t)(pvd_info->pvd_info.zero_checkpoint * 100 / pvd_info->pvd_info.capacity);
    }
    /* fill in pvd_info->power_save_info */
    status = fbe_database_forward_packet_to_object(packet, 
                                                   FBE_BASE_CONFIG_CONTROL_CODE_GET_OBJECT_POWER_SAVE_INFO,
                                                &pvd_info->power_save_info,
                                                sizeof(pvd_info->power_save_info),
                                                pvd_info->pvd_object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get power save info. Obj:0x%X, status:0x%x\n", 
                       __FUNCTION__,
                       pvd_info->pvd_object_id, status);
        return status;
    }

    /* fill in pvd pool_id */
    pvd_info->pool_id = FBE_POOL_ID_INVALID;/*some default stuff*/
    status = fbe_database_get_pvd_pool_id(pvd_info->pvd_object_id, &pvd_info->pool_id); 
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get pool id. Obj:0x%X, status:0x%x\n", 
                       __FUNCTION__,
                       pvd_info->pvd_object_id, status);
        return status;
    }

    /* fill in rg list */
    do {       
        status = fbe_database_get_rgs_on_top_of_pvd(pvd_info->pvd_object_id,
                                                    pvd_info->rg_list,
                                                    pvd_info->rg_number_list,
                                                    MAX_RG_COUNT_ON_TOP_OF_PVD,
                                                    &pvd_info->rg_count,
                                                    packet);
        retry_count--;

        if( status != FBE_STATUS_OK ) {
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get rg list. Obj:0x%X, status:0x%x\n", 
                           __FUNCTION__,
                           pvd_info->pvd_object_id, status);
            if (retry_count == 0) {
                return status;
            }    
            fbe_thread_delay(1000);   
        }
    } while ( status != FBE_STATUS_OK && retry_count != 0 );

    /*lifecycle*/
    status = fbe_database_get_object_lifecycle_state(pvd_info->pvd_object_id, &pvd_info->lifecycle_state, packet);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get lifecycle. Obj:0x%X, status:0x%x\n", 
                       __FUNCTION__,
                       pvd_info->pvd_object_id, status);
        return status;
    }

    return status;
}

/*!***************************************************************
 * @fn fbe_database_get_rg_user_private
 *****************************************************************
 * @brief
 *    Function to get the user_private for raid group.
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    05/03/2012: Vera Wang created
 *
 ****************************************************************/
fbe_status_t fbe_database_get_rg_user_private(fbe_object_id_t object_id, fbe_bool_t *user_private)
{
    fbe_status_t 			status = FBE_STATUS_GENERIC_FAILURE;
    database_user_entry_t   *out_entry_ptr = NULL;  

    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,	
                                                                   object_id,
                                                                   &out_entry_ptr);
    if ((status == FBE_STATUS_OK)&&(out_entry_ptr != NULL))
    {
        /* get the entry ok */
        if (out_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID)
        {
            /* copy the data */
            *user_private = out_entry_ptr->user_data_union.rg_user_data.user_private;
        }
    }
    else
    {
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: Failed to get user entry for the RG object 0x%x.\n", 
                       __FUNCTION__, object_id);

        status = FBE_STATUS_GENERIC_FAILURE;
    }

    return status;
}

static fbe_status_t database_control_get_raid_group(fbe_packet_t *packet)
{
    fbe_database_raid_group_info_t		*rg_info;
    fbe_status_t 						status;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&rg_info, sizeof(fbe_database_raid_group_info_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /* This is calling from FBE API to get RG Info */
    status = database_get_raid_group_info(rg_info, packet);
    fbe_database_complete_packet(packet, status);
    return status;


}

static fbe_status_t database_control_get_pvd(fbe_packet_t *packet)
{
    fbe_database_pvd_info_t		*pvd_info;
    fbe_status_t 				status;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&pvd_info, sizeof(fbe_database_pvd_info_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    status = database_get_pvd_info(pvd_info, packet);
    fbe_database_complete_packet(packet, status);
    return status;
}

/*!***************************************************************
* @fn database_control_commit_database_tables
*****************************************************************
* @brief
* update database tables in memory and on disk
* step1: persist new sep version and structure sizes onto disk
* step2: update peer's system db header with new sep version and structure sizes
* step3: update our own system db header with new sep version and structure sizes
*
* @param packet - request packet
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
* @version
*  04/28/2012 - Created. Zhipeng Hu 

****************************************************************/
static fbe_status_t database_control_commit_database_tables(fbe_packet_t * packet)
{
    fbe_status_t                             status  =  FBE_STATUS_OK;
    database_system_db_header_t*   new_system_db_header = NULL;

    /*check whether the sep versions on the two SPs are same
     *only check when we have ever communicated with peer*/
    if(fbe_database_service.peer_sep_version != INVALID_SEP_PACKAGE_VERSION 
        && fbe_database_service.peer_sep_version != SEP_PACKAGE_VERSION)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: the sep versions on the two SPs are not same with each other\n",
                    __FUNCTION__);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* check for global_info and object table first */
    if (fbe_database_is_table_commit_required(DATABASE_CONFIG_TABLE_OBJECT))
    {
        fbe_database_update_table_prior_commit(DATABASE_CONFIG_TABLE_OBJECT);
        status = fbe_database_commit_table(DATABASE_CONFIG_TABLE_OBJECT);
        if (status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: persisting object table failed\n",
                        __FUNCTION__);
            fbe_database_complete_packet(packet, status);
            return status;
        }
    }

    if (fbe_database_is_table_commit_required(DATABASE_CONFIG_TABLE_GLOBAL_INFO))
    {
        fbe_database_update_table_prior_commit(DATABASE_CONFIG_TABLE_GLOBAL_INFO);
        status = fbe_database_commit_table(DATABASE_CONFIG_TABLE_GLOBAL_INFO);
        if (status != FBE_STATUS_OK)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: persisting global info table failed\n",
                        __FUNCTION__);
            fbe_database_complete_packet(packet, status);
            return status;
        }
    }

    /* first initialize the temp system db header with the current software version and structure size */
    new_system_db_header = (database_system_db_header_t*)fbe_transport_allocate_buffer(); /*8 blks are enough to hold db header*/
    if(NULL == new_system_db_header)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: meomory allocation failed\n",
                    __FUNCTION__);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_database_system_db_header_init(new_system_db_header);

    database_trace(FBE_TRACE_LEVEL_INFO, 
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: new system db header persisted version is: 0x%x \n",
                        __FUNCTION__,
                        (unsigned int)new_system_db_header->persisted_sep_version);

    /* update peer's system db header. If failed, try again, again and again ...... */
    do
    {
            status = fbe_database_cmi_update_peer_system_db_header(new_system_db_header, &fbe_database_service.peer_sep_version);
            if (FBE_STATUS_OK != status)
            {
                database_trace(FBE_TRACE_LEVEL_WARNING, 
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: update peer system db header failed. Now we will retry...\n",
                        __FUNCTION__);
            }
    }while(FBE_STATUS_OK != status);

    /* update our system db header */
    fbe_spinlock_lock(&fbe_database_service.system_db_header_lock);
    fbe_copy_memory(&fbe_database_service.system_db_header, new_system_db_header, sizeof(database_system_db_header_t));
    fbe_spinlock_unlock(&fbe_database_service.system_db_header_lock);

    if(FBE_TRUE)
    {
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: new system objects needed, update system db.\n", __FUNCTION__);

        fbe_database_service.transient_commit_level = CURRENT_FLARE_COMMIT_LEVEL;
        fbe_database_service.use_transient_commit_level = FBE_TRUE;

        status = fbe_database_system_db_persist_entries(&fbe_database_service);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to persist system objects entries\n", __FUNCTION__);
            fbe_database_complete_packet(packet, status);
            return status;
        }

        fbe_database_service.use_transient_commit_level = FBE_FALSE;
        fbe_database_service.transient_commit_level = 0;
    }
    /* persist the initialized temp system db header onto disk*/
    /* this has to be the last, so that the persist sep version is not updated if any of the previous
     * persist work fails
     */
    status = fbe_database_system_db_persist_header(new_system_db_header);
    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: persist system db header failed\n",
                __FUNCTION__);
        fbe_database_complete_packet(packet, status);
        fbe_transport_release_buffer((void*)new_system_db_header);
        return status;
    }

    status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                         NULL,
                                                         FBE_DATABE_CMI_MSG_TYPE_DATABASE_COMMIT_UPDATE_TABLE);
    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: request peer to update table failed\n",
                __FUNCTION__);
        fbe_database_complete_packet(packet, status);
        fbe_transport_release_buffer((void*)new_system_db_header);
        return status;
    }

    fbe_transport_release_buffer((void*)new_system_db_header);
    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}
static fbe_bool_t database_new_system_lun_object_needed(void)
{
    fbe_status_t                        status = FBE_STATUS_INVALID;
    fbe_bool_t                          new_system_lun_object = FBE_FALSE;
    fbe_private_space_layout_lun_info_t lun;
    fbe_u32_t                           i;
    fbe_u64_t                           current_commit_level;

    fbe_spinlock_lock(&fbe_database_service.system_db_header_lock);
    current_commit_level = fbe_database_service.system_db_header.persisted_sep_version;
    fbe_spinlock_unlock(&fbe_database_service.system_db_header_lock);

    for(i = 0; i < FBE_PRIVATE_SPACE_LAYOUT_MAX_PRIVATE_LUNS; i++ )
    {
        status = fbe_private_space_layout_get_lun_by_index(i, &lun);
        if(status != FBE_STATUS_OK) {
            break;
        }
        if(lun.lun_number == FBE_PRIVATE_SPACE_LAYOUT_LUN_ID_INVALID)
        {
            break;
        }
        if(lun.commit_level >= current_commit_level)
        {
            new_system_lun_object = FBE_TRUE;
            break;
        }
    }

    return new_system_lun_object;
}

/*!***************************************************************
* @fn database_check_entry_version_header
*****************************************************************
* @brief
* check the version header of entry
* Only used in fbe test and debug
*
* @param void
*
* @return status
* 
****************************************************************/
static fbe_status_t database_check_entry_version_header(void)
{
    fbe_status_t status = FBE_STATUS_OK;
    database_config_table_type_t type;
    database_table_t *  table_ptr = NULL;
    database_object_entry_t *   object_entry_ptr = NULL;
    database_edge_entry_t *     edge_entry_ptr = NULL;
    database_user_entry_t *     user_entry_ptr = NULL;
    fbe_u32_t   i,j;


    for (type = DATABASE_CONFIG_TABLE_USER; type <= DATABASE_CONFIG_TABLE_EDGE; type++) {
        status = fbe_database_get_service_table_ptr(&table_ptr, type);
        if ((status != FBE_STATUS_OK) || table_ptr == NULL) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: get_service_table_ptr failed, status = 0x%x\n",
                __FUNCTION__, status);
            return status;
        }

        for (i = 0; i < 50; i++) {
            switch (type) {
            case DATABASE_CONFIG_TABLE_OBJECT:
                status = fbe_database_config_table_get_object_entry_by_id(table_ptr,
                                                                          i,
                                                                          &object_entry_ptr);
                if (status != FBE_STATUS_OK) {
                    database_trace(FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: failed to get object entry, status = 0x%x\n",
                                   __FUNCTION__, status);
                    return status;
                }
                if (object_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) {
                    if (object_entry_ptr->header.version_header.size != database_common_object_entry_size(object_entry_ptr->db_class_id)) {
                        database_trace(FBE_TRACE_LEVEL_ERROR,
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s: size is not equal: size1 = %d, size2 = %d,objid = %d, type = %d\n",
                                       __FUNCTION__,
                                       object_entry_ptr->header.version_header.size,
                                       database_common_object_entry_size(object_entry_ptr->db_class_id),
                                       i, type);
                        status = FBE_STATUS_GENERIC_FAILURE;
                        return status;
                    }
                }
                break;
            case DATABASE_CONFIG_TABLE_USER:
                status = fbe_database_config_table_get_user_entry_by_object_id(table_ptr,
                                                                               i,
                                                                               &user_entry_ptr);
                if (status != FBE_STATUS_OK) {
                    database_trace(FBE_TRACE_LEVEL_ERROR,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s: failed to get user entry, status = 0x%x\n",
                                   __FUNCTION__, status);
                    return status;
                }
                if (user_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) {
                    if (user_entry_ptr->header.version_header.size != database_common_user_entry_size(user_entry_ptr->db_class_id)) {
                        database_trace(FBE_TRACE_LEVEL_ERROR,
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s: size is not equal: size1 = %d, size2 = %d, objid = %d, type = %d\n",
                                       __FUNCTION__,
                                       user_entry_ptr->header.version_header.size,
                                       database_common_user_entry_size(user_entry_ptr->db_class_id),
                                       i,type);
                        status = FBE_STATUS_GENERIC_FAILURE;
                        return status;
                    }
                }
                break;
            case DATABASE_CONFIG_TABLE_EDGE:
                for (j = 0; j < DATABASE_MAX_EDGE_PER_OBJECT; j++ ) {
                    status = fbe_database_config_table_get_edge_entry(table_ptr,
                                                                      i,
                                                                      j,
                                                                      &edge_entry_ptr);
                    if (status != FBE_STATUS_OK) {
                        database_trace(FBE_TRACE_LEVEL_ERROR,
                                       FBE_TRACE_MESSAGE_ID_INFO,
                                       "%s: failed to get edge entry, status = 0x%x\n",
                                       __FUNCTION__, status);
                        return status;
                    }
                    if (edge_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) {
                        if (edge_entry_ptr->header.version_header.size != database_common_edge_entry_size()) {
                            database_trace(FBE_TRACE_LEVEL_ERROR,
                                           FBE_TRACE_MESSAGE_ID_INFO,
                                           "%s: size is not equal: size1 = %d, size2 = %d, objid = %d, index = %d\n",
                                           __FUNCTION__,
                                           edge_entry_ptr->header.version_header.size,
                                           database_common_edge_entry_size(),
                                           i, j);
                            status = FBE_STATUS_GENERIC_FAILURE;
                            return status;
                        }
                    }
                }
                break;
            default:
                database_trace(FBE_TRACE_LEVEL_WARNING,
                               FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: Table type:%d is not supported!\n",
                               __FUNCTION__,
                               type);
                status = FBE_STATUS_GENERIC_FAILURE;
                break;
            }//switch
        }//for
    }
    return status;
}

/*!***************************************************************
* @fn database_control_versioning_operation
*****************************************************************
* @brief
* Check database versioning related information.
*
* @param packet - the request packet
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
* @version
*    11/28/2012: Wenxuan Yin - created
* 
****************************************************************/
static fbe_status_t database_control_versioning_operation(fbe_packet_t * packet)
{
    fbe_status_t                     status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_versioning_op_code_t  *versioning_op_code_p = NULL;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&versioning_op_code_p, sizeof(fbe_database_control_versioning_op_code_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /* Call different routines based on different op code*/
    switch(*versioning_op_code_p)
    {
        case FBE_DATABASE_CONTROL_CHECK_VERSION_HEADER_IN_ENTRY:
            status = database_check_entry_version_header();
            break;
        case FBE_DATABASE_CONTROL_CHECK_UNKNOWN_CMI_MSG_HANDLING:
            status = database_check_unknown_cmi_msg_handling_test();
            break;
        default:
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s: unrecognized database versioning op code: 0x%x\n",
                    __FUNCTION__, *versioning_op_code_p);
    }

    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn database_control_set_invalidate_config_flag
 *****************************************************************
* @brief
*       read data from disk DDMI PSL region
*
* @param packet -packet
*
* @return fbe_status_t - FBE_STATUS_OK if no issue
 *
 * @version
 *    11/28/2012: Wenxuan Yin - created
 *
 ****************************************************************/
static fbe_status_t database_control_get_disk_ddmi_data (fbe_packet_t * packet)
{
    fbe_status_t                            status = FBE_STATUS_GENERIC_FAILURE;
    database_physical_drive_info_t          drive_location_info;
    fbe_database_control_ddmi_data_t        *get_ddmi = NULL;  
    fbe_u8_t                                *ddmi_data = NULL;
    fbe_payload_ex_t *                      payload = NULL;
    fbe_sg_element_t *                      sg_element = NULL;
    fbe_u32_t                               sg_elements = 0;
    fbe_u32_t                               number_of_bytes_requested = 0;
    fbe_database_physical_drive_info_t      pdo_info;
    fbe_object_id_t                         pdo_id = FBE_OBJECT_ID_INVALID;
    fbe_lifecycle_state_t                   pdo_state;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&get_ddmi, sizeof(fbe_database_control_ddmi_data_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_elements);

    /*point to start of the buffer*/
    ddmi_data = (fbe_u8_t *)sg_element->address;

    /*sanity check*/
    if ((sg_element->count / sizeof(fbe_u8_t)) != get_ddmi->number_of_bytes_requested){
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s Byte count: 0x%x doesn't agree with num objects: %d.\n",
                       __FUNCTION__, sg_element->count, get_ddmi->number_of_bytes_requested);

        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_database_get_pdo_object_id_by_location(get_ddmi->bus, get_ddmi->enclosure, get_ddmi->slot, &pdo_id);
    if (FBE_STATUS_OK != status || pdo_id == FBE_OBJECT_ID_INVALID)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "database get pdo: failed to get pdo id %u_%u_%u, status 0x%x\n", 
                       get_ddmi->bus, get_ddmi->enclosure, get_ddmi->slot, status);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }    

    status = fbe_database_generic_get_object_state(pdo_id, &pdo_state, FBE_PACKAGE_ID_PHYSICAL);
    if (FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "database get pdo state: failed to get pdo state, object id: 0x%x, status 0x%x\n", 
                       pdo_id, status);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (pdo_state != FBE_LIFECYCLE_STATE_READY)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "pdo state: pdo is not in READY state, object id: 0x%x\n", 
                       pdo_id);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get pdo info before we read signature for later re-check in case that the drive
     * corresponding to this object id changed during the read
     */
    status = fbe_database_get_pdo_info(pdo_id, &pdo_info);
    if (FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "database get pdo info: failed to get pdo info: 0x%x/0x%x\n", 
                     pdo_id, status);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* at least make sure when we get pdo info the drive does not change*/
    if(pdo_info.bus != get_ddmi->bus || pdo_info.enclosure != get_ddmi->enclosure || pdo_info.slot != get_ddmi->slot)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "%s: drive for objid 0x%x has changed to %d_%d_%d.\n", 
                     __FUNCTION__, pdo_id, pdo_info.bus, pdo_info.enclosure, pdo_info.slot);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    

    number_of_bytes_requested = get_ddmi->number_of_bytes_requested;
    
    drive_location_info.port_number = get_ddmi->bus;
    drive_location_info.enclosure_number = get_ddmi->enclosure;
    drive_location_info.slot_number = get_ddmi->slot;
    drive_location_info.block_geometry = pdo_info.block_geometry;

    status = fbe_database_read_data_from_single_raw_drive(ddmi_data,
                                        number_of_bytes_requested,
                                        &drive_location_info,
                                        FBE_PRIVATE_SPACE_LAYOUT_REGION_ID_DDMI);
    if (FBE_STATUS_OK != status){
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s Read single raw drive FAILED!\n", __FUNCTION__);
        fbe_database_complete_packet(packet, status);
        return status;
    }
    
    get_ddmi->number_of_bytes_returned = number_of_bytes_requested;
    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;
    
}

/*!***************************************************************
* @fn database_control_ndu_commit
*****************************************************************
* @brief
* perform sep ndu commit. called by ndu api
* step1: register sep_ndu_commit_job completion notification
* step2: send sep_ndu_commit_job
* step3: wait for the job completion notification
* step4: return the execution status of the job to the api caller
*
* @param packet - request packet
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
* @version
*  05/01/2012 - Created. Zhipeng Hu 

****************************************************************/
static fbe_status_t database_control_ndu_commit(fbe_packet_t * packet)
{
    fbe_status_t    status;
    database_ndu_commit_context_t   ndu_commit_context;
    fbe_notification_element_t  notification_element;

    if (fbe_database_ndu_is_committed())
    {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
            "database_control_ndu_commit: Already committed.\n");
        /* we don't have to anything if we've already committed. */
        fbe_database_complete_packet(packet, FBE_STATUS_OK);
        return FBE_STATUS_OK;
    }

    /*initialize the context*/
    ndu_commit_context.commit_status = FBE_STATUS_GENERIC_FAILURE;
    fbe_semaphore_init(&ndu_commit_context.commit_semaphore, 0, 1);

    /*register the ndu commit job notification*/
    notification_element.notification_function = database_ndu_commit_job_notification_callback;
    notification_element.notification_context = &ndu_commit_context;
    notification_element.targe_package = FBE_PACKAGE_ID_SEP_0;
    notification_element.notification_type = FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED;
    notification_element.object_type = FBE_TOPOLOGY_OBJECT_TYPE_ALL;
    status = database_common_register_notification ( &notification_element, FBE_PACKAGE_ID_SEP_0);
    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "database_control_ndu_commit: fail register notification for ndu commit job\n");
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /*send ndu commit request to job service*/
    status = fbe_database_send_packet_to_service(FBE_JOB_CONTROL_CODE_SEP_NDU_COMMIT,
                                                 &ndu_commit_context.ndu_commit_job_request,
                                                 sizeof(fbe_job_service_sep_ndu_commit_t),
                                                 FBE_SERVICE_ID_JOB_SERVICE,
                                                 NULL,  /* no sg list*/
                                                 0,  /* no sg list*/
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 FBE_PACKAGE_ID_SEP_0);
    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "database_control_ndu_commit: fail send ndu commit job\n");
        fbe_database_complete_packet(packet, status);
        return status;
    }

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                            "database_control_ndu_commit: successfully send ndu commit job. job num:0x%x\n", 
                            (unsigned int)ndu_commit_context.ndu_commit_job_request.job_number);

    /*wait the completion of the job*/
    fbe_semaphore_wait_ms(&ndu_commit_context.commit_semaphore, 0);
    fbe_semaphore_destroy(&ndu_commit_context.commit_semaphore);

    /*unregister the notification*/
    database_common_unregister_notification (&notification_element, FBE_PACKAGE_ID_SEP_0);

    if(FBE_STATUS_OK != ndu_commit_context.commit_status)
    {
        /*only when system db header persist fails can the status here be not OK*/
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                                "database_control_ndu_commit: ndu commit job execution failed\n");
    }
    
    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                   "%s: SEP ndu commit is done\n", 
                   __FUNCTION__);

    fbe_database_complete_packet(packet, ndu_commit_context.commit_status);
    return ndu_commit_context.commit_status;
    
}

static fbe_status_t database_control_get_compat_mode(fbe_packet_t * packet)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_compatibility_mode_t    *compat_mode;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&compat_mode, sizeof(fbe_compatibility_mode_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    compat_mode->datasize = sizeof(fbe_compatibility_mode_t);

    compat_mode->version = SEP_K10ATTACH_CURRENT_VERSION;

    fbe_spinlock_lock(&fbe_database_service.system_db_header_lock);

    /* convert the persisted_sep_version to fbe_u16_t type */
    compat_mode->commit_level = (fbe_u16_t)fbe_database_service.system_db_header.persisted_sep_version;

    fbe_spinlock_unlock(&fbe_database_service.system_db_header_lock);

    /* If the driver is upgraded, the mode should be K10_DRIVER_COMPATIBILITY_MODE_OLD */
    if (compat_mode->commit_level == SEP_PACKAGE_VERSION) {
        compat_mode->mode = K10_DRIVER_COMPATIBILITY_MODE_LATEST;
    } else if (compat_mode->commit_level < SEP_PACKAGE_VERSION) {
        compat_mode->mode = K10_DRIVER_COMPATIBILITY_MODE_OLD;
    } else {
        database_trace(FBE_TRACE_LEVEL_WARNING,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: the persisted sep version(0x%x) is newer than driver version(0x%x)\n",
                       __FUNCTION__, compat_mode->commit_level, SEP_PACKAGE_VERSION);
    }
    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                   "%s: wVersion is 0x%x, persisted sep version is 0x%x.\n", 
                   __FUNCTION__, compat_mode->version, compat_mode->commit_level);

    fbe_database_complete_packet(packet, status);
    return status;

}

/*!***************************************************************
* @fn database_control_cleanup_reinit_persist_service
*****************************************************************
* @brief
* clear the database lun and re-initialize the persist service on
* the clean and empty lun
*
* @param packet - the packet of the request
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
* @version
*  07/17/2012 - Created. Jingcheng Zhang for backup/restore
****************************************************************/
static fbe_status_t database_control_cleanup_reinit_persist_service(fbe_packet_t * packet)
{
    fbe_status_t    status = FBE_STATUS_OK;


    /*cleanup and re-initialize*/
    status = fbe_database_cleanup_and_reinitialize_persist_service(FBE_TRUE);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                       "Fail to cleanup and re-initalize persist service, status %d\n", 
                       status);
    }

    fbe_database_complete_packet(packet, status);

    return status;

}

/*!***************************************************************
* @fn database_control_generate_configuration_for_system_parity_RG_and_LUN
*****************************************************************
* @brief
*   This function is used to recover the system parity RG and releated LUN after they are 
*   broken. The recovery should be follow a procedure.
*   1. unbind the LUNs
*   2. destroy parity RGs
*   3. recover the drives
*   4. generate the configurations for RGs and LUNs
*   5. recreate the RGs and LUNs by the configurations

*
* @param packet - the packet of the request
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
* @version
*  11/02/2012 - Created. gaoh1
****************************************************************/
static fbe_status_t database_control_generate_configuration_for_system_parity_RG_and_LUN(fbe_packet_t * packet)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_bool_t        *ndb_ptr = NULL;
    fbe_bool_t        ndb = FBE_FALSE;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&ndb_ptr, sizeof(fbe_bool_t));
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get payload of the packet\n",
                       __FUNCTION__);
        fbe_database_complete_packet(packet, status);
        return status;
    } 
    
    ndb = *ndb_ptr;

    status = fbe_database_system_objects_generate_config_for_parity_rg_and_lun_from_psl(ndb);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                       "Fail to generate config for parity RGs and LUNs, status:%d, ndb:%d\n", 
                       status, ndb);
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /* Persist the system entries to raw mirror of system db 
       * after generating new configuration for parity RGs and related LUNs */
    status = fbe_database_system_db_persist_entries(&fbe_database_service);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                       "Fail to generate config for parity RGs and LUNs, status:%d\n", 
                       status);
    }

    fbe_database_complete_packet(packet, status);

    return status;

}

/*!***************************************************************
* @fn database_control_restore_user_configuration
*****************************************************************
* @brief
* restore the user configuration (object, user and edge entries)
* to the database LUN with persist service.  No database transaction
* invloved in this operation. The caller should be in charge of the
* configuration change disable
*
* @param packet - the packet of the request
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
* @version
*  07/17/2012 - Created. Jingcheng Zhang for backup/restore
****************************************************************/
static fbe_status_t database_control_restore_user_configuration(fbe_packet_t * packet)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_database_config_entries_restore_t    *restore_op_p = NULL;
    fbe_persist_sector_type_t  persist_type = 0;
    fbe_u32_t res_entry_num = 0;
    fbe_u8_t *cur_entry = NULL;
    fbe_u32_t  max_persist_write_size = 16;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&restore_op_p, sizeof(fbe_database_config_entries_restore_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }


    switch (restore_op_p->restore_type) {
    case FBE_DATABASE_CONFIG_RESTORE_TYPE_EDGE:
        persist_type = FBE_PERSIST_SECTOR_TYPE_SEP_EDGES;
        break;
    case FBE_DATABASE_CONFIG_RESTORE_TYPE_GLOBAL_INFO:
        persist_type = FBE_PERSIST_SECTOR_TYPE_SYSTEM_GLOBAL_DATA;
        break;
    case FBE_DATABASE_CONFIG_RESTORE_TYPE_OBJECT:
        persist_type = FBE_PERSIST_SECTOR_TYPE_SEP_OBJECTS;
        break;
    case FBE_DATABASE_CONFIG_RESTORE_TYPE_USER:
        persist_type = FBE_PERSIST_SECTOR_TYPE_SEP_ADMIN_CONVERSION;
        break;
    default:
        persist_type = FBE_PERSIST_SECTOR_TYPE_INVALID;
        break;
    }

    if (persist_type == FBE_PERSIST_SECTOR_TYPE_INVALID) {
        database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                   "%s: invalid restore type %d\n", 
                   __FUNCTION__, restore_op_p->restore_type);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*persist service can only support persist maximum FBE_PERSIST_TRAN_ENTRY_MAX 
      entries one time. so try restore with many persist transactions*/
    cur_entry = restore_op_p->entries;
    res_entry_num = restore_op_p->entries_num;
    while (res_entry_num > 0) {
        if (res_entry_num > max_persist_write_size) {
            status = fbe_database_no_tran_persist_entries(cur_entry, 
                                                          max_persist_write_size,
                                                          restore_op_p->entry_size,
                                                          persist_type);
            cur_entry += max_persist_write_size * restore_op_p->entry_size;
            res_entry_num -= max_persist_write_size;
        } else {
            status = fbe_database_no_tran_persist_entries(cur_entry, 
                                                          res_entry_num,
                                                          restore_op_p->entry_size,
                                                          persist_type);
            cur_entry += res_entry_num * restore_op_p->entry_size;
            res_entry_num = 0;
        }
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_INFO, 
                           FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                           "%s: Fail restore with persist service\n", 
                           __FUNCTION__);
            break;
        }
    }

    fbe_database_complete_packet(packet, status);
    return status;
}


static fbe_status_t database_ndu_commit_job_notification_callback(fbe_object_id_t object_id, 
                                                                        fbe_notification_info_t notification_info,
                                                                        fbe_notification_context_t context)
{
    database_ndu_commit_context_t*  ndu_commit_context = (database_ndu_commit_context_t*)context;

    if(FBE_NOTIFICATION_TYPE_JOB_ACTION_STATE_CHANGED != notification_info.notification_type)
        return FBE_STATUS_OK;

    if(NULL == ndu_commit_context)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: the ndu commit context is NULL. Unexpected!\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*judge whether this notification is triggered by the ndu commit job we sent*/
    if(notification_info.notification_data.job_service_error_info.job_number != ndu_commit_context->ndu_commit_job_request.job_number)
        return FBE_STATUS_OK;

    /*return the job execution result status through the ndu commit context*/
    if(FBE_JOB_SERVICE_ERROR_NO_ERROR == notification_info.notification_data.job_service_error_info.error_code)
        ndu_commit_context->commit_status = FBE_STATUS_OK;        
    else
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: ndu commit job failed.\n", __FUNCTION__);
        ndu_commit_context->commit_status = FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_semaphore_release(&ndu_commit_context->commit_semaphore, 0, 1, FBE_FALSE);
    
    return FBE_STATUS_OK;
}

/*!***************************************************************
* @fn database_control_get_shared_emv_info
*****************************************************************
* @brief
* get shared emv info from database service, including ctm value and semv value. 
* called by fbe api
*
* @param packet - request packet
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
* @version
*  05/18/2012 - Created. Zhipeng Hu
****************************************************************/
static fbe_status_t database_control_get_shared_emv_info(fbe_packet_t *packet)
{
    fbe_status_t                     status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_emv_t * get_emv = NULL;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&get_emv, sizeof(fbe_database_emv_t));
    if (status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    fbe_copy_memory(get_emv, &fbe_database_service.shared_expected_memory_info, sizeof(fbe_database_emv_t));

    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;
}

/*!***************************************************************
* @fn database_control_set_shared_emv_info
*****************************************************************
* @brief
* set shared emv info from database service, including ctm value and semv value. 
* called by fbe api
*
* @param packet - request packet
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
* @version
*  05/18/2012 - Created. Zhipeng Hu
****************************************************************/
static fbe_status_t database_control_set_shared_emv_info(fbe_packet_t *packet)
{
    fbe_status_t                     status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_emv_t * set_emv = NULL;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&set_emv, sizeof(fbe_database_emv_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    status = database_set_shared_expected_memory_info(&fbe_database_service, *set_emv);

    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to set shared expected memory info.\n", __FUNCTION__);
    }
    else
    {
        /*update this command to peer*/
        status = fbe_database_cmi_update_peer_shared_emv_info(&fbe_database_service.shared_expected_memory_info);
    }

    fbe_database_complete_packet(packet, status);
    return status;
}

/*!***************************************************************
* @fn fbe_database_get_shared_emv_info
*****************************************************************
* @brief
* get shared emv info from database service, including ctm value and semv value. 
* called by sep ioctl process routine
*
* @param shared_emv_info - the buffer to store the returned shared emv info
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
* @version
*  05/23/2012 - Created. Zhipeng Hu
****************************************************************/
fbe_status_t  fbe_database_get_shared_emv_info(fbe_u64_t *shared_emv_info)
{
    if(NULL == shared_emv_info)
        return FBE_STATUS_GENERIC_FAILURE;

    *shared_emv_info = fbe_database_service.shared_expected_memory_info.shared_expected_memory_info;

    return FBE_STATUS_OK;
}

/*!***************************************************************
* @fn fbe_database_get_shared_emv_info
*****************************************************************
* @brief
* set shared emv info from database service, including ctm value and semv value. 
* called by sep ioctl process routine
*
* @param shared_emv_info - the shared emv info we want to set
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
* @version
*  05/23/2012 - Created. Zhipeng Hu
****************************************************************/
fbe_status_t  fbe_database_set_shared_emv_info(fbe_u64_t shared_emv_info)
{
    fbe_status_t    status;
    fbe_database_emv_t  set_emv;

    set_emv.shared_expected_memory_info = shared_emv_info;    
    status = database_set_shared_expected_memory_info(&fbe_database_service, set_emv);
    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to set shared expected memory info.\n", __FUNCTION__);
    }

    return status;
}

/*!***************************************************************
 * @fn database_control_set_load_balance
 *****************************************************************
 * @brief
 *   Control function to set load balancing.
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_OK 
 *
 * @version
 *    5/31/2012: Peter Puhov - created
 *
 ****************************************************************/
static fbe_status_t database_control_set_load_balance(fbe_packet_t *packet, fbe_bool_t is_enable)
{    
    fbe_payload_ex_t *payload = NULL;
    fbe_payload_control_operation_t *control_operation = NULL;

    fbe_base_config_set_load_balance(is_enable);

    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: Load balance set to %d \n", 
                   __FUNCTION__, is_enable);


    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);
    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);

    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}


fbe_status_t fbe_database_lookup_raid_by_object_id(fbe_object_id_t object_id, fbe_u32_t *raid_group_number)
{
    fbe_status_t                     status = FBE_STATUS_GENERIC_FAILURE;
    database_user_entry_t           *out_entry_ptr = NULL;     

    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,	
                                                                   object_id,
                                                                   &out_entry_ptr);
    if ((status == FBE_STATUS_OK)&&(out_entry_ptr != NULL)) {
        /* get the entry ok */
        if (out_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) {
            /* only return the object when the entry is valid */
            *raid_group_number = out_entry_ptr->user_data_union.rg_user_data.raid_group_number;
        }
    }else{
    //	lookup_raid->raid_id = NULL;
        database_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to find RG %d; returned 0x%x.\n",
            __FUNCTION__, object_id, status);
        status = FBE_STATUS_NO_OBJECT;
    }
    return status;
}
/*!***************************************************************
* @fn database_control_get_system_db_header
*****************************************************************
* @brief
*   Get system db header in memory .It is used for system backup and restore tool
*
* @param packet - request packet
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
* @version
*  07/04/2012 - Created. Yang Zhang 

****************************************************************/
static fbe_status_t database_control_get_system_db_header(fbe_packet_t * packet)
{
    fbe_status_t                       status = FBE_STATUS_OK;
    database_system_db_header_t        *system_db_header = NULL;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&system_db_header, sizeof(database_system_db_header_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /* Copy the contents from the database table */
    
    fbe_spinlock_lock(&fbe_database_service.system_db_header_lock);
    fbe_copy_memory(system_db_header,
                    &fbe_database_service.system_db_header, 
                    sizeof(database_system_db_header_t));
    fbe_spinlock_unlock(&fbe_database_service.system_db_header_lock);

    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;
}

/*!***************************************************************
* @fn database_control_get_system_db_header
*****************************************************************
* @brief
*   Set system db header in memory .
*
* @param packet - request packet
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
* @version
*  11/27/2012 - Created. Wenxuan Yin 

****************************************************************/
static fbe_status_t database_control_set_system_db_header(fbe_packet_t * packet)
{
    fbe_status_t                       status = FBE_STATUS_OK;
    database_system_db_header_t        *system_db_header = NULL;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&system_db_header, sizeof(database_system_db_header_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /* Copy the contents into the database table */

    fbe_spinlock_lock(&fbe_database_service.system_db_header_lock);
    fbe_copy_memory(&fbe_database_service.system_db_header,
                    system_db_header,                     
                    sizeof(database_system_db_header_t));
    fbe_spinlock_unlock(&fbe_database_service.system_db_header_lock);

    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;
}

/*!***************************************************************
* @fn database_control_init_system_db_header
*****************************************************************
* @brief
*   Init system db header.It is used to recover the system_db_header tool.
*
* @param packet - request packet
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
* @version
*  May 7, 2013 - Created. Yingying Song

****************************************************************/
static fbe_status_t database_control_init_system_db_header(fbe_packet_t * packet)
{
    fbe_status_t                       status = FBE_STATUS_OK;
    database_system_db_header_t        *system_db_header = NULL;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&system_db_header, sizeof(database_system_db_header_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /* Init the system db header */
    status = fbe_database_system_db_header_init(system_db_header);
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;
}



/*!***************************************************************
* @fn database_control_persist_system_db_header
*****************************************************************
* @brief
*   Persist system db header to disk .
*   It is used for system backup and restore tool
*
* @param packet - request packet
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
* @version
*  07/04/2012 - Created. Yang Zhang 

****************************************************************/
static fbe_status_t database_control_persist_system_db_header(fbe_packet_t * packet)
{
    fbe_status_t                       status = FBE_STATUS_OK;
    database_system_db_header_t        *system_db_header = NULL;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&system_db_header, sizeof(database_system_db_header_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /* Persist the db header to disk */
    
    status = fbe_database_system_db_persist_header(system_db_header);
    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: persist system db header failed\n",
                __FUNCTION__);
        fbe_database_complete_packet(packet, status);
        return status;
    }

    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;
}

/*!***************************************************************
* @fn database_control_get_system_object_recreate_flags
*****************************************************************
* @brief
*   Get system object operation flags which persisted in the raw mirror.
*   It is used for system object recreation.
*
* @param packet - request packet
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
* @version
*  17/08/2012 - Created. gaoh1 
****************************************************************/
static fbe_status_t database_control_get_system_object_recreate_flags(fbe_packet_t *packet)
{
    fbe_status_t                       status = FBE_STATUS_OK;
    fbe_database_system_object_recreate_flags_t *system_object_op_flags = NULL;
    fbe_u32_t                           valid_db_drives;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&system_object_op_flags, sizeof(fbe_database_system_object_recreate_flags_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    status = fbe_database_raw_mirror_get_valid_drives(&valid_db_drives);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO, 
            "Database service failed to get valid raw-mirror drives, instead load from all drives \n");
        valid_db_drives = DATABASE_SYSTEM_RAW_MIRROR_ALL_DB_DRIVE_MASK;
    }
    status = fbe_database_read_system_object_recreate_flags(system_object_op_flags, valid_db_drives);
    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;

}
/*!***************************************************************
* @fn database_control_persist_system_object_recreate_flags
*****************************************************************
* @brief
*   Persist system object operation flags which persisted in the raw mirror.
*   It is used for system object recreation.
*
* @param packet - request packet
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
* @version
*  17/08/2012 - Created. gaoh1 
****************************************************************/
static fbe_status_t database_control_persist_system_object_recreate_flags(fbe_packet_t *packet)
{
    fbe_status_t                       status = FBE_STATUS_OK;
    fbe_database_system_object_recreate_flags_t *system_object_op_flags = NULL;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&system_object_op_flags, sizeof(fbe_database_system_object_recreate_flags_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    status = fbe_database_persist_system_object_recreate_flags(system_object_op_flags);
    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;
}


/*!***************************************************************
* @fn database_control_generate_system_object_config_and_persist
*****************************************************************
* @brief
*   Generate system object config for specified object from PSL 
*   and persist the config to disk
*
* @param packet - request packet
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
* @version
*  May 24, 2013 - Created. Yingying Song
****************************************************************/
static fbe_status_t database_control_generate_system_object_config_and_persist(fbe_packet_t *packet)
{
    fbe_status_t           status = FBE_STATUS_OK;
    fbe_object_id_t        *object_id_ptr = NULL;
    fbe_object_id_t        object_id = FBE_OBJECT_ID_INVALID;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&object_id_ptr, sizeof(fbe_object_id_t));
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to get payload of the packet\n",
                       __FUNCTION__);
        fbe_database_complete_packet(packet, status);
        return status;
    } 
    
    object_id = *object_id_ptr;
    
    status = fbe_database_system_objects_generate_config_for_system_object_and_persist(object_id, FBE_FALSE, FBE_FALSE);
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: failed to generate config for obj 0x%x\n",
                       __FUNCTION__, object_id);
        fbe_database_complete_packet(packet, status);
        return status;
    }

    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;
}

/*!***************************************************************
* @fn database_control_generate_all_system_rg_and_lun_config_and_persist
*****************************************************************
* @brief
*   Generate all system rg and lun config and persist all config to disk
*
* @param packet - request packet
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
* @version
*  May 24, 2013 - Created. Yingying Song
****************************************************************/
static fbe_status_t database_control_generate_all_system_rg_and_lun_config_and_persist(fbe_packet_t *packet)
{
    fbe_status_t           status = FBE_STATUS_OK;
    
    /* Generate system config for all system rg and lun in memory */
    status = fbe_database_system_objects_generate_config_for_all_system_rg_and_lun();
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: Failed to generate config for all system RGs and LUNs,status: %d\n",
                       __FUNCTION__, status);
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /* Persist all the system config to raw mirror of system db 
     * after generating new configuration for all RGs and LUNs */
    status = fbe_database_system_db_persist_entries(&fbe_database_service);
    if (status != FBE_STATUS_OK) 
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                       "%s: Failed to generate config for all system RGs and LUNs, status:%d\n",
                       __FUNCTION__, status);
        fbe_database_complete_packet(packet, status);
        return status;
    }

    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;
}




/*!***************************************************************
 * @fn database_control_get_tables_in_range_with_type
 *****************************************************************
 * @brief
 *   Control function to get object entries,user entries or edge entries in one shot
 *   according to the input type.
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    07/10/2012: Yang Zhang created
 *
 ****************************************************************/
static fbe_status_t database_control_get_tables_in_range_with_type(fbe_packet_t *packet)
{
    fbe_status_t                     		status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_get_tables_in_range_t *	get_tables_in_range = NULL;  
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&get_tables_in_range, sizeof(fbe_database_control_get_tables_in_range_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
      
    switch(get_tables_in_range->table_type) {
        case FBE_DATABASE_TABLE_TYPE_OBJECT:
            status = database_control_get_object_tables_in_range(packet);
            break;
        case FBE_DATABASE_TABLE_TYPE_USER:
            status = database_control_get_user_tables_in_range(packet);
            break;
        case FBE_DATABASE_TABLE_TYPE_EDGE:
            status = database_control_get_edge_tables_in_range(packet);
            break;
        default:
            database_trace(FBE_TRACE_LEVEL_ERROR,
                           FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s: Unknown table type 0x%X\n", __FUNCTION__, get_tables_in_range->table_type);
            fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
            status = FBE_STATUS_GENERIC_FAILURE;
            fbe_transport_complete_packet(packet);
            break;
            
    }
     
    return status;

}
static fbe_status_t database_control_get_object_tables_in_range(fbe_packet_t *packet)
{
    fbe_status_t                     		status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_get_tables_in_range_t *	get_tables_in_range = NULL;  
    fbe_payload_ex_t *                  	payload = NULL;
    fbe_sg_element_t *                      sg_element = NULL;
    fbe_u32_t                               sg_elements = 0;
    fbe_object_id_t							start_object_id = 0;
    fbe_u32_t 								number_of_objects_requested = 0;
    fbe_u8_t*                               start_object_entry_p = NULL;
    database_table_t *                      in_table_ptr = NULL;
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&get_tables_in_range, sizeof(fbe_database_control_get_tables_in_range_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    
    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_elements);
    number_of_objects_requested = (get_tables_in_range->end_object_id - get_tables_in_range->start_object_id) + 1;
    start_object_id = get_tables_in_range->start_object_id;

    /*point to start of the buffer*/
    start_object_entry_p = sg_element->address;

    /*sanity check*/
    if ((sg_element->count / sizeof(database_object_entry_t)) != number_of_objects_requested){
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s Byte count: 0x%x doesn't agree with num objects: %d.\n",
                       __FUNCTION__, sg_element->count, number_of_objects_requested);

        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*get the table lock*/
    fbe_spinlock_lock(&fbe_database_service.object_table.table_lock);
    in_table_ptr = &fbe_database_service.object_table;
    fbe_copy_memory(start_object_entry_p,&in_table_ptr->table_content.object_entry_ptr[start_object_id],number_of_objects_requested*sizeof(database_object_entry_t));
    /*release the table lock*/
    fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);

    get_tables_in_range->number_of_objects_returned = number_of_objects_requested;
    
    fbe_database_complete_packet(packet, status);
    return status;

}
static fbe_status_t database_control_get_user_tables_in_range(fbe_packet_t *packet)
{
    fbe_status_t                     		status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_get_tables_in_range_t *	get_tables_in_range = NULL;  
    fbe_payload_ex_t *                  	payload = NULL;
    fbe_sg_element_t *                      sg_element = NULL;
    fbe_u32_t                               sg_elements = 0;
    fbe_object_id_t							start_object_id = 0;
    fbe_u32_t 								number_of_objects_requested = 0;
    fbe_u8_t*                               start_user_entry_p = NULL;
    database_table_t *                      in_table_ptr = NULL;
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&get_tables_in_range, sizeof(fbe_database_control_get_tables_in_range_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
   
    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_elements);
    number_of_objects_requested = (get_tables_in_range->end_object_id - get_tables_in_range->start_object_id) + 1;
    start_object_id = get_tables_in_range->start_object_id;

    /*point to start of the buffer*/
    start_user_entry_p = sg_element->address;

    /*sanity check*/
    if ((sg_element->count / sizeof(database_user_entry_t)) != number_of_objects_requested){
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s Byte count: 0x%x doesn't agree with num objects: %d.\n",
                       __FUNCTION__, sg_element->count, number_of_objects_requested);

        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /*Get the table lock*/
    fbe_spinlock_lock(&fbe_database_service.user_table.table_lock);
    in_table_ptr = &fbe_database_service.user_table;
    fbe_copy_memory(start_user_entry_p,&in_table_ptr->table_content.user_entry_ptr[start_object_id],number_of_objects_requested*sizeof(database_user_entry_t));
    /*release the table lock*/
    fbe_spinlock_unlock(&fbe_database_service.user_table.table_lock);

    get_tables_in_range->number_of_objects_returned = number_of_objects_requested;

    fbe_database_complete_packet(packet, status);
    return status;

}
static fbe_status_t database_control_get_edge_tables_in_range(fbe_packet_t *packet)
{
    fbe_status_t                     		status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_get_tables_in_range_t *	get_tables_in_range = NULL;  
    fbe_payload_ex_t *                  	payload = NULL;
    fbe_sg_element_t *                      sg_element = NULL;
    fbe_u32_t                               sg_elements = 0;
    fbe_object_id_t							start_object_id = 0;
    fbe_u32_t 								number_of_objects_requested = 0;
    fbe_u8_t*                               start_edge_entry_p = NULL;
    database_table_t *                      in_table_ptr = NULL;
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&get_tables_in_range, sizeof(fbe_database_control_get_tables_in_range_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
   
    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_elements);
    number_of_objects_requested = (get_tables_in_range->end_object_id - get_tables_in_range->start_object_id) + 1;
    start_object_id = get_tables_in_range->start_object_id;

    /*point to start of the buffer*/
    start_edge_entry_p = sg_element->address;

    /*sanity check*/
    if ((sg_element->count / (sizeof(database_edge_entry_t) * DATABASE_MAX_EDGE_PER_OBJECT)) != number_of_objects_requested){
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s Byte count: 0x%x doesn't agree with num objects: %d.\n",
                       __FUNCTION__, sg_element->count, number_of_objects_requested);

        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /*Get the table lock*/
    fbe_spinlock_lock(&fbe_database_service.edge_table.table_lock);
    in_table_ptr = &fbe_database_service.edge_table;
    fbe_copy_memory(start_edge_entry_p,&in_table_ptr->table_content.edge_entry_ptr[start_object_id * DATABASE_MAX_EDGE_PER_OBJECT],number_of_objects_requested * DATABASE_MAX_EDGE_PER_OBJECT *sizeof(database_edge_entry_t));
    /*release the table lock*/
    fbe_spinlock_unlock(&fbe_database_service.edge_table.table_lock);

    get_tables_in_range->number_of_objects_returned = number_of_objects_requested;

    fbe_database_complete_packet(packet, status);
    return status;

}

/*!***************************************************************
 * @fn database_control_persist_system_db
 *****************************************************************
 * @brief
 *   Control function to persist system object entries,user entries and edge entries 
 *   to system db in one shot.
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    07/10/2012: Yang Zhang created
 *
 ****************************************************************/
#define FBE_DATABASE_SYSTEM_EDGE_ENTRIES_NUM  ((FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST + 1)*DATABASE_MAX_EDGE_PER_OBJECT)
static fbe_status_t database_control_persist_system_db(fbe_packet_t *packet)
{
    fbe_u8_t *          write_buffer;
    fbe_lba_t           lba;
    fbe_u64_t           write_count = 0;
    fbe_u32_t           system_object_count = 0;
    fbe_status_t        status = FBE_STATUS_GENERIC_FAILURE;
    fbe_payload_ex_t *                  	payload = NULL;
    fbe_sg_element_t *                      sg_element = NULL;
    fbe_u32_t                               sg_elements = 0;
    fbe_u32_t 								all_entries_size_per_obj = 0;
    fbe_u8_t*                               object_entry_offset_p = NULL;
    fbe_u8_t*                               user_entry_offset_p = NULL;	
    fbe_u8_t*                               edge_entry_offset_p = NULL;
    fbe_u32_t                               i = 0;
    fbe_u32_t                               j = 0;
    fbe_u8_t *                              current_data = NULL;
    fbe_database_control_persist_sep_objets_t *	persist_system_db = NULL;  
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&persist_system_db, sizeof(fbe_database_control_persist_sep_objets_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    /*init the returen number to 0*/
    persist_system_db->number_of_objects_returned = 0;
    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_elements);
    all_entries_size_per_obj = sizeof(database_object_entry_t) + sizeof(database_user_entry_t) + sizeof(database_edge_entry_t) * DATABASE_MAX_EDGE_PER_OBJECT;
    database_system_objects_get_reserved_count(&system_object_count);
    if(persist_system_db->number_of_objects_requested < system_object_count)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s requested object count: 0x%x doesn't agree with num objects: %d.\n",
                       __FUNCTION__, persist_system_db->number_of_objects_requested, system_object_count);

        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;

    }
    /*point to start of the buffer*/
    object_entry_offset_p = sg_element->address;
    user_entry_offset_p = object_entry_offset_p + system_object_count * sizeof(database_object_entry_t);
    edge_entry_offset_p = user_entry_offset_p + system_object_count * sizeof(database_user_entry_t);

    /*sanity check*/
    if ((sg_element->count / all_entries_size_per_obj) != system_object_count){
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s Byte count: 0x%x doesn't agree with num objects: %d.\n",
                       __FUNCTION__, sg_element->count, system_object_count);

        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    write_buffer = (fbe_u8_t *)fbe_memory_ex_allocate(system_object_count * DATABASE_MAX_EDGE_PER_OBJECT * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY);
    if (write_buffer == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s Can't allocate the memory.\n",
                       __FUNCTION__);

        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* First, persist the system object entries and user enties */
    /* Copy the system object entries to buffer */
    current_data = write_buffer;
    for (i = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST; i < system_object_count; i++) {
        fbe_copy_memory(current_data,object_entry_offset_p, sizeof(database_object_entry_t));
        /* goto next entry */
        current_data += DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY;
        object_entry_offset_p += sizeof(database_object_entry_t);
    }
    /* Copy the user entries */
    current_data = write_buffer + system_object_count * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY;
    for (i = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST; i < system_object_count; i++) {
        fbe_copy_memory(current_data, user_entry_offset_p, sizeof(database_user_entry_t));
        /* goto next entry */
        current_data += DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY;
        user_entry_offset_p += sizeof(database_user_entry_t);
    }
    status = database_system_db_get_entry_lba(FBE_DATABASE_SYSTEM_DB_OBJECT_ENTRY, FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST, 0, &lba);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: fail to get entry's lba, status = 0x%x\n",
                       __FUNCTION__,status);
        fbe_memory_ex_release(write_buffer);		
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    /* write system object entries and system user entries */
    status = database_system_db_interface_write(write_buffer,
        lba,
        system_object_count * 2 * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY,
        &write_count);
    if (status != FBE_STATUS_OK || (write_count != system_object_count * 2 * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE*DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY)) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: database_system_db_interface_write failed, status = 0x%x, write_count = 0x%llx\n",
                       __FUNCTION__,
                       status,
                       (unsigned long long)write_count);
        fbe_memory_ex_release(write_buffer);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* persist system edge entries */
    fbe_zero_memory(write_buffer, system_object_count * DATABASE_MAX_EDGE_PER_OBJECT * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE);
    write_count = 0;
    current_data = write_buffer;
    /* copy the edge entries */
    for (i = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST; i < system_object_count; i++) {
        for (j = 0; j < DATABASE_MAX_EDGE_PER_OBJECT; j++ ) {
            fbe_copy_memory(current_data, edge_entry_offset_p, sizeof(database_edge_entry_t));
            /* goto next entry */
            current_data += DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY;
            edge_entry_offset_p += sizeof(database_edge_entry_t);
        }
    }

    /*we persist edge entries object by object*/
    current_data = write_buffer;
    for (i = FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_FIRST; i < system_object_count; i++)
    {
        status = database_system_db_get_entry_lba(FBE_DATABASE_SYSTEM_DB_EDGE_ENTRY, i, 0, &lba);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: fail to get entry's lba, status = 0x%x\n",
                           __FUNCTION__,
                           status);
            fbe_memory_ex_release(write_buffer);
            fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        status = database_system_db_interface_write(current_data,
                 lba,
                 DATABASE_MAX_EDGE_PER_OBJECT * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY,
                 &write_count);
        if (status != FBE_STATUS_OK || (write_count != DATABASE_MAX_EDGE_PER_OBJECT * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY)) {
            database_trace(FBE_TRACE_LEVEL_ERROR,
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: database_system_db_interface_write failed, status = 0x%x, write_count = 0x%llx\n",
                           __FUNCTION__,
                           status,
                           (unsigned long long)write_count);
            fbe_memory_ex_release(write_buffer);
            fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        persist_system_db->number_of_objects_returned ++;
        current_data += DATABASE_MAX_EDGE_PER_OBJECT * DATABASE_SYSTEM_DB_BLOCK_DATA_SIZE * DATABASE_BLOCK_NUM_PER_SYSTEM_DB_ENTRY;
    }

    fbe_memory_ex_release(write_buffer);
    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}
static fbe_status_t database_control_get_max_configurable_objects(fbe_packet_t * packet)
{
    fbe_status_t                     status = FBE_STATUS_OK;
    fbe_database_control_get_max_configurable_objects_t  *get_max_config_objs = NULL;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&get_max_config_objs, sizeof(fbe_database_control_get_max_configurable_objects_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    get_max_config_objs->max_configurable_objects = fbe_database_service.max_configurable_objects;
    fbe_database_complete_packet(packet, status);
    return status;
}



/*!***************************************************************
* @fn database_control_make_planned_drive_online
*****************************************************************
* @brief
* make a planned drive online on this side, which was blocked online by homewrecker
* in earlier time. Then sync the same operations to peer SP.
*
* @param packet -
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
* @version
*    07/05/2012: Zhipeng Hu - created
****************************************************************/
static fbe_status_t database_control_make_planned_drive_online(fbe_packet_t * packet)
{
    fbe_database_control_online_planned_drive_t       *online_drive = NULL;
    fbe_status_t                      status;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&online_drive, sizeof(*online_drive));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: make planned drive online  %d_%d_%d.\n",
                   __FUNCTION__, online_drive->port_number,
                   online_drive->enclosure_number, online_drive->slot_number);

    /*make drive online on this side*/
    status = database_make_planned_drive_online(online_drive);
    if (status == FBE_STATUS_OK && is_peer_update_allowed(&fbe_database_service)) 
    {
        /*synch to the peer*/
        status = fbe_database_cmi_sync_online_drive_request_to_peer(online_drive);
    }


    fbe_database_complete_packet(packet, status);
    return status;
}

/*!***************************************************************
* @fn database_make_planned_drive_online
*****************************************************************
* @brief
* make a planned drive online on this SP, which was blocked online by homewrecker
* in earlier time
*
* @param packet -
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
* @version
*    07/05/2012: Zhipeng Hu - created
****************************************************************/
fbe_status_t database_make_planned_drive_online(fbe_database_control_online_planned_drive_t* online_drive)
{
    fbe_status_t    status;
    fbe_topology_control_get_physical_drive_by_location_t    topology_location;
    fbe_base_object_mgmt_get_lifecycle_state_t    get_lifecycle;

    if(NULL == online_drive)
        return FBE_STATUS_GENERIC_FAILURE;

    /*  get LDO id based on location */
    topology_location.port_number = online_drive->port_number;
    topology_location.enclosure_number = online_drive->enclosure_number;
    topology_location.slot_number = online_drive->slot_number;
    topology_location.physical_drive_object_id = FBE_OBJECT_ID_INVALID;

    status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_GET_PHYSICAL_DRIVE_BY_LOCATION,
                                                 &topology_location, 
                                                 sizeof(topology_location),
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 NULL,  /* no sg list*/
                                                 0,  /* no sg list*/
                                                 FBE_PACKET_FLAG_EXTERNAL,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if( FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "%s: failed to get pdo object id for drive %d_%d_%d\n", 
                     __FUNCTION__,
                     online_drive->port_number,
                     online_drive->enclosure_number,
                     online_drive->slot_number);

        return status;
    }

    online_drive->pdo_object_id = topology_location.physical_drive_object_id;

    /* get lifecycle of the LDO object */
    status  = fbe_database_send_packet_to_object(FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
                                                &get_lifecycle,
                                                sizeof(get_lifecycle),
                                                online_drive->pdo_object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_EXTERNAL,
                                                FBE_PACKAGE_ID_PHYSICAL);
    if( FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "%s: failed to get lifecycle state of LDO 0x%x\n", 
                     __FUNCTION__,
                     online_drive->pdo_object_id);
        return status;
    }

    /*add this drive to the pvd discover queue so that it would be processed by homewrecker
    * and do corresponding operations (such as create new PVD, reinitialize existing PVD, etc.)*/
    status = fbe_database_add_drive_to_be_processed(&fbe_database_service.pvd_operation,
                                                online_drive->pdo_object_id,
                                                get_lifecycle.lifecycle_state,
                                                DATABASE_PVD_DISCOVER_FLAG_HOMEWRECKER_FORECE_ONLINE);
    if( FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                     "%s: failed to add LDO 0x%x into discover queue \n", 
                     __FUNCTION__,
                     online_drive->pdo_object_id);
        return status;
    }

    return FBE_STATUS_OK; 

}

/*!***************************************************************
 * @fn fbe_database_get_rg_free_non_contigous_capacity
 *****************************************************************
 * @brief
 *   This function get free non contigous capaicty for a given raid group.
 *
 * @param  fbe_database_raid_group_info_t *rg_info
 *
 * @return FBE_STATUS_GENERIC_FAILURE if any issues.
 *
 * @version
 *    08/03/2012: Vera Wang - created
 *
 ****************************************************************/       
fbe_status_t fbe_database_get_rg_free_non_contigous_capacity(fbe_database_raid_group_info_t *rg_info, fbe_packet_t *packet)
{
    fbe_status_t                status;
    fbe_u32_t                   lun_index;
    fbe_database_lun_info_t     lun_info = {0};
    fbe_lba_t                   imported_capacity;

    if (rg_info == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: No Memory\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    rg_info->free_non_contigous_capacity = rg_info->rg_info.capacity;
    for (lun_index =0; lun_index < rg_info->lun_count; lun_index++) 
    {
        lun_info.lun_object_id = rg_info->lun_list[lun_index];
        status = database_get_lun_info(&lun_info, packet);
        if ( status != FBE_STATUS_OK )
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get lun info for object 0x%X\n", 
                           __FUNCTION__,
                           rg_info->lun_list[lun_index]);
            continue;
        }
        status = fbe_database_get_lun_imported_capacity(lun_info.capacity, rg_info->rg_info.element_size, &imported_capacity);
        if ( status != FBE_STATUS_OK )
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get LUN imported_capacity for object 0x%X\n", 
                           __FUNCTION__,
                           rg_info->lun_list[lun_index]);
            continue;
        }
        rg_info->free_non_contigous_capacity = rg_info->free_non_contigous_capacity - imported_capacity;
    }

    return FBE_STATUS_OK;        
}

/*!***************************************************************
 * @fn fbe_database_get_rg_free_contigous_capacity
 *****************************************************************
 * @brief
 *   This function get free contigous capaicty for a given raid group.
 *   Users may use this free contigous capacity to bind a single LUN to
 *   use up the whole free contigous capacity. To make this work,
 *   we have to report the LUN exported capacity as free contigous cap
 *   by subtracting the LUN metadata from the unused_extent_size.
 *
 * @param  fbe_database_raid_group_info_t *rg_info
 *
 * @return FBE_STATUS_GENERIC_FAILURE if any issues.
 *
 * @version
 *    11/05/2012: Vera Wang - created
 *
 ****************************************************************/       
fbe_status_t fbe_database_get_rg_free_contigous_capacity(fbe_database_raid_group_info_t *rg_info, fbe_packet_t * packet)
{
    fbe_status_t                status;
    fbe_block_transport_control_get_max_unused_extent_size_t rg_unused_extent_size;

    if (rg_info == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: No Memory\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* get the max unused extent size. */
    status = fbe_database_forward_packet_to_object(packet, 
                                                FBE_BLOCK_TRANSPORT_CONTROL_CODE_GET_MAX_UNUSED_EXTENT_SIZE,
                                                &rg_unused_extent_size,
                                                sizeof(rg_unused_extent_size),
                                                rg_info->rg_object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get max unused extent size for rg object 0x%X\n", 
                       __FUNCTION__,
                       rg_info->rg_object_id);
        return status;
    }

    /* the max unused extent size is LUN imported cap, it return LUN exported cap
       to rg_info extent size which represents max free contigous capacity.*/
    status = fbe_database_get_lun_exported_capacity(rg_unused_extent_size.extent_size, 
                                                    rg_info->rg_info.element_size, 
                                                    &rg_info->extent_size.extent_size);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get LUN exported_capacity for object 0x%X\n", 
                           __FUNCTION__,
                           rg_info->rg_object_id);
        return status;
    }

    return FBE_STATUS_OK;        
}

fbe_status_t database_perform_drive_connection(fbe_database_control_drive_connection_t* drive_connection_req)
{
    fbe_status_t    status;
    fbe_u32_t   i;
    database_object_entry_t*    pvd_entry  = NULL;

    if(NULL == drive_connection_req)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for(i = 0 ; i < drive_connection_req->request_size; i++)
    {
        fbe_spinlock_lock(&fbe_database_service.object_table.table_lock);
        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table, drive_connection_req->PVD_list[i], &pvd_entry);
        if(FBE_STATUS_OK != status)
        {
            fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get object entryfor pvd 0x%x\n", 
                           __FUNCTION__,
                           drive_connection_req->PVD_list[i]);
            continue;
        }

        if(DATABASE_CONFIG_ENTRY_INVALID == pvd_entry->header.state
            || DATABASE_CLASS_ID_PROVISION_DRIVE != pvd_entry->db_class_id)
        {    
            fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: the entry with objid 0x%x is not a valid PVD entry\n", 
                           __FUNCTION__,
                           drive_connection_req->PVD_list[i]);
            continue;
        }

        fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);
        status = fbe_database_add_pvd_to_be_connected(&fbe_database_service.pvd_operation, pvd_entry->header.object_id);
        if(FBE_STATUS_OK != status)
        {
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to add pvd 0x%x to connect queue. Maybe it is already in the queue\n", 
                           __FUNCTION__,
                           drive_connection_req->PVD_list[i]);
        }
    }

    return FBE_STATUS_OK;

}

static fbe_status_t database_control_connect_pvds(fbe_packet_t* packet)
{
    fbe_database_control_drive_connection_t      *drive_connect_request = NULL;        
    fbe_status_t                             status;
    fbe_status_t                             status_connect;

    if(NULL == packet)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&drive_connect_request, sizeof(*drive_connect_request));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    database_trace(FBE_TRACE_LEVEL_INFO, 
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "%s: enter entry.\n",
        __FUNCTION__);

    if (is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)drive_connect_request,
                                                             FBE_DATABE_CMI_MSG_TYPE_CONNECT_DRIVE);
        if(FBE_STATUS_OK != status)
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                FBE_TRACE_MESSAGE_ID_INFO,
                "%s: fail to update peer to connect drives.\n",
                __FUNCTION__);
        }
    }

    status_connect = database_perform_drive_connection(drive_connect_request);
    if(FBE_STATUS_OK != status_connect)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: fail to connect drives in our side.\n",
            __FUNCTION__);
    }

    if(FBE_STATUS_OK != status || FBE_STATUS_OK != status_connect)
    {
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_database_complete_packet(packet, status);
    return status;
}

/*!***************************************************************
 * @fn database_control_set_invalidate_config_flag
 *****************************************************************
* @brief
*       set invalidate config flag
*
* @param packet -packet
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    08/03/2012: Vera Wang - created
 *
 ****************************************************************/ 
static fbe_status_t database_control_set_invalidate_config_flag(fbe_packet_t* packet)
{
    fbe_database_control_set_invalidate_config_flag_t      *invalidate_config_flag = NULL;        
    fbe_status_t                             status;

    if(NULL == packet)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&invalidate_config_flag, sizeof(fbe_database_control_set_invalidate_config_flag_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                   "%s: enter entry. Invalidate config flag is 0x%x.\n",
                   __FUNCTION__, invalidate_config_flag->flag);  

    status = fbe_database_set_invalidate_configuration_flag (invalidate_config_flag->flag);
    if(status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: fail to set invalidate configuration flag.\n",
                       __FUNCTION__);
    }

    fbe_database_complete_packet(packet, status);
    return status;
}

/*!*******************************************************************************************
 * @fn database_perform_hook
 *********************************************************************************************
* @brief
*  set/unset/ hook or get hook state from this function
*
* @param packet - request packet with fbe_database_control_hook_t control buffer in
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    11/25/2012: Zhipeng Hu - created
 *
 ********************************************************************************************/ 
static fbe_status_t database_perform_hook(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                  payload;
    fbe_payload_control_operation_t*    control_operation;
    fbe_database_control_hook_t*         hook_operation;
    fbe_u32_t                           buffer_length;
    fbe_status_t                        status;

    payload = fbe_transport_get_payload_ex(packet);
    if (payload == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: get_sep_payload failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    control_operation = fbe_payload_ex_get_control_operation(payload);
    if (control_operation == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s: get_control_operation() failed\n", __FUNCTION__);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    hook_operation = NULL;
    fbe_payload_control_get_buffer(control_operation, &hook_operation);
    if (hook_operation == NULL){
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s fbe_payload_control_get_buffer failed\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    buffer_length = 0;
    fbe_payload_control_get_buffer_length(control_operation, &buffer_length);
    if(buffer_length != sizeof(fbe_database_control_hook_t)){
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s Invalid control buffer length %d, expected %d\n",
            __FUNCTION__, buffer_length, (int)sizeof(fbe_database_control_hook_t));
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, FBE_STATUS_GENERIC_FAILURE, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch(hook_operation->hook_op_code)
    {
    case FBE_DATABASE_HOOK_OPERATION_GET_STATE:
        status = database_get_hook_state(hook_operation->hook_type, &hook_operation->is_set,&hook_operation->is_triggered);
        break;
    case FBE_DATABASE_HOOK_OPERATION_REMOVE_HOOK:
        status = database_remove_hook(hook_operation->hook_type);
        break;

    case FBE_DATABASE_HOOK_OPERATION_SET_HOOK:
        status = database_set_hook(hook_operation->hook_type);
        break;

    default:
        status = FBE_STATUS_GENERIC_FAILURE;
        break;
    }

    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR,
            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
            "%s failed opt. hook type: %d, hook opt: %d\n",
            __FUNCTION__, hook_operation->hook_type, hook_operation->hook_op_code);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return status;
    }

    fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return FBE_STATUS_OK;
}

static fbe_status_t database_trace_lun_user_info(database_user_entry_t *lun_user_entry)
{
    fbe_u32_t   index = 0;
    fbe_u8_t wwn_bytes[3*FBE_WWN_BYTES + 16];
    fbe_u8_t *wwn_bytes_ptr = &wwn_bytes[0];
    database_lu_user_data_t *lun_data = NULL;

    if(lun_user_entry == NULL)
        return FBE_STATUS_GENERIC_FAILURE;

    fbe_zero_memory(wwn_bytes_ptr, sizeof(wwn_bytes));
    lun_data = &lun_user_entry->user_data_union.lu_user_data;
    
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");
    
    for(index = 0; index < FBE_WWN_BYTES; index++)
    {
        fbe_sprintf(wwn_bytes_ptr, 4, "%02x",(fbe_u32_t)(lun_data->world_wide_name.bytes[index]));
        wwn_bytes_ptr++;
        wwn_bytes_ptr++;
        
        if(index+1 != FBE_WWN_BYTES)
        {
            *wwn_bytes_ptr = ':';
            wwn_bytes_ptr++;
        }
    }

    database_trace(FBE_TRACE_LEVEL_INFO,
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "    world_wide_name: %s\n", &wwn_bytes[0]);
    
    database_trace(FBE_TRACE_LEVEL_INFO,
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "    user_defined_name: %s\n", &lun_data->user_defined_name.name[0]);
    
    database_trace(FBE_TRACE_LEVEL_INFO,
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "    lun_number: 0x%x\n", lun_data->lun_number);
        
    database_trace(FBE_TRACE_LEVEL_INFO,
            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "    user_private %d, bind_time 0x%llx\n",
            lun_data->user_private,
            (unsigned long long)lun_data->bind_time);

    return FBE_STATUS_OK;
}


static fbe_status_t database_trace_rg_user_info(database_user_entry_t *rg_user_entry)
{
    database_rg_user_data_t *rg_data = NULL;

    if(rg_user_entry == NULL)
        return FBE_STATUS_GENERIC_FAILURE;

    rg_data = &rg_user_entry->user_data_union.rg_user_data;
    
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
            "- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -\n");

    database_trace(FBE_TRACE_LEVEL_INFO,
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "    raid_group_number: 0x%x\n", rg_data->raid_group_number);
    
    database_trace(FBE_TRACE_LEVEL_INFO,
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "    is_system: %d\n", rg_data->is_system);
    
    database_trace(FBE_TRACE_LEVEL_INFO,
        FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
        "    user_private: %d\n", rg_data->user_private);

    return FBE_STATUS_OK;
}

/* is VD'd for this RG connected to system PVD */
fbe_bool_t fbe_database_is_vd_connected_to_system_drive(fbe_object_id_t rg_object_id, fbe_u32_t width)
{
    fbe_u32_t vd_edge_index;
    fbe_u32_t disk_index;
    database_edge_entry_t *edge_to_vd = NULL;
    database_edge_entry_t *edge_to_pvd = NULL;
    fbe_status_t status;

    /* Determine if any VD is pointing to a PVD on the system drives. */
    for (disk_index = 0; disk_index < width; disk_index++) {
        status = fbe_database_config_table_get_edge_entry(&fbe_database_service.edge_table,
                                                          rg_object_id,
                                                          disk_index,
                                                          &edge_to_vd);
        if (status != FBE_STATUS_OK){
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get edge entry for rg object %u index %u\n", 
                           __FUNCTION__, rg_object_id, disk_index);
            return FBE_FALSE;
        }
        /* Check all vd edges since if the VD did a copy or is performing a copy it 
         * might be using all edges. 
         */
        for (vd_edge_index = 0; vd_edge_index < 2; vd_edge_index++)
        {
            status = fbe_database_config_table_get_edge_entry(&fbe_database_service.edge_table,
                                                              edge_to_vd->server_id,
                                                              vd_edge_index,
                                                              &edge_to_pvd);    
            if (status != FBE_STATUS_OK){
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: failed to get edge entry for vd object 0x%x rg object 0x%x\n", 
                               __FUNCTION__, edge_to_vd->server_id, rg_object_id);
                return FBE_FALSE;
            }
            if ((edge_to_pvd->header.state == DATABASE_CONFIG_ENTRY_VALID) &&
                (edge_to_pvd->server_id != FBE_OBJECT_ID_INVALID) &&
                (edge_to_pvd->server_id <= FBE_PRIVATE_SPACE_LAYOUT_OBJECT_ID_LAST)){
                return FBE_TRUE;
                
            }
        }
    }

    return FBE_FALSE;
}


fbe_bool_t fbe_database_is_rg_with_system_drive(fbe_object_id_t rg_object_id)
{
    fbe_status_t status;
    database_edge_entry_t *edge = NULL;
    database_object_entry_t *rg_object_entry = NULL;
    fbe_u32_t edge_index;

    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              rg_object_id,
                                                              &rg_object_entry);  
    if (status != FBE_STATUS_OK){
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get rg entry for rg object %u\n", 
                       __FUNCTION__, rg_object_id);
        return FBE_FALSE;
    }

    if (fbe_private_space_layout_object_id_is_system_raid_group(rg_object_entry->header.object_id)) {
        return FBE_TRUE;
    }

    /* Check if it is Raid 10 */
    if(rg_object_entry->set_config_union.rg_config.raid_type == FBE_RAID_GROUP_TYPE_RAID10){
        for(edge_index = 0; edge_index < rg_object_entry->set_config_union.rg_config.width; edge_index++){
            status = fbe_database_config_table_get_edge_entry(&fbe_database_service.edge_table,
                                                              rg_object_entry->header.object_id,
                                                              edge_index,
                                                              &edge);    
            if (status != FBE_STATUS_OK){
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: failed to get edge entry for vd object 0x%x rg object 0x%x\n", 
                               __FUNCTION__, edge_index, rg_object_entry->header.object_id);
                return FBE_FALSE;
            }

            if(fbe_database_is_vd_connected_to_system_drive(edge->server_id, 2)){
                return FBE_TRUE;
            }

        }/* loop over the edges of Raid 10 */
    } else { /* Not Raid 10 */
        if(fbe_database_is_vd_connected_to_system_drive(rg_object_entry->header.object_id, rg_object_entry->set_config_union.rg_config.width)){
            return FBE_TRUE;
        }
    }
    return FBE_FALSE;
}

fbe_status_t fbe_database_get_ds_objects(fbe_object_id_t object_id,
                                         fbe_u32_t edges,
                                         fbe_database_ds_object_list_t *ds_list_p)
{
    fbe_status_t status;
    fbe_u32_t vd_edge_index;
    database_edge_entry_t *edge = NULL;
    database_object_entry_t *object_entry = NULL;

    ds_list_p->number_of_downstream_objects = 0;
    /* Check all edges since if the VD did a copy or is performing a copy it might be using all edges.
     */
    for (vd_edge_index = 0; vd_edge_index < edges; vd_edge_index++) {
        status = fbe_database_config_table_get_edge_entry(&fbe_database_service.edge_table,
                                                          object_id,
                                                          vd_edge_index,
                                                          &edge);    
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get edge entry for ds object 0x%x\n", 
                           __FUNCTION__, object_id);
            return status;
        }

        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                  edge->server_id,
                                                                  &object_entry);  

        if ((status == FBE_STATUS_OK) && 
            database_is_entry_valid(&object_entry->header)){
            ds_list_p->downstream_object_list[ds_list_p->number_of_downstream_objects] = edge->server_id;
            ds_list_p->number_of_downstream_objects++;
        }
    }
    return FBE_STATUS_OK;
}

fbe_status_t fbe_database_rg_get_vd_pvd_list(fbe_object_id_t rg_object_id,
                                             fbe_database_ds_object_list_t *ds_list_p)
{
    fbe_status_t status;
    fbe_u32_t disk_index;
    database_edge_entry_t *edge_to_vd = NULL;
    database_object_entry_t *rg_object_entry = NULL;
    database_object_entry_t *vd_object_entry = NULL;
    database_object_entry_t *pvd_object_entry = NULL;
    fbe_database_ds_object_list_t ds_list;
    fbe_database_ds_object_list_t ds2_list;
    fbe_u32_t lev1_index, lev2_index;

    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              rg_object_id,
                                                              &rg_object_entry);  
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get rg entry for rg object %u\n", 
                       __FUNCTION__, rg_object_id);
        return status;
    }

    /* Get the VD's PVD object id.
     */
    ds_list_p->number_of_downstream_objects = 0;
    for (disk_index = 0; disk_index < rg_object_entry->set_config_union.rg_config.width; disk_index++) {
        status = fbe_database_config_table_get_edge_entry(&fbe_database_service.edge_table,
                                                          rg_object_entry->header.object_id,
                                                          disk_index,
                                                          &edge_to_vd);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get edge entry for rg object %u index %u\n", 
                           __FUNCTION__, rg_object_entry->header.object_id, disk_index);
            return status;
        }

        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                  edge_to_vd->server_id,
                                                                  &vd_object_entry);  
        
        if ((status != FBE_STATUS_OK) || 
            !database_is_entry_valid(&vd_object_entry->header)){
            continue;
        }
        /* Add the next object.
         */
        ds_list_p->downstream_object_list[ds_list_p->number_of_downstream_objects] = edge_to_vd->server_id;
        ds_list_p->number_of_downstream_objects++;

        if (vd_object_entry->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE){
            /* There is no VD.  PVD was already added above.
             */
            continue;
        }
        status = fbe_database_get_ds_objects(edge_to_vd->server_id, 2, &ds_list);
        if (status != FBE_STATUS_OK) {
            continue;
        }
        for (lev1_index = 0; lev1_index < ds_list.number_of_downstream_objects; lev1_index++) {
            status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                      ds_list.downstream_object_list[lev1_index],
                                                                      &pvd_object_entry);  

            if ((status == FBE_STATUS_OK) &&
                database_is_entry_valid(&pvd_object_entry->header)) {
                if ((pvd_object_entry->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE) ||
                    (pvd_object_entry->db_class_id == DATABASE_CLASS_ID_VIRTUAL_DRIVE)) {
                    ds_list_p->downstream_object_list[ds_list_p->number_of_downstream_objects] = ds_list.downstream_object_list[lev1_index];
                    ds_list_p->number_of_downstream_objects++;

                    if (pvd_object_entry->db_class_id == DATABASE_CLASS_ID_VIRTUAL_DRIVE) {
                        status = fbe_database_get_ds_objects(ds_list.downstream_object_list[lev1_index], 2, &ds2_list);
                        if (status != FBE_STATUS_OK) {
                            continue;
                        }
                        for (lev2_index = 0; lev2_index < ds_list.number_of_downstream_objects; lev2_index++) {
                            status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                                      ds2_list.downstream_object_list[lev2_index],
                                                                                      &pvd_object_entry);  

                            if ((status == FBE_STATUS_OK) &&
                                database_is_entry_valid(&pvd_object_entry->header)) {
                                if (pvd_object_entry->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE) {
                                    ds_list_p->downstream_object_list[ds_list_p->number_of_downstream_objects] = 
                                        ds2_list.downstream_object_list[lev2_index];
                                    ds_list_p->number_of_downstream_objects++;
                                }
                            }

                        }
                    }
                }
            }
        }
    }
    return FBE_STATUS_OK;
}

/*this is used to make sure we spend as little time as possible collecting information for the
IOCTL_FLARE_GET_RAID_INFO ioctl since there is a system requirement to complete is fast,
but MCR breaks the context to the ioctl thread so we have to make sure we don't make things even worse
by allocating pacets and waiting for memory*/
fbe_status_t fbe_database_get_sep_shim_raid_info(fbe_database_get_sep_shim_raid_info_t *sep_shim_info)
{
    database_object_entry_t             *lun_object_entry = NULL;
    database_object_entry_t             *rg_object_entry = NULL;
    database_user_entry_t *				lun_user_entry = NULL;
    database_user_entry_t *				rg_user_entry = NULL;
    fbe_status_t                     	status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_lun_configuration_t *	lun_config = NULL;
    fbe_raid_group_configuration_t *	rg_config = NULL;
    database_lu_user_data_t *			lun_user = NULL;
    fbe_block_count_t 					cache_zero_bit_map_size;
    database_edge_entry_t               *edge_to_raid = NULL;
    fbe_u32_t							attributes = 0;
    fbe_u16_t 							data_disks = 0;
    fbe_u32_t                           width = 0;


    /******************************************************************/
    /* when/if adding to this function, do the best you can not to     */
    /* use packets to get the info but rather get it only from memory */ 
    /* accessible to you at the DB level                              */
    /******************************************************************/

    
    /* Get the object table entry for the lun */		
    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              sep_shim_info->lun_object_id,
                                                              &lun_object_entry);  

    if ((status != FBE_STATUS_OK) || !database_is_entry_valid(&lun_object_entry->header)){
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get object entry for LU object %u\n", 
                       __FUNCTION__, 
                       sep_shim_info->lun_object_id);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (lun_object_entry->db_class_id == DATABASE_CLASS_ID_EXTENT_POOL_LUN){
        return fbe_database_get_sep_shim_ext_lun_raid_info(sep_shim_info);
    }

    lun_config = &lun_object_entry->set_config_union.lu_config;

    /* Get the user table entry for the lun */		
    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                                   sep_shim_info->lun_object_id,
                                                                   &lun_user_entry);  

    if ((status != FBE_STATUS_OK) || !database_is_entry_valid(&lun_user_entry->header)){
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get user entry for LU object %u\n", 
                       __FUNCTION__, 
                       sep_shim_info->lun_object_id);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    lun_user = &lun_user_entry->user_data_union.lu_user_data;

    /* Get the edge to the raid object */		
    status = fbe_database_config_table_get_edge_entry(&fbe_database_service.edge_table,
                                                      sep_shim_info->lun_object_id,
                                                      0,
                                                      &edge_to_raid);  

    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              edge_to_raid->server_id,
                                                              &rg_object_entry);  

    if ((status != FBE_STATUS_OK) || !database_is_entry_valid(&rg_object_entry->header)){
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get object entry for RG object %u\n", 
                       __FUNCTION__, 
                       edge_to_raid->server_id);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    /*and the user table of the RG*/
    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                                   edge_to_raid->server_id,
                                                                   &rg_user_entry);  

    if ((status != FBE_STATUS_OK) || !database_is_entry_valid(&rg_user_entry->header)){
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get user entry for RG object %u\n", 
                       __FUNCTION__, 
                       sep_shim_info->lun_object_id);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    rg_config = &rg_object_entry->set_config_union.rg_config;
    width = rg_config->width;
    if (rg_config->raid_type == FBE_RAID_GROUP_TYPE_RAID10){
        width *= 2;
    }
    fbe_raid_group_class_get_queue_depth(rg_object_entry->header.object_id,
                                         width,
                                         &sep_shim_info->max_queue_depth);


    
    /*fill up all the data*/
    sep_shim_info->bind_time = lun_user->bind_time;
    sep_shim_info->raid_group_number = rg_user_entry->user_data_union.rg_user_data.raid_group_number;
    sep_shim_info->raid_group_id = rg_object_entry->header.object_id;

    /*we need to hide from the user the fact we created a bit more for sp cache to use*/
    fbe_lun_calculate_cache_zero_bit_map_size_to_remove(lun_config->capacity, &cache_zero_bit_map_size);
    sep_shim_info->capacity = lun_config->capacity - cache_zero_bit_map_size;

    sep_shim_info->element_size = rg_config->element_size;
    sep_shim_info->elements_per_parity_stripe = rg_config->elements_per_parity;

    /*technically we need to get that from RAID. If we change this in the future, we'll need to get that directly 
    from RAID using the susurper FBE_RAID_GROUP_CONTROL_CODE_GET_INFO because the RG sets this when it does the negotiation.
    Per Rob, this is OK to use it for the very long run...*/
    sep_shim_info->exported_block_size = FBE_BE_BYTES_PER_BLOCK;

    database_map_psl_lun_to_attributes(sep_shim_info->lun_object_id, &attributes);

    /*if it's any of the VCX luns, we'll set it's characteristics as FLARE_LUN_CHARACTERISTIC_CELERRA*/
    if (attributes & FBE_LU_ATTRIBUTES_VCX_MASK) {
        sep_shim_info->lun_characteristics = FBE_LUN_CHARACTERISTIC_CELERRA; 
    }

    /*get rotational rate (flash or not)*/
    database_set_lun_rotational_rate(rg_object_entry, &sep_shim_info->rotational_rate);

    /*and the sectors per stripe*/
    sep_shim_info->width = width;
    sep_shim_info->raid_type = rg_config->raid_type;

    status = fbe_raid_type_get_data_disks(rg_config->raid_type, rg_config->width, &data_disks);
    if (status != FBE_STATUS_OK){
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get data disk from RG: %u\n", 
                       __FUNCTION__, 
                       edge_to_raid->server_id);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    sep_shim_info->sectors_per_stripe = rg_config->element_size * data_disks;

    /*and other naming related things*/
    fbe_copy_memory(sep_shim_info->user_defined_name.name, lun_user->user_defined_name.name, sizeof(fbe_user_defined_lun_name_t));
    fbe_copy_memory(sep_shim_info->world_wide_name.bytes, lun_user->world_wide_name.bytes, sizeof(fbe_assigned_wwid_t));

    /******************************************************************/
    /* when/if adding to this function, do the best you can not to     */
    /* use packets to get the info but rather get it only from memory */ 
    /* accessible to you at the DB level                              */
    /******************************************************************/

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_database_get_sep_shim_ext_lun_raid_info(fbe_database_get_sep_shim_raid_info_t *sep_shim_info)
{
    database_object_entry_t             *lun_object_entry = NULL;
    database_object_entry_t             *ext_pool_object_entry = NULL;
    database_user_entry_t *				lun_user_entry = NULL;
    database_user_entry_t *				ext_pool_user_entry = NULL;
    fbe_status_t                     	status = FBE_STATUS_GENERIC_FAILURE;
    fbe_ext_pool_lun_configuration_t *	lun_config = NULL;
    database_ext_pool_lun_user_data_t *	lun_user = NULL;
    fbe_block_count_t 					cache_zero_bit_map_size;
    fbe_u32_t							attributes = 0;
    fbe_u16_t 							data_disks = 0;
    fbe_u32_t                           width = 0;


    /******************************************************************/
    /* when/if adding to this function, do the best you can not to     */
    /* use packets to get the info but rather get it only from memory */ 
    /* accessible to you at the DB level                              */
    /******************************************************************/

    
    /* Get the object table entry for the lun */		
    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              sep_shim_info->lun_object_id,
                                                              &lun_object_entry);  

    if ((status != FBE_STATUS_OK) || !database_is_entry_valid(&lun_object_entry->header)){
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get object entry for LU object %u\n", 
                       __FUNCTION__, 
                       sep_shim_info->lun_object_id);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    lun_config = &lun_object_entry->set_config_union.ext_pool_lun_config;

    /* Get the user table entry for the lun */		
    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                                   sep_shim_info->lun_object_id,
                                                                   &lun_user_entry);  

    if ((status != FBE_STATUS_OK) || !database_is_entry_valid(&lun_user_entry->header)){
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get user entry for LU object %u\n", 
                       __FUNCTION__, 
                       sep_shim_info->lun_object_id);

        return FBE_STATUS_GENERIC_FAILURE;
    }

    lun_user = &lun_user_entry->user_data_union.ext_pool_lun_user_data;

    status = fbe_database_config_table_get_user_entry_by_ext_pool_id(&fbe_database_service.user_table, 
                                                                     lun_user_entry->user_data_union.ext_pool_lun_user_data.pool_id,
                                                                     &ext_pool_user_entry);

    if ((status != FBE_STATUS_OK) || !database_is_entry_valid(&ext_pool_user_entry->header)){
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get object entry for ext pool id %u\n", 
                       __FUNCTION__, 
                       lun_user_entry->user_data_union.ext_pool_lun_user_data.pool_id);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              ext_pool_user_entry->header.object_id,
                                                              &ext_pool_object_entry);  

    if ((status != FBE_STATUS_OK) || !database_is_entry_valid(&ext_pool_object_entry->header)){
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get object entry for ext pool %u\n", 
                       __FUNCTION__, 
                       lun_user_entry->user_data_union.ext_pool_lun_user_data.pool_id);

        return FBE_STATUS_GENERIC_FAILURE;
    }
    width = ext_pool_object_entry->set_config_union.ext_pool_config.width;
    sep_shim_info->max_queue_depth = width * 100;
    
    /*fill up all the data*/
    sep_shim_info->bind_time = 0; /*lun_user->bind_time;*/
    sep_shim_info->raid_group_number = ext_pool_user_entry->user_data_union.ext_pool_user_data.pool_id;

    /*we need to hide from the user the fact we created a bit more for sp cache to use*/
    fbe_lun_calculate_cache_zero_bit_map_size_to_remove(lun_config->capacity, &cache_zero_bit_map_size);
    sep_shim_info->capacity = lun_config->capacity - cache_zero_bit_map_size;

    sep_shim_info->element_size = FBE_RAID_SECTORS_PER_ELEMENT; /*! @todo  hard coded. */
    sep_shim_info->elements_per_parity_stripe = FBE_RAID_ELEMENTS_PER_PARITY; /*! @todo  hard coded. */

    /*technically we need to get that from RAID. If we change this in the future, we'll need to get that directly 
    from RAID using the susurper FBE_RAID_GROUP_CONTROL_CODE_GET_INFO because the RG sets this when it does the negotiation.
    Per Rob, this is OK to use it for the very long run...*/
    sep_shim_info->exported_block_size = FBE_BE_BYTES_PER_BLOCK;

    database_map_psl_lun_to_attributes(sep_shim_info->lun_object_id, &attributes);

    /*if it's any of the VCX luns, we'll set it's characteristics as FLARE_LUN_CHARACTERISTIC_CELERRA*/
    if (attributes & FBE_LU_ATTRIBUTES_VCX_MASK) {
        sep_shim_info->lun_characteristics = FBE_LUN_CHARACTERISTIC_CELERRA; 
    }

    /*get rotational rate (flash or not)*/
    if (ext_pool_object_entry->set_config_union.ext_pool_config.drives_type == FBE_DRIVE_TYPE_SAS_FLASH_HE
        || ext_pool_object_entry->set_config_union.ext_pool_config.drives_type == FBE_DRIVE_TYPE_SATA_FLASH_HE
        || ext_pool_object_entry->set_config_union.ext_pool_config.drives_type == FBE_DRIVE_TYPE_SAS_FLASH_ME
        || ext_pool_object_entry->set_config_union.ext_pool_config.drives_type == FBE_DRIVE_TYPE_SAS_FLASH_LE
        || ext_pool_object_entry->set_config_union.ext_pool_config.drives_type == FBE_DRIVE_TYPE_SAS_FLASH_RI) {
        /* if the raid group is on flash drive, set rotational_rate to 1. */
        sep_shim_info->rotational_rate = 1;
    } else {
        sep_shim_info->rotational_rate = 0;
    } 

    /*and the sectors per stripe*/
    sep_shim_info->width = width;
    sep_shim_info->raid_type = FBE_RAID_GROUP_TYPE_RAID5;/*! @todo hard coded for now. */

    data_disks = (width / 5) * 4; /*! @todo hard coded for now. */

    sep_shim_info->sectors_per_stripe = sep_shim_info->element_size * 4; /* previously used data disks, hard coding to 4 for now.*/

    /*and other naming related things*/
    fbe_copy_memory(sep_shim_info->user_defined_name.name, lun_user->user_defined_name.name, sizeof(fbe_user_defined_lun_name_t));
    fbe_copy_memory(sep_shim_info->world_wide_name.bytes, lun_user->world_wide_name.bytes, sizeof(fbe_assigned_wwid_t));

    /******************************************************************/
    /* when/if adding to this function, do the best you can not to     */
    /* use packets to get the info but rather get it only from memory */ 
    /* accessible to you at the DB level                              */
    /******************************************************************/

    return FBE_STATUS_OK;
}


static fbe_status_t database_control_lu_operation_from_cli(fbe_packet_t * packet)
{
    fbe_status_t                     status = FBE_STATUS_OK;
    fbe_database_set_lu_operation_t  *lu_operation = NULL;
    
    if(NULL == packet)
    {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&lu_operation, sizeof(fbe_database_set_lu_operation_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    if (lu_operation->operation == FBE_DATABASE_LUN_CREATE_FBE_CLI) {

       status = fbe_event_log_write(SEP_INFO_LUN_CREATED_FBE_CLI, NULL, 0, "%x %d",                                         
                                 lu_operation->lun_id,
                                 lu_operation->status);  
       database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Created LUN 0x%x from FBE CLI; Status: 0x%x.\n",
            __FUNCTION__, lu_operation->lun_id, lu_operation->status);
    }
    else {
       status = fbe_event_log_write(SEP_INFO_LUN_DESTROYED_FBE_CLI, NULL, 0, "%x %d",                                         
                                 lu_operation->lun_id,
                                 lu_operation->status);  
       database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Destroyed LUN 0x%x from FBE CLI; Status: 0x%x\n",
            __FUNCTION__, lu_operation->lun_id, lu_operation->status); 
    }

    
    fbe_database_complete_packet(packet, FBE_STATUS_OK);

    return FBE_STATUS_OK;
}
static fbe_status_t fbe_database_get_lun_raid_info(fbe_packet_t * packet)
{
    fbe_database_get_sep_shim_raid_info_t *raid_info_p = NULL;
    fbe_status_t status = FBE_STATUS_OK;
    
    if (packet == NULL){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&raid_info_p, sizeof(fbe_database_get_sep_shim_raid_info_t));
    if ( status != FBE_STATUS_OK ){
        fbe_database_complete_packet(packet, status);
        return status;
    }
    status = fbe_database_get_sep_shim_raid_info(raid_info_p);

    fbe_database_complete_packet(packet, status);

    return FBE_STATUS_OK;
}

/*!***************************************************************
* @fn fbe_database_cmi_disable_service_when_lost_peer
*****************************************************************
* @brief
* Disable the database service for serving clients (i.e., job service) when peer is
* lost.
*
* This is executed in the CMI callback context, and only do actuall disabling if:
* (1) We are passive SP before peer lost
* (2) We are already being able to serve clients before peer lost
*
* The reason why we do this is passive SP database reinitialization after peer died
* is performed in another context (fbe_db_cmi_thread_func), rather than in this
* CMI callback context. So db reinit may happen later than job serivce being able
* to execute jobs after its processing peer lost. If this case happens, the job execution and
* db reinit would be in parallel, which would corrupt database cfg. So we must disable
* database here in the CMI callback context, which is before the job service's CMI callback.
*
* @param NONE
*
* @return none
* @created: Zhipeng Hu, 05/18/2013
* 
****************************************************************/
void fbe_database_cmi_disable_service_when_lost_peer(void)
{
    /*If we are able to serve client (such as job service) now and we are passive SP before
      *peer panics, we should disable the service temporarily because we will do reinitialize
      *in another thread context*/
    if(fbe_database_service.state == FBE_DATABASE_STATE_READY &&
       fbe_database_service.db_become_ready_in_passiveSP == FBE_TRUE)
    {
        /*indicate database was initialized but not READY to serve client*/
        fbe_database_service.state = FBE_DATABASE_STATE_INITIALIZED;
    }

    return;
}
/*!***************************************************************
* @end fbe_database_cmi_disable_service_when_lost_peer
*****************************************************************/

/*!**************************************************************
 * fbe_db_init_data_memory()
 ****************************************************************
 * @brief
 *  This function initialize data memory from CMM. 
 *
 * @param total_mem_mb - total MB assigned from CMM.
 * @param virtual_addr - virtual address.
 *
 * @return fbe_status_t
 * 
 * @author
 *  09/25/2013 - Created. Ashok Tamilarasan
 *
 ****************************************************************/
fbe_status_t fbe_db_init_data_memory(fbe_u32_t total_mem_mb, void * virtual_addr)
{
    fbe_database_encryption_init(total_mem_mb, virtual_addr);

    
    return FBE_STATUS_OK;
}

/*!***************************************************************
* @fn database_control_get_peer_state
*****************************************************************
* @brief
* Get DB Peer state
*
* @param NONE
*
* @return status
****************************************************************/
static fbe_status_t database_control_get_peer_state(fbe_packet_t * packet)
{
    fbe_status_t                     status = FBE_STATUS_OK;
    fbe_database_control_get_state_t *get_state = NULL;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&get_state, sizeof(*get_state));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    /* Get peer state */
    get_database_service_peer_state(&fbe_database_service, &get_state->state);

    fbe_database_complete_packet(packet, status);

    return status;
} /* end of database_control_get_peer_state() */

/*!**************************************************************
 * fbe_database_is_table_commit_required()
 ****************************************************************
 * @brief
 *  This function checks to see if a table needs to be updated during commit
 *
 * @param table_type - table type.
 *
 * @return fbe_bool_t
 * 
 * @author
 *
 ****************************************************************/
fbe_bool_t fbe_database_is_table_commit_required(database_config_table_type_t table_type)
{
    fbe_bool_t                          commit_required = FBE_FALSE;

    switch (table_type)
    {
    case DATABASE_CONFIG_TABLE_GLOBAL_INFO:
        commit_required = fbe_database_config_is_global_info_commit_required(&fbe_database_service.global_info);
        break;
    case DATABASE_CONFIG_TABLE_OBJECT:
        commit_required = fbe_database_config_is_object_table_commit_required(&fbe_database_service.object_table);
        break;
    default:
        break;
    }
    return (commit_required);
}

/*!**************************************************************
 * fbe_database_update_table_prior_commit()
 ****************************************************************
 * @brief
 *  This function updates in memory database table as need as for NDU commit
 *
 * @param table_type - table type.
 *
 * @return fbe_status_t
 * 
 * @author
 *
 ****************************************************************/
fbe_status_t fbe_database_update_table_prior_commit(database_config_table_type_t table_type)
{
    switch (table_type)
    {
    case DATABASE_CONFIG_TABLE_GLOBAL_INFO:
        fbe_database_config_update_global_info_prior_commit(&fbe_database_service.global_info);
        break;
    case DATABASE_CONFIG_TABLE_OBJECT:
        fbe_database_config_update_object_table_prior_commit(&fbe_database_service.object_table);
        break;
    default:
        break;
    }

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_database_commit_table()
 ****************************************************************
 * @brief
 *  This function persist in memory database table 
 *
 * @param table_type - table type.
 *
 * @return fbe_status_t
 * 
 * @author
 *
 ****************************************************************/
fbe_status_t fbe_database_commit_table(database_config_table_type_t table_type)
{
    fbe_status_t status = FBE_STATUS_OK;

    switch (table_type)
    {
    case DATABASE_CONFIG_TABLE_GLOBAL_INFO:
        status = fbe_database_config_commit_global_info(&fbe_database_service.global_info);
        break;
    case DATABASE_CONFIG_TABLE_OBJECT:
        status = fbe_database_config_commit_object_table(&fbe_database_service.object_table);
        break;
    default:
        break;
    }

    return status;
}

/*!**************************************************************
 * fbe_database_update_local_table_after_peer_commit()
 ****************************************************************
 * @brief
 *  This function update local database table after peer commits
 *
 * @return fbe_status_t
 * 
 * @author
 *
 ****************************************************************/
fbe_status_t fbe_database_update_local_table_after_peer_commit(void)
{
    if (fbe_database_is_table_commit_required(DATABASE_CONFIG_TABLE_OBJECT))
    {
        fbe_database_update_table_prior_commit(DATABASE_CONFIG_TABLE_OBJECT);
    }

    if (fbe_database_is_table_commit_required(DATABASE_CONFIG_TABLE_GLOBAL_INFO))
    {
        fbe_database_update_table_prior_commit(DATABASE_CONFIG_TABLE_GLOBAL_INFO);
    }
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_control_get_total_locked_objects_of_class()
 *****************************************************************
 * @brief
 *   This functions gather the total number of Objects that are
 *   in the locked state waiting for keys
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    10/23/2013: Ashok Tamilarasan - Created
 *
 ****************************************************************/
static fbe_status_t fbe_database_control_get_total_locked_objects_of_class(fbe_packet_t * packet)
{
    fbe_status_t                     		status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_get_total_locked_objects_of_class_t *	get_all_objects = NULL;  
    database_object_entry_t *	            object_entry = NULL;
    fbe_object_id_t							object_id;
    fbe_base_config_control_get_encryption_state_t get_encryption_state;
    database_transaction_t    * transaction;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&get_all_objects, sizeof(fbe_database_control_get_total_locked_objects_of_class_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    get_all_objects->number_of_objects = 0;
    /* Report nothing if we are in the middle of transaction */
    /* It is not safe to do it without proper locking, but may work for now */
    transaction = fbe_database_service.transaction_ptr;
    if(transaction != NULL){
        if(transaction->state != DATABASE_TRANSACTION_STATE_INACTIVE){
            fbe_database_complete_packet(packet, FBE_STATUS_OK);
            return FBE_STATUS_OK;
        }
    }

    /*go over the tables, get objects that are rgs and get the encryption state */
    for (object_id = 0; object_id < fbe_database_service.object_table.table_size; object_id++) {
        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                  object_id,
                                                                  &object_entry);  

        if ( status != FBE_STATUS_OK){
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get object entry for object 0x%X\n",__FUNCTION__, object_id);
            continue;
        }
        if (database_is_entry_valid(&object_entry->header)) 
        {
            if(((get_all_objects->class_id == DATABASE_CLASS_ID_RAID_GROUP) &&
                 object_entry->db_class_id > DATABASE_CLASS_ID_RAID_START && 
                 object_entry->db_class_id < DATABASE_CLASS_ID_RAID_END) ||
               (get_all_objects->class_id == object_entry->db_class_id)) 
            {
                /* Get the encryption state of the object */
                status = fbe_database_forward_packet_to_object(packet, 
                                                               FBE_BASE_CONFIG_CONTROL_CODE_GET_ENCRYPTION_STATE,
                                                               &get_encryption_state,
                                                               sizeof(fbe_base_config_control_get_encryption_state_t),
                                                               object_id,
                                                               NULL,
                                                               0,
                                                               FBE_PACKET_FLAG_DESTROY_ENABLED,
                                                               FBE_PACKAGE_ID_SEP_0);
    
               if ( status != FBE_STATUS_OK){
                   database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                              "%s: failed to get encryption state for object 0x%X\n",__FUNCTION__, object_id);
                   continue;
               }
    
               if(fbe_base_config_is_object_encryption_state_locked(get_encryption_state.encryption_state)) {
                   get_all_objects->number_of_objects++;
               }
            }
        }
    }

    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;

}

/*!***************************************************************
 * @fn fbe_database_control_get_locked_object_info_of_class()
 *****************************************************************
 * @brief
 *   This functions gather the information on objects that are
 *   in the locked state waiting for keys
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    10/23/2013: Ashok Tamilarasan - Created
 *
 ****************************************************************/
static fbe_status_t fbe_database_control_get_locked_object_info_of_class(fbe_packet_t *packet)
{
    fbe_status_t                     		status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_get_locked_info_of_class_header_t *	get_all_objects = NULL;  
    fbe_payload_ex_t *                  	payload = NULL;
    fbe_sg_element_t *                      sg_element = NULL;
    fbe_u32_t                               sg_elements = 0;
    database_object_entry_t *	            object_entry = NULL;
    fbe_object_id_t							object_id;
    fbe_u32_t 								number_of_objects_requested = 0;
    fbe_database_control_get_locked_object_info_t  *get_locked_info;
    fbe_base_config_control_get_encryption_state_t get_encryption_state;
    database_user_entry_t           *out_entry_ptr = NULL;
    database_transaction_t    * transaction;


    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&get_all_objects, sizeof(fbe_database_control_get_locked_info_of_class_header_t));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    payload = fbe_transport_get_payload_ex(packet);
    fbe_payload_ex_get_sg_list(payload, &sg_element, &sg_elements);

    /*point to start of the buffer*/
    get_locked_info = (fbe_database_control_get_locked_object_info_t *)sg_element->address;

    /*sanity check*/
    if ((sg_element->count / sizeof(fbe_database_control_get_locked_object_info_t)) != get_all_objects->number_of_objects){
        database_trace(FBE_TRACE_LEVEL_ERROR,
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s Byte count: 0x%x doesn't agree with num objects: %d.\n",
                       __FUNCTION__, sg_element->count, get_all_objects->number_of_objects);

        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if((get_all_objects->class_id != DATABASE_CLASS_ID_RAID_GROUP) &&
       (get_all_objects->class_id == DATABASE_CLASS_ID_PROVISION_DRIVE)) {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                                            "%s: Unhandled class ID 0x%X\n",__FUNCTION__, get_all_objects->class_id);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    number_of_objects_requested = get_all_objects->number_of_objects;
    get_all_objects->number_of_objects_copied = 0;

    /* Report nothing if we are in the middle of transaction */
    /* It is not safe to do it without proper locking, but may work for now */
    transaction = fbe_database_service.transaction_ptr;
    if(transaction != NULL){
        if(transaction->state != DATABASE_TRANSACTION_STATE_INACTIVE){
            fbe_database_complete_packet(packet, FBE_STATUS_OK);
            return FBE_STATUS_OK;
        }
    }

    /*go over the tables, get objects that are rgs in locked state and fill the information */
    for (object_id = 0; object_id < fbe_database_service.object_table.table_size && number_of_objects_requested > 0; object_id++) {
        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                  object_id,
                                                                  &object_entry);  

        if ( status != FBE_STATUS_OK){
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get object entry for object 0x%X\n",__FUNCTION__, object_id);
            continue;
        }
        if (database_is_entry_valid(&object_entry->header)) 
        {
            if(((get_all_objects->class_id == DATABASE_CLASS_ID_RAID_GROUP) &&
                 object_entry->db_class_id > DATABASE_CLASS_ID_RAID_START && 
                 object_entry->db_class_id < DATABASE_CLASS_ID_RAID_END) ||
               (get_all_objects->class_id == object_entry->db_class_id)) 
            {
                /* Get the encryption state of the object */
                status = fbe_database_forward_packet_to_object(packet, 
                                                               FBE_BASE_CONFIG_CONTROL_CODE_GET_ENCRYPTION_STATE,
                                                               &get_encryption_state,
                                                               sizeof(fbe_base_config_control_get_encryption_state_t),
                                                               object_id,
                                                               NULL,
                                                               0,
                                                               FBE_PACKET_FLAG_DESTROY_ENABLED,
                                                               FBE_PACKAGE_ID_SEP_0);
    
                if ( status != FBE_STATUS_OK){
                    database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: failed to get encryption state for object 0x%X\n",__FUNCTION__, object_id);
                    continue;
                }
    
                if(fbe_base_config_is_object_encryption_state_locked(get_encryption_state.encryption_state) || !(get_all_objects->get_locked_only)) 
                {
                    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,    
                                                                                   object_id,
                                                                                   &out_entry_ptr);
    
                    get_locked_info->object_id = object_id; 
                    get_locked_info->encryption_state = get_encryption_state.encryption_state;

                    if(get_all_objects->class_id == DATABASE_CLASS_ID_RAID_GROUP) {
                        get_locked_info->generation_number = object_entry->set_config_union.rg_config.generation_number;
                        get_locked_info->raid_type = object_entry->set_config_union.rg_config.raid_type;
                        get_locked_info->control_number = out_entry_ptr->user_data_union.rg_user_data.raid_group_number;
                        if (object_entry->set_config_union.rg_config.raid_type == FBE_RAID_GROUP_TYPE_INTERNAL_MIRROR_UNDER_STRIPER)
                        {
                            fbe_database_lookup_raid_for_internal_rg(object_id, &get_locked_info->control_number);
                        }
                        get_locked_info->width = object_entry->set_config_union.rg_config.width;
                    }
                    else if(get_all_objects->class_id == DATABASE_CLASS_ID_PROVISION_DRIVE)
                    {
                        get_locked_info->generation_number = object_entry->set_config_union.pvd_config.generation_number;
                        get_locked_info->control_number = 0; /* For now just set to zero */
                        get_locked_info->width = 1;
                    }
                    else
                    {
                         

                    }
                    get_locked_info++;/*point to next memory address*/
                    get_all_objects->number_of_objects_copied++;
                    number_of_objects_requested--;
                }
            }
        }
    }

    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}



/*!***************************************************************
 * @fn fbe_database_get_user_capacity_limit
 *****************************************************************
 * @brief
 *   This function gets the pvd capacity limit.
 *
 * @param capacity_limit - pointer to capacity limit
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    11/18/2013: Lili Chen - created
 *
 ****************************************************************/
fbe_status_t fbe_database_get_user_capacity_limit(fbe_u32_t *capacity_limit)
{
    fbe_status_t                           status = FBE_STATUS_OK;
    database_global_info_entry_t           *global_info_entry;
    
    
    /* Get the Global Info entry from the config table */		
    status = fbe_database_config_table_get_global_info_entry(&fbe_database_service.global_info, 
                                                             DATABASE_GLOBAL_INFO_TYPE_GLOBAL_PVD_CONFIG, 
                                                             &global_info_entry);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get Global Info entry from the DB service.\n", 
                       __FUNCTION__);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    *capacity_limit = global_info_entry->info_union.pvd_config.user_capacity_limit;
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_set_user_capacity_limit
 *****************************************************************
 * @brief
 *   This function sets the pvd capacity limit.
 *
 * @param capacity_limit - capacity limit
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    11/18/2013: Lili Chen - created
 *
 ****************************************************************/
fbe_status_t fbe_database_set_user_capacity_limit(fbe_u32_t capacity_limit)
{
    fbe_status_t                           status = FBE_STATUS_OK;
    database_global_info_entry_t           new_global_info_entry;
    database_global_info_entry_t           *old_global_info_entry = NULL;
    
    /* Get the Global Info entry from the config table */		
    status = fbe_database_config_table_get_global_info_entry(&fbe_database_service.global_info, 
                                                             DATABASE_GLOBAL_INFO_TYPE_GLOBAL_PVD_CONFIG, 
                                                             &old_global_info_entry);
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get Global Info entry from the DB service.\n", 
                       __FUNCTION__);
        
        return FBE_STATUS_GENERIC_FAILURE;
    }

    if (capacity_limit == old_global_info_entry->info_union.pvd_config.user_capacity_limit)
    {
        database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: capacity limit not changed.\n", 
                       __FUNCTION__);
        
        return FBE_STATUS_OK;
    }

    /* Set capacity limit */
    database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,
                   "%s: setting capacity limit from %d to %d.\n", 
                   __FUNCTION__, old_global_info_entry->info_union.pvd_config.user_capacity_limit, capacity_limit);
    old_global_info_entry->info_union.pvd_config.user_capacity_limit = capacity_limit;

    if (!database_common_cmi_is_active())
    {
        return FBE_STATUS_OK;
    }

    /* Fill up the pvd config info to global info in database table */
    fbe_copy_memory(&new_global_info_entry, old_global_info_entry, sizeof(database_global_info_entry_t));
    new_global_info_entry.header.state = DATABASE_CONFIG_ENTRY_VALID;

    /* Persist global info from table entry */
    status = fbe_database_no_tran_persist_entries((fbe_u8_t *)&new_global_info_entry, 
                                                  1,
                                                  sizeof(database_global_info_entry_t),
                                                  FBE_PERSIST_SECTOR_TYPE_SYSTEM_GLOBAL_DATA);

    if (is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)&new_global_info_entry.info_union.pvd_config,
                                                             FBE_DATABE_CMI_MSG_TYPE_SET_GLOBAL_PVD_CONFIG);
    }

    return status;
}

/*!***************************************************************
 * @fn database_control_get_capacity_limit
 *****************************************************************
 * @brief
 *   Control function to get the pvd capacity limit.
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    11/18/2013: Lili Chen - created
 *
 ****************************************************************/
static fbe_status_t database_control_get_capacity_limit(fbe_packet_t *packet)
{
    fbe_status_t                       status = FBE_STATUS_OK;
    fbe_database_capacity_limit_t      *cap_limit = NULL;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&cap_limit, sizeof(fbe_database_capacity_limit_t));
    if (status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /* Get the Global Info entry from the config table */		
    status = fbe_database_get_user_capacity_limit(&cap_limit->cap_limit);
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get capacity limit from the DB service.\n", 
                       __FUNCTION__);
    }

    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn database_control_set_capacity_limit
 *****************************************************************
 * @brief
 *   Control function to set the pvd capacity limit.
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    11/18/2013: Lili Chen - created
 *
 ****************************************************************/
static fbe_status_t database_control_set_capacity_limit(fbe_packet_t *packet)
{
    fbe_status_t                           status = FBE_STATUS_OK;
    fbe_database_capacity_limit_t          *cap_limit = NULL;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&cap_limit, sizeof(fbe_database_capacity_limit_t));
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Get Payload failed.\n", 
                       __FUNCTION__);

        fbe_database_complete_packet(packet, status);
        return status;
    }

    status = fbe_database_set_user_capacity_limit(cap_limit->cap_limit);
    if (status != FBE_STATUS_OK)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to set capacity limit to the DB service.\n", 
                       __FUNCTION__);
    }

    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_lookup_raid_for_internal_rg()
 *****************************************************************
 * @brief
 *   This function looks up RG number for internal mirror under striper.
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *    12/27/2013: Lili Chen - created
 *
 ****************************************************************/
fbe_status_t fbe_database_lookup_raid_for_internal_rg(fbe_object_id_t object_id, fbe_u32_t *raid_group_number)
{
    fbe_status_t                     status = FBE_STATUS_GENERIC_FAILURE;
    database_user_entry_t           *out_entry_ptr = NULL;  
    fbe_base_config_upstream_object_list_t rg_upstream_object_list;

    /*get the list of objects on top of this RG*/
    status = fbe_database_send_packet_to_object(FBE_BASE_CONFIG_CONTROL_CODE_GET_UPSTREAM_OBJECT_LIST,
                                                &rg_upstream_object_list,
                                                sizeof(fbe_base_config_upstream_object_list_t),
                                                object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK){
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get upstream_list for RG 0x%X\n", __FUNCTION__, object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    if (rg_upstream_object_list.number_of_upstream_objects != 1){
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s: Illegal numer of edges for RG 0x%X\n", __FUNCTION__, object_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,	
                                                                   rg_upstream_object_list.upstream_object_list[0],
                                                                   &out_entry_ptr);
    if ((status == FBE_STATUS_OK)&&(out_entry_ptr != NULL)) {
        /* get the entry ok */
        if (out_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) {
            /* only return the object when the entry is valid */
            *raid_group_number = out_entry_ptr->user_data_union.rg_user_data.raid_group_number;
        }
    }else{
        database_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to find RG %d; returned 0x%x.\n",
            __FUNCTION__, rg_upstream_object_list.upstream_object_list[0], status);
        status = FBE_STATUS_NO_OBJECT;
    }
    return status;
}

/*!***************************************************************
 * @fn fbe_database_control_scrub_old_user_data()
 *****************************************************************
 * @brief
 *   This function goes through all valid and unused PVD, and 
 * scrub the old user data.
 *
 * @param packet - packet
 *
 * @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
 *
 * @version
 *
 ****************************************************************/
fbe_status_t fbe_database_control_scrub_old_user_data(fbe_packet_t * packet)
{
    fbe_status_t 					status = FBE_STATUS_OK;
    fbe_object_id_t                 object_id;
    database_table_t              * object_table_ptr = NULL;
    database_object_entry_t       * object_entry_ptr = NULL;
    fbe_semaphore_t                 sem;
    fbe_packet_t                  * db_packet_p = NULL;

    db_packet_p = fbe_transport_allocate_packet();
    if (db_packet_p == NULL)
    {
        database_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: allocate packet failed.\n",
            __FUNCTION__);
        fbe_database_complete_packet(packet, FBE_STATUS_OK);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_transport_initialize_sep_packet (db_packet_p);

    fbe_semaphore_init(&sem, 0, 1);

    status = fbe_database_get_service_table_ptr(&object_table_ptr, DATABASE_CONFIG_TABLE_OBJECT);

    fbe_spinlock_lock(&object_table_ptr->table_lock);
    for (object_id = 0; object_id < object_table_ptr->table_size; object_id++ ) {
        /* get the current object, user, edge entry pointer */
        object_entry_ptr = &object_table_ptr->table_content.object_entry_ptr[object_id];

        if ((object_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) &&
            (object_entry_ptr->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE) &&
            ((object_entry_ptr->set_config_union.pvd_config.update_type_bitmask & FBE_UPDATE_PVD_AFTER_ENCRYPTION_ENABLED) != FBE_UPDATE_PVD_AFTER_ENCRYPTION_ENABLED))
        {
            fbe_spinlock_unlock(&object_table_ptr->table_lock);
            database_trace(FBE_TRACE_LEVEL_INFO, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: scrubbing object 0x%x.\n",
                            __FUNCTION__, object_id);
            status = fbe_database_send_packet_to_object_async(db_packet_p,
                                                              FBE_PROVISION_DRIVE_CONTROL_CODE_INITIATE_CONSUMED_AREA_ZEROING,
                                                              NULL, /* No payload for this control code*/
                                                              0,  /* No payload for this control code*/
                                                              object_id, 
                                                              NULL,  /* no sg list*/
                                                              0,  /* no sg list*/
                                                              FBE_PACKET_FLAG_NO_ATTRIB,
                                                              fbe_database_control_scrub_old_user_data_completion,
                                                              &sem,
                                                              FBE_PACKAGE_ID_SEP_0);
            fbe_semaphore_wait(&sem, NULL);

            fbe_spinlock_lock(&object_table_ptr->table_lock);
        }
    }

    fbe_spinlock_unlock(&object_table_ptr->table_lock);
    fbe_semaphore_destroy(&sem); 
        
    fbe_transport_release_packet(db_packet_p);

    fbe_database_complete_packet(packet, FBE_STATUS_OK);

    return FBE_STATUS_OK;
}

/*!***************************************************************
 * @fn fbe_database_control_scrub_old_user_data_completion()
 *****************************************************************
 * @brief
 *   This function is the completion for scrubing old user data.
 *
 * @param packet  - packet
 * @param context - context
 *
 * @return fbe_status_t - FBE_STATUS_OK
 *
 * @version
 *
 ****************************************************************/
static fbe_status_t fbe_database_control_scrub_old_user_data_completion(fbe_packet_t * packet, fbe_packet_completion_context_t context)
{
    fbe_semaphore_t * sem;

    sem = (fbe_semaphore_t *)context;
    fbe_semaphore_release(sem, 0, 1, FALSE);
    return FBE_STATUS_OK;
}

static fbe_status_t database_control_set_backup_info(fbe_packet_t *packet)
{
    fbe_status_t                      status = FBE_STATUS_OK;
    fbe_database_backup_info_t      * backup_info = NULL;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&backup_info, sizeof(fbe_database_backup_info_t));
    if (status != FBE_STATUS_OK ) {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    
    fbe_database_service.encryption_backup_state = backup_info->encryption_backup_state; 

    if (database_common_peer_is_alive())
    {
        status = fbe_database_cmi_send_encryption_backup_state_to_peer(backup_info->encryption_backup_state);
    }

    fbe_database_complete_packet(packet, status);

    return status;
}

static fbe_status_t database_control_get_backup_info(fbe_packet_t *packet)
{
    fbe_status_t                      status = FBE_STATUS_OK;
    fbe_database_backup_info_t      * backup_info = NULL;
    fbe_cmi_sp_id_t cmi_sp_id;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&backup_info, sizeof(fbe_database_backup_info_t));
    if (status != FBE_STATUS_OK ) {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    
    backup_info->encryption_backup_state = fbe_database_service.encryption_backup_state; /* KMS will use special api to set this on both SP's */

    /* SP_A = 0x0,  SP_B = 0x1, SP_NA = 0xFD, SP_INVALID = 0xFF */
    fbe_cmi_get_sp_id(&cmi_sp_id);

    if(database_common_cmi_is_active()){
        if(cmi_sp_id == FBE_CMI_SP_ID_A){ /* That means that SPA is Active (primary) */
            backup_info->primary_SP_ID = SP_A;
        } else if(cmi_sp_id == FBE_CMI_SP_ID_B) { /* That means that SPB is Active (primary) */
            backup_info->primary_SP_ID = SP_B;
        } else {
            backup_info->primary_SP_ID = SP_NA;
        }
    } else {/* We are on a Passive side */
        if(cmi_sp_id == FBE_CMI_SP_ID_A){ /* That means that SPA is Passive and SPB is (primary) */
            backup_info->primary_SP_ID = SP_B;
        } else if(cmi_sp_id == FBE_CMI_SP_ID_B) { /* That means that SPB is Passive and SPA (primary) */
            backup_info->primary_SP_ID = SP_A;
        } else {
            backup_info->primary_SP_ID = SP_NA;
        }
    }

    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_database_control_get_be_port_info()
 ******************************************************************************
 * @brief
 *  This control entry function to setup the key encryption keys for the objects
 * 
 * @param  packet            - Pointer to the packet 
 * 
 * @return status               - status of the operation.
 *
 * @author
 *  10/05/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_database_control_get_be_port_info(fbe_packet_t * packet)
{
    fbe_payload_ex_t *                  payload;
    fbe_payload_control_operation_t*    control_operation;
    fbe_database_control_get_be_port_info_t *be_port_info;
    fbe_status_t                        status;
    fbe_cmi_sp_id_t                     my_sp_id;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&be_port_info, sizeof(fbe_database_control_get_be_port_info_t));

    if (status != FBE_STATUS_OK)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: unable to get payload memory for operation\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_cmi_get_sp_id(&my_sp_id);

    if(be_port_info->sp_id != my_sp_id) {
        /* This command is for the peer and so send it to the peer*/
        status = fbe_database_cmi_send_get_be_port_info_to_peer(&fbe_database_service,
                                                                be_port_info);
    }
    else
    {
        status = fbe_database_get_be_port_info(be_port_info);
    }

    if (status != FBE_STATUS_OK)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return status;

}
/*!****************************************************************************
 * fbe_database_control_get_be_port_info()
 ******************************************************************************
 * @brief
 *  This control entry function to setup the key encryption keys for the objects
 * 
 * @param  packet            - Pointer to the packet 
 * 
 * @return status               - status of the operation.
 *
 * @author
 *  10/05/2013 - Created. Ashok Tamilarasan
 *
 ******************************************************************************/
fbe_status_t fbe_database_get_be_port_info(fbe_database_control_get_be_port_info_t *be_port_info)
{
    fbe_object_id_t                     port_object_id[100];
    fbe_u32_t                           num_actual_port;
    fbe_u32_t                           i;
    fbe_port_info_t                     base_port_info;
    fbe_status_t                        status;
    fbe_cmi_sp_id_t                     my_sp_id;
    fbe_port_hardware_info_t            hw_port_info;
    fbe_base_board_get_port_serial_num_t serial_num_info;
    fbe_topology_control_get_board_t    board;
    fbe_u32_t index = 0;

    fbe_cmi_get_sp_id(&my_sp_id);
    if(be_port_info->sp_id != my_sp_id) 
    {
        /* At this point, the desitnation should be this SP */
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: Incorrect SP destination. Expected:%d actual:%d\n", 
                        __FUNCTION__, my_sp_id, be_port_info->sp_id);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    /* First get the list of all the port objects */
    status = fbe_database_get_list_be_port_object_id(&port_object_id[0], &num_actual_port, 100);

    if (status != FBE_STATUS_OK) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: unable to get port object IDs\n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

     /* First get the object id of the board */
    status = fbe_database_send_packet_to_service(FBE_TOPOLOGY_CONTROL_CODE_GET_BOARD,
                                                 &board,
                                                 sizeof(board),
                                                 FBE_SERVICE_ID_TOPOLOGY,
                                                 NULL,
                                                 0,
                                                 FBE_PACKET_FLAG_NO_ATTRIB,
                                                 FBE_PACKAGE_ID_PHYSICAL);

    if (status != FBE_STATUS_OK) {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: unable to get board object ID \n", __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    be_port_info->num_be_ports = 0;

    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "Total Number of Ports %d\n", num_actual_port);

    /* Get info about all the ports */
    for(i = 0; i < num_actual_port; i++)
    {

        database_trace (FBE_TRACE_LEVEL_INFO,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "Get port info 0x%08x\n", port_object_id[i]);

        status = fbe_database_send_packet_to_object(FBE_BASE_PORT_CONTROL_CODE_GET_PORT_INFO,
                                                    &base_port_info,
                                                    sizeof(fbe_port_info_t),
                                                    port_object_id[i],
                                                    NULL,  /* no sg list*/
                                                    0,  /* no sg list*/
                                                    FBE_PACKET_FLAG_EXTERNAL,
                                                    FBE_PACKAGE_ID_PHYSICAL);
        if (status != FBE_STATUS_OK) {
            database_trace (FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "%s: unable to get port info from object 0x%x, Index:%d \n", __FUNCTION__, port_object_id[i], i);
            return FBE_STATUS_GENERIC_FAILURE;
        }

        if(base_port_info.port_link_info.port_connect_class != FBE_PORT_CONNECT_CLASS_SAS) {
            /* We want just SAS ports */
            continue;
        }
        else {
            if(be_port_info->num_be_ports >= FBE_MAX_BE_PORTS_PER_SP) {
                database_trace (FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: Number of ports buffer too small. Expected:%d, actual:%d\n", 
                                __FUNCTION__,
                                FBE_MAX_BE_PORTS_PER_SP, num_actual_port);

                /* We dont have enough buffer and so just return with whatever we have */
                return FBE_STATUS_OK;
            }

            status = fbe_database_send_packet_to_object(FBE_BASE_PORT_CONTROL_CODE_GET_HARDWARE_INFORMATION,
                                                            &hw_port_info,
                                                            sizeof(fbe_port_hardware_info_t),
                                                            port_object_id[i],
                                                            NULL,  /* no sg list*/
                                                            0,  /* no sg list*/
                                                            FBE_PACKET_FLAG_EXTERNAL,
                                                            FBE_PACKAGE_ID_PHYSICAL);
            if (status != FBE_STATUS_OK){
                database_trace (FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: unable to HW info from object 0x%x, Index:%d \n", __FUNCTION__, port_object_id[i], i);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            serial_num_info.pci_bus = hw_port_info.pci_bus;
            serial_num_info.pci_function = hw_port_info.pci_function;
            serial_num_info.pci_slot = hw_port_info.pci_slot;
            serial_num_info.phy_map = base_port_info.port_link_info.u.sas_port.phy_map;

            status = fbe_database_send_packet_to_object(FBE_BASE_BOARD_CONTROL_CODE_GET_PORT_SERIAL_NUM,
                                                        &serial_num_info,
                                                        sizeof(fbe_base_board_get_port_serial_num_t),
                                                        board.board_object_id,
                                                        NULL,  /* no sg list*/
                                                        0,  /* no sg list*/
                                                        FBE_PACKET_FLAG_EXTERNAL,
                                                        FBE_PACKAGE_ID_PHYSICAL);
            if (status != FBE_STATUS_OK){
                database_trace (FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s: unable to get serial Number from object 0x%x, Index:%d \n", __FUNCTION__, port_object_id[i], i);
                return FBE_STATUS_GENERIC_FAILURE;
            }

            be_port_info->port_info[index].port_object_id = port_object_id[i];
            be_port_info->port_info[index].be_number = base_port_info.assigned_bus_number;
            fbe_copy_memory(be_port_info->port_info[index].serial_number, serial_num_info.serial_num, FBE_PORT_SERIAL_NUM_SIZE);
            if (base_port_info.assigned_bus_number == FBE_INVALID_PORT_NUM){
                be_port_info->port_info[index].port_encrypt_mode = FBE_PORT_ENCRYPTION_MODE_UNCOMMITTED;
            }
            else {
                be_port_info->port_info[index].port_encrypt_mode = base_port_info.enc_mode;
            }
        }
        be_port_info->num_be_ports++;
        index++;
    }

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * database_control_set_update_peer_db()
 ******************************************************************************
 * @brief
 *  This control entry function to continue update peer db table
 * 
 * @param  packet            - Pointer to the packet 
 * 
 * @return status               - status of the operation.
 *
 * @author
 *
 ******************************************************************************/
static fbe_status_t database_control_set_update_peer_db(fbe_packet_t *packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_database_control_db_update_peer_t * update_peer = NULL;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&update_peer, sizeof(fbe_database_control_db_update_peer_t));
    if (status != FBE_STATUS_OK ) {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    
    if (update_peer->update_op == DATABASE_CONTROL_DB_UPDATE_PROCEED)
    {
        set_database_service_state(&fbe_database_service, FBE_DATABASE_STATE_KMS_APPROVED);
    }
    else
    {
        database_trace (FBE_TRACE_LEVEL_WARNING,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s:update peer db stopped, reason %d\n", __FUNCTION__, update_peer->update_op);

        /* peer will stuck waiting for tables until database timeout and panic,
         * eventually, it will be in degraded mode.
         * we may want to handle this more graceful.
         */
        set_database_service_state(&fbe_database_service, FBE_DATABASE_STATE_READY);

    }

    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_database_get_drive_sn()
 ******************************************************************************
 * @brief
 *  This control entry function is used to get drive serial numbers for a RG.
 * 
 * @param  packet            - Pointer to the packet 
 * 
 * @return status               - status of the operation.
 *
 * @author
 *  01/24/2014 - Created. Lili Chen
 *
 ******************************************************************************/
static fbe_status_t fbe_database_get_drive_sn(fbe_database_control_get_drive_sn_t *drive_sn, fbe_packet_t * packet)
{
    fbe_status_t                        status;
    database_object_entry_t            *object_entry = NULL;
    fbe_u32_t                           i;
    fbe_object_id_t                     pvd_list[FBE_RAID_MAX_DISK_ARRAY_WIDTH+1];
    fbe_base_config_downstream_object_list_t rg_downstream_list;

    /* Initialize */
    for (i = 0; i < (FBE_RAID_MAX_DISK_ARRAY_WIDTH + 1); i++)
    {
        pvd_list[i] = FBE_OBJECT_ID_INVALID;
    }

    /* Get downstream objects */
    status = fbe_database_forward_packet_to_object(packet,
                                                   FBE_BASE_CONFIG_CONTROL_CODE_GET_DOWNSTREAM_OBJECT_LIST,
                                                   &rg_downstream_list,
                                                   sizeof(rg_downstream_list),
                                                   drive_sn->rg_id,
                                                   NULL,
                                                   0,
                                                   FBE_PACKET_FLAG_NO_ATTRIB,
                                                   FBE_PACKAGE_ID_SEP_0);
    if ( status != FBE_STATUS_OK )
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get downstream list for object 0x%x\n", 
                       __FUNCTION__,
                       drive_sn->rg_id);
        return status;
    }

    for (i = 0; i < rg_downstream_list.number_of_downstream_objects; i++) 
    {
        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table, 
                                                                  rg_downstream_list.downstream_object_list[i],
                                                                  &object_entry);
        if (status != FBE_STATUS_OK) 
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get user entry for object 0x%x\n", 
                       __FUNCTION__,
                       rg_downstream_list.downstream_object_list[i]);
            return status;
        }

        if(object_entry->db_class_id > DATABASE_CLASS_ID_RAID_START && object_entry->db_class_id < DATABASE_CLASS_ID_RAID_END)
        {
            database_trace(FBE_TRACE_LEVEL_WARNING, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: this RG is the striper of R10 0x%x\n", 
                       __FUNCTION__,
                       rg_downstream_list.downstream_object_list[i]);
            return FBE_STATUS_GENERIC_FAILURE;
        }
        else if (object_entry->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE)
        {
            pvd_list[i] = rg_downstream_list.downstream_object_list[i];
        }
        else if (object_entry->db_class_id == DATABASE_CLASS_ID_VIRTUAL_DRIVE) 
        {
            status = fbe_database_get_all_pvd_for_vd(rg_downstream_list.downstream_object_list[i],
                                                     &pvd_list[i], 
                                                     &pvd_list[FBE_RAID_MAX_DISK_ARRAY_WIDTH], 
                                                     packet);
            if ( status != FBE_STATUS_OK )
            {
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: failed to get pvd list for object 0x%x\n", 
                               __FUNCTION__,
                               rg_downstream_list.downstream_object_list[i]);
                return status;
            }
        }
    }

    for (i = 0; i < (FBE_RAID_MAX_DISK_ARRAY_WIDTH + 1); i++)
    {
        fbe_zero_memory(drive_sn->serial_number[i], FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE + 1);
        if (pvd_list[i] != FBE_OBJECT_ID_INVALID) 
        {
            status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                      pvd_list[i],
                                                                      &object_entry);  
            if ( status != FBE_STATUS_OK){
                database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                               "%s: failed to get object entry for object 0x%X\n",__FUNCTION__, pvd_list[i]);
                continue;
            }

            fbe_copy_memory(drive_sn->serial_number[i], object_entry->set_config_union.pvd_config.serial_num, FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);
        }
    }

    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_database_control_get_drive_sn_for_raid()
 ******************************************************************************
 * @brief
 *  This control entry function is used to get drive serial numbers for a RG.
 * 
 * @param  packet            - Pointer to the packet 
 * 
 * @return status               - status of the operation.
 *
 * @author
 *  01/24/2014 - Created. Lili Chen
 *
 ******************************************************************************/
fbe_status_t fbe_database_control_get_drive_sn_for_raid(fbe_packet_t * packet)
{
    fbe_status_t                        status;
    fbe_payload_ex_t *                  payload;
    fbe_payload_control_operation_t*    control_operation;
    fbe_database_control_get_drive_sn_t *drive_sn;

    payload = fbe_transport_get_payload_ex(packet);
    control_operation = fbe_payload_ex_get_control_operation(payload);

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&drive_sn, sizeof(fbe_database_control_get_drive_sn_t));
    if (status != FBE_STATUS_OK)
    {
        database_trace (FBE_TRACE_LEVEL_ERROR,
                        FBE_TRACE_MESSAGE_ID_INFO,
                        "%s: unable to get payload memory for operation\n", __FUNCTION__);
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
        fbe_transport_set_status(packet, status, 0);
        fbe_transport_complete_packet(packet);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = fbe_database_get_drive_sn(drive_sn, packet);

    if (status != FBE_STATUS_OK)
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_FAILURE);
    }
    else
    {
        fbe_payload_control_set_status(control_operation, FBE_PAYLOAD_CONTROL_STATUS_OK);
    }
    fbe_transport_set_status(packet, FBE_STATUS_OK, 0);
    fbe_transport_complete_packet(packet);
    return status;

}

fbe_status_t fbe_database_get_object_encryption_mode(fbe_object_id_t object_id,
                                                     fbe_base_config_encryption_mode_t *encryption_mode_p)
{
    fbe_status_t status;
    database_object_entry_t *entry_p = NULL;
    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              object_id,
                                                              &entry_p);
    if (status != FBE_STATUS_OK || entry_p == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: get destination object entry failed!\n", 
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }
    *encryption_mode_p = entry_p->base_config.encryption_mode;
    return status;
}


/*!****************************************************************************
 * fbe_database_control_set_extended_cache_enabled()
 ******************************************************************************
 * @brief
 *  Set the global variable that is used to determine whether or not to shoot drives
 * 
 * @param  packet            - Pointer to the packet 
 * 
 * @return status               - status of the operation.
 *
 * @author
 *
 ******************************************************************************/
static fbe_status_t fbe_database_control_set_extended_cache_enabled(fbe_packet_t *packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_database_control_bool_t             *drive_tier_enabled;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&drive_tier_enabled, sizeof(fbe_database_control_bool_t));
    if (status != FBE_STATUS_OK ) {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    fbe_database_vault_drive_tier_enabled = drive_tier_enabled->is_enabled;
    
    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s:set extended cache enabled to %d\n", __FUNCTION__, fbe_database_vault_drive_tier_enabled);

    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 * fbe_database_get_extended_cache_enabled()
 ******************************************************************************
 * @brief
 *  Get the global on whether extended cache is enabled
 
 * @return bool
 
 * @author
 *
 ******************************************************************************/
fbe_bool_t fbe_database_get_extended_cache_enabled(void) 
{
    return fbe_database_vault_drive_tier_enabled;
}


/*!****************************************************************************
 * fbe_database_control_set_garbage_collection_debouncer()
 ******************************************************************************
 * @brief
 *  Sets the forced garbage collection debouncer time.
 
 * @return bool
 
 * @author
 *
 ******************************************************************************/
static fbe_status_t fbe_database_control_set_garbage_collection_debouncer(fbe_packet_t *packet)
{
    fbe_status_t    status = FBE_STATUS_OK;
    fbe_u32_t       *timeout_ms;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&timeout_ms, sizeof(fbe_u32_t));
    if (status != FBE_STATUS_OK ) {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    database_trace (FBE_TRACE_LEVEL_INFO,
                    FBE_TRACE_MESSAGE_ID_INFO,
                    "%s:changing garbage collect debouncer %d->%d ms\n", 
                    __FUNCTION__, fbe_database_service.forced_garbage_collection_debounce_timeout, *timeout_ms);

    fbe_database_service.forced_garbage_collection_debounce_timeout = *timeout_ms;


    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;    
}

/*!***************************************************************
* @fn fbe_database_get_pvd_drive_type
*****************************************************************
* @brief
* This function gets pvd drive type from PVD.
*
* @param object_id - Object id.
* @param packet - Pointer to the packet.
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
fbe_status_t fbe_database_get_pvd_drive_type(fbe_object_id_t object_id, fbe_drive_type_t * drive_type, fbe_packet_t *packet)
{
    fbe_status_t                                    status = FBE_STATUS_GENERIC_FAILURE;
    fbe_provision_drive_info_t  pvd_info;

    status = fbe_database_forward_packet_to_object(packet,
                                         FBE_PROVISION_DRIVE_CONTROL_CODE_GET_PROVISION_DRIVE_INFO,
                                         &pvd_info,
                                         sizeof(pvd_info),
                                         object_id,
                                         NULL,  /* no sg list*/
                                         0,  /* no sg list*/
                                         FBE_PACKET_FLAG_NO_ATTRIB,
                                         FBE_PACKAGE_ID_SEP_0);
    if(FBE_STATUS_OK != status)
    {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "%s: failed to get information of PVD 0x%x.\n", 
                                        __FUNCTION__, object_id);
        return status;
    }
   
    *drive_type = pvd_info.drive_type;

    return status;
}

/*!***************************************************************
* @fn fbe_database_control_get_pvd_list_for_ext_pool
*****************************************************************
* @brief
* This function gets pvd list when creating EXTENT POOL object.
*
* @param packet - Pointer to the packet.
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
static fbe_status_t 
fbe_database_control_get_pvd_list_for_ext_pool(fbe_packet_t *packet)
{
    fbe_status_t                     		status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_get_pvd_list_for_ext_pool_t *	get_pvd_list = NULL;  
    database_object_entry_t *               object_entry = NULL;
    fbe_object_id_t	                        object_id;
    fbe_u32_t 								number_of_objects_requested = 0;
    fbe_object_id_t                        * pvd_id;
    fbe_lifecycle_state_t                   lifecycle_state;
    fbe_drive_type_t                        drive_type;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&get_pvd_list, sizeof(fbe_database_control_get_pvd_list_for_ext_pool_t));
    if (status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    pvd_id = get_pvd_list->pvd_list;
    number_of_objects_requested = get_pvd_list->drive_count;

    /*go over the tables, get objects that are rgs in locked state and fill the information */
    for (object_id = 0; object_id < fbe_database_service.object_table.table_size && number_of_objects_requested > 0; object_id++) {
        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                  object_id,
                                                                  &object_entry);  

        if (status != FBE_STATUS_OK){
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                           "%s: failed to get object entry for object 0x%X\n",__FUNCTION__, object_id);
            continue;
        }
        if (database_is_entry_valid(&object_entry->header)) 
        {
            if (object_entry->db_class_id == DATABASE_CLASS_ID_PROVISION_DRIVE && 
                !fbe_private_space_layout_object_id_is_system_pvd(object_id) &&
                object_entry->set_config_union.pvd_config.config_type == FBE_PROVISION_DRIVE_CONFIG_TYPE_UNCONSUMED) 
            {
                status = fbe_database_get_object_lifecycle_state(object_id, &lifecycle_state, packet);
                if (status != FBE_STATUS_OK || lifecycle_state != FBE_LIFECYCLE_STATE_READY) {
                    continue;
                }

                status = fbe_database_get_pvd_drive_type(object_id, &drive_type, packet);
                if ((status == FBE_STATUS_OK) && 
                    ((get_pvd_list->drive_type == FBE_DRIVE_TYPE_INVALID) || (drive_type == get_pvd_list->drive_type))) {
                    *pvd_id = object_id;
                    pvd_id++;
                    number_of_objects_requested--;
                }
            }
        }
    }

    if (number_of_objects_requested) {
        status = FBE_STATUS_GENERIC_FAILURE;
    }

    fbe_database_complete_packet(packet, status);
    return FBE_STATUS_OK;
}

/*!***************************************************************
* @fn fbe_database_control_create_extent_pool
*****************************************************************
* @brief
* This function process usurper request to create EXTENT POOL object.
*
* @param packet - Pointer to the packet.
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
static fbe_status_t fbe_database_control_create_extent_pool(fbe_packet_t * packet)
{
    fbe_database_control_create_ext_pool_t      *create_ext_pool = NULL;
    fbe_database_transaction_id_t     current_id = FBE_DATABASE_TRANSACTION_ID_INVALID;
    fbe_status_t                      status;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&create_ext_pool, sizeof(fbe_database_control_create_ext_pool_t));
    if (status != FBE_STATUS_OK)
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &current_id);
    /* check if the transaction id is valid */
    if (create_ext_pool->transaction_id != current_id){
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Invalid id 0x%llx.  Current id is 0x%llx\n",
            __FUNCTION__, (unsigned long long)create_ext_pool->transaction_id,
        (unsigned long long)current_id);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = database_create_extent_pool(create_ext_pool);
    if (status == FBE_STATUS_OK && is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)create_ext_pool,
                                                             FBE_DATABASE_CMI_MSG_TYPE_CREATE_EXTENT_POOL);

    }

    fbe_database_complete_packet(packet, status);
    return status;
}

/*!***************************************************************
* @fn database_create_extent_pool
*****************************************************************
* @brief
* This function creates EXTENT POOL object.
*
* @param create_ext_pool - Pointer to create information.
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
fbe_status_t database_create_extent_pool(fbe_database_control_create_ext_pool_t * create_ext_pool)
{
    database_object_entry_t           object_entry;
    database_user_entry_t             user_entry;
    fbe_status_t                      status;
    fbe_u32_t                         size = 0;

    /*update transaction user and object tables */
    database_common_init_object_entry(&object_entry);
    object_entry.header.object_id = create_ext_pool->object_id;
    object_entry.header.state = DATABASE_CONFIG_ENTRY_CREATE;
    object_entry.db_class_id = database_common_map_class_id_fbe_to_database(create_ext_pool->class_id);
    /* Initialize the size in version header*/
    status = database_common_get_committed_object_entry_size(object_entry.db_class_id, &size);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: get committed object entry size failed, size = %d \n",
                       __FUNCTION__, size);
        return status;
    }
    object_entry.header.version_header.size = size;
    if (database_common_cmi_is_active()){  /* set the generation number on the active side */
        create_ext_pool->generation_number = 
            fbe_database_transaction_get_next_generation_id(&fbe_database_service);
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Active side: Assigned generation number: 0x%llx for RG.\n",
                       __FUNCTION__, 
                       (unsigned long long)create_ext_pool->generation_number);
    }

    object_entry.set_config_union.ext_pool_config.generation_number = create_ext_pool->generation_number;
    object_entry.set_config_union.ext_pool_config.width = create_ext_pool->drive_count;

    /* object id is assigned by the following function, so must call before add it to transaction table */
    status = database_common_create_object_from_table_entry(&object_entry);
    create_ext_pool->object_id = object_entry.header.object_id;  /* now set the object_id that is returned to caller */
    /*Initialize RG configuration*/
    status = database_common_set_config_from_table_entry(&object_entry);

#if 0 /* No encryption yet */
    status = fbe_database_get_system_encryption_mode(&system_encryption_mode);
    /* Temporary hack */
    base_config_encryption_mode = system_encryption_mode;

    object_entry.base_config.encryption_mode= base_config_encryption_mode;

    status = database_common_update_encryption_mode_from_table_entry(&object_entry);
#endif

    status = fbe_database_transaction_add_object_entry(fbe_database_service.transaction_ptr, 
        &object_entry);

    database_common_init_user_entry(&user_entry);
    user_entry.header.object_id = create_ext_pool->object_id;
    user_entry.header.state = DATABASE_CONFIG_ENTRY_CREATE;
    user_entry.db_class_id = database_common_map_class_id_fbe_to_database(create_ext_pool->class_id);
    /* Initialize the size in version header*/
    status = database_common_get_committed_user_entry_size(user_entry.db_class_id, &size);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: get committed user entry size failed, size = %d \n",
                       __FUNCTION__, size);
        return status;
    }
    user_entry.header.version_header.size = size;
    user_entry.user_data_union.ext_pool_user_data.pool_id = create_ext_pool->pool_id;
    status = fbe_database_transaction_add_user_entry(fbe_database_service.transaction_ptr, 
        &user_entry);

#if 0
    /* Log message to the event-log when DB has already added a RG to the Table, and not logging it at the RG
     * create commit time.  We cannot write to the Event Log in CMI thread.
     */
    status = fbe_event_log_write(SEP_INFO_RAID_GROUP_CREATED, NULL, 0, FBE_WIDE_CHAR("%x %d"),
                    create_raid->object_id, 
                    create_raid->raid_id); 
#endif

    return status;
}

/*!***************************************************************
* @fn fbe_database_get_ext_pool_configuration_info
*****************************************************************
* @brief
* This function gets the EXTENT POOL object config information.
*
* @param pool_object_id - object id of the extent pool.
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
fbe_status_t 
fbe_database_get_ext_pool_configuration_info(fbe_object_id_t pool_object_id, 
                                             fbe_u32_t * pool_id, 
                                             fbe_u32_t * drive_count, 
                                             fbe_object_id_t * pvd_list)
{
    fbe_status_t                     		status = FBE_STATUS_GENERIC_FAILURE;
    database_object_entry_t *	            object_entry = NULL;
    database_user_entry_t *                 user_entry = NULL;
    fbe_object_id_t	                        object_id;
    fbe_u32_t                               number_of_objects_requested = 0;
    fbe_u32_t                               index;

    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              pool_object_id,
                                                              &object_entry);  
    if (status != FBE_STATUS_OK || !database_is_entry_valid(&object_entry->header)){
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get object entry for object 0x%X\n",__FUNCTION__, pool_object_id);
        return FBE_STATUS_NO_OBJECT;
    }
    *drive_count = object_entry->set_config_union.ext_pool_config.width;

    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                                   pool_object_id,
                                                                   &user_entry);
    if (status != FBE_STATUS_OK || user_entry == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: get user entry failed!\n", 
                       __FUNCTION__);
        return FBE_STATUS_NO_OBJECT;
    }
    *pool_id = user_entry->user_data_union.ext_pool_user_data.pool_id;

    number_of_objects_requested = *drive_count;

    if (pvd_list != NULL) {
        /*go over the tables, get objects that are rgs in locked state and fill the information */
        for (object_id = 0; object_id < fbe_database_service.object_table.table_size && number_of_objects_requested > 0; object_id++) {
            status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                      object_id,
                                                                      &object_entry);  
    
            if ( status != FBE_STATUS_OK || !database_is_entry_valid(&object_entry->header)){
                continue;
            }
            if (object_entry->db_class_id != DATABASE_CLASS_ID_PROVISION_DRIVE ||
                object_entry->set_config_union.pvd_config.config_type != FBE_PROVISION_DRIVE_CONFIG_TYPE_EXT_POOL) 
            {
                continue;
            }
    
            status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                                           object_id,
                                                                           &user_entry);
            if (status != FBE_STATUS_OK || user_entry == NULL) {
                continue;
            }
            if (*pool_id == (user_entry->user_data_union.pvd_user_data.pool_id & 0x0000ffff)){
                index = (user_entry->user_data_union.pvd_user_data.pool_id >> 16) & 0x0000ffff;
                pvd_list[index] = object_id;
                number_of_objects_requested--;
            }
        }
        if (number_of_objects_requested) {
            status = FBE_STATUS_GENERIC_FAILURE;
        }
    }
    return status;
}
/*!***************************************************************
* @fn fbe_database_get_pvd_capacity
*****************************************************************
* @brief
* This function gets a pvd's capacity
*
* @param object_id - object id of the PVD
* @param capacity_p - capacity of pvd output.
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
fbe_status_t 
fbe_database_get_pvd_capacity(fbe_object_id_t object_id,
                              fbe_block_count_t *capacity_p)
{
    fbe_status_t                     		status = FBE_STATUS_GENERIC_FAILURE;
    database_object_entry_t *	            object_entry = NULL;

    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              object_id,
                                                              &object_entry);  
    if (status != FBE_STATUS_OK || !database_is_entry_valid(&object_entry->header)){
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get object entry for object 0x%X\n",__FUNCTION__, object_id);
        return FBE_STATUS_NO_OBJECT;
    }
    *capacity_p = object_entry->set_config_union.pvd_config.configured_capacity;

    return status;
}

/*!***************************************************************
* @fn fbe_database_control_create_ext_pool_lun
*****************************************************************
* @brief
* This function process usurper request to create EXTENT POOL LUN object.
*
* @param packet - Pointer to the packet.
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
static fbe_status_t fbe_database_control_create_ext_pool_lun(fbe_packet_t * packet)
{
    fbe_database_control_create_ext_pool_lun_t      *create_ext_pool_lun = NULL;
    fbe_database_transaction_id_t     current_id = FBE_DATABASE_TRANSACTION_ID_INVALID;
    fbe_status_t                      status;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&create_ext_pool_lun, sizeof(fbe_database_control_create_ext_pool_lun_t));
    if (status != FBE_STATUS_OK)
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &current_id);
    /* check if the transaction id is valid */
    if (create_ext_pool_lun->transaction_id != current_id){
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Invalid id 0x%llx.  Current id is 0x%llx\n",
            __FUNCTION__, (unsigned long long)create_ext_pool_lun->transaction_id,
        (unsigned long long)current_id);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    status = database_create_ext_pool_lun(create_ext_pool_lun);
    if (status == FBE_STATUS_OK && is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)create_ext_pool_lun,
                                                             FBE_DATABASE_CMI_MSG_TYPE_CREATE_EXTENT_POOL_LUN);

    }

    fbe_database_complete_packet(packet, status);
    return status;
}

/*!***************************************************************
* @fn database_create_ext_pool_lun
*****************************************************************
* @brief
* This function creates EXTENT POOL LUN object.
*
* @param create_ext_pool - Pointer to create information.
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
fbe_status_t database_create_ext_pool_lun(fbe_database_control_create_ext_pool_lun_t * create_ext_pool_lun)
{
    database_object_entry_t           object_entry;
    database_user_entry_t             user_entry;
    fbe_status_t                      status;
    fbe_u32_t                         size = 0;

    /*update transaction user and object tables */
    database_common_init_object_entry(&object_entry);
    object_entry.header.object_id = create_ext_pool_lun->object_id;
    object_entry.header.state = DATABASE_CONFIG_ENTRY_CREATE;
    object_entry.db_class_id = database_common_map_class_id_fbe_to_database(create_ext_pool_lun->class_id);
    /* Initialize the size in version header*/
    status = database_common_get_committed_object_entry_size(object_entry.db_class_id, &size);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: get committed object entry size failed, size = %d \n",
                       __FUNCTION__, size);
        return status;
    }
    object_entry.header.version_header.size = size;
    if (database_common_cmi_is_active()){  /* set the generation number on the active side */
        create_ext_pool_lun->generation_number = 
            fbe_database_transaction_get_next_generation_id(&fbe_database_service);
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: Active side: Assigned generation number: 0x%llx for RG.\n",
                       __FUNCTION__, 
                       (unsigned long long)create_ext_pool_lun->generation_number);
    }

    object_entry.set_config_union.ext_pool_lun_config.generation_number = create_ext_pool_lun->generation_number;
    object_entry.set_config_union.ext_pool_lun_config.capacity = create_ext_pool_lun->capacity;
    if (create_ext_pool_lun->class_id == FBE_CLASS_ID_EXTENT_POOL_METADATA_LUN) {
        object_entry.set_config_union.ext_pool_lun_config.server_index = 0;
        object_entry.set_config_union.ext_pool_lun_config.offset = 0;
    } else {
        fbe_database_get_next_edge_index_for_extent_pool_lun(create_ext_pool_lun->pool_id, 
            &object_entry.set_config_union.ext_pool_lun_config.server_index,
            &object_entry.set_config_union.ext_pool_lun_config.offset);
    }

    /*the upper layers has the option not to assigne the LUN number so we have to do that
    We do it on the active side only becase by the time it gets to the passive side, we would
    have already populated the number*/
    if (database_common_cmi_is_active() && create_ext_pool_lun->lun_id == FBE_LUN_ID_INVALID) {
        status = database_get_next_available_lun_number(&create_ext_pool_lun->lun_id);
        if (status != FBE_STATUS_OK) {
            database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,"%s: can't find a free lun number\n",__FUNCTION__);
            return status;

        }else{
            database_trace(FBE_TRACE_LEVEL_INFO, FBE_TRACE_MESSAGE_ID_INFO,"%s: picked up lun number %d for user\n",__FUNCTION__, create_ext_pool_lun->lun_id);

        }
    }

    /* object id is assigned by the following function, so must call before add it to transaction table */
    status = database_common_create_object_from_table_entry(&object_entry);
    create_ext_pool_lun->object_id = object_entry.header.object_id;  /* now set the object_id that is returned to caller */
    /*Initialize RG configuration*/
    status = database_common_set_config_from_table_entry(&object_entry);

#if 0 /* No encryption yet */
    status = fbe_database_get_system_encryption_mode(&system_encryption_mode);
    /* Temporary hack */
    base_config_encryption_mode = system_encryption_mode;

    object_entry.base_config.encryption_mode= base_config_encryption_mode;

    status = database_common_update_encryption_mode_from_table_entry(&object_entry);
#endif

    status = fbe_database_transaction_add_object_entry(fbe_database_service.transaction_ptr, 
        &object_entry);

    database_trace(FBE_TRACE_LEVEL_INFO, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "Create ext pool lun: pool id 0x%x lun id 0x%x index %d offset 0x%llx.\n",
                   create_ext_pool_lun->pool_id, create_ext_pool_lun->lun_id, 
                   object_entry.set_config_union.ext_pool_lun_config.server_index, object_entry.set_config_union.ext_pool_lun_config.offset);

    database_common_init_user_entry(&user_entry);
    user_entry.header.object_id = create_ext_pool_lun->object_id;
    user_entry.header.state = DATABASE_CONFIG_ENTRY_CREATE;
    user_entry.db_class_id = database_common_map_class_id_fbe_to_database(create_ext_pool_lun->class_id);
    /* Initialize the size in version header*/
    status = database_common_get_committed_user_entry_size(user_entry.db_class_id, &size);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: get committed user entry size failed, size = %d \n",
                       __FUNCTION__, size);
        return status;
    }
    user_entry.header.version_header.size = size;
    user_entry.user_data_union.ext_pool_lun_user_data.pool_id = create_ext_pool_lun->pool_id;
    user_entry.user_data_union.ext_pool_lun_user_data.lun_id = create_ext_pool_lun->lun_id;
    user_entry.user_data_union.ext_pool_lun_user_data.world_wide_name = create_ext_pool_lun->world_wide_name;
    user_entry.user_data_union.ext_pool_lun_user_data.user_defined_name = create_ext_pool_lun->user_defined_name;
    status = fbe_database_transaction_add_user_entry(fbe_database_service.transaction_ptr, 
        &user_entry);

#if 0
    /* Log message to the event-log when DB has already added a RG to the Table, and not logging it at the RG
     * create commit time.  We cannot write to the Event Log in CMI thread.
     */
    status = fbe_event_log_write(SEP_INFO_RAID_GROUP_CREATED, NULL, 0, FBE_WIDE_CHAR("%x %d"),
                    create_raid->object_id, 
                    create_raid->raid_id); 
#endif

    return status;
}


/*!****************************************************************************
 *          fbe_database_control_mark_pvd_swap_pending()
 ******************************************************************************
 * @brief
 *  Mark the provision drive `swap pending'.
 * 
 * @param   packet_p - packet with database request
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  06/05/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_control_mark_pvd_swap_pending(fbe_packet_t *packet)
{
    fbe_status_t                                    status = FBE_STATUS_OK;
    fbe_database_control_mark_pvd_swap_pending_t   *mark_pvd_swap_pending_p = NULL;
    fbe_provision_drive_set_swap_pending_t          set_swap_pending;
    fbe_provision_drive_swap_pending_reason_t       set_swap_pending_reason;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, 
                                      (void **)&mark_pvd_swap_pending_p, 
                                      sizeof(fbe_database_control_mark_pvd_swap_pending_t));
    if ( status != FBE_STATUS_OK ) {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /* Validate the swap type.
     */
    switch (mark_pvd_swap_pending_p->swap_command) {
        case FBE_SPARE_INITIATE_PROACTIVE_COPY_COMMAND:
            set_swap_pending_reason = FBE_PROVISION_DRIVE_DRIVE_SWAP_PENDING_REASON_PROACTIVE_COPY;
            break;
        case FBE_SPARE_INITIATE_USER_COPY_COMMAND:
        case FBE_SPARE_INITIATE_USER_COPY_TO_COMMAND:
            set_swap_pending_reason = FBE_PROVISION_DRIVE_DRIVE_SWAP_PENDING_REASON_USER_COPY;
            break;
        default:
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO,
                           "%s to obj: 0x%x unsupported swap type: %d \n",
                           __FUNCTION__, mark_pvd_swap_pending_p->object_id, mark_pvd_swap_pending_p->swap_command);
            fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE);
            return FBE_STATUS_GENERIC_FAILURE;
    }

    /* Setup the request.
     */
    fbe_zero_memory(&set_swap_pending, sizeof(fbe_provision_drive_set_swap_pending_t));
    set_swap_pending.set_swap_pending_reason = set_swap_pending_reason;

    /* Send it.
     */
    status = fbe_database_send_packet_to_object(FBE_PROVISION_DRIVE_CONTROL_SET_SWAP_PENDING,
                                                &set_swap_pending, 
                                                sizeof(fbe_provision_drive_set_swap_pending_t),  
                                                mark_pvd_swap_pending_p->object_id, 
                                                NULL,  /* no sg list*/
                                                0,  /* no sg list*/
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s to obj: 0x%x failed - status: 0x%x\n",
                       __FUNCTION__, mark_pvd_swap_pending_p->object_id, status);
        fbe_database_complete_packet(packet, status);
        return status;
    }
    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 *          fbe_database_control_get_peer_sep_version()
 ****************************************************************************** 
 * 
 * @brief   Get the peer SP SEP Version
 * 
 * @param   packet_p - packet with database request
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  09/09/2014  Ashok Tamilarasan  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_control_get_peer_sep_version(fbe_packet_t *packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u64_t *peer_sep_version = NULL;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, 
                                      (void **)&peer_sep_version, 
                                      sizeof(fbe_u64_t));
    if (status != FBE_STATUS_OK) {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    database_trace(FBE_TRACE_LEVEL_WARNING, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s Peer SEP Version:%d\n",
                   __FUNCTION__, (unsigned int)&fbe_database_service.peer_sep_version);

    *peer_sep_version = fbe_database_service.peer_sep_version;

    /* Return success.
     */
    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 *          fbe_database_control_set_peer_sep_version()
 ****************************************************************************** 
 * 
 * @brief   Set the peer SEP version
 * 
 * @param   packet_p - packet with database request
 * 
 * @return  status - status of the operation.
 *
 * @notes  TESTING PURPOSES ONLY
 *
 * @author
 *   09/09/2014  Ashok Tamilarasan  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_control_set_peer_sep_version(fbe_packet_t *packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_u64_t *peer_sep_version = NULL;

   /* Verify packet contents */
    status = fbe_database_get_payload(packet, 
                                      (void **)&peer_sep_version, 
                                      sizeof(fbe_u64_t));
    if (status != FBE_STATUS_OK) {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    database_trace(FBE_TRACE_LEVEL_WARNING, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s Peer SEP Version:%d\n",
                   __FUNCTION__, (unsigned int)&fbe_database_service.peer_sep_version);

    fbe_database_service.peer_sep_version = *peer_sep_version;

    /* Return success.
     */
    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 *          fbe_database_control_clear_pvd_swap_pending()
 ******************************************************************************
 * @brief
 *  Clear the `mark offline' flags for the provision drive specified.
 * 
 * @param   packet_p - packet with database request
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  06/05/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_control_clear_pvd_swap_pending(fbe_packet_t *packet)
{
    fbe_status_t                                status = FBE_STATUS_OK;
    fbe_database_control_clear_pvd_swap_pending_t   *clear_pvd_swap_pending_p = NULL;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, 
                                      (void **)&clear_pvd_swap_pending_p, 
                                      sizeof(fbe_database_control_clear_pvd_swap_pending_t));
    if ( status != FBE_STATUS_OK ) {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /* Send it.
     */
    status = fbe_database_send_packet_to_object(FBE_PROVISION_DRIVE_CONTROL_CLEAR_SWAP_PENDING,
                                                NULL, 
                                                0,  
                                                clear_pvd_swap_pending_p->object_id, 
                                                NULL,  /* no sg list*/
                                                0,  /* no sg list*/
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s to obj: 0x%x failed - status: 0x%x\n",
                       __FUNCTION__, clear_pvd_swap_pending_p->object_id, status);
        fbe_database_complete_packet(packet, status);
        return status;
    }
    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 *          fbe_database_control_set_debug_flags()
 ****************************************************************************** 
 * 
 * @brief   Set the database debug flags (enable validate, etc).      
 * 
 * @param   packet_p - packet with database request
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  06/26/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_control_set_debug_flags(fbe_packet_t *packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_database_control_set_debug_flags_t *set_debug_flags_p = NULL;
    fbe_database_service_t                 *database_p = fbe_database_get_database_service();

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, 
                                      (void **)&set_debug_flags_p, 
                                      sizeof(fbe_database_control_set_debug_flags_t));
    if (status != FBE_STATUS_OK) {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /* Set the debug flags.
     */
    fbe_database_set_debug_flags(database_p, set_debug_flags_p->set_db_debug_flags);

    /* Return success.
     */
    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 *          fbe_database_control_get_debug_flags()
 ****************************************************************************** 
 * 
 * @brief   Get the database debug flags (enable validate, etc).      
 * 
 * @param   packet_p - packet with database request
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  06/27/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_control_get_debug_flags(fbe_packet_t *packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_database_control_get_debug_flags_t *get_debug_flags_p = NULL;
    fbe_database_service_t                 *database_p = fbe_database_get_database_service();

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, 
                                      (void **)&get_debug_flags_p, 
                                      sizeof(fbe_database_control_set_debug_flags_t));
    if (status != FBE_STATUS_OK) {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /* Get the debug flags.
     */
    fbe_database_get_debug_flags(database_p, &get_debug_flags_p->get_db_debug_flags);

    /* Return success.
     */
    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 *          fbe_database_control_is_validation_allowed()
 ****************************************************************************** 
 * 
 * @brief   Check if validation is allowed or not.  
 * 
 * @param   packet_p - packet with database request
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  07/03/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_control_is_validation_allowed(fbe_packet_t *packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_database_control_is_validation_allowed_t *is_allowed_p = NULL;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, 
                                      (void **)&is_allowed_p, 
                                      sizeof(fbe_database_control_is_validation_allowed_t));
    if (status != FBE_STATUS_OK) {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /* User requests are always allowed.
     */
    if (is_allowed_p->validate_caller == FBE_DATABASE_VALIDATE_REQUEST_TYPE_USER) {
        is_allowed_p->not_allowed_reason = FBE_DATABASE_VALIDATE_NOT_ALLOWED_REASON_NONE;
        is_allowed_p->b_allowed = FBE_TRUE;
        fbe_database_complete_packet(packet, FBE_STATUS_OK);
        return FBE_STATUS_OK;
    }

    /* Validate the database
     */
    is_allowed_p->b_allowed = fbe_database_is_validation_allowed(is_allowed_p->validate_caller,
                                                                 &is_allowed_p->not_allowed_reason);

    /* Return success.
     */
    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 *          fbe_database_control_validate_database()
 ****************************************************************************** 
 * 
 * @brief   Validate the database.     
 * 
 * @param   packet_p - packet with database request
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  06/27/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_control_validate_database(fbe_packet_t *packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_database_control_validate_database_t *validate_request_p = NULL;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, 
                                      (void **)&validate_request_p, 
                                      sizeof(fbe_database_control_validate_database_t));
    if (status != FBE_STATUS_OK) {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /* Currently `correct entry' is not supported.
     */
    if (validate_request_p->failure_action == FBE_DATABASE_VALIDATE_FAILURE_ACTION_CORRECT_ENTRY) {
        status = FBE_STATUS_GENERIC_FAILURE;
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s Currently correct entry: (%d) is not supported\n",
                       __FUNCTION__, validate_request_p->failure_action);
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /* Validate the database
     */
    status = fbe_database_validate_database_if_enabled(validate_request_p->validate_caller,
                                                       validate_request_p->failure_action,
                                                       &validate_request_p->validation_status);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s failed - status: 0x%x\n",
                       __FUNCTION__, status);
        fbe_database_complete_packet(packet, status);
        return status;
    }

    /* Return success.
     */
    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 *          fbe_database_control_enter_degraded_mode()
 ****************************************************************************** 
 * 
 * @brief   Tell the database to enter degraded mode.
 * 
 * @param   packet_p - packet with database request
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  06/27/2014  Ron Proulx  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_control_enter_degraded_mode(fbe_packet_t *packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_database_control_enter_degraded_mode_t *degraded_request_p = NULL;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, 
                                      (void **)&degraded_request_p, 
                                      sizeof(fbe_database_control_enter_degraded_mode_t));
    if (status != FBE_STATUS_OK) {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    database_trace(FBE_TRACE_LEVEL_WARNING, 
                   FBE_TRACE_MESSAGE_ID_INFO,
                   "%s enter degraded reason: %d\n",
                   __FUNCTION__, degraded_request_p->degraded_mode_reason);

    /*! @note Enter degraded mode for the reason requested.
     */
    fbe_database_enter_service_mode(degraded_request_p->degraded_mode_reason);

    /* Return success.
     */
    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}

/*!***************************************************************
* @fn fbe_database_get_ext_pool_object_id_from_pool_id
*****************************************************************
* @brief
* This function gets the EXTENT POOL object id.
*
* @param pool_id        - pool id of the extent pool.
* @param pool_object_id - Pointer to the pool object id.
*
* @return fbe_status_t
* 
****************************************************************/
fbe_status_t 
fbe_database_get_ext_pool_object_id_from_pool_id(fbe_u32_t pool_id, 
                                                 fbe_object_id_t * pool_object_id)
{
    fbe_status_t                     		status = FBE_STATUS_GENERIC_FAILURE;
    database_object_entry_t *	            object_entry = NULL;
    database_user_entry_t *                 user_entry = NULL;
    fbe_object_id_t	                        object_id;

    /* go over the tables */
    for (object_id = 0; object_id < fbe_database_service.object_table.table_size; object_id++) {
        status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                                  object_id,
                                                                  &object_entry);  

        if (status != FBE_STATUS_OK || !database_is_entry_valid(&object_entry->header)){
            continue;
        }
        if (object_entry->db_class_id != DATABASE_CLASS_ID_EXTENT_POOL) 
        {
            continue;
        }

        status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                                       object_id,
                                                                       &user_entry);
        if (status != FBE_STATUS_OK || user_entry == NULL) {
            continue;
        }
        if (pool_id == user_entry->user_data_union.ext_pool_user_data.pool_id){
            *pool_object_id = object_id;
            return FBE_STATUS_OK;
        }
    }

    return FBE_STATUS_GENERIC_FAILURE;
}

/*!***************************************************************
* @fn fbe_database_get_ext_pool_lun_config_info
*****************************************************************
* @brief
* This function gets the EXTENT POOL object config information.
*
* @param lun_object_id - object id of the extent pool.
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
fbe_status_t 
fbe_database_get_ext_pool_lun_config_info(fbe_object_id_t lun_object_id, 
                                          fbe_u32_t * index, 
                                          fbe_lba_t * capacity, 
                                          fbe_lba_t * offset, 
                                          fbe_object_id_t * pool_object_id)
{
    fbe_status_t                     		status = FBE_STATUS_GENERIC_FAILURE;
    database_object_entry_t *	            object_entry = NULL;
    database_user_entry_t *                 user_entry = NULL;
    fbe_u32_t                               pool_id;
    fbe_base_object_mgmt_get_lifecycle_state_t  get_lifecycle;

    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              lun_object_id,
                                                              &object_entry);  
    if (status != FBE_STATUS_OK || !database_is_entry_valid(&object_entry->header)){
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get object entry for object 0x%X\n",__FUNCTION__, lun_object_id);
        return FBE_STATUS_NO_OBJECT;
    }
    *index = object_entry->set_config_union.ext_pool_lun_config.server_index;
    *capacity = object_entry->set_config_union.ext_pool_lun_config.capacity;
    *offset = object_entry->set_config_union.ext_pool_lun_config.offset;

    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                                   lun_object_id,
                                                                   &user_entry);
    if (status != FBE_STATUS_OK || user_entry == NULL) {
        database_trace(FBE_TRACE_LEVEL_ERROR, 
                       FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                       "%s: get user entry failed!\n", 
                       __FUNCTION__);
        return FBE_STATUS_NO_OBJECT;
    }
    pool_id = user_entry->user_data_union.ext_pool_lun_user_data.pool_id;

    status = fbe_database_get_ext_pool_object_id_from_pool_id(pool_id, pool_object_id);
    if (status != FBE_STATUS_OK){
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: failed to get object for pool_id 0x%X\n",__FUNCTION__, pool_id);
        return FBE_STATUS_NO_OBJECT;
    }

    /* Wait till ext_pool object to be ready */
    get_lifecycle.lifecycle_state = FBE_LIFECYCLE_STATE_INVALID;
    status = fbe_database_send_packet_to_object(FBE_BASE_OBJECT_CONTROL_CODE_GET_LIFECYCLE_STATE,
                                                &get_lifecycle,
                                                sizeof(fbe_base_object_mgmt_get_lifecycle_state_t),
                                                *pool_object_id,
                                                NULL,
                                                0,
                                                FBE_PACKET_FLAG_NO_ATTRIB,
                                                FBE_PACKAGE_ID_SEP_0);
    if (status != FBE_STATUS_OK || get_lifecycle.lifecycle_state != FBE_LIFECYCLE_STATE_READY) {
        database_trace(FBE_TRACE_LEVEL_INFO, 
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s: pool object not ready\n", 
                       __FUNCTION__);
        return FBE_STATUS_GENERIC_FAILURE;
    }

    return FBE_STATUS_OK;
}

/*!***************************************************************
* @fn database_control_lookup_extent_pool_lun_by_number
*****************************************************************
* @brief
* This function gets the EXTENT POOL object id.
*
* @param packet - Pointer to the packet.
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
static fbe_status_t database_control_lookup_ext_pool_lun_by_number(fbe_packet_t * packet)
{
    fbe_status_t                     status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_lookup_ext_pool_lun_t *lookup_lun = NULL;
    database_user_entry_t           *out_entry_ptr = NULL;     

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&lookup_lun, sizeof(*lookup_lun));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    status = fbe_database_config_table_get_user_entry_by_ext_pool_lun_id(&fbe_database_service.user_table, 
                                                                lookup_lun->pool_id,
                                                                lookup_lun->lun_id,
                                                                &out_entry_ptr);
    if ((status == FBE_STATUS_OK)&&(out_entry_ptr != NULL)) {
        /* get the entry ok */
        if (out_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) {
            /* only return the object when the entry is valid */
            lookup_lun->object_id = out_entry_ptr->header.object_id;
        }
    }else{
        lookup_lun->object_id = FBE_OBJECT_ID_INVALID;
        /*trace to a low level since many times it is used just to look for the next free lun number and it will be confusing in traces*/
        database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to find LUN %d; returned 0x%x.\n",
            __FUNCTION__, lookup_lun->lun_id, status);
        status = FBE_STATUS_NO_OBJECT;
    }
    fbe_database_complete_packet(packet, status);
    return status;
}

/*!***************************************************************
* @fn database_control_lookup_extent_pool_by_number
*****************************************************************
* @brief
* This function gets the EXTENT POOL object id.
*
* @param packet - Pointer to the packet.
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
static fbe_status_t database_control_lookup_ext_pool_by_number(fbe_packet_t * packet)
{
    fbe_status_t                     status = FBE_STATUS_GENERIC_FAILURE;
    fbe_database_control_lookup_ext_pool_t *lookup_pool = NULL;  
    database_user_entry_t           *out_entry_ptr = NULL;     

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&lookup_pool, sizeof(fbe_database_control_lookup_ext_pool_t));
    if (status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    status = fbe_database_config_table_get_user_entry_by_ext_pool_id(&fbe_database_service.user_table, 
                                                               lookup_pool->pool_id,
                                                               &out_entry_ptr);
    if ((status == FBE_STATUS_OK)&&(out_entry_ptr != NULL)) {
        /* get the entry ok */
        if (out_entry_ptr->header.state == DATABASE_CONFIG_ENTRY_VALID) {
            /* only return the object when the entry is valid */
            lookup_pool->object_id = out_entry_ptr->header.object_id;
        }
    }else{
        lookup_pool->object_id = FBE_OBJECT_ID_INVALID;
        status = FBE_STATUS_NO_OBJECT;

        database_trace(FBE_TRACE_LEVEL_INFO, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: failed to find pool %d; returned 0x%x.\n",
            __FUNCTION__, lookup_pool->pool_id, status);
    }
    fbe_database_complete_packet(packet, status);
    return status;
}



/*!***************************************************************
* @fn fbe_database_control_destroy_extent_pool
*****************************************************************
* @brief
* destroy extent pool object
*
* @param packet -
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
static fbe_status_t fbe_database_control_destroy_extent_pool(fbe_packet_t * packet)
{
    fbe_database_control_destroy_object_t      *destroy_pool = NULL;
    fbe_database_transaction_id_t              current_id = FBE_DATABASE_TRANSACTION_ID_INVALID;
    fbe_status_t                               status;
    
    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&destroy_pool, sizeof(*destroy_pool));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &current_id);
    /* check if the transaction id is valid */
    if (destroy_pool->transaction_id != current_id){
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Invalid id 0x%llx.  Current id is 0x%llx\n",
            __FUNCTION__, (unsigned long long)destroy_pool->transaction_id,
        (unsigned long long)current_id);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE );
        return FBE_STATUS_GENERIC_FAILURE;
    }
    database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Destroying Extent Pool - transaction 0x%x.\n",
        __FUNCTION__, (unsigned int)destroy_pool->transaction_id);	

    /*let's start with the peer first if it's there*/
    if (is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)destroy_pool,
                                                             FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL);

    }
    
    if (status == FBE_STATUS_OK) {
        status = database_destroy_extent_pool(destroy_pool, FALSE);
    }
    
    fbe_database_complete_packet(packet, status);
    return status;
}


/*!***************************************************************
* @fn database_destroy_extent_pool
*****************************************************************
* @brief
* destroy extent pool object
*
* @param packet -
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
fbe_status_t database_destroy_extent_pool(fbe_database_control_destroy_object_t * destroy_pool,
                                          fbe_bool_t                              confirm_peer)
{
    database_object_entry_t                    object_entry;
    database_user_entry_t                      user_entry;	
    fbe_status_t                               status;
    database_user_entry_t *                    user_entry_ptr;
    fbe_database_cmi_msg_t *                   msg_memory;    
    database_object_entry_t*                   existing_object_entry = NULL;
    database_user_entry_t*                      existing_user_entry = NULL;

    /*let' makr the lun number is valid to be reused by other users if they wish to*/
    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table, destroy_pool->object_id, &user_entry_ptr);
    if (status != FBE_STATUS_OK) {
        return status;
    }

    /*update transaction user and object tables */	  
    fbe_spinlock_lock(&fbe_database_service.object_table.table_lock);
    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              destroy_pool->object_id,
                                                              &existing_object_entry);
    if (status != FBE_STATUS_OK )
    {
        fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s Failed to find object %d in object table\n",
                     __FUNCTION__,destroy_pool->object_id);
        return status;
    }
    fbe_copy_memory(&object_entry, existing_object_entry,sizeof(database_object_entry_t));
    fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);

    status = database_common_destroy_object_from_table_entry(&object_entry);

    if(confirm_peer)
    {
        msg_memory = fbe_database_cmi_get_msg_memory();

        if (msg_memory == NULL) 
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to get CMI msg memory\n", __FUNCTION__);
        }
        else
        {
            /*we'll return the status as a handshake to the master*/
            msg_memory->msg_type = FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL_CONFIRM;
            msg_memory->completion_callback = fbe_database_config_change_request_from_peer_completion;
            msg_memory->payload.transaction_confirm.status = status;

            fbe_database_cmi_send_message(msg_memory, NULL);
        }
    }

    status = database_common_wait_destroy_object_complete();
    
    object_entry.header.state = DATABASE_CONFIG_ENTRY_DESTROY;

    status = fbe_database_transaction_add_object_entry(fbe_database_service.transaction_ptr, 
                                                       &object_entry);


    fbe_spinlock_lock(&fbe_database_service.user_table.table_lock);
    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                              destroy_pool->object_id,
                                                              &existing_user_entry);
    if (status != FBE_STATUS_OK )
    {
        fbe_spinlock_unlock(&fbe_database_service.user_table.table_lock);
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s Failed to find object %d in user table\n",
                     __FUNCTION__,destroy_pool->object_id);
        return status;
    }
    fbe_copy_memory(&user_entry, existing_user_entry,sizeof(database_user_entry_t));
    fbe_spinlock_unlock(&fbe_database_service.user_table.table_lock);

    user_entry.header.state = DATABASE_CONFIG_ENTRY_DESTROY;
    status = fbe_database_transaction_add_user_entry(fbe_database_service.transaction_ptr, 
                                                     &user_entry);
    
    database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Destroy RG transaction 0x%x returned 0x%x.\n",
        __FUNCTION__, (unsigned int)destroy_pool->transaction_id, status);

    /* Log message to the event-log when DB has already added a RG to the Table, and not logging it at the RG
     * destroy commit time.  We cannot write to the Event Log in CMI thread.
     */
#if 0
    status = fbe_event_log_write(SEP_INFO_RAID_GROUP_DESTROYED, NULL, 0, FBE_WIDE_CHAR("%x %d"),
                    destroy_raid->object_id, 
                    user_entry.user_data_union.rg_user_data.raid_group_number); 
#endif

    return status;

}

/*!***************************************************************
* @fn fbe_database_control_destroy_ext_pool_lun
*****************************************************************
* @brief
* destroy extent pool lun object
*
* @param packet -
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
static fbe_status_t fbe_database_control_destroy_ext_pool_lun(fbe_packet_t * packet)
{
    fbe_database_control_destroy_object_t      *destroy_lun = NULL;		
    fbe_database_transaction_id_t              current_id = FBE_DATABASE_TRANSACTION_ID_INVALID;	
    fbe_status_t                               status;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&destroy_lun, sizeof(*destroy_lun));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }
    fbe_database_transaction_get_id(fbe_database_service.transaction_ptr, &current_id);
    /* check if the transaction id is valid */
    if (destroy_lun->transaction_id != current_id){
        database_trace(FBE_TRACE_LEVEL_WARNING, 
            FBE_TRACE_MESSAGE_ID_INFO,
            "%s: Invalid id 0x%llx.  Current id is 0x%llx\n",
            __FUNCTION__, (unsigned long long)destroy_lun->transaction_id,
        current_id);
        fbe_database_complete_packet(packet, FBE_STATUS_GENERIC_FAILURE );
        return FBE_STATUS_GENERIC_FAILURE;
    }
    database_trace(FBE_TRACE_LEVEL_DEBUG_MEDIUM, 
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Destroying LUN - transaction 0x%x.\n",
        __FUNCTION__, (unsigned int)destroy_lun->transaction_id);	

    /*let's start with the peer first if it's there*/
    if (is_peer_update_allowed(&fbe_database_service)) {
        /*synch to the peer*/
        status = fbe_database_cmi_send_update_config_to_peer(&fbe_database_service,
                                                             (void *)destroy_lun,
                                                             FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL_LUN);

    }

    if (status == FBE_STATUS_OK) {
        status = database_destroy_ext_pool_lun(destroy_lun, FALSE);
    }

    fbe_database_complete_packet(packet, status);
    return status;
}

/*!***************************************************************
* @fn database_destroy_ext_pool_lun
*****************************************************************
* @brief
* destroy extent pool lun object
*
* @param packet -
*
* @return fbe_status_t - FBE_STATUS_GENERIC_FAILURE if any issue
* 
****************************************************************/
fbe_status_t database_destroy_ext_pool_lun(fbe_database_control_destroy_object_t * destroy_lun,
                                           fbe_bool_t                            confirm_peer)
{
    database_object_entry_t                    object_entry;
    database_user_entry_t                      user_entry;
    fbe_status_t                               status;
    database_user_entry_t *                    user_entry_ptr;
    //fbe_u32_t                                  lun_number;  
    fbe_database_cmi_msg_t *                   msg_memory;   
    database_object_entry_t*                   existing_object_entry = NULL;
    database_user_entry_t*                     existing_user_entry = NULL;

    /*let' makr the lun number is valid to be reused by other users if they wish to*/
    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table, destroy_lun->object_id, &user_entry_ptr);
    if (status != FBE_STATUS_OK) {
        return status;
    }
    
    /*update transaction user and object tables */	  
    fbe_spinlock_lock(&fbe_database_service.object_table.table_lock);
    status = fbe_database_config_table_get_object_entry_by_id(&fbe_database_service.object_table,
                                                              destroy_lun->object_id,
                                                              &existing_object_entry);
    if (status != FBE_STATUS_OK )
    {
        fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s Failed to find object %d in object table\n",
                     __FUNCTION__,destroy_lun->object_id);
        return status;
    }
    fbe_copy_memory(&object_entry, existing_object_entry,sizeof(database_object_entry_t));
    fbe_spinlock_unlock(&fbe_database_service.object_table.table_lock);

    status = database_common_destroy_object_from_table_entry(&object_entry);

    if(confirm_peer)
    {
        msg_memory = fbe_database_cmi_get_msg_memory();
        if (msg_memory == NULL) 
        {
            database_trace(FBE_TRACE_LEVEL_ERROR, 
                           FBE_TRACE_MESSAGE_ID_INFO, 
                           "%s: failed to get CMI msg memory\n", __FUNCTION__);
        }
        else
        {
            /*we'll return the status as a handshake to the master*/
            msg_memory->msg_type = FBE_DATABASE_CMI_MSG_TYPE_DESTROY_EXTENT_POOL_LUN_CONFIRM;
            msg_memory->completion_callback = fbe_database_config_change_request_from_peer_completion;
            msg_memory->payload.transaction_confirm.status = status;

            fbe_database_cmi_send_message(msg_memory, NULL);
        }
    }

    status = database_common_wait_destroy_object_complete();
    
    object_entry.header.state = DATABASE_CONFIG_ENTRY_DESTROY;

    status = fbe_database_transaction_add_object_entry(fbe_database_service.transaction_ptr, 
                                                       &object_entry);

    fbe_spinlock_lock(&fbe_database_service.user_table.table_lock);
    status = fbe_database_config_table_get_user_entry_by_object_id(&fbe_database_service.user_table,
                                                              destroy_lun->object_id,
                                                              &existing_user_entry);
    if (status != FBE_STATUS_OK )
    {
        fbe_spinlock_unlock(&fbe_database_service.user_table.table_lock);
        database_trace(FBE_TRACE_LEVEL_ERROR, FBE_TRACE_MESSAGE_ID_INFO,
                     "%s Failed to find object %d in user table\n",
                     __FUNCTION__,destroy_lun->object_id);
        return status;
    }
    fbe_copy_memory(&user_entry, existing_user_entry,sizeof(database_user_entry_t));
    fbe_spinlock_unlock(&fbe_database_service.user_table.table_lock);

    user_entry.header.state = DATABASE_CONFIG_ENTRY_DESTROY;
    status = fbe_database_transaction_add_user_entry(fbe_database_service.transaction_ptr, 
                                                     &user_entry);

#if 0
    /* get LUN number based on object ID */
    fbe_database_get_lun_number(destroy_lun->object_id, &lun_number);

    /* Log message to the event-log when DB has already updated a LUN to the Table, and not logging it at the LUN
     * destroy commit time.  We cannot write to the Event Log in CMI thread.
     */
    status = fbe_event_log_write(SEP_INFO_LUN_DESTROYED, NULL, 0, FBE_WIDE_CHAR("%d"),                                         
                                 lun_number);  
#endif
    database_trace(FBE_TRACE_LEVEL_DEBUG_LOW, 
        FBE_TRACE_MESSAGE_ID_INFO,
        "%s: Destroy ext pool lun obj: 0x%x, trans=0x%llx, sts=0x%x.\n",
        __FUNCTION__, destroy_lun->object_id,
    (unsigned long long)destroy_lun->transaction_id, status);

    return status;
}

/*!***************************************************************
* @fn fbe_database_mini_mode
*****************************************************************
* @brief
* In Unity stream, the SEP driver will be loaded into bootflash to replace ntmirror.
* However we don't have CMI available in bootflash, the CMI drivers are disabled in the mini-safe container.
* In bootflash mode, we assume both SPs are ACTIVE and database service will runs in mini mode.
* 
* @param  -
*
* @return fbe_bool_t - FBE_TRUE if in bootflash 
* 
****************************************************************/
fbe_bool_t fbe_database_mini_mode(void)
{
    return fbe_cmi_service_disabled();
}

/*!***************************************************************
* @fn database_control_check_bootflash_mode
*****************************************************************
* @brief
* check if the database runs in bootflash mode
* 
* 
* @param packet -
*
* @return fbe_status_t - FBE_STATUS_OK
* 
****************************************************************/
static fbe_status_t database_control_check_bootflash_mode(fbe_packet_t * packet)
{    
    fbe_status_t status;
    fbe_bool_t  *bootflash_mode_flag = NULL;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, (void **)&bootflash_mode_flag, sizeof(*bootflash_mode_flag));
    if ( status != FBE_STATUS_OK )
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }    

    *bootflash_mode_flag = fbe_database_mini_mode();

    fbe_database_complete_packet(packet, status);
    return status;
}

/*!***************************************************************
* @fn fbe_database_control_update_mirror_pvd_map 
*****************************************************************
* @brief
* 
*  update mirror pvd mapping
* 
* @param packet -
*
* @return fbe_status_t - FBE_STATUS_OK
* 
****************************************************************/
static fbe_status_t fbe_database_control_update_mirror_pvd_map(fbe_packet_t *packet)
{

    fbe_status_t    status;
    fbe_database_control_mirror_update_t *mirror_update = NULL;

    status = fbe_database_get_payload(packet, (void **)&mirror_update, sizeof(*mirror_update));
    if (status != FBE_STATUS_OK)
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    status = fbe_c4_mirror_rginfo_modify_drive_SN(mirror_update->rg_obj_id,
                                               mirror_update->edge_index,
                                               mirror_update->sn,
                                               FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);

    fbe_database_complete_packet(packet,  status);

    return status;
}

/*!***************************************************************
* @fn fbe_database_control_get_mirror_pvd_map 
*****************************************************************
* @brief
* 
*  get mirror pvd mapping
* 
* @param packet -
*
* @return fbe_status_t - FBE_STATUS_OK
* 
****************************************************************/
static fbe_status_t fbe_database_control_get_mirror_pvd_map(fbe_packet_t *packet)
{

    fbe_status_t    status;
    fbe_database_control_mirror_update_t *mirror_update = NULL;

    status = fbe_database_get_payload(packet, (void **)&mirror_update, sizeof(*mirror_update));
    if (status != FBE_STATUS_OK)
    {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    status = fbe_c4_mirror_rginfo_get_pvd_mapping(mirror_update->rg_obj_id,
                                               mirror_update->edge_index,
                                               mirror_update->sn,
                                               FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE);

    fbe_database_complete_packet(packet,  status);

    return status;
}


/*!****************************************************************************
 *          fbe_database_control_get_supported_drive_types()
 ****************************************************************************** 
 * 
 * @brief   A control operation to get the supported drive types.
 * 
 * @param   packet_p - packet with database request
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  08/15/2015  Wayne Garrett  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_control_get_supported_drive_types(fbe_packet_t *packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_database_additional_drive_types_supported_t    *supported_types_p = NULL;
    fbe_database_service_t                  *database_p = fbe_database_get_database_service();

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, 
                                      (void **)&supported_types_p, 
                                      sizeof(*supported_types_p));
    if (status != FBE_STATUS_OK) {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    *supported_types_p = database_p->supported_drive_types;

    /* Return success.
     */
    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 *          fbe_database_control_set_supported_drive_types()
 ****************************************************************************** 
 * 
 * @brief   A control operation to set the supported drive types.
 * 
 * @param   packet_p - packet with database request
 * 
 * @return  status - status of the operation.
 *
 * @author
 *  08/15/2015  Wayne Garrett  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_control_set_supported_drive_types(fbe_packet_t *packet)
{
    fbe_status_t                            status = FBE_STATUS_OK;
    fbe_database_service_t                  *database_p = fbe_database_get_database_service();
    fbe_database_additional_drive_types_supported_t    previous_types, new_types;
    fbe_database_control_set_supported_drive_type_t  *set_drive_type_p = NULL;

    /* Verify packet contents */
    status = fbe_database_get_payload(packet, 
                                      (void **)&set_drive_type_p, 
                                      sizeof(*set_drive_type_p));
    if (status != FBE_STATUS_OK) {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    previous_types = database_p->supported_drive_types;
    
    if (set_drive_type_p->do_enable)  
    {
        new_types =  previous_types | set_drive_type_p->type;
    }        
    else  /* disable bit. */
    {
        new_types =  previous_types & ~set_drive_type_p->type;
    }


    /* change registry, change memory value, then rediscover any offline drives that may
       now be supported */

    status = fbe_database_registry_addl_set_supported_drive_types(new_types);
    if (status != FBE_STATUS_OK) {
        fbe_database_complete_packet(packet, status);
        return status;
    }

    database_p->supported_drive_types = new_types;

    if (set_drive_type_p->do_enable)
    {
        fbe_database_reactivate_pdos_of_drive_type(set_drive_type_p->type);
    }

    /* Return success.
     */
    fbe_database_complete_packet(packet, FBE_STATUS_OK);
    return FBE_STATUS_OK;
}

/*!****************************************************************************
 *          fbe_database_initialize_supported_drive_types()
 ****************************************************************************** 
 * 
 * @brief   This function initialize the supported drive types from registry.
 *          If the registry does not set, retrun FBE_DATABASE_ADDITIONAL_DRIVE_TYPE_SUPPORTED_DEFAULT instead.
 * 
 * @param   void 
 * 
 * @return  supported_drive_types - pointer 
 *
 * @author
 *  11/19/2015  Jibing Dong  - Created.
 *
 ******************************************************************************/
static fbe_status_t fbe_database_initialize_supported_drive_types(fbe_database_additional_drive_types_supported_t *supported_drive_types)
{
    fbe_status_t status;
    fbe_board_mgmt_platform_info_t platform_info;

    /* use registry value if the key is set */ 
    status = fbe_database_registry_get_addl_supported_drive_types(supported_drive_types);
    if (status == FBE_STATUS_OK) 
    {
        return status;
    }
   
    /* otherwise, set the supported_drive_type by platform type */
    status = fbe_database_get_platform_info(&platform_info);
    if (status != FBE_STATUS_OK) {
        database_trace(FBE_TRACE_LEVEL_WARNING, 
                       FBE_TRACE_MESSAGE_ID_INFO, 
                       "%s: failed to get platform information, status %d\n",
                       __FUNCTION__, status);
    }

    switch (platform_info.hw_platform_info.cpuModule) {
        case BLACKWIDOW_CPU_MODULE:
        case WILDCATS_CPU_MODULE:
        case MAGNUM_CPU_MODULE:
        case BOOMSLANG_CPU_MODULE:
        case INTREPID_CPU_MODULE:
        case ARGONAUT_CPU_MODULE:
        case SENTRY_CPU_MODULE:
        case EVERGREEN_CPU_MODULE:
        case DEVASTATOR_CPU_MODULE:
        case MEGATRON_CPU_MODULE:
        case STARSCREAM_CPU_MODULE:
        case JETFIRE_CPU_MODULE:
        case BEACHCOMBER_CPU_MODULE:
        case SILVERBOLT_CPU_MODULE:
        case TRITON_ERB_CPU_MODULE:
        case TRITON_CPU_MODULE:
        case MERIDIAN_CPU_MODULE:
        case TUNGSTEN_CPU_MODULE:
 
            *supported_drive_types |= FBE_DATABASE_DRIVE_TYPE_SUPPORTED_520_HDD;
            break;

        case HYPERION_CPU_MODULE:
        case OBERON_CPU_MODULE:
        case CHARON_CPU_MODULE:
            /* use default */
            break;

        /* add new platform support here */

        default:
            break;
    }

    return status;
}
/***********************************************
* end file fbe_database_main.c
***********************************************/
