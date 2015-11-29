#ifndef FBE_BASE_PHYSICAL_DRIVE_DEBUG_H
#define FBE_BASE_PHYSICAL_DRIVE_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_base_physical_drive_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the base physical drive debug library.
 *
 * @author
 *   01/Apr/2009:  Created. chenl6
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"
#include "fbe_base_object.h"

fbe_status_t fbe_base_physical_drive_debug_trace(const fbe_u8_t * module_name,
                                                 fbe_dbgext_ptr base_physical_drive,
                                                 fbe_trace_func_t trace_func,
                                                 fbe_trace_context_t trace_context);
fbe_status_t fbe_sepls_pdo_info_trace(const fbe_u8_t * module_name,
                                      fbe_object_id_t pdo_id,
                                      fbe_u32_t capacity,
                                      fbe_trace_func_t trace_func,
                                      fbe_trace_context_t trace_context);


#endif /* FBE_BASE_PHYSICAL_DRIVE_DEBUG_H */

/*************************
 * end file fbe_base_physical_drive_debug.h
 *************************/