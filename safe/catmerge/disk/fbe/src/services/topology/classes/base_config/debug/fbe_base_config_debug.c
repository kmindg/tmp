/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_base_config_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the base config object.
 *
 * @author
 *  9/8/2010 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_base_object.h"
#include "fbe_base_object_debug.h"
#include "fbe_base_object_trace.h"
#include "fbe_transport_debug.h"
#include "fbe/fbe_base_config.h"
#include "fbe_base_config_debug.h"
#include "../fbe_base_config_private.h"
#include "fbe_metadata.h"
#include "fbe_provision_drive_private.h"
#include "fbe_raid_group_object.h"
#include "fbe_lun_private.h"
#include "fbe_metadata_cmi.h"
//#include "fbe_metadata_stripe_lock.h"
#include "fbe_metadata_debug.h"
#include "fbe_raid_group_debug.h"
#include "fbe_provision_drive_debug.h"
#include "fbe_lun_debug.h"
#include "fbe_virtual_drive_debug.h"
#include "fbe_metadata_stripe_lock_debug.h"
#include "pp_ext.h"

static fbe_class_id_t fbe_class_id = FBE_CLASS_ID_INVALID;


static fbe_status_t fbe_base_config_encryption_mode_trace(const fbe_u8_t * module_name,
                                                          fbe_dbgext_ptr base_ptr,
                                                          fbe_trace_func_t trace_func,
                                                          fbe_trace_context_t trace_context,
                                                          fbe_debug_field_info_t *field_info_p,
                                                          fbe_u32_t spaces_to_indent);

static fbe_status_t fbe_base_config_encryption_state_trace(const fbe_u8_t * module_name,
                                                          fbe_dbgext_ptr base_ptr,
                                                          fbe_trace_func_t trace_func,
                                                          fbe_trace_context_t trace_context,
                                                          fbe_debug_field_info_t *field_info_p,
                                                          fbe_u32_t spaces_to_indent);
/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * fbe_metadata_paged_blob_queue_debug_trace()
 ****************************************************************
 * @brief
 *  Display the metadata paged blob queue information.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param object_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Field to init. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  28/02/2011 - Created. Omer Miranda
 *
 ****************************************************************/ 
fbe_status_t fbe_metadata_paged_blob_queue_debug_trace(const fbe_u8_t * module_name,
                                                       fbe_dbgext_ptr object_ptr,
                                                       fbe_trace_func_t trace_func,
                                                       fbe_trace_context_t trace_context,
                                                       fbe_debug_field_info_t *field_info_p,
                                                       fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
	trace_func(trace_context, "\n");
	fbe_trace_indent(trace_func, trace_context, 8 /*spaces_to_indent*/);
    trace_func(trace_context, "metadata paged blob:");
    status = fbe_metadata_paged_blob_debug_trace(module_name,object_ptr + field_info_p->offset,
                                                                trace_func,trace_context,8);

    return status;
}

 /*!*******************************************************************
 * @var fbe_rdgen_debug_active_ts_queue_info_t
 *********************************************************************
 * @brief This structure holds info used to display a stripe
 *        lock operation queue using the "queue_element" field.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_QUEUE_INFO(fbe_payload_stripe_lock_operation_t, "queue_element", 
                             FBE_FALSE, /* queue element is the first field in the object. */
                             fbe_stripe_lock_operation_queue_info,
                             fbe_transport_print_stripe_lock_payload);


/*!*******************************************************************
 * @var fbe_sep_version_header_field_info
 *********************************************************************
 * @brief Information about each of the fields of fbe sep version header.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_sep_version_header_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("size", fbe_u32_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};


/*!*******************************************************************
 * @var fbe_sep_version_header_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        version header.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_sep_version_header_t,
                              fbe_sep_version_header_struct_info,
                              &fbe_sep_version_header_field_info[0]);






fbe_debug_field_info_t fbe_metadata_stripe_lock_hash_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("table_size", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("divisor", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_metadata_stripe_lock_hash_t,
                              fbe_metadata_fbe_metadata_stripe_lock_hash_struct_info,
                              &fbe_metadata_stripe_lock_hash_field_info[0]);

fbe_status_t fbe_metadata_stripe_lock_hash_debug_trace(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr object_ptr,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_debug_field_info_t *field_info_p,
                                             fbe_u32_t spaces_to_indent);


fbe_debug_field_info_t fbe_metadata_stripe_lock_hash_table_entry_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_QUEUE("head", fbe_queue_head_t, FBE_FALSE, &fbe_stripe_lock_operation_queue_info),

    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_metadata_stripe_lock_hash_table_entry_t,
                              fbe_metadata_stripe_lock_hash_table_entry_struct_info,
                              &fbe_metadata_stripe_lock_hash_table_entry_field_info[0]);








/*!**************************************************************
 * fbe_non_paged_version_header_debug_trace()
 ****************************************************************
 * @brief
 *  Display the non-paged metadata version header information.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param object_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Field to init. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  5/15/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/ 
fbe_status_t fbe_non_paged_version_header_debug_trace(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr object_ptr,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_debug_field_info_t *field_info_p,
                                             fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
     
	trace_func(trace_context, "\n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "%s: ", field_info_p->name);

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr + field_info_p->offset,
                                         &fbe_sep_version_header_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent /*spaces_to_indent*/);

    return status;	
}




/*!*******************************************************************
 * @var fbe_base_config_nonpaged_metadata_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config
 *        non-paged metadata.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_base_config_nonpaged_metadata_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("generation_number", fbe_generation_code_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("nonpaged_metadata_state", fbe_base_config_nonpaged_metadata_state_t, FBE_FALSE, "0x%x"),
	FBE_DEBUG_DECLARE_FIELD_INFO("operation_bitmask", fbe_base_config_background_operation_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("version_header", fbe_sep_version_header_t, FBE_TRUE, "0x%x", 
                                    fbe_non_paged_version_header_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};



/*!*******************************************************************
 * @var fbe_base_config_nonpaged_metadata_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        generation number.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_base_config_nonpaged_metadata_t,
                              fbe_base_config_nonpaged_metadata_struct_info,
                              &fbe_base_config_nonpaged_metadata_field_info[0]);

/*!**************************************************************
 * fbe_base_config_nonpaged_metadata_debug_trace()
 ****************************************************************
 * @brief
 *  Display the base config non paged metadata generation number.
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
 *  01/27/2011 - Created. Omer Miranda
 *  05/22/2012 - Modified. Jingcheng Zhang
 ****************************************************************/
fbe_status_t fbe_base_config_nonpaged_metadata_debug_trace(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr base_ptr,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_debug_field_info_t *field_info_p,
                                                           fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, base_ptr,
                                         &fbe_base_config_nonpaged_metadata_struct_info,
                                         4 /* fields per line */,
                                         10);
	trace_func(trace_context, "\n");
    return FBE_STATUS_OK;
}



/*!**************************************************************
 * fbe_raid_group_non_paged_record_debug_trace()
 ****************************************************************
 * @brief
 *  Display the non-paged record for all objects like - raid , lun and provision drive.
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
 *  12/15/2010 - Created. Hari singh chauhan
 *
 ****************************************************************/
fbe_status_t fbe_non_paged_record_debug_trace(const fbe_u8_t * module_name,
                                            fbe_dbgext_ptr object_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_u32_t spaces_to_indent)
{
    fbe_u32_t data_ptr_offset;
    ULONG64 data_ptr;
       
    FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_record_t, "data_ptr", &data_ptr_offset);
    FBE_READ_MEMORY(object_ptr + data_ptr_offset, &data_ptr, sizeof(ULONG64));	

    if (data_ptr == NULL)
    {
        return FBE_STATUS_OK;
    }

    trace_func(trace_context, "\n");
    switch(fbe_class_id)
    {
        case FBE_CLASS_ID_PARITY:
        case FBE_CLASS_ID_STRIPER:
        case FBE_CLASS_ID_MIRROR:
        case FBE_CLASS_ID_VIRTUAL_DRIVE: /* Virtual drive objects metadata is the same as its parent class (RAID) */
            fbe_raid_group_non_paged_record_debug_trace(module_name, data_ptr,trace_func,trace_context,spaces_to_indent);
            break;
        case FBE_CLASS_ID_PROVISION_DRIVE:
            fbe_provision_drive_non_paged_record_debug_trace(module_name, data_ptr,trace_func,trace_context,spaces_to_indent);
            break;
        case FBE_CLASS_ID_LUN:
            fbe_lun_non_paged_record_debug_trace(module_name, data_ptr,trace_func,trace_context,spaces_to_indent);
            break;
        default:	
                     /* There is no handling for this class yet.
                       * Display only class id for it. 
                       */
            trace_func(trace_context, "CLASS ID: %d", fbe_class_id);
    }

   return FBE_STATUS_OK;
}

/*!*******************************************************************
 * @var fbe_metadata_record_field_info
 *********************************************************************
 * @brief Information about each of the fields of metadata record.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_metadata_record_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("lba", fbe_lba_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("data_size", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("data_ptr", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};


/*!*******************************************************************
 * @var fbe_metadata_record_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        metadata record.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_metadata_record_t,
                              fbe_metadata_record_struct_info,
                              &fbe_metadata_record_field_info[0]);


/*!**************************************************************
 * fbe_non_paged_record_metadata_debug_trace()
 ****************************************************************
 * @brief
 *  Display the non-paged record metadata information.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param object_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Field to init. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  12/13/2010 - Created. Hari singh chauhan
 *
 ****************************************************************/ 
fbe_status_t fbe_non_paged_record_metadata_debug_trace(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr object_ptr,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_debug_field_info_t *field_info_p,
                                             fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
     
	trace_func(trace_context, "\n");
	fbe_trace_indent(trace_func, trace_context, 8 /*spaces_to_indent*/);
    trace_func(trace_context, "%s: ", field_info_p->name);

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr + field_info_p->offset,
                                         &fbe_metadata_record_struct_info,
                                         4 /* fields per line */,
                                         8 /*spaces_to_indent*/);

    status = fbe_non_paged_record_debug_trace(module_name,object_ptr + field_info_p->offset,
                                                                trace_func,trace_context,8);

    return status;	
}

/*!**************************************************************
 * fbe_object_metadata_memory_debug_trace()
 ****************************************************************
 * @brief
 *  Display the clustered memory(metadata memory and metadata_memory_peer) information.
 *  for all objects like raid , lun and provision drive
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
 *  12/15/2010 - Created. Hari singh chauhan
 *
 ****************************************************************/
fbe_status_t fbe_object_metadata_memory_debug_trace(const fbe_u8_t * module_name,
                                            fbe_dbgext_ptr object_ptr,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context,
                                            fbe_u32_t spaces_to_indent)
{
    ULONG64 data_ptr;

    FBE_READ_MEMORY(object_ptr, &data_ptr, sizeof(ULONG64));

    if (data_ptr == NULL)
    {
        return FBE_STATUS_OK;
    }

    switch(fbe_class_id)
    {
        /* All the below are derived from a raid group and do not have any 
         * class specific metadata memory of their own. 
         */
        case FBE_CLASS_ID_PARITY:
        case FBE_CLASS_ID_STRIPER:
        case FBE_CLASS_ID_MIRROR:
        case FBE_CLASS_ID_VIRTUAL_DRIVE:
            fbe_raid_group_metadata_memory_debug_trace(module_name, data_ptr,trace_func,trace_context,spaces_to_indent);	
            break;
        case FBE_CLASS_ID_PROVISION_DRIVE:
            fbe_provision_drive_metadata_memory_debug_trace(module_name, data_ptr,trace_func,trace_context,spaces_to_indent);	
            break;
        case FBE_CLASS_ID_LUN:
            fbe_lun_object_metadata_memory_debug_trace(module_name, data_ptr,trace_func,trace_context,spaces_to_indent);	
            break;
        default:	
            /* There is no handling for this class yet.
              * Display only class id for it. 
              */
            trace_func(trace_context, "CLASS ID: %d", fbe_class_id);			 
         
    }
    return FBE_STATUS_OK;	
}

/*!*******************************************************************
 * @var fbe_metadata_memory_field_info
 *********************************************************************
 * @brief Information about each of the fields of metadata memory.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_metadata_memory_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("memory_ptr", fbe_u64_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("memory_size", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};


/*!*******************************************************************
 * @var fbe_metadata_memory_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        metadata memory.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_metadata_memory_t,
                              fbe_metadata_memory_struct_info,
                              &fbe_metadata_memory_field_info[0]);

/*!**************************************************************
 * fbe_metadata_memory_debug_trace()
 ****************************************************************
 * @brief
 *  Display the clustered memory(metadata memory and metadata_memory_peer) information.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param object_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Field to init. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  12/13/2010 - Created. Hari singh chauhan
 *
 ****************************************************************/ 
fbe_status_t fbe_metadata_memory_debug_trace(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr object_ptr,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_debug_field_info_t *field_info_p,
                                             fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t metadata_memory_size;
        
    FBE_GET_TYPE_SIZE(module_name, fbe_metadata_memory_t, &metadata_memory_size);
    if (field_info_p->size != metadata_memory_size)
    {
        trace_func(trace_context, "%s size is %d not %d\n",
                   field_info_p->name, field_info_p->size, metadata_memory_size);
        return FBE_STATUS_GENERIC_FAILURE; 
    }
	trace_func(trace_context, "\n");
	fbe_trace_indent(trace_func, trace_context, 8 /*spaces_to_indent*/);
    trace_func(trace_context, "%s:", field_info_p->name);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr + field_info_p->offset,
                                         &fbe_metadata_memory_struct_info,
                                         4 /* fields per line */,
                                         8 /*spaces_to_indent */);

    status = fbe_object_metadata_memory_debug_trace(module_name,object_ptr + field_info_p->offset,
                                                    trace_func,trace_context,8);
        
    return status;	
}

/*!**************************************************************
 * fbe_paged_record_metadata_debug_trace()
 ****************************************************************
 * @brief
 *  Display the paged metadata record queue information.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param object_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Field to init. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  12/17/2010 - Created. Hari singh chauhan
 *
 ****************************************************************/ 
fbe_status_t fbe_paged_record_metadata_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr object_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_field_info_t *field_info_p,
                                                   fbe_u32_t spaces_to_indent)
{
    fbe_u32_t item_count = 0, offset;
    fbe_u64_t curr_queue_element_p, next_queue_element_p;
    fbe_dbgext_ptr queue_p;
    fbe_u32_t payload_union_offset = 0;
    fbe_u32_t metadata_operation_offset = 0;
    fbe_u64_t payload_metadata_ptr = 0;
    fbe_u64_t metadata_opcode_ptr = 0;
    fbe_u32_t metadata_opcode_offset = 0;
/*    fbe_u32_t repeat_offset = 0; */
    fbe_u32_t metadata_operation_union_offset = 0;
    fbe_u32_t metadata_operation_repeat_count = 0;
/*    fbe_u64_t metadata_operation_repeat_offset = 0; */
    fbe_u32_t record_data_offset = 0;
    fbe_u32_t repeat_count_index = 0;
    fbe_u64_t paged_metadata_ptr = 0;
    fbe_payload_metadata_operation_opcode_t opcode_debug = FBE_PAYLOAD_METADATA_OPERATION_OPCODE_INVALID;
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t ptr_size;
    fbe_u32_t raid_group_paged_metadata_size;
    fbe_u32_t provision_drive_paged_metadata_size;
    fbe_u32_t payload_metadata_operation_opcode_size;
    fbe_u32_t queue_head_size;

    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        trace_func(trace_context, "%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return status; 
    }
    FBE_GET_TYPE_SIZE(module_name, fbe_queue_head_t, &queue_head_size);
           
    if (field_info_p->size != queue_head_size)
    {
        trace_func(trace_context, "%s size is %d not %d\n",
                   field_info_p->name, field_info_p->size, queue_head_size);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    FBE_GET_TYPE_SIZE(module_name, fbe_raid_group_paged_metadata_t, &raid_group_paged_metadata_size);
    FBE_GET_TYPE_SIZE(module_name, fbe_provision_drive_paged_metadata_t, &provision_drive_paged_metadata_size);
    FBE_GET_TYPE_SIZE(module_name, fbe_payload_metadata_operation_opcode_t, &payload_metadata_operation_opcode_size);

    queue_p = object_ptr + field_info_p->offset;
    fbe_transport_get_queue_size(module_name,queue_p,trace_func,trace_context,&item_count);
    if (item_count == 0)
    {
		fbe_trace_indent(trace_func, trace_context, 10 /*spaces_to_indent*/);
        trace_func(trace_context, "queue (paged_record_queue_head): EMPTY\n");
    }
    else
    {
		fbe_trace_indent(trace_func, trace_context, 10 /*spaces_to_indent*/);
        trace_func(trace_context, "paged_record_queue_head: (%d items)\n",item_count);
        FBE_GET_FIELD_DATA(module_name,
                           queue_p,
                           fbe_queue_head_t,
                           next,
                           ptr_size,
                           &next_queue_element_p);

        FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "queue_element", &offset);

        /* Iterate through the queue to display the packet detail.
         */
        while (next_queue_element_p != queue_p)
        {
            if (FBE_CHECK_CONTROL_C())
            {
                return FBE_STATUS_GENERIC_FAILURE;
            }

            curr_queue_element_p = next_queue_element_p;
			fbe_trace_indent(trace_func, trace_context, 8 /*spaces_to_indent*/);
            trace_func(trace_context, "fbe_packet_ptr: 0x%llx\n",
		       (unsigned long long)(curr_queue_element_p - offset));

            FBE_GET_FIELD_OFFSET(module_name, fbe_packet_t, "payload_union", &payload_union_offset);
            FBE_GET_FIELD_OFFSET(module_name, fbe_payload_ex_t, "metadata_operation", &metadata_operation_offset);
            payload_metadata_ptr = (curr_queue_element_p - offset) + payload_union_offset + metadata_operation_offset;

            status = fbe_transport_print_metadata_payload(module_name, payload_metadata_ptr,trace_func,trace_context,8 /*spaces_to_indent*/);
            
            FBE_GET_FIELD_OFFSET(module_name, fbe_payload_metadata_operation_t, "opcode", &metadata_opcode_offset);
            metadata_opcode_ptr = payload_metadata_ptr + metadata_opcode_offset;
            FBE_READ_MEMORY(metadata_opcode_ptr,&opcode_debug, payload_metadata_operation_opcode_size);
            FBE_GET_FIELD_OFFSET(module_name, fbe_payload_metadata_operation_t, "u", &metadata_operation_union_offset);

            if((opcode_debug == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_SET_BITS) ||
               (opcode_debug == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_CLEAR_BITS) ||
               (opcode_debug == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE) ||
               (opcode_debug == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE_VERIFY) ||
               (opcode_debug == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_VERIFY_WRITE) ||
               (opcode_debug == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_XOR_BITS) ||
               (opcode_debug == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_READ) ||
               (opcode_debug == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_UPDATE) ||
               (opcode_debug == FBE_PAYLOAD_METADATA_OPERATION_OPCODE_PAGED_WRITE_VERIFY_UPDATE))
            {
                /* FBE_GET_FIELD_OFFSET(module_name, fbe_payload_metadata_operation_metadata_t, "repeat_count", &repeat_offset);
                FBE_READ_MEMORY(payload_metadata_ptr + metadata_operation_union_offset + repeat_offset,
                                &metadata_operation_repeat_count, sizeof(fbe_u32_t)); 
                FBE_GET_FIELD_OFFSET(module_name, fbe_payload_metadata_operation_metadata_t, "repeat_offset", &repeat_offset);
                FBE_READ_MEMORY(payload_metadata_ptr + metadata_operation_union_offset + repeat_offset,
                                &metadata_operation_repeat_offset, sizeof(fbe_u64_t)); */

                FBE_GET_FIELD_OFFSET(module_name, fbe_payload_metadata_operation_metadata_t, "record_data", &record_data_offset);
            
                for(repeat_count_index = 0; repeat_count_index < metadata_operation_repeat_count;repeat_count_index++)
                {
					fbe_trace_indent(trace_func, trace_context, 8 /*spaces_to_indent*/);
                    trace_func(trace_context, "chunk_index: 0x%x\n", repeat_count_index + 1);
                    switch(fbe_class_id)
                    {
                        case FBE_CLASS_ID_PARITY:
                        case FBE_CLASS_ID_STRIPER:
                        case FBE_CLASS_ID_MIRROR:
                        case FBE_CLASS_ID_VIRTUAL_DRIVE: /* Virtual drive objects metadata is the same as its parent class (RAID) */
                            paged_metadata_ptr = payload_metadata_ptr + metadata_operation_union_offset + record_data_offset + 
                                                    (repeat_count_index * raid_group_paged_metadata_size);
                            fbe_metadata_operation_rg_paged_metadata_debug_trace(module_name,paged_metadata_ptr,trace_func,trace_context,8);            
                            break;
                        case FBE_CLASS_ID_PROVISION_DRIVE:
                            paged_metadata_ptr = payload_metadata_ptr + metadata_operation_union_offset + record_data_offset + 
                                                    (repeat_count_index * provision_drive_paged_metadata_size);
                            fbe_metadata_operation_pvd_paged_metadata_debug_trace(module_name,paged_metadata_ptr,trace_func,trace_context,8);
                            break;
                        case FBE_CLASS_ID_LUN:
                            //Currently the LUN object does not use its paged metadata
                            break;
                        default:	
                        /* There is no handling for this class yet.
                        * Display only class id for it. 
                        */
                        trace_func(trace_context, "CLASS ID: %d", fbe_class_id);
                    }
                    
                }
            }

            FBE_GET_FIELD_DATA(module_name,
                               curr_queue_element_p,
                               fbe_queue_element_t,
                               next,
                               ptr_size,
                               &next_queue_element_p);
        }
    }

    return status;	
}

/*!*******************************************************************
 * @var fbe_metadata_element_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_metadata_element_field_info[] =
{
    /* base object is displayed first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("paged_record_start_lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("paged_mirror_metadata_offset", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("metadata_element_state", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("paged_record_request_count", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("attributes", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("peer_metadata_element_ptr", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("stripe_lock_number_of_stripes", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("stripe_lock_number_of_private_stripes", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("stripe_lock_count", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("local_collision_count", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("peer_collision_count", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("cmi_message_count", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("attributes", fbe_u32_t, FBE_FALSE, "0x%x"),

    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("nonpaged_record", fbe_metadata_record_t, FBE_FALSE, "0x%x", 
                                    fbe_non_paged_record_metadata_debug_trace),   
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("metadata_memory", fbe_metadata_memory_t, FBE_FALSE, "0x%x", 
                                    fbe_metadata_memory_debug_trace), 
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("metadata_memory_peer", fbe_metadata_memory_t, FBE_FALSE, "0x%x", 
                                    fbe_metadata_memory_debug_trace),

    FBE_DEBUG_DECLARE_FIELD_INFO_FN("stripe_lock_hash", fbe_metadata_stripe_lock_hash_t, FBE_FALSE, "0x%x", 
                                    fbe_metadata_stripe_lock_hash_debug_trace),

    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_metadata_element_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_metadata_element_t,
                              fbe_metadata_element_struct_info,
                              &fbe_metadata_element_field_info[0]);
/*!*******************************************************************
 * @var fbe_metadata_element_queues_field_info
 *********************************************************************
 * @brief Information queue fields of base config.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_metadata_element_queues_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_QUEUE("stripe_lock_queue_head", fbe_queue_head_t, FBE_FALSE, 
                                       &fbe_stripe_lock_operation_queue_info),


    FBE_DEBUG_DECLARE_FIELD_INFO_FN("paged_record_queue_head", fbe_queue_head_t, FBE_FALSE, "0x%x", 
                                    fbe_paged_record_metadata_debug_trace),
    
	//FBE_DEBUG_DECLARE_FIELD_INFO_FN("stripe_lock_blob", metadata_stripe_lock_blob_t, FBE_FALSE, "0x%x", fbe_metadata_stripe_lock_slot_debug_trace), 

    FBE_DEBUG_DECLARE_FIELD_INFO_FN("paged_blob_queue_head", fbe_queue_head_t, FBE_FALSE, "0x%x", 
                                    fbe_metadata_paged_blob_queue_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_metadata_element_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_metadata_element_t,
                              fbe_metadata_element_queues_struct_info,
                              &fbe_metadata_element_queues_field_info[0]);

/*!**************************************************************
 * fbe_metadata_element_debug_trace()
 ****************************************************************
 * @brief
 *  Display info on metadata element.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t
 *
 * @author
 *  9/8/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_metadata_element_debug_trace(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr object_ptr,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_debug_field_info_t *field_info_p,
                                              fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;

    trace_func(trace_context, "%s: \n", field_info_p->name);
	fbe_trace_indent(trace_func, trace_context, 8 /*spaces_to_indent*/);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, 
                                         object_ptr + field_info_p->offset,
                                         &fbe_metadata_element_struct_info,
                                         4 /* fields per line */,
                                         8 /*spaces_to_indent*/);
   /* Do not display job service queues if active queue display is disabled 
     * using !disable_active_queue_display' debug macro
     */
     if(get_active_queue_display_flag() == FBE_FALSE)
    {
        return FBE_STATUS_OK;
    }
    else
    {
        status = fbe_debug_display_structure(module_name, trace_func, trace_context, 
                                         object_ptr + field_info_p->offset,
                                         &fbe_metadata_element_queues_struct_info,
                                         4 /* fields per line */,
                                         8 /*spaces_to_indent*/);
    }
    return status;
}

/*!*******************************************************************
 * @var fbe_base_config_signature_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config signature.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_base_config_signature_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("signature_crc", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("object_id", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("class_id", fbe_class_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_base_config_signature_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base signature data.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_base_config_signature_data_t,
                              fbe_base_config_signature_info,
                              &fbe_base_config_signature_field_info[0]);

/*!**************************************************************
 * fbe_base_config_signature_debug_trace()
 ****************************************************************
 * @brief
 *  Display all the trace information about the object signature.
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
 *  1/14/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_base_config_signature_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr base_p,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_field_info_t *field_info_p,
                                                   fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    trace_func(trace_context, "%s: ", field_info_p->name);

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, 
                                         base_p + field_info_p->offset,
                                         &fbe_base_config_signature_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);
    return status;
}

/*!**************************************************************
 * fbe_base_config_signature_trace()
 ****************************************************************
 * @brief
 *  Display all the trace information about the signature.
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
 *  12/3/2012 - Created. Arun S
 *
 ****************************************************************/
fbe_status_t fbe_base_config_signature_trace(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr base_p,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_debug_field_info_t *field_info_p,
                                             fbe_u32_t spaces_to_indent)
{
    fbe_u64_t wwn = 0;
    fbe_u8_t serial_num[FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1];

    trace_func(trace_context, "\n\tSignature details:");
    if(fbe_class_id == FBE_CLASS_ID_PROVISION_DRIVE)
    {
        FBE_READ_MEMORY(base_p + field_info_p->offset, &serial_num[0], FBE_SCSI_INQUIRY_SERIAL_NUMBER_SIZE+1);	
        trace_func(trace_context, "Serial number: %s\n", serial_num);
    }
    else
    {
        FBE_READ_MEMORY(base_p + field_info_p->offset, &wwn, sizeof(fbe_u64_t));	
        trace_func(trace_context, "WWN: 0x%016llx\n", wwn);
    }

    return FBE_STATUS_OK;
}


/******************************************
 * end fbe_metadata_element_debug_trace()
 ******************************************/
/*!*******************************************************************
 * @def FBE_BASE_CONFIG_WIDTH_FIELD_INFO_INDEX
 *********************************************************************
 * @brief Index of the width in the field info array.
 *
 *********************************************************************/
#define FBE_BASE_CONFIG_WIDTH_FIELD_INFO_INDEX 2
/*!*******************************************************************
 * @var fbe_base_config_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_base_config_field_info[] =
{
    /* base object is displayed first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_object", fbe_u32_t, FBE_TRUE, "0x%x",
                                    fbe_base_object_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    /* IMPORTANT: If the index of the width changes please update the above. 
     *            FBE_BASE_CONFIG_WIDTH_FIELD_INFO_INDEX 
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("width", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("power_saving_idle_time_in_seconds", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("background_operation_deny_count", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("hibernation_time", fbe_time_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("global_path_attr", fbe_path_attr_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("encryption_mode", fbe_base_config_encryption_mode_t, FBE_FALSE, "0x%x",
                                    fbe_base_config_encryption_mode_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("encryption_state", fbe_base_config_encryption_state_t, FBE_FALSE, "0x%x",
                                    fbe_base_config_encryption_state_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("flags", fbe_base_config_flags_t, FBE_FALSE, "0x%x",
                                    fbe_base_config_flag_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO("encryption_state", fbe_base_config_encryption_state_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("encryption_mode", fbe_base_config_encryption_mode_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("key_handle", fbe_encryption_setup_encryption_keys_t, FBE_FALSE, "0x%x", 
                                    fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("flags", fbe_base_config_flags_t, FBE_FALSE, "0x%x",
                                    fbe_base_config_flag_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("block_edge_p", fbe_block_edge_t, FBE_FALSE, "0x%x", 
                                    fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("metadata_element", fbe_metadata_element_t, FBE_FALSE, "0x%x", 
                                    fbe_metadata_element_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("u", fbe_u64_t, FBE_FALSE, "0x%x", fbe_base_config_signature_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_base_config_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_base_config_t,
                              fbe_base_config_struct_info,
                              &fbe_base_config_field_info[0]);


/*!**************************************************************
 * fbe_base_config_debug_trace()
 ****************************************************************
 * @brief
 *  Display all the trace information about the raid group object.
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
 *  9/8/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_base_config_debug_trace(const fbe_u8_t * module_name,
                                         fbe_dbgext_ptr base_p,
                                         fbe_trace_func_t trace_func,
                                         fbe_trace_context_t trace_context,
                                         fbe_debug_field_info_t *field_info_p,
                                         fbe_u32_t overall_spaces_to_indent)
{
    fbe_status_t status;
    fbe_u32_t transport_server_offset = 0;
    fbe_u64_t block_edge_p = 0;
    fbe_u32_t ptr_size = 0;
    fbe_u32_t block_edge_size = 0;
    fbe_u32_t edge_index = 0;
    fbe_u32_t width;
    fbe_u32_t block_edge_ptr_offset;
    fbe_u32_t spaces_to_indent = overall_spaces_to_indent + 2;
    fbe_u32_t base_object_scheduler_element_offset = 0;
    fbe_u32_t scheduler_element_packet_offset = 0;
    fbe_u64_t scheduler_packet_p = 0;
    fbe_u32_t class_id_offset =  0;
    fbe_u64_t base_config_p = base_p + field_info_p->offset;

    FBE_GET_FIELD_OFFSET(module_name, fbe_base_object_t, "class_id", &class_id_offset);
    FBE_READ_MEMORY(base_p + class_id_offset, &fbe_class_id, sizeof(fbe_class_id_t));	

    trace_func(trace_context, "%s: \n", field_info_p->name);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, base_config_p,
                                         &fbe_base_config_struct_info,
                                         4 /* fields per line */,
                                         6 /*spaces_to_indent*/);

    if (status != FBE_STATUS_OK) 
    { 
        return status; 
    }
	
    /* Display the Block Edge information.
     */
    fbe_debug_get_ptr_size(module_name, &ptr_size);
    FBE_GET_FIELD_OFFSET(module_name, fbe_base_config_t, "block_edge_p", &block_edge_ptr_offset);
    FBE_READ_MEMORY(base_config_p + block_edge_ptr_offset, &block_edge_p, ptr_size);
    FBE_GET_TYPE_SIZE(module_name, fbe_block_edge_t, &block_edge_size);

    FBE_READ_MEMORY(base_config_p + fbe_base_config_field_info[FBE_BASE_CONFIG_WIDTH_FIELD_INFO_INDEX].offset,
                    &width, sizeof(fbe_u32_t));
    for ( edge_index = 0; edge_index < width; edge_index++)
    {
        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_CANCELED;
        }
        fbe_block_transport_display_edge(module_name,
                                         block_edge_p + (block_edge_size * edge_index),
                                         trace_func,
                                         trace_context,
                                         spaces_to_indent + 2 /* spaces to indent */);
    }
    /* If there is a monitor packet, display it.
     */
    FBE_GET_FIELD_OFFSET(module_name, fbe_base_object_t, "scheduler_element", &base_object_scheduler_element_offset);
    FBE_GET_FIELD_OFFSET(module_name, fbe_scheduler_element_t, "packet", &scheduler_element_packet_offset);
    FBE_READ_MEMORY(base_config_p + base_object_scheduler_element_offset + scheduler_element_packet_offset, 
                    &scheduler_packet_p, ptr_size);
    if (scheduler_packet_p != 0)
    {
       fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
       trace_func(trace_context, "scheduler packet: fbe_packet_ptr: 0x%llx\n",
                  (unsigned long long)scheduler_packet_p);
       fbe_transport_print_fbe_packet(module_name,
                                      scheduler_packet_p,
                                      trace_func,
                                      trace_context,
                                      spaces_to_indent );
        trace_func(trace_context, "\n");
    }
    /* Display the Transport Server information.
     */
    FBE_GET_FIELD_OFFSET(module_name,
                         fbe_base_config_t,
                         "block_transport_server",
                         &transport_server_offset);
    fbe_block_transport_print_server_detail(module_name,
                                            base_config_p + transport_server_offset,
                                            trace_func,
                                            trace_context);

    fbe_class_id = FBE_CLASS_ID_INVALID;
    return FBE_STATUS_OK;
} 
/**************************************
 * end fbe_base_config_debug_trace()
 **************************************/

/*!**************************************************************
 * fbe_base_config_flag_debug_trace()
 ****************************************************************
 * @brief
 *  Display the base config flag.
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
 *  12/03/2010 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_base_config_flag_debug_trace(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr base_ptr,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_debug_field_info_t *field_info_p,
                                              fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_base_config_flags_t flags;
    fbe_base_config_flags_t flag_to_trace;
    fbe_u32_t               shift_count = 0;
    if (field_info_p->size != sizeof(fbe_base_config_flags_t))
    {
        trace_func(trace_context, "%s size is %d not %llu\n",
                   field_info_p->name, field_info_p->size,
		   (unsigned long long)sizeof(fbe_base_config_flags_t));
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &flags, field_info_p->size);

    trace_func(trace_context, "%s: 0x%x ", field_info_p->name, (unsigned int)flags);
    while((flags != 0)       &&
          (shift_count < 32)    )
    {
        flag_to_trace = (1ULL << shift_count);
        if ((flags & flag_to_trace) != 0)
        {
            fbe_base_config_flag_string_trace(flag_to_trace, trace_func, trace_context);
            flags &= ~flag_to_trace;
        }
        shift_count++;
    }
    trace_func(trace_context, "%s\n", field_info_p->name);
    return status;
}
/**************************************
 * end fbe_class_id_debug_trace
 **************************************/

/*!***************************************************************************
 * fbe_base_config_flag_string_trace()
 ******************************************************************************
 * @brief
 *  Display's the base config flag string
 *
 * @param  flags - config flag to be converted to string format
  *@param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK 
 *
 * @author
 *  12/03/2009 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_base_config_flag_string_trace(fbe_base_config_flags_t flag, 
                                               fbe_trace_func_t trace_func, 
                                               fbe_trace_context_t trace_context)
{
    switch(flag)
    {
        case FBE_BASE_CONFIG_FLAG_POWER_SAVING_ENABLED:
            trace_func(trace_context, "POWER_SAVING_ENABLED ");
            break;
        case FBE_BASE_CONFIG_FLAG_CONFIGURATION_COMMITED:
            trace_func(trace_context, "CONFIGURATION_COMMITED ");
            break;
        case FBE_BASE_CONFIG_FLAG_USER_INIT_QUIESCE:
            trace_func(trace_context, "USER_QUIESCE ");
            break;
        case FBE_BASE_CONFIG_FLAG_DENY_OPERATION_PERMISSION:
            trace_func(trace_context, "DENY_PERMISSION ");
            break;
        case FBE_BASE_CONFIG_FLAG_PERM_DESTROY:
            trace_func(trace_context, "PERM_DESTROY ");
            break;
        case FBE_BASE_CONFIG_FLAG_RESPECIALIZE:
            trace_func(trace_context, "RESPECIALIZE ");
            break;
        case FBE_BASE_CONFIG_FLAG_INITIAL_CONFIGURATION:
            trace_func(trace_context, "INITIAL_CONFIG ");
            break;
        default:
            trace_func(trace_context, "UNKNOWN BASE CONFIG FLAG 0x%x", (unsigned int)flag);
        break;
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_base_config_flag_string_trace()
 ******************************************/

/*!*******************************************************************
 * @var fbe_base_config_metadata_memory_field_info
 *********************************************************************
 * @brief Information about each of the fields of lun object
 *        metadata memeory.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_base_config_metadata_memory_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("lifecycle_state", fbe_lifecycle_state_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("power_save_state", fbe_power_save_state_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("last_io_time", fbe_time_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_base_config_clustered_flags_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_base_config_metadata_memory_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        lun object metadata memory.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_base_config_metadata_memory_t,
                              fbe_base_config_metadata_memory_struct_info,
                              &fbe_base_config_metadata_memory_field_info[0]);

/*!**************************************************************
 * fbe_base_config_metadata_memory_debug_trace()
 ****************************************************************
 * @brief
 *  Display the base config object metadata memeory.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param record_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  7/15/2011 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_base_config_metadata_memory_debug_trace(const fbe_u8_t * module_name,
                                                         fbe_dbgext_ptr record_ptr,
                                                         fbe_trace_func_t trace_func,
                                                         fbe_trace_context_t trace_context,
                                                         fbe_debug_field_info_t *field_info_p,
                                                         fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    trace_func(trace_context, "\n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);  
    trace_func(trace_context, "base_config_metadata_memory: \n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 4);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, record_ptr + field_info_p->offset,
                                         &fbe_base_config_metadata_memory_struct_info,
                                         4    /* fields per line */,
                                         spaces_to_indent + 4);
    return FBE_STATUS_OK;
}
/**************************************
 * end fbe_base_config_metadata_memory_debug_trace
 **************************************/

fbe_status_t fbe_metadata_stripe_lock_hash_debug_trace(const fbe_u8_t * module_name,
                                             fbe_dbgext_ptr object_ptr,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context,
                                             fbe_debug_field_info_t *field_info_p,
                                             fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_u32_t size;
	fbe_u64_t hash_ptr;
	fbe_u64_t table_size;
	fbe_u32_t table_size_offset;
	fbe_u32_t table_entry_size;
	fbe_u32_t i;
	fbe_u32_t table_offset;
	fbe_u64_t table_ptr;
	fbe_u32_t head_offset;
	fbe_u32_t item_count = 0;
        
    FBE_GET_TYPE_SIZE(module_name, fbe_metadata_stripe_lock_hash_t, &size);
    if (field_info_p->size != size)
    {
        trace_func(trace_context, "%s size is %d not %d\n",
                   field_info_p->name, field_info_p->size, size);
        return FBE_STATUS_GENERIC_FAILURE; 
    }

    FBE_READ_MEMORY(object_ptr + field_info_p->offset, &hash_ptr, sizeof(fbe_u64_t));

	if(hash_ptr == NULL){
		trace_func(trace_context, "\n Stripe Lock hash disabled \n");
		return FBE_STATUS_OK;
	}

	trace_func(trace_context, "\n");
	fbe_trace_indent(trace_func, trace_context, 8 /*spaces_to_indent*/);
    trace_func(trace_context, "%s: hash_ptr = 0x%016llx ", field_info_p->name, hash_ptr);

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, hash_ptr,
                                         &fbe_metadata_fbe_metadata_stripe_lock_hash_struct_info,
                                         4 /* fields per line */,
                                         8 /*spaces_to_indent */);


	
    FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_stripe_lock_hash_t, "table_size", &table_size_offset);

    FBE_READ_MEMORY(hash_ptr + table_size_offset, &table_size, sizeof(fbe_u64_t));

    FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_stripe_lock_hash_t, "table", &table_offset);

    FBE_READ_MEMORY(hash_ptr + table_offset, &table_ptr, sizeof(fbe_u64_t));

	FBE_GET_TYPE_SIZE(module_name, fbe_metadata_stripe_lock_hash_table_entry_t, &table_entry_size);
    FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_stripe_lock_hash_table_entry_t, "head", &head_offset);

	for(i = 0; i < table_size; i++){

		fbe_transport_get_queue_size(module_name,table_ptr + (i * table_entry_size) + head_offset,trace_func,trace_context,&item_count);

		if(item_count > 0){
			trace_func(trace_context, "Q %d : \n", i );
			status = fbe_debug_display_structure(module_name, trace_func, trace_context, 
												table_ptr + (i * table_entry_size),
												&fbe_metadata_stripe_lock_hash_table_entry_struct_info,
												4 /* fields per line */,
												8 /*spaces_to_indent*/);
		}
	}
	
    return status;	
}
fbe_status_t fbe_base_config_encryption_mode_string_trace(fbe_base_config_encryption_mode_t  mode, 
                                                          fbe_trace_func_t trace_func, 
                                                          fbe_trace_context_t trace_context)
{
    switch(mode)
    {
        case FBE_BASE_CONFIG_ENCRYPTION_MODE_INVALID:
            trace_func(trace_context, "INVALID ");
            break;
        case FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTION_IN_PROGRESS:
            trace_func(trace_context, "ENCRYPTION_IN_PROGRESS ");
            break;
        case FBE_BASE_CONFIG_ENCRYPTION_MODE_REKEY_IN_PROGRESS:
            trace_func(trace_context, "REKEY_IN_PROGRESS ");
            break;
        case FBE_BASE_CONFIG_ENCRYPTION_MODE_ENCRYPTED:
            trace_func(trace_context, "ENCRYPTED");
            break;
        default:
            trace_func(trace_context, "UNKNOWN Encryption Mode 0x%x", (unsigned int)mode);
        break;
    }

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_base_config_encryption_mode_trace(const fbe_u8_t * module_name,
                                                          fbe_dbgext_ptr base_ptr,
                                                          fbe_trace_func_t trace_func,
                                                          fbe_trace_context_t trace_context,
                                                          fbe_debug_field_info_t *field_info_p,
                                                          fbe_u32_t spaces_to_indent) {
    fbe_base_config_encryption_mode_t  mode;

    if (field_info_p->size != sizeof(fbe_base_config_encryption_mode_t)){
        trace_func(trace_context, "%s size is %d not %llu\n",
                   field_info_p->name, field_info_p->size,
		   (unsigned long long)sizeof(fbe_base_config_encryption_mode_t));
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &mode, field_info_p->size);

    trace_func(trace_context, "%s: 0x%x ", field_info_p->name, (unsigned int)mode);

    fbe_base_config_encryption_mode_string_trace(mode, trace_func, trace_context);

    return FBE_STATUS_OK;
}

fbe_status_t fbe_base_config_encryption_state_string_trace(fbe_base_config_encryption_state_t  mode, 
                                                           fbe_trace_func_t trace_func, 
                                                           fbe_trace_context_t trace_context)
{
    switch(mode)
    {
        case FBE_BASE_CONFIG_ENCRYPTION_STATE_INVALID:
            trace_func(trace_context, "INVALID ");
            break;
        case FBE_BASE_CONFIG_ENCRYPTION_STATE_ENCRYPTION_IN_PROGRESS:
            trace_func(trace_context, "ENCRYPTION_IN_PROGRESS ");
            break;
        case FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEYING_IN_PROGRESS:
            trace_func(trace_context, "REKEYING_IN_PROGRESS ");
            break;
        case FBE_BASE_CONFIG_ENCRYPTION_STATE_ENCRYPTED:
            trace_func(trace_context, "ENCRYPTED");
            break;
        case FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED:
            trace_func(trace_context, "KEYS_PROVIDED");
            break;
        case FBE_BASE_CONFIG_ENCRYPTION_STATE_KEYS_PROVIDED_FOR_REKEY:
            trace_func(trace_context, "KEYS_PROVIDED_FOR_REKEY");
            break;
        case FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_KEY_ERROR:
            trace_func(trace_context, "LOCKED_KEY_ERROR");
            break;
        case FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_KEYS_INCORRECT:
            trace_func(trace_context, "LOCKED_KEYS_INCORRECT");
            break;
        case FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_CURRENT_KEYS_REQUIRED:
            trace_func(trace_context, "LOCKED_CURRENT_KEYS_REQUIRED");
            break;
        case FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_NEW_KEYS_REQUIRED:
            trace_func(trace_context, "LOCKED_NEW_KEYS_REQUIRED");
            break;
        case FBE_BASE_CONFIG_ENCRYPTION_STATE_LOCKED_PUSH_KEYS_DOWNSTREAM:
            trace_func(trace_context, "LOCKED_PUSH_KEYS_DOWNSTREAM");
            break;
        case FBE_BASE_CONFIG_ENCRYPTION_STATE_PUSH_KEYS_FOR_REKEY:
            trace_func(trace_context, "PUSH_KEYS_FOR_REKEY");
            break;
        case FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE:
            trace_func(trace_context, "REKEY_COMPLETE");
            break;
        case FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_DONE:
            trace_func(trace_context, "REKEY_COMPLETE_KEYS_REMOVED");
            break;
        case FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_PUSH_KEYS:
            trace_func(trace_context, "REKEY_COMPLETE_PUSH_KEYS");
            break;
		case FBE_BASE_CONFIG_ENCRYPTION_STATE_REKEY_COMPLETE_REMOVE_OLD_KEY:
            trace_func(trace_context, "REKEY_COMPLETE_REMOVE_OLD_KEY");
            break;
        case FBE_BASE_CONFIG_ENCRYPTION_STATE_UNENCRYPTED:
            trace_func(trace_context, "UNENCRYPTED");
            break;
        default:
            trace_func(trace_context, "UNKNOWN Encryption Mode 0x%x", (unsigned int)mode);
        break;
    }

    return FBE_STATUS_OK;
}

static fbe_status_t fbe_base_config_encryption_state_trace(const fbe_u8_t * module_name,
                                                          fbe_dbgext_ptr base_ptr,
                                                          fbe_trace_func_t trace_func,
                                                          fbe_trace_context_t trace_context,
                                                          fbe_debug_field_info_t *field_info_p,
                                                          fbe_u32_t spaces_to_indent) {
    fbe_base_config_encryption_state_t  mode;

    if (field_info_p->size != sizeof(fbe_base_config_encryption_state_t)){
        trace_func(trace_context, "%s size is %d not %llu\n",
                   field_info_p->name, field_info_p->size,
		   (unsigned long long)sizeof(fbe_base_config_encryption_state_t));
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &mode, field_info_p->size);

    trace_func(trace_context, "%s: 0x%x ", field_info_p->name, (unsigned int)mode);

    fbe_base_config_encryption_state_string_trace(mode, trace_func, trace_context);

    return FBE_STATUS_OK;
}
/*************************
 * end file fbe_base_config_debug.c
 *************************/

 
