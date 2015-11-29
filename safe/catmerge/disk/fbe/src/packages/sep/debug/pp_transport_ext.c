/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file pp_transport_ext.c
 ***************************************************************************
 *
 * @brief
 *  This file contains transport related extensions for the physical package.
 *
 * @version
 *   3/3/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "pp_dbgext.h"
#include "fbe_transport_debug.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/


#pragma data_seg ("EXT_HELP$4pp_display_packet")
static char usageMsg_pp_display_packet[] =
"!pp_display_packet packet_ptr\n"
"  Display the packet referenced by the input pointer\n";
#pragma data_seg ()

/*!**************************************************************
 * pp_display_packet()
 ****************************************************************
 * @brief
 *  Debugger command to display a packet.
 *
 * @param hCurrentProcess
 * @param hCurrentThread
 * @param dwCurrentPc
 * @param dwProcessor
 * @param args - The argument string.
 * 
 * @return VOID
 *
 * @author
 *  03/02/2009 - Created. RPF
 *
 ****************************************************************/
DECLARE_API(pp_display_packet)
{
    fbe_u64_t packet_p = 0;

    /* We expect to receive a single argument of a packet ptr.
     */
    if (strlen(args))
    {
        /* Get and display our packet ptr.
         */
        packet_p = GetArgument64(args, 1);
        fbe_transport_print_fbe_packet(pp_ext_module_name, packet_p, fbe_debug_trace_func, NULL, 
                                       2 /* spaces to indent */);
    }
    else
    {
        /* Cannot display if no packet ptr given.
         */
        csx_dbg_ext_print("!pp_display_packet: packet ptr required\n");
    }
    return;
}
/**************************************
 * end pp_display_packet()
 **************************************/

#pragma data_seg ("EXT_HELP$4pp_display_packet_queue")
static char usageMsg_pp_display_packet_queue[] =
"!pp_display_packet_queue packet_queue_ptr\n"
"  Display the packet queue referenced by the input pointer\n";
#pragma data_seg ()

/*!**************************************************************
 * pp_display_packet_queue()
 ****************************************************************
 * @brief
 *  Debugger command to display a queue of packets.
 *
 * @param hCurrentProcess
 * @param hCurrentThread
 * @param dwCurrentPc
 * @param dwProcessor
 * @param args - The argument string.
 * 
 * @return VOID
 *
 * @author
 *  03/02/2009 - Created. RPF
 *
 ****************************************************************/
DECLARE_API ( pp_display_packet_queue )
{
    fbe_trace_func_t trace_func = fbe_debug_trace_func;
    fbe_trace_context_t trace_context = NULL;
    fbe_u64_t packet_queue_p = 0;
    fbe_u32_t spaces_to_indent = 2;
    fbe_u64_t packet_queue_p = 0;

    /* We expect to receive a single argument of a packet ptr.
     */
    if (strlen(args))
    {
        /* Get and display our packet ptr.
         */
        packet_queue_p = GetArgument64(args, 1);
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent);
        trace_func(trace_context, "packet queue: 0x%llx\n",
		   (unsigned long long)packet_queue_p);
        fbe_trace_indent(trace_func, trace_context, spaces_to_indent + 2);
        fbe_transport_print_packet_queue(pp_ext_module_name,
                                         packet_queue_p,
                                         trace_func,
                                         trace_context,
                                         spaces_to_indent);
    }
    else
    {
        /* Cannot display if no packet ptr given.
         */
        csx_dbg_ext_print("!pp_display_packet_queue: packet queue ptr required\n");
    }

    return;
}
/*************************
 * end file pp_transport_ext.c
 *************************/
