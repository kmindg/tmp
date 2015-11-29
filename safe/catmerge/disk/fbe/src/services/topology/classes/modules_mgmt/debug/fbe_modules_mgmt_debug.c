/***************************************************************************
 * Copyright (C) EMC Corporation 2010
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_modules_mgmt_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the Modules Mgmt object.
 *
 * @author
 *  6-May-2010 - Created. Vaibhav Gaonkar
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_base_object_debug.h"
#include "fbe_base_object.h"
#include "fbe_modules_mgmt_debug.h"


/*!**************************************************************
 * @fn fbe_modules_mgmt_debug_trace(const fbe_u8_t * module_name,
 *                                   fbe_dbgext_ptr modules_mgmt_p,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display all the trace information about the virtual drive object.
 *
 * @param  module_name - This is the name of the module we are debugging.
 * @param modules_mgmt_p - Ptr to modules Mgmt object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author 
 *  6-May-2010 - Created. Vaibhav Gaonkar
 *
 ****************************************************************/
fbe_status_t fbe_modules_mgmt_debug_trace(const fbe_u8_t * module_name,
                                           fbe_dbgext_ptr modules_mgmt_p,
                                           fbe_trace_func_t trace_func,
                                           fbe_trace_context_t trace_context)
{
    /* Display the object id.
     */
    trace_func(trace_context, "  object id: ");
    fbe_base_object_debug_trace_object_id(module_name,
                                          modules_mgmt_p,
                                          trace_func,
                                          trace_context);
    /* Display the class id.
     */
    trace_func(trace_context, "  class id: ");
    fbe_base_object_debug_trace_class_id(module_name,
                                         modules_mgmt_p,
                                         trace_func,
                                         trace_context);
    /* Display the Life Cycle State.
     */
    trace_func(trace_context, "  lifecycle state: ");
    fbe_base_object_debug_trace_state(module_name,
                                      modules_mgmt_p,
                                      trace_func,
                                      trace_context);
    
    trace_func(trace_context, "\n");

   /* Display the Trace Level.
    */
    trace_func(trace_context, "  trace_level: ");
    fbe_base_object_debug_trace_level(module_name,
                                      modules_mgmt_p,
                                      trace_func,
                                      trace_context);

    trace_func(trace_context, "\n");

    return FBE_STATUS_OK;
}
