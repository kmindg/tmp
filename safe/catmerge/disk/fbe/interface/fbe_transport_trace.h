/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_transport_trace.h
 ***************************************************************************
 *
 * @brief
 *  This file contains prototypes of trace functions used with transports.
 *
 * @version
 *   4/30/2009:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

fbe_status_t fbe_transport_print_base_edge_path_state(fbe_u32_t path_state,
                                                      fbe_trace_func_t trace_func,
                                                      fbe_trace_context_t trace_context);

fbe_status_t fbe_transport_print_base_edge_transport_id(fbe_u32_t transport_id, 
                                                        fbe_trace_func_t trace_func, 
                                                        fbe_trace_context_t trace_context);
/*************************
 * end file fbe_transport_trace.h
 *************************/
