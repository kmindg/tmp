#ifndef FBE_SPS_DEBUG_H
#define FBE_SPS_DEBUG_H
/***************************************************************************
 * Copyright (C) EMC Corporation 2011
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_sps_debug.h
 ***************************************************************************
 *
 * @brief
 *  This file contains the interfaces of the sps mgmt debug library for
 *  the sps_mgmt debug macro.
 *
 * @author
 *  19/9/2011 - Created. Omer Miranda
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_sps_mgmt_private.h"

char *fbe_sps_mgmt_get_sps_test_state_string (fbe_sps_testing_state_t spsTestState,
                                              fbe_trace_func_t trace_func,
                                              fbe_trace_context_t trace_context);

#endif /* FBE_SPS_DEBUG_H */

/*************************
 * end file fbe_sps_debug.h
 *************************/