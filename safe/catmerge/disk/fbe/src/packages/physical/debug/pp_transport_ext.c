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
static char CSX_MAYBE_UNUSED usageMsg_pp_display_packet[] =
"!pp_display_packet [-l|-t]packet_ptr\n"
"  Display the packet referenced by the input pointer\n"
"  -t not to print master packet\n"
"  -l print all packets in the same queue.\n"
"  -s print brief summary of each packet.\n";
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
CSX_DBG_EXT_DEFINE_CMD(pp_display_packet, "pp_display_packet")
{
    fbe_status_t status;
    fbe_u64_t packet_p = 0;
    fbe_u64_t master_packet_p = 0;
    fbe_u32_t master_offset;
    fbe_u32_t ptr_size = 0;
	fbe_u32_t magic_number_offset;
    fbe_u64_t magic_number;
    fbe_bool_t b_summary = FBE_FALSE;

    status = fbe_debug_get_ptr_size(pp_ext_module_name, &ptr_size);
    if (status != FBE_STATUS_OK)
    {
        fbe_debug_trace_func(NULL, "%s unable to get ptr size status: %d\n", __FUNCTION__, status);
        return; 
    }
    if (strlen(args) && strncmp(args, "-s", 2) == 0)
    {
        /* Increment past the terse flag.
         */
        args += FBE_MIN(3, strlen(args));
        b_summary = FBE_TRUE;
    }
    if (strlen(args) && strncmp(args, "-t", 2) == 0)
    {
        /* Increment past the terse flag.
         */
        args += FBE_MIN(3, strlen(args));

        /* Get and display our packet ptr.
         */
        if (strlen(args))
        {
            packet_p = GetArgument64(args, 1);

            if (b_summary) {
                fbe_debug_set_display_queue_header(FBE_FALSE);
                fbe_transport_print_packet_summary(pp_ext_module_name, packet_p, fbe_debug_trace_func, NULL, 
                                                   2 /* spaces to indent */);
                fbe_debug_set_display_queue_header(FBE_TRUE);
            }
            else {
                fbe_transport_print_fbe_packet(pp_ext_module_name, packet_p, fbe_debug_trace_func, NULL, 
                                               2 /* spaces to indent */);
            }
        }
        return;
    }

    FBE_GET_FIELD_OFFSET(pp_ext_module_name, fbe_packet_t, "magic_number", &magic_number_offset);

    if (strlen(args) && strncmp(args, "-l", 2) == 0)
    {
        fbe_u32_t queue_element_offset;
        fbe_u32_t element_count = 0;
        fbe_u64_t packet_element = 0;
        fbe_u64_t next_element = 0;
        fbe_u64_t next_packet_p = 0;
        /* Increment past the terse flag.
         */
        args += FBE_MIN(3, strlen(args));

        /* Get and display our packet ptr.
         */
        if (strlen(args))
        {
            packet_p = GetArgument64(args, 1);
            fbe_transport_print_fbe_packet(pp_ext_module_name, packet_p, fbe_debug_trace_func, NULL, 
                                           2 /* spaces to indent */);
            FBE_GET_FIELD_OFFSET(pp_ext_module_name, fbe_packet_t, "queue_element", &queue_element_offset);
            packet_element = packet_p + queue_element_offset;
            FBE_READ_MEMORY(packet_element, &next_element, ptr_size);
            while ((packet_element != next_element) && (next_element != 0) && (element_count < 100))
            {
                fbe_debug_trace_func(NULL, "packet element 0x%llx, next element 0x%llx\n", (unsigned long long)packet_element, (unsigned long long)next_element);
                element_count ++;
                next_packet_p = next_element - queue_element_offset;

                FBE_READ_MEMORY(next_packet_p + magic_number_offset, &magic_number, sizeof(fbe_u64_t));
                if (magic_number != FBE_MAGIC_NUMBER_BASE_PACKET)
                {
                    fbe_debug_trace_func(NULL, "this is not a valid packet: %llx magic number 0x%llx != 0x%llx", 
                                         next_packet_p, (unsigned long long)magic_number, (unsigned long long)FBE_MAGIC_NUMBER_BASE_PACKET);
                }
                else if (b_summary) {
                    fbe_debug_set_display_queue_header(FBE_FALSE);
                    fbe_transport_print_packet_summary(pp_ext_module_name, next_packet_p, fbe_debug_trace_func, NULL, 
                                                       2 /* spaces to indent */);
                    fbe_debug_set_display_queue_header(FBE_TRUE);
                } else {
                    fbe_transport_print_fbe_packet(pp_ext_module_name, next_packet_p, fbe_debug_trace_func, NULL, 
                                           4 /* spaces to indent */);
                }
                FBE_READ_MEMORY(next_element, &next_element, ptr_size);
            }
        }
        return;
    }

    /* We expect to receive a single argument of a packet ptr.
     */
    if (strlen(args))
    {
        FBE_GET_FIELD_OFFSET(pp_ext_module_name, fbe_packet_t, "master_packet", &master_offset);
        /* Get and display our packet ptr.
         */
        packet_p = GetArgument64(args, 1);

        /* Simply iterate until we find the top level master to display. 
         * This allows us to show the entire chain. 
         */
        FBE_READ_MEMORY(packet_p + master_offset, &master_packet_p, ptr_size);
        while (master_packet_p != 0)
        {
            if (FBE_CHECK_CONTROL_C())
            {
                return;
            }
            FBE_READ_MEMORY(master_packet_p + magic_number_offset, &magic_number, sizeof(fbe_u64_t));
            if (magic_number != FBE_MAGIC_NUMBER_BASE_PACKET)
            {
                fbe_debug_trace_func(NULL, "this is not a valid master packet: 0x%llx magic number 0x%llx != 0x%llx", 
                                     (unsigned long long)master_packet_p,
				     (unsigned long long)magic_number,
				     (unsigned long long)FBE_MAGIC_NUMBER_BASE_PACKET);
                break;
            }
            packet_p = master_packet_p;
            FBE_READ_MEMORY(packet_p + master_offset, &master_packet_p, ptr_size);
        }

        if (b_summary) {
            fbe_debug_set_display_queue_header(FBE_FALSE);
            fbe_transport_print_packet_summary(pp_ext_module_name, packet_p, fbe_debug_trace_func, NULL, 
                                               2 /* spaces to indent */);
            fbe_debug_set_display_queue_header(FBE_TRUE);
        } else {
            fbe_transport_print_fbe_packet(pp_ext_module_name, packet_p, fbe_debug_trace_func, NULL, 
                                           2 /* spaces to indent */);
        }
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
static char CSX_MAYBE_UNUSED usageMsg_pp_display_packet_queue[] =
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
CSX_DBG_EXT_DEFINE_CMD( pp_display_packet_queue , " pp_display_packet_queue ")
{
    fbe_trace_func_t trace_func = fbe_debug_trace_func;
    fbe_trace_context_t trace_context = NULL;
    fbe_u64_t packet_queue_p = 0;
    fbe_u32_t spaces_to_indent = 2;

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
