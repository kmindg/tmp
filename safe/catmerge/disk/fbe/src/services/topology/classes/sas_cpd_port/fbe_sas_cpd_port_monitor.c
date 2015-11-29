#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"

#include "sas_port_private.h"
#include "sas_cpd_port_private.h"

/* Forward declarations */
static fbe_status_t sas_cpd_port_update_phy_table(fbe_sas_cpd_port_t * sas_cpd_port, fbe_packet_t * packet);
static fbe_status_t sas_cpd_port_process_phy_table(fbe_sas_cpd_port_t * sas_cpd_port, fbe_sas_element_t * phy_table);

fbe_status_t 
fbe_sas_cpd_port_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_sas_cpd_port_t * sas_cpd_port = NULL;

	sas_cpd_port = (fbe_sas_cpd_port_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_base_object_trace((fbe_base_object_t*)sas_cpd_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	status = fbe_lifecycle_crank_object(&fbe_sas_cpd_port_lifecycle_const, (fbe_base_object_t*)sas_cpd_port, packet);

	return status;	
}

fbe_status_t fbe_sas_cpd_port_monitor_load_verify(void)
{
	return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(sas_cpd_port));
}

/*--- local function prototypes --------------------------------------------------------*/

static fbe_lifecycle_status_t sas_cpd_port_discovery_poll_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_cpd_port_update_discovery_table_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_cpd_port_discovery_update_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);

static fbe_lifecycle_status_t sas_cpd_port_init_miniport_shim_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);


/*--- lifecycle callback functions -----------------------------------------------------*/

FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(sas_cpd_port);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(sas_cpd_port);

static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(sas_cpd_port) = {
	FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
		sas_cpd_port,
		FBE_LIFECYCLE_NULL_FUNC,       /* online function */
		FBE_LIFECYCLE_NULL_FUNC)         /* no pending function */
};

/*--- constant derived condition entries -----------------------------------------------*/

static fbe_lifecycle_const_cond_t sas_cpd_port_discovery_poll_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_TIMER_COND(
		FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_POLL,
		sas_cpd_port_discovery_poll_cond_function)
};


static fbe_lifecycle_const_cond_t sas_cpd_port_discovery_update_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE,
		sas_cpd_port_discovery_update_cond_function)
};

static fbe_lifecycle_const_cond_t sas_cpd_port_init_miniport_shim_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_PORT_LIFECYCLE_COND_INIT_MINIPORT_SHIM,
		sas_cpd_port_init_miniport_shim_cond_function)
};

/* FBE_SAS_PORT_LIFECYCLE_COND_UPDATE_DISCOVERY_TABLE condition */
static fbe_lifecycle_const_base_cond_t sas_cpd_port_update_discovery_table_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"sas_cpd_port_update_discovery_table_cond",
		FBE_SAS_CPD_PORT_LIFECYCLE_COND_UPDATE_DISCOVERY_TABLE,
		sas_cpd_port_update_discovery_table_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,        /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,           /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,       /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,            /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};


/*--- constant base condition entries --------------------------------------------------*/

static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(sas_cpd_port)[] = {
	&sas_cpd_port_update_discovery_table_cond,
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(sas_cpd_port);


/*--- constant rotaries ----------------------------------------------------------------*/
static fbe_lifecycle_const_rotary_cond_t sas_cpd_port_specialize_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_cpd_port_init_miniport_shim_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t sas_cpd_port_ready_rotary[] = {
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_cpd_port_discovery_poll_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_cpd_port_update_discovery_table_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_cpd_port_discovery_update_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),	
};


static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(sas_cpd_port)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, sas_cpd_port_specialize_rotary),
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, sas_cpd_port_ready_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(sas_cpd_port);

/*--- global base board lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sas_cpd_port) = {
	FBE_LIFECYCLE_DEF_CONST_DATA(
		sas_cpd_port,
		FBE_CLASS_ID_SAS_CPD_PORT,             /* This class */
		FBE_LIFECYCLE_CONST_DATA(sas_port))    /* The super class */
};

/*--- Local Condition Functions --------------------------------------------------------*/

static fbe_lifecycle_status_t 
sas_cpd_port_invalid_type_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_sas_cpd_port_t * sas_cpd_port = (fbe_sas_cpd_port_t*)base_object;
	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_cpd_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)sas_cpd_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
sas_cpd_port_discovery_poll_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
	fbe_status_t status; 
	fbe_sas_cpd_port_t * sas_cpd_port = (fbe_sas_cpd_port_t*)p_object;

	fbe_base_object_trace((fbe_base_object_t*)sas_cpd_port,
	                      FBE_TRACE_LEVEL_INFO,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_cpd_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)sas_cpd_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
sas_cpd_port_update_discovery_table_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
	fbe_status_t status; 
	fbe_sas_cpd_port_t * sas_cpd_port = (fbe_sas_cpd_port_t*)p_object;

	fbe_base_object_trace((fbe_base_object_t*)sas_cpd_port,
	                      FBE_TRACE_LEVEL_INFO,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	fbe_sas_cpd_port_update_sas_table(sas_cpd_port, packet);

/*
	status = fbe_sas_cpd_port_update_phy_table(sas_cpd_port, packet);
	if (status != FBE_STATUS_OK) {
		return FBE_LIFECYCLE_STATUS_DONE;
	}
*/
	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_cpd_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)sas_cpd_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
sas_cpd_port_discovery_update_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
	fbe_status_t status;
	fbe_sas_cpd_port_t * sas_cpd_port = (fbe_sas_cpd_port_t*)p_object;
	fbe_u32_t number_of_clients = 0;

	fbe_base_object_trace((fbe_base_object_t*)sas_cpd_port,
	                      FBE_TRACE_LEVEL_INFO,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	status = fbe_base_discovering_get_number_of_clients((fbe_base_discovering_t *)sas_cpd_port, &number_of_clients);

	if(number_of_clients == 0) {
		status = fbe_base_discovering_instantiate_client((fbe_base_discovering_t *) sas_cpd_port, 
														FBE_CLASS_ID_SAS_ENCLOSURE, 
														0 /* We should have only one client */);
	}


	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_cpd_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)sas_cpd_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
sas_cpd_port_init_miniport_shim_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_sas_cpd_port_t * sas_cpd_port = (fbe_sas_cpd_port_t*)base_object;
	fbe_port_number_t port_number;

	fbe_base_object_trace(	(fbe_base_object_t*)sas_cpd_port,
							FBE_TRACE_LEVEL_INFO,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry\n", __FUNCTION__);

	status = fbe_base_port_get_port_number((fbe_base_port_t *)sas_cpd_port, &port_number);

	status = fbe_cpd_shim_port_init(port_number, /* io_port_number */ 0); /* We should ask board for io_por_number */

	/* Register io_completion */
	/* fbe_cpd_shim_port_register_io_completion(port_number, sas_port_send_io_completion, sas_cpd_port); */

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_cpd_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)sas_cpd_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}
