/***************************************************************************
 * Copyright (C) EMC Corporation 2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!*************************************************************************
 * @file fbe_block_transport_trace.h
 ***************************************************************************
 *
 * @brief
 *  This file contains prototypes of trace functions used with the block transport.
 *
 * @version
 *   5/07/2009:  Created. rfoley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

void fbe_block_transport_trace_negotiate_info(fbe_block_transport_negotiate_t *negotiate_p,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context);

void fbe_block_transport_trace_identify_hex(fbe_block_transport_identify_t *identify_p,
                                            fbe_trace_func_t trace_func,
                                            fbe_trace_context_t trace_context);

void fbe_block_transport_trace_identify_char(fbe_block_transport_identify_t *identify_p,
                                             fbe_trace_func_t trace_func,
                                             fbe_trace_context_t trace_context);

/*************************
 * end file fbe_block_transport_trace.h
 *************************/
