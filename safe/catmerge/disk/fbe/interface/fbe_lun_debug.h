#ifndef FBE_LUN_DEBUG_H
#define FBE_LUN_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_lun_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the lun debug library.
 *
 * @author
 *  12/02/2010 - Created. Omer Miranda
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"

/*! @struct fbe_lun_database_info_t
*  
* @brief Lun object information structure. This structure holds information related to LUN object.
*/
typedef struct fbe_lun_database_info_s
{
    fbe_lun_number_t lun_number;
    fbe_lba_t capacity;
}fbe_lun_database_info_t;

fbe_status_t fbe_lun_debug_trace(const fbe_u8_t * module_name,
                                 fbe_dbgext_ptr lun_object_ptr,
                                 fbe_trace_func_t trace_func,
                                 fbe_trace_context_t trace_context,
                                 fbe_u32_t overall_spaces_to_indent);

fbe_status_t fbe_lun_debug_display_terminator_queue(const fbe_u8_t * module_name,
                                                    fbe_dbgext_ptr lun_object_p,
                                                    fbe_trace_func_t trace_func,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_u32_t spaces_to_indent);
fbe_status_t fbe_lun_non_paged_record_debug_trace(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr record_ptr,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_u32_t spaces_to_indent);
fbe_status_t fbe_lun_object_metadata_memory_debug_trace(const fbe_u8_t * module_name,
                                                        fbe_dbgext_ptr record_ptr,
                                                        fbe_trace_func_t trace_func,
                                                        fbe_trace_context_t trace_context,
                                                        fbe_u32_t spaces_to_indent);
fbe_status_t fbe_lun_paged_record_metadata_debug_trace(const fbe_u8_t * module_name,
                                                       fbe_dbgext_ptr object_ptr,
                                                       fbe_trace_func_t trace_func,
                                                       fbe_trace_context_t trace_context,
                                                       fbe_u32_t spaces_to_indent);
fbe_lba_t fbe_sepls_lun_info_trace(const fbe_u8_t* module_name,
                                   fbe_object_id_t lun_id,
                                   fbe_object_id_t rg_id,
                                   fbe_block_size_t exported_block_size,
								   fbe_u32_t display_format,
                                   fbe_trace_func_t fbe_debug_trace_func,
                                   fbe_trace_context_t trace_context);
fbe_status_t fbe_sepls_get_lun_persist_data(const fbe_u8_t* module_name,
                                            fbe_dbgext_ptr lun_object_ptr,
											fbe_dbgext_ptr lun_user_ptr,
                                            fbe_lun_database_info_t *lun_data_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context);

fbe_status_t fbe_lun_dump_debug_trace(const fbe_u8_t * module_name,
                                 fbe_dbgext_ptr lun_object_ptr,
                                 fbe_trace_func_t trace_func,
                                 fbe_trace_context_t trace_context,
                                 fbe_u32_t overall_spaces_to_indent);

fbe_status_t fbe_lun_non_paged_record_dump_debug_trace(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr record_ptr,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_u32_t spaces_to_indent);

fbe_status_t fbe_lun_object_metadata_memory_dump_debug_trace(const fbe_u8_t * module_name,
                                                        fbe_dbgext_ptr record_ptr,
                                                        fbe_trace_func_t trace_func,
                                                        fbe_trace_context_t trace_context,
                                                        fbe_u32_t spaces_to_indent);


#endif /* FBE_LUN_DEBUG_H */

/*************************
 * end file fbe_lun_debug.h
 *************************/
