#ifndef SEPLS_H
#define SEPLS_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_sepls_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces for sepls macro.
 *
 * @author 
 *  10-May-2011:  Created. Omer Miranda
 *
 ***************************************************************************/

#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"


/*!*******************************************************************
 * @def FBE_SEPLS_DISPLAY_FORMAT_MASK
 *********************************************************************
 * @brief Mask to extract the display format
 *
 *********************************************************************/
#define FBE_SEPLS_DISPLAY_FORMAT_MASK 0xffff

/*!********************************************************************* 
 * @enum fbe_sepls_display_level_e
 *  
 * @brief 
 * This enumeration lists the sepls display level.
 *
 **********************************************************************/
typedef enum fbe_sepls_display_level_e
{
    FBE_SEPLS_DISPLAY_LEVEL_1 = 0xff01,
    FBE_SEPLS_DISPLAY_LEVEL_2 = 0xff02,
    FBE_SEPLS_DISPLAY_LEVEL_3 = 0xff04,
    FBE_SEPLS_DISPLAY_LEVEL_4 = 0xff08,
} fbe_sepls_display_level_t;

/*!********************************************************************* 
 * @enum fbe_sepls_display_object_e
 *  
 * @brief 
 * This enumeration lists the sepls display object.
 *
 **********************************************************************/
typedef enum fbe_sepls_display_object_e
{
    FBE_SEPLS_DISPLAY_OBJECTS_NONE = 0x0000,
    FBE_SEPLS_DISPLAY_ONLY_LUN_OBJECTS = 0x0001,
    FBE_SEPLS_DISPLAY_ONLY_RG_OBJECTS = 0x0002,
    FBE_SEPLS_DISPLAY_ONLY_VD_OBJECTS = 0x0004,
    FBE_SEPLS_DISPLAY_ONLY_PVD_OBJECTS = 0x0008,
} fbe_sepls_display_object_t;

typedef struct fbe_sep_offsets_s {
    fbe_bool_t is_fbe_sep_offsets_set;
    
    fbe_u32_t control_handle_offset;                    /* topology_object_table_entry_t */
    fbe_u32_t object_status_offset;                     /* topology_object_table_entry_t */
    
    fbe_u32_t class_id_offset;                          /* fbe_base_object_s */
    fbe_u32_t object_id_offset;                         /* fbe_base_object_s */

    fbe_u32_t db_service_object_table_offset;           /* fbe_database_service_t */
    fbe_u32_t db_service_user_table_offset;             /* fbe_database_service_t */
    fbe_u32_t db_service_edge_table_offset;             /* fbe_database_service_t */
    fbe_u32_t db_service_global_info_offset;            /* fbe_database_service_t */

    fbe_u32_t db_obj_entry_class_id_offset;             /* database_object_entry_t */
    fbe_u32_t db_obj_entry_set_config_union_offset;     /* database_object_entry_t */
    fbe_u32_t db_obj_entry_object_header_offset;        /* database_object_entry_t */

    fbe_u32_t db_table_content_offset;                  /* database_table_t */
    fbe_u32_t db_table_size_offset;                     /* database_table_t */

    fbe_u32_t db_table_header_object_id_offset;         /* database_table_header_t */
    fbe_u32_t db_table_header_state_offset;             /* database_table_header_t */
    fbe_u32_t db_table_header_entry_id_offset;          /* database_table_header_t */
    
    fbe_u32_t db_user_entry_user_data_union_offset;     /* database_user_entry_t */
    fbe_u32_t db_user_entry_header_offset;              /* database_user_entry_t */
    fbe_u32_t db_user_entry_db_class_id_offset;         /* database_user_entry_t */
    
    fbe_u32_t db_edge_entry_header_offset;              /* database_edge_entry_t */
    fbe_u32_t db_edge_entry_server_id_offset;           /* database_edge_entry_t */
    fbe_u32_t db_edge_entry_client_index_offset;        /* database_edge_entry_t */
    fbe_u32_t db_edge_entry_capacity_offset;            /* database_edge_entry_t */
    fbe_u32_t db_edge_entry_offset_offset;              /* database_edge_entry_t */
} fbe_sep_offsets_t;

/* saved offsets */
extern fbe_sep_offsets_t fbe_sep_offsets;

void fbe_sep_offsets_clear(void);
void fbe_sep_offsets_set(const fbe_u8_t * module_name);

#define FBE_SEPLS_MAX_OFFSET    0xFFFFFFFF

void fbe_sepls_set_sys_vd_id(fbe_object_id_t);
void fbe_sepls_display_sys_vds(fbe_bool_t, fbe_u32_t, fbe_u32_t);

fbe_status_t fbe_sepls_get_lifecycle_state_name(fbe_lifecycle_state_t state, const char ** state_name);
fbe_status_t fbe_sepls_get_config_data_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_object_id_t object_id,
                                                   void *object_data_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context);
fbe_status_t fbe_sepls_get_persist_data_debug_trace(const fbe_u8_t *module_name,
                                                    fbe_trace_func_t trace_func,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_dbgext_ptr object_ptr,
                                                    fbe_dbgext_ptr user_ptr,
                                                    void *object_data_ptr);
fbe_status_t fbe_sepls_get_drives_info_debug_trace(const fbe_u8_t* module_name,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_object_id_t in_object_id);
fbe_object_id_t fbe_sepls_get_downstream_object_debug_trace(const fbe_u8_t* module_name,
                                                            fbe_dbgext_ptr object_ptr,
                                                            fbe_trace_func_t trace_func,
                                                            fbe_trace_context_t trace_context);
fbe_status_t fbe_sepls_display_object_state_debug_trace(fbe_trace_func_t trace_func,
                                                        fbe_trace_context_t trace_context,
                                                        fbe_lifecycle_state_t state);


#endif /* SEPLS_H */
/*************************
 * end file fbe_sepls_debug.h
 *************************/
