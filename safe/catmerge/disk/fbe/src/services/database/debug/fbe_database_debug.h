#ifndef FBE_DATABASE_DEBUG_H
#define FBE_DATABASE_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_database_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces for database service macro.
 *
 * @author 
 *  3-Aug-2011:  Created. Omer Miranda
 *
 ***************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"


/*!********************************************************************* 
 * @enum fbe_database_display_table_e
 *  
 * @brief 
 * This enumeration lists the database table to be displayed 
 *
 **********************************************************************/
typedef enum fbe_database_display_table_e
{
    FBE_DATABASE_DISPLAY_TABLE_NONE = 0x0000,
    FBE_DATABASE_DISPLAY_OBJECT_TABLE = 0x0001,
    FBE_DATABASE_DISPLAY_EDGE_TABLE = 0x0002,
    FBE_DATABASE_DISPLAY_USER_TABLE = 0x0004,
    FBE_DATABASE_DISPLAY_GLOBAL_INFO_TABLE = 0x0008,
} fbe_database_display_table_t;


fbe_status_t fbe_database_service_debug_trace(const fbe_u8_t * module_name,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_dbgext_ptr database_service_ptr,
                                              fbe_u32_t display_format,
                                              fbe_object_id_t object_id);

/* Edge table */
fbe_status_t fbe_database_display_edge_table(const fbe_u8_t * module_name,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_dbgext_ptr edge_table_ptr,
                                             fbe_object_id_t in_object_id);
/* Global Info table */
fbe_status_t fbe_database_display_global_info_table(const fbe_u8_t * module_name,
                                                    fbe_trace_func_t trace_func,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_dbgext_ptr global_info_ptr,
                                                    fbe_object_id_t in_object_id);
fbe_status_t fbe_global_info_power_saving_debug_trace(const fbe_u8_t * module_name,
                                                      fbe_trace_func_t trace_func,
                                                      fbe_trace_context_t trace_context,
                                                      fbe_dbgext_ptr power_saving_info_ptr);
fbe_status_t fbe_global_info_spare_config_debug_trace(const fbe_u8_t * module_name,
                                                      fbe_trace_func_t trace_func,
                                                      fbe_trace_context_t trace_context,
                                                      fbe_dbgext_ptr spare_config_info_ptr);
fbe_status_t fbe_global_info_system_generation_debug_trace(const fbe_u8_t * module_name,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_dbgext_ptr system_generation_info_ptr);
/* Object table */
fbe_status_t fbe_database_display_object_table(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr object_table_ptr,
                                               fbe_object_id_t in_object_id);
fbe_status_t fbe_pvd_object_config_debug_trace(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr pvd_object_ptr);
fbe_status_t fbe_lun_object_config_debug_trace(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr lun_object_ptr);
fbe_status_t fbe_vd_object_config_debug_trace(const fbe_u8_t * module_name,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_dbgext_ptr vd_object_ptr);
fbe_status_t fbe_rg_object_config_debug_trace(const fbe_u8_t * module_name,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_dbgext_ptr rg_object_ptr);
fbe_status_t fbe_ext_pool_object_config_debug_trace(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr ext_pool_object_ptr);
fbe_status_t fbe_ext_pool_lun_object_config_debug_trace(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr ext_pool_lun_object_ptr);
/* User table */
fbe_status_t fbe_database_display_user_table(const fbe_u8_t * module_name,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_dbgext_ptr user_table_ptr,
                                             fbe_object_id_t in_object_id);
fbe_status_t fbe_pvd_user_config_debug_trace(const fbe_u8_t * module_name,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_dbgext_ptr pvd_user_ptr);
fbe_status_t fbe_lun_user_config_debug_trace(const fbe_u8_t * module_name,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_dbgext_ptr lun_user_ptr);
fbe_status_t fbe_rg_user_config_debug_trace(const fbe_u8_t * module_name,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_dbgext_ptr rg_user_ptr);
fbe_status_t fbe_ext_pool_user_config_debug_trace(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr ext_pool_user_ptr);
fbe_status_t fbe_ext_pool_lun_user_config_debug_trace(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr ext_pool_lun_user_ptr);

fbe_status_t fbe_database_service_debug_dump_all(const fbe_u8_t * module_name,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context);


fbe_status_t fbe_database_service_debug_dump(const fbe_u8_t * module_name,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_dbgext_ptr database_service_ptr,
                                              fbe_u32_t display_format,
                                              fbe_object_id_t object_id);

/* Edge table */
fbe_status_t fbe_database_dump_edge_table(const fbe_u8_t * module_name,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_dbgext_ptr edge_table_ptr,
                                             fbe_object_id_t in_object_id);
/* Global Info table */
fbe_status_t fbe_database_dump_global_info_table(const fbe_u8_t * module_name,
                                                    fbe_trace_func_t trace_func,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_dbgext_ptr global_info_ptr,
                                                    fbe_object_id_t in_object_id);
fbe_status_t fbe_global_info_power_saving_debug_dump(const fbe_u8_t * module_name,
                                                      fbe_trace_func_t trace_func,
                                                      fbe_trace_context_t trace_context,
                                                      fbe_dbgext_ptr power_saving_info_ptr);
fbe_status_t fbe_global_info_spare_config_debug_dump(const fbe_u8_t * module_name,
                                                      fbe_trace_func_t trace_func,
                                                      fbe_trace_context_t trace_context,
                                                      fbe_dbgext_ptr spare_config_info_ptr);
fbe_status_t fbe_global_info_system_generation_debug_dump(const fbe_u8_t * module_name,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_dbgext_ptr system_generation_info_ptr);
fbe_status_t fbe_global_info_system_time_threshold_debug_dump(const fbe_u8_t * module_name,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_dbgext_ptr system_time_threshold_info_ptr);
/* Object table */
fbe_status_t fbe_database_dump_object_table(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr object_table_ptr,
                                               fbe_object_id_t in_object_id);
fbe_status_t fbe_pvd_object_config_debug_dump(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr pvd_object_ptr);
fbe_status_t fbe_lun_object_config_debug_dump(const fbe_u8_t * module_name,
                                               fbe_trace_func_t trace_func,
                                               fbe_trace_context_t trace_context,
                                               fbe_dbgext_ptr lun_object_ptr);
fbe_status_t fbe_vd_object_config_debug_dump(const fbe_u8_t * module_name,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_dbgext_ptr vd_object_ptr);
fbe_status_t fbe_rg_object_config_debug_dump(const fbe_u8_t * module_name,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_dbgext_ptr rg_object_ptr);
/* User table */
fbe_status_t fbe_database_dump_user_table(const fbe_u8_t * module_name,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_dbgext_ptr user_table_ptr,
                                             fbe_object_id_t in_object_id);
fbe_status_t fbe_pvd_user_config_debug_dump(const fbe_u8_t * module_name,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_dbgext_ptr pvd_user_ptr);
fbe_status_t fbe_lun_user_config_debug_dump(const fbe_u8_t * module_name,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_dbgext_ptr lun_user_ptr);
fbe_status_t fbe_rg_user_config_debug_dump(const fbe_u8_t * module_name,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_dbgext_ptr rg_user_ptr);
fbe_status_t fbe_system_db_header_debug_dump(const fbe_u8_t * module_name,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_dbgext_ptr database_service_ptr);

fbe_char_t *
fbe_database_get_lun_type_string(fbe_u32_t type);

void fbe_database_get_lun_config_debug(fbe_object_id_t lun_obj_id,
					                    fbe_lun_number_t * lun_number,
                                        fbe_assigned_wwid_t *wwn,
                                        fbe_u32_t *type,
                                        fbe_user_defined_lun_name_t *name,
                                        fbe_bool_t *status,
                                        fbe_bool_t *is_exists);

fbe_u32_t fbe_database_get_max_number_of_lun(void);

fbe_u32_t fbe_database_get_total_object_count(void);

fbe_bool_t fbe_database_is_object_id_lun(fbe_u32_t object_id);

void fbe_database_get_all_lun_object_ids(fbe_u32_t *buffer, fbe_u32_t size, fbe_u32_t *object_count);

fbe_bool_t fbe_database_is_object_id_user_lun(fbe_u32_t object_id);

void fbe_database_get_all_user_lun_object_ids(fbe_u32_t *buffer, fbe_u32_t size, fbe_u32_t *object_count);



#ifdef __cplusplus
};
#endif

#endif /* FBE_DATABASE_DEBUG_H */
/*************************
 * end file fbe_database_debug.h
 *************************/
