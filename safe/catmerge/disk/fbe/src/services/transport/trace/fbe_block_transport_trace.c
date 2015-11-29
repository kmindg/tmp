/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_block_transport_trace.c
 ***************************************************************************
 *
 * @brief
 *  This file contains trace functions for the fbe block transport.
 *
 * @version
 *   5/07/2009:  Created. rfoley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe_trace.h"
#include "fbe/fbe_transport.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * fbe_block_transport_trace_negotiate_info()
 ****************************************************************
 * @brief
 *  Display block edge info structure.
 *
 * @param edge_info_p - Ptr to the edge info struct to display.
 * @param trace_func - Function to use for printing.
 * @param trace_context - Context for the trace function.               
 *
 * @return - None.
 *
 * @author
 *  5/08/2009 - Created. rfoley
 *
 ****************************************************************/

void fbe_block_transport_trace_negotiate_info(fbe_block_transport_negotiate_t *negotiate_p,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context)
{
    /* Display basic info about the block edge.
     */
    trace_func(trace_context, " Negotiate Info (requested->520):\n");
    trace_func(trace_context, "   Block size:             0x%x (%d)\n", 
               negotiate_p->block_size, negotiate_p->block_size);
    trace_func(trace_context, "   Imported capacity:      0x%llx\n",
	       (unsigned long long)negotiate_p->block_count);
    trace_func(trace_context, "   Optimum Block Size:     0x%x (%d)\n", 
               negotiate_p->optimum_block_size, negotiate_p->optimum_block_size);
    trace_func(trace_context, "   Opt Blk Align:          0x%x (%d)\n", 
               negotiate_p->optimum_block_alignment, negotiate_p->optimum_block_alignment);
    trace_func(trace_context, "   Physical Block Size:    0x%x (%d)\n", 
               negotiate_p->physical_block_size, negotiate_p->physical_block_size);
    trace_func(trace_context, "   Physical Opt Blk Size:  0x%x (%d)\n", 
               negotiate_p->physical_optimum_block_size, negotiate_p->physical_optimum_block_size);
    trace_func(trace_context, "   Requested Block Size:   0x%x (%d)\n", 
               negotiate_p->requested_block_size, negotiate_p->requested_block_size);
    trace_func(trace_context, "   Requested Opt Blk Size: 0x%x (%d)\n", 
               negotiate_p->requested_optimum_block_size, negotiate_p->requested_optimum_block_size);
    return;
}
/******************************************
 * end fbe_block_transport_trace_negotiate_info()
 ******************************************/

/*!**************************************************************
 * fbe_block_transport_trace_identify_hex()
 ****************************************************************
 * @brief
 *  Display the block transport identify information in hex.
 *
 * @param attrib_p - Ptr to the attributes struct to display.
 * @param trace_func - Function to use for printing.
 * @param trace_context - Context for the trace function.
 *
 * @return - None.
 *
 * @author
 *  5/07/2009 - Created. rfoley
 *
 ****************************************************************/

void fbe_block_transport_trace_identify_hex(fbe_block_transport_identify_t *identify_p,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context)
{
    fbe_u32_t identity_index;

    /* Display the initial identity information in hex.
     */
    for (identity_index = 0;
         identity_index < FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH;
         identity_index++)
    {
        trace_func(trace_context, "%02x ", identify_p->identify_info[identity_index]);
    }
    trace_func(trace_context, "\n");

    return;
}
/******************************************
 * end fbe_block_transport_trace_identify_hex()
 ******************************************/

/*!**************************************************************
 * fbe_block_transport_trace_identify_char()
 ****************************************************************
 * @brief
 *  Display the block transport identify information as characters.
 *
 * @param attrib_p - Ptr to the attributes struct to display.
 * @param trace_func - Function to use for printing.
 * @param trace_context - Context for the trace function.
 *
 * @return - None.
 *
 * @author
 *  5/07/2009 - Created. rfoley
 *
 ****************************************************************/

void fbe_block_transport_trace_identify_char(fbe_block_transport_identify_t *identify_p,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context)
{
    fbe_u32_t identity_index;

    /* Display the initial identity information in hex.
     */
    for (identity_index = 0;
         identity_index < FBE_BLOCK_TRANSPORT_CONTROL_IDENTIFY_LENGTH;
         identity_index++)
    {
        /* Printable ascii chars go from " " (20) to ~ (126).
         */
        if ((identify_p->identify_info[identity_index] >= ' ') &&
            (identify_p->identify_info[identity_index] <= '~'))
        {
            trace_func(trace_context, "%2c ", identify_p->identify_info[identity_index]);
        }
        else
        {
            trace_func(trace_context, "   ");
        }
    }
    trace_func(trace_context, "\n");

    return;
}
/******************************************
 * end fbe_block_transport_trace_identify_char()
 ******************************************/

/*************************
 * end file fbe_block_transport_trace.c
 *************************/


