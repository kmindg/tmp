
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_lun_dump_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the lun object.
 *
 * @author
 *  8/15/2012 - Created. Jingcheng Zhang
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_base_object.h"
#include "fbe_base_object_debug.h"
#include "fbe_lun_private.h"
#include "fbe_base_object_trace.h"
#include "fbe_lun_debug.h"
#include "fbe_transport_debug.h"
#include "fbe/fbe_raid_group.h"
#include "fbe_raid_geometry.h"
#include "fbe_base_config_debug.h"
#include "fbe_raid_library_debug.h"

#include "fbe_topology_debug.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!*******************************************************************
 * @var fbe_lun_object_metadata_memory_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of lun object
 *        metadata memeory.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_lun_object_metadata_memory_dump_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_config_metadata_memory", fbe_base_config_metadata_memory_t, FBE_TRUE, "0x%x",
                                    fbe_base_config_metadata_memory_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_lun_object_metadata_memory_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        lun object metadata memory.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_lun_metadata_memory_t,
                              fbe_lun_object_metadata_memory_dump_struct_info,
                              &fbe_lun_object_metadata_memory_dump_field_info[0]);

/*!**************************************************************
 * fbe_lun_object_metadata_memory_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display the lun object metadata memeory.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param object_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  8/15/2012 - Created Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_lun_object_metadata_memory_dump_debug_trace(const fbe_u8_t * module_name,
                                                        fbe_dbgext_ptr record_ptr,
                                                        fbe_trace_func_t trace_func,
                                                        fbe_trace_context_t trace_context,
                                                        fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dump_debug_trace_prefix_t new_prefix;

    fbe_debug_dump_generate_prefix(&new_prefix, 
                                  (fbe_dump_debug_trace_prefix_t *)trace_context,
                                  "lun_metadata_memory");

    status = fbe_debug_display_structure(module_name, trace_func, &new_prefix, record_ptr,
                                         &fbe_lun_object_metadata_memory_dump_struct_info,
                                         1 /* fields per line */,
                                         0);
    return status;
}

/*!*******************************************************************
 * @var fbe_lun_nonpaged_metadata_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of lun object
 *        non-paged metadata record.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_lun_nonpaged_metadata_dump_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_config_nonpaged_metadata", fbe_base_config_nonpaged_metadata_t, FBE_TRUE, "0x%x", 
                                    fbe_base_config_nonpaged_metadata_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO("zero_checkpoint", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("has_been_written", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_lun_nonpaged_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_lun_nonpaged_metadata_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        lun object non paged metadata record.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_lun_nonpaged_metadata_t,
                              fbe_lun_nonpaged_metadata_dump_struct_info,
                              &fbe_lun_nonpaged_metadata_dump_field_info[0]);

/*!**************************************************************
 * fbe_lun_non_paged_record_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display the lun object non-paged record.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param object_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  8/15/2012 - Created Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_lun_non_paged_record_dump_debug_trace(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr record_ptr,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dump_debug_trace_prefix_t new_prefix;

    fbe_debug_dump_generate_prefix(&new_prefix, (fbe_dump_debug_trace_prefix_t *)trace_context,
                                   "lun_non_paged_metadata");

    status = fbe_debug_display_structure(module_name, trace_func, &new_prefix, record_ptr,
                                         &fbe_lun_nonpaged_metadata_dump_struct_info,
                                         1 /* fields per line */,
                                         0);
    return status;
}

/*!*******************************************************************
 * @var fbe_lun_object_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_lun_object_dump_field_info[] =
{
    /* base config is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("dirty_pending", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_lun_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("local_state", fbe_lun_local_state_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("marked_dirty", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("dirty_flags_start_lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("lun_attributes", fbe_lun_attributes_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("lu_has_been_written", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("noinitialverify_b", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("clear_need_zero_b", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("write_bypass_mode", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("last_rebuild_percent", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("last_zero_checkpoint", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("clean_time", fbe_time_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("start_time", fbe_time_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("clean_pending", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("power_save_io_drain_delay_in_sec", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("max_lun_latency_time_is_sec", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("wait_for_power_save", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("io_counter", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("ndb_b", fbe_bool_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_config", fbe_object_id_t, FBE_TRUE, "0x%x",
                                    fbe_base_config_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_lun_object_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_lun_t,
                              fbe_lun_object_dump_struct_info,
                              &fbe_lun_object_dump_field_info[0]);

/*!**************************************************************
 * @fn fbe_lun_dump_debug_trace(const fbe_u8_t * module_name,
 *                                   fbe_dbgext_ptr lun_object_ptr,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display all the trace information about the lun object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param logical_drive_p - Ptr to lun object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  8/15/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_lun_dump_debug_trace(const fbe_u8_t * module_name,
                                 fbe_dbgext_ptr lun_object_ptr,
                                 fbe_trace_func_t trace_func,
                                 fbe_trace_context_t trace_context,
                                 fbe_u32_t overall_spaces_to_indent)
{
    fbe_status_t status;


    status = fbe_debug_display_structure(module_name, trace_func, trace_context, lun_object_ptr,
                                         &fbe_lun_object_dump_struct_info,
                                         1 /* fields per line */,
                                         0);
    trace_func(NULL, "\n");

    return status;
}
/**************************************
 * end fbe_lun_dump_debug_trace()
 **************************************/


/*************************
 * end file fbe_lun_dump_debug.c
 *************************/

