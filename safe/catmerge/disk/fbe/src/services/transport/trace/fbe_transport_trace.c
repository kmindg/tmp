/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_transport_trace.c
 ***************************************************************************
 *
 * @brief
 *  This file contains trace functions for the fbe transport.
 *
 * @version
 *   4/30/2009:  Created. rfoley
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
 * fbe_transport_print_base_edge_path_state
 ****************************************************************
 * @brief
 *  Display's the trace information related to path state.
 *
 * @param  path_state - Path state.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author
 *  02/13/2009 - Created. Nishit Singh
 *
 ****************************************************************/
fbe_status_t fbe_transport_print_base_edge_path_state(fbe_u32_t path_state,
                                                      fbe_trace_func_t trace_func,
                                                      fbe_trace_context_t trace_context)
{
    switch(path_state)
    {
        case FBE_PATH_STATE_INVALID:
          trace_func(trace_context, "INVALID");
          break;
        case FBE_PATH_STATE_ENABLED:
          trace_func(trace_context, "ENABLED");
          break;
        case FBE_PATH_STATE_DISABLED:
          trace_func(trace_context, "DISABLED");
          break;
        case FBE_PATH_STATE_BROKEN:
          trace_func(trace_context, "BROKEN");
          break;
        case FBE_PATH_STATE_SLUMBER:
          trace_func(trace_context, "SLUMBER");
          break;
        case FBE_PATH_STATE_GONE:
          trace_func(trace_context, "GONE");
          break;
        default:
          trace_func(trace_context, "UNKNOWN PATH STATE 0x%x", path_state);
          break;
    }

    return FBE_STATUS_OK;
}

/****************************************
 * end fbe_transport_print_base_edge_path_state()
 ****************************************/

/*!**************************************************************
 * fbe_tansport_print_base_edge_transport_id
 ****************************************************************
 * @brief
 *  Display's the trace information related to transport id.
 *
 * @param  transport_id - transport id.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author
 *  02/13/2009 - Created. Nishit Singh
 *
 ****************************************************************/
fbe_status_t fbe_transport_print_base_edge_transport_id(fbe_u32_t transport_id, 
                                                        fbe_trace_func_t trace_func, 
                                                        fbe_trace_context_t trace_context)
{
    switch(transport_id)
    {
        case FBE_TRANSPORT_ID_INVALID:
          trace_func(trace_context, "INVALID");
          break;
        case FBE_TRANSPORT_ID_BASE:
          trace_func(trace_context, "BASE");
          break;
        case FBE_TRANSPORT_ID_DISCOVERY:
          trace_func(trace_context, "DISCOVERY");
          break;
        case FBE_TRANSPORT_ID_SSP:
          trace_func(trace_context, "SSP");
          break;
        case FBE_TRANSPORT_ID_BLOCK:
          trace_func(trace_context, "BLOCK");
          break;
        case FBE_TRANSPORT_ID_SMP:
          trace_func(trace_context, "SMP");
          break;
        case FBE_TRANSPORT_ID_STP:
          trace_func(trace_context, "STP");
          break;
        case FBE_TRANSPORT_ID_LAST:
          trace_func(trace_context, "LAST");
          break;
        default:
          trace_func(trace_context, "UNKNOWN TRANSPORT ID 0x%x", transport_id);
          break;
    }

    return FBE_STATUS_OK;
}
/********************************************
 * end fbe_tansport_print_base_edge_transport_id()
 ********************************************/

/*************************
 * end file fbe_transport_trace.c
 *************************/


