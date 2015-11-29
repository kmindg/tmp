#ifndef FBE_SPS_MGMT_DEBUG_H
#define FBE_SPS_MGMT_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_sps_mgmt_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the SPS Mgmt debug library.
 *
 * @author
 *  06-May-2010 - Created. Vaibhav Gaonkar
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"


fbe_status_t fbe_sps_mgmt_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr sps_mgmt_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context);
fbe_status_t fbe_sps_mgmt_debug_stat(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr sps_mgmt_p,
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context);


#endif  /* FBE_SPS_MGMT_DEBUG_H*/