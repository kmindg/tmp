/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rightsche reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_raid_group_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the raid group object.
 *
 * @author
 *  11/24/2009 - Created. Rob Foley
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
 * fbe_metadata_operation_rg_paged_metadata_debug_trace()
 ****************************************************************
 * @brief
 *  Display the raid group paged metadata.
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
 *  03/14/2011 - Created Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_metadata_operation_rg_paged_metadata_debug_trace(const fbe_u8_t * module_name,
                                                                  fbe_dbgext_ptr paged_metadata_ptr,
                                                                  fbe_trace_func_t trace_func,
                                                                  fbe_trace_context_t trace_context,
                                                                  fbe_u32_t spaces_to_indent)
{
    fbe_raid_group_paged_metadata_t raid_group_paged_metadata = {0};

	fbe_trace_indent(trace_func, trace_context,spaces_to_indent);
    trace_func(trace_context, "raid_group_paged_metadata: \n");

    FBE_READ_MEMORY(paged_metadata_ptr,&raid_group_paged_metadata,sizeof(fbe_raid_group_paged_metadata_t));
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 2);
    trace_func(trace_context, "verify_bits: 0x%x\n", raid_group_paged_metadata.verify_bits);
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "needs_rebuild_bits: 0x%x\n", raid_group_paged_metadata.needs_rebuild_bits);
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "reserved_bits: 0x%x\n", raid_group_paged_metadata.reserved_bits);
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);

    return FBE_STATUS_OK;
}

/*!**************************************************************
 * fbe_raid_group_paged_metadata_debug_trace()
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
 *  28/01/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_paged_metadata_debug_trace(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr base_ptr,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_debug_field_info_t *field_info_p,
                                              fbe_u32_t spaces_to_indent)
{
    fbe_u32_t loop_counter = 0;
    fbe_raid_group_paged_metadata_t paged_metadata_info[FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS] = {0};
    fbe_u32_t paged_metadata_offset;
    trace_func(trace_context, "\n");
	fbe_trace_indent(trace_func, trace_context, 8);
    trace_func(trace_context, "raid group paged metadata:\n");
    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_nonpaged_metadata_t, "paged_metadata_metadata", &paged_metadata_offset);
    FBE_READ_MEMORY(base_ptr + paged_metadata_offset,&paged_metadata_info, (sizeof(fbe_raid_group_paged_metadata_t)*FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS));

    for(loop_counter = 0; loop_counter < FBE_RAID_GROUP_MAX_PAGED_METADATA_METADATA_SLOTS; ++loop_counter)
    {
		fbe_trace_indent(trace_func, trace_context, 12);
        trace_func(trace_context, "verify_bits[%d]: 0x%d", loop_counter,paged_metadata_info[loop_counter].verify_bits);	
        fbe_trace_indent(trace_func, trace_context, 6);
		trace_func(trace_context, "needs_rebuild_bits[%d]: 0x%x\n", loop_counter,paged_metadata_info[loop_counter].needs_rebuild_bits);	
    }
    return FBE_STATUS_OK;
}

/*!*******************************************************************
 * @var fbe_base_config_nonpaged_metadata_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config
 *        non-paged metadata.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_raid_group_rebuild_nonpaged_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("rebuild_logging_bitmask", fbe_raid_position_bitmask_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};


/*!*******************************************************************
 * @var fbe_base_config_nonpaged_metadata_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        generation number.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_raid_group_rebuild_nonpaged_info_t,
                              fbe_raid_group_rebuild_nonpaged_struct_info,
                              &fbe_raid_group_rebuild_nonpaged_field_info[0]);

/*!**************************************************************
 * fbe_rebuild_nonpaged_info_debug_trace()
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
fbe_status_t fbe_rebuild_nonpaged_info_debug_trace(const fbe_u8_t * module_name,
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
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, base_ptr + field_info_p->offset,
                                         &fbe_raid_group_rebuild_nonpaged_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);

    FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_nonpaged_metadata_t, "rebuild_info", &rebuild_info_offset);
    FBE_READ_MEMORY(base_ptr + rebuild_info_offset,&debug_rebuild_info, sizeof(fbe_raid_group_rebuild_nonpaged_info_t));
    trace_func(trace_context, "\n");
    for(loop_counter = 0; loop_counter < FBE_RAID_GROUP_MAX_REBUILD_POSITIONS; ++loop_counter)
    {
		fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(trace_context, "rebuild_checkpoint[%d]: 0x%llx", loop_counter, (unsigned long long)debug_rebuild_info.rebuild_checkpoint_info[loop_counter].checkpoint);
        fbe_trace_indent(trace_func, trace_context, 6);
		trace_func(trace_context, "rebuild_position[%d]: 0x%x\n", loop_counter,debug_rebuild_info.rebuild_checkpoint_info[loop_counter].position);	
    }
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent-1);
    return FBE_STATUS_OK;
}
/*!*******************************************************************
 * @var fbe_raid_group_encryption_nonpaged_field_info
 *********************************************************************
 * @brief Information about each of the fields of the
 *        encryption info.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_raid_group_encryption_nonpaged_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("rekey_positions", fbe_raid_position_bitmask_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("rekey_checkpoint", fbe_lba_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};


/*!*******************************************************************
 * @var fbe_raid_group_encryption_nonpaged_struct_info
 *********************************************************************
 * @brief Info on encryption structure
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_raid_group_encryption_nonpaged_info_t,
                              fbe_raid_group_encryption_nonpaged_struct_info,
                              &fbe_raid_group_encryption_nonpaged_field_info[0]);

/*!**************************************************************
 * fbe_encryption_nonpaged_info_debug_trace()
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
 *  10/28/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_encryption_nonpaged_info_debug_trace(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr base_ptr,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_debug_field_info_t *field_info_p,
                                              fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, base_ptr + field_info_p->offset,
                                         &fbe_raid_group_encryption_nonpaged_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);

    fbe_trace_indent(trace_func, trace_context, spaces_to_indent-1);
    return FBE_STATUS_OK;
}
/*!*******************************************************************
 * @var fbe_raid_group_nonpaged_metadata_field_info
 *********************************************************************
 * @brief Information about each of the fields of raid group
 *        non-paged metadata record.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_raid_group_nonpaged_metadata_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_config_nonpaged_metadata", fbe_base_config_nonpaged_metadata_t, FBE_TRUE, "0x%x", 
                                    fbe_base_config_nonpaged_metadata_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("rebuild_info", fbe_raid_group_rebuild_nonpaged_info_t, FBE_FALSE, "0x%x", 
                                    fbe_rebuild_nonpaged_info_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("encryption", fbe_raid_group_encryption_nonpaged_info_t, FBE_FALSE, "0x%x", 
                                    fbe_encryption_nonpaged_info_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO("ro_verify_checkpoint", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("rw_verify_checkpoint", fbe_lba_t, FBE_FALSE, "0x%x"),

    FBE_DEBUG_DECLARE_FIELD_INFO("error_verify_checkpoint", fbe_lba_t, FBE_FALSE, "0x%x"), 
    FBE_DEBUG_DECLARE_FIELD_INFO("journal_verify_checkpoint", fbe_lba_t, FBE_FALSE, "0x%x"),  
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("incomplete_write_verify_checkpoint", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("system_verify_checkpoint", fbe_lba_t, FBE_FALSE, "0x%x"),    
    FBE_DEBUG_DECLARE_FIELD_INFO("raid_group_np_md_flags", fbe_u8_t, FBE_FALSE, "0x%x"),
    
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("paged_metadata_metadata", fbe_raid_group_paged_metadata_t, FBE_FALSE, "0x%x", 
                                    fbe_raid_group_paged_metadata_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};


/*!*******************************************************************
 * @var fbe_raid_group_nonpaged_metadata_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        raid group non paged metadata record.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_raid_group_nonpaged_metadata_t,
                              fbe_raid_group_nonpaged_metadata_struct_info,
                              &fbe_raid_group_nonpaged_metadata_field_info[0]);

/*!**************************************************************
 * fbe_raid_group_non_paged_record_debug_trace()
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
 *  01/27/2011 - Created Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_non_paged_record_debug_trace(const fbe_u8_t * module_name,
                                            fbe_dbgext_ptr record_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "raid_group_non_paged_metadata: \n");
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, record_ptr,
                                         &fbe_raid_group_nonpaged_metadata_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent +4);
    return FBE_STATUS_OK;
}

/*!*******************************************************************
 * @var fbe_raid_group_metadata_memory_field_info
 *********************************************************************
 * @brief Information about each of the fields of raid group
 *        metadata memeory.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_raid_group_metadata_memory_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_config_metadata_memory", fbe_base_config_metadata_memory_t, FBE_TRUE, "0x%x",
                                    fbe_base_config_metadata_memory_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_raid_group_clustered_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("failed_drives_bitmap", fbe_raid_position_bitmask_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("failed_ios_pos_bitmap", fbe_raid_position_bitmask_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_raid_group_metadata_memory_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        raid group metadata memory.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_raid_group_metadata_memory_t,
                              fbe_raid_group_metadata_memory_struct_info,
                              &fbe_raid_group_metadata_memory_field_info[0]);

/*!**************************************************************
 * fbe_raid_group_metadata_memory_debug_trace()
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
 *  01/28/2011 - Created Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_metadata_memory_debug_trace(const fbe_u8_t * module_name,
                                                        fbe_dbgext_ptr record_ptr,
                                                        fbe_trace_func_t trace_func,
                                                        fbe_trace_context_t trace_context,
                                                        fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    trace_func(trace_context, "\n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);	
    trace_func(trace_context, "raid_group_metadata_memory: \n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 4);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, record_ptr,
                                         &fbe_raid_group_metadata_memory_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent + 4);
    return FBE_STATUS_OK;
}


/*!**************************************************************
 * fbe_raid_group_rebuilt_pvds_debug_trace()
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
 *  copied from fbe_raid_group_rebuilt_pvds_dump_debug_trace
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_rebuilt_pvds_debug_trace(const fbe_u8_t * module_name,
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

/*!**************************************************************
 * fbe_raid_group_bg_info_debug_trace()
 ****************************************************************
 * @brief
 *  Display the raid group background operation information.
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
 ****************************************************************/
fbe_status_t fbe_raid_group_bg_info_debug_trace(const fbe_u8_t * module_name,
                                                fbe_dbgext_ptr base_ptr,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context,
                                                fbe_debug_field_info_t *field_info_p,
                                                fbe_u32_t spaces_to_indent)
{
    fbe_raid_group_bg_op_flags_t flags;
    fbe_u32_t offset;
    fbe_u32_t memory_ptr_offset;
    fbe_u32_t memory_header_offset;
    fbe_u64_t memory_header_p = 0;
    fbe_u64_t memory_chunk_p = 0;
    fbe_u32_t ptr_size;
    fbe_u64_t bg_op_ptr = 0;
    fbe_u64_t magic_number = 0;

    fbe_debug_get_ptr_size(module_name, &ptr_size);
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &bg_op_ptr, ptr_size);
    if (bg_op_ptr != NULL) {
        FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_bg_op_info_t, "flags", &offset);
        FBE_READ_MEMORY(bg_op_ptr + offset, &flags, sizeof(flags));
        trace_func(trace_context, "bg_op flags: 0x%x ", flags);
    
        FBE_GET_FIELD_OFFSET(module_name, fbe_raid_group_bg_op_info_t, "memory_request", &offset);
        FBE_GET_FIELD_OFFSET(module_name, fbe_memory_request_t, "ptr", &memory_ptr_offset);
        FBE_READ_MEMORY(bg_op_ptr + offset + memory_ptr_offset, &memory_header_p, ptr_size);
    
        if (memory_header_p != NULL) {
            FBE_GET_FIELD_OFFSET(module_name, fbe_memory_header_t, "data", &memory_header_offset);
            FBE_READ_MEMORY(memory_header_p + memory_header_offset, &memory_chunk_p, ptr_size);
            trace_func(trace_context, "packet_p: 0x%llx\n", memory_chunk_p);
            if (memory_chunk_p != NULL) {
                FBE_GET_FIELD_DATA(module_name, memory_chunk_p, fbe_packet_t, magic_number, sizeof(fbe_u64_t), &magic_number);
                if ((magic_number == FBE_MAGIC_NUMBER_BASE_PACKET) ||
                    (magic_number == FBE_MAGIC_NUMBER_DESTROYED_PACKET))  {
                    fbe_transport_print_fbe_packet(module_name, memory_chunk_p, trace_func, trace_context, spaces_to_indent);
                    trace_func(trace_context, "\n");
                }
            }
        }
    }
    return FBE_STATUS_OK;
}


/*!*******************************************************************
 * @var fbe_raid_group_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_raid_group_field_info[] =
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
                                    fbe_raid_group_rebuilt_pvds_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("bg_info_p", fbe_object_id_t, FBE_FALSE, "0x%x", 
                                    fbe_raid_group_bg_info_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("bg_info_p", fbe_object_id_t, FBE_FALSE, "0x%x", 
                                    fbe_raid_group_bg_info_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("geo", fbe_raid_geometry_t, FBE_FALSE, "0x%x",
                                    fbe_raid_geometry_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_config", fbe_object_id_t, FBE_TRUE, "0x%x",
                                    fbe_base_config_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_raid_group_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_raid_group_t,
                              fbe_raid_group_struct_info,
                              &fbe_raid_group_field_info[0]);

/*!**************************************************************
 * fbe_raid_group_debug_trace()
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

fbe_status_t fbe_raid_group_debug_trace(const fbe_u8_t * module_name,
                                        fbe_dbgext_ptr raid_group_p,
                                        fbe_trace_func_t trace_func,
                                        fbe_trace_context_t trace_context,
                                        fbe_debug_field_info_t *field_info_p,
                                        fbe_u32_t overall_spaces_to_indent)
{
    fbe_status_t status;
    fbe_u32_t spaces_to_indent = overall_spaces_to_indent + 2;

    if (field_info_p != NULL)
    {
        /* If field info is provided, the ptr passed in is a ptr to the base structure. 
         * We need to add the offset to get to the actual raid group ptr.
         */
        raid_group_p += field_info_p->offset;
    }
    
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, raid_group_p,
                                         &fbe_raid_group_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent + 2);
        trace_func(trace_context, "\n");
    return status;
} 
/**************************************
 * end fbe_raid_group_debug_trace()
 **************************************/

/*!**************************************************************
 * fbe_raid_group_debug_display_terminator_queue()
 ****************************************************************
 * @brief
 *  Display the raid siots structure.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param raid_group_p - ptr to raid_group to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * @param b_summarize - TRUE to only print a summary of I/Os.
 * 
 * @return fbe_status_t
 *
 * @author
 *  10/23/2009 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_raid_group_debug_display_terminator_queue(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr raid_group_p,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_u32_t spaces_to_indent,
                                                           fbe_bool_t b_summarize)
{
    fbe_u32_t terminator_queue_offset;
    fbe_u64_t terminator_queue_head_ptr;
    fbe_u64_t queue_head_p = 0;
    fbe_u64_t queue_element_p = 0;
    fbe_u64_t next_queue_element_p = 0;
    fbe_u32_t packet_queue_element_offset = 0;
    fbe_u32_t payload_offset;
    fbe_u32_t iots_offset;
    fbe_u32_t ptr_size;

    if (b_summarize)
    {
        fbe_debug_set_display_queue_header(FBE_FALSE);
    }
    FBE_GET_TYPE_SIZE(module_name, fbe_u32_t*, &ptr_size);

    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "payload_union", &payload_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_payload_ex_t, "iots", &iots_offset);

    FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "queue_element", &packet_queue_element_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_base_object_t, "terminator_queue_head", &terminator_queue_offset);
    terminator_queue_head_ptr = raid_group_p + terminator_queue_offset;

    FBE_READ_MEMORY(raid_group_p + terminator_queue_offset, &queue_head_p, ptr_size);
    queue_element_p = queue_head_p;

    {
        fbe_u32_t item_count = 0;
        fbe_transport_get_queue_size(module_name,
                                     raid_group_p + terminator_queue_offset,
                                     trace_func,
                                     trace_context,
                                     &item_count);
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        if (item_count == 0)
        {
            trace_func(trace_context, "terminator queue: EMPTY\n");
        }
        else
        {
            trace_func(trace_context, "terminator queue: (%d items)\n",
                       item_count);
        }
    }

    /* Loop over all entries on the terminator queue, displaying each packet and iots. 
     * We stop when we reach the end (head) or NULL. 
     */
    while ((queue_element_p != terminator_queue_head_ptr) && (queue_element_p != NULL))
    {
        fbe_u64_t packet_p = queue_element_p - packet_queue_element_offset;

        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_CANCELED;
        }

        /* Display packet.
         */
        if (b_summarize)
        {
            fbe_transport_print_packet_summary(module_name, packet_p, trace_func, trace_context, spaces_to_indent + 2);
        }
        else
        {
            fbe_transport_print_fbe_packet(module_name, packet_p, trace_func, trace_context, spaces_to_indent + 2);
        }

        FBE_READ_MEMORY(queue_element_p, &next_queue_element_p, ptr_size);
        queue_element_p = next_queue_element_p;
    }
    
    if (b_summarize)
    {
        fbe_debug_set_display_queue_header(FBE_TRUE);
    }
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raid_group_debug_display_terminator_queue()
 ******************************************/

/*************************
 * end file fbe_raid_group_debug.c
 *************************/
