/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_sas_pmc_port_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for SAS port object.
 *
 * @author
 *   01/16/2009:  Adapted for SAS Port object. 
 *   12/3/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_base_object_debug.h"
#include "fbe_base_port_debug.h"
#include "iscsi_port_private.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * @fn fbe_iscsi_port_debug_trace(const fbe_u8_t * module_name, 
 *                                   fbe_dbgext_ptr logical_drive,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display all the trace information about the port object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param port_object_p - Ptr to port object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *  01/16/2009:  Adapted for SAS Port object.
 *  12/3/2008 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_iscsi_port_debug_trace(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr port_object_p, 
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context)
{

	fbe_base_port_debug_trace(module_name,port_object_p,trace_func,trace_context);    
	/*
    FBE_GET_FIELD_DATA(module_name, 
                        port_object_p,
                        fbe_base_object_s,
                        object_id,
                        sizeof(fbe_object_id_t),
                        &object_id);
	*/

	/* 
	 * TODO: Trace info extracted from the transport servers.
	 */

	return FBE_STATUS_OK;
}
/**************************************
 * end fbe_iscsi_port_debug_trace()
 **************************************/

/*************************
 * end file fbe_iscsi_port_debug.c
 *************************/
