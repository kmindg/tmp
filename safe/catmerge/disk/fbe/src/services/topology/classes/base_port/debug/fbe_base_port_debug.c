/***************************************************************************
 * Copyright (C) EMC Corporation 2008
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_base_port_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for base port object.
 *
 * @author
 *   01/16/2009:  Adapted for Base Port object. 
 *   12/3/2008:  Created. RPF
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_base_object_debug.h"
#include "base_port_private.h"

/*************************
 *   FUNCTION DEFINITIONS
 *************************/

/*!**************************************************************
 * @fn fbe_base_port_debug_trace(const fbe_u8_t * module_name, 
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
 *  01/16/2009:  Adapted for Base Port object.
 *  12/3/2008 - Created. RPF
 *
 ****************************************************************/

fbe_status_t fbe_base_port_debug_trace(const fbe_u8_t * module_name, 
                                           fbe_dbgext_ptr port_object_p, 
                                           fbe_trace_func_t trace_func, 
                                           fbe_trace_context_t trace_context)
{
    fbe_u32_t io_port_number,io_portal_number,assigned_bus_number;   

    csx_dbg_ext_print("  Class id: ");
    fbe_base_object_debug_trace_class_id(module_name, port_object_p, trace_func, trace_context);
    
    csx_dbg_ext_print("  Lifecycle State: ");
    fbe_base_object_debug_trace_state(module_name, port_object_p, trace_func, trace_context);

    csx_dbg_ext_print("\n");

	/*
    FBE_GET_FIELD_DATA(module_name, 
                        port_object_p,
                        fbe_base_object_s,
                        object_id,
                        sizeof(fbe_object_id_t),
                        &object_id);
	*/

    FBE_GET_FIELD_DATA(module_name, 
                        port_object_p,
                        fbe_base_port_s,
                        io_port_number,
                        sizeof(fbe_u32_t),
                        &io_port_number);
    FBE_GET_FIELD_DATA(module_name, 
                        port_object_p,
                        fbe_base_port_s,
                        io_portal_number,
                        sizeof(fbe_u32_t),
                        &io_portal_number);
    FBE_GET_FIELD_DATA(module_name, 
                        port_object_p,
                        fbe_base_port_s,
                        assigned_bus_number,
                        sizeof(fbe_u32_t),
                        &assigned_bus_number);


    csx_dbg_ext_print("\t\tIO Port: 0x%x ", io_port_number);
    csx_dbg_ext_print("IO Portal: 0x%x ", io_portal_number);
	csx_dbg_ext_print("Backend Port #: 0x%x\n", assigned_bus_number);


	return FBE_STATUS_OK;
}
/**************************************
 * end fbe_base_port_debug_trace()
 **************************************/

/*************************
 * end file fbe_base_port_debug.c
 *************************/
