 /***************************************************************************
  * Copyright (C) EMC Corporation 2009
  * All rights reserved.
  * Licensed material -- property of EMC Corporation
  ***************************************************************************/
 
 /*!*************************************************************************
  * @file fbe_logical_drive_trace.c
  ***************************************************************************
  *
  * @brief
  *  This file contains functions to display logical drive structures.
  *
  * @version
  *   5/7/2009:  Created. RPF
  *
  ***************************************************************************/
 
 /*************************
  *   INCLUDE FILES
  *************************/
#include "fbe/fbe_types.h"
#include "fbe_trace.h"
#include "fbe/fbe_logical_drive.h"
#include "fbe_block_transport_trace.h"
 
 /*************************
  *   FUNCTION DEFINITIONS
  *************************/
 
/*!**************************************************************
 * fbe_logical_drive_trace_attributes()
 ****************************************************************
 * @brief
 *  Display the logical drive attributes structure.
 *
 * @param attrib_p - Ptr to the attributes struct to display.
 * @param trace_func - Function to use for printing.
 * @param trace_context - Context for the trace function.
 *
 * @return - None.
 *
 * @author
 *  4/29/2009 - Created. rfoley
 *
 ****************************************************************/

void fbe_logical_drive_trace_attributes(fbe_logical_drive_attributes_t *attrib_p,
                                        fbe_trace_func_t trace_func,
                                        fbe_trace_context_t trace_context)
{
    /* Display basic info about the logical drive.
     */
    trace_func(trace_context, " Logical Drive Attributes:\n");
    trace_func(trace_context, "   Imported block size:    0x%x (%d)\n", attrib_p->imported_block_size, attrib_p->imported_block_size);
    trace_func(trace_context, "   Imported capacity:      0x%llx\n",
	       (unsigned long long)attrib_p->imported_capacity);

    /* Display the initial identity information in hex.
     */
    trace_func(trace_context, "   Initial identity info:  (hex) ");
    fbe_block_transport_trace_identify_hex(&attrib_p->initial_identify, trace_func, trace_context);

    /* Display the initial identity information as characters.
     */
    trace_func(trace_context, "   Initial identity info:  (char)");
    fbe_block_transport_trace_identify_char(&attrib_p->initial_identify, trace_func, trace_context);

    /* Display the last identity information as hex.
     */
    trace_func(trace_context, "   Last identity info:     (hex) ");
    fbe_block_transport_trace_identify_hex(&attrib_p->last_identify, trace_func, trace_context);

    /* Display the last identity information as characters.
     */
    trace_func(trace_context, "   Last identity info:     (char)");
    fbe_block_transport_trace_identify_char(&attrib_p->last_identify, trace_func, trace_context);

    trace_func(trace_context, "   Server Info:\n");
    trace_func(trace_context, "     attributes:           0x%x\n", attrib_p->server_info.attributes);
    trace_func(trace_context, "     optional queued cmds: %s\n", (attrib_p->server_info.b_optional_queued ? "yes" : "no"));
    trace_func(trace_context, "     low queued cmds:      %s\n", (attrib_p->server_info.b_low_queued ? "yes" : "no"));
    trace_func(trace_context, "     normal queued cmds:   %s\n", (attrib_p->server_info.b_normal_queued ? "yes" : "no"));
    trace_func(trace_context, "     urgent queued cmds:   %s\n", (attrib_p->server_info.b_urgent_queued ? "yes" : "no"));
    trace_func(trace_context, "     number of clients:    %d\n", attrib_p->server_info.number_of_clients);
    trace_func(trace_context, "     outstanding io count: %d\n", attrib_p->server_info.outstanding_io_count);
    trace_func(trace_context, "     outstanding io max:   %d\n", attrib_p->server_info.outstanding_io_max);
    trace_func(trace_context, "     server object id:     0x%x\n", attrib_p->server_info.server_object_id);
    trace_func(trace_context, "     tag_bigfield:         0x%x\n", attrib_p->server_info.tag_bitfield);
    return;
}
/******************************************
 * end fbe_logical_drive_trace_attributes()
 ******************************************/

 /*************************
  * end file fbe_logical_drive_trace.c
  *************************/
 
 
