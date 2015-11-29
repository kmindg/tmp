/***************************************************************************
 *  Copyright (C)  EMC Corporation 2011
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 *  All rights reserved.
 ***************************************************************************/

/*!**************************************************************************
 * @file logical_error_injection_ext.c
 ***************************************************************************
 *
 * @brief
 *   Logical erorr injection debug extensions.
 *
 * @author
 *  4/18/2011 - Created. Rob Foley
 *
 ***************************************************************************/

#include "pp_dbgext.h"

#include "fbe/fbe_topology_interface.h"
#include "fbe_logical_error_injection_debug.h"


CSX_DBG_EXT_DEFINE_CMD( logical_error_injection , " logical_error_injection ")
{
    fbe_u32_t ptr_size;
    /* We either are in simulation or on hardware.
     * Depending on where we are, use a different module name. 
     * We validate the module name by checking the ptr size. 
     */
    const fbe_u8_t * module_name = "NewNeitPackage";
    const fbe_u8_t * alt_module_name = "new_neit";
    fbe_debug_get_ptr_size(module_name, &ptr_size);
    if (ptr_size == 0)
    {
        fbe_logical_error_injection_debug_display(alt_module_name, fbe_debug_trace_func, NULL, 2 /* spaces to indent */);
    }
    else
    {
        fbe_logical_error_injection_debug_display(module_name, fbe_debug_trace_func, NULL, 2 /* spaces to indent */);
    }
    return;
}
#pragma data_seg ("EXT_HELP$4logical_error_injection")
static char CSX_MAYBE_UNUSED logical_error_injectionUsageMsg[] =
"!logical_error_injection\n"
"   Display information about the logical error injection service.\n";
#pragma data_seg (".data")

/*************************
 * end file logical_error_injection_ext
 *************************/
