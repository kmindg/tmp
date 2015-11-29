/***************************************************************************
 * Copyright (C) EMC Corporation 2008-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/*!**************************************************************************
 * @file fbe_dmrb_transport_debug.c
 ****************************************************************************
 *
 * @brief
 *  This file contains the debug functions for Block Transport.
 *
 * @author
 * 
 *
 ***************************************************************************/

/*************************
 *   INCLUDE FILES
 *************************/

#include "fbe/fbe_types.h"
#include "fbe/fbe_dbgext.h"
#include "fbe/fbe_payload_dmrb_operation.h"
#include "fbe/fbe_port.h"
#include "fbe_base_object_debug.h"
#include "fbe_base_object_trace.h"
#include "fbe_logical_drive_debug.h"
#include "fbe_transport_debug.h"


/*!**************************************************************
 * fbe_transport_print_fbe_packet_dmrb_payload()
 ****************************************************************
 * @brief
 *  Display all the trace information related to DMRB operation
 *
 * @param  dmrb_operation_p - Ptr to DMRB operation payload.
 * @param trace_func - Function to use when tracing.
 * @param trace_context - The context to be passed to the trace function.
 *
 * @return - FBE_STATUS_OK  
 *
 * HISTORY:
 *
 ****************************************************************/

fbe_status_t fbe_tansport_print_fbe_packet_dmrb_payload(const fbe_u8_t * module_name,
                                                         fbe_u64_t dmrb_operation_p,
                                                         fbe_trace_func_t trace_func,
                                                         fbe_trace_context_t trace_context,
                                                         fbe_u32_t spaces_to_indent)
{   
	fbe_u32_t offset;
	fbe_u64_t dmrb_opaque_p;

	FBE_GET_FIELD_OFFSET(module_name, fbe_payload_dmrb_operation_t, "opaque", &offset);
	dmrb_opaque_p = dmrb_operation_p + offset;
	trace_func(trace_context, "  Opaque: 0x%llx \n",
		   (unsigned long long)dmrb_opaque_p );
	/* fbe_cpd_shim_debug_trace_dmrb_payload(dmrb_opaque_p,trace_func,trace_context,spaces_to_indent + 4);*/

    return FBE_STATUS_OK;
}
/*************************************************
 * end fbe_tansport_print_fbe_packet_dmrb_payload()
 *************************************************/
