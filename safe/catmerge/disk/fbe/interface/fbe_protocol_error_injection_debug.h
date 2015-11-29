#ifndef FBE_PROTOCOL_ERROR_INJECTION_DEBUG_H
#define FBE_PROTOCOL_ERROR_INJECTION_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_protocol_error_injection_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the protocol error injection
 *  debug library.
 *
 * @author
 *  10/28/2011 - Created.Omer Miranda
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"

fbe_status_t fbe_protocol_error_injection_debug(const fbe_u8_t * module_name,
												fbe_dbgext_ptr protocol_error_service_ptr,
                                                fbe_trace_func_t trace_func,
                                                fbe_trace_context_t trace_context);
#endif /* FBE_PROTOCOL_ERROR_INJECTION_DEBUG_H */

/*************************
 * end file fbe_protocol_error_injection_debug.h
 *************************/