#include "fbe/fbe_winddk.h"
#include "fbe/fbe_memory.h"
#include "fbe_topology.h"
#include "fbe_base_port.h"
#include "fbe_scheduler.h"
#include "fbe_cpd_shim.h"
#include "fbe/fbe_notification_interface.h"
#include "fbe_notification.h"

#include "base_discovering_private.h"
#include "base_port_private.h"
#include "fbe/fbe_drive_configuration_interface.h"

fbe_status_t 
fbe_base_port_monitor_entry(fbe_object_handle_t object_handle, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_base_port_t * base_port = NULL;

	base_port = (fbe_base_port_t *)fbe_base_handle_to_pointer(object_handle);

	fbe_base_object_trace((fbe_base_object_t*)base_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	status = fbe_lifecycle_crank_object(&fbe_base_port_lifecycle_const, (fbe_base_object_t*)base_port, packet);

	return status;	
}

fbe_status_t fbe_base_port_monitor_load_verify(void)
{
	return fbe_lifecycle_class_const_verify(&FBE_LIFECYCLE_CONST_DATA(base_port));
}

/*--- local function prototypes --------------------------------------------------------*/

static fbe_lifecycle_status_t base_port_invalid_type_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_port_update_port_number_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_port_get_hardware_info_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_port_get_config_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_port_sfp_information_change_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_port_unregister_config_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_port_register_config_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_port_init_miniport_shim_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_port_destroy_miniport_shim_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_port_get_port_data_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);

static fbe_status_t base_port_convert_hardware_info(fbe_cpd_shim_hardware_info_t *hdw_info, fbe_port_hardware_info_t *port_hardware_info);
static fbe_port_role_t base_port_translate_port_role(fbe_cpd_shim_port_role_t cpd_shim_port_role);
static fbe_port_connect_class_t base_port_translate_port_connect_class(fbe_cpd_shim_connect_class_t cpd_shim_port_type);
static fbe_status_t base_port_convert_sfp_info(fbe_cpd_shim_sfp_media_interface_info_t *media_interface_info,
                                               fbe_port_sfp_info_t                     *port_sfp_info);

static void base_port_send_link_data_change_notification(fbe_base_port_t * base_port);
static fbe_status_t base_port_convert_shim_link_info(fbe_cpd_shim_port_lane_info_t   *cpd_port_lane_info,
                                                     fbe_cpd_shim_connect_class_t    cpd_connect_class,
                                                     fbe_port_link_information_t  *port_link_info);
static fbe_lifecycle_status_t base_port_register_callbacks_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);
static fbe_lifecycle_status_t base_port_unregister_callbacks_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet);

/*--- lifecycle callback functions -----------------------------------------------------*/

FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_DATA(base_port);
FBE_LIFECYCLE_DEF_CALLBACK_GET_INST_COND(base_port);

static fbe_lifecycle_const_callback_t FBE_LIFECYCLE_CALLBACKS(base_port) = {
	FBE_LIFECYCLE_DEF_CONST_CALLBACKS(
		base_port,
		FBE_LIFECYCLE_NULL_FUNC,       /* online function */
		FBE_LIFECYCLE_NULL_FUNC)         /* no pending function */
};

/*--- constant derived condition entries -----------------------------------------------*/

static fbe_lifecycle_const_cond_t base_port_invalid_type_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_DISCOVERED_LIFECYCLE_COND_INVALID_TYPE,
		base_port_invalid_type_cond_function)
};

static fbe_lifecycle_const_cond_t base_port_get_port_object_id_cond = {
	FBE_LIFECYCLE_DEF_CONST_DERIVED_COND(
		FBE_BASE_DISCOVERED_LIFECYCLE_COND_GET_PORT_OBJECT_ID,
		FBE_LIFECYCLE_NULL_FUNC)
};

/*--- constant base condition entries -------------------------------------------------------*/

/* FBE_BASE_PORT_LIFECYCLE_COND_UPDATE_PORT_NUMBER condition */
static fbe_lifecycle_const_base_cond_t base_port_update_port_number_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"base_port_update_port_number_cond",
		FBE_BASE_PORT_LIFECYCLE_COND_UPDATE_PORT_NUMBER,
		base_port_update_port_number_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)        /* DESTROY */
};

/* FBE_BASE_PORT_LIFECYCLE_COND_INIT_MINIPORT_SHIM condition */
static fbe_lifecycle_const_base_cond_t base_port_init_miniport_shim_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"base_port_init_miniport_shim_cond",
		FBE_BASE_PORT_LIFECYCLE_COND_INIT_MINIPORT_SHIM,
		base_port_init_miniport_shim_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)        /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_port_get_hardware_info_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"base_port_get_hardware_info_cond",
		FBE_BASE_PORT_LIFECYCLE_COND_GET_HARDWARE_INFO,
		base_port_get_hardware_info_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)        /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_port_get_config_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"base_port_get_config_cond ",
		FBE_BASE_PORT_LIFECYCLE_COND_GET_CONFIG,
		base_port_get_config_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)        /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_port_get_capabilities_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"base_port_get_capabilities_cond",
		FBE_BASE_PORT_LIFECYCLE_COND_GET_CAPABILITIES,
		FBE_LIFECYCLE_NULL_FUNC),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_port_get_port_data_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"base_port_get_port_data_cond",
		FBE_BASE_PORT_LIFECYCLE_COND_GET_PORT_DATA,
		base_port_get_port_data_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,        /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,           /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,       /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,            /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_port_sfp_information_change_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"base_port_sfp_information_change_cond",
		FBE_BASE_PORT_LIFECYCLE_COND_SFP_INFORMATION_CHANGE,
		base_port_sfp_information_change_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,      /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,        /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,           /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)         /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_port_register_callbacks_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"base_port_register_callbacks_cond",
		FBE_BASE_PORT_LIFECYCLE_COND_REGISTER_CALLBACKS,
		base_port_register_callbacks_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,     /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,        /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,          /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)        /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_port_unregister_callbacks_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"base_port_unregister_callbacks_cond",
		FBE_BASE_PORT_LIFECYCLE_COND_UNREGISTER_CALLBACKS,
		base_port_unregister_callbacks_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,        /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)        /* DESTROY */
};

/* FBE_BASE_PORT_LIFECYCLE_COND_DESTROY_MINIPORT_SHIM condition */
static fbe_lifecycle_const_base_cond_t base_port_destroy_miniport_shim_cond = {
	FBE_LIFECYCLE_DEF_CONST_BASE_COND(
		"base_port_destroy_miniport_shim",
		FBE_BASE_PORT_LIFECYCLE_COND_DESTROY_MINIPORT_SHIM,
		base_port_destroy_miniport_shim_cond_function),
	FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
		FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
		FBE_LIFECYCLE_STATE_ACTIVATE,         /* ACTIVATE */
		FBE_LIFECYCLE_STATE_READY,         /* READY */
		FBE_LIFECYCLE_STATE_HIBERNATE,         /* HIBERNATE */
		FBE_LIFECYCLE_STATE_OFFLINE,         /* OFFLINE */
		FBE_LIFECYCLE_STATE_FAIL,         /* FAIL */
		FBE_LIFECYCLE_STATE_DESTROY)        /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_port_register_config_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_port_register_config_cond",
        FBE_BASE_PORT_LIFECYCLE_COND_REGISTER_CONFIG,
        base_port_register_config_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t base_port_unregister_config_cond = {
    FBE_LIFECYCLE_DEF_CONST_BASE_COND(
        "base_port_unregister_config_cond",
        FBE_BASE_PORT_LIFECYCLE_COND_UNREGISTER_CONFIG,
        base_port_unregister_config_cond_function),
    FBE_LIFECYCLE_DEF_CONST_COND_STATE_TRANSITIONS(
        FBE_LIFECYCLE_STATE_SPECIALIZE,         /* SPECIALIZE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* ACTIVATE */
        FBE_LIFECYCLE_STATE_ACTIVATE,           /* READY */
        FBE_LIFECYCLE_STATE_HIBERNATE,          /* HIBERNATE */
        FBE_LIFECYCLE_STATE_OFFLINE,            /* OFFLINE */
        FBE_LIFECYCLE_STATE_FAIL,               /* FAIL */
        FBE_LIFECYCLE_STATE_DESTROY)            /* DESTROY */
};

static fbe_lifecycle_const_base_cond_t * FBE_LIFECYCLE_BASE_COND_ARRAY(base_port)[] = {
	&base_port_update_port_number_cond,
	&base_port_init_miniport_shim_cond,	
    &base_port_get_hardware_info_cond,    
	&base_port_get_config_cond,
    &base_port_sfp_information_change_cond,
    &base_port_get_capabilities_cond,    
    &base_port_register_callbacks_cond,
    &base_port_get_port_data_cond,
	&base_port_unregister_callbacks_cond,
	&base_port_destroy_miniport_shim_cond,
    &base_port_register_config_cond,
	&base_port_unregister_config_cond
	
};

FBE_LIFECYCLE_DEF_CONST_BASE_CONDITIONS(base_port);

/*--- constant rotaries ----------------------------------------------------------------*/

static fbe_lifecycle_const_rotary_cond_t base_port_specialize_rotary[] = {
	/* Derived conditions */	
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_port_get_port_object_id_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),

	/* Base conditions */
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_port_invalid_type_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_port_update_port_number_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_port_init_miniport_shim_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_port_get_hardware_info_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_port_get_config_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_port_get_port_data_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_cond_t base_port_activate_rotary[] = {    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_port_sfp_information_change_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_port_get_port_data_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_port_unregister_config_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_port_register_config_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t base_port_ready_rotary[] = {    
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_port_sfp_information_change_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_port_register_callbacks_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_port_get_port_data_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
};

static fbe_lifecycle_const_rotary_cond_t base_port_destroy_rotary[] = {
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_port_get_port_data_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_port_unregister_callbacks_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
    FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_port_destroy_miniport_shim_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_IS_PRESET),
	FBE_LIFECYCLE_DEF_CONST_ROTARY_COND(base_port_unregister_config_cond, FBE_LIFECYCLE_ROTARY_COND_ATTR_NULL),
};

static fbe_lifecycle_const_rotary_t FBE_LIFECYCLE_ROTARY_ARRAY(base_port)[] = {
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_SPECIALIZE, base_port_specialize_rotary),
	FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_ACTIVATE, base_port_activate_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_READY, base_port_ready_rotary),
    FBE_LIFECYCLE_DEF_CONST_ROTARY(FBE_LIFECYCLE_STATE_DESTROY, base_port_destroy_rotary),
};

FBE_LIFECYCLE_DEF_CONST_ROTARIES(base_port);

/*--- global base board lifecycle constant data ----------------------------------------*/

fbe_lifecycle_const_t FBE_LIFECYCLE_CONST_DATA(base_port) = {
	FBE_LIFECYCLE_DEF_CONST_DATA(
		base_port,
		FBE_CLASS_ID_BASE_PORT,                        /* This class */
		FBE_LIFECYCLE_CONST_DATA(base_discovering))    /* The super class */
};

/*--- Local Functions ------------------------------------------------------------------*/

static void base_port_send_sfp_data_change_notification(fbe_base_port_t * base_port);

/*--- Condition Functions --------------------------------------------------------------*/

static fbe_lifecycle_status_t 
base_port_invalid_type_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_base_port_t * base_port = (fbe_base_port_t*)base_object;

	fbe_base_object_trace((fbe_base_object_t*)base_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
base_port_update_port_number_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_base_port_t * base_port = (fbe_base_port_t*)base_object;

	fbe_base_object_trace((fbe_base_object_t*)base_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

	/* Set the port info from the board. */
	status = fbe_base_port_set_port_info(base_port);
	if (status == FBE_STATUS_OK) {
		/* Clear the current condition. */
		status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_port);
		if (status != FBE_STATUS_OK) {
			fbe_base_object_trace((fbe_base_object_t*)base_port, 
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s can't clear current condition, status: 0x%X",
									__FUNCTION__, status);
		}
	}
	else {
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't set port info from the board, status: 0x%X",
								__FUNCTION__, status);
		/* Setting this condition causes a state change to DESTROY. */
		status = fbe_lifecycle_set_cond(&fbe_base_port_lifecycle_const,
										(fbe_base_object_t*)base_port,
										FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}



static fbe_lifecycle_status_t 
base_port_get_config_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
	fbe_status_t status; 
	fbe_base_port_t * base_port = (fbe_base_port_t*)p_object;
    fbe_port_number_t                   port_number;	
    fbe_port_connect_class_t            connect_class;
    fbe_port_role_t                     port_role;

    fbe_class_id_t                      port_class_id = FBE_CLASS_ID_BASE_PORT;
    fbe_cpd_shim_port_configuration_t   port_config;


	fbe_base_object_trace((fbe_base_object_t*)base_port,
	                      FBE_TRACE_LEVEL_DEBUG_HIGH,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

    
    (void)fbe_base_port_get_port_number(base_port, &port_number);
    fbe_zero_memory(&port_config,sizeof(port_config));
    status = fbe_cpd_shim_get_port_config_info(port_number, &port_config);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s COnfiguration info query error,status: 0x%X port 0x%X",
								__FUNCTION__, status,port_number);
		/* Setting this condition causes a state change to DESTROY. */
		status = fbe_lifecycle_set_cond(&fbe_base_port_lifecycle_const,
										(fbe_base_object_t*)base_port,
										FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
        return FBE_LIFECYCLE_STATUS_DONE;
    }

    
    connect_class = base_port_translate_port_connect_class(port_config.connect_class);
    port_role = base_port_translate_port_role(port_config.port_role);

    /* Store configuration information.*/
    fbe_base_port_set_port_connect_class(base_port,connect_class);
    fbe_base_port_set_port_role(base_port,port_role);
    fbe_base_port_set_assigned_bus_number(base_port, port_config.flare_bus_num);

    fbe_base_port_set_core_affinity_info(base_port, port_config.multi_core_affinity_enabled,
                                                    port_config.core_affinity_proc_mask);


    switch (connect_class){
        case FBE_PORT_CONNECT_CLASS_SAS:
            port_class_id = FBE_CLASS_ID_SAS_PORT;
            break;
        case FBE_PORT_CONNECT_CLASS_FC:
            port_class_id = FBE_CLASS_ID_FC_PORT;
            break;
        case FBE_PORT_CONNECT_CLASS_ISCSI:
            port_class_id = FBE_CLASS_ID_ISCSI_PORT;
            break;
        case FBE_PORT_CONNECT_CLASS_FCOE: // stay as base port
        default:
            port_class_id = FBE_CLASS_ID_BASE_PORT;
	        fbe_base_object_trace((fbe_base_object_t *)base_port, 
                                    FBE_TRACE_LEVEL_INFO, 
							        FBE_TRACE_MESSAGE_ID_INFO,
							        "Mapped to base port connect_class: 0x%X \n", connect_class);            
            break;

    }

    fbe_base_object_lock((fbe_base_object_t*)base_port);
    fbe_base_object_change_class_id((fbe_base_object_t*)base_port, port_class_id);
    fbe_base_object_unlock((fbe_base_object_t*)base_port);
    fbe_base_object_trace((fbe_base_object_t *)base_port, 
                            FBE_TRACE_LEVEL_INFO, 
                            FBE_TRACE_MESSAGE_ID_INFO,
                            "Set port class id: 0x%X \n", port_class_id);

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
base_port_unregister_config_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
	fbe_status_t 		status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_port_t * 	base_port = (fbe_base_port_t*)base_object;

    fbe_base_object_trace((fbe_base_object_t*)base_port,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);


	if(base_port->port_configuration_handle != FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE){
		fbe_drive_configuration_unregister_port (base_port->port_configuration_handle);
		base_port->port_configuration_handle = FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE;
	}

    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_port);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_port, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;

}

static fbe_lifecycle_status_t 
base_port_register_config_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
	fbe_status_t 								status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_port_t * 							base_port = (fbe_base_port_t*)base_object;
	fbe_drive_configuration_port_info_t			registration_info;

    fbe_base_object_trace((fbe_base_object_t*)base_port,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

	if(base_port->port_configuration_handle == FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE){
		    /*TODO - this is where you populate registration_info*/
			registration_info.port_type = FBE_PORT_TYPE_SAS_PMC;/*FIXME - HACK !~!!!*/

			fbe_drive_configuration_register_port (&registration_info, &base_port->port_configuration_handle);		
	}

	if(base_port->port_configuration_handle != FBE_DRIVE_CONFIGURATION_SERVICE_INVALID_HANLDLE){
		status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_port);
		if (status != FBE_STATUS_OK) {
			fbe_base_object_trace((fbe_base_object_t*)base_port, 
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s can't clear current condition, status: 0x%X",
									__FUNCTION__, status);
		}
	}
    return FBE_LIFECYCLE_STATUS_DONE;

}

static fbe_lifecycle_status_t 
base_port_init_miniport_shim_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
	fbe_status_t 								status = FBE_STATUS_GENERIC_FAILURE;
    fbe_base_port_t * 							base_port = (fbe_base_port_t*)base_object;	
	fbe_port_number_t port_number;	
    fbe_u32_t	io_port_number,io_portal_number;

    fbe_base_object_trace((fbe_base_object_t*)base_port,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

	fbe_base_port_get_io_port_number((fbe_base_port_t *)base_object, &io_port_number);
	fbe_base_port_get_io_portal_number((fbe_base_port_t *)base_object, &io_portal_number);	
	status = fbe_cpd_shim_port_init(io_port_number, io_portal_number,&port_number);
	if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t *) base_object, 
                              FBE_TRACE_LEVEL_WARNING,
                              FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                              "%s Registration with shim failed!!!\n. status: 0x%X", __FUNCTION__, FBE_STATUS_INSUFFICIENT_RESOURCES);

		status = fbe_lifecycle_set_cond(&fbe_base_port_lifecycle_const, 
								(fbe_base_object_t*)base_port, 
								FBE_BASE_PORT_LIFECYCLE_COND_DESTROY_MINIPORT_SHIM);
		if (status != FBE_STATUS_OK) {
			fbe_base_object_trace((fbe_base_object_t*)base_object, 
									FBE_TRACE_LEVEL_ERROR,
									FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
									"%s can't set destroy miniport shim condition, status: 0x%X",
									__FUNCTION__, status);
		}		

		return FBE_LIFECYCLE_STATUS_DONE;
    }

	fbe_base_port_set_port_number((fbe_base_port_t *)base_object, port_number);


	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

    return FBE_LIFECYCLE_STATUS_DONE;
}


static fbe_lifecycle_status_t 
base_port_destroy_miniport_shim_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_base_port_t * 							base_port = (fbe_base_port_t*)base_object;	
	fbe_port_number_t port_number;

	fbe_base_object_trace(	(fbe_base_object_t*)base_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry\n", __FUNCTION__);

	
	status = fbe_base_port_get_port_number((fbe_base_port_t *)base_port, &port_number);

	status = fbe_cpd_shim_port_destroy(port_number); 

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
base_port_get_hardware_info_cond_function(fbe_base_object_t * base_object, fbe_packet_t * packet)
{
	fbe_status_t status;
	fbe_base_port_t * 				base_port = (fbe_base_port_t*)base_object;	
	fbe_port_number_t               port_number;
    fbe_cpd_shim_hardware_info_t    hdw_info;
    fbe_port_hardware_info_t        port_hardware_info;

	fbe_base_object_trace(	(fbe_base_object_t*)base_port,
							FBE_TRACE_LEVEL_DEBUG_HIGH,
							FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
							"%s entry\n", __FUNCTION__);

	
	(void)fbe_base_port_get_port_number(base_port, &port_number);

	status = fbe_cpd_shim_get_hardware_info(port_number,&hdw_info);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s Hardware info query error,status: 0x%X port 0x%X",
								__FUNCTION__, status,port_number);
		/* Setting this condition causes a state change to DESTROY. */
		status = fbe_lifecycle_set_cond(&fbe_base_port_lifecycle_const,
										(fbe_base_object_t*)base_port,
										FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
    }else{

        /* Convert shim information to port information.*/
        (void)base_port_convert_hardware_info(&hdw_info,&port_hardware_info);

        /* Store information.*/
        (void)fbe_base_port_set_hardware_info(base_port,&port_hardware_info);

	    /* Clear the current condition. */
	    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_port);
	    if (status != FBE_STATUS_OK) {
		    fbe_base_object_trace((fbe_base_object_t*)base_port, 
								    FBE_TRACE_LEVEL_ERROR,
								    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								    "%s can't clear current condition, status: 0x%X",
								    __FUNCTION__, status);
	    }
    }

	return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_lifecycle_status_t 
base_port_sfp_information_change_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
    {
    fbe_status_t status;
    fbe_base_port_t * 				base_port = (fbe_base_port_t*)p_object;	
    fbe_port_number_t               port_number;
    fbe_cpd_shim_sfp_media_interface_info_t media_interface_info = {0};
    fbe_port_sfp_info_t             port_sfp_info = {0};
    
    fbe_base_object_trace(	(fbe_base_object_t*)base_port,
                            FBE_TRACE_LEVEL_DEBUG_HIGH,
                            FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                            "%s entry\n", __FUNCTION__);
    
    
    (void)fbe_base_port_get_port_number(base_port, &port_number);
    
    status = fbe_cpd_shim_get_media_inteface_info(port_number, FBE_CPD_SHIM_MEDIA_INTERFACE_INFO_TYPE_HIGHEST,
                                                    &media_interface_info);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_port, 
                                FBE_TRACE_LEVEL_DEBUG_HIGH,
                                FBE_TRACE_MESSAGE_ID_INFO,
                                "%s SFP info query error,status: 0x%X port 0x%X",
                                __FUNCTION__, status,port_number);
        /* This is not a fatal error at this point. Trace and continue.*/
        /* Setting this condition causes a state change to DESTROY. 
        status = fbe_lifecycle_set_cond(&fbe_base_port_lifecycle_const,
                                        (fbe_base_object_t*)base_port,
                                        FBE_BASE_OBJECT_LIFECYCLE_COND_DESTROY);
        */
    }else{
    
        /* Convert shim information to port information.*/
        (void)base_port_convert_sfp_info(&media_interface_info,&port_sfp_info);
    
        /* Store information.*/
        (void)fbe_base_port_set_sfp_info(base_port,&port_sfp_info);
    
        /* Notify ESP about SFP change. */
        base_port_send_sfp_data_change_notification(base_port);
        /* Clear the current condition. */
        status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_port);
        if (status != FBE_STATUS_OK) {
            fbe_base_object_trace((fbe_base_object_t*)base_port, 
                                    FBE_TRACE_LEVEL_ERROR,
                                    FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                    "%s can't clear current condition, status: 0x%X",
                                    __FUNCTION__, status);
        }
    }
    
    return FBE_LIFECYCLE_STATUS_DONE;
}

static fbe_status_t
base_port_convert_hardware_info(fbe_cpd_shim_hardware_info_t *hdw_info, fbe_port_hardware_info_t *port_hardware_info)
{
    if ((hdw_info == NULL) || (port_hardware_info == NULL)){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    port_hardware_info->pci_bus = hdw_info->bus;
    port_hardware_info->pci_slot = hdw_info->slot;
    port_hardware_info->pci_function = hdw_info->function;
    port_hardware_info->vendor_id = hdw_info->vendor;
    port_hardware_info->device_id = hdw_info->device;

    port_hardware_info->hw_major_rev = hdw_info->hw_major_rev;
    port_hardware_info->hw_minor_rev = hdw_info->hw_minor_rev;
    port_hardware_info->firmware_rev_1 = hdw_info->firmware_rev_1;
    port_hardware_info->firmware_rev_2 = hdw_info->firmware_rev_2;
    port_hardware_info->firmware_rev_3 = hdw_info->firmware_rev_3;
    port_hardware_info->firmware_rev_4 = hdw_info->firmware_rev_4;

    return FBE_STATUS_OK;
}

static fbe_status_t
base_port_convert_sfp_info(fbe_cpd_shim_sfp_media_interface_info_t *media_interface_info,
                           fbe_port_sfp_info_t                     *port_sfp_info)
{
    fbe_port_sfp_condition_type_t       port_sfp_condition = FBE_PORT_SFP_CONDITION_INVALID;
    fbe_port_sfp_sub_condition_type_t   port_sfp_subcondition = FBE_PORT_SFP_SUBCONDITION_NONE;
    fbe_port_sfp_media_type_t           port_sfp_media_type = FBE_PORT_SFP_MEDIA_TYPE_INVALID;
    fbe_u32_t                           string_size = 0;

    if ((media_interface_info == NULL) || (port_sfp_info == NULL)){
        return FBE_STATUS_GENERIC_FAILURE;
    }

    switch(media_interface_info->condition_type){
        case FBE_CPD_SHIM_SFP_CONDITION_GOOD:
            port_sfp_condition = FBE_PORT_SFP_CONDITION_GOOD;
            break;
        case FBE_CPD_SHIM_SFP_CONDITION_INSERTED:
            port_sfp_condition = FBE_PORT_SFP_CONDITION_INSERTED;
            break;
        case FBE_CPD_SHIM_SFP_CONDITION_REMOVED:
            port_sfp_condition = FBE_PORT_SFP_CONDITION_REMOVED;
            break;
        case FBE_CPD_SHIM_SFP_CONDITION_FAULT:
            port_sfp_condition = FBE_PORT_SFP_CONDITION_FAULT;
            break;
        case FBE_CPD_SHIM_SFP_CONDITION_WARNING:
            port_sfp_condition = FBE_PORT_SFP_CONDITION_WARNING;
            break;
        case FBE_CPD_SHIM_SFP_CONDITION_INFO:
            port_sfp_condition = FBE_PORT_SFP_CONDITION_INFO;
            break;

    }
    port_sfp_info->condition_type = port_sfp_condition;

    switch(media_interface_info->condition_additional_info)
    {
        case FBE_CPD_SHIM_SFP_DIAG_TXFAULT:
        case FBE_CPD_SHIM_SFP_DIAG_TEMP_HI_ALARM:
        case FBE_CPD_SHIM_SFP_DIAG_TEMP_LO_ALARM:
        case FBE_CPD_SHIM_SFP_DIAG_VCC_HI_ALARM:
        case FBE_CPD_SHIM_SFP_DIAG_VCC_LO_ALARM:
        case FBE_CPD_SHIM_SFP_DIAG_TX_BIAS_HI_ALARM:
        case FBE_CPD_SHIM_SFP_DIAG_TX_BIAS_LO_ALARM:
        case FBE_CPD_SHIM_SFP_DIAG_TX_POWER_HI_ALARM:
        case FBE_CPD_SHIM_SFP_DIAG_TX_POWER_LO_ALARM:
        case FBE_CPD_SHIM_SFP_DIAG_RX_POWER_HI_ALARM:
        case FBE_CPD_SHIM_SFP_DIAG_RX_POWER_LO_ALARM:
        case FBE_CPD_SHIM_SFP_DIAG_TEMP_HI_WARNING:
        case FBE_CPD_SHIM_SFP_DIAG_TEMP_LO_WARNING:
        case FBE_CPD_SHIM_SFP_DIAG_VCC_HI_WARNING:
        case FBE_CPD_SHIM_SFP_DIAG_VCC_LO_WARNING:
        case FBE_CPD_SHIM_SFP_DIAG_TX_BIAS_HI_WARNING:
        case FBE_CPD_SHIM_SFP_DIAG_TX_BIAS_LO_WARNING:
        case FBE_CPD_SHIM_SFP_DIAG_TX_POWER_HI_WARNING:
        case FBE_CPD_SHIM_SFP_DIAG_TX_POWER_LO_WARNING:
        case FBE_CPD_SHIM_SFP_DIAG_RX_POWER_HI_WARNING:
        case FBE_CPD_SHIM_SFP_DIAG_RX_POWER_LO_WARNING:
            port_sfp_subcondition = FBE_PORT_SFP_SUBCONDITION_GOOD;
            break;

        case FBE_CPD_SHIM_SFP_INFO_RECHECK_MII:
        case FBE_CPD_SHIM_SFP_INFO_SPD_LEN_AVAIL:
            port_sfp_subcondition = FBE_PORT_SFP_SUBCONDITION_CHECKSUM_PENDING;
            break;
    
        case FBE_CPD_SHIM_SFP_INFO_BAD_EMC_CHKSUM:
        case FBE_CPD_SHIM_SFP_BAD_CHKSUM:
        case FBE_CPD_SHIM_SFP_BAD_I2C:
        case FBE_CPD_SHIM_SFP_DEV_ERR:
            port_sfp_subcondition = FBE_PORT_SFP_SUBCONDITION_DEVICE_ERROR;
            break;
    
        case FBE_CPD_SHIM_SFP_UNQUAL_OPT_NOT_4G:
        case FBE_CPD_SHIM_SFP_UNQUAL_COP_AUTO:
        case FBE_CPD_SHIM_SFP_UNQUAL_COP_SFP_SPEED:
        case FBE_CPD_SHIM_SFP_UNQUAL_SPEED_EXCEED_SFP:
        case FBE_CPD_SHIM_SFP_UNQUAL_PART:
        case FBE_CPD_SHIM_SFP_UNKNOWN_TYPE:
        case FBE_CPD_SHIM_SFP_SPEED_MISMATCH:
        case FBE_CPD_SHIM_SFP_EDC_MODE_MISMATCH:
        case FBE_CPD_SHIM_SFP_SAS_SPECL_ERROR:
            port_sfp_subcondition = FBE_PORT_SFP_SUBCONDITION_UNSUPPORTED;
            break;
    
        case FBE_CPD_SHIM_SFP_NONE:
        default:
            port_sfp_subcondition = FBE_PORT_SFP_SUBCONDITION_NONE;
            break;
    }

    port_sfp_info->condition_additional_info = port_sfp_subcondition;
    port_sfp_info->speeds                    = media_interface_info->speeds;
    port_sfp_info->hw_type                   = media_interface_info->hw_type;
    port_sfp_info->cable_length              = media_interface_info->cable_length;

    switch(media_interface_info->media_type){
        case FBE_CPD_SHIM_SFP_MEDIA_TYPE_COPPER:
            port_sfp_media_type = FBE_PORT_SFP_MEDIA_TYPE_COPPER;
            break;
        case FBE_CPD_SHIM_SFP_MEDIA_TYPE_OPTICAL:
            port_sfp_media_type = FBE_PORT_SFP_MEDIA_TYPE_OPTICAL;
            break;
        case FBE_CPD_SHIM_SFP_MEDIA_TYPE_NAS_COPPER:
            port_sfp_media_type = FBE_PORT_SFP_MEDIA_TYPE_NAS_COPPER;
            break;
        case FBE_CPD_SHIM_SFP_MEDIA_TYPE_UNKNOWN:
            port_sfp_media_type = FBE_PORT_SFP_MEDIA_TYPE_UNKNOWN;
            break;
        case FBE_CPD_SHIM_SFP_MEDIA_TYPE_MINI_SAS_HD:
            port_sfp_media_type = FBE_PORT_SFP_MEDIA_TYPE_MINI_SAS_HD;
            break;
    }
    port_sfp_info->media_type = port_sfp_media_type;

    string_size = CSX_MIN(FBE_CPD_SHIM_SFP_EMC_DATA_LENGTH,FBE_PORT_SFP_EMC_DATA_LENGTH);
    fbe_copy_memory(port_sfp_info->emc_part_number,media_interface_info->emc_part_number,string_size);
    fbe_copy_memory(port_sfp_info->emc_part_revision,media_interface_info->emc_part_revision,string_size);
    fbe_copy_memory(port_sfp_info->emc_serial_number,media_interface_info->emc_serial_number,string_size);

    string_size = CSX_MIN(FBE_CPD_SHIM_SFP_VENDOR_DATA_LENGTH,FBE_PORT_SFP_VENDOR_DATA_LENGTH);
    fbe_copy_memory(port_sfp_info->vendor_part_number,media_interface_info->vendor_part_number,string_size);
    fbe_copy_memory(port_sfp_info->vendor_part_revision,media_interface_info->vendor_part_revision,string_size);
    fbe_copy_memory(port_sfp_info->vendor_serial_number,media_interface_info->vendor_serial_number,string_size);
    
    return FBE_STATUS_OK;
}

static fbe_port_connect_class_t 
base_port_translate_port_connect_class(fbe_cpd_shim_connect_class_t cpd_shim_port_type)
{
    fbe_port_connect_class_t port_type = FBE_PORT_CONNECT_CLASS_INVALID;

    switch(cpd_shim_port_type){
        case FBE_CPD_SHIM_CONNECT_CLASS_SAS:
            port_type = FBE_PORT_CONNECT_CLASS_SAS;
            break;
        case FBE_CPD_SHIM_CONNECT_CLASS_FC:
            port_type = FBE_PORT_CONNECT_CLASS_FC;
            break;
        case FBE_CPD_SHIM_CONNECT_CLASS_ISCSI:
            port_type = FBE_PORT_CONNECT_CLASS_ISCSI;
            break;
        case FBE_CPD_SHIM_CONNECT_CLASS_FCOE:
            port_type = FBE_PORT_CONNECT_CLASS_FCOE;
            break;
    }

    return port_type;
}
static fbe_port_role_t 
base_port_translate_port_role(fbe_cpd_shim_port_role_t cpd_shim_port_role)
{
    fbe_port_role_t  port_role = FBE_PORT_ROLE_INVALID;

    switch (cpd_shim_port_role){
        case FBE_CPD_SHIM_PORT_ROLE_FE:
            port_role = FBE_PORT_ROLE_FE;
            break;
        case FBE_CPD_SHIM_PORT_ROLE_BE:
            port_role = FBE_PORT_ROLE_BE;
            break;
        case FBE_CPD_SHIM_PORT_ROLE_UNC:
            port_role = FBE_PORT_ROLE_UNC;
            break;
        case FBE_CPD_SHIM_PORT_ROLE_BOOT:
            port_role = FBE_PORT_ROLE_BOOT;
            break;
        case FBE_CPD_SHIM_PORT_ROLE_SPECIAL:
            port_role = FBE_PORT_ROLE_SPECIAL;
            break;
    }

    return port_role;
}

/*!*************************************************************************
* @fn base_port_send_sfp_data_change_notification(                  
*                    fbe_base_port_t * base_port)                  
***************************************************************************
*
* @brief
*       Notigy ESP that LINK status has changed. 
*
*
* @param      p_object - The pointer to the fbe_base_object_t.
*
*
* NOTES
*
* HISTORY
*
***************************************************************************/

static void 
base_port_send_sfp_data_change_notification(fbe_base_port_t * base_port)
{
    fbe_notification_info_t     notification;
	fbe_port_hardware_info_t    port_hardware_info;
	fbe_notification_data_changed_info_t * data_change_info;
    fbe_object_id_t             object_id;
    
    fbe_zero_memory(&notification,sizeof(notification));
    fbe_zero_memory(&port_hardware_info,sizeof(port_hardware_info));
    fbe_base_object_get_object_id((fbe_base_object_t *)base_port, &object_id);

	fbe_base_port_get_hardware_info(base_port, &port_hardware_info);

    notification.notification_type = FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED;
    notification.class_id = FBE_CLASS_ID_BASE_PORT;
    notification.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PORT;    

	/* TODO: Confirm these values. */
	data_change_info = &(notification.notification_data.data_change_info);

	data_change_info->phys_location.bus = port_hardware_info.pci_bus;
	data_change_info->phys_location.enclosure = 0;
	data_change_info->phys_location.slot = port_hardware_info.pci_slot;
	data_change_info->phys_location.sp = 0;/*???*/
	data_change_info->phys_location.port = object_id;/*???*/

	data_change_info->device_mask = FBE_DEVICE_TYPE_SFP;
	data_change_info->data_type = FBE_DEVICE_DATA_TYPE_GENERIC_INFO;

    fbe_base_object_trace((fbe_base_object_t *)base_port,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, bus %d, encl %d, slot %d, deviceMask 0x%llX.\n", 
                          __FUNCTION__, 
                          data_change_info->phys_location.bus,
                          data_change_info->phys_location.enclosure,
                          data_change_info->phys_location.slot,
                          data_change_info->device_mask);

    fbe_notification_send(object_id, notification);
    return;

}


/*!*************************************************************************
* @fn base_port_get_port_data_cond_function(                  
*                    fbe_base_object_t * p_object, 
*                    fbe_packet_t * packet)                  
***************************************************************************
*
* @brief
*       This condition function handles link degraded condition. 
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
base_port_get_port_data_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
    fbe_status_t status; 
    fbe_base_port_t * base_port = (fbe_base_port_t*)p_object;
    fbe_port_connect_class_t        connect_class = FBE_PORT_CONNECT_CLASS_INVALID;
    fbe_cpd_shim_connect_class_t    cpd_connect_class = FBE_CPD_SHIM_CONNECT_CLASS_INVALID;
    fbe_cpd_shim_port_lane_info_t   cpd_port_lane_info;
    fbe_port_link_information_t     port_link_info;
    fbe_u32_t                       port_handle;

    fbe_base_object_trace((fbe_base_object_t*)base_port,
                          FBE_TRACE_LEVEL_DEBUG_HIGH,
                          FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
                          "%s entry\n", __FUNCTION__);

    status = fbe_base_port_get_port_number((fbe_base_port_t *)base_port, &port_handle);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_port, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Error getting port number, status: 0x%X",
                                __FUNCTION__, status);
        }

    status = fbe_base_port_get_port_connect_class((fbe_base_port_t *)base_port, &connect_class);
    if (status == FBE_STATUS_OK) {        
        switch(connect_class){
            case FBE_PORT_CONNECT_CLASS_SAS:
                cpd_connect_class = FBE_CPD_SHIM_CONNECT_CLASS_SAS;
                break;
            case FBE_PORT_CONNECT_CLASS_FC:
                cpd_connect_class = FBE_CPD_SHIM_CONNECT_CLASS_FC;
                break;
            case FBE_PORT_CONNECT_CLASS_ISCSI:
                cpd_connect_class = FBE_CPD_SHIM_CONNECT_CLASS_ISCSI;
                break;
                
            case FBE_PORT_CONNECT_CLASS_FCOE:
                cpd_connect_class = FBE_CPD_SHIM_CONNECT_CLASS_FCOE;
                break;
                
            default:
                cpd_connect_class = FBE_CPD_SHIM_CONNECT_CLASS_INVALID;
                break;
        };
    }else{
        fbe_base_object_trace((fbe_base_object_t*)base_port, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Error getting port connect class, status: 0x%X",
                                __FUNCTION__, status);
     }
    /* Retrieve information from shim/miniport about the lane. */
    status = fbe_cpd_shim_get_port_lane_info(port_handle, cpd_connect_class, &cpd_port_lane_info);
    if (status == FBE_STATUS_OK) {    
        /* Update locally cached information. */
        (void)base_port_convert_shim_link_info(&cpd_port_lane_info, cpd_connect_class, &port_link_info);
        port_link_info.port_connect_class = connect_class;
        (void)fbe_base_port_set_link_info(base_port, &port_link_info);

        /* Notify ESP about the change. */
        base_port_send_link_data_change_notification(base_port);
    }else{
        fbe_base_object_trace((fbe_base_object_t*)base_port, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s Error querying port lane info, status: 0x%X",
                                __FUNCTION__, status);        
     }


    /* Clear the current condition. */
    status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_port);
    if (status != FBE_STATUS_OK) {
        fbe_base_object_trace((fbe_base_object_t*)base_port, 
                                FBE_TRACE_LEVEL_ERROR,
                                FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
                                "%s can't clear current condition, status: 0x%X",
                                __FUNCTION__, status);
    }

    return FBE_LIFECYCLE_STATUS_DONE;
}

/*!*************************************************************************
* @fn base_port_send_link_data_change_notification(                  
*                    fbe_base_port_t * base_port)                  
***************************************************************************
*
* @brief
*       Notigy ESP that LINK status has changed. 
*
*
* @param      p_object - The pointer to the fbe_base_object_t.
*
*
* NOTES
*
* HISTORY
*
***************************************************************************/

static void 
base_port_send_link_data_change_notification(fbe_base_port_t * base_port)
{
    fbe_notification_info_t     notification;
    fbe_port_hardware_info_t    port_hardware_info;
    fbe_notification_data_changed_info_t * data_change_info;
    fbe_object_id_t             object_id;
    
    fbe_zero_memory(&notification,sizeof(notification));
    fbe_zero_memory(&port_hardware_info,sizeof(port_hardware_info));

    fbe_base_object_get_object_id((fbe_base_object_t *)base_port, &object_id);

    fbe_base_port_get_hardware_info((fbe_base_port_t *) base_port, &port_hardware_info);

    notification.notification_type = FBE_NOTIFICATION_TYPE_OBJECT_DATA_CHANGED;
    notification.class_id = FBE_CLASS_ID_BASE_PORT;
    notification.object_type = FBE_TOPOLOGY_OBJECT_TYPE_PORT;    

    /* TODO: Confirm these values. */
    data_change_info = &(notification.notification_data.data_change_info);

    data_change_info->phys_location.bus = port_hardware_info.pci_bus;
    data_change_info->phys_location.enclosure = 0;
    data_change_info->phys_location.slot = port_hardware_info.pci_slot;
    data_change_info->phys_location.sp = 0;/*???*/
    data_change_info->phys_location.port = object_id;

    data_change_info->device_mask = FBE_DEVICE_TYPE_PORT_LINK;
    data_change_info->data_type = FBE_DEVICE_DATA_TYPE_PORT_INFO;

    fbe_base_object_trace((fbe_base_object_t *)base_port,
                          FBE_TRACE_LEVEL_DEBUG_LOW,
                          FBE_TRACE_MESSAGE_ID_INFO,
                          "%s, bus %d, encl %d, slot %d, deviceMask 0x%llX.\n", 
                          __FUNCTION__, 
                          data_change_info->phys_location.bus,
                          data_change_info->phys_location.enclosure,
                          data_change_info->phys_location.slot,
                          data_change_info->device_mask);

    fbe_notification_send(object_id, notification);
    return;

}
static fbe_status_t
base_port_convert_shim_link_info(fbe_cpd_shim_port_lane_info_t   *cpd_port_lane_info,
                                 fbe_cpd_shim_connect_class_t    cpd_connect_class,
                                 fbe_port_link_information_t  *port_link_info){

    port_link_info->portal_number    =   cpd_port_lane_info->portal_number;
    port_link_info->link_speed       =   cpd_port_lane_info->link_speed;
    switch(cpd_port_lane_info->link_state){
        case CPD_SHIM_PORT_LINK_STATE_UP:
            port_link_info->link_state = FBE_PORT_LINK_STATE_UP;
            break;
        case CPD_SHIM_PORT_LINK_STATE_DOWN:
            port_link_info->link_state = FBE_PORT_LINK_STATE_DOWN;
            break;
        case CPD_SHIM_PORT_LINK_STATE_DEGRADED:
            port_link_info->link_state = FBE_PORT_LINK_STATE_DEGRADED;
            break;
        case CPD_SHIM_PORT_LINK_STATE_NOT_INITIALIZED:
            port_link_info->link_state = FBE_PORT_LINK_STATE_NOT_INITIALIZED;
            break;
        default:
            port_link_info->link_state = FBE_PORT_LINK_STATE_INVALID;
            break;
    }

    /* SAS specific. */
    if (cpd_connect_class == FBE_CPD_SHIM_CONNECT_CLASS_SAS){
        port_link_info->u.sas_port.nr_phys          =   cpd_port_lane_info->nr_phys;
        port_link_info->u.sas_port.phy_map          =   cpd_port_lane_info->phy_map;
        port_link_info->u.sas_port.nr_phys_enabled  =   cpd_port_lane_info->nr_phys_enabled;
        port_link_info->u.sas_port.nr_phys_up       =   cpd_port_lane_info->nr_phys_up;    
    }


    return FBE_STATUS_OK;
}

/*!*************************************************************************
* @fn base_port_register_callbacks_cond_function(                  
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
base_port_register_callbacks_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
	fbe_status_t status; 
	fbe_base_port_t * base_port = (fbe_base_port_t*)p_object;
   	fbe_u32_t port_handle;

	fbe_base_object_trace((fbe_base_object_t*)base_port,
	                      FBE_TRACE_LEVEL_INFO,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

    /* Register callback with shim.*/
	status = fbe_base_port_get_port_number((fbe_base_port_t *)base_port, &port_handle);
	status = fbe_cpd_shim_port_register_callback(port_handle,                                                 
                                                 (FBE_CPD_SHIM_REGISTER_CALLBACKS_INITIATOR | FBE_CPD_SHIM_REGISTER_CALLBACKS_SFP_EVENTS),
                                                 fbe_base_port_callback, base_port);
	if (status == FBE_STATUS_OK) {		
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_INFO,
								FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Registered with PMC Shim. Status : 0x%X",
								__FUNCTION__, status);		

	}

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}

/*!*************************************************************************
* @fn base_port_unregister_callbacks_cond_function(                  
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
base_port_unregister_callbacks_cond_function(fbe_base_object_t * p_object, fbe_packet_t * packet)
{
	fbe_status_t status; 
	fbe_base_port_t * base_port = (fbe_base_port_t*)p_object;
   	fbe_port_number_t port_number;


	fbe_base_object_trace((fbe_base_object_t*)base_port,
	                      FBE_TRACE_LEVEL_INFO,
	                      FBE_TRACE_MESSAGE_ID_FUNCTION_ENTRY,
	                      "%s entry\n", __FUNCTION__);

    /* Unregister callback with shim.*/
	status = fbe_base_port_get_port_number((fbe_base_port_t *)base_port, &port_number);
	status = fbe_cpd_shim_port_unregister_callback(port_number);
	if (status == FBE_STATUS_OK) {		
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_DEBUG_HIGH,
								FBE_TRACE_MESSAGE_ID_INFO,
                                "%s Registered with PMC Shim. Status : 0x%X",
								__FUNCTION__, status);
	}

	/* Clear the current condition. */
	status = fbe_lifecycle_clear_current_cond((fbe_base_object_t*)base_port);
	if (status != FBE_STATUS_OK) {
		fbe_base_object_trace((fbe_base_object_t*)base_port, 
								FBE_TRACE_LEVEL_ERROR,
								FBE_TRACE_MESSAGE_ID_FUNCTION_FAILED,
								"%s can't clear current condition, status: 0x%X",
								__FUNCTION__, status);
	}

	return FBE_LIFECYCLE_STATUS_DONE;
}

