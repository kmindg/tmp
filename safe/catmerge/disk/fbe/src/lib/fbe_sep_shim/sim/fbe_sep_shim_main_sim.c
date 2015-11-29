/***************************************************************************
 * Copyright (C) EMC Corporation 2001-2009
 * All rights reserved.
 * Licensed material -- property of EMC Corporation
 ***************************************************************************/

/***************************************************************************
 *  fbe_sep_shim_main_sim.c
 ***************************************************************************
 *
 *  Description
 *      Simulation implementation for the SEP shim
 *      
 *
 *  History:
 *      06/24/09    sharel - Created
 *    
 ***************************************************************************/
#include "fbe/fbe_winddk.h"
#include "fbe/fbe_types.h"
#include "fbe/fbe_trace_interface.h"
#include "fbe/fbe_library_interface.h"
#include "fbe_service_manager.h"
#include "fbe_topology.h"
#include "fbe/fbe_queue.h"
#include "fbe_sep_shim_private_interface.h"
#include "bvd_interface.h"
#include "simulation/cache_to_mcr_transport.h"
#include "simulation/cache_to_mcr_transport_server_interface.h"
#include "fbe_sep_shim_sim_cache_io.h"


/**************************************
				Local variables
**************************************/

/*******************************************
				Local functions
********************************************/

/*********************************************************************
 *            fbe_sep_shim_init ()
 *********************************************************************
 *
 *  Description: Initialize the sep shim
 *
 *	Inputs: 
 *
 *  Return Value: 
 *
 *    
 *********************************************************************/
fbe_status_t fbe_sep_shim_init (void)
{
    fbe_sep_shim_init_io_memory();
	
	return FBE_STATUS_OK;
}

fbe_status_t fbe_sep_shim_destroy()
{
	fbe_sep_shim_destroy_io_memory();
	return FBE_STATUS_OK;
}

fbe_status_t fbe_sep_shim_sim_map_device_name_to_block_edge(const fbe_u8_t *dev_name, void **block_edge, fbe_object_id_t *bvd_object_id)
{
	return fbe_bvd_interface_map_os_device_name_to_block_edge(dev_name, block_edge, bvd_object_id);

}

fbe_status_t fbe_sep_shim_sim_cache_io_entry(cache_to_mcr_operation_code_t opcode,
											 cache_to_mcr_serialized_io_t * serialized_io,
											 server_command_completion_function_t completion_function,
											 void * completion_context)
{
	switch(opcode) {
	case CACHE_TO_MCR_TRANSPORT_MJ_READ:
	case CACHE_TO_MCR_TRANSPORT_MJ_WRITE:
		fbe_sep_shim_sim_mj_read_write(opcode,
									   serialized_io,
									   completion_function,
									   completion_context);
		break;
	case CACHE_TO_MCR_TRANSPORT_SGL_READ:
	case CACHE_TO_MCR_TRANSPORT_SGL_WRITE:
		fbe_sep_shim_sim_sgl_read_write(serialized_io,
									   completion_function,
									   completion_context);
		break;
	default:
		fbe_sep_shim_trace(FBE_TRACE_LEVEL_ERROR, "%s: Unknown opcode: %d\n", __FUNCTION__, opcode);
		return FBE_STATUS_GENERIC_FAILURE;

	}

	return FBE_STATUS_PENDING;

}

static fbe_bool_t sep_shim_async_io = FBE_FALSE;
static fbe_bool_t sep_shim_async_io_compl = FBE_FALSE;
static fbe_transport_rq_method_t sep_shim_rq_method = FBE_TRANSPORT_RQ_METHOD_SAME_CORE;
static fbe_u32_t sep_shim_alert_time = 0;

fbe_status_t 
fbe_sep_shim_set_async_io(fbe_bool_t async_io)
{
	sep_shim_async_io = async_io;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sep_shim_set_async_io_compl(fbe_bool_t async_io_compl)
{
	sep_shim_async_io_compl = async_io_compl;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sep_shim_set_rq_method(fbe_transport_rq_method_t rq_method)
{
	sep_shim_rq_method = rq_method;
	return FBE_STATUS_OK;
}

fbe_status_t 
fbe_sep_shim_set_alert_time(fbe_u32_t alert_time)
{
	sep_shim_alert_time = alert_time;
	return FBE_STATUS_OK;
}
fbe_status_t fbe_sep_shim_display_irp(PEMCPAL_IRP PIrp, fbe_trace_level_t trace_level)
{
    return FBE_STATUS_OK; 
}
