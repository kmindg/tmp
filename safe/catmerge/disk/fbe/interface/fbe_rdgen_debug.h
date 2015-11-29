#ifndef FBE_RDGEN_DEBUG_H
#define FBE_RDGEN_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_rdgen_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the rdgen debug library.
 *
 * @author
 *  3/25/2010 - Created. Rob Foley
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"

fbe_status_t fbe_rdgen_debug_display(const fbe_u8_t * module_name,
                                     fbe_trace_func_t trace_func,
                                     fbe_trace_context_t trace_context,
                                     fbe_u32_t spaces_to_indent,
                                     fbe_bool_t b_display_packets,
                                     fbe_bool_t b_display_queues);
#endif /* FBE_RDGEN_DEBUG_H */

/*************************
 * end file fbe_rdgen_debug.h
 *************************/