#ifndef FBE_COOLING_MGMT_DEBUG_H
#define FBE_COOLING_MGMT_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_cooling_mgmt_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the Cooling Mgmt debug library.
 *
 * @author
 *  23-Oct-2012: PHE - Created
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"

fbe_status_t fbe_cooling_mgmt_debug_trace(const fbe_u8_t * module_name,
                                        fbe_dbgext_ptr cooling_mgmt_p,
                                        fbe_trace_func_t trace_func,
                                        fbe_trace_context_t trace_context);

#endif  /* FBE_COOLING_MGMT_DEBUG_H*/