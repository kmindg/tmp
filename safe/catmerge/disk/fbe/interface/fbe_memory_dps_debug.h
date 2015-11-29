#ifndef FBE_MEMORY_DPS_DEBUG_H
#define FBE_MEMORY_DPS_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_base_config_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the memory dps debug library.
 *
 * @author
 *  03/24/2011 - Created. Swati Fursule
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"


fbe_status_t fbe_memory_dps_statistics_debug_trace(const fbe_u8_t * module_name,
                                                   fbe_trace_func_t trace_func,
                                                   fbe_trace_context_t trace_context,
                                                   fbe_dbgext_ptr base_p,
                                                   fbe_u32_t spaces_to_indent);
#endif /* FBE_MEMORY_DPS_DEBUG_H */

/*************************
 * end file fbe_memory_dps_debug.h
 *************************/