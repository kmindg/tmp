/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_metadata_stripe_lock_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the stripe lock metadata.
 *
 * @author
 *  01/31/2011 - Created. Hari Singh
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
#include "fbe_metadata.h"
#include "fbe_metadata_cmi.h"
//#include "fbe_metadata_stripe_lock.h"

#include "fbe_block_transport.h"
#include "fbe_transport_trace.h"
#include "fbe_block_transport_trace.h"

static fbe_status_t fbe_metadata_sl_blob_flags_debug_trace(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr base_ptr,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_debug_field_info_t *field_info_p,
                                                           fbe_u32_t spaces_to_indent);

static fbe_status_t fbe_metadata_sl_slot_flags_debug_trace(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr base_ptr,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_debug_field_info_t *field_info_p,
                                                           fbe_u32_t spaces_to_indent);
static fbe_status_t fbe_metadata_sl_slot_state_debug_trace(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr base_ptr,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_debug_field_info_t *field_info_p,
                                                           fbe_u32_t spaces_to_indent);

/*!*******************************************************************
 * @var fbe_metadata_stripe_lock_blob_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_metadata_stripe_lock_blob_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("flags", fbe_u32_t, FBE_FALSE, "0x%x",
                                    fbe_metadata_sl_blob_flags_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO("metadata_element", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_QUEUE("peer_queue_head", fbe_queue_head_t, FBE_FALSE, 
                                       &fbe_packet_debug_packet_queue_info),
    FBE_DEBUG_DECLARE_FIELD_INFO_QUEUE("reserve_queue_head", fbe_queue_head_t, FBE_FALSE, 
                                       &fbe_packet_debug_packet_queue_info),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_metadata_stripe_lock_blob_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_metadata_stripe_lock_blob_t,
                              fbe_metadata_stripe_lock_blob_struct_info,
                              &fbe_metadata_stripe_lock_blob_field_info[0]);


/*!*******************************************************************
 * @var fbe_metadata_stripe_lock_slot_field_info
 *********************************************************************
 * @brief Information about each of the fields of base config.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_metadata_stripe_lock_slot_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("flags", fbe_u32_t, FBE_FALSE, "0x%x",
                                    fbe_metadata_sl_slot_flags_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO("slot_state", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("slot_state", fbe_u32_t, FBE_FALSE, "0x%x",
                                    fbe_metadata_sl_slot_state_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO("shared_reference_count", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("exclusive_reference_count", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("stripe_number", fbe_u64_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("stripe_count", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_metadata_stripe_lock_slot_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(metadata_stripe_lock_slot_t,
                              fbe_metadata_stripe_lock_slot_struct_info,
                              &fbe_metadata_stripe_lock_slot_field_info[0]);

/*!**************************************************************
 * fbe_metadata_stripe_lock_slot_debug_trace()
 ****************************************************************
 * @brief
 *  Display the metadata stripe lock slot information.
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
 *  12/16/2010 - Created. Hari singh chauhan
 *
 ****************************************************************/
 
fbe_status_t fbe_metadata_stripe_lock_slot_debug_trace(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr object_ptr, /* pointer to mde */
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_debug_field_info_t *field_info_p,
                                              fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_u32_t loop_counter = 0;	
    fbe_u32_t ptr_size;
    fbe_u32_t metadata_stripe_lock_slot_size;
    fbe_u32_t slot_state_size;
    fbe_dbgext_ptr slot_offset_ptr;
	fbe_u64_t blob_p;
	fbe_u32_t slot_offset;

    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) {
        trace_func(trace_context, "%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return status; 
    }

	trace_func(trace_context, "object %llX, offset %x\n",(unsigned long long)object_ptr, field_info_p->offset);

	FBE_READ_MEMORY(object_ptr + field_info_p->offset, &blob_p, ptr_size); /* Get blob pointer */

	trace_func(trace_context, "blob_p %llX\n",(unsigned long long)blob_p);

    FBE_GET_TYPE_SIZE(module_name, metadata_stripe_lock_slot_t, &metadata_stripe_lock_slot_size);
    FBE_GET_TYPE_SIZE(module_name, metadata_lock_slot_state_t, &slot_state_size);
	FBE_GET_FIELD_OFFSET(module_name, fbe_metadata_stripe_lock_blob_t, "slot", &slot_offset);

	fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "%s:\n", field_info_p->name);

    if (blob_p==NULL) {
        return FBE_STATUS_OK;
    }

    if (FBE_CHECK_CONTROL_C()) {
        return FBE_STATUS_GENERIC_FAILURE;
    }

    for(loop_counter = 0; loop_counter < 0 /*METADATA_STRIPE_LOCK_MAX_SLOTS*/; ++loop_counter)
    {
        if (FBE_CHECK_CONTROL_C()) {
            return FBE_STATUS_GENERIC_FAILURE;
        }

        slot_offset_ptr = (fbe_dbgext_ptr)(blob_p + slot_offset + loop_counter * metadata_stripe_lock_slot_size);


        fbe_trace_indent(trace_func, trace_context, spaces_to_indent+2);
        trace_func(trace_context, "slot %2d) ", loop_counter);

        status = fbe_debug_display_structure(module_name, trace_func, trace_context, slot_offset_ptr,
                                             &fbe_metadata_stripe_lock_slot_struct_info,
                                             4 /* fields per line */,
                                             spaces_to_indent + 11);
        if (status != FBE_STATUS_OK) { return status; }
        trace_func(trace_context, "\n");
    }

    fbe_trace_indent(trace_func, trace_context, spaces_to_indent+2);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, 
                                         (fbe_dbgext_ptr)blob_p,
                                         &fbe_metadata_stripe_lock_blob_struct_info,
                                         4    /* fields per line */,
                                         spaces_to_indent+2    /*spaces_to_indent*/);
    
    return FBE_STATUS_OK;
}

/*!*******************************************************************
 * @struct fbe_metadata_sl_slot_flags_strings
 *********************************************************************
 * @brief Provides a mapping of enum to string for metadata_lock_slot_flags_e
 *
 *********************************************************************/
fbe_debug_enum_trace_info_t fbe_metadata_sl_slot_flags_strings[] = 
{
#if 0
    {METADATA_STRIPE_LOCK_SLOT_FLAG_PEER_SHARED_REQUEST, "PEER_SHARED_REQUEST"},
    {METADATA_STRIPE_LOCK_SLOT_FLAG_PEER_EXCLUSIVE_REQUEST, "PEER_EXCLUSIVE_REQUEST"},
    {METADATA_STRIPE_LOCK_SLOT_FLAG_SHARED_REQUEST, "SHARED_REQUEST"},
    {METADATA_STRIPE_LOCK_SLOT_FLAG_EXCLUSIVE_REQUEST, "EXCLUSIVE_REQUEST"},
    {METADATA_STRIPE_LOCK_SLOT_FLAG_SHARED_PENDING, "SHARED_PENDING"},
    {METADATA_STRIPE_LOCK_SLOT_FLAG_EXCLUSIVE_PENDING, "EXCLUSIVE_PENDING"},
#endif
    FBE_DEBUG_ENUM_TRACE_INFO_TERMINATOR,
};

/*!*******************************************************************
 * @struct fbe_metadata_sl_slot_state_strings
 *********************************************************************
 * @brief Provides a mapping of enum to string metadata_lock_slot_state_t
 *
 *********************************************************************/
fbe_debug_enum_trace_info_t fbe_metadata_sl_slot_state_strings[] = 
{
    {METADATA_STRIPE_LOCK_SLOT_STATE_SHARED,          "PEER_SHARED_REQUEST"},
    {METADATA_STRIPE_LOCK_SLOT_STATE_EXCLUSIVE_LOCAL, "EXCLUSIVE_LOCAL"},
    {METADATA_STRIPE_LOCK_SLOT_STATE_EXCLUSIVE_PEER,  "EXCLUSIVE_PEER"},
    FBE_DEBUG_ENUM_TRACE_INFO_TERMINATOR,
};

/*!*******************************************************************
 * @struct fbe_metadata_sl_blob_flags_strings
 *********************************************************************
 * @brief Provides a mapping of enum to string metadata_lock_blob_flags_e
 *
 *********************************************************************/
fbe_debug_enum_trace_info_t fbe_metadata_sl_blob_flags_strings[] = 
{
#if 0
    {METADATA_STRIPE_LOCK_BLOB_FLAG_PEER_RESERVE_REQUEST, "PEER_RESERVE_REQUEST"},
    {METADATA_STRIPE_LOCK_BLOB_FLAG_RESERVE_REQUEST, "RESERVE_REQUEST"},
    {METADATA_STRIPE_LOCK_BLOB_FLAG_RESERVE_PENDING, "RESERVE_PENDING"},
    {METADATA_STRIPE_LOCK_BLOB_FLAG_RESERVE_GRANTED, "RESERVE_GRANTED"},
    {METADATA_STRIPE_LOCK_BLOB_FLAG_RESERVE_IN_PROGRESS, "RESERVE_IN_PROGRESS"},
    {METADATA_STRIPE_LOCK_BLOB_FLAG_RESERVE_DONE, "RESERVE_DONE"},
#endif
    FBE_DEBUG_ENUM_TRACE_INFO_TERMINATOR,
};

/*!**************************************************************
 * fbe_metadata_sl_blob_flags_debug_trace()
 ****************************************************************
 * @brief
 *  Display a stripe lock slot flag.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param base_ptr - This is the ptr to the structure this field.
 *                   is located inside of.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function.
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 *
 * @return - FBE_STATUS_OK  
 *
 * @author
 *  9/15/2011 - Created. Rob Foley
 *
 ****************************************************************/
static fbe_status_t fbe_metadata_sl_blob_flags_debug_trace(const fbe_u8_t * module_name,
                                                     fbe_dbgext_ptr base_ptr,
                                                     fbe_trace_func_t trace_func,
                                                     fbe_trace_context_t trace_context,
                                                     fbe_debug_field_info_t *field_info_p,
                                                     fbe_u32_t spaces_to_indent)
{
    return fbe_debug_trace_flags_strings(module_name, base_ptr, trace_func, trace_context, 
                                         field_info_p, spaces_to_indent, fbe_metadata_sl_blob_flags_strings);
}
/********************************************
 * end fbe_metadata_sl_blob_flags_debug_trace()
 ********************************************/
/*!**************************************************************
 * fbe_metadata_sl_slot_flags_debug_trace()
 ****************************************************************
 * @brief
 *  Display the sl slot flags field.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param base_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  9/15/2011 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_metadata_sl_slot_flags_debug_trace(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr base_ptr,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_debug_field_info_t *field_info_p,
                                                           fbe_u32_t spaces_to_indent)
{
    return fbe_debug_trace_flags_strings(module_name, base_ptr, trace_func, trace_context, 
                                         field_info_p, spaces_to_indent, fbe_metadata_sl_slot_flags_strings);
}
/**************************************
 * end fbe_metadata_sl_slot_flags_debug_trace()
 **************************************/
/*!**************************************************************
 * fbe_metadata_sl_slot_state_debug_trace()
 ****************************************************************
 * @brief
 *  Display the sl slot state field.
 *
 * @param module_name - Module name to use in symbol lookup.
 * @param base_ptr - ptr to value to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param field_info_p - Info for getting and displaying the field. 
 * @param spaces_to_indent - Number of spaces to indent the next line(s).
 * 
 * @return fbe_status_t   
 *
 * @author
 *  9/15/2011 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_metadata_sl_slot_state_debug_trace(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr base_ptr,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_debug_field_info_t *field_info_p,
                                                           fbe_u32_t spaces_to_indent)
{
    return fbe_debug_trace_enum_strings(module_name, base_ptr, trace_func, trace_context, 
                                        field_info_p, spaces_to_indent, fbe_metadata_sl_slot_state_strings);
}
/**************************************
 * end fbe_metadata_sl_slot_state_debug_trace()
 **************************************/
/*************************
 * end file fbe_metadata_stripe_lock_debug.c
 *************************/
