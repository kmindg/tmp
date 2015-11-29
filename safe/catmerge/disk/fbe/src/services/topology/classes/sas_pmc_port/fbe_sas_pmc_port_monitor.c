#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_scheduler.h"
#include "fbe_cpd_shim.h"
#include "fbe_pmc_shim.h"

#include "sas_port_private.h"
#include "sas_pmc_port_private.h"

/* Forward declaration.*/
#define FBE_SAS_PMC_PORT_MAX_DA_LOGIN_DELAY_POLL_COUNT		2 /* ie 6s */

fbe_status_t 
fbe_sas_pmc_port_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_sas_pmc_port_t * sas_pmc_port = NULL;

	sas_pmc_port = (fbe_sas_pmc_port_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	status = fbe_lifecycle_crank_object(&fbe_sas_pmc_port_lifecycle_const, (fbe_base_object_t*)sas_pmc_port, packet);

	return status;	
}

fbe_status_t fbe_sas_pmc_port_monitor_load_verify(void)
{
	return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(sas_pmc_port));
}

/*--- local function prototypes --------------------------------------------------------*/

/*--- local function prototypes --------------------------------------------------------*/
static fbe_lifecycle_status_t sas_pmc_port_discovery_poll_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_pmc_port_discovery_update_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_pmc_port_link_up_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_pmc_port_init_miniport_shim_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_pmc_port_destroy_miniport_shim_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_pmc_port_get_capabilities_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_pmc_port_get_config_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_pmc_port_register_callbacks_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_pmc_port_unregister_callbacks_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_pmc_port_driver_reset_begin_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_pmc_port_wait_for_driver_reset_complete_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_pmc_port_ctrl_reset_complete_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_pmc_port_self_init_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);
fbe_status_t sas_pmc_port_ctrl_reset_kek_register_completion(fbe_packet_t * packet,
                                                             fbe_packet_completion_context_t context);

/*--- lifecycle callback functions -----------------------------------------------------*/
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(sas_pmc_port);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(sas_pmc_port);

static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(sas_pmc_port) = {
	FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
		sas_pmc_port,
		FBE_LIFECYCLE_NULL_FUNC,       /* online function */
		FBE_LIFECYCLE_NULL_FUNC)         /* no pending function */
};

/*--- constant derived condition entries -----------------------------------------------*/
static fbe_lifecycle_const_cond_t sas_pmc_port_self_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_OBJECT_LIFECYCLE_COND_SELF_INIT,
        sas_pmc_port_self_init_cond_function)
};

static fbe_lifecycle_const_cond_t sas_pmc_port_init_miniport_shim_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_PORT_LIFECYCLE_COND_INIT_MINIPORT_SHIM,
		sas_pmc_port_init_miniport_shim_cond_function)
};

static fbe_lifecycle_const_cond_t sas_pmc_port_destroy_miniport_shim_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_PORT_LIFECYCLE_COND_DESTROY_MINIPORT_SHIM,
		sas_pmc_port_destroy_miniport_shim_cond_function)
};

static fbe_lifecycle_const_cond_t sas_pmc_port_discovery_poll_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_TIMER_COND(
		FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_POLL,
		sas_pmc_port_discovery_poll_cond_function)
};

static fbe_lifecycle_const_cond_t sas_pmc_port_discovery_update_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE,
		sas_pmc_port_discovery_update_cond_function)
};

static fbe_lifecycle_const_cond_t sas_pmc_port_link_up_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_SAS_PORT_LIFECYCLE_COND_LINK_UP,
		sas_pmc_port_link_up_cond_function)
};


static fbe_lifecycle_const_cond_t sas_pmc_port_register_callbacks_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_PORT_LIFECYCLE_COND_REGISTER_CALLBACKS,
		sas_pmc_port_register_callbacks_cond_function)
};

static fbe_lifecycle_const_cond_t sas_pmc_port_unregister_callbacks_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_PORT_LIFECYCLE_COND_UNREGISTER_CALLBACKS,
		sas_pmc_port_unregister_callbacks_cond_function)
};


static fbe_lifecycle_const_cond_t sas_pmc_port_driver_reset_begin_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_SAS_PORT_LIFECYCLE_COND_DRIVER_RESET_BEGIN_RECEIVED,
		sas_pmc_port_driver_reset_begin_cond_function)
};

static fbe_lifecycle_const_cond_t sas_pmc_port_wait_for_driver_reset_complete_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_SAS_PORT_LIFECYCLE_COND_WAIT_FOR_DRIVER_RESET_COMPLETE,
		sas_pmc_port_wait_for_driver_reset_complete_cond_function)
};

static fbe_lifecycle_const_base_cond_t sas_pmc_port_ctrl_reset_complete_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"sas_pmc_port_ctrl_reset_complete_cond",
		FBE_SAS_PMC_PORT_LIFECYCLE_COND_CTRL_RESET_COMPLETED,
		sas_pmc_port_ctrl_reset_complete_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,        /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(sas_pmc_port)[] = {
	&sas_pmc_port_ctrl_reset_complete_cond
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(sas_pmc_port);

/*--- constant rotaries ----------------------------------------------------------------*/

static fbe_lifecycle_const_rotary_cond_t sas_pmc_port_specialize_rotary[] = {    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_self_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_init_miniport_shim_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),    
};

static fbe_lifecycle_const_rotary_cond_t sas_pmc_port_activate_rotary[] = {
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_unregister_callbacks_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_driver_reset_begin_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_wait_for_driver_reset_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_ctrl_reset_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_link_up_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),	
};

static fbe_lifecycle_const_rotary_cond_t sas_pmc_port_ready_rotary[] = {		
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_discovery_update_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_discovery_poll_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_register_callbacks_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_driver_reset_begin_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_ctrl_reset_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_link_up_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),	
};

static fbe_lifecycle_const_rotary_cond_t sas_pmc_port_hibernate_rotary[] = {	
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_discovery_update_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_discovery_poll_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_driver_reset_begin_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_ctrl_reset_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_link_up_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),	
};

static fbe_lifecycle_const_rotary_cond_t sas_pmc_port_offline_rotary[] = {	
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_discovery_update_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
		FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_discovery_poll_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_driver_reset_begin_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_ctrl_reset_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_link_up_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),	
};


static fbe_lifecycle_const_rotary_cond_t sas_pmc_port_fail_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_discovery_update_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_driver_reset_begin_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_ctrl_reset_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_link_up_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),	
};

static fbe_lifecycle_const_rotary_cond_t sas_pmc_port_destroy_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_unregister_callbacks_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_destroy_miniport_shim_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_pmc_port_link_up_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(sas_pmc_port)[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, sas_pmc_port_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, sas_pmc_port_activate_rotary),
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, sas_pmc_port_ready_rotary),
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_HIBERNATE, sas_pmc_port_hibernate_rotary),
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_OFFLINE, sas_pmc_port_offline_rotary),
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_FAIL, sas_pmc_port_fail_rotary),
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_DESTROY, sas_pmc_port_destroy_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(sas_pmc_port);

/*--- global PMC  port lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sas_pmc_port) = {
	FBE_LIFECYCLE_DEF_CONST_DATA(
		sas_pmc_port,
		FBE_CLASS_ID_SAS_PMC_PORT,             /* This class */
		FBE_LIFECYCLE_CONST_DATA(sas_port))    /* The super class */
};

/*--- Local Functions ------------------------------------------------------------------*/


/*--- Condition Functions --------------------------------------------------------------*/
/*!*************************************************************************
* @fn sas_pmcport_self_init_cond_function(
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition completes the initialization of sas port object.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
static fbe_lifecycle_status_t 
sas_pmc_port_self_init_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_sas_pmc_port_t * sas_pmc_port = (fbe_sas_pmc_port_t*)p_object;

    
    fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry \n", __FUNCTION__ );
    
    fbe_sas_pmc_port_init(sas_pmc_port);
#if 0
	fbe_lifecycle_set_cond(&fbe_sas_pmc_port_lifecycle_const, 
										(fbe_base_object_t*)sas_pmc_port, 
										FBE_BASE_PORT_LIFECYCLE_COND_INIT_MINIPORT_SHIM);
#endif	
    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_pmc_port);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s can't clear current condition, status: 0x%X",
                            __FUNCTION__, status);
        
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!*************************************************************************
* @fn sas_pmc_port_discovery_poll_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition function is triggered every 3 seconds, 
* and initiates the discovery update.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/

static fbe_lifecycle_status_t 
sas_pmc_port_discovery_poll_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
	fbe_status_t status;
	fbe_sas_pmc_port_t * sas_pmc_port = (fbe_sas_pmc_port_t*)p_object;
    fbe_u32_t assigned_bus_number;

	fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);


    /* We need to disable the monitoring, if da_enclosure is in the middle of activating new firmware */
    if ((sas_pmc_port->monitor_da_enclosure_login) &&
        (!sas_pmc_port->da_enclosure_activating_fw)) {
		sas_pmc_port->da_login_delay_poll_count++;
	}
 
	if (sas_pmc_port->callback_registered){
        status = fbe_base_port_get_assigned_bus_number((fbe_base_port_t *)sas_pmc_port, &assigned_bus_number);
        if ((status== FBE_STATUS_OK) ||(assigned_bus_number != 0xFFFF))
        {
		status = fbe_lifecycle_set_cond(&fbe_sas_pmc_port_lifecycle_const, 
										(fbe_base_object_t*)sas_pmc_port, 
										FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_UPDATE);
		if (status != FBE_STATUS_OK) {
			fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s can't set discovery update condition, status: 0x%X",
									__FUNCTION__, status);
		}
	}
	}

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_pmc_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}


/*!*************************************************************************
* @fn sas_pmc_port_register_callbacks_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition function registers callback functions with the shim.
*
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
static fbe_lifecycle_status_t 
sas_pmc_port_register_callbacks_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
	fbe_status_t status; 
	fbe_sas_pmc_port_t * sas_pmc_port = (fbe_sas_pmc_port_t*)p_object;
   	fbe_u32_t port_handle;

	fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	sas_pmc_port->reset_complete_received = FALSE;

	/* (Re)initialize the table.*/
	if (sas_pmc_port->device_table_ptr != NULL){
		fbe_zero_memory(sas_pmc_port->device_table_ptr,
				sas_pmc_port->device_table_max_index * sizeof(fbe_sas_pmc_device_table_entry_t));
	}
    /* Register callback with shim.*/
	status = fbe_base_port_get_port_number((fbe_base_port_t *)sas_pmc_port, &port_handle);
	status = fbe_cpd_shim_port_register_callback(port_handle,
                                                 (FBE_CPD_SHIM_REGISTER_CALLBACKS_INITIATOR
                                                   | FBE_CPD_SHIM_REGISTER_CALLBACKS_NOTIFY_EXISTING_LOGINS 
                                                   | FBE_CPD_SHIM_REGISTER_CALLBACKS_SFP_EVENTS
												   | FBE_CPD_SHIM_REGISTER_CALLBACKS_ENCRYPTION),
                                                  sas_port_pmc_callback, sas_pmc_port);
	if (status == FBE_STATUS_OK) {
		sas_pmc_port->callback_registered = FBE_TRUE;
		fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
								FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Registered with PMC Shim. Status : 0x%X",
								__FUNCTION__, status);
		
		status = fbe_lifecycle_set_cond(&fbe_sas_pmc_port_lifecycle_const, 
								(fbe_base_object_t*)sas_pmc_port, 
								FBE_BASE_DISCOVERING_LIFECYCLE_COND_DISCOVERY_POLL);
		if (status != FBE_STATUS_OK) {
			fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s can't set discovery poll condition, status: 0x%X",
									__FUNCTION__, status);				
			}

	}

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_pmc_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}

/*!*************************************************************************
* @fn sas_pmc_port_init_miniport_shim_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition function initializes the shim. Queries shim about the
* device table size and initialize the device table.
*
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
static fbe_lifecycle_status_t 
sas_pmc_port_init_miniport_shim_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_sas_pmc_port_t * sas_pmc_port = (fbe_sas_pmc_port_t*)base_object;
	fbe_port_number_t port_number;	
	fbe_u32_t	port_max_dev_table_index;
	fbe_port_name_t port_name;

	fbe_base_object_trace(	(fbe_base_object_t*)sas_pmc_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry\n", __FUNCTION__);

	
    status = fbe_base_port_get_port_number((fbe_base_port_t *)sas_pmc_port, &port_number);
	status = fbe_cpd_shim_get_port_name(port_number, &port_name);
	if (status == FBE_STATUS_OK){
		sas_pmc_port->port_address = port_name.port_name.sas_address;
	}

	status = fbe_cpd_shim_port_get_device_table_max_index(port_number, &port_max_dev_table_index);

    /* Allocate memory for the table.*/
    sas_pmc_port->device_table_ptr = (fbe_sas_pmc_device_table_entry_t*)
        fbe_memory_native_allocate(port_max_dev_table_index*sizeof(fbe_sas_pmc_device_table_entry_t));

    if (sas_pmc_port->device_table_ptr == NULL){
        fbe_base_object_trace((fbe_base_object_t *) sas_pmc_port, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Memory allocation failed!!!\n. status: 0x%X", __FUNCTION__, FBE_STATUS_INSUFFICIENT_RESOURCES);

		status = fbe_lifecycle_set_cond(&fbe_sas_pmc_port_lifecycle_const, 
								(fbe_base_object_t*)sas_pmc_port, 
								FBE_BASE_PORT_LIFECYCLE_COND_DESTROY_MINIPORT_SHIM);
		if (status != FBE_STATUS_OK) {
			fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s can't set destroy miniport shim condition, status: 0x%X",
									__FUNCTION__, status);
		}		
    }else{
        fbe_zero_memory(sas_pmc_port->device_table_ptr,
                port_max_dev_table_index * sizeof(fbe_sas_pmc_device_table_entry_t));
		sas_pmc_port->device_table_max_index = port_max_dev_table_index;

		fbe_cpd_shim_port_register_payload_completion(port_number, sas_pmc_port_send_payload_completion, sas_pmc_port);
		
		status = fbe_lifecycle_set_cond(&fbe_sas_pmc_port_lifecycle_const, 
										(fbe_base_object_t*)sas_pmc_port, 
										FBE_BASE_PORT_LIFECYCLE_COND_REGISTER_CALLBACKS);
		if (status != FBE_STATUS_OK) {
			fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s can't set discovery update condition, status: 0x%X",
									__FUNCTION__, status);
		}
	}	
	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_pmc_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}

/*!*************************************************************************
* @fn sas_pmc_port_destroy_miniport_shim_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition function unlinks from the shim. 
*
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
static fbe_lifecycle_status_t 
sas_pmc_port_destroy_miniport_shim_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_sas_pmc_port_t * sas_pmc_port = (fbe_sas_pmc_port_t*)base_object;
	fbe_port_number_t port_number;

	fbe_base_object_trace(	(fbe_base_object_t*)sas_pmc_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry\n", __FUNCTION__);

	
	status = fbe_base_port_get_port_number((fbe_base_port_t *)sas_pmc_port, &port_number);

	status = fbe_cpd_shim_port_destroy(port_number); 
    /* Release memory allocated for device table.*/
    fbe_memory_native_release(sas_pmc_port->device_table_ptr);
    sas_pmc_port->device_table_ptr = NULL;

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_pmc_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}

/*!*************************************************************************
* @fn sas_pmc_port_link_up_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition function handles link up condition. 
*
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
static fbe_lifecycle_status_t 
sas_pmc_port_link_up_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
	fbe_status_t status; 
	fbe_sas_pmc_port_t * sas_pmc_port = (fbe_sas_pmc_port_t*)p_object;

	fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_pmc_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}

/*!*************************************************************************
* @fn sas_pmc_port_discovery_update_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition function retrieves the device table from the shim and
* updates port's table using the information. Updates in the table will 
* trigger the log in/out of devices and the creation of the direct attached 
* enclosure object.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
static fbe_lifecycle_status_t 
sas_pmc_port_discovery_update_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_sas_pmc_port_t * sas_pmc_port = (fbe_sas_pmc_port_t*)p_object;
    fbe_sas_pmc_device_table_entry_t *this_entry;
    fbe_sas_pmc_device_table_entry_t *parent_entry;
	fbe_lifecycle_status_t	lifecycle_status = FBE_LIFECYCLE_STATUS_RESCHEDULE;
	fbe_bool_t set_change_attribute;
    fbe_bool_t direct_attached_child_inserted;
	/* fbe_u32_t direct_attached_child_index; */
    fbe_u32_t number_of_clients;
    fbe_u32_t server_index,table_index;
    fbe_s32_t ii;
    fbe_status_t status; 
    fbe_smp_edge_t *smp_edge;
    fbe_ssp_edge_t *ssp_edge;
    fbe_stp_edge_t *stp_edge;
	fbe_port_number_t port_number;
	fbe_u32_t							device_table_size = 0;
	fbe_cpd_shim_device_table_t			*shim_device_table_ptr = NULL;
	fbe_cpd_shim_device_table_entry_t	*current_shim_table_entry_ptr = NULL;
	fbe_cpd_shim_device_table_entry_t	local_shim_table_device_entry;
	 fbe_bool_t							smp_change_notify = FALSE;  
	fbe_atomic_t                        local_gen_number;
    fbe_port_role_t					    port_role = FBE_PORT_ROLE_INVALID;
	fbe_u32_t							assigned_bus_number = 0;
	fbe_bool_t                          debug_remove = FBE_FALSE;
    fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    set_change_attribute = direct_attached_child_inserted = FBE_FALSE;
    fbe_base_port_get_debug_remove((fbe_base_port_t *)p_object, &debug_remove);

    status = fbe_base_port_get_assigned_bus_number((fbe_base_port_t *)sas_pmc_port, &assigned_bus_number);
    if ((status!= FBE_STATUS_OK) ||(assigned_bus_number == 0xFFFF))
    {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_pmc_port);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
	    }
        return FBE_LIFECYCLE_STATUS_DONE;
    }
	status = fbe_base_port_get_port_role((fbe_base_port_t *)sas_pmc_port, &port_role);
    if ((status!= FBE_STATUS_OK) ||(port_role != FBE_PORT_ROLE_BE))
    {
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_pmc_port);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
	    }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

	status = fbe_base_port_get_port_number((fbe_base_port_t *)sas_pmc_port, &port_number);
	
	status = fbe_cpd_shim_port_get_device_table_ptr(port_number,&shim_device_table_ptr);	
    if ((status != FBE_STATUS_OK)||(shim_device_table_ptr == NULL)) {
        fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                              FBE_TRACE_LEVEL_ERROR,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s can't get device_table ptr, status: 0x%X",
                              __FUNCTION__, status);

        /* We can't get device_table ptr, DESTROY the port. */
        (void)fbe_lifecycle_set_cond(&fbe_sas_pmc_port_lifecycle_const,
                                     (fbe_base_object_t*)sas_pmc_port,
                                     FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
    

        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_pmc_port);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                                FBE_TRACE_LEVEL_WARNING,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
        }
        return FBE_LIFECYCLE_STATUS_DONE;
    }

	device_table_size = CSX_MIN(shim_device_table_ptr->device_table_size,sas_pmc_port->device_table_max_index);	

    for (ii = (device_table_size - 1); ii >= 0; ii--) {
		current_shim_table_entry_ptr = &(shim_device_table_ptr->device_entry[ii]);
        this_entry = &(sas_pmc_port->device_table_ptr[ii]);      
		/* Does this entry need to be updated? */
		if (current_shim_table_entry_ptr->current_gen_number != this_entry->current_gen_number){
            local_gen_number = current_shim_table_entry_ptr->current_gen_number;
				fbe_copy_memory (&local_shim_table_device_entry,
							current_shim_table_entry_ptr,
			    sizeof(fbe_pmc_shim_device_table_entry_t));
            break;
        }
    }

    if (ii < 0){
        /* No entry to process in the table.*/
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_pmc_port);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
	    }
        lifecycle_status = FBE_LIFECYCLE_STATUS_DONE;
	}else{

	/* Data changed during copy operation. Shim data could be updated at high IRQL level
     *   and is not locked. Reschedule for another pass on this modified data.
     */
    if (local_gen_number != local_shim_table_device_entry.current_gen_number){
        return FBE_LIFECYCLE_STATUS_RESCHEDULE;
    }

			if (this_entry->device_logged_in){
				if (((local_shim_table_device_entry.device_id == this_entry->device_id) &&
						local_shim_table_device_entry.log_out_received) ||
					(local_shim_table_device_entry.device_id != this_entry->device_id)){
						/* Log out device.*/
					fbe_spinlock_lock(&sas_pmc_port->list_lock);
					this_entry->device_logged_in = FBE_FALSE;
					fbe_spinlock_unlock(&sas_pmc_port->list_lock);

                    fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                                   FBE_TRACE_LEVEL_DEBUG_LOW,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s BE %d process logout for device %d.\n",
                                   __FUNCTION__, assigned_bus_number, ii);
						/* edge types are mutually exclusive */
						if (SAS_PMC_PORT_DEVICE_TABLE_ENTRY_IS_SSP_PROTOCOL_EDGE_VALID(this_entry)) {							
							ssp_edge = fbe_sas_port_get_ssp_edge_by_server_index((fbe_sas_port_t *) sas_pmc_port, ii);
							/* clean up the server index, and open will re-establish */
							if (ssp_edge!=NULL) {
                                fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                                   FBE_TRACE_LEVEL_DEBUG_LOW,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s BE %d Setting SSP path to closed for device %d.\n",
                                   __FUNCTION__, assigned_bus_number, ii);
								fbe_sas_port_set_ssp_path_attr((fbe_sas_port_t *) sas_pmc_port, ii, FBE_SSP_PATH_ATTR_CLOSED);
								SAS_PMC_PORT_DEVICE_TABLE_ENTRY_CLEAR_SSP_PROTOCOL_EDGE_VALID(this_entry);
								fbe_ssp_transport_set_server_index(ssp_edge, FBE_EDGE_INDEX_INVALID);								
							}
						}
						else if (SAS_PMC_PORT_DEVICE_TABLE_ENTRY_IS_STP_PROTOCOL_EDGE_VALID(this_entry)) {							
							stp_edge = fbe_sas_port_get_stp_edge_by_server_index((fbe_sas_port_t *) sas_pmc_port, ii);
							/* clean up the server index, and open will re-establish */
							if (stp_edge!=NULL) {
                                fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                                   FBE_TRACE_LEVEL_DEBUG_LOW,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s BE %d Setting STP path to closed for device %d.\n",
                                   __FUNCTION__, assigned_bus_number, ii);
								fbe_sas_port_set_stp_path_attr((fbe_sas_port_t *) sas_pmc_port, ii, FBE_STP_PATH_ATTR_CLOSED);
								SAS_PMC_PORT_DEVICE_TABLE_ENTRY_CLEAR_STP_PROTOCOL_EDGE_VALID(this_entry);
								fbe_stp_transport_set_server_index(stp_edge, FBE_EDGE_INDEX_INVALID);
							}
						}
						else if (SAS_PMC_PORT_DEVICE_TABLE_ENTRY_IS_SMP_PROTOCOL_EDGE_VALID(this_entry)) {							
							smp_edge = fbe_sas_port_get_smp_edge_by_server_index((fbe_sas_port_t *) sas_pmc_port, ii);
							/* clean up the server index, and open will re-establish */
							if (smp_edge!=NULL) {
                                fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                                   FBE_TRACE_LEVEL_DEBUG_LOW,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s BE %d Setting SMP path to closed for device %d.\n",
                                   __FUNCTION__, assigned_bus_number, ii);
								fbe_sas_port_set_smp_path_attr((fbe_sas_port_t *) sas_pmc_port, ii, FBE_SMP_PATH_ATTR_CLOSED);
								SAS_PMC_PORT_DEVICE_TABLE_ENTRY_CLEAR_SMP_PROTOCOL_EDGE_VALID(this_entry);
								fbe_smp_transport_set_server_index(smp_edge, FBE_EDGE_INDEX_INVALID);
							}
						}

						if (FBE_SAS_PMC_IS_DIRECT_ATTACHED_DEVICE(this_entry->parent_device_id)){
							/* Handle log out of direct attached enclosure.*/
                            fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                            						FBE_TRACE_LEVEL_INFO,
                                                   FBE_TRACE_MESSAGE_ID_INFO,
                                                   "%s BE %d setting FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT for device %d.\n",
                                                   __FUNCTION__,assigned_bus_number, ii);
							fbe_base_discovering_set_path_attr((fbe_base_discovering_t *)sas_pmc_port,
																0, FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT);
							sas_pmc_port->da_login_delay_poll_count = 0;
							sas_pmc_port->monitor_da_enclosure_login = FBE_TRUE;

						}else{
							server_index = FBE_CPD_GET_INDEX_FROM_CONTEXT(this_entry->parent_device_id);
							parent_entry = &(sas_pmc_port->device_table_ptr[server_index]);
							if ((parent_entry->device_logged_in) && (parent_entry->device_id == this_entry->parent_device_id)
								&& SAS_PMC_PORT_DEVICE_TABLE_ENTRY_IS_SMP_PROTOCOL_EDGE_VALID(parent_entry)) {
								fbe_sas_port_set_smp_path_attr((fbe_sas_port_t*)sas_pmc_port,
																server_index, FBE_SMP_PATH_ATTR_ELEMENT_CHANGE);
							}
						}					

						//clear the flags for this entry
						this_entry->device_info_flags = 0;
					}
			}

			/* Copy new entry to port device table. */
    fbe_spinlock_lock(&sas_pmc_port->list_lock);
			this_entry->current_gen_number = local_shim_table_device_entry.current_gen_number;
			this_entry->device_type =  local_shim_table_device_entry.device_type;
			this_entry->device_id =  local_shim_table_device_entry.device_id;
			this_entry->device_address =  local_shim_table_device_entry.device_address;
			this_entry->parent_device_id =  local_shim_table_device_entry.parent_device_id;
            this_entry->device_login_reason =  local_shim_table_device_entry.device_login_reason;
			/*copy the information of the device locator, we need it for phy number and so on*/
			fbe_copy_memory (&(this_entry->device_locator),
							&local_shim_table_device_entry.device_locator,
							sizeof(fbe_cpd_shim_device_locator_t));

			this_entry->device_logged_in = 
				(local_shim_table_device_entry.log_out_received ? FBE_FALSE : FBE_TRUE);
			this_entry->generation_code++; 
			if (this_entry->device_logged_in){
				/* Process log-in.*/
				if (FBE_SAS_PMC_IS_DIRECT_ATTACHED_DEVICE(this_entry->parent_device_id)) {
                     fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                                FBE_TRACE_LEVEL_DEBUG_LOW,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s BE %d process login for direct attatched device %d\n",
                                __FUNCTION__, assigned_bus_number, ii);

					sas_pmc_port->da_child_index = (fbe_u32_t)this_entry->device_id;
					sas_pmc_port->da_login_delay_poll_count = 0;
					sas_pmc_port->monitor_da_enclosure_login = FBE_FALSE;
				}else{
					server_index = FBE_CPD_GET_INDEX_FROM_CONTEXT(this_entry->parent_device_id);
					parent_entry = &(sas_pmc_port->device_table_ptr[server_index]);
					if ((parent_entry->device_logged_in) && (parent_entry->device_id == this_entry->parent_device_id) &&
						SAS_PMC_PORT_DEVICE_TABLE_ENTRY_IS_SMP_PROTOCOL_EDGE_VALID(parent_entry)) {
                    /* Notification happens after the lock is released and will use the server_index.*/
                        fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                                   FBE_TRACE_LEVEL_DEBUG_LOW,
                                   FBE_TRACE_MESSAGE_ID_INFO,
                                   "%s BE %d process login for device %d send smp change notification.\n",
                                   __FUNCTION__, assigned_bus_number, ii);

                    smp_change_notify = TRUE;
					}
				}
			}			
    fbe_spinlock_unlock(&sas_pmc_port->list_lock);

    
    if (smp_change_notify){
		fbe_sas_port_set_smp_path_attr((fbe_sas_port_t*)sas_pmc_port,
										server_index, FBE_SMP_PATH_ATTR_ELEMENT_CHANGE);
    }

		lifecycle_status = FBE_LIFECYCLE_STATUS_RESCHEDULE;
	}/* end else of if(ii < 0)*/	


	/* Execute the following code irrespective of whether there is a change in the device table entry for the port. */
	/* We need to do this to ensure that the direct attached enclosure is instantiated again if it self destructs. */
	if (sas_pmc_port->da_child_index != FBE_SAS_PMC_INVALID_TABLE_INDEX){
		table_index = FBE_CPD_GET_INDEX_FROM_CONTEXT(sas_pmc_port->da_child_index);
		this_entry = &(sas_pmc_port->device_table_ptr[table_index]);
		direct_attached_child_inserted = ((this_entry->device_logged_in) &&
											FBE_SAS_PMC_IS_DIRECT_ATTACHED_DEVICE(this_entry->parent_device_id) &&
											!debug_remove);
	}

	number_of_clients = 0;
	(void)fbe_base_discovering_get_number_of_clients((fbe_base_discovering_t *)sas_pmc_port, &number_of_clients);

	if (direct_attached_child_inserted){
		if (number_of_clients == 0){ /* We support only one child.*/
			(void)fbe_base_discovering_instantiate_client((fbe_base_discovering_t *)sas_pmc_port, FBE_CLASS_ID_SAS_ENCLOSURE, 0);
            fbe_spinlock_lock(&sas_pmc_port->list_lock);
            sas_pmc_port->da_enclosure_activating_fw = FBE_FALSE;  /* clear it for new enclosure */
            fbe_spinlock_unlock(&sas_pmc_port->list_lock);
            fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                       FBE_TRACE_LEVEL_DEBUG_LOW,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s BE %d creating direct attatched enclosure object.\n",
                       __FUNCTION__,assigned_bus_number);

		} else {
			/* We need to notify the first enclosure of discovery edge attribute change.*/
            fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                       FBE_TRACE_LEVEL_DEBUG_LOW,
                       FBE_TRACE_MESSAGE_ID_INFO,
                       "%s BE %d clearing FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT for direct attatched enclosure.\n",
                       __FUNCTION__,assigned_bus_number);			 
			fbe_base_discovering_clear_path_attr((fbe_base_discovering_t *)sas_pmc_port, 0, FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT);
		}    
	}else
    {
        if ((assigned_bus_number != 0) &&
            (number_of_clients != 0) && 
            (sas_pmc_port->monitor_da_enclosure_login) || 
            debug_remove)
        {
            if(debug_remove) 
            {
                /*
                 * Debug only. Set the path attribute to simulate the enclosure 0 removal.
                 * This is a work around method to directly destroy the enclosure object.
                 * We need to find a more effective way to simulate the real removal.
                 */
                fbe_base_discovering_set_path_attr((fbe_base_discovering_t *)sas_pmc_port,
                                                    0, FBE_DISCOVERY_PATH_ATTR_REMOVED | FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT);
            } 
            else 
            {
                if(fbe_sas_port_get_power_failure_detected((fbe_sas_port_t *) sas_pmc_port))
                {
                    /* In a case of power failure, ktrace it.*/
                    fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
                                        FBE_TRACE_LEVEL_INFO,
                                        FBE_TRACE_MESSAGE_ID_INFO,
                                        "Power failure detected, setting discovery path to REMOVED for Bus %d",
                                        assigned_bus_number);
                    fbe_base_discovering_set_path_attr((fbe_base_discovering_t *)sas_pmc_port,
                                                       0, 
                                                       FBE_DISCOVERY_PATH_ATTR_REMOVED);
                }
                else if (!sas_pmc_port->da_enclosure_activating_fw)
                {
                    fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
                                    FBE_TRACE_LEVEL_INFO,
                                    FBE_TRACE_MESSAGE_ID_INFO,
                                    "No direct attached children, setting discovery path to REMOVED for Bus %d",
                                    assigned_bus_number);

                /*
                 * We need to set the path attribute to REMOVED. 
                 * Otherwise, when the first enclosure on the bus (except Encl 0_0) was in a Fail lifecycle state,
                 * the first enclosure on the bus can NOT be removed even if the cable was physically disconnected.
                 * - AR732200
                 */
                fbe_base_discovering_set_path_attr((fbe_base_discovering_t *)sas_pmc_port,
                                                    0, FBE_DISCOVERY_PATH_ATTR_REMOVED);
                }
            }
            sas_pmc_port->da_login_delay_poll_count = 0;
            sas_pmc_port->monitor_da_enclosure_login = FBE_FALSE;
        } //if ((assigned_bus_number != 0) && (number_of_clients != 0) && (sas_pmc_port->monitor_da_enclosure_login) || debug_remove)
    }

    return lifecycle_status;
}

/*!*************************************************************************
* @fn sas_pmc_port_unregister_callbacks_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition function unregisters callback with the shim.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
static fbe_lifecycle_status_t 
sas_pmc_port_unregister_callbacks_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
	fbe_status_t status; 
	fbe_sas_pmc_port_t * sas_pmc_port = (fbe_sas_pmc_port_t*)p_object;
   	fbe_port_number_t port_number;
    fbe_u32_t io_port_number;
    fbe_u32_t io_portal_number;


    fbe_base_port_get_io_port_number((fbe_base_port_t *)p_object, &io_port_number);
    fbe_base_port_get_io_portal_number((fbe_base_port_t *)p_object, &io_portal_number);


	fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
	                      FBE_TRACE_LEVEL_INFO,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry for IOPort:%d Portal:%d\n", 
                          __FUNCTION__, 
                          io_port_number, 
                          io_portal_number);

    /* Register callback with shim.*/
	status = fbe_base_port_get_port_number((fbe_base_port_t *)sas_pmc_port, &port_number);
	sas_pmc_port->callback_registered = FBE_FALSE;
	status = fbe_cpd_shim_port_unregister_callback(port_number);
	if (status == FBE_STATUS_OK) {		
		fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
								FBE_TRACE_LEVEL_DEBUG_HIGH,
								FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Registered with PMC Shim. Status : 0x%X",
								__FUNCTION__, status);
	}

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_pmc_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}

/*!*************************************************************************
* @fn sas_pmc_port_driver_reset_begin_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition function handles miniport driver reset.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
static fbe_lifecycle_status_t 
sas_pmc_port_driver_reset_begin_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
	fbe_status_t status; 
	fbe_s32_t table_index = 0;
	fbe_sas_pmc_port_t * sas_pmc_port = (fbe_sas_pmc_port_t*)p_object;
	fbe_sas_pmc_device_table_entry_t *device_table_entry = NULL;
    fbe_sas_pmc_device_table_entry_t *parent_entry = NULL;
    fbe_u32_t server_index;
    fbe_smp_edge_t *smp_edge;
    fbe_ssp_edge_t *ssp_edge;
    fbe_stp_edge_t *stp_edge;
    fbe_u32_t io_port_number;
    fbe_u32_t io_portal_number;
    fbe_u32_t assigned_bus_number = 0;


    fbe_base_port_get_io_port_number((fbe_base_port_t *)p_object, &io_port_number);
    fbe_base_port_get_io_portal_number((fbe_base_port_t *)p_object, &io_portal_number);
    status = fbe_base_port_get_assigned_bus_number((fbe_base_port_t *)sas_pmc_port, &assigned_bus_number);

	fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
	                      FBE_TRACE_LEVEL_INFO,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry for IOPort:%d Portal:%d\n", 
                          __FUNCTION__, 
                          io_port_number, 
                          io_portal_number);

	/* 1. This condition causes a transition to ACTIVATE state. 
	 * 2. Set NOT PRESENT bit on discovery edge.
	 * 3. Set CLOSED bit on all protocol edges.	 
	 */
	
    status = fbe_lifecycle_set_cond(&fbe_sas_pmc_port_lifecycle_const, 
							(fbe_base_object_t*)sas_pmc_port, 
							FBE_SAS_PORT_LIFECYCLE_COND_WAIT_FOR_DRIVER_RESET_COMPLETE);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't set wait for port reset condition, status: 0x%X",
								__FUNCTION__, status);
	}	

    /* Set the CLOSED attribute of all protocol edges.  */
    /* Called in the ACTIVATE state.*/    
    for(table_index = (FBE_SAS_PMC_PORT_MAX_TABLE_ENTRY_COUNT(sas_pmc_port) - 1) ; table_index >= 0 ; table_index--){
        device_table_entry = &(sas_pmc_port->device_table_ptr[table_index]);
        if (device_table_entry->device_logged_in) {
			if (FBE_SAS_PMC_IS_DIRECT_ATTACHED_DEVICE(device_table_entry->parent_device_id)){
                fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                                                   FBE_TRACE_LEVEL_DEBUG_LOW,
                                                   FBE_TRACE_MESSAGE_ID_INFO,
                                                   "%s BE %d setting 1. FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT  device %d.\n",
                                                   __FUNCTION__,assigned_bus_number, table_index);
				fbe_base_discovering_set_path_attr((fbe_base_discovering_t *) sas_pmc_port, 0, FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT);
			}
			/* Log out device.*/
			fbe_spinlock_lock(&sas_pmc_port->list_lock);
			device_table_entry->device_logged_in = FBE_FALSE;
			fbe_spinlock_unlock(&sas_pmc_port->list_lock);

            /* edge types are mutually exclusive */
            if (SAS_PMC_PORT_DEVICE_TABLE_ENTRY_IS_SSP_PROTOCOL_EDGE_VALID(device_table_entry)) {                
                ssp_edge = fbe_sas_port_get_ssp_edge_by_server_index((fbe_sas_port_t *) sas_pmc_port, table_index);
                /* clean up the server index, and open will re-establish */
                if (ssp_edge!=NULL) {
                    fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                                                       FBE_TRACE_LEVEL_DEBUG_LOW,
                                                       FBE_TRACE_MESSAGE_ID_INFO,
                                                       "%s setting SSP Edge to closed BE %d for device %d.\n",
                                                       __FUNCTION__,assigned_bus_number,table_index);

					fbe_sas_port_set_ssp_path_attr((fbe_sas_port_t *) sas_pmc_port, table_index, FBE_SSP_PATH_ATTR_CLOSED);
					SAS_PMC_PORT_DEVICE_TABLE_ENTRY_CLEAR_SSP_PROTOCOL_EDGE_VALID(device_table_entry);
                    fbe_ssp_transport_set_server_index(ssp_edge, FBE_EDGE_INDEX_INVALID);
                }
            }
            else if (SAS_PMC_PORT_DEVICE_TABLE_ENTRY_IS_STP_PROTOCOL_EDGE_VALID(device_table_entry)) {                
                stp_edge = fbe_sas_port_get_stp_edge_by_server_index((fbe_sas_port_t *) sas_pmc_port, table_index);
                /* clean up the server index, and open will re-establish */
                if (stp_edge!=NULL) {
                    fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                                                       FBE_TRACE_LEVEL_DEBUG_LOW,
                                                       FBE_TRACE_MESSAGE_ID_INFO,
                                                       "%s setting STP Edge to closed BE %d for device %d.\n",
                                                       __FUNCTION__,assigned_bus_number,table_index);
					fbe_sas_port_set_stp_path_attr((fbe_sas_port_t *) sas_pmc_port, table_index, FBE_STP_PATH_ATTR_CLOSED);
					SAS_PMC_PORT_DEVICE_TABLE_ENTRY_CLEAR_STP_PROTOCOL_EDGE_VALID(device_table_entry);
                    fbe_stp_transport_set_server_index(stp_edge, FBE_EDGE_INDEX_INVALID);
                }
            }
            else if (SAS_PMC_PORT_DEVICE_TABLE_ENTRY_IS_SMP_PROTOCOL_EDGE_VALID(device_table_entry)) {                
                smp_edge = fbe_sas_port_get_smp_edge_by_server_index((fbe_sas_port_t *) sas_pmc_port, table_index);
                /* clean up the server index, and open will re-establish */
                if (smp_edge!=NULL) {
                    fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                                                       FBE_TRACE_LEVEL_DEBUG_LOW,
                                                       FBE_TRACE_MESSAGE_ID_INFO,
                                                       "%s setting SMP Edge to closed BE %d for device %d.\n",
                                                       __FUNCTION__,assigned_bus_number, table_index);
					fbe_sas_port_set_smp_path_attr((fbe_sas_port_t *) sas_pmc_port, table_index, FBE_SMP_PATH_ATTR_CLOSED);
					SAS_PMC_PORT_DEVICE_TABLE_ENTRY_CLEAR_SMP_PROTOCOL_EDGE_VALID(device_table_entry);
                    fbe_smp_transport_set_server_index(smp_edge, FBE_EDGE_INDEX_INVALID);
                }
            }

			if (FBE_SAS_PMC_IS_DIRECT_ATTACHED_DEVICE(device_table_entry->parent_device_id)){
				/* Handle log out of direct attached enclosure.*/

                fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
                                                   FBE_TRACE_LEVEL_DEBUG_LOW,
                                                   FBE_TRACE_MESSAGE_ID_INFO,
                                                   "%s setting 2. FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT BE %d for device %d.\n",
                                                   __FUNCTION__,assigned_bus_number, table_index);
				fbe_base_discovering_set_path_attr((fbe_base_discovering_t *)sas_pmc_port,
													0, FBE_DISCOVERY_PATH_ATTR_NOT_PRESENT);
				sas_pmc_port->da_login_delay_poll_count = 0;
				sas_pmc_port->monitor_da_enclosure_login = FBE_TRUE;

			}else{
				server_index = FBE_CPD_GET_INDEX_FROM_CONTEXT(device_table_entry->parent_device_id);
				parent_entry = &(sas_pmc_port->device_table_ptr[server_index]);
				if ((parent_entry->device_logged_in) && (parent_entry->device_id == device_table_entry->parent_device_id)
					&& SAS_PMC_PORT_DEVICE_TABLE_ENTRY_IS_SMP_PROTOCOL_EDGE_VALID(parent_entry)) {
					fbe_sas_port_set_smp_path_attr((fbe_sas_port_t*)sas_pmc_port,
													server_index, FBE_SMP_PATH_ATTR_ELEMENT_CHANGE);
				}
			}
        }
    }

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_pmc_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}

/*!*************************************************************************
* @fn sas_pmc_port_wait_for_driver_reset_complete_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition function handles miniport driver reset.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
static fbe_lifecycle_status_t 
sas_pmc_port_wait_for_driver_reset_complete_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
	fbe_status_t status; 
	fbe_lifecycle_status_t  lifecycle_status = FBE_LIFECYCLE_STATUS_DONE;
	fbe_sas_pmc_port_t * sas_pmc_port = (fbe_sas_pmc_port_t*)p_object;
    fbe_u32_t io_port_number;
    fbe_u32_t io_portal_number;


    fbe_base_port_get_io_port_number((fbe_base_port_t *)p_object, &io_port_number);
    fbe_base_port_get_io_portal_number((fbe_base_port_t *)p_object, &io_portal_number);



	fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
	                      FBE_TRACE_LEVEL_INFO,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry for IOPort:%d Portal:%d reset complete:%d\n", 
                          __FUNCTION__, 
                          io_port_number, 
                          io_portal_number,
                          sas_pmc_port->reset_complete_received);

	if (sas_pmc_port->reset_complete_received)
	{

		/* Unregister the current callback..*/
		status = fbe_lifecycle_set_cond(&fbe_sas_pmc_port_lifecycle_const, 
										(fbe_base_object_t*)sas_pmc_port, 
										FBE_BASE_PORT_LIFECYCLE_COND_UNREGISTER_CALLBACKS);
		if (status != FBE_STATUS_OK) {
			fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s can't set discovery update condition, status: 0x%X",
									__FUNCTION__, status);
		}

		/* Register new callback..*/
		
		status = fbe_lifecycle_set_cond(&fbe_sas_pmc_port_lifecycle_const, 
										(fbe_base_object_t*)sas_pmc_port, 
										FBE_BASE_PORT_LIFECYCLE_COND_REGISTER_CALLBACKS);
		if (status != FBE_STATUS_OK) {
			fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s can't set discovery update condition, status: 0x%X",
									__FUNCTION__, status);
		}

		/* Clear the current condition. */
		status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_pmc_port);
		if (status != FBE_STATUS_OK) {
			fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s can't clear current condition, status: 0x%X",
									__FUNCTION__, status);
		}
		lifecycle_status = FBE_LIFECYCLE_STATUS_DONE;
	}
	else
	{
		lifecycle_status =  FBE_LIFECYCLE_STATUS_DONE ;
	}

	return lifecycle_status;
}

/*!*************************************************************************
* @fn sas_pmc_port_ctrl_reset_complete_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition function handles miniport controller reset.
*
* @param      p_object - The pointer to the fbe_base_object_t.
* @param      packet - The pointer to the fbe_packet_t.
*
* @return     fbe_lifecycle_status_t.
*
* NOTES
*
* HISTORY
*
***************************************************************************/
static fbe_lifecycle_status_t 
sas_pmc_port_ctrl_reset_complete_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
	fbe_sas_pmc_port_t * sas_pmc_port = (fbe_sas_pmc_port_t*)p_object;
    fbe_u32_t port_number;
	fbe_base_port_mgmt_register_keys_t port_register_keys;
	fbe_semaphore_t                 kek_semaphore;
	fbe_status_t					status;

	fbe_semaphore_init(&kek_semaphore, 0, 1);

    /* Register the KEK again */
	fbe_zero_memory(&port_register_keys, sizeof(fbe_base_port_mgmt_register_keys_t));

    port_register_keys.key_size = FBE_ENCRYPTION_WRAPPED_KEK_SIZE ;
    port_register_keys.key_ptr = (fbe_key_handle_t *)((fbe_base_port_t *)sas_pmc_port)->kek_key;
    port_register_keys.num_of_keys = 1;
	port_register_keys.kek_handle = ((fbe_base_port_t *)sas_pmc_port)->kek_kek_handle;

	(void)fbe_base_port_get_port_number((fbe_base_port_t *)sas_pmc_port, &port_number);

	fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
						  FBE_TRACE_LEVEL_INFO,
						  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
						  "%s Register keys for Port:%d\n", 
						  __FUNCTION__, 
						  port_number);
	
    fbe_transport_set_completion_function(packet, sas_pmc_port_ctrl_reset_kek_register_completion, &kek_semaphore);
    status = fbe_cpd_shim_register_kek(port_number, &port_register_keys, packet);

	fbe_semaphore_wait(&kek_semaphore, NULL);
    if (status != FBE_STATUS_OK){
		fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s status: 0x%X returned from registration of keys",
								__FUNCTION__, status);
         fbe_transport_set_status(packet, status, 0);
    }
    else {
		fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port,
						  FBE_TRACE_LEVEL_INFO,
						  FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
						  "%s Rebase all keys for Port:%d\n", 
						  __FUNCTION__, 
						  port_number);
		status = fbe_cpd_shim_rebase_all_keys(port_number, port_register_keys.mp_key_handle[0], packet);
		if(status != FBE_STATUS_OK) {
			fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s status:%d: Rebase of keys failed\n",
									__FUNCTION__, status);
		} else {
			fbe_base_port_set_kek_handle((fbe_base_port_t *)sas_pmc_port, port_register_keys.mp_key_handle[0]);
			/* Clear the current condition. */
			status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_pmc_port);
			if (status != FBE_STATUS_OK) {
				fbe_base_object_trace((fbe_base_object_t*)sas_pmc_port, 
									  FBE_TRACE_LEVEL_ERROR,
									  FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									  "%s can't clear current condition, status: 0x%X",
									  __FUNCTION__, status);
			}
		}
	}

	fbe_semaphore_destroy(&kek_semaphore);
	return FBE_LIFECYCLE_STATUS_DONE;
}

fbe_status_t sas_pmc_port_ctrl_reset_kek_register_completion(fbe_packet_t * packet,
                                                             fbe_packet_completion_context_t context)
{
	fbe_semaphore_t *kek_semaphore = (fbe_semaphore_t *)context;

    if (kek_semaphore != NULL) {
        fbe_semaphore_release(kek_semaphore, 0, 1, FALSE); 
    }

	/* We dont want the packet to complete at this point as we have more work to do to rebase the key now*/
    return FBE_STATUS_MORE_PROCESSING_REQUIRED;
}

