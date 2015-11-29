#ifndef FBE_BOARD_MGMT_DEBUG_H
#define FBE_BOARD_MGMT_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_board_mgmt_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the Board Mgmt debug library.
 *
 * @author
 *  04-Aug-2010: Created  Vaibhav Gaonkar
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_trace.h"

fbe_status_t fbe_board_mgmt_debug_trace(const fbe_u8_t * module_name,
                                        fbe_dbgext_ptr board_mgmt_p,
                                        fbe_trace_func_t trace_func,
                                        fbe_trace_context_t trace_context);

#endif  /* FBE_BOARD_MGMT_DEBUG_H*/