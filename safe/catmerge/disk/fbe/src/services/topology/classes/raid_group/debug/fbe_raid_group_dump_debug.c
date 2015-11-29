
/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_raid_group_dump_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the raid group object.
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
#include "fbe/fbe_topology_interface.h"
#include "fbe_base_object.h"
#include "fbe_base_object_debug.h"
#include "fbe_raid_group_object.h"
#include "fbe_base_object_trace.h"
#include "fbe_transport_debug.h"
#include "fbe_raid_library_debug.h"
#include "fbe_base_config_debug.h"
#include "fbe_raid_group_debug.h"
#include "fbe/fbe_payload_metadata_operation.h"
#include "fbe_metadata_debug.h"
#include "fbe_topology_debug.h"


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * fbe_raid_group_paged_metadata_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display the raid group paged metadata information.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param struct_ptr - This is the ptr to the structure.
 * @param field_info_p - Field to init. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  8/21/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_paged_metadata_dump_debug_trace(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr base_ptr,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_debug_field_info_t *field_info_p,
                                              fbe_u32_t spaces_to_indent)
{
    fbe_u32_t loop_counter = 0;
    fbe_raid_group_paged_metadata_t paged_metadata_info[FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS] = {0};
    fbe_u32_t paged_metadata_offset;

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_nonpaged_metadata_t, "paged_metadata_metadata", &paged_metadata_offset);
    FBE_READ_MEMORY(base_ptr + paged_metadata_offset,&paged_metadata_info, (sizeof(fbe_raid_group_paged_metadata_t)*FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS));

    for(loop_counter = 0; loop_counter < FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS; ++loop_counter)
    {
        trace_func(trace_context, "valid_bit[%d]: 0x%x\n", loop_counter, paged_metadata_info[loop_counter].valid_bit);	
        trace_func(trace_context, "verify_bits[%d]: 0x%x\n", loop_counter, paged_metadata_info[loop_counter].verify_bits);	
		trace_func(trace_context, "needs_rebuild_bits[%d]: 0x%x\n", loop_counter,paged_metadata_info[loop_counter].needs_rebuild_bits);
		trace_func(trace_context, "reserved_bits[%d]: 0x%x\n", loop_counter,paged_metadata_info[loop_counter].reserved_bits);	
    }

    return FBE_STATUS_OK;
}


/*!*******************************************************************
 * @var fbe_raid_group_rebuild_nonpaged_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config
 *        non-paged metadata.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_raid_group_rebuild_nonpaged_dump_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("rebuild_logging_bitmask", fbe_raid_position_bitmask_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};


/*!*******************************************************************
 * @var fbe_raid_group_rebuild_nonpaged_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        generation number.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_raid_group_rebuild_nonpaged_info_t,
                              fbe_raid_group_rebuild_nonpaged_dump_struct_info,
                              &fbe_raid_group_rebuild_nonpaged_dump_field_info[0]);

/*!**************************************************************
 * fbe_rebuild_nonpaged_info_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display the raid group rebuild and NR non paged information.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param struct_ptr - This is the ptr to the structure.
 * @param field_info_p - Field to init. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  27/01/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_rebuild_nonpaged_info_dump_debug_trace(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr base_ptr,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_debug_field_info_t *field_info_p,
                                              fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t loop_counter = 0;
    fbe_raid_group_rebuild_nonpaged_info_t debug_rebuild_info = {0};
    fbe_u32_t rebuild_info_offset;
    fbe_dump_debug_trace_prefix_t new_prefix;

    fbe_debug_dump_generate_prefix(&new_prefix, (fbe_dump_debug_trace_prefix_t *)trace_context,
                                   field_info_p->name);


    status = fbe_debug_display_structure(module_name, trace_func, &new_prefix, base_ptr + field_info_p->offset,
                                         &fbe_raid_group_rebuild_nonpaged_dump_struct_info,
                                         1 /* fields per line */,
                                         0);

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_nonpaged_metadata_t, "rebuild_info", &rebuild_info_offset);
    FBE_READ_MEMORY(base_ptr + rebuild_info_offset,&debug_rebuild_info, sizeof(fbe_raid_group_rebuild_nonpaged_info_t));
    trace_func(NULL, "\n");
    for(loop_counter = 0; loop_counter < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; ++loop_counter)
    {
        trace_func(&new_prefix, "rebuild_checkpoint[%d]: 0x%llx\n", loop_counter,(unsigned long long)debug_rebuild_info.rebuild_checkpoint_info[loop_counter].checkpoint);
		trace_func(&new_prefix, "rebuild_position[%d]: 0x%x\n", loop_counter,debug_rebuild_info.rebuild_checkpoint_info[loop_counter].position);	
    }

    return FBE_STATUS_OK;
}

/*!*******************************************************************
 * @var fbe_raid_group_nonpaged_metadata_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of raid group
 *        non-paged metadata record.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_raid_group_nonpaged_metadata_dump_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_config_nonpaged_metadata", fbe_base_config_nonpaged_metadata_t, FBE_TRUE, "0x%x", 
                                    fbe_base_config_nonpaged_metadata_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("rebuild_info", fbe_raid_group_rebuild_nonpaged_info_t, FBE_FALSE, "0x%x", 
                                    fbe_rebuild_nonpaged_info_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO("ro_verify_checkpoint", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("rw_verify_checkpoint", fbe_lba_t, FBE_FALSE, "0x%x"),

    FBE_DEBUG_DECLARE_FIELD_INFO("error_verify_checkpoint", fbe_lba_t, FBE_FALSE, "0x%x"), 
    FBE_DEBUG_DECLARE_FIELD_INFO("journal_verify_checkpoint", fbe_lba_t, FBE_FALSE, "0x%x"),  
    FBE_DEBUG_DECLARE_FIELD_INFO("incomplete_write_verify_checkpoint", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("system_verify_checkpoint", fbe_lba_t, FBE_FALSE, "0x%x"),    
    FBE_DEBUG_DECLARE_FIELD_INFO("raid_group_np_md_flags", fbe_u8_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("glitching_disks_bitmask", fbe_u16_t, FBE_FALSE, "0x%x"),   

    FBE_DEBUG_DECLARE_FIELD_INFO_FN("paged_metadata_metadata", fbe_raid_group_paged_metadata_t, FBE_FALSE, "0x%x", 
                                    fbe_raid_group_paged_metadata_dump_debug_trace),

    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};


/*!*******************************************************************
 * @var fbe_raid_group_nonpaged_metadata_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        raid group non paged metadata record.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_raid_group_nonpaged_metadata_t,
                              fbe_raid_group_nonpaged_metadata_dump_struct_info,
                              &fbe_raid_group_nonpaged_metadata_dump_field_info[0]);

/*!**************************************************************
 * fbe_raid_group_non_paged_record_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display the raid group non-paged record.
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
 *  08/15/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_non_paged_record_dump_debug_trace(const fbe_u8_t * module_name,
                                            fbe_dbgext_ptr record_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dump_debug_trace_prefix_t new_prefix;
    fbe_debug_dump_generate_prefix(&new_prefix, (fbe_dump_debug_trace_prefix_t *)trace_context,
                                   "raid_group_non_paged_metadata");

    status = fbe_debug_display_structure(module_name, trace_func, &new_prefix, record_ptr,
                                         &fbe_raid_group_nonpaged_metadata_dump_struct_info,
                                         1 /* fields per line */,
                                         0);
    return FBE_STATUS_OK;
}

/*!*******************************************************************
 * @var fbe_raid_group_metadata_memory_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of raid group
 *        metadata memeory.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_raid_group_metadata_memory_dump_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_config_metadata_memory", fbe_base_config_metadata_memory_t, FBE_TRUE, "0x%x",
                                    fbe_base_config_metadata_memory_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_raid_group_clustered_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("failed_drives_bitmap", fbe_raid_position_bitmask_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("failed_ios_pos_bitmap", fbe_raid_position_bitmask_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_raid_group_metadata_memory_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        raid group metadata memory.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_raid_group_metadata_memory_t,
                              fbe_raid_group_metadata_memory_dump_struct_info,
                              &fbe_raid_group_metadata_memory_dump_field_info[0]);

/*!**************************************************************
 * fbe_raid_group_metadata_memory_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display the raid group metadata memeory.
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
fbe_status_t fbe_raid_group_metadata_memory_dump_debug_trace(const fbe_u8_t * module_name,
                                                        fbe_dbgext_ptr record_ptr,
                                                        fbe_trace_func_t trace_func,
                                                        fbe_trace_context_t trace_context,
                                                        fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_dump_debug_trace_prefix_t new_prefix;

    fbe_debug_dump_generate_prefix(&new_prefix, 
                                  (fbe_dump_debug_trace_prefix_t *)trace_context,
                                  "raid_group_metadata_memory");

    status = fbe_debug_display_structure(module_name, trace_func, &new_prefix, record_ptr,
                                         &fbe_raid_group_metadata_memory_dump_struct_info,
                                         1,
                                         0);
    return status;
}


/*!*******************************************************************
 * @var fbe_debug_raid_geometry_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of raid geometry.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_debug_raid_geometry_dump_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("object_id", fbe_object_id_t, FBE_TRUE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("class_id", fbe_class_id_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("raid_type", fbe_raid_group_type_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("geometry_flags", fbe_raid_geometry_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("debug_flags", fbe_raid_library_debug_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("attribute_flags", fbe_raid_attribute_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("prefered_mirror_pos", fbe_mirror_prefered_position_t , FBE_FALSE, "%d"),


    FBE_DEBUG_DECLARE_FIELD_INFO_FN("generate_state", fbe_raid_common_state_t, FBE_FALSE, "0x%x",
                                    fbe_debug_display_function_ptr),
    FBE_DEBUG_DECLARE_FIELD_INFO("element_size", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("elements_per_parity", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("exported_block_size", fbe_block_size_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("imported_block_size", fbe_block_size_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("optimal_block_size", fbe_block_size_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("max_blocks_per_drive", fbe_block_count_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("block_edge_p", fbe_u32_t*, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("width", fbe_u32_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("configured_capacity", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("metadata_start_lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("metadata_capacity", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("metadata_copy_offset", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("metadata_element_p", fbe_u32_t*, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_debug_raid_geometry_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        raid geometry structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_raid_geometry_t, 
                              fbe_debug_raid_geometry_dump_struct_info,
                              &fbe_debug_raid_geometry_dump_field_info[0]);


/*!**************************************************************
 * fbe_raid_geometry_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display all the trace information about the raid group object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param geo_p - Ptr to raid group object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 * @param spaces_to_indent - Spaces to indent this display.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author
 *  8/17/2012 - Created Jingcheng Zhang
 ****************************************************************/

fbe_status_t fbe_raid_geometry_dump_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr base_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
                                           fbe_debug_field_info_t *field_info_p,
                                           fbe_u32_t spaces_to_indent)
{
    fbe_dump_debug_trace_prefix_t new_prefix;

    fbe_debug_dump_generate_prefix(&new_prefix, 
                                  (fbe_dump_debug_trace_prefix_t *)trace_context,
                                  field_info_p->name);

    fbe_debug_display_structure(module_name, trace_func, (fbe_trace_context_t) &new_prefix, base_p + field_info_p->offset,
                                &fbe_debug_raid_geometry_dump_struct_info,
                                1 ,
                                0);

    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_raid_group_rebuilt_pvds_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display the raid group rebuilt PVDs information.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param struct_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param struct_ptr - This is the ptr to the structure.
 * @param field_info_p - Field to init. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  8/21/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_rebuilt_pvds_dump_debug_trace(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr base_ptr,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_debug_field_info_t *field_info_p,
                                              fbe_u32_t spaces_to_indent)
{
    fbe_u32_t loop_counter = 0;
    fbe_object_id_t rebuilt_pvds_info[FBE_RAID_GROUP_MAX_REBUILD_POSITIONS];
    fbe_u32_t rebuilt_pvds_offset;

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_t, "rebuilt_pvds", &rebuilt_pvds_offset);
    FBE_READ_MEMORY(base_ptr + rebuilt_pvds_offset, &rebuilt_pvds_info, (sizeof(fbe_object_id_t)* FBE_RAID_GROUP_MAX_REBUILD_POSITIONS));

    for(loop_counter = 0; loop_counter < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; ++loop_counter)
    {
        trace_func(trace_context, "      rebuilt_pvds[%d]: 0x%x", loop_counter, rebuilt_pvds_info[loop_counter]);		
    }

    return FBE_STATUS_OK;
}


/*!*******************************************************************
 * @var fbe_raid_group_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_raid_group_dump_field_info[] =
{
    /* base config is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_raid_group_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("debug_flags", fbe_raid_group_debug_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("local_state", fbe_raid_group_local_state_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("download_request_bitmask", fbe_u16_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("download_granted_bitmask", fbe_u16_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("chunk_size", fbe_chunk_size_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("paged_metadata_verify_pass_count", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("last_checkpoint_time", fbe_time_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("previous_rebuild_percent", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("last_check_rg_broken_timestamp", fbe_time_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("emeh_current_mode", fbe_raid_emeh_mode_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO("emeh_reliability", fbe_raid_emeh_reliability_t, FBE_FALSE, "%d"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("rebuilt_pvds", fbe_object_id_t, FBE_FALSE, "0x%x", 
                                    fbe_raid_group_rebuilt_pvds_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("geo", fbe_raid_geometry_t, FBE_FALSE, "0x%x",
                                    fbe_raid_geometry_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_config", fbe_object_id_t, FBE_TRUE, "0x%x",
                                    fbe_base_config_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_raid_group_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_raid_group_t,
                              fbe_raid_group_dump_struct_info,
                              &fbe_raid_group_dump_field_info[0]);

/*!**************************************************************
 * fbe_raid_group_dump_debug_trace()
 ****************************************************************
 * @brief
 *  Display all the trace information about the raid group object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param raid_group_p - Ptr to raid group object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 * @param spaces_to_indent - Spaces to indent this display.
 *
 * @return - FBE_STATUS_OK  
 *
 ****************************************************************/

fbe_status_t fbe_raid_group_dump_debug_trace(const fbe_u8_t * module_name,
                                        fbe_dbgext_ptr raid_group_p,
                                        fbe_trace_func_t trace_func,
                                        fbe_trace_context_t trace_context,
                                        fbe_debug_field_info_t *field_info_p,
                                        fbe_u32_t overall_spaces_to_indent)
{
    fbe_status_t status;
    fbe_dump_debug_trace_prefix_t new_prefix;

    if (field_info_p != NULL)
    {
        /* If field info is provided, the ptr passed in is a ptr to the base structure. 
         * We need to add the offset to get to the actual raid group ptr.
         */
        raid_group_p += field_info_p->offset;
        fbe_debug_dump_generate_prefix(&new_prefix, 
                                       (fbe_dump_debug_trace_prefix_t *)trace_context,
                                       field_info_p->name);
        status = fbe_debug_display_structure(module_name, trace_func, (fbe_trace_context_t)&new_prefix, raid_group_p,
                                         &fbe_raid_group_dump_struct_info,
                                         1 /* fields per line */,
                                         0);
    } else {
        status = fbe_debug_display_structure(module_name, trace_func, trace_context, raid_group_p,
                                             &fbe_raid_group_dump_struct_info,
                                             1 /* fields per line */,
                                             0);
    }

    trace_func(NULL, "\n");

    return status;
} 
/**************************************
 * end fbe_raid_group_dump_debug_trace()
 **************************************/


/*************************
 * end file fbe_raid_group_dump_debug.c
 *************************/
