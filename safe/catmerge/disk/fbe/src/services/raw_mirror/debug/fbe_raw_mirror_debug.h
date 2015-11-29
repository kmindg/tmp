#ifndef FBE_RAW_MIRROR_DEBUG_H
#define FBE_RAW_MIRROR_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_raw_mirror_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the protocol error injection service
 *  debug library.
 *
 * @author
 *  11/16/2011 - Created.Trupti Ghate
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"

fbe_status_t fbe_raw_mirror_block_edge_debug_trace(const fbe_u8_t * module_name,
                                         fbe_dbgext_ptr raw_mirror_p,
                                         fbe_trace_func_t trace_func,
                                         fbe_trace_context_t trace_context,
                                         fbe_debug_field_info_t *field_info_p,
                                         fbe_u32_t overall_spaces_to_indent);

fbe_status_t fbe_raw_mirror_debug(const fbe_u8_t * module_name,
                                  fbe_dbgext_ptr raw_mirror_ptr,
                                  fbe_trace_func_t trace_func,
                                  fbe_trace_context_t trace_context);
#endif /* FBE_RAW_MIRROR_DEBUG_H */

/*************************
 * end file fbe_raw_mirror_debug.h
 *************************/