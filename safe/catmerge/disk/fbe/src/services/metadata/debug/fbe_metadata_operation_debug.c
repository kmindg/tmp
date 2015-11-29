/***************************************************************************
 * Copyright (C) EMC Corporation 2010-2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_metadata_operation_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the metadata operation.
 *
 * @author
 *  02/21/2011 - Created. Omer Miranda
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
#include "fbe_metadata_debug.h"



/*!*******************************************************************
 * @var fbe_metadata_paged_blob_sg_list_debug_field_info
 *********************************************************************
 * @brief Information about each of the fields of fbe_sg_element_t.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_metadata_paged_blob_sg_list_debug_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("count", fbe_u32_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("address", fbe_u8_t, FBE_FALSE, "0x%x",
                                    fbe_debug_display_field_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_metadata_paged_blob_sg_list_debug_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        metadata paged blob sg_list structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_sg_element_t,
                              fbe_metadata_paged_blob_sg_list_debug_struct_info,
                              &fbe_metadata_paged_blob_sg_list_debug_field_info[0]);


/*!**************************************************************
 * fbe_metadata_paged_blob_sg_element_debug_trace()
 ****************************************************************
 * @brief
 *  Display the metadata paged blob sg element information.
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
 *  03/14/2011 - Created. Omer Miranda
 *
 ****************************************************************/ 
fbe_status_t fbe_metadata_paged_blob_sg_element_debug_trace(const fbe_u8_t * module_name,
                                                            fbe_dbgext_ptr sg_list_ptr,
                                                            fbe_trace_func_t trace_func,
                                                            fbe_trace_context_t trace_context,
                                                            fbe_debug_field_info_t *field_info_p,
                                                            fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, sg_list_ptr,
                                         &fbe_metadata_paged_blob_sg_list_debug_struct_info,
                                         4    /* fields per line */,
                                         spaces_to_indent + 4);	

    return status;
}

/*!*******************************************************************
 * @var fbe_metadata_paged_blob_debug_field_info
 *********************************************************************
 * @brief Information about each of the fields of metadata paged blob.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_metadata_paged_blob_debug_field_info[] =
{
    
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("metadata_element", fbe_metadata_element_t, FBE_FALSE, "0x%x",
                                    fbe_debug_display_field_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO("lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("block_count", fbe_block_count_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("flags", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("state", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("stripe_offset", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("stripe_count", fbe_block_count_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("sg_list", fbe_sg_element_t, FBE_FALSE, "0x%x",
                                    fbe_metadata_paged_blob_sg_element_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_metadata_paged_blob_debug_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        metadata paged blob structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(metadata_paged_blob_t,
                              fbe_metadata_paged_blob_debug_struct_info,
                              &fbe_metadata_paged_blob_debug_field_info[0]);

/*!**************************************************************
 * fbe_metadata_paged_blob_debug_trace()
 ****************************************************************
 * @brief
 *  Display all the trace information related to metadata paged
 *  blob structure.
 *
 * @param  paged_blob_ptr - Ptr to metadata paged blob structure.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author
 *  03/04/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_metadata_paged_blob_debug_trace(const fbe_u8_t * module_name,
                                                  fbe_dbgext_ptr paged_blob_ptr,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context,
                                                  fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, paged_blob_ptr,
                                         &fbe_metadata_paged_blob_debug_struct_info,
                                         4    /* fields per line */,
                                         12 /*spaces_to_indent */);
    trace_func(trace_context, "\n");
    return status;
}

/*!*******************************************************************
 * @var fbe_metadata_operation_metadata_debug_field_info
 *********************************************************************
 * @brief Information about each of the fields of metadata operation metadata.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_metadata_operation_metadata_debug_field_info[] =
{
    /* Offset is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("offset", fbe_u64_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("record_data", fbe_u8_t, FBE_FALSE, "0x%x",
                                    fbe_debug_display_field_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO("record_data_size", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("repeat_count", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("client_blob", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("stripe_offset", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("stripe_count", fbe_block_count_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("current_count_private", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_metadata_operation_metadata_debug_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        metadata operation metadata structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_payload_metadata_operation_metadata_t,
                              fbe_metadata_operation_metadata_debug_struct_info,
                              &fbe_metadata_operation_metadata_debug_field_info[0]);

/*!**************************************************************
 * fbe_metadata_operation_metadata_debug_trace()
 ****************************************************************
 * @brief
 *  Display all the trace information related to metadata operation
 *  metadata
 *
 * @param  metadata_operation_p - Ptr to block operation payload.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author
 *  02/18/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_metadata_operation_metadata_debug_trace(const fbe_u8_t * module_name,
                                                         fbe_u64_t metadata_p,
                                                         fbe_trace_func_t trace_func,
                                                         fbe_trace_context_t trace_context,
                                                         fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 2);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, metadata_p,
                                         &fbe_metadata_operation_metadata_debug_struct_info,
                                         4    /* fields per line */,
                                         spaces_to_indent + 2);
    trace_func(trace_context, "\n");
    return status;
}
/*************************************************
 * end fbe_metadata_operation_metadata_debug_trace()
 *************************************************/

/*!*******************************************************************
 * @var fbe_metadata_write_log_debug_field_info
 *********************************************************************
 * @brief Information about each of the fields of metadata write log.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_metadata_write_log_debug_field_info[] =
{
    /* Offset is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("original_lba", fbe_lba_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("data", fbe_u8_t, FBE_FALSE, "0x%x",
                                    fbe_debug_display_field_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO("data_size", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("slot_index", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("blob_index", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_metadata_write_log_debug_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        metadata write log structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_payload_metadata_operation_write_log_t,
                              fbe_metadata_write_log_debug_struct_info,
                              &fbe_metadata_write_log_debug_field_info[0]);


/*!**************************************************************
 * fbe_metadata_operation_write_log_debug_trace()
 ****************************************************************
 * @brief
 *  Display all the trace information related of metadata write
 *  log
 *
 * @param  metadata_write_log_p - Ptr to metadata write log structure.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author
 *  03/10/2011 - Created. Omer Miranda
 *
 ****************************************************************/
fbe_status_t fbe_metadata_operation_write_log_debug_trace(const fbe_u8_t * module_name,
                                                          fbe_u64_t metadata_write_log_p,
                                                          fbe_trace_func_t trace_func,
                                                          fbe_trace_context_t trace_context,
                                                          fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, metadata_write_log_p,
                                         &fbe_metadata_write_log_debug_struct_info,
                                         4    /* fields per line */,
                                         spaces_to_indent + 4);
    trace_func(trace_context, "\n");
    return status;
}
/*************************************************
 * end fbe_metadata_operation_metadata_debug_trace()
 *************************************************/

/*************************
 * end file fbe_metadata_operation_debug.c
 *************************/


