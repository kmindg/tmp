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
 *  This file contains the interfaces of the raw mirror
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

fbe_status_t fbe_raw_mirror_debug(const fbe_u8_t * module_name,
                                  fbe_dbgext_ptr raw_mirror_ptr,
                                  fbe_trace_func_t trace_func,
                                  fbe_trace_context_t trace_context);

#endif /* FBE_RAW_MIRROR_DEBUG_H */

/*************************
 * end file fbe_raw_mirror_debug.h
 *************************/