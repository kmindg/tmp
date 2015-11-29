#ifndef FBE_RAID_GROUP_DEBUG_H
#define FBE_RAID_GROUP_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_raid_group_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the raid group debug library.
 *
 * @author
 *  12/3/2009 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"
#include "fbe_database.h"

/*! @struct fbe_raid_group_database_info_t
 *  
 * @brief RG object information structure. This structure holds information related to RG object.
 */
typedef struct fbe_raid_group_database_info_s
{
    fbe_bool_t system;
    fbe_raid_group_number_t rg_number;
    fbe_lba_t capacity;
}fbe_raid_group_database_info_t;


fbe_status_t fbe_raid_group_debug_trace(const fbe_u8_t * module_name,
                                        fbe_dbgext_ptr raid_group_p,
                                        fbe_trace_func_t trace_func,
                                        fbe_trace_context_t trace_context,
                                        fbe_debug_field_info_t *field_info_p,
                                        fbe_u32_t spaces_to_indent);
fbe_status_t fbe_raid_group_debug_display_terminator_queue(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr raid_group_p,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_u32_t spaces_to_indent,
                                                           fbe_bool_t b_summarize);
fbe_status_t fbe_raid_group_non_paged_record_debug_trace(const fbe_u8_t * module_name,
                                                         fbe_dbgext_ptr record_ptr,
                                                         fbe_trace_func_t trace_func,
                                                         fbe_trace_context_t trace_context,
                                                         fbe_u32_t spaces_to_indent);
fbe_status_t fbe_raid_group_metadata_memory_debug_trace(const fbe_u8_t * module_name,
                                                        fbe_dbgext_ptr record_ptr,
                                                        fbe_trace_func_t trace_func,
                                                        fbe_trace_context_t trace_context,
                                                        fbe_u32_t spaces_to_indent);
fbe_status_t fbe_metadata_operation_rg_paged_metadata_debug_trace(const fbe_u8_t * module_name,
                                                                  fbe_dbgext_ptr paged_metadata_ptr,
                                                                  fbe_trace_func_t trace_func,
                                                                  fbe_trace_context_t trace_context,
                                                                  fbe_u32_t spaces_to_indent);
fbe_status_t fbe_sepls_rg_info_trace(const fbe_u8_t* module_name,
                                     fbe_dbgext_ptr control_handle_ptr,
                                     fbe_bool_t display_all_flag,
                                     fbe_u32_t display_format,
                                     fbe_trace_func_t trace_func,
                                     fbe_trace_context_t trace_context);
fbe_status_t fbe_sepls_get_rg_persist_data(const fbe_u8_t* module_name,
                                           fbe_dbgext_ptr rg_object_ptr,
                                           fbe_dbgext_ptr rg_user_ptr,
                                           fbe_raid_group_database_info_t *rg_data_ptr,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context);
fbe_status_t fbe_sepls_rg1_0_info_trace(const fbe_u8_t * module_name,
                                       fbe_object_id_t mirror_id,
                                        fbe_lba_t lun_capacity,
                                        fbe_raid_group_number_t rg_number,
                                        fbe_u32_t display_format,
                                       fbe_trace_func_t trace_func,
                                       fbe_trace_context_t trace_context);

fbe_status_t fbe_sepls_get_rg_rebuild_info(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr rg_base_config_ptr,
                                           void *rebuild_info_p);

fbe_status_t fbe_sepls_get_rg_encryption_info(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr rg_base_config_ptr,
                                              void *encr_info_p);

fbe_status_t fbe_raid_group_dump_debug_trace(const fbe_u8_t * module_name,
                                        fbe_dbgext_ptr raid_group_p,
                                        fbe_trace_func_t trace_func,
                                        fbe_trace_context_t trace_context,
                                        fbe_debug_field_info_t *field_info_p,
                                        fbe_u32_t overall_spaces_to_indent);

fbe_status_t fbe_raid_group_metadata_memory_dump_debug_trace(const fbe_u8_t * module_name,
                                                        fbe_dbgext_ptr record_ptr,
                                                        fbe_trace_func_t trace_func,
                                                        fbe_trace_context_t trace_context,
                                                        fbe_u32_t spaces_to_indent);

fbe_status_t fbe_raid_group_non_paged_record_dump_debug_trace(const fbe_u8_t * module_name,
                                            fbe_dbgext_ptr record_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_u32_t spaces_to_indent);


#endif /* FBE_RAID_GROUP_DEBUG_H */

/*************************
 * end file fbe_raid_group_debug.h
 *************************/
