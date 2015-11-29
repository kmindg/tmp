#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_iscsi_port.h"
#include "fbe_scheduler.h"

#include "base_port_private.h"
#include "iscsi_port_private.h"

fbe_status_t 
fbe_iscsi_port_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_iscsi_port_t * iscsi_port = NULL;

	iscsi_port = (fbe_iscsi_port_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_base_object_trace((fbe_base_object_t*)iscsi_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	status = fbe_lifecycle_crank_object(&fbe_iscsi_port_lifecycle_const, (fbe_base_object_t*)iscsi_port, packet);

	return status;	
}

fbe_status_t fbe_iscsi_port_monitor_load_verify(void)
{
	return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(iscsi_port));
}

/*--- condition function prototypes --------------------------------------------------------*/
static fbe_lifecycle_status_t iscsi_port_get_capabilities_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t iscsi_port_init_miniport_shim_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t iscsi_port_get_hardware_info_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t iscsi_port_self_init_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet);
static fbe_lifecycle_status_t iscsi_port_register_callbacks_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);

static fbe_lifecycle_status_t iscsi_port_unregister_callbacks_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);


/*--- lifecycle callback functions -----------------------------------------------------*/
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(iscsi_port);
/*FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(iscsi_port);*/
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_NULL_COND(iscsi_port);

static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(iscsi_port) = {
	FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
		iscsi_port,
		FBE_LIFECYCLE_NULL_FUNC,       /* online function */
		FBE_LIFECYCLE_NULL_FUNC)
};


/*--- constant derived condition entries -----------------------------------------------*/
static fbe_lifecycle_const_cond_t iscsi_port_self_init_cond = {
    FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
        FBE_BASE_OBJECT_LIFECYCLE_COND_SELF_INIT,
        iscsi_port_self_init_cond_function)
};

static fbe_lifecycle_const_cond_t iscsi_port_init_miniport_shim_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_PORT_LIFECYCLE_COND_INIT_MINIPORT_SHIM,
		iscsi_port_init_miniport_shim_cond_function)
};

static fbe_lifecycle_const_cond_t iscsi_port_get_capabilities_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_PORT_LIFECYCLE_COND_GET_CAPABILITIES,
		iscsi_port_get_capabilities_cond_function)
};

static fbe_lifecycle_const_cond_t iscsi_port_get_hardware_info_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_PORT_LIFECYCLE_COND_GET_HARDWARE_INFO,
		iscsi_port_get_hardware_info_cond_function)
};
static fbe_lifecycle_const_cond_t iscsi_port_register_callbacks_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_PORT_LIFECYCLE_COND_REGISTER_CALLBACKS,
		iscsi_port_register_callbacks_cond_function)
};

static fbe_lifecycle_const_cond_t iscsi_port_unregister_callbacks_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_PORT_LIFECYCLE_COND_UNREGISTER_CALLBACKS,
		iscsi_port_unregister_callbacks_cond_function)
};

/*--- constant base condition entries --------------------------------------------------*/

static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(iscsi_port)[] CSX_MAYBE_UNUSED = {
    NULL /* empty for now. */
};

FBE_LIFECYCLE_DEF_CONST_NULL_BASE_CONDITIONS(iscsi_port);

/*--- constant condition entries -------------------------------------------------------*/

/*--- constant rotaries ----------------------------------------------------------------*/

static fbe_lifecycle_const_rotary_cond_t iscsi_port_specialize_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(iscsi_port_self_init_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(iscsi_port_init_miniport_shim_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
   	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(iscsi_port_get_capabilities_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(iscsi_port_get_hardware_info_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t iscsi_port_ready_rotary[] = {    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(iscsi_port_register_callbacks_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t iscsi_port_destroy_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(iscsi_port_unregister_callbacks_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(iscsi_port)[] = {
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, iscsi_port_specialize_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, iscsi_port_ready_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_DESTROY, iscsi_port_destroy_rotary),    
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(iscsi_port);

/*--- global base board lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(iscsi_port) = {
	FBE_LIFECYCLE_DEF_CONST_DATA(
		iscsi_port,
		FBE_CLASS_ID_ISCSI_PORT,                  /* This class */
		FBE_LIFECYCLE_CONST_DATA(base_port))    /* The super class */
};

/*--- Local Functions ------------------------------------------------------------------*/


/*--- Condition Functions --------------------------------------------------------------*/

/*!*************************************************************************
* @fn iscsi_port_self_init_cond_function(                  
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
iscsi_port_self_init_cond_function(fbe_base_object_t * p_object, fbe_packet_t * p_packet)
{
    fbe_status_t status;
    fbe_iscsi_port_t * iscsi_port = (fbe_iscsi_port_t*)p_object;

    
    fbe_base_object_trace((fbe_base_object_t*)iscsi_port,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry \n", __FUNCTION__ );
    
    fbe_iscsi_port_init(iscsi_port);
    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)iscsi_port);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)iscsi_port,
                            FBE_TRACE_LEVEL_ERROR,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                            "%s can't clear current condition, status: 0x%X",
                            __FUNCTION__, status);
        
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/* Stub functions .. */
static fbe_lifecycle_status_t 
iscsi_port_get_capabilities_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
	fbe_status_t status; 
	fbe_iscsi_port_t * iscsi_port = (fbe_iscsi_port_t*)p_object;

	fbe_base_object_trace((fbe_base_object_t*)iscsi_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)iscsi_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)iscsi_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}

/*!*************************************************************************
* @fn iscsi_port_init_miniport_shim_cond_function(                  
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
iscsi_port_init_miniport_shim_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_iscsi_port_t * iscsi_port = (fbe_iscsi_port_t*)base_object;
	fbe_port_number_t port_number;	
#if 0
	fbe_port_name_t port_name;
#endif
	fbe_base_object_trace(	(fbe_base_object_t*)iscsi_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry\n", __FUNCTION__);

	
	

	status = fbe_base_port_get_port_number((fbe_base_port_t *)iscsi_port, &port_number);

#if 0
    /* TODO: Update function for iscsi_port. */
	status = fbe_cpd_shim_get_port_name(port_number, &port_name);
	if (status == FBE_STATUS_OK){
		iscsi_port->port_address = port_name.port_name.sas_address;
	}
#endif

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)iscsi_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)iscsi_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
iscsi_port_get_hardware_info_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
	fbe_status_t status;     
	fbe_iscsi_port_t * iscsi_port = (fbe_iscsi_port_t*)p_object;

	fbe_base_object_trace((fbe_base_object_t*)iscsi_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);


    /* Retrieve hardware info and specialize to vendor specific port as required.*/

    /* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)iscsi_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)iscsi_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}


	return FBE_LIFECYCLE_STATUS_DONE;
}

/*!*************************************************************************
* @fn iscsi_port_register_callbacks_cond_function(                  
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
iscsi_port_register_callbacks_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
	fbe_status_t status; 
	fbe_iscsi_port_t * iscsi_port = (fbe_iscsi_port_t*)p_object;
   	fbe_u32_t port_handle;

	fbe_base_object_trace((fbe_base_object_t*)iscsi_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

    /* Register callback with shim.*/
	status = fbe_base_port_get_port_number((fbe_base_port_t *)iscsi_port, &port_handle);
	status = fbe_cpd_shim_port_register_callback(port_handle,                                                 
                                                 (FBE_CPD_SHIM_REGISTER_CALLBACKS_INITIATOR | FBE_CPD_SHIM_REGISTER_CALLBACKS_SFP_EVENTS),
                                                 fbe_iscsi_port_callback, iscsi_port);
	if (status == FBE_STATUS_OK) {		
		fbe_base_object_trace((fbe_base_object_t*)iscsi_port, 
								FBE_TRACE_LEVEL_DEBUG_HIGH,
								FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Registered with PMC Shim. Status : 0x%X",
								__FUNCTION__, status);		

	}

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)iscsi_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)iscsi_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}

/*!*************************************************************************
* @fn iscsi_port_unregister_callbacks_cond_function(                  
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
iscsi_port_unregister_callbacks_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
	fbe_status_t status; 
	fbe_iscsi_port_t * iscsi_port = (fbe_iscsi_port_t*)p_object;
   	fbe_port_number_t port_number;


	fbe_base_object_trace((fbe_base_object_t*)iscsi_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

    /* Register callback with shim.*/
	status = fbe_base_port_get_port_number((fbe_base_port_t *)iscsi_port, &port_number);
	status = fbe_cpd_shim_port_unregister_callback(port_number);
	if (status == FBE_STATUS_OK) {		
		fbe_base_object_trace((fbe_base_object_t*)iscsi_port, 
								FBE_TRACE_LEVEL_DEBUG_HIGH,
								FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Registered with PMC Shim. Status : 0x%X",
								__FUNCTION__, status);
	}

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)iscsi_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)iscsi_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}
