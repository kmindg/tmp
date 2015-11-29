
/***************************************************************************
 *  Copyright (C)  EMC Corporation 2000-10
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 *  All rights reserved.
 ***************************************************************************/

/*!**************************************************************************
 * @file nonpaged_metadata_ext.c
 ***************************************************************************
 *
 * @brief
 *   Nonpaged Metadata debug extensions.
 *
 * @author
 *  @5/15/2012 - Created. Jingcheng Zhang
 *
 ***************************************************************************/

#include "pp_dbgext.h"

#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_raid_common.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_metadata_interface.h"
#include "fbe_transport_debug.h"
#include "fbe/fbe_base_config.h"
#include "fbe_base_config_debug.h"

CSX_DBG_EXT_DEFINE_CMD(baseconfig_nonpage, "baseconfig_nonpage")
{
    fbe_trace_func_t trace_func = fbe_debug_trace_func;
    fbe_trace_context_t trace_context = NULL;
    fbe_u64_t nonpaged_metadata_array = 0;
    fbe_u32_t spaces_to_indent = 2;
    fbe_object_id_t  object_id = FBE_OBJECT_ID_INVALID;
    fbe_u32_t        offset = 0;
    const fbe_u8_t * module_name = "sep";
    fbe_u64_t        base_addr = 0;
    fbe_u32_t        ptr_size = 0;
    fbe_status_t     status;


    /* For now we assume that only sep package has metadata objects.
     * If it is not set to sep, then some things might not get resolved properly.
     */

    if (strlen(args) <= 0) {
        csx_dbg_ext_print("\tusage: !baseconfig_nonpage <object_id>\n");
        return;
    }

    status = fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (status != FBE_STATUS_OK) 
    {
        fbe_debug_trace_func(NULL, "%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return ; 
    }

    object_id = (fbe_object_id_t)GetArgument64(args, 1);
    offset = (fbe_u32_t)(&((fbe_metadata_nonpaged_entry_t *)0)->data);
    FBE_GET_EXPRESSION(module_name, fbe_metadata_nonpaged_array, &nonpaged_metadata_array);
    FBE_READ_MEMORY(nonpaged_metadata_array, &base_addr, ptr_size);
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "base_config nonpaged metadata: \n");

    base_addr += object_id * sizeof (fbe_metadata_nonpaged_entry_t);
    fbe_base_config_nonpaged_metadata_debug_trace(module_name,
                                                  base_addr + offset,
                                                  trace_func,
                                                  trace_context,
                                                  NULL,
                                                  10);
    return;
}
#pragma data_seg ("EXT_HELP$4bc_nonpage")
static char CSX_MAYBE_UNUSED baseconfig_nonpageUsageMsg[] =
"!baseconfig_nonpage\n"
"   Display information for the object's base_config nonpaged metadata\n";
#pragma data_seg (".data")
/*************************
 * end file metadata_service_ext.c
 *************************/
