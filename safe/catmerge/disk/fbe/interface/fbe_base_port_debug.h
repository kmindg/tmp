#ifndef FBE_BASE_PORT_DEBUG_H
#define FBE_BASE_PORT_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_base_port_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of base port object.
 *
 * @author 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

/*************************
 *   FUNCTION DEFINITIONS
 *************************/


#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"

fbe_status_t fbe_base_port_debug_trace(const fbe_u8_t * module_name, 
                                      fbe_dbgext_ptr port_object_p, 
                                      fbe_trace_func_t trace_func, 
                                      fbe_trace_context_t trace_context);

#endif /* FBE_BASE_PORT_DEBUG_H */

/*************************
 * end file fbe_base_port_debug.h
 *************************/