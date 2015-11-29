/***************************************************************************
 *  Copyright (C)  EMC Corporation 2012
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 *  All rights reserved.
 ***************************************************************************/

/*!**************************************************************************
 * @file cmi_service_ext.c
 ***************************************************************************
 *
 * @brief
 *   CMI service debug extensions.
 *
 * @author
 *  06/04/2012 - Created. Lili Chen
 *
 ***************************************************************************/

#include "pp_dbgext.h"

#include "fbe_transport_debug.h"
#include "fbe_cmi_debug.h"

CSX_DBG_EXT_DEFINE_CMD(cmi_info, "cmi_info")
{
    fbe_bool_t b_display_io = FBE_FALSE;
    fbe_u32_t spaces_to_indent = 4;
    fbe_trace_func_t trace_func = fbe_debug_trace_func;
    fbe_trace_context_t trace_context = NULL;

    if (strlen(args) && strncmp(args, "-io", 4) == 0)
    {
        b_display_io = FBE_TRUE;
    }

    fbe_cmi_debug_display_info(pp_ext_module_name,
                               trace_func,
                               trace_context,
                               spaces_to_indent);

    if (b_display_io && strncmp(pp_ext_module_name, "sep", 3) == 0)
    {
        fbe_cmi_debug_display_io(pp_ext_module_name,
                                 trace_func,
                                 trace_context,
                                 spaces_to_indent);
    }

    return;
}

#pragma data_seg ("EXT_HELP$4cmi_info")
static char CSX_MAYBE_UNUSED cmi_infoUsageMsg[] =
"!cmi_info\n"
"   Display information for FBE CMI service\n"
"   -io display the contents of all IOs that are waiting or outstanding\n";
#pragma data_seg (".data")

/*************************
 * end file cmi_service_ext.c
 *************************/
