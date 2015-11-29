/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_discovery_transport_debug.c
 ****************************************************************************
 *
 * @brief
 *  This file contains the debug functions of the Discovery Transport.
 *
 * @author
 *   02/13/2009:  Created. Nishit Singh
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


/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * @fn fbe_discovery_transport_print_detail(const fbe_u8_t * module_name,
   *                                        fbe_u64_t base_discovered_p,
 *                                          fbe_trace_func_t trace_func,
 *                                          fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display's information about the base discovery.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param  base_discovered_p - Ptr to the base discovery.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  02/16/2008 - Created. Nishit Singh
 *
 ****************************************************************/
fbe_status_t fbe_discovery_transport_print_detail(const fbe_u8_t * module_name,
                                                  fbe_u64_t base_discovered_p,
                                                  fbe_trace_func_t trace_func,
                                                  fbe_trace_context_t trace_context)

{
    fbe_object_id_t port_object_id;
    fbe_u32_t base_edge_offset, discovery_edge_offset;
    fbe_u64_t base_edge_p;

    /* Display the discovery edge information.
     */
    FBE_GET_FIELD_DATA(module_name, 
                       base_discovered_p,
                       fbe_base_discovered_t,
                       port_object_id,
                       sizeof(fbe_object_id_t),
                       &port_object_id);

    FBE_GET_FIELD_OFFSET(module_name,
                         fbe_base_discovered_t,
                         "discovery_edge",
                         &discovery_edge_offset);

    FBE_GET_FIELD_OFFSET(module_name,
                         fbe_discovery_edge_t,
                         "base_edge",
                         &base_edge_offset);

    base_edge_p = base_discovered_p + discovery_edge_offset + base_edge_offset;
    trace_func(trace_context, "  Discovery edge: port_object_id: 0x%x \n", port_object_id);
    fbe_transport_print_base_edge(module_name,
                                  base_edge_p,
                                  trace_func,
                                  trace_context);

    return FBE_STATUS_OK;

}
/******************************************
 * end fbe_discovery_transport_print_detail()
 ******************************************/


/*************************
 * end file fbe_discovery_transport_debug.c
 *************************/