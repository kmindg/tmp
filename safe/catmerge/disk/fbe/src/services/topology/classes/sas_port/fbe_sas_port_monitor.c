#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_sas_port.h"
#include "fbe_scheduler.h"

#include "base_port_private.h"
#include "sas_port_private.h"

fbe_status_t 
fbe_sas_port_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_sas_port_t * sas_port = NULL;

	sas_port = (fbe_sas_port_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_base_object_trace((fbe_base_object_t*)sas_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	status = fbe_lifecycle_crank_object(&fbe_sas_port_lifecycle_const, (fbe_base_object_t*)sas_port, packet);

	return status;	
}

fbe_status_t fbe_sas_port_monitor_load_verify(void)
{
	return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(sas_port));
}

/*--- condition function prototypes --------------------------------------------------------*/
static fbe_lifecycle_status_t sas_port_pending_func(fbe_base_object_t * base_object, fbe_packet_t * packet);

static fbe_lifecycle_status_t sas_port_get_capabilities_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_port_init_miniport_shim_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_port_get_hardware_info_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_port_self_init_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);
static fbe_lifecycle_status_t sas_port_register_callbacks_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t sas_port_unregister_callbacks_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);

/*--- lifecycle callback functions -----------------------------------------------------*/
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(sas_port);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(sas_port);

static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(sas_port) = {
	FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
		sas_port,
		FBE_LIFECYCLE_NULL_FUNC,       /* online function */
		sas_port_pending_func)
};


/*--- constant derived condition entries -----------------------------------------------*/
static fbe_lifecycle_const_cond_t sas_port_self_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_OBJECT_LIFECYCLE_COND_SELF_INIT,
        sas_port_self_init_cond_function)
};

static fbe_lifecycle_const_cond_t sas_port_init_miniport_shim_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_PORT_LIFECYCLE_COND_INIT_MINIPORT_SHIM,
		sas_port_init_miniport_shim_cond_function)
};

static fbe_lifecycle_const_cond_t sas_port_get_capabilities_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_PORT_LIFECYCLE_COND_GET_CAPABILITIES,
		sas_port_get_capabilities_cond_function)
};

static fbe_lifecycle_const_cond_t sas_port_get_hardware_info_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_PORT_LIFECYCLE_COND_GET_HARDWARE_INFO,
		sas_port_get_hardware_info_cond_function)
};

static fbe_lifecycle_const_cond_t sas_port_register_callbacks_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_PORT_LIFECYCLE_COND_REGISTER_CALLBACKS,
		sas_port_register_callbacks_cond_function)
};

static fbe_lifecycle_const_cond_t sas_port_unregister_callbacks_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_PORT_LIFECYCLE_COND_UNREGISTER_CALLBACKS,
		sas_port_unregister_callbacks_cond_function)
};

/*--- constant base condition entries --------------------------------------------------*/

/* FBE_SAS_PORT_LIFECYCLE_COND_UPDATE_DISCOVERY_TABLE condition */
static fbe_lifecycle_const_base_cond_t sas_port_update_discovery_table_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"sas_port_update_discovery_table_cond",
		FBE_SAS_PORT_LIFECYCLE_COND_UPDATE_DISCOVERY_TABLE,
		FBE_LIFECYCLE_NULL_FUNC),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,        /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,           /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,       /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,            /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sas_port_link_up_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"sas_port_link_up_cond",
		FBE_SAS_PORT_LIFECYCLE_COND_LINK_UP,
		FBE_LIFECYCLE_NULL_FUNC),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,       /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,		/* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,        /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,           /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)        /* DESTROY */
};


static fbe_lifecycle_const_base_cond_t sas_port_driver_reset_begin_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"sas_port_driver_reset_begin_cond",
		FBE_SAS_PORT_LIFECYCLE_COND_DRIVER_RESET_BEGIN_RECEIVED,
		FBE_LIFECYCLE_NULL_FUNC),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,        /* ACTIVATE */
		FBE_LIFECYCLE_STATE_ACTIVATE,        /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,       /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,            /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t sas_port_wait_for_driver_reset_complete_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"sas_port_wait_for_driver_reset_complete_cond",
		FBE_SAS_PORT_LIFECYCLE_COND_WAIT_FOR_DRIVER_RESET_COMPLETE,
		FBE_LIFECYCLE_NULL_FUNC),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,        /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(sas_port)[] = {
	&sas_port_update_discovery_table_cond,	
	&sas_port_driver_reset_begin_cond,
	&sas_port_wait_for_driver_reset_complete_cond,
	&sas_port_link_up_cond,
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(sas_port);

/*--- constant condition entries -------------------------------------------------------*/


/*--- constant rotaries ----------------------------------------------------------------*/

static fbe_lifecycle_const_rotary_cond_t sas_port_specialize_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_port_self_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    /*FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_port_init_miniport_shim_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),*/
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_port_get_hardware_info_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
   	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_port_get_capabilities_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),    
};

static fbe_lifecycle_const_rotary_cond_t sas_port_activate_rotary[] = {
   	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_port_driver_reset_begin_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_port_wait_for_driver_reset_complete_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_port_link_up_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_cond_t sas_port_ready_rotary[] = {
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_port_update_discovery_table_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_port_register_callbacks_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_port_link_up_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_cond_t sas_port_hibernate_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_port_link_up_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_cond_t sas_port_offline_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_port_link_up_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_cond_t sas_port_fail_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_port_link_up_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

/* TODO: Derive FBE_BASE_PORT_LIFECYCLE_COND_UNREGISTER_CALLBACKS	& FBE_BASE_PORT_LIFECYCLE_COND_DESTROY_MINIPORT_SHIM.
     Add to rotary. If different Shims are used..
	 During SPECIALIZE, SAS Port could transition to DESTROY..(?)
*/
static fbe_lifecycle_const_rotary_cond_t sas_port_destroy_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_port_unregister_callbacks_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(sas_port_link_up_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(sas_port)[] = {
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, sas_port_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, sas_port_activate_rotary), 
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, sas_port_ready_rotary), 
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_HIBERNATE, sas_port_hibernate_rotary), 
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_OFFLINE, sas_port_offline_rotary), 
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_FAIL, sas_port_fail_rotary), 
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_DESTROY, sas_port_destroy_rotary), 
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(sas_port);

/*--- global base board lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(sas_port) = {
	FBE_LIFECYCLE_DEF_CONST_DATA(
		sas_port,
		FBE_CLASS_ID_SAS_PORT,                  /* This class */
		FBE_LIFECYCLE_CONST_DATA(base_port))    /* The super class */
};

/*--- Local Functions ------------------------------------------------------------------*/


/*--- Condition Functions --------------------------------------------------------------*/

/*!*************************************************************************
* @fn sas_port_self_init_cond_function(                  
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
sas_port_self_init_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_sas_port_t * sas_port = (fbe_sas_port_t*)p_object;

    
    fbe_base_object_trace((fbe_base_object_t*)sas_port,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry \n", __FUNCTION__ );
    
    fbe_sas_port_init(sas_port);
    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_port);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)sas_port,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s can't clear current condition, status: 0x%X",
                            __FUNCTION__, status);
        
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
sas_port_get_capabilities_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
	fbe_status_t status; 
	fbe_sas_port_t * sas_port = (fbe_sas_port_t*)p_object;
    fbe_port_number_t                   port_number;
    fbe_cpd_shim_port_capabilities_t    port_capabilities;

	fbe_base_object_trace((fbe_base_object_t*)sas_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

    (void)fbe_base_port_get_port_number((fbe_base_port_t*)sas_port, &port_number);
    status = fbe_cpd_shim_get_port_capabilities(port_number,&port_capabilities);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Port Capability query error,status: 0x%X port 0x%X",
								__FUNCTION__, status,port_number);
		/* Setting this condition causes a state change to DESTROY. */
		status = fbe_lifecycle_set_cond(&fbe_sas_port_lifecycle_const,
										(fbe_base_object_t*)sas_port,
										FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
        return FBE_LIFECYCLE_STATUS_DONE;
    }
    /* TODO: Map from CPD informationand store common information in base(?) port.*/
    fbe_base_port_set_maximum_transfer_length((fbe_base_port_t*)sas_port, port_capabilities.maximum_transfer_length);
    fbe_base_port_set_maximum_sg_entries((fbe_base_port_t*)sas_port, port_capabilities.maximum_sg_entries);

    /* TODO: Update to query SAS specific capbilities and store in sas_port.*/

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}

/*!*************************************************************************
* @fn sas_port_init_miniport_shim_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition function initializes the shim.
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
sas_port_init_miniport_shim_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_sas_port_t * sas_port = (fbe_sas_port_t*)base_object;
	fbe_port_number_t port_number;	
#if 0
	fbe_port_name_t port_name;
#endif

	fbe_base_object_trace(	(fbe_base_object_t*)sas_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry\n", __FUNCTION__);

	
	

	status = fbe_base_port_get_port_number((fbe_base_port_t *)sas_port, &port_number);

#if 0
    /* TODO: Move port_address from sas_port to sas_port. */
	status = fbe_cpd_shim_get_port_name(port_number, &port_name);
	if (status == FBE_STATUS_OK){
		sas_port->port_address = port_name.port_name.sas_address;
	}
#endif

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
sas_port_get_hardware_info_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
	fbe_status_t status; 
    fbe_u32_t  vendor_id;
	fbe_sas_port_t * sas_port = (fbe_sas_port_t*)p_object;

	fbe_base_object_trace((fbe_base_object_t*)sas_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);


    /* Retrieve hardware info and specialize to vendor specific port as required.*/
    status = fbe_base_port_get_port_vendor_id((fbe_base_port_t *)sas_port,&vendor_id);
    if (status == FBE_STATUS_OK){
        /* Currently we support only PMC SAS 6G.*/
        if (vendor_id == SAS_PORT_VENDOR_ID_PMC){
            fbe_base_object_lock((fbe_base_object_t*)sas_port);
            fbe_base_object_change_class_id((fbe_base_object_t*)sas_port, FBE_CLASS_ID_SAS_PMC_PORT);
		    fbe_base_object_unlock((fbe_base_object_t*)sas_port);
		    fbe_base_object_trace((fbe_base_object_t *)sas_port, 
                                FBE_TRACE_LEVEL_INFO, 
							    FBE_TRACE_MESSAGE_ID_INFO,
							    "Set port class id: 0x%X \n", FBE_CLASS_ID_SAS_PMC_PORT);
        }else{
            /* Stay as a SAS PORT.*/
            /* Register for SFP notifications.*/
		    status = fbe_lifecycle_set_cond(&fbe_sas_port_lifecycle_const,
										    (fbe_base_object_t*)sas_port,
										    FBE_BASE_PORT_LIFECYCLE_COND_REGISTER_CALLBACKS);
	        if (status != FBE_STATUS_OK) {
		        fbe_base_object_trace((fbe_base_object_t*)sas_port, 
								        FBE_TRACE_LEVEL_ERROR,
								        FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								        "%s Error setting REGISTER_CALLBACK condition.,status: 0x%X",
								        __FUNCTION__, status);
		        /* Setting this condition causes a state change to DESTROY. */
		        status = fbe_lifecycle_set_cond(&fbe_sas_port_lifecycle_const,
										        (fbe_base_object_t*)sas_port,
										        FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
                return FBE_LIFECYCLE_STATUS_DONE;
            }

        }
    }

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}

/*!*************************************************************************
* @fn sas_port_get_port_data_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition function is set during SPECIALIZE to retrieve 
* any required information from the miniport driver through shim.
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
sas_port_get_port_data_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
	fbe_status_t status; 
	fbe_sas_port_t * sas_port = (fbe_sas_port_t*)p_object;

	fbe_base_object_trace((fbe_base_object_t*)sas_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}


/*!*************************************************************************
* @fn sas_port_register_callbacks_cond_function(                  
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
sas_port_register_callbacks_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
	fbe_status_t status; 
	fbe_sas_port_t * sas_port = (fbe_sas_port_t*)p_object;
   	fbe_u32_t port_handle;

	fbe_base_object_trace((fbe_base_object_t*)sas_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

    /* Register callback with shim.*/
	status = fbe_base_port_get_port_number((fbe_base_port_t *)sas_port, &port_handle);
	status = fbe_cpd_shim_port_register_callback(port_handle,                                                 
                                                  (FBE_CPD_SHIM_REGISTER_CALLBACKS_INITIATOR | FBE_CPD_SHIM_REGISTER_CALLBACKS_SFP_EVENTS),
                                                 fbe_sas_port_callback, sas_port);
	if (status == FBE_STATUS_OK) {		
		fbe_base_object_trace((fbe_base_object_t*)sas_port, 
								FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Registered with PMC Shim. Status : 0x%X",
								__FUNCTION__, status);		

	}

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}


/*!*************************************************************************
* @fn sas_port_unregister_callbacks_cond_function(                  
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
sas_port_unregister_callbacks_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
	fbe_status_t status; 
	fbe_sas_port_t * sas_port = (fbe_sas_port_t*)p_object;
   	fbe_port_number_t port_number;


	fbe_base_object_trace((fbe_base_object_t*)sas_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

    /* Register callback with shim.*/
	status = fbe_base_port_get_port_number((fbe_base_port_t *)sas_port, &port_number);
	status = fbe_cpd_shim_port_unregister_callback(port_number);
	if (status == FBE_STATUS_OK) {		
		fbe_base_object_trace((fbe_base_object_t*)sas_port, 
								FBE_TRACE_LEVEL_DEBUG_HIGH,
								FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Registered with PMC Shim. Status : 0x%X",
								__FUNCTION__, status);
	}

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)sas_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)sas_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
sas_port_pending_func(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
	fbe_lifecycle_status_t lifecycle_status;
	fbe_sas_port_t * sas_port = NULL;

	sas_port = (fbe_sas_port_t *)base_object;

	fbe_base_object_trace((fbe_base_object_t*)sas_port,
	                    FBE_TRACE_LEVEL_DEBUG_HIGH,
	                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                    "%s entry\n", __FUNCTION__);

	lifecycle_status = fbe_ssp_transport_server_pending(&sas_port->ssp_transport_server,
														&fbe_sas_port_lifecycle_const,
														(fbe_base_object_t *) sas_port);

	if(lifecycle_status == FBE_LIFECYCLE_STATUS_DONE){
		lifecycle_status = fbe_smp_transport_server_pending(&sas_port->smp_transport_server,
															&fbe_sas_port_lifecycle_const,
															(fbe_base_object_t *) sas_port);
	}

	if(lifecycle_status == FBE_LIFECYCLE_STATUS_DONE){
		lifecycle_status = fbe_stp_transport_server_pending(&sas_port->stp_transport_server,
															&fbe_sas_port_lifecycle_const,
															(fbe_base_object_t *) sas_port);
	}

	fbe_base_object_trace((fbe_base_object_t*)sas_port,
	                    FBE_TRACE_LEVEL_DEBUG_LOW,
	                    FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                    "%s lifecycle_status = %d\n", __FUNCTION__, lifecycle_status);


	return lifecycle_status;
}

