 /***************************************************************************
  * Copyright (C) EMC Corporation 2009
  * All rights reserved.
  * Licensed material -- property of EMC Corporation
  ***************************************************************************/
 
 /*!*************************************************************************
  * @file fbe_api_trace.c
  ***************************************************************************
  *
  * @brief
  *  This file contains trace functions for the fbe_api.
  *
  * @version
  *   5/7/2009:  Created. rfoley
  *
  ***************************************************************************/
 
 /*************************
  *   INCLUDE FILES
  *************************/
#include "fbe_trace.h"
#include "fbe/fbe_class.h"
#include "fbe/fbe_api_discovery_interface.h"
#include "fbe_transport_trace.h"
#include "fbe_base_object_trace.h"
#include "fbe_block_transport_trace.h"
#include "fbe/fbe_api_trace.h"

 /*************************
  *   FUNCTION DEFINITIONS
  *************************/
 
/*!**************************************************************
 * fbe_api_trace_block_edge_info()
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
 *  4/29/2009 - Created. rfoley
 *
 ****************************************************************/

void fbe_api_trace_block_edge_info(fbe_api_get_block_edge_info_t *edge_info_p,
                                   fbe_trace_func_t trace_func,
                                   fbe_trace_context_t trace_context)
{
    /* Display basic info about the block edge.
     */
    trace_func(trace_context, " Block Edge Info:\n");
    trace_func(trace_context, "   Exported block size:    0x%x (%d)\n", 
               edge_info_p->exported_block_size, edge_info_p->exported_block_size);
    trace_func(trace_context, "   Imported block size:    0x%x (%d)\n", 
               edge_info_p->imported_block_size, edge_info_p->imported_block_size);
    trace_func(trace_context, "   Imported capacity:      0x%llx\n",
	       (unsigned long long)edge_info_p->capacity);
    trace_func(trace_context, "   Offset:                 0x%llx (%llu)\n",
	       (unsigned long long)edge_info_p->offset,
	       (unsigned long long)edge_info_p->offset);
    trace_func(trace_context, "   Path state:             0x%x ", edge_info_p->path_state);

    fbe_transport_print_base_edge_path_state(edge_info_p->path_state,
                                             trace_func,
                                             NULL);
    trace_func(trace_context, "\n");
    trace_func(trace_context, "   Path attributes:        0x%x\n", edge_info_p->path_attr);

    trace_func(trace_context, "   Transport id:           0x%x ", edge_info_p->transport_id);

    fbe_transport_print_base_edge_transport_id(edge_info_p->transport_id, 
                                               trace_func, 
                                               NULL);
    trace_func(trace_context, "\n");
    trace_func(trace_context, "   Client id/index:        0x%x/0x%x\n", edge_info_p->client_id, edge_info_p->client_index);
    trace_func(trace_context, "   Server id/index:        0x%x/0x%x\n", edge_info_p->server_id, edge_info_p->server_index);
    return;
}
/******************************************
 * end fbe_api_trace_block_edge_info()
 ******************************************/

/*!**************************************************************
 * fbe_api_trace_discovery_edge_info()
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
 *  4/29/2009 - Created. rfoley
 *
 ****************************************************************/

void fbe_api_trace_discovery_edge_info(fbe_api_get_discovery_edge_info_t *edge_info_p,
                                       fbe_trace_func_t trace_func,
                                       fbe_trace_context_t trace_context)
{
    /* Display basic info about the block edge.
     */
    trace_func(trace_context, " Discovery Edge Info:\n");
    trace_func(trace_context, "   Path state:             0x%x ", edge_info_p->path_state);

    fbe_transport_print_base_edge_path_state(edge_info_p->path_state,
                                             trace_func,
                                             NULL);
    trace_func(trace_context, "\n");
    trace_func(trace_context, "   Path attributes:        0x%x\n", edge_info_p->path_attr);
    trace_func(trace_context, "   Transport id:           0x%x ", edge_info_p->transport_id);

    fbe_transport_print_base_edge_transport_id(edge_info_p->transport_id, 
                                               trace_func, 
                                               NULL);
    trace_func(trace_context, "\n");
    trace_func(trace_context, "   Client id/index:        0x%x/0x%x\n", edge_info_p->client_id, edge_info_p->client_index);
    trace_func(trace_context, "   Server id/index:        0x%x/0x%x\n", edge_info_p->server_id, edge_info_p->server_index);
    return;
}
/******************************************
 * end fbe_api_trace_discovery_edge_info()
 ******************************************/

/*!**************************************************************
 * fbe_api_trace_ssp_edge_info()
 ****************************************************************
 * @brief
 *  Display SSP edge info structure.
 *
 * @param edge_info_p - Ptr to the edge info struct to display.
 * @param trace_func - Function to use for printing.
 * @param trace_context - Context for the trace function.               
 *
 * @return - None.
 *
 * @author
 *  06/12/2009 - Created. chenl6
 *
 ****************************************************************/

void fbe_api_trace_ssp_edge_info(fbe_api_get_ssp_edge_info_t *edge_info_p,
                                 fbe_trace_func_t trace_func,
                                 fbe_trace_context_t trace_context)
{
    /* Display basic info about the SSP edge.
     */
    trace_func(trace_context, " SSP Edge Info:\n");
    trace_func(trace_context, "   Path state:             0x%x ", edge_info_p->path_state);

    fbe_transport_print_base_edge_path_state(edge_info_p->path_state,
                                             trace_func,
                                             NULL);
    trace_func(trace_context, "\n");
    trace_func(trace_context, "   Path attributes:        0x%x\n", edge_info_p->path_attr);
    trace_func(trace_context, "   Transport id:           0x%x ", edge_info_p->transport_id);

    fbe_transport_print_base_edge_transport_id(edge_info_p->transport_id, 
                                               trace_func, 
                                               NULL);
    trace_func(trace_context, "\n");
    trace_func(trace_context, "   Client id/index:        0x%x/0x%x\n", edge_info_p->client_id, edge_info_p->client_index);
    trace_func(trace_context, "   Server id/index:        0x%x/0x%x\n", edge_info_p->server_id, edge_info_p->server_index);
    return;
}
/******************************************
 * end fbe_api_trace_ssp_edge_info()
 ******************************************/

 /*!**************************************************************
 * fbe_api_trace_stp_edge_info()
 ****************************************************************
 * @brief
 *  Display STP edge info structure.
 *
 * @param edge_info_p - Ptr to the edge info struct to display.
 * @param trace_func - Function to use for printing.
 * @param trace_context - Context for the trace function.               
 *
 * @return - None.
 *
 * @author
 *  06/12/2009 - Created. chenl6
 *
 ****************************************************************/

void fbe_api_trace_stp_edge_info(fbe_api_get_stp_edge_info_t *edge_info_p,
                                 fbe_trace_func_t trace_func,
                                 fbe_trace_context_t trace_context)
{
    /* Display basic info about the STP edge.
     */
    trace_func(trace_context, " STP Edge Info:\n");
    trace_func(trace_context, "   Path state:             0x%x ", edge_info_p->path_state);

    fbe_transport_print_base_edge_path_state(edge_info_p->path_state,
                                             trace_func,
                                             NULL);
    trace_func(trace_context, "\n");
    trace_func(trace_context, "   Path attributes:        0x%x\n", edge_info_p->path_attr);
    trace_func(trace_context, "   Transport id:           0x%x ", edge_info_p->transport_id);

    fbe_transport_print_base_edge_transport_id(edge_info_p->transport_id, 
                                               trace_func, 
                                               NULL);
    trace_func(trace_context, "\n");
    trace_func(trace_context, "   Client id/index:        0x%x/0x%x\n", edge_info_p->client_id, edge_info_p->client_index);
    trace_func(trace_context, "   Server id/index:        0x%x/0x%x\n", edge_info_p->server_id, edge_info_p->server_index);
    return;
}
/******************************************
 * end fbe_api_trace_stp_edge_info()
 ******************************************/

/*************************
  * end file fbe_api_trace.c
  *************************/
 
 
