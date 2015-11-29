/***************************************************************************
 *  Copyright (C)  EMC Corporation 2000-10
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation.
 *  All rights reserved.
 ***************************************************************************/

/*!**************************************************************************
 * @file metadata_service_ext.c
 ***************************************************************************
 *
 * @brief
 *   Metadata debug extensions.
 *
 * @author
 *  @6/23/2010 - Created. Rob Foley
 *
 ***************************************************************************/

#include "pp_dbgext.h"

#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_raid_common.h"
#include "fbe_transport_debug.h"


CSX_DBG_EXT_DEFINE_CMD( metadata_service , " metadata_service ")
{
    fbe_trace_func_t trace_func = fbe_debug_trace_func;
    fbe_trace_context_t trace_context = NULL;
    fbe_u64_t packet_queue_p = 0;
    fbe_u32_t spaces_to_indent = 2;
    /* For now we assume that only sep package has metadata objects.
     * If it is not set to sep, then some things might not get resolved properly.
     */
    const fbe_u8_t * module_name = "sep";

    FBE_GET_EXPRESSION(module_name, stripe_lock_packet_queue_head, &packet_queue_p);
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
    trace_func(trace_context, "stripe lock packet queue: \n");
    fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 2);
    fbe_transport_print_packet_queue(module_name,
                                     packet_queue_p,
                                     trace_func,
                                     trace_context,
                                     spaces_to_indent);
    return;
}
#pragma data_seg ("EXT_HELP$4metadata_service")
static char CSX_MAYBE_UNUSED metadata_serviceUsageMsg[] =
"!metadata_service\n"
"   Display information for the metadata service\n";
#pragma data_seg (".data")
/*************************
 * end file metadata_service_ext.c
 *************************/
