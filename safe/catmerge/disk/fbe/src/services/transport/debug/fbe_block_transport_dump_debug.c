
/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_block_transport_dump_debug.c
 ****************************************************************************
 *
 * @brief
 *  This file contains the debug functions for dump Block Transport to backup file.
 *
 * @author
 *   8/17/2012:  Created. Jingcheng Zhang for configuration backup
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
fbe_status_t fbe_base_transport_server_dump_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr object_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_field_info_t *field_info_p,
                                                   fbe_u32_t spaces_to_indent);
fbe_status_t fbe_block_transport_server_dump_debug_trace(const fbe_u8_t * module_name,
                                                    fbe_dbgext_ptr object_ptr,
                                                    fbe_trace_func_t trace_func,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_u32_t spaces_to_indent);
static fbe_status_t fbe_base_edge_path_state_dump_debug_trace(const fbe_u8_t * module_name,
                                                         fbe_dbgext_ptr base_ptr,
                                                         fbe_trace_func_t trace_func,
                                                         fbe_trace_context_t trace_context,
                                                         fbe_debug_field_info_t *field_info_p,
                                                         fbe_u32_t spaces_to_indent);
static fbe_status_t fbe_base_edge_transport_id_dump_debug_trace(const fbe_u8_t * module_name,
                                                           fbe_dbgext_ptr base_ptr,
                                                           fbe_trace_func_t trace_func,
                                                           fbe_trace_context_t trace_context,
                                                           fbe_debug_field_info_t *field_info_p,
                                                           fbe_u32_t spaces_to_indent);
static fbe_status_t fbe_base_edge_path_attributes_dump_debug_trace(const fbe_u8_t * module_name,
                                                              fbe_dbgext_ptr base_ptr,
                                                              fbe_trace_func_t trace_func,
                                                              fbe_trace_context_t trace_context,
                                                              fbe_debug_field_info_t *field_info_p,
                                                              fbe_u32_t spaces_to_indent);

fbe_status_t fbe_block_transport_dump_client_edge(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr edge_p,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_u32_t spaces_to_indent);

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

fbe_u32_t fbe_dump_client_edge_index = 0;

/*!*******************************************************************
 * @var fbe_block_transport_server_queue_info
 *********************************************************************
 * @brief This structure holds info used to display a queue of edges.
 *        using the "next_edge".
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_QUEUE_INFO(fbe_base_edge_t, "next_edge", 
                             FBE_TRUE, /* queue element is the first field in the object. */
                             fbe_block_transport_server_dump_queue_info,
                             fbe_block_transport_dump_client_edge);

/*!*******************************************************************
 * @var fbe_block_transport_server_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of block transport server.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_block_transport_server_dump_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_transport_server", fbe_u32_t, FBE_TRUE, "%d", fbe_base_transport_server_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};


/*!*******************************************************************
 * @var fbe_block_transport_server_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        transport server.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_block_transport_server_t,
                              fbe_block_transport_server_dump_struct_info,
                              &fbe_block_transport_server_dump_field_info[0]);

/*!*******************************************************************
 * @var fbe_base_transport_server_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of base transport server.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_base_transport_server_dump_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO_QUEUE("client_list", fbe_u32_t, FBE_TRUE, &fbe_block_transport_server_dump_queue_info),
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
                              fbe_base_transport_server_dump_struct_info,
                              &fbe_base_transport_server_dump_field_info[0]);

/*!*******************************************************************
 * @def FBE_BASE_EDGE_PATH_ATTRIBUTES_FIELD_INFO_OFFSET
 *********************************************************************
 * @brief Offset of the path attributes in the field info array.
 *
 *********************************************************************/
#define FBE_BASE_EDGE_PATH_ATTRIBUTES_FIELD_INFO_OFFSET 4

/*!*******************************************************************
 * @var fbe_base_edge_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of base edge.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_base_edge_dump_field_info[] =
{
    /* Object id is first.
     */
    FBE_DEBUG_DECLARE_FIELD_INFO("client_id", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("server_id", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("client_index", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("server_index", fbe_u32_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("transport_id", fbe_u32_t, FBE_FALSE, "0x%x",
                                    fbe_base_edge_transport_id_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("path_state", fbe_u32_t, FBE_FALSE, "0x%x",
                                    fbe_base_edge_path_state_dump_debug_trace),

    /* IMPORTANT:  IF any other fields are added above this line change 
     *             the FBE_BASE_EDGE_PATH_ATTRIBUTES_FIELD_INFO_OFFSET above. 
     */
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("path_attr", fbe_path_attr_t, FBE_FALSE, "0x%x",
                                    fbe_base_edge_path_attributes_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("hook", fbe_edge_hook_function_t, FBE_FALSE, "0x%x", 
                                    fbe_debug_display_function_ptr),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("next_edge", fbe_base_edge_t, FBE_TRUE, "0x%x", 
                                    fbe_debug_display_pointer),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_base_edge_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        base edge.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_base_edge_t,
                              fbe_base_edge_dump_struct_info,
                              &fbe_base_edge_dump_field_info[0]);


/*!*******************************************************************
 * @var fbe_block_edge_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of block edge.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_block_edge_dump_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("server_package_id", fbe_package_id_t , FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("client_package_id", fbe_package_id_t , FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("capacity", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("offset", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("medic_action_priority", fbe_medic_action_priority_t  , FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("block_edge_geometry", fbe_block_edge_geometry_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("traffic_priority", fbe_traffic_priority_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("time_to_become_readay_in_sec", fbe_u64_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_edge", fbe_base_edge_t, FBE_TRUE, "%d", fbe_base_edge_dump_debug_trace),
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
                              fbe_block_edge_dump_struct_info,
                              &fbe_block_edge_dump_field_info[0]);

/*!*******************************************************************
 * @var fbe_block_transport_control_get_edge_info_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of block transport control edge.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_block_transport_control_get_edge_info_dump_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("capacity", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("offset", fbe_lba_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("exported_block_size", fbe_block_size_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("imported_block_size", fbe_block_size_t, FBE_FALSE, "0x%x"),
	
    FBE_DEBUG_DECLARE_FIELD_INFO("edge_p", fbe_u64_t , FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("base_edge_info", fbe_base_transport_control_get_edge_info_t, FBE_TRUE, "%d", fbe_base_edge_info_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_block_transport_control_get_edge_info_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        block transport control get edge info.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_block_transport_control_get_edge_info_t ,
                              fbe_block_transport_control_get_edge_info_dump_struct_info,
                              &fbe_block_transport_control_get_edge_info_dump_field_info[0]);

/*!*******************************************************************
 * @var fbe_base_transport_control_get_edge_info_dump_field_info
 *********************************************************************
 * @brief Information about each of the fields of block transport control edge.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_base_transport_control_get_edge_info_dump_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO("client_id", fbe_object_id_t, FBE_TRUE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("server_id", fbe_object_id_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("client_index", fbe_edge_index_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO("server_index", fbe_edge_index_t, FBE_FALSE, "0x%x"),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("path_state", fbe_u32_t, FBE_FALSE, "0x%x",
                                    fbe_base_edge_path_state_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("path_attr", fbe_path_attr_t, FBE_FALSE, "0x%x",
                                    fbe_base_edge_path_attributes_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("transport_id", fbe_u32_t, FBE_FALSE, "0x%x",
                                    fbe_base_edge_transport_id_dump_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_base_transport_control_get_edge_info_dump_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        block transport control get edge info.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_base_transport_control_get_edge_info_t  ,
                              fbe_base_transport_control_get_edge_info_dump_struct_info,
                              &fbe_base_transport_control_get_edge_info_dump_field_info[0]);

/*!**************************************************************
 * fbe_block_transport_dump_edge()
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
 *  8/17/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_block_transport_dump_edge(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr edge_p,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;

    status = fbe_debug_display_structure(module_name, trace_func, trace_context, 
                                         edge_p,
                                         &fbe_block_edge_dump_struct_info,
                                         1 /* fields per line */,
                                         0);
    return status;
}
/******************************************
 * end fbe_block_transport_dump_edge()
 ******************************************/

/*!**************************************************************
 * fbe_base_transport_server_dump_debug_trace()
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
 *  8/17/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_base_transport_server_dump_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_dbgext_ptr object_ptr,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_debug_field_info_t *field_info_p,
                                                   fbe_u32_t spaces_to_indent)
{
    fbe_dump_debug_trace_prefix_t new_prefix;

    fbe_debug_dump_generate_prefix(&new_prefix, (fbe_dump_debug_trace_prefix_t *)trace_context,
                                   field_info_p->name);
    fbe_dump_client_edge_index = 0;

    fbe_debug_display_structure(module_name, trace_func, &new_prefix, object_ptr,
                                &fbe_base_transport_server_dump_struct_info,
                                1 /* fields per line */,
                                0);
    fbe_dump_client_edge_index = 0;

    trace_func(NULL, "\n");
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_base_transport_server_debug_trace()
 ******************************************/

/*!**************************************************************
 * fbe_block_transport_server_dump_debug_trace()
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
 *  8/17/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_block_transport_server_dump_debug_trace(const fbe_u8_t * module_name,
                                                    fbe_dbgext_ptr object_ptr,
                                                    fbe_trace_func_t trace_func,
                                                    fbe_trace_context_t trace_context,
                                                    fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    /* Don't indent since the first structure will indent itself.
     */
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, object_ptr,
                                         &fbe_block_transport_server_dump_struct_info,
                                         1 /* fields per line */,
                                         0);
    return status;
}
/******************************************
 * end fbe_block_transport_server_debug_trace()
 ******************************************/

/*!**************************************************************
 * @fn fbe_block_transport_dump_identify(const fbe_u8_t * module_name,
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
fbe_status_t fbe_block_transport_dump_identify(const fbe_u8_t * module_name,
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
 * fbe_base_edge_dump_debug_trace()
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
 *  8/17/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_base_edge_dump_debug_trace(const fbe_u8_t * module_name,
                                       fbe_dbgext_ptr object_ptr,
                                       fbe_trace_func_t trace_func,
                                       fbe_trace_context_t trace_context,
                                       fbe_debug_field_info_t *field_info_p,
                                       fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_dump_debug_trace_prefix_t new_prefix;

    fbe_debug_dump_generate_prefix(&new_prefix, (fbe_dump_debug_trace_prefix_t *)trace_context,
                                   field_info_p->name);

    status = fbe_debug_display_structure(module_name, trace_func, &new_prefix, object_ptr,
                                         &fbe_base_edge_dump_struct_info,
                                         1 /* fields per line */,
                                         0);
    trace_func(NULL, "\n");

    return status;
}
/******************************************
 * end fbe_base_edge_dump_debug_trace()
 ******************************************/

/*!**************************************************************
 * fbe_block_edge_dump_debug_trace()
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
 *  8/17/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_block_edge_dump_debug_trace(const fbe_u8_t * module_name,
                                       fbe_dbgext_ptr object_ptr,
                                       fbe_trace_func_t trace_func,
                                       fbe_trace_context_t trace_context,
                                       fbe_debug_field_info_t *field_info_p,
                                       fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);

    trace_func(trace_context, "%s: ", field_info_p->name);
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, 
                                         object_ptr + field_info_p->offset,
                                         &fbe_block_edge_dump_struct_info,
                                         1 /* fields per line */,
                                         spaces_to_indent);
    trace_func(trace_context, "\n");
    return status;
}
/******************************************
 * end fbe_block_edge_dump_debug_trace()
 ******************************************/

/*!**************************************************************
 * fbe_base_edge_transport_id_dump_debug_trace()
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
 *  8/17/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/

static fbe_status_t fbe_base_edge_transport_id_dump_debug_trace(const fbe_u8_t * module_name,
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
        trace_func(trace_context, "path state size is %d not %d\n",
                   field_info_p->size, (int)sizeof(fbe_u32_t));
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &transport_id, field_info_p->size);

    trace_func(trace_context, "%s: 0x%x ", field_info_p->name, transport_id);
    fbe_transport_print_base_edge_transport_id(transport_id, trace_func, NULL);

    return status;
}
/**************************************
 * end fbe_base_edge_transport_id_dump_debug_trace
 **************************************/

/*!**************************************************************
 * fbe_base_edge_path_state_dump_debug_trace()
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
 *  8/17/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/

static fbe_status_t fbe_base_edge_path_state_dump_debug_trace(const fbe_u8_t * module_name,
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
        trace_func(trace_context, "path state size is %d not %d\n",
                   field_info_p->size, (int)sizeof(fbe_path_state_t));
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &path_state, field_info_p->size);

    trace_func(trace_context, "%s: 0x%x ", field_info_p->name, path_state);
    fbe_transport_print_base_edge_path_state(path_state, trace_func, NULL);

    return status;
}
/**************************************
 * end fbe_base_edge_path_state_dump_debug_trace
 **************************************/

/*!**************************************************************
 * fbe_transport_dump_block_edge_path_attributes
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
 *  8/17/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_transport_dump_block_edge_path_attributes(fbe_path_attr_t path_attr,
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
 * end fbe_transport_dump_block_edge_path_attributes()
 ****************************************/

/*!**************************************************************
 * fbe_base_edge_path_attributes_dump_debug_trace()
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
 *  8/17/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/

static fbe_status_t fbe_base_edge_path_attributes_dump_debug_trace(const fbe_u8_t * module_name,
                                                              fbe_dbgext_ptr base_ptr,
                                                              fbe_trace_func_t trace_func,
                                                              fbe_trace_context_t trace_context,
                                                              fbe_debug_field_info_t *field_info_p,
                                                              fbe_u32_t spaces_to_indent)
{
    fbe_status_t status = FBE_STATUS_OK;
    fbe_path_attr_t path_attr;
    fbe_transport_id_t transport_id;
    fbe_u32_t transport_id_offset = fbe_base_edge_dump_field_info[FBE_BASE_EDGE_PATH_ATTRIBUTES_FIELD_INFO_OFFSET].offset;
    fbe_u32_t transport_id_size = fbe_base_edge_dump_field_info[FBE_BASE_EDGE_PATH_ATTRIBUTES_FIELD_INFO_OFFSET].size;

    if (field_info_p->size != sizeof(fbe_path_attr_t))
    {
        trace_func(trace_context, "path attributes size is %d not %d\n",
                   field_info_p->size, (int)sizeof(fbe_path_attr_t));
        return FBE_STATUS_GENERIC_FAILURE; 
    }
    FBE_READ_MEMORY(base_ptr + field_info_p->offset, &path_attr, field_info_p->size);

    FBE_READ_MEMORY(base_ptr + transport_id_offset, &transport_id, transport_id_size);

    /* Translate the transport id and display the appropriate path attribute flags.
     */
    if (transport_id == FBE_TRANSPORT_ID_BLOCK)
    {
        trace_func(trace_context, "%s: 0x%x ", field_info_p->name, path_attr);
        fbe_transport_dump_block_edge_path_attributes(path_attr, trace_func, NULL);
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
 * @fn fbe_block_transport_dump_server_detail(const fbe_u8_t * module_name,
 *                                             fbe_u64_t block_transport_p,
 *                                             fbe_trace_func_t trace_func,
 *                                             fbe_trace_context_t trace_context,
 *                                             int spaces_to_indent)
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
 *  8/17/2012 - Created. Jingcheng Zhang
 *  
 ****************************************************************/

fbe_status_t fbe_block_transport_dump_server_detail(const fbe_u8_t * module_name,
                                                     fbe_u64_t block_transport_p,
                                                     fbe_trace_func_t trace_func,
                                                     fbe_trace_context_t trace_context,
                                                     int spaces_to_indent)

{
    fbe_u64_t outstanding_io_count;
    fbe_u32_t outstanding_io_max;
    fbe_u64_t attributes;
    fbe_u32_t tag_bitfield;
    fbe_u64_t block_transport_const_p;
    fbe_dump_debug_trace_prefix_t new_prefix;

    fbe_debug_dump_generate_prefix(&new_prefix, (fbe_dump_debug_trace_prefix_t *)trace_context,
                                   "block_transport_server");


    /* Display the transport server information.
     */
    FBE_GET_FIELD_DATA(module_name,
                       block_transport_p,
                       fbe_block_transport_server_s,
                       outstanding_io_count,
                       sizeof(fbe_atomic_t),
                       &outstanding_io_count);

    FBE_GET_FIELD_DATA(module_name,
                        block_transport_p,
                        fbe_block_transport_server_s,
                        outstanding_io_max,
                        sizeof(fbe_u32_t),
                        &outstanding_io_max);

    FBE_GET_FIELD_DATA(module_name,
                       block_transport_p,
                       fbe_block_transport_server_s,
                       tag_bitfield,
                       sizeof(fbe_u32_t),
                       &tag_bitfield);

    FBE_GET_FIELD_DATA(module_name,
                       block_transport_p,
                       fbe_block_transport_server_t,
                       block_transport_const,
                       sizeof(fbe_u64_t),
                       &block_transport_const_p);

    FBE_GET_FIELD_DATA(module_name,
                       block_transport_p,
                       fbe_block_transport_server_s,
                       attributes,
                       sizeof(fbe_block_transport_attributes_t),
                       &attributes);


    trace_func(&new_prefix, "outstanding_io_count: 0x%x\n",
               (unsigned int)outstanding_io_count);
    trace_func(&new_prefix, "outstanding_io_max: 0x%x\n", outstanding_io_max);
    trace_func(&new_prefix, "tag_bitfield: 0x%x\n", tag_bitfield);
    trace_func(&new_prefix, "block_transport_const: 0x%llx\n",
               (unsigned long long)block_transport_const_p);
    trace_func(&new_prefix, "attributes: 0x%llx\n",
               (unsigned long long)attributes);

    fbe_block_transport_server_dump_debug_trace(module_name, block_transport_p, trace_func, &new_prefix, 0);

    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_block_transport_dump_server_detail()
 ******************************************/


/*!**************************************************************
 * fbe_block_edge_info_dump_debug_trace()
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
 *  8/17/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
 fbe_status_t fbe_block_edge_info_dump_debug_trace(const fbe_u8_t * module_name,
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
                                         &fbe_block_transport_control_get_edge_info_dump_struct_info,
                                         1 /* fields per line */,
                                         spaces_to_indent + 2);
    trace_func(trace_context, "\n");

    return status;
}


/********************************************
 * end fbe_block_edge_info_dump_debug_trace()
 ********************************************/

/*!**************************************************************
 * fbe_base_edge_info_dump_debug_trace()
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
 *  8/17/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
  fbe_status_t fbe_base_edge_info_dump_debug_trace(const fbe_u8_t * module_name,
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
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 4);
    status = fbe_debug_display_structure(module_name, trace_func, trace_context, 
                                         base_edge_info_ptr,
                                         &fbe_base_transport_control_get_edge_info_dump_struct_info,
                                         1 /* fields per line */,
                                         spaces_to_indent + 4);
    trace_func(trace_context, "\n");

    return status;
}


/*!**************************************************************
 * fbe_block_transport_dump_client_edge()
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
 *  8/17/2012 - Created. Jingcheng Zhang
 *
 ****************************************************************/
fbe_status_t fbe_block_transport_dump_client_edge(const fbe_u8_t * module_name,
                                              fbe_dbgext_ptr edge_p,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context,
                                              fbe_u32_t spaces_to_indent)
{
    fbe_status_t status;
    fbe_dump_debug_trace_prefix_t new_prefix;
    char edge_prefix[64];

    _snprintf(edge_prefix, 63, "client_edge[%d]", fbe_dump_client_edge_index);
    fbe_dump_client_edge_index ++;
    fbe_debug_dump_generate_prefix(&new_prefix, (fbe_dump_debug_trace_prefix_t *)trace_context,
                                   edge_prefix);
    status = fbe_debug_display_structure(module_name, trace_func, &new_prefix, 
                                         edge_p,
                                         &fbe_block_edge_dump_struct_info,
                                         1 /* fields per line */,
                                         0);
    return status;
}

/********************************************
 * end fbe_base_edge_info_dump_debug_trace()
 ********************************************/
/*************************
 * end file fbe_block_transport_dump_debug.c
 *************************/
