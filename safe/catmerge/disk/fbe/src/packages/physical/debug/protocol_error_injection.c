/***************************************************************************
 *  Copyright (C)  EMC Corporation 2000-11
 *  All rights reserved.
 *  Licensed material - property of EMC Corporation. * All rights reserved.
 ***************************************************************************/

/*!**************************************************************************
 * @file protocol_error_injection.c
 ***************************************************************************
 *
 * @brief
 *  This file contains debugging commands about protocol error injection
.*
 * @version
 *   25-Oct-2011:  Created. Omer Miranda
 *
 ***************************************************************************/

#include "pp_dbgext.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_topology_interface.h"
#include "fbe/fbe_class.h"
#include "fbe_base_object_debug.h"
#include "fbe_base_object_trace.h"
#include "fbe/fbe_protocol_error_injection_service.h"
#include "fbe_protocol_error_injection_debug.h"


/* ***************************************************************************
 *
 * @brief
 *  Debug macro to display the protocol error injection related information
 *
 *  Arguments: None
 *  Return Value: None
 * 
 * @version
 *   25-Oct-2011:  Created. Omer Miranda
 *
 ***************************************************************************/

#pragma data_seg ("EXT_HELP$4protocol_error_injection")
static char CSX_MAYBE_UNUSED usageMsg_protocol_error_injection[] =
"!protocol_error_injection\n"
"  Displays all the protocol error injection related information.\n"
"  By default we show all the information\n";
#pragma data_seg ()


CSX_DBG_EXT_DEFINE_CMD(protocol_error_injection, "protocol_error_injection")
{
    fbe_dbgext_ptr fbe_protocol_error_injection_service_ptr = 0;
	fbe_dbgext_ptr protocol_error_injection_started_ptr = 0;
    fbe_u32_t ptr_size;
	fbe_bool_t error_injection_started;

    /* We either are in simulation or on hardware.
     * Depending on where we are, use a different module name. 
     * We validate the module name by checking the ptr size. 
     */
    const fbe_u8_t * module_name;
    fbe_debug_get_ptr_size("NewNeitPackage", &ptr_size);
    if (ptr_size == 0)
    {
        module_name = "new_neit";
    }
    else
    {
        module_name = "NewNeitPackage";
    }

    /* Get the protocol error injection service ptr. */
    FBE_GET_EXPRESSION(module_name, fbe_protocol_error_injection_service, &fbe_protocol_error_injection_service_ptr);
    if(fbe_protocol_error_injection_service_ptr == 0)
    {
        fbe_trace_indent(fbe_debug_trace_func, NULL, 4);
        fbe_debug_trace_func(NULL, "fbe_protocol_error_injection_service_ptr is not available \n");
		return;
    }
	/* Get the protocol error injection service ptr. */
    FBE_GET_EXPRESSION(module_name, protocol_error_injection_started, &protocol_error_injection_started_ptr);
	fbe_trace_indent(fbe_debug_trace_func, NULL, 4);
	FBE_READ_MEMORY(protocol_error_injection_started_ptr, &error_injection_started, sizeof(fbe_bool_t));
	fbe_debug_trace_func(NULL, "protocol_error_injection_started:%s \n",error_injection_started?"TRUE":"FALSE");
	fbe_protocol_error_injection_debug(module_name, fbe_protocol_error_injection_service_ptr, fbe_debug_trace_func, NULL);

    return;
}

//end of scheduler.c
