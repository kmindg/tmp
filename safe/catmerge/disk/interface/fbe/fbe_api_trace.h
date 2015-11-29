/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_api_trace.h
 ***************************************************************************
 *
 * @brief
 *  This file contains prototypes of trace functions used with the fbe_api.
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

void fbe_api_trace_block_edge_info(fbe_api_get_block_edge_info_t *edge_info_p,
                                   fbe_trace_func_t trace_func,
                                   fbe_trace_context_t trace_context);

void fbe_api_trace_discovery_edge_info(fbe_api_get_discovery_edge_info_t *edge_info_p,
                                       fbe_trace_func_t trace_func,
                                       fbe_trace_context_t trace_context);

void fbe_api_trace_ssp_edge_info(fbe_api_get_ssp_edge_info_t *edge_info_p,
                                 fbe_trace_func_t trace_func,
                                 fbe_trace_context_t trace_context);

void fbe_api_trace_stp_edge_info(fbe_api_get_stp_edge_info_t *edge_info_p,
                                 fbe_trace_func_t trace_func,
                                 fbe_trace_context_t trace_context);

/*************************
 * end file fbe_api_trace.h
 *************************/
