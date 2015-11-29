#ifndef FBE_TOPOLOGY_DEBUG_H
#define FBE_TOPOLOGY_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_topology_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the topology debug library.
 *
 * @author
 *  16/06/2011 - Created. Omer Miranda
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe_base.h"

char *fbe_topology_object_getStatusString(fbe_topology_object_status_t topoObjectStatus);

void 
fbe_topology_print_object_by_id(const fbe_u8_t * module_name,
                          fbe_class_id_t filter_class,
                          fbe_object_id_t filter_object,
                          fbe_bool_t b_parity_write_log,
                          fbe_bool_t b_display_io,
                          fbe_bool_t b_summary,
                          fbe_trace_func_t trace_func,
                          fbe_trace_context_t trace_context,
                          fbe_u32_t spaces_to_indent);

#endif /* FBE_TOPOLOGY_DEBUG_H */

/*************************
 * end file fbe_topology_debug.h
 *************************/
