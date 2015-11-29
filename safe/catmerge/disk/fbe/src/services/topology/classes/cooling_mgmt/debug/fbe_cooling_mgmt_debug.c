/***************************************************************************
 * Copyright (C) EMC Corporation 2012
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 * @file fbe_cooling_mgmt_debug.c
 ***************************************************************************
 *
 * @brief
 *  This file contains the debug functions for the Cooling Mgmt object.
 *
 * @author
 *  23-Oct-2012: PHE - Created.
 *
 ***************************************************************************/
/*************************
 *   INCLUDE FILES
 *************************/
#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe_base_object_debug.h"
#include "fbe_base_object.h"
#include "fbe_cooling_mgmt_debug.h"


/*!**************************************************************
 *  fbe_cooling_mgmt_debug_trace(const fbe_u8_t * module_name,
 *                                   fbe_dbgext_ptr cooling_mgmt_p,
 *                                   fbe_trace_func_t trace_func,
 *                                   fbe_trace_context_t trace_context)
 ****************************************************************
 * @brief
 *  Display all the trace information about the cooling mgmt object.
 *
 * @param module_name - This is the name of the module we are debugging.
 * @param board_mgmt_p - Ptr to Cooling Mgmt object.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * @author 
 *  23-Oct-2012: PHE - Created.
 *
 ****************************************************************/
fbe_status_t fbe_cooling_mgmt_debug_trace(const fbe_u8_t * module_name,
                                        fbe_dbgext_ptr cooling_mgmt_p,
                                        fbe_trace_func_t trace_func,
                                        fbe_trace_context_t trace_context)
{
    /* Display the object id.
     */
    trace_func(trace_context, "  object id: ");
    fbe_base_object_debug_trace_object_id(module_name,
                                          cooling_mgmt_p,
                                          trace_func,
                                          trace_context);
    /* Display the class id.
     */
    trace_func(trace_context, "  class id: ");
    fbe_base_object_debug_trace_class_id(module_name,
                                         cooling_mgmt_p,
                                         trace_func,
                                         trace_context);
    /* Display the Life Cycle State.
     */
    trace_func(trace_context, "  lifecycle state: ");
    fbe_base_object_debug_trace_state(module_name,
                                      cooling_mgmt_p,
                                      trace_func,
                                      trace_context);

    trace_func(trace_context, "\n");

   /* Display the Trace Level.
    */
    trace_func(trace_context, "  trace_level: ");
    fbe_base_object_debug_trace_level(module_name,
                                      cooling_mgmt_p,
                                      trace_func,
                                      trace_context);
    trace_func(trace_context, "\n");
    return FBE_STATUS_OK;
}
/******************************************
*   end of fbe_cooling_mgmt_debug_trace()
*******************************************/
