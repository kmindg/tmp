#ifndef FBE_PS_MGMT_DEBUG_H
#define FBE_PS_MGMT_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_ps_mgmt_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the PS Mgmt debug library.
 *
 * @author
 *  06-May-2010 - Created. Vaibhav Gaonkar
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe_trace.h"
#include "fbe/fbe_devices.h"
#include "fbe/fbe_dbgext.h"

fbe_status_t fbe_ps_mgmt_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr ps_mgmt_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context);
fbe_status_t fbe_ps_mgmt_debug_stat(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr ps_mgmt_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context,
                                           fbe_device_physical_location_t *plocation,
                                           fbe_bool_t   revision_or_status,
                                           fbe_bool_t   location_flag);

#endif  /* FBE_PS_MGMT_DEBUG_H*/
