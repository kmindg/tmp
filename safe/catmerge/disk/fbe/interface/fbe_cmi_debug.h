#ifndef FBE_CMI_DEBUG_H
#define FBE_CMI_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_cmi_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the cmi debug library.
 *
 * @author
 *  06/04/2012 - Created. Lili Chen
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"

fbe_status_t fbe_cmi_debug_display_info(const fbe_u8_t * module_name,
                                        fbe_trace_func_t trace_func,
                                        fbe_trace_context_t trace_context,
                                        fbe_u32_t spaces_to_indent);
fbe_status_t fbe_cmi_debug_display_io(const fbe_u8_t * module_name,
                                      fbe_trace_func_t trace_func,
                                      fbe_trace_context_t trace_context,
                                      fbe_u32_t spaces_to_indent);
#endif /* FBE_CMI_DEBUG_H */

/*************************
 * end file fbe_cmi_debug.h
 *************************/