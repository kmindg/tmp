/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_block_transport_debug.c
 ****************************************************************************
 *
 * @brief
 *  This file contains the debug functions for Block Transport.
 *
 * @author
 *   02/11/2009:  Created. Nishit Singh
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_base_object_debug.h"
#include "fbe_block_transport.h"
#include "fbe_base_object_trace.h"
#include "fbe_logical_drive_debug.h"
#include "fbe_transport_debug.h"
#include "fbe_transport_trace.h"
#include "fbe_block_transport_trace.h"
#include "pp_ext.h"


/*************************
 *   FUNCTION PROTOTYPES
 *************************/
fbe_status_t fbe_base_transport_server_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr object_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_field_info_t *field_info_p,
                                                   fbe_u32_t spaces_to_indent);
fbe_status_t fbe_block_transport_server_debug_trace(const fbe_u8_t * module_name,
                                                    fbe_dbgext_ptr object_ptr,
                                                    fbe_trace_func_t trace_func,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_u32_t spaces_to_indent);
static fbe_status_t fbe_base_edge_path_state_debug_trace(const fbe_u8_t * module_name,
                                                         fbe_dbgext_ptr base_ptr,
                                                         fbe_trace_func_t trace_func,
                                                         fbe_trace_context_t trace_context,
                                                         fbe_debug_field_info_t *field_info_p,
                                                         fbe_u32_t spaces_to_indent);
static fbe_status_t fbe_base_edge_transport_id_debug_trace(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr base_ptr,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_debug_field_info_t *field_info_p,
                                                           fbe_u32_t spaces_to_indent);
static fbe_status_t fbe_base_edge_path_attributes_debug_trace(const fbe_u8_t * module_name,
                                                              fbe_dbgext_ptr base_ptr,
                                                              fbe_trace_func_t trace_func,
                                                              fbe_trace_context_t trace_context,
                                                              fbe_debug_field_info_t *field_info_p,
                                                              fbe_u32_t spaces_to_indent);
/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!*******************************************************************
 * @var fbe_block_transport_server_queue_info
 *********************************************************************
 * @brief This structure holds info used to display a queue of edges.
 *        using the "next_edge".
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_QUEUE_INFO(fbe_base_edge_t, "next_edge", 
                             FBE_TRUE, /* queue element is the first field in the object. */
                             fbe_block_transport_server_queue_info,
                             fbe_block_transport_display_edge);

/*!*******************************************************************
 * @var fbe_block_transport_server_field_info
 *********************************************************************
 * @brief Information about each of the fields of block transport server.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_block_transport_server_field_info[] =
{
    /* BTS is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_transport_server", fbe_u32_t, FBE_TRUE, "%d", fbe_base_transport_server_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO("outstanding_io_count", fbe_atomic_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("outstanding_io_max", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("tag_bitfield", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("capacity", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("default_offset", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("block_transport_const", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("attributes", fbe_block_transport_attributes_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("io_throttle_max", fbe_block_count_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("io_throttle_count", fbe_block_count_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("outstanding_io_credits", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("io_credits_max", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};


/*!*******************************************************************
 * @var fbe_block_transport_server_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        transport server.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_block_transport_server_t,
                              fbe_block_transport_server_struct_info,
                              &fbe_block_transport_server_field_info[0]);

/*!*******************************************************************
 * @var fbe_base_transport_server_field_info
 *********************************************************************
 * @brief Information about each of the fields of base transport server.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_base_transport_server_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO_QUEUE("client_list", fbe_u32_t, FBE_TRUE, &fbe_block_transport_server_queue_info),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_base_transport_server_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        transport server.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_base_transport_server_t,
                              fbe_base_transport_server_struct_info,
                              &fbe_base_transport_server_field_info[0]);

/*!*******************************************************************
 * @def FBE_BASE_EDGE_PATH_ATTRIBUTES_FIELD_INFO_OFFSET
 *********************************************************************
 * @brief Offset of the path attributes in the field info array.
 *
 *********************************************************************/
#define FBE_BASE_EDGE_PATH_ATTRIBUTES_FIELD_INFO_OFFSET 4

/*!*******************************************************************
 * @var fbe_base_edge_field_info
 *********************************************************************
 * @brief Information about each of the fields of base edge.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_base_edge_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("client_id", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("server_id", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("client_index", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("server_index", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("transport_id", fbe_u32_t, FBE_FALSE, "0x%x",
                                    fbe_base_edge_transport_id_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("path_state", fbe_u32_t, FBE_FALSE, "0x%x",
                                    fbe_base_edge_path_state_debug_trace),

    /* IMPORTANT:  IF any other fields are added above this line change 
     *             the FBE_BASE_EDGE_PATH_ATTRIBUTES_FIELD_INFO_OFFSET above. 
     */
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("path_attr", fbe_path_attr_t, FBE_FALSE, "0x%x",
                                    fbe_base_edge_path_attributes_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("hook", fbe_edge_hook_function_t, FBE_FALSE, "0x%x", 
                                    fbe_debug_display_function_ptr),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("next_edge", fbe_base_edge_t, FBE_TRUE, "0x%x", 
                                    fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_base_edge_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base edge.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_base_edge_t,
                              fbe_base_edge_struct_info,
                              &fbe_base_edge_field_info[0]);


/*!*******************************************************************
 * @var fbe_block_edge_field_info
 *********************************************************************
 * @brief Information about each of the fields of block edge.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_block_edge_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("server_package_id", fbe_package_id_t , FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("client_package_id", fbe_package_id_t , FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("capacity", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("offset", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("medic_action_priority", fbe_medic_action_priority_t  , FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("block_edge_geometry", fbe_block_edge_geometry_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("traffic_priority", fbe_traffic_priority_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("time_to_become_readay_in_sec", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_NEWLINE(),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_edge", fbe_base_edge_t, FBE_TRUE, "%d", fbe_base_edge_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_block_edge_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        block edge.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_block_edge_t,
                              fbe_block_edge_struct_info,
                              &fbe_block_edge_field_info[0]);

/*!*******************************************************************
 * @var fbe_block_transport_control_get_edge_info_field_info
 *********************************************************************
 * @brief Information about each of the fields of block transport control edge.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_block_transport_control_get_edge_info_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("capacity", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("offset", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("exported_block_size", fbe_block_size_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("imported_block_size", fbe_block_size_t, FBE_FALSE, "0x%x"),
	
    FBE_DEBUG_DECLARE_FIELD_INFO("edge_p", fbe_u64_t , FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_edge_info", fbe_base_transport_control_get_edge_info_t, FBE_TRUE, "%d", fbe_base_edge_info_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_block_transport_control_get_edge_info_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        block transport control get edge info.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_block_transport_control_get_edge_info_t ,
                              fbe_block_transport_control_get_edge_info_struct_info,
                              &fbe_block_transport_control_get_edge_info_field_info[0]);

/*!*******************************************************************
 * @var fbe_block_transport_control_get_edge_info_field_info
 *********************************************************************
 * @brief Information about each of the fields of block transport control edge.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_base_transport_control_get_edge_info_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("client_id", fbe_object_id_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("server_id", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("client_index", fbe_edge_index_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("server_index", fbe_edge_index_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("path_state", fbe_u32_t, FBE_FALSE, "0x%x",
                                    fbe_base_edge_path_state_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("path_attr", fbe_path_attr_t, FBE_FALSE, "0x%x",
                                    fbe_base_edge_path_attributes_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("transport_id", fbe_u32_t, FBE_FALSE, "0x%x",
                                    fbe_base_edge_transport_id_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_block_transport_control_get_edge_info_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        block transport control get edge info.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_base_transport_control_get_edge_info_t  ,
                              fbe_base_transport_control_get_edge_info_struct_info,
                              &fbe_base_transport_control_get_edge_info_field_info[0]);

/*!**************************************************************
 * fbe_block_transport_display_edge()
 ****************************************************************
 * @brief
 *  Display info on edge
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  8/5/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_block_transport_display_edge(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr edge_p,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;

    trace_func(trace_context, "\tblock_edge: ");
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, 
                                         edge_p,
                                         &fbe_block_edge_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent + 5);
    trace_func(trace_context, "\n");
    return status;
}
/******************************************
 * end fbe_block_transport_display_edge()
 ******************************************/

/*!**************************************************************
 * fbe_base_transport_server_debug_trace()
 ****************************************************************
 * @brief
 *  Display info on base transport server.
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
 *  8/5/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_base_transport_server_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr object_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_field_info_t *field_info_p,
                                                   fbe_u32_t spaces_to_indent)
{
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);

    fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr,
                                &fbe_base_transport_server_struct_info,
                                4 /* fields per line */,
                                spaces_to_indent);
    trace_func(trace_context, "\n");
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_base_transport_server_debug_trace()
 ******************************************/

/*!**************************************************************
 * fbe_block_transport_server_debug_trace()
 ****************************************************************
 * @brief
 *  Display info on block transport server.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  8/5/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_block_transport_server_debug_trace(const fbe_u8_t * module_name,
                                                    fbe_dbgext_ptr object_ptr,
                                                    fbe_trace_func_t trace_func,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    /* Don't indent since the first structure will indent itself.
     */
    trace_func(trace_context, "    Transport server: ");
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr,
                                         &fbe_block_transport_server_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);
    return status;
}
/******************************************
 * end fbe_block_transport_server_debug_trace()
 ******************************************/

/*!**************************************************************
 * @fn fbe_block_transport_print_identify(const fbe_u8_t * module_name,
 *                                        fbe_u64_t block_transport_identify_p,
 *                                        fbe_trace_func_t trace_func,
 *                                        fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display's the identify information..
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param block_transport_identify_p - Ptr to identify.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  02/11/2009 - Created. Nishit Singh
 *
 ****************************************************************/
fbe_status_t fbe_block_transport_print_identify(const fbe_u8_t * module_name,
                                                fbe_u64_t block_transport_identify_p,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context)
{
    fbe_u32_t loop_index;
    char identify_info[FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH];

    FBE_GET_FIELD_DATA(module_name,
                       block_transport_identify_p,
                       fbe_block_transport_identify_t,
                       identify_info,
                       sizeof(char) * FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH,
                       identify_info);

    for (loop_index = 0;
         loop_index < FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH;
         loop_index++)
    {
        trace_func(trace_context, "%02x ", identify_info[loop_index]);
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_block_transport_print_identify()
 ******************************************/
/*!**************************************************************
 * fbe_base_edge_debug_trace()
 ****************************************************************
 * @brief
 *  Display info on base edge.
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
 *  9/7/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_base_edge_debug_trace(const fbe_u8_t * module_name,
                                       fbe_dbgext_ptr object_ptr,
                                       fbe_trace_func_t trace_func,
                                       fbe_trace_context_t trace_context,
                                       fbe_debug_field_info_t *field_info_p,
                                       fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;

    trace_func(trace_context, "%s: ", field_info_p->name);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr,
                                         &fbe_base_edge_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent+2);
    trace_func(trace_context, "\n");
    return status;
}
/******************************************
 * end fbe_base_edge_debug_trace()
 ******************************************/

/*!**************************************************************
 * fbe_block_edge_debug_trace()
 ****************************************************************
 * @brief
 *  Display info on base edge.
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
 *  9/7/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_block_edge_debug_trace(const fbe_u8_t * module_name,
                                       fbe_dbgext_ptr object_ptr,
                                       fbe_trace_func_t trace_func,
                                       fbe_trace_context_t trace_context,
                                       fbe_debug_field_info_t *field_info_p,
                                       fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);

    trace_func(trace_context, "%s: ", field_info_p->name);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, 
                                         object_ptr + field_info_p->offset,
                                         &fbe_block_edge_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent);
    trace_func(trace_context, "\n");
    return status;
}
/******************************************
 * end fbe_block_edge_debug_trace()
 ******************************************/

/*!**************************************************************
 * fbe_base_edge_transport_id_debug_trace()
 ****************************************************************
 * @brief
 *  Display the pointer to this field.  Which is just the
 *  base ptr plus the offset to this point in the structure.
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
 *  9/7/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_base_edge_transport_id_debug_trace(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr base_ptr,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_debug_field_info_t *field_info_p,
                                                           fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_transport_id_t transport_id;
    if (field_info_p->size != sizeof(fbe_transport_id_t))
    {
        trace_func(trace_context, "path state size is %d not %llu\n",
                   field_info_p->size, (unsigned long long)sizeof(fbe_u32_t));
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &transport_id, field_info_p->size);

    trace_func(trace_context, "%s: ", field_info_p->name);
    fbe_transport_print_base_edge_transport_id(transport_id, trace_func, trace_context);
    return status;
}
/**************************************
 * end fbe_base_edge_transport_id_debug_trace
 **************************************/

/*!**************************************************************
 * fbe_base_edge_path_state_debug_trace()
 ****************************************************************
 * @brief
 *  Display the path_state.
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
 *  9/7/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_base_edge_path_state_debug_trace(const fbe_u8_t * module_name,
                                                         fbe_dbgext_ptr base_ptr,
                                                         fbe_trace_func_t trace_func,
                                                         fbe_trace_context_t trace_context,
                                                         fbe_debug_field_info_t *field_info_p,
                                                         fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_path_state_t path_state;
    if (field_info_p->size != sizeof(fbe_path_state_t))
    {
        trace_func(trace_context, "path state size is %d not %llu\n",
                   field_info_p->size, (unsigned long long)sizeof(fbe_path_state_t));
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &path_state, field_info_p->size);

    trace_func(trace_context, "%s: ", field_info_p->name);
    fbe_transport_print_base_edge_path_state(path_state, trace_func, trace_context);
    return status;
}
/**************************************
 * end fbe_base_edge_path_state_debug_trace
 **************************************/

/*!**************************************************************
 * fbe_transport_print_block_edge_path_attributes
 ****************************************************************
 * @brief
 *  Display's the trace information related to path attributes.
 *
 * @param  path_attr -  Current path attributes.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author
 *  9/7/2010 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_transport_print_block_edge_path_attributes(fbe_path_attr_t path_attr,
                                                            fbe_trace_func_t trace_func,
                                                            fbe_trace_context_t trace_context)
{
    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE)
    {
        trace_func(trace_context, "END_OF_LIFE ");
        path_attr &= ~FBE_BLOCK_PATH_ATTR_FLAGS_END_OF_LIFE;
    }
    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_HIBERNATING)
    {
        trace_func(trace_context, "CLIENT_IS_HIBERNATING ");
        path_attr &= ~FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_IS_HIBERNATING;
    }
    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CALL_HOME_END_OF_LIFE)
    {
        trace_func(trace_context, "CALL_HOME_END_OF_LIFE ");
        path_attr &= ~FBE_BLOCK_PATH_ATTR_FLAGS_CALL_HOME_END_OF_LIFE;
    }
    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CALL_HOME_KILL)
    {
        trace_func(trace_context, "CALL_HOME_KILL ");
        path_attr &= ~FBE_BLOCK_PATH_ATTR_FLAGS_CALL_HOME_KILL;
    }
    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CHECK_QUEUED_IO_TIMER)
    {
        trace_func(trace_context, "CHECK_QUEUED_IO_TIMER ");
        path_attr &= ~FBE_BLOCK_PATH_ATTR_FLAGS_CHECK_QUEUED_IO_TIMER;
    }
    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS)
    {
        trace_func(trace_context, "TIMEOUT_ERRORS");
        path_attr &= ~FBE_BLOCK_PATH_ATTR_FLAGS_TIMEOUT_ERRORS;
    }
    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_GRANT)
    {
        trace_func(trace_context, "DOWNLOAD_GRANT");
        path_attr &= ~FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_GRANT;
    }
    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ)
    {
        trace_func(trace_context, "DOWNLOAD_REQ");
        path_attr &= ~FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ;
    }
    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ_FAST_DL)
    {
        trace_func(trace_context, "DOWNLOAD_REQ_FAST_DL");
        path_attr &= ~FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ_FAST_DL;
    }
    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ_TRIAL_RUN)
    {
        trace_func(trace_context, "DOWNLOAD_REQ_TRIAL_RUN");
        path_attr &= ~FBE_BLOCK_PATH_ATTR_FLAGS_CLIENT_DOWNLOAD_REQ_TRIAL_RUN;
    }
    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_HEALTH_CHECK_REQUEST)
    {
        trace_func(trace_context, "HEALTH CHECK REQ");
        path_attr &= ~FBE_BLOCK_PATH_ATTR_FLAGS_HEALTH_CHECK_REQUEST;
    }
    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_LINK_FAULT)
    {
        trace_func(trace_context, "LINK FAULT");
        path_attr &= ~FBE_BLOCK_PATH_ATTR_FLAGS_LINK_FAULT;
    }
    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_DRIVE_FAULT)
    {
        trace_func(trace_context, "DRIVE FAULT");
        path_attr &= ~FBE_BLOCK_PATH_ATTR_FLAGS_DRIVE_FAULT;
    }
    if (path_attr & FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED_NEEDS_REBUILD)
    {
        trace_func(trace_context, "DEGRADED_NEEDS_REBUILD");
        path_attr &= ~FBE_BLOCK_PATH_ATTR_FLAGS_DEGRADED_NEEDS_REBUILD;
    }

    if (path_attr != 0)
    {
        trace_func(trace_context, "remaining path_attr: 0x%x ", path_attr);
    }
    return FBE_STATUS_OK;
}

/****************************************
 * end fbe_transport_print_block_edge_path_attributes()
 ****************************************/

/*!**************************************************************
 * fbe_base_edge_path_attributes_debug_trace()
 ****************************************************************
 * @brief
 *  Display the path attributes.
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
 *  9/7/2010 - Created. Rob Foley
 *
 ****************************************************************/

static fbe_status_t fbe_base_edge_path_attributes_debug_trace(const fbe_u8_t * module_name,
                                                              fbe_dbgext_ptr base_ptr,
                                                              fbe_trace_func_t trace_func,
                                                              fbe_trace_context_t trace_context,
                                                              fbe_debug_field_info_t *field_info_p,
                                                              fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_path_attr_t path_attr;
    fbe_transport_id_t transport_id;
    fbe_u32_t transport_id_offset = fbe_base_edge_field_info[FBE_BASE_EDGE_PATH_ATTRIBUTES_FIELD_INFO_OFFSET].offset;
    fbe_u32_t transport_id_size = fbe_base_edge_field_info[FBE_BASE_EDGE_PATH_ATTRIBUTES_FIELD_INFO_OFFSET].size;

    if (field_info_p->size != sizeof(fbe_path_attr_t))
    {
        trace_func(trace_context, "path attributes size is %d not %llu\n",
                   field_info_p->size, (unsigned long long)sizeof(fbe_path_attr_t));
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &path_attr, field_info_p->size);

    FBE_READ_MEMORY(base_ptr + transport_id_offset, &transport_id, transport_id_size);

    /* Translate the transport id and display the appropriate path attribute flags.
     */
    if (transport_id == FBE_TRANSPORT_ID_BLOCK)
    {
        trace_func(trace_context, "%s: 0x%x ", field_info_p->name, path_attr);
        fbe_transport_print_block_edge_path_attributes(path_attr, trace_func, trace_context);
    }
    else
    {
        trace_func(trace_context, "%s: 0x%x ", field_info_p->name, path_attr);
    }
    return status;
}
/**************************************
 * end fbe_base_edge_path_attributes_debug_trace
 **************************************/

/*!**************************************************************
 * @fn fbe_transport_print_base_edge(const fbe_u8_t * module_name,
                                     fbe_u64_t base_edge_p,
                                     fbe_trace_func_t trace_func,
                                     fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display's information related to base edge.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param base_edge_p - Ptr to base edge object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK 
 *
 * HISTORY:
 *  02/11/2009 - Nishit Singh
 *
 ****************************************************************/
fbe_status_t fbe_transport_print_base_edge(const fbe_u8_t * module_name,
                                           fbe_u64_t base_edge_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context)
{
    fbe_u64_t hook_function_p;
    fbe_object_id_t client_id;
    fbe_object_id_t  server_id;
    fbe_edge_index_t client_index;
    fbe_edge_index_t server_index;
    fbe_path_state_t path_state;
    fbe_path_attr_t  path_attr;
    fbe_transport_id_t transport_id;

    FBE_GET_FIELD_DATA(module_name,
                       base_edge_p,
                       fbe_base_edge_t,
                       hook,
                       sizeof(fbe_u64_t),
                       &hook_function_p);

    FBE_GET_FIELD_DATA(module_name,
                       base_edge_p,
                       fbe_base_edge_t,
                       client_id,
                       sizeof(fbe_object_id_t),
                       &client_id);

    FBE_GET_FIELD_DATA(module_name,
                       base_edge_p,
                       fbe_base_edge_t,
                       server_id,
                       sizeof(fbe_object_id_t),
                       &server_id);

    FBE_GET_FIELD_DATA(module_name,
                       base_edge_p,
                       fbe_base_edge_t,
                       client_index,
                       sizeof(fbe_edge_index_t),
                       &client_index);

    FBE_GET_FIELD_DATA(module_name,
                       base_edge_p,
                       fbe_base_edge_t,
                       server_index,
                       sizeof(fbe_edge_index_t),
                       &server_index);

    FBE_GET_FIELD_DATA(module_name,
                       base_edge_p,
                       fbe_base_edge_t,
                       path_state,
                       sizeof(fbe_path_state_t),
                       &path_state);

    FBE_GET_FIELD_DATA(module_name,
                       base_edge_p,
                       fbe_base_edge_t,
                       path_attr,
                       sizeof(fbe_path_attr_t),
                       &path_attr);
    
    FBE_GET_FIELD_DATA(module_name,
                       base_edge_p,
                       fbe_base_edge_t,
                       transport_id,
                       sizeof(fbe_transport_id_t),
                       &transport_id);

    trace_func(trace_context, "                  hook: 0x%llx ",
	       (unsigned long long)hook_function_p);
    trace_func(trace_context, "client_id: 0x%x ", client_id);
    trace_func(trace_context, "server_id: 0x%x", server_id);
    trace_func(trace_context, " client_index: 0x%x", client_index);
    trace_func(trace_context, " server_index: 0x%x", server_index);
    trace_func(trace_context, "\n");

    trace_func(trace_context, "                  path_state: ");
    fbe_transport_print_base_edge_path_state(path_state,
                                             trace_func,
                                             trace_context);

    trace_func(trace_context, " path_attributes: 0x%x", path_attr);

    trace_func(trace_context, " transport_id: ");
    fbe_transport_print_base_edge_transport_id(transport_id, 
                                               trace_func, 
                                               trace_context);

    trace_func(trace_context, "\n");

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_transport_print_base_edge()
 ******************************************/

/*!**************************************************************
 * @fn fbe_block_transport_print_server_detail(const fbe_u8_t * module_name,
 *                                             fbe_u64_t block_transport_p,
 *                                             fbe_trace_func_t trace_func,
 *                                             fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Displays information about the transport server.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param block_transport_p - Ptr to the block transport server .
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  02/13/2009 - Created. Nishit Singh
 *  
 ****************************************************************/

fbe_status_t fbe_block_transport_print_server_detail(const fbe_u8_t * module_name,
                                                     fbe_u64_t block_transport_p,
                                                     fbe_trace_func_t trace_func,
                                                     fbe_trace_context_t trace_context)

{
    fbe_u32_t offset;
    const int spaces_to_indent = 22;

    fbe_block_transport_server_debug_trace(module_name, block_transport_p, trace_func, trace_context, 2);
    
    /* Display queues when active_queue_display is 'True' 
      */
    if(get_active_queue_display_flag() == FBE_TRUE)
    /* Display the queue information.
     */
    {
        fbe_u64_t queue_p;
        fbe_u32_t item_count = 0;
        fbe_u32_t priority;
        fbe_u32_t index;
        fbe_char_t priority_string[32];
        trace_func(trace_context, "\n");

        /* Process the priority_optional_queue_head queue to display 
         * its detail.
         */
        FBE_GET_FIELD_OFFSET(module_name,
                             fbe_block_transport_server_s,
                             "queue_head",
                             &offset);
        for(priority = FBE_PACKET_PRIORITY_INVALID + 1; priority < FBE_PACKET_PRIORITY_LAST; priority++)
        {
            index = priority - 1;

            if(priority == FBE_PACKET_PRIORITY_LOW)
            {
                strncpy(priority_string, "LOW", 32);
            }
            else if(priority == FBE_PACKET_PRIORITY_NORMAL)
            {
                strncpy(priority_string, "NORMAL", 32);
            }
            else if(priority == FBE_PACKET_PRIORITY_URGENT)
            {
                strncpy(priority_string, "URGENT", 32);
            }
            else
            {
                strncpy(priority_string, "UNKNOWN", 32);
            }
            queue_p =  block_transport_p + offset + (index * sizeof(fbe_queue_head_t));
            fbe_transport_get_queue_size(module_name,
                                         queue_p,
                                         trace_func,
                                         trace_context,
                                         &item_count);
            if (item_count == 0)
            {
                trace_func(trace_context, "        queue_head[%s]: EMPTY\n", priority_string);
            }
            else
            {
                trace_func(trace_context, "        queue_head[%s]: (%d items)\n",
                           priority_string, item_count);
                fbe_transport_print_packet_queue(module_name,
                                                 queue_p,
                                                 trace_func,
                                                 trace_context,
                                                 spaces_to_indent);
            }
        }
    }

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_block_transport_print_server_detail()
 ******************************************/

/*!**************************************************************
 * fbe_transport_print_fbe_packet_block_payload()
 ****************************************************************
 * @brief
 *  Display all the trace information related to block operation
 *
 * @param  block_operation_p - Ptr to block operation payload.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  02/13/2009 - Created. Nishit Singh
 *
 ****************************************************************/
fbe_status_t fbe_tansport_print_fbe_packet_block_payload(const fbe_u8_t * module_name,
                                                         fbe_u64_t block_operation_p,
                                                         fbe_trace_func_t trace_func,
                                                         fbe_trace_context_t trace_context,
                                                         fbe_u32_t spaces_to_indent)
{

    fbe_lba_t lba;
    fbe_payload_block_operation_opcode_t block_opcode;
    fbe_payload_block_operation_flags_t block_flags;
    fbe_block_count_t block_count;
    fbe_block_count_t throttle_count;
    fbe_u32_t io_credits;
    fbe_block_size_t block_size;
    fbe_block_size_t optimum_block_size;
    fbe_u64_t pre_read_desc_p;
    fbe_u32_t repeat_count;

    /* Display the block payload detail.
     */
    FBE_GET_FIELD_DATA(module_name,
                       block_operation_p,
                       fbe_payload_block_operation_t,
                       lba,
                       sizeof(fbe_lba_t),
                       &lba);

    FBE_GET_FIELD_DATA(module_name, 
                       block_operation_p,
                       fbe_payload_block_operation_t,
                       block_opcode,
                       sizeof(fbe_payload_block_operation_opcode_t),
                       &block_opcode);
     
    FBE_GET_FIELD_DATA(module_name, 
                       block_operation_p,
                       fbe_payload_block_operation_t,
                       block_count,
                       sizeof(fbe_block_count_t),
                       &block_count);
    FBE_GET_FIELD_DATA(module_name, 
                       block_operation_p,
                       fbe_payload_block_operation_t,
                       throttle_count,
                       sizeof(fbe_block_count_t),
                       &throttle_count);
    FBE_GET_FIELD_DATA(module_name, 
                       block_operation_p,
                       fbe_payload_block_operation_t,
                       io_credits,
                       sizeof(fbe_u32_t),
                       &io_credits);
     
    FBE_GET_FIELD_DATA(module_name, 
                       block_operation_p,
                       fbe_payload_block_operation_t,
                       block_size,
                       sizeof(fbe_block_size_t),
                       &block_size);
          
    FBE_GET_FIELD_DATA(module_name, 
                       block_operation_p,
                       fbe_payload_block_operation_t,
                       optimum_block_size,
                       sizeof(fbe_block_size_t),
                       &optimum_block_size);
          
    FBE_GET_FIELD_DATA(module_name, 
                       block_operation_p,
                       fbe_payload_block_operation_t,
                       block_flags,
                       sizeof(fbe_payload_block_operation_flags_t),
                       &block_flags);
    {
        fbe_u32_t payload_sg_descriptor_offset, repeat_count_offset;
        FBE_GET_FIELD_OFFSET(module_name, fbe_payload_block_operation_t, "payload_sg_descriptor", 
                             &payload_sg_descriptor_offset);
        FBE_GET_FIELD_OFFSET(module_name, fbe_payload_sg_descriptor_t, "repeat_count", 
                             &repeat_count_offset);
        FBE_READ_MEMORY((block_operation_p + payload_sg_descriptor_offset + repeat_count_offset),
                        &repeat_count,
                        sizeof(fbe_u32_t));
    }

    trace_func(trace_context, " lba: 0x%llx", (unsigned long long)lba);
    trace_func(trace_context, " block_count: 0x%llx",
	       (unsigned long long)block_count);
    trace_func(trace_context, " block_opcode: 0x%x ", block_opcode);
    fbe_transport_print_block_opcode(block_opcode,
                                     trace_func,
                                     trace_context);
    trace_func(trace_context, " block_flags: 0x%x ", block_flags);

    trace_func(trace_context, "\n");

    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, " block_size: 0x%x", block_size);
    trace_func(trace_context, " optimum_block_size: 0x%x", optimum_block_size);
    trace_func(trace_context, " repeat_count: 0x%x", repeat_count);
    trace_func(trace_context, " throttle_count: 0x%llx io_credits: %d", throttle_count, io_credits);
    trace_func(trace_context, "\n");

    /* Read the address of pre_read_desc stored in 
     * fbe_payload_block_operation_t.
     */
    FBE_GET_FIELD_DATA(module_name,
                       block_operation_p,
                       fbe_payload_block_operation_t,
                       pre_read_descriptor,
                       sizeof(fbe_u64_t),
                       &pre_read_desc_p);

    /* Display the pre_read_descriptor information.
     */
    {
        fbe_lba_t pre_read_desc_lba;
        fbe_block_count_t pre_read_desc_block_count;
        fbe_u64_t pre_desc_sg_list_p;

        FBE_GET_FIELD_DATA(module_name, 
                           pre_read_desc_p,
                           fbe_payload_pre_read_descriptor_t,
                           lba,
                           sizeof(fbe_lba_t),
                           &pre_read_desc_lba);
        
        FBE_GET_FIELD_DATA(module_name, 
                           pre_read_desc_p,
                           fbe_payload_pre_read_descriptor_t,
                           block_count,
                           sizeof(fbe_block_count_t),
                           &pre_read_desc_block_count);
        
        FBE_GET_FIELD_DATA(module_name, 
                           pre_read_desc_p,
                           fbe_payload_pre_read_descriptor_t,
                           sg_list,
                           sizeof(fbe_u64_t),
                           &pre_desc_sg_list_p);

        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(trace_context, " pre_read_descriptor:  lba: 0x%llx",
                   (unsigned long long)pre_read_desc_lba);
        trace_func(trace_context, " block_count: 0x%llx",
		   (unsigned long long)pre_read_desc_block_count);
        trace_func(trace_context, " sg_list: 0x%llx\n",
		   (unsigned long long)pre_desc_sg_list_p);
    }

    {
        fbe_u32_t status_offset, status_qualifier_offset;
        fbe_payload_block_operation_status_t status;
        fbe_payload_block_operation_qualifier_t status_qualifier;

        FBE_GET_FIELD_OFFSET(module_name, fbe_payload_block_operation_t, "status", &status_offset);
        FBE_GET_FIELD_OFFSET(module_name, fbe_payload_block_operation_t, "status_qualifier", &status_qualifier_offset);
		

        FBE_READ_MEMORY((block_operation_p + status_offset),
                        &status,
                        sizeof(fbe_u32_t));
        FBE_READ_MEMORY((block_operation_p + status_qualifier_offset),
                        &status_qualifier, 
                        sizeof(fbe_u32_t));

        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(trace_context, " status: 0x%x status_qualifier: 0x%x\n", 
                   status, status_qualifier);
    }

    return FBE_STATUS_OK;

}
/*************************************************
 * end fbe_tansport_print_fbe_packet_block_payload()
 *************************************************/

/*!**************************************************************
 * fbe_transport_print_block_opcode()
 ****************************************************************
 * @brief
 *  Display all the trace information related to block opcode.
 *
 * @param  block_opcode - block operation code.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  02/13/2009 - Created. Nishit Singh
 *
 ****************************************************************/
fbe_status_t fbe_transport_print_block_opcode(fbe_u32_t block_opcode,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context)
{
    switch(block_opcode)
    {
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INVALID:
          trace_func(trace_context, "(invalid)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ:
          trace_func(trace_context, "(read)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE:
          trace_func(trace_context, "(write)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ZERO:
          trace_func(trace_context, "(zero)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CHECK_ZEROED:
          trace_func(trace_context, "(check zeroed)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_FORCE_ZERO:
          trace_func(trace_context, "(force zero)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_BACKGROUND_ZERO:
          trace_func(trace_context, "(background zero)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_SAME:
          trace_func(trace_context, "(write same)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY:
          trace_func(trace_context, "(verify)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WRITE:
          trace_func(trace_context, "(verify write)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_VERIFY:
          trace_func(trace_context, "(write verify)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_WITH_BUFFER:
          trace_func(trace_context, "(verify with buffer)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_REBUILD:
          trace_func(trace_context, "(rebuild)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_IDENTIFY:
          trace_func(trace_context, "(identify)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_NEGOTIATE_BLOCK_SIZE:
          trace_func(trace_context, "(negotiate block size)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY:
          trace_func(trace_context, "(read only verify)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ERROR_VERIFY:
          trace_func(trace_context, "(error verify)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INCOMPLETE_WRITE_VERIFY:
          trace_func(trace_context, "(incomplete write verify)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_SYSTEM_VERIFY:
          trace_func(trace_context, "(system verify)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_MARK_FOR_REBUILD:
          trace_func(trace_context, "(mark for rebuild)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_SPECIFIC_AREA:
          trace_func(trace_context, "(verify specific)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_NONCACHED:
          trace_func(trace_context, "(write noncached)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_CORRUPT_DATA:
          trace_func(trace_context, "(corrupt data)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_HDR_RD:
          trace_func(trace_context, "(write_log header read)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_LOG_FLUSH:
          trace_func(trace_context, "(write_log flush)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_VERIFY_INVALIDATE:
          trace_func(trace_context, "(Verify invalidate)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_RO_VERIFY:
          trace_func(trace_context, "(RO Verify)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_USER_VERIFY:
          trace_func(trace_context, "(User Verify)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_SYSTEM_VERIFY:
          trace_func(trace_context, "(Sys Verify)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_ERROR_VERIFY:
          trace_func(trace_context, "(Err Verify)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INITIATE_INCOMPLETE_WRITE_VERIFY:
          trace_func(trace_context, "(Inc WR Verify)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_INIT_EXTENT_METADATA:
          trace_func(trace_context, "(Init MD)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_MARK_CONSUMED:
          trace_func(trace_context, "(Mark consumed)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_MARK_CONSUMED_ZERO:
          trace_func(trace_context, "(Mark consumed zero)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_READ_PAGED:
          trace_func(trace_context, "(Enc Read Paged)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE:
          trace_func(trace_context, "(Enc Write)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_WRITE_ZEROS:
          trace_func(trace_context, "(Enc Write Zeros)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_REKEY_ZERO:
          trace_func(trace_context, "(Enc Zero)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_ENCRYPTION_SET_CHKPT:
          trace_func(trace_context, "(Enc Set Chkpt)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_WRITE_ZEROS:
          trace_func(trace_context, "(Write zeros)");
          break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_READ_ONLY_VERIFY_SPECIFIC_AREA:
          trace_func(trace_context, "(Read only verify specific)");
            break;
        case FBE_PAYLOAD_BLOCK_OPERATION_OPCODE_LAST:
          trace_func(trace_context, "(last)");
          break;
        default:
          trace_func(trace_context, "UNKNOWN opcode 0x%x", block_opcode);
          break;
    }

    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_transport_print_block_opcode()
 ********************************************/

/*!**************************************************************
 * fbe_block_edge_info_debug_trace()
 ****************************************************************
 * @brief
 *  Display block transport control edge info
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  11/16/2011 - Created. Trupti Ghate
 *
 ****************************************************************/
 fbe_status_t fbe_block_edge_info_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr object_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_field_info_t *field_info_p,
                                                   fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_dbgext_ptr block_edge_info_ptr = object_ptr + field_info_p->offset;
    trace_func(trace_context, "\n");
    trace_func(trace_context, " %s: ", field_info_p->name);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, 
                                         block_edge_info_ptr,
                                         &fbe_block_transport_control_get_edge_info_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent + 2);
    trace_func(trace_context, "\n");

    return status;
}


/********************************************
 * end fbe_block_edge_info_debug_trace()
 ********************************************/

/*!**************************************************************
 * fbe_base_edge_info_debug_trace()
 ****************************************************************
 * @brief
 *  Display base edge info
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  11/16/2011 - Created. Trupti Ghate
 *
 ****************************************************************/
  fbe_status_t fbe_base_edge_info_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr object_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_field_info_t *field_info_p,
                                                   fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_dbgext_ptr base_edge_info_ptr = object_ptr + field_info_p->offset;
    trace_func(trace_context, "\n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);

    trace_func(trace_context, "%s: ", field_info_p->name);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, 
                                         base_edge_info_ptr,
                                         &fbe_base_transport_control_get_edge_info_struct_info,
                                         4 /* fields per line */,
                                         spaces_to_indent + 4);
    trace_func(trace_context, "\n");

    return status;
}

/********************************************
 * end fbe_base_edge_info_debug_trace()
 ********************************************/

/*!**************************************************************
 * fbe_payload_block_print_struct_opcode()
 ****************************************************************
 * @brief
 *  Print out a block opcode that is part of an overriding struct.
 *
 * @param module_name
 * @param base_ptr
 * @param trace_func
 * @param trace_context
 * @param field_info_p
 * @param spaces_to_indent
 * 
 * @return fbe_status_t
 *
 * @author
 *  4/8/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_payload_block_print_struct_opcode(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr base_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_field_info_t *field_info_p,
                                                   fbe_u32_t spaces_to_indent)
{
    fbe_payload_block_operation_opcode_t block_opcode;

    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &block_opcode, field_info_p->size);

    fbe_transport_print_block_opcode(block_opcode, trace_func, trace_context);
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_payload_block_print_struct_opcode()
 ******************************************/
/*!*******************************************************************
 * @var fbe_payload_block_summary_field_info
 *********************************************************************
 * @brief Information about each of the fields of iots.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_payload_block_summary_field_info[] =
{
    /* Just display state info.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("lba", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("block_count", fbe_block_count_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("block_opcode", fbe_payload_block_operation_opcode_t, FBE_FALSE, "0x%x",
                                    fbe_payload_block_print_struct_opcode),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_payload_block_summary_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base config.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_payload_block_operation_t,
                              fbe_payload_block_summary_struct_info,
                              &fbe_payload_block_summary_field_info[0]);

/*!**************************************************************
 * fbe_payload_block_debug_print_summary()
 ****************************************************************
 * @brief
 *  Display a one line summary of the payload block structure.
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param iots_p - ptr to iots to display.
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * @param spaces_to_indent - Spaces to indent this display.
 * 
 * @return fbe_status_t
 *
 * @author
 *  4/8/2013 - Created. Rob Foley
 *
 ****************************************************************/
fbe_status_t fbe_payload_block_debug_print_summary(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr base_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_field_info_t *field_info_p,
                                                   fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_u32_t offset = 0;
    if (field_info_p!= NULL) {
        offset = field_info_p->offset;
    }

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, base_ptr + offset,
                                         &fbe_payload_block_summary_struct_info,
                                         20 /* fields per line */,
                                         0);
    return status;
}
/******************************************
 * end fbe_payload_block_debug_print_summary()
 ******************************************/
/*************************
 * end file fbe_block_transport_debug.c
 *************************/
