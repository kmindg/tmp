#ifndef FBE_LOGICAL_DRIVE_DEBUG_H
#define FBE_LOGICAL_DRIVE_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_logical_drive_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the logical drive debug library.
 *
 * @author
 *   12/3/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"

fbe_status_t fbe_logical_drive_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr logical_drive,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context);

fbe_status_t fbe_sepls_ldo_info_trace(const fbe_u8_t * module_name,
                                      fbe_object_id_t ldo_id,
                                      fbe_u32_t capacity,
                                      fbe_trace_func_t trace_func,
                                      fbe_trace_context_t trace_context);


#endif /* FBE_LOGICAL_DRIVE_DEBUG_H */

/*************************
 * end file fbe_logical_drive_debug.h
 *************************/