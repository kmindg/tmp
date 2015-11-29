/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_raw_mirror_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the raw mirror.
 *
 * @author
 *  11/16/2011 - Created. Trupti Ghate
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe/fbe_transport.h"
#include "fbe_block_transport.h"
#include "fbe_transport_debug.h"
#include "fbe_raid_library.h"
#include "fbe_raw_mirror_debug.h"
#include "fbe_raid_library_debug.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!*******************************************************************
 * @var fbe_raw_mirror_field_info
 *********************************************************************
 * @brief Information about each of the fields of raw mirror.
 *
 *********************************************************************/
fbe_debug_field_info_t fbe_raw_mirror_debug_field_info[] =
{
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("edge", fbe_block_edge_t, FBE_TRUE, "0x%x",
                                    fbe_raw_mirror_block_edge_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("geo", fbe_raid_geometry_t, FBE_FALSE, "0x%x",
                                    fbe_raid_geometry_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_FN("edge_info", fbe_block_transport_control_get_edge_info_t, FBE_FALSE, "0x%x",
                                    fbe_block_edge_info_debug_trace),
    FBE_DEBUG_DECLARE_FIELD_INFO_TERMINATOR(),
};

/*!*******************************************************************
 * @var fbe_raw_mirror_debug_struct_info
 *********************************************************************
 * @brief This structure holds information used to display the
 *        raw_mirror structure.
 *
 *********************************************************************/
FBE_DEBUG_DECLARE_STRUCT_INFO(fbe_raw_mirror_t,
                              fbe_raw_mirror_debug_struct_info,
                              &fbe_raw_mirror_debug_field_info[0]); 


/*!**************************************************************
 * fbe_raw_mirror_debug()
 ****************************************************************
 * @brief
 *  Display info on raw_mirror structures..
 *  
 * @param module_name - Module name to use in symbol lookup.
 * @param raw_mirror_ptr -
 * @param trace_func - trace function to use.
 * @param trace_context - context to send to trace function
 * 
 * @return fbe_status_t
 *
 * @author
 *  11/16/2011 - Created. Trupti Ghate
 *
 ****************************************************************/
fbe_status_t fbe_raw_mirror_debug(const fbe_u8_t * module_name,
                                  fbe_dbgext_ptr raw_mirror_ptr,
                                  fbe_trace_func_t trace_func,
                                  fbe_trace_context_t trace_context)
{
    fbe_u32_t spaces_to_indent = 4;
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    fbe_debug_display_structure(module_name, trace_func, trace_context, raw_mirror_ptr,
                                &fbe_raw_mirror_debug_struct_info,
                                4 /* fields per line */,
                                spaces_to_indent);
    trace_func(trace_context, "\n");
    return FBE_STATUS_OK;
}
/******************************************
 * end fbe_raw_mirror_debug()
 ******************************************/
/*!**************************************************************
 * fbe_raw_mirror_block_edge_debug_trace()
 ****************************************************************
 * @brief
 *  Display info on raw mirror block edge element.
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
 *  16- Nov- 2011 - Created. Trupti Ghate
 *
 ****************************************************************/
fbe_status_t fbe_raw_mirror_block_edge_debug_trace(const fbe_u8_t * module_name,
                                         fbe_dbgext_ptr base_p,
                                         fbe_trace_func_t trace_func,
                                         fbe_trace_context_t trace_context,
                                         fbe_debug_field_info_t *field_info_p,
                                         fbe_u32_t overall_spaces_to_indent)
{
    fbe_u64_t block_edge_p  ;
    fbe_u32_t block_edge_size = 0;
    fbe_u32_t edge_index = 0;
    fbe_u32_t ptr_size = 0;
    fbe_dbgext_ptr block_edge_ptr = base_p + field_info_p->offset;

    trace_func(NULL,"\n");
    trace_func(trace_context, " %s: \n", field_info_p->name);
    FBE_GET_TYPE_SIZE(module_name, fbe_block_edge_t*, &ptr_size);
    FBE_GET_TYPE_SIZE(module_name, fbe_block_edge_t, &block_edge_size);
    for ( edge_index = 0; edge_index < FBE_RAW_MIRROR_WIDTH; edge_index++)
    {
        if (FBE_CHECK_CONTROL_C())
        {
            return FBE_STATUS_CANCELED;
        }
        FBE_READ_MEMORY(block_edge_ptr + (ptr_size * edge_index), &block_edge_p, ptr_size);
        if(block_edge_ptr == 0)
        {
            trace_func(trace_context, "block_edge_ptr is not available \n");
            return FBE_STATUS_OK;
        }
        fbe_block_transport_display_edge(module_name,
                                         block_edge_p,
                                         trace_func,
                                         trace_context,
                                         overall_spaces_to_indent + 2 /* spaces to indent */);
        trace_func(trace_context, "\n");
    }
    return FBE_STATUS_OK;
}
/* end of fbe_raw_mirror_debug.c */

