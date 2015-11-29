#ifndef FBE_DATABASE_CONFIG_TABLES_H
#define FBE_DATABASE_CONFIG_TABLES_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_database_private.h
 ***************************************************************************
 *
 * @brief
 *  This file contains local definitions for the database service.
 *
 * @version
 *   12/15/2010:  Created. 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe_database_common.h"
#include "fbe_database_private.h"
#include "fbe_database.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/
fbe_status_t fbe_database_allocate_config_tables(fbe_database_service_t *fbe_database_service);
fbe_status_t fbe_database_config_tables_destroy(fbe_database_service_t *fbe_database_service);

fbe_status_t fbe_database_populate_default_global_info(database_table_t *in_table_ptr);
fbe_status_t fbe_database_populate_system_power_save_info(fbe_system_power_saving_info_t *in_table_ptr);
fbe_status_t fbe_database_populate_system_encryption_info(fbe_system_encryption_info_t *in_table_ptr);
fbe_status_t fbe_database_populate_system_spare(fbe_system_spare_config_info_t *in_entry_ptr);
fbe_status_t fbe_database_populate_system_generation_info(fbe_system_generation_info_t *in_entry_ptr);
fbe_status_t fbe_database_populate_global_pvd_config(fbe_global_pvd_config_t *in_entry_ptr);

fbe_status_t fbe_database_config_table_get_free_object_entry(database_object_entry_t *in_object_table_ptr, 
                                                             database_table_size_t size, 
                                                             database_object_entry_t **out_object_entry_ptr);

fbe_status_t fbe_database_config_table_get_free_user_entry(database_user_entry_t *in_user_table_ptr, 
                                                             database_table_size_t size, 
                                                             database_user_entry_t **out_user_entry_ptr);

fbe_status_t fbe_database_config_table_get_free_edge_entry(database_edge_entry_t *in_edge_table_ptr, 
                                                             database_table_size_t size, 
                                                             database_edge_entry_t **out_edge_entry_ptr);

fbe_status_t fbe_database_config_table_get_free_global_info_entry(database_global_info_entry_t *in_global_info_table_ptr, 
                                                                  database_table_size_t size, 
                                                                  database_global_info_entry_t **out_global_info_table_ptr);

fbe_status_t fbe_database_config_table_get_object_entry_by_id(database_table_t *in_table_ptr,
                                                              fbe_object_id_t object_id,
                                                              database_object_entry_t **out_entry_ptr);

fbe_status_t fbe_database_config_table_get_object_entry_by_pvd_SN(database_object_entry_t *in_table_ptr,
                                                                  database_table_size_t size, 
                                                                  fbe_u8_t *SN,
                                                                  database_object_entry_t **out_entry_ptr);

fbe_status_t fbe_database_config_table_get_last_object_entry_by_pvd_SN(database_object_entry_t *in_table_ptr,
                                                                  database_table_size_t size, 
                                                                  fbe_u8_t *SN,
                                                                  database_object_entry_t **out_entry_ptr);

fbe_status_t fbe_database_config_table_get_last_object_entry_by_pvd_SN_or_max_entries(database_object_entry_t *in_table_ptr,
                                                                  database_table_size_t size, 
                                                                  fbe_u8_t *SN,
                                                                  database_object_entry_t **out_entry_ptr,
                                                                  fbe_u32_t *out_num_drive_entries);

fbe_status_t fbe_database_config_table_get_num_pvds(database_object_entry_t *in_table_ptr,
                                                    database_table_size_t size, 
                                                    fbe_u32_t *out_num_pvds);

fbe_status_t fbe_database_config_table_get_user_entry_by_object_id(database_table_t *in_table_ptr,
                                                              fbe_object_id_t object_id,
                                                              database_user_entry_t **out_entry_ptr);

fbe_status_t fbe_database_config_table_get_user_entry_by_rg_id(database_table_t *in_table_ptr, 
                                                               fbe_raid_group_number_t raid_group_number,
                                                               database_user_entry_t **out_entry_ptr);

fbe_status_t fbe_database_config_table_get_user_entry_by_lun_id(database_table_t *in_table_ptr, 
                                                               fbe_lun_number_t lun_number,
                                                               database_user_entry_t **out_entry_ptr);
fbe_status_t fbe_database_config_table_get_user_entry_by_ext_pool_id(database_table_t *in_table_ptr, 
                                                                     fbe_u32_t pool_id,
                                                                     database_user_entry_t **out_entry_ptr);
fbe_status_t fbe_database_config_table_get_user_entry_by_ext_pool_lun_id(database_table_t *in_table_ptr, 
                                                                         fbe_u32_t pool_number,
                                                                         fbe_lun_number_t lun_number,
                                                                         database_user_entry_t **out_entry_ptr);

fbe_status_t fbe_database_config_table_get_edge_entry(database_table_t *in_table_ptr,
                                                      fbe_object_id_t object_id,
                                                      fbe_edge_index_t client_index,
                                                      database_edge_entry_t **out_entry_ptr);

fbe_status_t fbe_database_config_table_update_user_entry(database_table_t *in_table_ptr,
                                                         database_user_entry_t *in_entry_ptr);

fbe_status_t fbe_database_config_table_remove_user_entry(database_table_t *in_table_ptr, 
														 database_user_entry_t *in_entry_ptr);

fbe_status_t fbe_database_config_table_update_object_entry(database_table_t *in_table_ptr,
                                                         database_object_entry_t *in_entry_ptr);

fbe_status_t fbe_database_config_table_remove_object_entry(database_table_t *in_table_ptr, 
														   database_object_entry_t *in_entry_ptr);

fbe_status_t fbe_database_config_table_remove_edge_entry(database_table_t *in_table_ptr, 
														 database_edge_entry_t *in_entry_ptr);

fbe_status_t fbe_database_config_table_update_edge_entry(database_table_t *in_table_ptr,
                                                         database_edge_entry_t *in_entry_ptr);

fbe_status_t fbe_database_config_table_copy_edge_entries(database_table_t *in_table_ptr,fbe_object_id_t object_id,
														 database_transaction_t *transaction);

fbe_status_t fbe_database_config_table_get_global_info_entry(database_table_t *in_table_ptr, 
                                                             database_global_info_type_t type, 
                                                             database_global_info_entry_t **out_entry_ptr);

fbe_status_t fbe_database_config_table_remove_global_info_entry(database_table_t *in_table_ptr, 
                                                                database_global_info_entry_t *in_entry_ptr);

fbe_status_t fbe_database_config_table_update_global_info_entry(database_table_t *in_table_ptr, 
                                                                database_global_info_entry_t *in_entry_ptr);

fbe_status_t fbe_database_config_table_get_system_spare_entry(database_table_t *in_table_ptr, 
                                                              fbe_object_id_t object_id, 
                                                              database_system_spare_entry_t **out_entry_ptr);
fbe_status_t fbe_database_config_table_update_system_spare_entry(database_table_t *in_table_ptr, 
                                                                database_system_spare_entry_t *in_entry_ptr);

fbe_status_t fbe_database_config_table_get_count_by_db_class(database_table_t *in_table_ptr,
															 database_class_id_t class_id,
                                                             fbe_u32_t *count);

fbe_status_t fbe_database_config_destroy_all_objects_of_class(database_class_id_t class_id,
															  fbe_database_service_t *database_service_ptr,
                                                              fbe_bool_t internal_obj_only);

fbe_status_t fbe_database_config_table_get_non_reserved_free_object_id(database_object_entry_t *in_object_table_ptr, 
                                                             database_table_size_t size, 
                                                             fbe_object_id_t *out_object_id_p);


fbe_status_t fbe_database_config_table_get_user_entry_by_lun_wwn(database_table_t *in_table_ptr, 
																 fbe_assigned_wwid_t lun_wwid,
																 database_user_entry_t **out_entry_ptr);

fbe_bool_t fbe_database_config_is_global_info_commit_required(database_table_t *in_table_ptr);
fbe_bool_t fbe_database_config_is_object_table_commit_required(database_table_t *in_table_ptr);

fbe_status_t fbe_database_config_update_global_info_prior_commit(database_table_t *in_table_ptr);
fbe_status_t fbe_database_config_update_object_table_prior_commit(database_table_t *in_table_ptr);

fbe_bool_t fbe_database_config_is_global_info_out_of_sync(database_table_t *in_table_ptr);

/***********************************************
 * end file fbe_database_config_tables.h
 ***********************************************/

#endif /* end FBE_DATABASE_CONFIG_TABLES_H */




